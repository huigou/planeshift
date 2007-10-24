/*
* pslaunch.cpp - Author: Mike Gist
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

#include <psconfig.h>

#ifndef CS_COMPILER_MSVC
#include <sys/wait.h>
#endif

#include <iutil/eventq.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include "globals.h"
#include "pslaunch.h"
#include "pawslauncherwindow.h"

CS_IMPLEMENT_APPLICATION

psLauncherGUI* psLaunchGUI;

psLauncherGUI::psLauncherGUI(iObjectRegistry* _object_reg, bool *_exitGUI, bool *_updateNeeded, bool *_performUpdate, bool *_execPSClient, csArray<csString> *_consoleOut)
{
    exitGUI = _exitGUI;
    updateNeeded = _updateNeeded;
    performUpdate = _performUpdate;
    execPSClient = _execPSClient;
    consoleOut = _consoleOut;
    object_reg = _object_reg;
    psLaunchGUI = this;
    paws = NULL;
}

void psLauncherGUI::Run()
{
    if(InitApp())
        csDefaultRunLoop(object_reg);

    delete paws;
    paws = NULL;

    csInitializer::CloseApplication(object_reg);
}

bool psLauncherGUI::InitApp()
{
    vfs = csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        printf("vfs failed to Init!\n");
        return false;
    }

    configManager = csQueryRegistry<iConfigManager> (object_reg);
    if (!configManager)
    {
        printf("configManager failed to Init!\n");
        return false;
    }

    engine = csQueryRegistry<iEngine> (object_reg);
    if (!engine)
    {
        printf("No iEngine plugin!\n");
        return false;
    }

    queue = csQueryRegistry<iEventQueue> (object_reg);
    if (!queue)
    {
        printf("No iEventQueue plugin!\n");
        return false;
    }

    g3d = csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
    {
        printf("iGraphics3D failed to Init!\n");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        printf("GetDriver2D failed to Init!\n");
        return false;
    }
    
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (nw)
        nw->SetTitle(APPNAME);
    g3d->GetDriver2D()->AllowResize(false);

    if(!csInitializer::OpenApplication(object_reg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    // Mount the skin
    if (!vfs->Mount ("/planeshift/", "$^"))
    {
        printf("Failed to mount skin!\n");
        return false;
    }

    // paws initialization
    csString skinPath;
    skinPath = configManager->GetStr("Planeshift.GUI.Skin", "/planeshift/art/apps.zip");
    paws = new PawsManager(object_reg, skinPath);
    if (!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Register factory
    pawsWidgetFactory* factory = new pawsLauncherWindowFactory;

    // Load and assign a default button click sound for pawsbutton
    paws->LoadSound("/planeshift/art/music/gui/ccreate/next.wav","sound.standardButtonClick");

    // Load widgets
    if (!paws->LoadWidget("data/gui/launcherwindow.xml"))
    {
        printf("Warning: Loading 'data/gui/launcherwindow.xml' failed!");
        return false;
    }

    pawsWidget* launcher = paws->FindWidget("launcher");
    launcher->SetBackgroundAlpha(0);

    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    event_handler = csPtr<EventHandler> (new EventHandler (this));
    csEventID esub[] = {
	  csevPreProcess (object_reg),
	  csevProcess (object_reg),
	  csevPostProcess (object_reg),
	  csevFinalProcess (object_reg),
	  csevFrame (object_reg),
	  csevMouseEvent (object_reg),
	  csevKeyboardEvent (object_reg),
	  CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    return true;
}

bool psLauncherGUI::HandleEvent (iEvent &ev)
{
    if (paws->HandleEvent(ev))
        return true;

    if (ev.Name == csevProcess (object_reg))
        return true;
    else if (ev.Name == csevFinalProcess (object_reg))
    {
	g3d->FinishDraw ();
	g3d->Print (NULL);
	return true;
    }
    else if (ev.Name == csevPostProcess (object_reg))
    {
	if (drawScreen)
        {
	        FrameLimit();
            g3d->BeginDraw(engine->GetBeginDrawFlags() | CSDRAW_2DGRAPHICS);
            paws->Draw();
        }
        else
        {
            csSleep(150);
        }
    }
    else if (ev.Name == csevCanvasHidden (object_reg, g3d->GetDriver2D ()))
    {
	drawScreen = false;
    }
    else if (ev.Name == csevCanvasExposed (object_reg, g3d->GetDriver2D ()))
    {
	drawScreen = true;
    }
    return false;
}

void psLauncherGUI::FrameLimit()
{
    csTicks sleeptime;
    csTicks elapsedTime = csGetTicks() - elapsed;

    // Define sleeptime to limit fps to around 30
    sleeptime = 30;

    // Here we sacrifice drawing time
    if(elapsedTime < sleeptime)
        csSleep(sleeptime - elapsedTime);

    elapsed = csGetTicks();
}

void psLauncherGUI::Quit()
{
    queue->GetEventOutlet()->Broadcast(csevQuit (object_reg));
}

int main(int argc, char* argv[])
{
    // Set to true to exit the app.
    bool exitApp = false;
    while(!exitApp)
    {
        // Set up CS
        iObjectRegistry* object_reg = csInitializer::CreateEnvironment (argc, argv);
        if(!object_reg)
        {
            printf("Object Reg failed to Init!\n");
            PS_PAUSEEXIT(1);
        }

        // Request needed plugins for updater.
        csInitializer::RequestPlugins(object_reg, CS_REQUEST_VFS, CS_REQUEST_END);

        // Convert args to an array of csString.
        csArray<csString> args;
        for(int i=0; i<argc; i++)
        {
            args.Push(argv[i]);
        }

        // Set to true to tell the GUI to exit.
        bool exitGUI = false;
        // Set to true to tell the GUI that an update is required.
        bool updateNeeded = false;
        // Set to true to tell the updater that it's okay to perform the update.
        bool performUpdate = false;
        // Console output.
        csArray<csString> consoleOut;

        // Initialize updater engine.
        psUpdaterEngine* engine = new psUpdaterEngine(args, object_reg, "pslaunch", &exitGUI, &updateNeeded, &performUpdate, &consoleOut);

        // If we're self updating, continue self update.
        if(engine->GetConfig()->IsSelfUpdating())
        {
            exitApp = engine->selfUpdate(engine->GetConfig()->IsSelfUpdating());
        }

        // If we don't have to exit the app, create GUI thread and run updater.
        if(!exitApp)
        {
            // Set to true by GUI if we have to launch the client.
            bool execPSClient = false;
            // Ping stuff.

            // Request needed plugins for GUI.
            csInitializer::RequestPlugins(object_reg, CS_REQUEST_FONTSERVER, CS_REQUEST_IMAGELOADER,
                                          CS_REQUEST_OPENGL3D, CS_REQUEST_ENGINE, CS_REQUEST_VFS, CS_REQUEST_END);

            // Start up GUI.
            csRef<Runnable> gui;
            gui.AttachNew(new psLauncherGUI(object_reg, &exitGUI, &updateNeeded, &performUpdate, &execPSClient, &consoleOut));
            csRef<Thread> guiThread;
            guiThread.AttachNew(new Thread(gui));
            guiThread->Start();

            // Begin update checking.
            if(engine)
                engine->checkForUpdates();

            // Wait for the gui to exit.
            while(guiThread->IsRunning())
            {
                csSleep(1000);
                // Update Ping.
            }

            // Free updater.
            delete engine;
            engine = NULL;

            // Free the GUI.
            guiThread = NULL;
            gui = NULL;

            // Clean up everything else.
            csInitializer::DestroyApplication(object_reg);
            object_reg = NULL;

            if(execPSClient)
            {
                // Execute psclient process.

#ifdef CS_PLATFORM_WIN32

                // Info for CreateProcess.
                STARTUPINFO siStartupInfo;
                DWORD dwExitCode;
                PROCESS_INFORMATION piProcessInfo;
                memset(&siStartupInfo, 0, sizeof(siStartupInfo));
                memset(&piProcessInfo, 0, sizeof(piProcessInfo));
                siStartupInfo.cb = sizeof(siStartupInfo);

                CreateProcess(NULL, "psclient.exe", 0, 0, false,
                    CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
                GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
                while (dwExitCode == STILL_ACTIVE)
                {
                    csSleep(1000);
                    GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
                }
                exitApp = dwExitCode ? 0 : !0;
#else
                if(fork() == 0)
                {
                    execl("psclient", "psclient", NULL, NULL);
                }
                else
                {
                    int status;
                    wait(&status);
                    exitApp = status ? 0 : !0;
                }
#endif
            }
            else
            {
                exitApp = true;
            }
        }
    }

    return 0;
}
