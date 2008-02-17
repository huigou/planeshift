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
        csSleep(2000);
        // Continue the self update, passing the update stage.
        if(engine->selfUpdate(engine->GetConfig()->IsSelfUpdating()))
            return;
    }

    // Begin update checking!
    printf("Checking for updates:\n");
    engine->checkForUpdates();
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

    // Run the update process!
    updater->RunUpdate(engine);

    // Terminate updater!
    delete engine;
    delete updater;
    engine = NULL;
    updater = NULL;

    return 0;
}
