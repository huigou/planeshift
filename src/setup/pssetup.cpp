/*
* pssetup.cpp
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pssetup.h"

#include <iutil/cfgmgr.h>
#include <csutil/event.h>
#include <iutil/eventq.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/vfs.h>
#include <iutil/virtclk.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>
#include <iengine/engine.h>

#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "pawssetupwindow.h"
#include "pawsskinwindow.h"

#include "util/pscssetup.h"

const char* psSetupApp::CONFIG_FILENAME = "/this/pssetup.cfg";
const char* psSetupApp::APP_NAME        = "planeshift.setup.application";
const char* psSetupApp::WINDOW_CAPTION  = "PlaneShift Setup Application";

psSetupApp *setupApp;

CS_IMPLEMENT_APPLICATION

psSetupApp::psSetupApp(iObjectRegistry *obj_reg)
{
    object_reg  = obj_reg;
    paws        = NULL;
    drawScreen  = true;
}

psSetupApp::~psSetupApp()
{
    if (event_handler && queue)
        queue->RemoveListener(event_handler);

    delete paws;
}

void psSetupApp::SevereError(const char* msg)
{
    csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, APP_NAME, msg);
}

bool psSetupApp::Init()
{
    queue =  csQueryRegistry<iEventQueue> (object_reg);
    if (!queue)
    {
        SevereError("No iEventQueue plugin!");
        return false;
    }

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        SevereError("No iVFS plugin!");
        return false;
    }

    engine =  csQueryRegistry<iEngine> (object_reg);
    if (!engine)
    {
        SevereError("No iEngine plugin!");
        return false;
    }

    cfgmgr =  csQueryRegistry<iConfigManager> (object_reg);
    if (!cfgmgr)
    {
        SevereError("No iConfigManager plugin!");
        return false;
    }

    g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
    {
        SevereError("No iGraphics3D plugin!");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        SevereError("Could not load iGraphics2D!");
        return false;
    }
    vc =  csQueryRegistry<iVirtualClock> (object_reg);
    if (!vc)
    {
        SevereError("No iVirtualClock plugin!");
        return false;
    }

    // set the window caption
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (nw)
        nw->SetTitle(WINDOW_CAPTION);
    g3d->GetDriver2D()->AllowResize(false);

    // Mount the skin
    if ( !vfs->Mount ("/planeshift/", "$^") )
        return false;

    // paws initialization
    csString skinPath;
    skinPath = cfgmgr->GetStr("PlaneShift.GUI.Skin", "/planeshift/art/apps.zip");
    paws = new PawsManager(object_reg, skinPath);
    if (!paws)
    {
        SevereError("Could not initialize PAWS!");
        return false;
    }
    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    RegisterFactories();

    // Load and assign a default button click sound for pawsbutton
    paws->LoadSound("/planeshift/art/music/gui/ccreate/next.wav","sound.standardButtonClick");

    if (!LoadWidgets())
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Some PAWS widgets failed to load");
    }

    pawsWidget* setup = paws->FindWidget("setup");
    setup->SetBackgroundAlpha(0);

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
    // Inform debug that everything initialized succesfully
    csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
        "Application initialized successfully.");

    return true;
}

bool psSetupApp::LoadWidgets()
{
    bool succeeded = true;

    if (!paws->LoadWidget("data/gui/setupwindow.xml"))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Loading 'data/gui/setupwindow.xml' failed!");
        succeeded = false;
    }
    
    if (!paws->LoadWidget("data/gui/skinwindow.xml"))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Loading 'data/gui/skinwindow.xml' failed!");
        succeeded = false;
    }

    if (!paws->LoadWidget("data/gui/ok.xml"))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Loading 'data/gui/ok.xml' failed!");
        succeeded = false;
    }

    return succeeded;
}

bool psSetupApp::HandleEvent (iEvent &ev)
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
            g3d->BeginDraw(CSDRAW_2DGRAPHICS);
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

void psSetupApp::FrameLimit()
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

void psSetupApp::RegisterFactories()
{
    pawsWidgetFactory* factory;
    
    factory = new pawsSetupWindowFactory();
    factory = new pawsSkinWindowFactory();
}

void psSetupApp::Quit()
{
    queue->GetEventOutlet()->Broadcast(csevQuit (object_reg));
}


/** Application entry point
 */
int main (int argc, char *argv[])
{
    psCSSetup *CSSetup = new psCSSetup(argc, argv, psSetupApp::CONFIG_FILENAME, 0);
    iObjectRegistry *object_reg = CSSetup->InitCS();
    
    setupApp = new psSetupApp(object_reg);

    // Initialize application
    if (!setupApp->Init())
    {
        setupApp->SevereError("Failed to init app!");
        PS_PAUSEEXIT(1);
    }

    if(argc > 1 && !strcmp(argv[1],"advanced"))
    {
        pawsSetupWindow* wnd = (pawsSetupWindow*)PawsManager::GetSingleton().FindWidget("setup");
        if(wnd)
            wnd->AdvancedMode(true);
    }

    // start the main event loop
    csDefaultRunLoop(object_reg);

    // clean up
    delete setupApp;
    delete CSSetup;

    csInitializer::DestroyApplication(object_reg);
    return 0;
}
