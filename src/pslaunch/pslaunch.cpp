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

#ifndef CS_PLATFORM_WIN32
#include <sys/wait.h>
#endif

#include <iutil/eventq.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include "download.h"
#include "globals.h"
#include "pslaunch.h"
#include "pawslauncherwindow.h"
#include "updater.h"

#include "paws/pawsbutton.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawsmanager.h"
#include "paws/pawsokbox.h"
#include "paws/pawstextbox.h"
#include "util/log.h"

CS_IMPLEMENT_APPLICATION

psLauncherGUI* psLaunchGUI;

using namespace CS::Threading;
    
psLauncherGUI::psLauncherGUI(iObjectRegistry* _object_reg, InfoShare *_infoShare, bool *_execPSClient)
{
    object_reg = _object_reg;
    infoShare = _infoShare;
    execPSClient = _execPSClient;
    
    drawScreen = true;
    elapsed = 0;
    psLaunchGUI = this;
    fileUtil = NULL;
    downloader = NULL;
    paws = NULL;
    updateTold = false;
}

void psLauncherGUI::Run()
{
    if(InitApp())
        csDefaultRunLoop(object_reg);

    delete paws;
    paws = NULL;
    delete downloader;
    downloader = NULL;
    delete fileUtil;
    fileUtil = NULL;

    csInitializer::CloseApplication(object_reg);
}

bool psLauncherGUI::InitApp()
{
    pslog::Initialize(object_reg);

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

    // Initialise downloader.
    downloader = new Downloader(vfs);

    // Initialise file utilities.
    fileUtil = new FileUtil(vfs);

    if(!csInitializer::OpenApplication(object_reg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    iNativeWindow *nw = g2d->GetNativeWindow();
    if (nw)
      nw->SetTitle(APPNAME);

    g2d->AllowResize(false);

    // paws initialization
    csString skinPath;
    skinPath = configManager->GetStr("PlaneShift.GUI.Skin", "/planeshift/art/pslaunch.zip");
    paws = new PawsManager(object_reg, skinPath);
    if (!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Register factory
    launcherWidget = new pawsLauncherWindowFactory();

    // Load and assign a default button click sound for pawsbutton
    paws->LoadSound("/planeshift/art/sounds/gui/next.wav","sound.standardButtonClick");

    // Load widgets
    if (!paws->LoadWidget("data/gui/pslaunch.xml"))
    {
        printf("Warning: Loading 'data/gui/pslaunch.xml' failed!");
        return false;
    }

    pawsWidget* launcher = paws->FindWidget("Launcher");
    launcher->SetBackgroundAlpha(0);

    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    event_handler = csPtr<EventHandler> (new EventHandler (this));
    csEventID esub[] = 
    {
        csevFrame (object_reg),
        csevMouseEvent (object_reg),
        csevKeyboardEvent (object_reg),
        csevQuit (object_reg),
        CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    return true;
}

bool psLauncherGUI::HandleEvent (iEvent &ev)
{
    if (ev.Name == csevQuit (object_reg))
    {
      if(!infoShare->GetExitGUI())
      {
        infoShare->SetCancelUpdater(true);
      }
      return false;
    }

    if(infoShare->GetExitGUI())
    {
        Quit();
        return false;
    }
    
    if(infoShare->GetCheckIntegrity())
    {
        pawsMessageTextBox* updateProgressOutput = (pawsMessageTextBox*)paws->FindWidget("UpdaterOutput");
        while(!infoShare->ConsoleIsEmpty())
        {
            csString message = infoShare->ConsolePop();
            if(message.FindFirst("\n") == 0)
            {
                updateProgressOutput->AddMessage(message);
            }
            else
            {
                updateProgressOutput->AppendLastMessage(message);
            }
        }
        if(infoShare->GetUpdateNeeded())
        {
            pawsButton* yes = (pawsButton*)paws->FindWidget("UpdaterYesButton");
            pawsButton* no = (pawsButton*)paws->FindWidget("UpdaterNoButton");
            yes->Show();
            no->Show();
        }
    }
    else if(infoShare->GetPerformUpdate())
    {
        if(!infoShare->GetUpdateNeeded())
        {
            infoShare->SetPerformUpdate(false);
            infoShare->Sync();
        }

        pawsMessageTextBox* updateProgressOutput = (pawsMessageTextBox*)paws->FindWidget("UpdaterOutput");
        while(!infoShare->ConsoleIsEmpty())
        {
            csString message = infoShare->ConsolePop();
            if(message.FindFirst("\n") == 0)
            {
                updateProgressOutput->AddMessage(message);
            }
            else
            {
                updateProgressOutput->AppendLastMessage(message);
            }
        }
    }
    else if(paws->FindWidget("LauncherUpdater")->IsVisible())
    {
        paws->FindWidget("UpdaterOkButton")->Show();
        paws->FindWidget("UpdaterCancelButton")->Hide();
    }
    else if(!updateTold)
    {
        if(infoShare->GetUpdateAdminNeeded())
        {
            pawsOkBox* notify = (pawsOkBox*)paws->FindWidget("Notify");
            notify->SetText("An update is available but you don't have the correct permissions to continue!\n\n"
                            "Please restart the program as an admin.");
            notify->Show();
            updateTold = true;
        }
        else if(infoShare->GetUpdateNeeded())
        {
            paws->FindWidget("UpdateAvailable")->Show();
            updateTold = true;
        }
    }

    if (paws->HandleEvent(ev))
        return true;

    if (ev.Name == csevFrame (object_reg))
    {
        if (drawScreen)
        {
            FrameLimit();
            g3d->BeginDraw(CSDRAW_2DGRAPHICS);
            paws->Draw();
        }
        else
        {
            csSleep(150);
        }

        g3d->FinishDraw ();
        g3d->Print (NULL);
        return true;
    }
    else if (ev.Name == csevCanvasHidden (object_reg, g2d))
    {
        drawScreen = false;
    }
    else if (ev.Name == csevCanvasExposed (object_reg, g2d))
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
    infoShare->SetExitGUI(true);
}

void psLauncherGUI::PerformUpdate(bool update, bool integrity)
{
    infoShare->SetPerformUpdate(update || integrity);
    infoShare->SetUpdateNeeded(update);
    infoShare->Sync();

    if(update && !infoShare->GetExitGUI())
    {
      paws->FindWidget("LauncherMain")->Hide();
      paws->FindWidget("LauncherUpdater")->Show();
      paws->FindWidget("LauncherUpdater")->OnGainFocus();
      paws->FindWidget("UpdaterCancelButton")->Show();
    }
}

void psLauncherGUI::PerformRepair()
{
    infoShare->EmptyConsole();
    if(infoShare->GetCheckIntegrity())
    {
        csSleep(500);
    }
    infoShare->SetCheckIntegrity(true);
}

int main(int argc, char* argv[])
{
    // Select between GUI and console mode.
    bool console = false;
    bool help = false;
    for(int i=0; i<argc; i++)
    {
        csString s(argv[i]);
        if(s.CompareNoCase("--console") || s.CompareNoCase("-console") ||
           s.CompareNoCase("--switch") || s.CompareNoCase("-switch") ||
           s.CompareNoCase("--repair") || s.CompareNoCase("-repair"))
        {
            console = true;
        }
        else if(s.CompareNoCase("--help") || s.CompareNoCase("-help"))
        {
            help = true;
        }
    }

    if(help)
    {
        // Create new string array to make sure --help is there.
        const char** argvs = new const char*[argc+1];
        for(int i=0; i<argc; i++)
        {
            argvs[i] = argv[i];
        }
        argvs[argc] = "--help";

        // Set up CS
        psUpdater* updater = new psUpdater(argc+1, argvs);

        // Convert args to an array of csString.
        csStringArray args;
        for(int i=0; i<argc+1; i++)
        {
            args.Push(argvs[i]);
        }

        // Initialize updater engine.
        UpdaterEngine* engine = new UpdaterEngine(args, updater->GetObjectRegistry(), "pslaunch");

        printf("PlaneShift Updater Version %1.2f for %s.\n"
               "Launcher and updater for Planeshift\n\n"
               "pslaunch [--help] [--console] [--repair] [--switch]\n\n"
               "--help      Displays this help dialog\n"
               "--console   Run updater without the GUI\n"
               "--switch    Switch active updater mirror\n"
               "--repair    Check for any problems and prompt to repair them\n", 
               UPDATER_VERSION, engine->GetConfig()->GetCurrentConfig()->GetPlatform());

        // Terminate updater!
        delete argvs;
        delete engine;
        delete updater;
        engine = NULL;
        updater = NULL;
    }
    else if(console)
    {
        // Create new string array to make sure --console is there.
        const char** argvs = new const char*[argc+1];
        for(int i=0; i<argc; i++)
        {
            argvs[i] = argv[i];
        }
        argvs[argc] = "--console";

        // Set up CS
        psUpdater* updater = new psUpdater(argc+1, argvs);

        // Convert args to an array of csString.
        csStringArray args;
        for(int i=0; i<argc+1; i++)
        {
            args.Push(argvs[i]);
        }

        // Initialize updater engine.
        UpdaterEngine* engine = new UpdaterEngine(args, updater->GetObjectRegistry(), "pslaunch");

        printf("\nPlaneShift Updater Version %1.2f for %s.\n\n", UPDATER_VERSION, engine->GetConfig()->GetCurrentConfig()->GetPlatform());

        // Run the update process!
        updater->RunUpdate(engine);

        // Maybe this fixes a bug.
        fflush(stdout);

        if(!engine->GetConfig()->IsSelfUpdating())
        {
#if defined(WIN32) && defined(NDEBUG)
            printf("\nUpdater finished!\n");
#else
            printf("\nUpdater finished, press enter to exit.\n");
            getchar();
#endif
        }

        // Terminate updater!
        delete argvs;
        delete engine;
        delete updater;
        engine = NULL;
        updater = NULL;
    }
    else
    {
        bool exitApp = false;
        while(!exitApp)
        {
            // Set up CS
            iObjectRegistry* object_reg = csInitializer::CreateEnvironment (argc, argv);

            if(!object_reg)
            {
                printf("Object Reg failed to Init!\n");
                exit(1);
            }

            // Request needed plugins for updater.
            csInitializer::SetupConfigManager(object_reg, LAUNCHER_CONFIG_FILENAME);
            csInitializer::RequestPlugins(object_reg, CS_REQUEST_VFS, CS_REQUEST_END);

            // Convert args to an array of csString.
            csStringArray args;
            for(int i=0; i<argc; i++)
            {
                args.Push(argv[i]);
            }

            // Mount the VFS paths.
            csRef<iVFS> vfs = csQueryRegistry<iVFS>(object_reg);
            if (!vfs->Mount ("/planeshift/", "$^"))
            {
                printf("Failed to mount /planeshift!\n");
                return false;
            }

            // Set config path.
            csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
            csString configPath = csGetPlatformConfigPath("PlaneShift");
            configPath.ReplaceAll("/.crystalspace/", "/.");
            configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);

            // Check that the path exists, else attempt to create it.
            FileUtil* fileUtil = new FileUtil(vfs);
            csRef<FileStat> filestat = fileUtil->StatFile(configPath);
            if (!filestat.IsValid() && CS_MKDIR(configPath) < 0)
            {
                printf("Could not create required %s directory!\n", configPath.GetData());
                return false;
            }
            filestat.Invalidate();
            delete fileUtil;
            fileUtil = NULL;

            // Mount config path.
            if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
            {
                printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
                return false;
            }

            // Init thread communication structure.
            InfoShare *infoShare = new InfoShare();
            infoShare->SetPerformUpdate(false);
            infoShare->SetUpdateNeeded(false);

            // Initialize updater engine.
            csRef<UpdaterEngine> engine;
            engine.AttachNew(new UpdaterEngine(args, object_reg, "pslaunch", infoShare));

            // If we're self updating, continue self update.
            if(engine->GetConfig()->IsSelfUpdating())
            {
                exitApp = engine->SelfUpdate(engine->GetConfig()->IsSelfUpdating());
            }

            // Set to true by GUI if we have to launch the client.
            bool execPSClient = false;

            // If we don't have to exit the app, create updater thread and run the GUI.
            if(!exitApp)
            {
                // Start up updater.
                csRef<Thread> updaterThread;
                updaterThread.AttachNew(new Thread(engine));
                updaterThread->Start();

                // Request needed plugins for GUI.
                csInitializer::RequestPlugins(object_reg, CS_REQUEST_FONTSERVER, CS_REQUEST_IMAGELOADER,
                    CS_REQUEST_OPENGL3D, CS_REQUEST_END);

                // Start GUI.
                psLauncherGUI* gui = new psLauncherGUI(object_reg, infoShare, &execPSClient);
                gui->Run();

                // Free GUI.
                delete gui;
            }

            // Set to exit app.
            exitApp = true;

            // Clean up everything else.
            engine.Invalidate();
            delete infoShare;
            configManager.Invalidate();
            vfs.Invalidate();
            csInitializer::DestroyApplication(object_reg);

            if (execPSClient)
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

              csString commandLine = "psclient.exe";

              for(int i=1; i<argc; ++i)
              {
                  commandLine.AppendFmt(" %s", argv[i]);
              }

              CreateProcess(NULL, (LPSTR)commandLine.GetData(), 0, 0, false,
                CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
              GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
              while (dwExitCode == STILL_ACTIVE)
              {
                csSleep(1000);
                GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
              }
              exitApp = dwExitCode ? 0 : !0;
              CloseHandle(piProcessInfo.hProcess);
              CloseHandle(piProcessInfo.hThread);
#else
              if(fork() == 0)
              {
#ifdef CS_PLATFORM_MACOSX
                  char* nargv[argc+2];
                  char* name = "/usr/bin/open";
                  char* psc = "psclient.app";
                  nargv[0] = name;
                  nargv[1] = psc;
                  for(int i=2; i<argc+1; ++i)
                  {
                      nargv[i] = argv[i-1];
                  }
                  nargv[argc+1] = (char*)0;
                  execv("/usr/bin/open", nargv); 
#else
                  char* nargv[argc+1];
                  char* name = const_cast<char*>("./psclient");
                  nargv[0] = name;
                  for(int i=1; i<argc; ++i)
                  {
                      nargv[i] = argv[i];
                  }
                  nargv[argc] = (char*)0;
                  execv("./psclient", nargv);  
#endif
              }                
              else
              {
                  int status;
                  wait(&status);
                  exitApp = (status == 0);
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
