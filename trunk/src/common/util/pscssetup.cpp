/*
 * pscssetup.cpp - Authored by Elliot Paquette, cetel@earthlink.net
 *
 * The code for class reg. was adapted from Andrew Craig's 
 * pawsmainwidget.cpp.
 *
 * This is an adaptation of Matze Braun's mountcfg and *.inc files
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <psconfig.h>


#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/plugin.h>
#include <iutil/objreg.h>

#include <ivaria/reporter.h>
#include <ivaria/stdrep.h>
#include <ivideo/natwin.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <cstool/initapp.h>

#include <csutil/util.h>
#include <csutil/cmdhelp.h>
#include <csutil/scf.h>
#include <csutil/xmltiny.h>
#include <csutil/csstring.h>
#include <csutil/csshlib.h>
#include <csutil/databuf.h>
#include <iutil/stringarray.h>

#include "util/fileutil.h"
#include "util/log.h"
#include "util/consoleout.h"
#include "pscssetup.h"

iObjectRegistry* psCSSetup::object_reg = NULL;

psCSSetup::psCSSetup(int _argc, char** _argv, 
                    const char* _engineConfigfile, const char* _userConfigfile)
{
  argc=_argc;
  argv=_argv;
  engineConfigfile=_engineConfigfile;
  userConfigfile=_userConfigfile;

  object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg) PS_PAUSEEXIT(1);

}


psCSSetup::~psCSSetup()
{
}


bool psCSSetup::InitCSWindow(const char *name)
{
    csRef<iGraphics3D> g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
        return false;

    if (!g3d->GetDriver2D())
        return false;
    
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (nw)
        nw->SetTitle(name);
    return true;
}


iObjectRegistry* psCSSetup::InitCS(iReporterListener * customReporter)
{ 
    if (!object_reg)
        PS_PAUSEEXIT(1);

    //Adding path for CEL plugins
    char * celpath = getenv("CEL");
    if (celpath)
    {
        char newpath[1024];
        strncpy(newpath, celpath, 1000);
        strcat(newpath, "/");
    iSCF::SCF->ScanPluginsPath(newpath);
//        strcat(newpath, "lib/");
//        csAddLibraryPath(newpath);
    }
    else
    {
    celpath = getenv("PATH");
    if (celpath)
    {
        char newpath[1024];
        strcpy(newpath,celpath); //non-read only copy
        char *tok = strtok(newpath,";");
        while (tok)
        {
        if (strstr(tok,"cel"))
        {
            char path[1024];
            strcpy(path,tok);
            strcat(path,"/");
            iSCF::SCF->ScanPluginsPath(path);
        }
        tok = strtok(NULL,";");
        }
    }
//        csReport(object_reg, CS_REPORTER_SEVERITY_WARNING, 
//               "psclient", "no CEL environment variable set!");
//        return 0;
    }

    if ( !csInitializer::SetupConfigManager(object_reg, engineConfigfile))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, "psclient",
               "csInitializer::SetupConfigManager failed!\n"
               "Is your CRYSTAL environment variable set?");
        PS_PAUSEEXIT(1);
    }

    vfs  =  csQueryRegistry<iVFS> (object_reg);
    configManager =  csQueryRegistry<iConfigManager> (object_reg);

    // Mount any directories required early - before trying to read our .cfg.
    MountEarly();
  
    if (userConfigfile != NULL)
    {
        cfg = configManager->AddDomain(userConfigfile,vfs,iConfigManager::ConfigPriorityUserApp);
        configManager->SetDynamicDomain(cfg);
    }

    bool pluginRequest;
    if (customReporter)
    {
        pluginRequest = csInitializer::RequestPlugins(object_reg,
            CS_REQUEST_REPORTER,
            CS_REQUEST_END);

        csRef<iReporter> reporter =  csQueryRegistry<iReporter> (object_reg);
        if (reporter)
            reporter->AddReporterListener(customReporter);
    }
    else
    {
        pluginRequest = csInitializer::RequestPlugins(object_reg,
            CS_REQUEST_REPORTER,
            CS_REQUEST_REPORTERLISTENER,
            CS_REQUEST_END);
    }
    if (!pluginRequest)
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, 
               "psclient", "Failed to initialize plugins!");
        return 0;
    }
  
    // Check for commandline help.
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        csCommandLineHelper::Help (object_reg);
        PS_PAUSEEXIT(1);
    }

    // Initialize Graphics Window
    if (!InitCSWindow(APPNAME)) 
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
            "psclient", "No 3d driver (iGraphics3D) plugin!");
        PS_PAUSEEXIT(1);
    }

    if (!csInitializer::OpenApplication (object_reg))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, 
                "psclient",
                "csInitializer::OpenApplication failed!\n"
                "Is your CRYSTAL environment var set?");
        csInitializer::DestroyApplication (object_reg);
        PS_PAUSEEXIT(1);
    }

    // Reset the window name because with Xwindows OpenApplication changes it
    InitCSWindow(APPNAME);

    // tweak reporter plugin to report everything...
    // is there a command line switch or something to do this which I've missed?
    if (customReporter == 0)
    {
        csRef<iStandardReporterListener> reporter = 
             csQueryRegistry<iStandardReporterListener> (object_reg);
        if (reporter) 
        {
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_BUG, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_ERROR, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_NOTIFY, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_DEBUG, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_WARNING, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_NOTIFY, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_ERROR, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_WARNING, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_BUG, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_DEBUG, true, false, false, false, true);
        }
    }

    MountArt();

    return object_reg;
}


char* psCSSetup::PS_GetFileName(char* path)
{
    size_t pos = strlen(path);
    for ( ; pos>0; pos--)
    {
        if (path[pos] == '/')
            break;
    }
    
    return path+pos+1;
}

/**
 * Creates a mount point for the a thing zip file.
 * The thing zip file contains 3 seperate things
 * 1) The spr model file
 * 2) The texture for the model
 * 3) The icon image for the inventory screen.
 *
 * It is mounted into /planeshift/<mount_point> and the location to mount
 * is defined in the psclient.cfg/psserver.cfg file under PlaneShift.Mount.<thing>.
 */
void psCSSetup::MountThings(const char *zip, const char *mount_point)
{
    csString cfg_mount;
    csString mount;
    cfg_mount.Format("PlaneShift.Mount.%s", zip);
    mount.Format("/planeshift/%s/", mount_point);
    
    csRef<iConfigIterator> i = configManager->Enumerate(cfg_mount);
    if (i)
    {
        while (i->HasNext())
        {
            i->Next();
            if (!i->GetKey() || !strcmp(i->GetKey(), "")) continue;
            const char* zipfile = configManager->GetStr(i->GetKey(), "");
            csRef<iDataBuffer> xrpath = vfs->GetRealPath(zipfile);
            vfs->Mount(mount, **xrpath);
        }
    }
    
    // Check for development versions.
    cfg_mount.Format("PlaneShift.Dev.%s", zip);
    csRef<iConfigIterator> i2 = configManager->Enumerate(cfg_mount);
    if (i2)
    {
        while (i2->HasNext())
        {
            i2->Next();
            if (!i2->GetKey() || !strcmp(i2->GetKey(), "")) continue;
            const char* zipfile = configManager->GetStr(i2->GetKey(), "");            
            vfs->Mount(mount, zipfile);
        }
    }
}


void psCSSetup::MountMaps()
{
    csRef<iConfigIterator> it = configManager->Enumerate("PlaneShift.Mount.zipmapdir");
    if (!it) 
        return;

    while ( it->HasNext() )
    {
        it->Next();

        if (it->GetKey())
        {
            const char* dir = configManager->GetStr(it->GetKey(), "");
            if (!strcmp (dir, "")) continue;
            csRef<iDataBuffer> xpath = vfs->ExpandPath(dir);
            csRef<iStringArray> files = vfs->FindFiles( **xpath );

            if (!files) continue;
            for (size_t i=0; i < files->GetSize(); i++)
            {
                const char* filename = files->Get(i);
                if (strcmp (filename + strlen(filename) - 4, ".zip"))
                    continue;

				csString finaldir(filename);
				// Get only filename
				size_t term = finaldir.FindLast('/');
				finaldir.DeleteAt(0,term);
				finaldir.Insert(0,"/planeshift/world");
				term = finaldir.FindLast('.');
				finaldir.Truncate(term);

                // char* name = csStrNew(filename);
                // char* onlyname = PS_GetFileName(name);
                // onlyname[strlen(onlyname) - 4] = '\0';
                // csString finaldir ("/planeshift/world/");
                // finaldir += onlyname;

                csRef<iDataBuffer> xrpath = vfs->GetRealPath(filename);
                vfs->Mount(finaldir, **xrpath);
            
                // delete[] name;
            }
        }
    } 

    it = configManager->Enumerate("PlaneShift.Mount.zipmapdir");
    if (!it) return;

    while ( it->HasNext() )
    {
        it->Next();
        if (!it->GetKey() || !strcmp(it->GetKey(), "")) continue;
        const char* dir = configManager->GetStr(it->GetKey(), "");
        if (!vfs->Mount(dir, "/planeshift/world/"))
        {
            printf("Couldn't mount user specified dir: %s.\n", dir);
        }
    }
}


void psCSSetup::MountModels ()
{
    // Check for the mount point using a VFS directory.
    MountModelsZip("PlaneShift.Mount.modelzip",true);
    MountModelsZip("PlaneShift.Mount.characterszip",true);
    MountModelsZip("PlaneShift.Mount.npcszip",true);

    // Check using the dev ( absolute real path ) directory.
    MountModelsZip("PlaneShift.Dev.modelzip",false);
    MountModelsZip("PlaneShift.Dev.characterszip",false);
    MountModelsZip("PlaneShift.Dev.npcszip",false);
}

void psCSSetup::MountModelsZip(const char* key, bool vfspath)
{
    csRef<iConfigIterator> i = configManager->Enumerate(key);
    while (i->Next())
    {    
        if (!i->GetKey() || !strcmp(i->GetKey(), ""))
            continue;

        const char* zipfile = configManager->GetStr(i->GetKey(), "");

        if (vfspath)
        {
            csRef<iDataBuffer> xrpath = vfs->GetRealPath(zipfile);
            vfs->Mount("/planeshift/models/", **xrpath);
        }
        else
        {
            vfs->Mount("/planeshift/models", zipfile);
        }
    }
}

void psCSSetup::MountUserData()
{
    // Mount the per-user configuration directory (platform specific)
    // - On Linux, use ~/.PlaneShift instead of ~/.crystalspace/PlaneShift.
    csString configPath = csGetPlatformConfigPath("PlaneShift");
    configPath.ReplaceAll("/.crystalspace/", "/.");

    // Alternatively, you can set it in psclient.cfg.
    configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);

    printf("Your configuration files are in... %s\n", configPath.GetData());

    // Create the mount point if it doesn't exist...die if we can't.
    FileUtil fileUtil(vfs);
    csRef<FileStat> filestat = fileUtil.StatFile(configPath);
    if (!filestat.IsValid() && CS_MKDIR(configPath) < 0)
    {
        printf("Could not create required %s directory!\n", configPath.GetData());
        PS_PAUSEEXIT(1);
    }

    if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
    {
        printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
        PS_PAUSEEXIT(1);
    }
}

void psCSSetup::MountEarly()
{
    if (!vfs->Mount("/planeshift/", "$^"))
    {
        printf("Could not mount /planeshift!");
        PS_PAUSEEXIT(1);
    }
    
    // We need /planeshift/userdata early, so we can read planeshift.cfg.
    MountUserData();
}

void psCSSetup::MountArt()
{
    MountMaps();
    MountModels();

    MountThings("itemzip",       "items");
    MountThings("potionszip",    "potions");
    MountThings("moneyzip",      "money");
    MountThings("bookszip",      "books");
    MountThings("weaponzip",     "weapons");
    MountThings("shieldszip",    "shields");
    MountThings("helmszip",      "helms");
    MountThings("toolszip",      "tools");
    MountThings("naturalreszip", "naturalres");
    MountThings("foodzip",       "food");
    MountThings("jewelryzip",    "jewelry");
    MountThings("furniturezip",  "furniture");
    MountThings("foliagezip",    "foliage");

    // Create the mount points for glyphs
    MountThings("azurezip",      "azure_way");
    MountThings("bluezip",       "blue_way");
    MountThings("brownzip",      "brown_way");
    MountThings("crystalzip",    "crystal_way");
    MountThings("darkzip",       "dark_way");
    MountThings("redzip",        "red_way");
}
