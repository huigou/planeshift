/*
* updaterengine.cpp - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2 of the License)
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#include <csutil/csmd5.h>
#include <csutil/hash.h>
#include <csutil/xmltiny.h>
#include <iutil/stringarray.h>

#include "updaterconfig.h"
#include "updaterengine.h"
#include "binarypatch.h"

#ifndef CS_COMPILER_MSVC
#include <unistd.h>
#endif

iObjectRegistry* UpdaterEngine::object_reg = NULL;

UpdaterEngine::UpdaterEngine(const csArray<csString> args, iObjectRegistry* object_reg, const char* appName)
{
    InfoShare *is = new InfoShare();
    hasGUI = false;
    Init(args, object_reg, appName, is);
}

UpdaterEngine::UpdaterEngine(const csArray<csString> args, iObjectRegistry* object_reg, const char* appName,
                             InfoShare *infoshare)
{
    hasGUI = true;
    Init(args, object_reg, appName, infoshare);
}

void UpdaterEngine::Init(const csArray<csString> args, iObjectRegistry* _object_reg, const char* _appName,
                         InfoShare* _infoShare)
{
    object_reg = _object_reg;
    vfs = csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
    {
        printf("No VFS!\n");
        exit(1);
    }
    vfs->ChDir("/this/");
    config = new UpdaterConfig(args, object_reg, vfs);
    fileUtil = new FileUtil(vfs);
    appName = _appName;
    infoShare = _infoShare;

    if(vfs->Exists("/this/updater.log"))
    {
        fileUtil->RemoveFile("/this/updater.log");
    }
    log = vfs->Open("/this/updater.log", VFS_FILE_WRITE);
}

UpdaterEngine::~UpdaterEngine()
{
    log = NULL;
    delete fileUtil;
    delete config;
    fileUtil = NULL;
    config = NULL;
    if(!hasGUI)
    {
        delete infoShare;
    }
}

void UpdaterEngine::PrintOutput(const char *string, ...)
{
    csString outputString;
    va_list args;
    va_start (args, string);
    outputString.FormatV (string, args);
    va_end (args);
    infoShare->ConsolePush(outputString);
    printf("%s", outputString.GetData());

    if(log.IsValid())
    {
        log->Write(outputString.GetData(), outputString.Length());
    }    
}

void UpdaterEngine::CheckForUpdates()
{
    if(!log.IsValid())
    {
        if(appName.Compare("psupdater"))
        {
            printf("This program requires admin/root or write privileges to run. Please restart the program with these.\n");
            printf("In windows, this means right clicking the Updater shortcut and selecting \"Run as administrator\".\n");
            printf("For everyone else, login as the root user. If you don't have root access, contact someone who does and ask them nicely to run the updater!\n");
        }
        else
        {
            /*
             * This will need fixing for pslaunch somehow. We don't want to need admin rights until we know we need to update...
             * Maybe write updaterinfo.xml to the userdata path instead of the install path, then execute a new thread which
             * needs admin if we don't have it.. for UAC anyway. For linux, we can think of something else (maybe just print a message in the pslaunch
             * window like we do to the console in psupdater.
             */
        }
    }
    else
    {

        // Make sure the old instance had time to terminate (self-update).
        if(config->IsSelfUpdating())
            csSleep(1000);

        // Check if the last attempt at a general updated ended in failure.
        if(!config->WasCleanUpdate())
        {
            // Safety check.
            if(!vfs->Exists("/this/updaterinfo.xml.bak"))
            {
                config->GetConfigFile()->SetBool("Update.Clean", true);
                config->GetConfigFile()->Save();
            }
            else
            {
                // Restore config file which gives us the last updated position.
                fileUtil->RemoveFile("/this/updaterinfo.xml");
                fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);
            }
        }


        // Load current config data.
        csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_FILENAME);
        if(!root)
        {
            PrintOutput("Unable to get root node\n");
            return;
        }

        csRef<iDocumentNode> confignode = root->GetNode("config");
        if (!confignode)
        {
            PrintOutput("Couldn't find config node in configfile!\n");
            return;
        }

        if(!confignode->GetAttributeValueAsBool("active", true))
        {
            PrintOutput("This updaterinfo.xml file is marked as inactive and will not work. Please get another one.\n");
            return;
        }

        // Load updater config
        if (!config->GetCurrentConfig()->Initialize(confignode))
        {
            PrintOutput("Failed to Initialize mirror config current!\n");
            return;
        }

        // Initialise downloader.
        downloader = new Downloader(GetVFS(), config);

        // Set proxy
        downloader->SetProxy(GetConfig()->GetProxy().host.GetData(),
            GetConfig()->GetProxy().port);

        PrintOutput("Checking for updates to the updater: ");

        if(CheckUpdater())
        {
            PrintOutput("Update Available!\n");

            // Restore config files.
            fileUtil->RemoveFile("/this/updaterinfo.xml");
            fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);

            // If using a GUI, prompt user whether or not to update.
            if(!appName.Compare("psupdater"))
            {
                infoShare->SetUpdateNeeded(true);         
                while(!infoShare->GetPerformUpdate() || !infoShare->GetExitGUI())
                {
                    csSleep(500);
                    // Make sure we die if we exit the gui as well.
                    if(!infoShare->GetUpdateNeeded() || infoShare->GetExitGUI())
                    {
                        delete downloader;
                        downloader = NULL;
                        return;
                    }

                    // If we're going to self-update, close the GUI.
                    if(infoShare->GetPerformUpdate())
                    {
                        infoShare->SetExitGUI(true);
                    }
                }
            }

            // Begin the self update process.
            SelfUpdate(false);
            return;
        }

        if(!vfs->Exists("/this/updaterinfo.xml.bak"))
            return;

        PrintOutput("No updates needed!\nChecking for updates to all files: ");

        // Check for normal updates.
        if(CheckGeneral())
        {
            PrintOutput("Updates Available!\n");
            // Mark update as incomplete.
            config->GetConfigFile()->SetBool("Update.Clean", false);
            config->GetConfigFile()->Save();

            // If using a GUI, prompt user whether or not to update.
            if(!appName.Compare("psupdater"))
            {
                infoShare->SetUpdateNeeded(true);
                while(!infoShare->GetPerformUpdate())
                {
                    csSleep(500);
                    if(!infoShare->GetUpdateNeeded())
                    {
                        delete downloader;
                        downloader = NULL;
                        return;
                    }
                }
            }

            // Begin general update.
            GeneralUpdate();

            // Maybe this fixes a bug.
            fflush(stdout);

            // Mark update as complete and clean up.
            config->GetConfigFile()->SetBool("Update.Clean", true);
            config->GetConfigFile()->Save();
            PrintOutput("Update finished!\n");
        }
        else
            PrintOutput("No updates needed!\n");

        delete downloader;
        downloader = NULL;
        infoShare->SetUpdateNeeded(false);
    }

    return;
}

bool UpdaterEngine::CheckUpdater()
{
    // Backup old config, download new.
    fileUtil->MoveFile("/this/updaterinfo.xml", "/this/updaterinfo.xml.bak", true, false);

    bool failed = false;
    
    if(!downloader->DownloadFile("updaterinfo.xml", "updaterinfo.xml", false, true))
        failed = true;

    // Load new config data.
    if(!failed)
    {
        csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_FILENAME);
        if(!root)
        {
            PrintOutput("Unable to get root node!\n");
            failed = true;
        }

        if(!failed)
        {
            csRef<iDocumentNode> confignode = root->GetNode("config");
            if (!confignode)
            {
                PrintOutput("Couldn't find config node in configfile!\n");
                failed = true;
            }

            if(!failed && !confignode->GetAttributeValueAsBool("active", true))
            {
                PrintOutput("The updater mirrors are down, possibly for an update. Please try again later.\n");
                failed = true;
            }

            if (!failed && !config->GetNewConfig()->Initialize(confignode))
            {
                PrintOutput("Failed to Initialize mirror config new!\n");
                failed = true;
            }
        }
    }

    if(failed)
    {
        fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);
        return false;
    }

    // Compare Versions.
    return(config->UpdatePlatform() && config->GetNewConfig()->GetUpdaterVersionLatest() != UPDATER_VERSION);        
}

bool UpdaterEngine::CheckGeneral()
{
    /*
    * Compare length of both old and new client version lists.
    * If they're the same, then compare the last lines to be extra sure.
    * If they're not, then we know there's some updates.
    */

    // Start by fetching the configs.
    const csRefArray<ClientVersion>& oldCvs = config->GetCurrentConfig()->GetClientVersions();
    const csRefArray<ClientVersion>& newCvs = config->GetNewConfig()->GetClientVersions();

    size_t newSize = newCvs.GetSize();

    // Old is bigger than the new (out of sync), or same size.
    if(oldCvs.GetSize() >= newSize)
    {
        // If both are empty then skip the extra name check!
        if(newSize != 0)
        {
            bool outOfSync = oldCvs.GetSize() > newSize;

            if(!outOfSync)
            {
                for(size_t i=0; i<newSize; i++)
                {
                    ClientVersion* oldCv = oldCvs.Get(i);
                    ClientVersion* newCv = newCvs.Get(i);

                    csString name(newCv->GetName());
                    if(!name.Compare(oldCv->GetName()))
                    {
                        outOfSync = true;
                        break;
                    }
                }
            }

            if(outOfSync)
            {
                // There's a problem and we can't continue. Throw a boo boo and clean up.
                PrintOutput("\nLocal config and server config are incompatible!\n");
                PrintOutput("This is caused when your local version becomes out of sync with the update mirrors.\n");
                PrintOutput("To resolve this, reinstall your client using the latest installer on the website.\n");
                fileUtil->RemoveFile("/this/updaterinfo.xml");
                fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);
                return false;
            }
        }
        // Remove the backup of the xml (they're the same).
        fileUtil->RemoveFile("/this/updaterinfo.xml.bak");
        return false;
    }

    // New is bigger than the old, so there's updates.
    return true;
}

csRef<iDocumentNode> UpdaterEngine::GetRootNode(const char* nodeName)
{
    // Load xml.
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem);
    if (!xml)
    {
        PrintOutput("Could not load the XML Document System\n");
        return NULL;
    }

    //Try to read file
    csRef<iDataBuffer> buf = vfs->ReadFile(nodeName);
    if (!buf || !buf->GetSize())
    {
        PrintOutput("Couldn't open xml file '%s'!\n", nodeName);
        return NULL;
    }

    //Try to parse file
    configdoc = xml->CreateDocument();
    const char* error = configdoc->Parse(buf);
    if (error)
    {
        PrintOutput("XML Parsing error in file '%s': %s.\n", nodeName, error);
        return NULL;
    }

    //Try to get root
    csRef<iDocumentNode> root = configdoc->GetRoot ();
    if (!root)
    {
        PrintOutput("Couldn't get config file rootnode!");
        return NULL;
    }

    return root;
}

#ifdef CS_PLATFORM_WIN32

bool UpdaterEngine::SelfUpdate(int selfUpdating)
{
    // Info for CreateProcess.
    STARTUPINFO siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);

    // Check what stage of the update we're in.
    switch(selfUpdating)
    {
    case 1: // We've downloaded the new file and executed it.
        {
            PrintOutput("Copying new files!\n");

            // Construct executable names.
            csString tempName = appName;
            appName.AppendFmt(".exe");
            tempName.AppendFmt("2.exe");

            // Delete the old updater file and copy the new in place.
            fileUtil->RemoveFile("/this/" + appName);
            fileUtil->CopyFile("/this/" + tempName, "/this/" + appName, true, true);

            // Copy any art and data.
            if(appName.Compare("pslaunch"))
            {
              csString zip = appName;
              zip.AppendFmt(config->GetCurrentConfig()->GetPlatform());
              zip.AppendFmt(".zip");

              // Mount zip
              csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
              vfs->Mount("/zip", realZipPath->GetData());

              csString artPath = "/art/";
              artPath.AppendFmt("%s.zip", appName.GetData());
              fileUtil->CopyFile("/zip" + artPath, artPath, true, false, true);

              csString dataPath = "/data/gui/";
              dataPath.AppendFmt("%s.xml", appName.GetData());
              fileUtil->CopyFile("/zip" + dataPath, dataPath, true, false, true);

              vfs->Unmount("/zip", realZipPath->GetData());
            }

            // Create a new process of the updater.
            CreateProcess(appName.GetData(), "selfUpdateSecond", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
            return true;
        }
    case 2: // We're now running the new updater in the correct location.
        {
            // Clean up left over files.
            PrintOutput("\nCleaning up!\n");

            // Construct zip name.
            csString zip = appName;
            zip.AppendFmt(config->GetCurrentConfig()->GetPlatform());
            zip.AppendFmt(".zip");

            // Remove updater zip.
            fileUtil->RemoveFile("/this/" + zip); 

            // Remove temp updater file.
            fileUtil->RemoveFile("/this/" + appName + "2.exe");            

            return false;
        }
    default: // We need to extract the new updater and execute it.
        {
            PrintOutput("Beginning self update!\n");

            // Construct zip name.
            csString zip = appName;
            zip.AppendFmt(config->GetCurrentConfig()->GetPlatform());
            zip.AppendFmt(".zip");

            if(vfs->Exists("/this/" + zip))
            {
                fileUtil->RemoveFile("/this/" + zip);
            }

            // Download new updater file.
            downloader->DownloadFile(zip, zip, false, true);         

            // Check md5sum is correct.
            csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + zip, true);
            if (!buffer)
            {
                PrintOutput("Could not get MD5 of updater zip!!\n");
                return false;
            }

            csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());
            csString md5sum = md5.HexString();

            if(!md5sum.Compare(config->GetNewConfig()->GetUpdaterVersionLatestMD5()))
            {
                PrintOutput("md5sum of updater zip does not match correct md5sum!!\n");
                return false;
            }

            // md5sum is correct, mount zip and copy file.
            csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
            vfs->Mount("/zip", realZipPath->GetData());

            csString from = "/zip/";
            from.AppendFmt(appName);
            from.AppendFmt(".exe");
            appName.AppendFmt("2.exe");

            fileUtil->CopyFile(from, "/this/" + appName, true, true);
            vfs->Mount("/zip", realZipPath->GetData());;

            // Create a new process of the updater.
            CreateProcess(appName.GetData(), "selfUpdateFirst", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
            return true;
        }
    }
}

#else

bool UpdaterEngine::SelfUpdate(int selfUpdating)
{
    // Check what stage of the update we're in.
    switch(selfUpdating)
    {
    case 2: // We're now running the new updater in the correct location.
        {
            // Clean up left over files.
            PrintOutput("\nCleaning up!\n");

            // Construct zip name.
            csString zip = appName;
            zip.AppendFmt(config->GetCurrentConfig()->GetPlatform());
            zip.AppendFmt(".zip");

            // Remove updater zip.
            fileUtil->RemoveFile("/this/" + zip); 

            return false;
        }
    default: // We need to extract the new updater and execute it.
        {
            PrintOutput("Beginning self update!\n");

            // Construct zip name.
            csString zip = appName;
            zip.AppendFmt(config->GetCurrentConfig()->GetPlatform());
            zip.AppendFmt(".zip");

            if(vfs->Exists("/this/" + zip))
            {
                fileUtil->RemoveFile("/this/" + zip);
            }

            // Download new updater file.
            downloader->DownloadFile(zip, zip, false, true);         

            // Check md5sum is correct.
            csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + zip, true);
            if (!buffer)
            {
                PrintOutput("Could not get MD5 of updater zip!!\n");
                return false;
            }

            csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());

            csString md5sum = md5.HexString();

            if(!md5sum.Compare(config->GetNewConfig()->GetUpdaterVersionLatestMD5()))
            {
                PrintOutput("md5sum of updater zip does not match correct md5sum!!\n");
                return false;
            }

            csString cmd;
            csRef<iDataBuffer> thisPath = vfs->GetRealPath("/this/");
            cmd.Format("cd %s; unzip -oqq %s", thisPath->GetData(), zip.GetData());
            system(cmd);

#if defined(CSPLATFORM_MACOSX)

            // Create a new process of the updater and exit.
            cmd.Clear();
            cmd.Format("%s%s.app/Contents/MacOS/%s_static selfUpdateSecond", thisPath->GetData(), appName.GetData(), appName.GetData());
            system(cmd);
#else
            if(fork() == 0)
                execl(appName, appName, "selfUpdateSecond", NULL);
#endif
            return true;
        }
    }
}

#endif


void UpdaterEngine::GeneralUpdate()
{
    /*
    * This function updates our non-updater files to the latest versions,
    * writes new files and deletes old files.
    * This may take several iterations of patching.
    * After each iteration we need to update updaterinfo.xml.bak as well as the array.
    */

    // Start by fetching the configs.
    csRefArray<ClientVersion>& oldCvs = config->GetCurrentConfig()->GetClientVersions();
    const csRefArray<ClientVersion>& newCvs = config->GetNewConfig()->GetClientVersions();
    csRef<iDocumentNode> rootnode = GetRootNode(UPDATERINFO_OLD_FILENAME);
    csRef<iDocumentNode> confignode = rootnode->GetNode("config");

    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return;
    }

    // Main loop.
    while(CheckGeneral())
    {
        // Find starting point in newCvs from oldCvs.
        size_t index = oldCvs.GetSize();

        // Use index to find the first update version in newCvs.
        ClientVersion* newCv = newCvs.Get(index);

        // Construct zip name.
        csString zip = config->GetCurrentConfig()->GetPlatform();
        zip.AppendFmt("-%s.zip", newCv->GetName());

        if(vfs->Exists("/this/" + zip))
        {
            fileUtil->RemoveFile("/this/" + zip);
        }

        // Download update zip.
        PrintOutput("\nDownloading update file..\n");
        downloader->DownloadFile(zip, zip, false, true);

        // Check md5sum is correct.
        csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + zip, true);
        if (!buffer)
        {
            PrintOutput("Could not get MD5 of updater zip!!\n");
            return;
        }

        csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());

        csString md5sum = md5.HexString();

        if(!md5sum.Compare(newCv->GetMD5Sum()))
        {
            PrintOutput("md5sum of client zip does not match correct md5sum!!\n");
            return;
        }

         // Mount zip
        csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
        vfs->Mount("/zip", realZipPath->GetData());

        // Parse deleted files xml, make a list.
        csArray<csString> deletedList;
        csRef<iDocumentNode> deletedrootnode = GetRootNode("/zip/deletedfiles.xml");
        if(deletedrootnode)
        {
            csRef<iDocumentNode> deletednode = deletedrootnode->GetNode("deletedfiles");
            csRef<iDocumentNodeIterator> nodeItr = deletednode->GetNodes();
            while(nodeItr->HasNext())
            {
                csRef<iDocumentNode> node = nodeItr->Next();
                deletedList.PushSmart(node->GetAttributeValue("name"));
            }
        }

        // Remove all those files from our real dir.
        for(uint i=0; i<deletedList.GetSize(); i++)
        {
            fileUtil->RemoveFile("/this/" + deletedList.Get(i));
        }

        // Parse new files xml, make a list.
        csArray<csString> newList;
        csArray<csString> newListPlatform;
        csArray<bool> newListExec;
        csRef<iDocumentNode> newrootnode = GetRootNode("/zip/newfiles.xml");
        if(newrootnode)
        {
            csRef<iDocumentNode> newnode = newrootnode->GetNode("newfiles");
            csRef<iDocumentNodeIterator> nodeItr = newnode->GetNodes();
            while(nodeItr->HasNext())
            {
                csRef<iDocumentNode> node = nodeItr->Next();
                newList.PushSmart(node->GetAttributeValue("name"));
                newListPlatform.Push(node->GetAttributeValue("platform"));
                newListExec.Push(node->GetAttributeValueAsBool("exec"));
            }
        }

        // Copy all those files to our real dir.
        for(uint i=0; i<newList.GetSize(); i++)
        {
            fileUtil->CopyFile("/zip/" + newList.Get(i), "/this/" + newList.Get(i), true, newListExec.Get(i));
        }

        // Parse changed files xml, binary patch each file.
        csRef<iDocumentNode> changedrootnode = GetRootNode("/zip/changedfiles.xml");
        if(changedrootnode)
        {
            csRef<iDocumentNode> changednode = changedrootnode->GetNode("changedfiles");
            csRef<iDocumentNodeIterator> nodeItr = changednode->GetNodes();
            while(nodeItr->HasNext())
            {
                csRef<iDocumentNode> next = nodeItr->Next();

                csString newFilePath = next->GetAttributeValue("filepath");
                csString diff = next->GetAttributeValue("diff");
                csString oldFilePath = newFilePath;
                oldFilePath.AppendFmt(".old");

                // Move old file to a temp location ready for input.
                fileUtil->MoveFile("/this/" + newFilePath, "/this/" + oldFilePath, true, false, true);

                // Move diff to a real location ready for input.
                fileUtil->CopyFile("/zip/" + diff, "/this/" + newFilePath + ".vcdiff", true, false, true);
                diff = newFilePath + ".vcdiff";

                // Get real paths.
                csRef<iDataBuffer> oldFP = vfs->GetRealPath("/this/" + oldFilePath);
                csRef<iDataBuffer> diffFP = vfs->GetRealPath("/this/" + diff);
                csRef<iDataBuffer> newFP = vfs->GetRealPath("/this/" + newFilePath);
                
                // Save permissions.
                csRef<FileStat> fs = fileUtil->StatFile(oldFP->GetData());
#ifdef CS_PLATFORM_UNIX
                if(next->GetAttributeValueAsBool("exec"))
                {
                    fs->mode = fs->mode | S_IXUSR | S_IXGRP;
                }
#endif

                // Binary patch.
                PrintOutput("Patching file %s: ", newFilePath.GetData());
                if(!PatchFile(oldFP->GetData(), diffFP->GetData(), newFP->GetData()))
                {
                    PrintOutput("Failed!\n");
                    PrintOutput("Attempting to download full version of %s: \n", newFilePath.GetData());

                    // Get the 'backup' mirror, should always be the first in the list.
                    csString baseurl = config->GetNewConfig()->GetMirror(0)->GetBaseURL();
                    baseurl.Append("backup/");

                    // Try path from base URL.
                    csString url = baseurl;
                    if(!downloader->DownloadFile(url.Append(newFilePath.GetData()), newFilePath.GetData(), true, true))
                    {
                        // Maybe it's in a platform specific subdirectory. Try that next.
                        url = baseurl;
                        url.AppendFmt("%s/", config->GetNewConfig()->GetPlatform());
                        if(!downloader->DownloadFile(url.Append(newFilePath.GetData()), newFilePath.GetData(), true, true))
                        {
                            PrintOutput("\nUnable to update file: %s. Reverting file!\n", newFilePath.GetData());
                            fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);
                        }
                        else
                            PrintOutput("Done!\n");
                    }
                    else
                        PrintOutput("Done!\n");
                }
                else
                {
                    PrintOutput("Done!\n");

                    // Check md5sum is correct.
                    PrintOutput("Checking for correct md5sum: ");
                    csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + newFilePath);
                    if(!buffer)
                    {
                        PrintOutput("Could not get MD5 of patched file %s! Reverting file!\n", newFilePath.GetData());
                        fileUtil->RemoveFile("/this/" + newFilePath);
                        fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);
                    }
                    else
                    {
                        csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());
                        csString md5sum = md5.HexString();

                        csString fileMD5 = next->GetAttributeValue("md5sum");

                        if(!md5sum.Compare(fileMD5))
                        {
                            PrintOutput("md5sum of file %s does not match correct md5sum! Reverting file!\n", newFilePath.GetData());
                            fileUtil->RemoveFile("/this/" + newFilePath);
                            fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);
                        }
                        else
                            PrintOutput("Success!\n");
                    }
                    fileUtil->RemoveFile("/this/" + oldFilePath);
                 }
                // Clean up temp files.
                fileUtil->RemoveFile("/this/" + diff, false);

                // Set permissions.
                if(fs.IsValid())
                {
                    fileUtil->SetPermissions(newFP->GetData(), fs);
                }
            }
        }

        // Unmount zip and delete.
        if(vfs->Unmount("/zip", realZipPath->GetData()))
        {
            vfs->Sync();
            fileUtil->RemoveFile("/this/" + zip);
        }
        else
        {
            printf("Failed to unmount file %s\n", zip.GetData());
        }

        // Add version info to updaterinfo.xml.bak and oldCvs.
        csString value("<version name=\"");
        value.AppendFmt("%s\" />", newCv->GetName());
        confignode->GetNode("client")->CreateNodeBefore(CS_NODE_TEXT)->SetValue(value);
        oldCvs.PushSmart(newCv);
     }
}

void UpdaterEngine::CheckIntegrity()
{
    // Load current config data.
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get root node!\n");
        return;
    }

    csRef<iDocumentNode> confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return;
    }

    if(!confignode->GetAttributeValueAsBool("active", true))
    {
        PrintOutput("This updaterinfo.xml file is marked as inactive and will not work. Please get another one.\n");
        return;
    }

    // Load updater config
    if (!config->GetCurrentConfig()->Initialize(confignode))
    {
        PrintOutput("Failed to Initialize mirror config current!\n");
        return;
    }

    // Initialise downloader.
    downloader = new Downloader(GetVFS(), config);

    // Set proxy
    downloader->SetProxy(GetConfig()->GetProxy().host.GetData(),
        GetConfig()->GetProxy().port);

    // Get the zip with md5sums.
    fileUtil->RemoveFile("integrity.zip", true);
    csString baseurl = config->GetCurrentConfig()->GetMirror(0)->GetBaseURL();
    baseurl.Append("backup/");
    if(!downloader->DownloadFile(baseurl + "integrity.zip", "integrity.zip", true, true))
    {
        PrintOutput("\nFailed to download integrity.zip!\n");
        return;
    }

    // Process the list of md5sums.
    csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/integrity.zip");
    vfs->Mount("/zip/", realZipPath->GetData());

    bool failed = false;
    csRef<iDocumentNode> r = GetRootNode("/zip/integrity.xml");
    if(!r)
    {
        PrintOutput("Unable to get root node!\n");
        failed = true;
    }

    if(!failed)
    {
        csRef<iDocumentNode> md5sums = r->GetNode("md5sums");
        if (!md5sums)
        {
            PrintOutput("Couldn't find md5sums node!\n");
            failed = true;
        }

        if(!failed)
        {
            csRefArray<iDocumentNode> failed;
#ifdef CS_PLATFORM_UNIX
            csHash<bool, csRef<iDocumentNode> > failedExec;
#endif
            csRef<iDocumentNodeIterator> md5nodes = md5sums->GetNodes("md5sum");
            while(md5nodes->HasNext())
            {
                csRef<iDocumentNode> node = md5nodes->Next();

                csString platform = node->GetAttributeValue("platform");

                if(!config->UpdatePlatform() && !platform.Compare("all"))
                    continue;

                csString path = node->GetAttributeValue("path");
                csString md5sum = node->GetAttributeValue("md5sum");

                csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + path);
                if(!buffer)
                {
                    // File is genuinely missing.
                    if(platform.Compare(config->GetCurrentConfig()->GetPlatform()) ||
                       platform.Compare("all"))
                    {
                        failed.Push(node);
#ifdef CS_PLATFORM_UNIX
                        failedExec.Put(node, node->GetAttributeValueAsBool("exec"));
#endif
                    }
                    continue;
                }

                csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());
                csString md5s = md5.HexString();

                if(!md5s.Compare(md5sum))
                {
                    failed.Push(node);
                }
            }

            size_t failedSize = failed.GetSize();
            if(failedSize == 0)
            {
                PrintOutput("\nAll files passed the check!\n");
            }
            else
            {
                PrintOutput("\nThe following files failed the check:\n");
                for(size_t i=0; i<failedSize; i++)
                {
                    PrintOutput("%s\n", failed.Get(i)->GetAttributeValue("path"));
                }

                PrintOutput("\nDo you wish to download the correct copies of these files? (y/n)\n");
                char c = getchar();
                while(c != 'y' && c != 'n')
                {
                    c = getchar();
                }
                
                if(c == 'y')
                {
                    for(size_t i=0; i<failedSize; i++)
                    {
                        PrintOutput("\nDownloading file: %s\n", failed.Get(i)->GetAttributeValue("path"));

                        csString downloadpath("/this/");
                        downloadpath.Append(failed.Get(i)->GetAttributeValue("path"));

                        // Save permissions.
                        csRef<iDataBuffer> rp = vfs->GetRealPath(downloadpath);
                        csRef<FileStat> fs = fileUtil->StatFile(rp->GetData());
                        fileUtil->MoveFile(downloadpath, downloadpath + ".bak", true, false, true);

                        // Download file.
                        if(!downloader->DownloadFile(baseurl + failed.Get(i)->GetAttributeValue("path"),
                                                     failed.Get(i)->GetAttributeValue("path"), true, true))
                        {
                            // Maybe it's in a platform specific subdirectory. Try that next.
                           csString url = baseurl + config->GetCurrentConfig()->GetPlatform() + "/";
                           if(!downloader->DownloadFile(url + failed.Get(i)->GetAttributeValue("path"),
                                                        failed.Get(i)->GetAttributeValue("path"), true, true))
                           {
                               // Restore file.
                               fileUtil->MoveFile(downloadpath + ".bak", downloadpath, true, false, true);
                               PrintOutput("Failed!\n");
                           }
                        }

                        // Restore permissions.
                        if(fs.IsValid())
                        {
#ifdef CS_PLATFORM_UNIX
                            bool failedEx = failedExec.Get(failed.Get(i), false);
                            if(failedEx)
                            {
                                fs->mode = fs->mode | S_IXUSR | S_IXGRP;
                            }
#endif
                            fileUtil->SetPermissions(rp->GetData(), fs);
                        }
                        fileUtil->RemoveFile(downloadpath + ".bak", true);
                        PrintOutput("Success!\n");
                    }
                    PrintOutput("Done!\n");
                }
            }
        }
    }

    vfs->Unmount("/zip/", realZipPath->GetData());

    fileUtil->RemoveFile("integrity.zip", true);

    return;
}
