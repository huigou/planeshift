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
#include <csutil/stringarray.h>
#include <csutil/xmltiny.h>

#include "updaterconfig.h"
#include "updaterengine.h"
#include "binarypatch.h"

#ifndef CS_COMPILER_MSVC
#include <unistd.h>
#endif

#ifdef _WIN32
#include <shlobj.h>
#endif

#define CHECK_QUIT \
    if(CheckQuit()) \
    { \
        infoShare->SetCancelUpdater(false); \
        return; \
    } \

UpdaterEngine::UpdaterEngine(csStringArray& args, iObjectRegistry* object_reg, const char* appName)
{
    InfoShare *is = new InfoShare();
    hasGUI = false;
    Init(args, object_reg, appName, is);
}

UpdaterEngine::UpdaterEngine(csStringArray& args, iObjectRegistry* object_reg, const char* appName,
                             InfoShare *infoshare)
{
    hasGUI = true;
    Init(args, object_reg, appName, infoshare);
}

void UpdaterEngine::Init(csStringArray& args, iObjectRegistry* _object_reg, const char* _appName,
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

void UpdaterEngine::Run()
{
  CheckForUpdates();

  while(!infoShare->GetExitGUI())
  {
      csSleep(100);
      if(infoShare->GetCheckIntegrity())
      {
          CheckIntegrity();
          infoShare->SetCheckIntegrity(false);
      }
  }
}

void UpdaterEngine::CheckForUpdates()
{
    // Make sure the old instance had time to terminate (self-update).
    if(config->IsSelfUpdating())
        csSleep(1000);

    // Load current config data.
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
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

    printf("Checking for updates to the updater: ");

    if(CheckUpdater())
    {
        printf("Update Available!\n");

        // Check if we have write permissions.
#ifdef _WIN32
        // TODO: Improve this.
        if(!IsUserAnAdmin() && !log.IsValid())
#else
        if(!log.IsValid())
#endif
        {
            printf("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
            infoShare->SetUpdateAdminNeeded(true);
            return;
        }

        // If using a GUI, prompt user whether or not to update.
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(true);         
            while(!infoShare->GetPerformUpdate())
            {
                // Make sure we die if we exit the gui as well.
                if(!infoShare->GetUpdateNeeded() || infoShare->GetExitGUI() || CheckQuit())
                {
                    infoShare->Sync();
                    delete downloader;
                    downloader = NULL;
                    return;
                }

                csSleep(100);
            }

            // If we're going to self-update, close the GUI.
            infoShare->SetExitGUI(true);
            infoShare->Sync();
        }

        // Begin the self update process.
        SelfUpdate(false);
        return;
    }

    printf("No updates needed!\nChecking for updates to all files: ");

    // Check for normal updates.
    if(CheckGeneral())
    {
        printf("Updates Available!\n");

        // Check if we have write permissions.
#ifdef _WIN32
        if(!IsUserAnAdmin())
#else
        if(!log.IsValid())
#endif
        {
            printf("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
            infoShare->SetUpdateAdminNeeded(true);
            return;
        }

        // If using a GUI, prompt user whether or not to update.
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(true);
            while(!infoShare->GetPerformUpdate())
            {
                if(!infoShare->GetUpdateNeeded() || infoShare->GetExitGUI() || CheckQuit())
                {
                    infoShare->Sync();
                    delete downloader;
                    downloader = NULL;
                    return;
                }

                csSleep(100);
            }
            infoShare->Sync();
        }

        // Begin general update.
        GeneralUpdate();

        // Maybe this fixes a bug.
        fflush(stdout);

        // Mark as finished and sync with gui.
        if(hasGUI)
        {
            PrintOutput("\nUpdate finished!\n");
            infoShare->SetUpdateNeeded(false);
            infoShare->Sync();
        }
    }
    else
    {
        // Mark no updates needed.
        printf("No updates needed!\n");
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(false);
        }
    }

    // Clean up.
    delete downloader;
    downloader = NULL;
}

bool UpdaterEngine::CheckUpdater()
{
    // Download the latest updaterinfo. 
    fileUtil->RemoveFile(UPDATERINFO_FILENAME, true);
    if(!downloader->DownloadFile("updaterinfo.xml", UPDATERINFO_FILENAME, false, true, 3, true))
    {
        return false;
    }

    // Load new config data.
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get root node!\n");
        return false;
    }

    csRef<iDocumentNode> confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return false;
    }

    if(!confignode->GetAttributeValueAsBool("active", true))
    {
        PrintOutput("The updater mirrors are down, possibly for an update. Please try again later.\n");
        return false;
    }

    if(!config->GetNewConfig()->Initialize(confignode))
    {
        PrintOutput("Failed to Initialize mirror config new!\n");
        return false;
    }

    // Compare Versions.
    return(config->UpdatePlatform() && (config->GetNewConfig()->GetUpdaterVersionLatest() - UPDATER_VERSION > 0.01));        
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

    size_t oldSize = oldCvs.GetSize();
    size_t newSize = newCvs.GetSize();

    // Old is bigger than the new (out of sync), or same size.
    if(oldSize >= newSize)
    {
        // If both are empty then skip the extra name check!
        if(newSize != 0)
        {
            bool outOfSync = oldSize > newSize;

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
                PrintOutput("To resolve this, run a repair.\n");
            }
        }

        return false;
    }

    // Check for updates concerning this platform.
    for(size_t i=oldSize; i<newSize; ++i)
    {
        ClientVersion* cv = newCvs[i];
        if(strcmp(cv->GetMD5Sum(), "") || strcmp(cv->GetGenericMD5Sum(), ""))
            return true;
    }
    
    // No updates related to this platform are available.
    return false;
}

csRef<iDocumentNode> UpdaterEngine::GetRootNode(const char* nodeName, csRef<iDocument>* document)
{
    // Load xml.
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem);
    if (!xml)
    {
        printf("Could not load the XML Document System\n");
        return NULL;
    }

    // Try to read file
    csRef<iDataBuffer> buf = vfs->ReadFile(nodeName);
    if (!buf || !buf->GetSize())
    {
        printf("Couldn't open xml file '%s'!\n", nodeName);
        return NULL;
    }

    // Try to parse file
    if(document)
    {
        *document = xml->CreateDocument();
        configdoc = *document;
    }
    else
    {
        configdoc = xml->CreateDocument();
    }

    const char* error = configdoc->Parse(buf);
    if (error)
    {
        printf("XML Parsing error in file '%s': %s.\n", nodeName, error);
        return NULL;
    }

    // Try to get root
    csRef<iDocumentNode> root = configdoc->GetRoot ();
    if (!root)
    {
        printf("Couldn't get config file rootnode!");
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
            while(!fileUtil->RemoveFile("/this/" + appName))
            {
                csSleep(50);
            }
            fileUtil->CopyFile("/this/" + tempName, "/this/" + appName, true, true);

            // Copy any art and data.
            if(hasGUI)
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
            while(!fileUtil->RemoveFile("/this/" + appName + "2.exe"))
            {
                csSleep(50);
            }

            GetConfig()->SetSelfUpdating(false);
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
                fileUtil->RemoveFile("/this/" + zip);
                return false;
            }

            // md5sum is correct, mount zip and copy file.
            csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
            vfs->Mount("/zip", realZipPath->GetData());

            csString from = "/zip/";
            from.AppendFmt("%s.exe", appName.GetData());
            appName.AppendFmt("2.exe");

            fileUtil->CopyFile(from, "/this/" + appName, true, true);
            vfs->Unmount("/zip", realZipPath->GetData());

            // Create a new process of the updater.
            CreateProcess(appName.GetData(), "selfUpdateFirst", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
            GetConfig()->SetSelfUpdating(true);
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
            zip.AppendFmt("%s", config->GetCurrentConfig()->GetPlatform());
            zip.AppendFmt(".zip");

            // Remove updater zip.
            fileUtil->RemoveFile("/this/" + zip); 

            GetConfig()->SetSelfUpdating(false);

            return false;
        }
    default: // We need to extract the new updater and execute it.
        {
            PrintOutput("Beginning self update!\n");

            // Construct zip name.
            csString zip = appName;
            zip.AppendFmt("%s", config->GetCurrentConfig()->GetPlatform());
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
            int res = system(cmd);
            if(res == -1)
            {
                printf("system(%s) failed!\n", cmd.GetData());
            }

#if defined(CS_PLATFORM_MACOSX)

            // Create a new process of the updater and exit.
            csString path = appName;
            if(fork() == 0)
                execl(path.AppendFmt(".app/Contents/MacOS/%s_static", appName), path.AppendFmt(".app/Contents/MacOS/%s_static", appName), "selfUpdateSecond", NULL);
#else
            if(fork() == 0)
                execl(appName, appName, "selfUpdateSecond", NULL);
#endif
            GetConfig()->SetSelfUpdating(true);
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
    csRef<iDocument> updaterinfo;
    csRef<iDocumentNode> rootnode = GetRootNode(UPDATERINFO_CURRENT_FILENAME, &updaterinfo);
    csRef<iDocumentNode> confignode = rootnode->GetNode("config");

    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return;
    }

    // True if the platform is 'generic' (data only).
    bool genericPlatform = !strcmp(config->GetCurrentConfig()->GetPlatform(),
        config->GetCurrentConfig()->GetGeneric());

    // Main loop.
    while(CheckGeneral())
    {
        // Find starting point in newCvs from oldCvs.
        size_t index = oldCvs.GetSize();

        // Use index to find the first update version in newCvs.
        ClientVersion* newCv = newCvs.Get(index);

        /**
         * Two passes:
         * First is for the platform specific update.
         * Second is for the generic update.
         */
        for(int pass=1; pass<=2; ++pass)
        {
            // Check that this update is relevent to this platform.
            if(pass == 1 && !strcmp(newCv->GetMD5Sum(), ""))
                continue;

            // Only do second pass if the platform isn't 'generic'.
            if(pass == 2 && (genericPlatform || !strcmp(newCv->GetGenericMD5Sum(), "")))
                break;

            // Construct zip name.
            csString zip;
            if(pass == 1)
            {
                zip = config->GetCurrentConfig()->GetPlatform();
            }
            else
            {
                zip = config->GetCurrentConfig()->GetGeneric();
            }

            zip.AppendFmt("-%s.zip", newCv->GetName());

            if(vfs->Exists("/this/" + zip))
            {
                fileUtil->RemoveFile("/this/" + zip);
            }

            // Download update zip.
            if(pass == 1 && !genericPlatform)
            {
                PrintOutput("Downloading platform specific update file..\n\n");
            }
            else
            {
                PrintOutput("Downloading generic update file..\n\n");
            }

            if(!downloader->DownloadFile(zip, zip, false, true))
            {
                PrintOutput("Failed to download the update file! Try again later.\n");
                return;
            }

            // Check md5sum is correct.
            csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + zip, true);
            if (!buffer)
            {
                PrintOutput("Could not get MD5 of updater zip!!\n");
                return;
            }

            csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());

            csString md5sum = md5.HexString();
            csString correctMD5Sum;
            if(pass == 1)
            {
                correctMD5Sum = newCv->GetMD5Sum();
            }
            else
            {
                correctMD5Sum = newCv->GetGenericMD5Sum();
            }
            
            if(md5sum != correctMD5Sum)
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

                    // Start by checking whether the existing file is already up to date.
                    csRef<iDataBuffer> buffer = vfs->ReadFile("/this/" + newFilePath);
                    if(buffer)
                    {
                        csMD5::Digest md5 = csMD5::Encode(buffer->GetData(), buffer->GetSize());
                        csString md5sum = md5.HexString();

                        csString fileMD5 = next->GetAttributeValue("md5sum");

                        // If it's up to date then skip this file.
                        if(md5sum.Compare(fileMD5))
                        {
                            continue;
                        }
                    }

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
                        PrintOutput("Attempting to download full version of %s: ", newFilePath.GetData());

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
                                PrintOutput("Please re-run the update.\n");
                                return;
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
                            PrintOutput("\nCould not get MD5 of patched file %s! Reverting file!\n", newFilePath.GetData());
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
                                PrintOutput("\nmd5sum of file %s does not match correct md5sum! Reverting file!\n", newFilePath.GetData());
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

            PrintOutput("\n");
        }

        // Add version info to updaterinfo.xml and oldCvs.
        csRef<iDocumentNode> newNode = confignode->GetNode("client")->CreateNodeBefore(CS_NODE_ELEMENT);
        newNode->SetValue("version");
        newNode->SetAttribute("name", newCv->GetName());
        updaterinfo->Write(vfs, UPDATERINFO_CURRENT_FILENAME);
        oldCvs.PushSmart(newCv);
     }
}

void UpdaterEngine::CheckIntegrity()
{
    PrintOutput("Beginning integrity check!\n");

    // Load current config data.
    csRef<iDocumentNode> confignode;
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
    bool success = true;
    if(!root.IsValid())
    {
        printf("Unable to get root node!\n");
        success = false;
    }
    else
    {
      confignode = root->GetNode("config");
      if (!confignode.IsValid())
      {
        printf("Couldn't find config node in configfile!\n");
        success = false;
      }
    }

    if(!success)
    {
        // Check if we have write permissions.
        if(!log.IsValid())
        {
            PrintOutput("Please run this program with administrator permissions to run the integrity check.\n");
            return;
        }

        printf("Attempting to restore updaterinfo.xml!\n");
        fileUtil->RemoveFile("/this/updaterinfo.xml", true);
        downloader = new Downloader(vfs);
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile("http://www.psmirror.org/repo/updaterinfo.xml", UPDATERINFO_CURRENT_FILENAME, true, true, 3, true))
        {
            PrintOutput("\nFailed to download updater info!\n");
            return;
        }

        delete downloader;

        root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
        if(!root)
        {
            printf("Unable to get root node!\n");
            return;
        }
       
        confignode = root->GetNode("config");
        if (!confignode)
        {
            printf("Couldn't find config node in configfile!\n");
            return;
        }
    }

    // Load updater config
    if (!config->GetCurrentConfig()->Initialize(confignode))
    {
        printf("Failed to Initialize mirror config current!\n");
        return;
    }

    // Initialise downloader.
    downloader = new Downloader(vfs, config);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

    // Download new config.
    fileUtil->RemoveFile(UPDATERINFO_CURRENT_FILENAME, true);
    if(!downloader->DownloadFile("updaterinfo.xml", UPDATERINFO_CURRENT_FILENAME, false, true, 3, true))
    {
        PrintOutput("\nFailed to download updater info from a mirror!\n");
        return;
    }

    delete downloader;

    root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
    if(!root)
    {
        printf("Unable to get root node!\n");
    }

    confignode = root->GetNode("config");
    if (!confignode)
    {
        printf("Couldn't find config node in configfile!\n");
    }

    // Load updater config
    if (!config->GetCurrentConfig()->Initialize(confignode))
    {
        PrintOutput("\nFailed to Initialize mirror config current!\n");
        return;
    }

    // Initialise downloader.
    downloader = new Downloader(vfs, config);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

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
        printf("Unable to get root node!\n");
        failed = true;
    }

    if(!failed)
    {
        csRef<iDocumentNode> md5sums = r->GetNode("md5sums");
        if (!md5sums)
        {
            printf("Couldn't find md5sums node!\n");
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
                CHECK_QUIT
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
                       platform.Compare("cfg") || platform.Compare("all"))
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

                if((platform.Compare(config->GetCurrentConfig()->GetPlatform()) ||
                    platform.Compare("cfg") || platform.Compare("all")) && !md5s.Compare(md5sum))
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
                PrintOutput("\nThe following files failed the check:\n\n");
                for(size_t i=0; i<failedSize; i++)
                {
                    PrintOutput("%s\n", failed.Get(i)->GetAttributeValue("path"));
                }

                char c = ' ';
                PrintOutput("\nDo you wish to download the correct copies of these files? (y/n)\n");
                if(!hasGUI)
                {
                    while(c != 'y' && c != 'n')
                    {
                        c = getchar();
                    }
                }
                else
                {
                    infoShare->SetUpdateNeeded(true);
                    while(infoShare->GetUpdateNeeded())
                    {
                        CHECK_QUIT
                        csSleep(100);
                    }
                    c = infoShare->GetPerformUpdate() ? 'y' : 'n';
                    infoShare->SetPerformUpdate(false);
                    infoShare->Sync();
                }
                
                if(c == 'n')
                {
                    if(vfs->Exists("/this/updaterinfo.xml.bak"))
                    {
                        fileUtil->RemoveFile("/this/updaterinfo.xml", true);
                        fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);
                    }
                }
                else if(c == 'y')
                {
                    for(size_t i=0; i<failedSize; i++)
                    {
                        CHECK_QUIT
                        PrintOutput("\nDownloading file: %s\n", failed.Get(i)->GetAttributeValue("path"));

                        csString downloadpath("/this/");
                        downloadpath.Append(failed.Get(i)->GetAttributeValue("path"));

                        // Save permissions.
                        csRef<iDataBuffer> rp = vfs->GetRealPath(downloadpath);
                        csRef<FileStat> fs = fileUtil->StatFile(rp->GetData());
                        fileUtil->MoveFile(downloadpath, downloadpath + ".bak", true, false, true);

                        // Make parent dir if needed.
                        csString parent = downloadpath;
                        fileUtil->MakeDirectory(parent.Truncate(parent.FindLast('/')));

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
                               fileUtil->RemoveFile(downloadpath, true);
                               fileUtil->MoveFile(downloadpath + ".bak", downloadpath, true, false, true);
                               PrintOutput(" Failed!\n");
                               continue;
                           }
                        }
#ifdef CS_PLATFORM_UNIX
                        // Restore permissions.
                        if(fs.IsValid())
                        {
                            bool failedEx = failedExec.Get(failed.Get(i), false);
                            if(failedEx)
                            {
                                fs->mode = fs->mode | S_IXUSR | S_IXGRP;
                            }
                            fileUtil->SetPermissions(rp->GetData(), fs);
                        }
#endif
                        fileUtil->RemoveFile(downloadpath + ".bak", true);
                        PrintOutput(" Success!\n");
                    }
                    fileUtil->RemoveFile("/this/updaterinfo.xml.bak", true);
                    PrintOutput("\nDone!\n");
                }
            }
        }
    }

    vfs->Unmount("/zip/", realZipPath->GetData());

    fileUtil->RemoveFile("integrity.zip", true);

    return;
}

bool UpdaterEngine::SwitchMirror()
{
    // Check if we have write permissions.
#ifdef _WIN32
    // TODO: Improve this.
    if(!IsUserAnAdmin() && !log.IsValid())
#else
    if(!log.IsValid())
#endif
    {
        printf("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
        return false;
    }

    csString xmlPath = UPDATERINFO_CURRENT_FILENAME;
    csString xmlBakPath;
    xmlBakPath.Format("%s_bak", xmlPath.GetData());

    fileUtil->MoveFile(xmlPath, xmlBakPath, true, false, true);
    
    // Initialise downloader.
    downloader = new Downloader(vfs);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

    // Download new xml file.
    if(!downloader->DownloadFile(config->GetNewMirrorAddress(), xmlPath, true, true, 3, true))
    {
        csString xmlAddress;
        xmlAddress.Format("%s/updaterinfo.xml", config->GetNewMirrorAddress());
        if(!downloader->DownloadFile(xmlAddress, xmlPath, true, true, 3, true))
        {
            printf("Failed to download updaterinfo from new mirror.\n");
            fileUtil->MoveFile(xmlBakPath, xmlPath, true, false, true);
            return false;
        }
    }

    fileUtil->RemoveFile(xmlBakPath, true);
    return true;
}
