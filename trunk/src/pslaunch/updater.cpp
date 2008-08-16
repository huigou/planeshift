/*
* updater.cpp - Author: Mike Gist
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

/* 
* This is the file for the console updater only. Everything in here needs to be
* written again for the launcher!
*/

#include <psconfig.h>

#include "updaterconfig.h"
#include "updaterengine.h"
#include "updater.h"


CS_IMPLEMENT_APPLICATION

iObjectRegistry* psUpdater::object_reg = NULL;

psUpdater::psUpdater(int argc, char* argv[])
{
    object_reg = csInitializer::CreateEnvironment (argc, argv);
    if(!object_reg)
    {
        printf("Object Reg failed to Init!\n");
        exit(1);
    }
    // Request needed plugins.
    csInitializer::RequestPlugins(object_reg, CS_REQUEST_VFS, CS_REQUEST_END);

    // Mount the VFS paths.
    csRef<iVFS> vfs = csQueryRegistry<iVFS>(object_reg);
    if (!vfs->Mount ("/planeshift/", "$^"))
    {
        printf("Failed to mount /planeshift!\n");
        exit(1);
    }

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
    csString configPath = csGetPlatformConfigPath("PlaneShift");
    configPath.ReplaceAll("/.crystalspace/", "/.");
    configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);
    FileUtil fileUtil(vfs);
    csRef<FileStat> filestat = fileUtil.StatFile(configPath);
    if (!filestat.IsValid() && CS_MKDIR(configPath) < 0)
    {
        printf("Could not create required %s directory!\n", configPath.GetData());
        exit(1);
    }

    if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
    {
        printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
        exit(1);
    }
}

psUpdater::~psUpdater()
{
    csInitializer::DestroyApplication(object_reg);
}

void psUpdater::RunUpdate(UpdaterEngine* engine) const
{
    // Check if we're already in the middle of a self-update.
    if(engine->GetConfig()->IsSelfUpdating())
    {
        // Continue the self update, passing the update stage.
        if(engine->SelfUpdate(engine->GetConfig()->IsSelfUpdating()))
            return;
    }

    // Check if we want to do an integrity check instead of an update.
    if(engine->GetConfig()->CheckForIntegrity())
    {
        printf("Checking the integrity of the install:\n");
        engine->CheckIntegrity();
        return;
    }

    // Begin update checking!
    printf("Checking for updates:\n");
    engine->CheckForUpdates();
    return;
}

int main(int argc, char* argv[])
{
    // Set up CS
    psUpdater* updater = new psUpdater(argc, argv);

    // Convert args to an array of csString.
    csArray<csString> args;
    for(int i=0; i<argc; i++)
    {
        args.Push(argv[i]);
    }

    // Initialize updater engine.
    UpdaterEngine* engine = new UpdaterEngine(args, updater->GetObjectRegistry(), "psupdater");

    printf("\nPlaneShift Updater Version %1.2f for %s.\n\n", UPDATER_VERSION, engine->GetConfig()->GetCurrentConfig()->GetPlatform());

    // Run the update process!
    updater->RunUpdate(engine);

    // Maybe this fixes a bug.
    fflush(stdout);

    if(!engine->GetConfig()->IsSelfUpdating())
    {
        printf("\nUpdater finished, press enter to exit.\n");
        getchar();
    }

    // Terminate updater!
    delete engine;
    delete updater;
    engine = NULL;
    updater = NULL;

    return 0;
}
