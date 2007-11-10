/*
* pslaunch.h - Author: Mike Gist
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

#ifndef __PSLAUNCH_H__
#define __PSLAUNCH_H__

#include <csutil/threading/thread.h>

#include "updaterconfig.h"
#include "updaterengine.h"

#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "util/genericevent.h"

class pawsMessageTextBox;

#define APPNAME "PlaneShift Launcher"

struct iObjectRegistry;

using namespace CS::Threading;

class psLauncherGUI : public Runnable
{
private:

    iObjectRegistry* object_reg;
    csRef<iVFS> vfs;
    csRef<iConfigManager> configManager;
    csRef<iEngine>          engine;
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iEventQueue>      queue;
    DeclareGenericEventHandler(EventHandler, psLauncherGUI, "planeshift.launcher");
    csRef<EventHandler> event_handler;

    // PAWS
    PawsManager*    paws;
    pawsMainWidget* mainWidget;
    
    pawsMessageTextBox* textBox;
    
    /* Array to store console output. */
    csArray<csString> *consoleOut;

    /* Set to true if we want the GUI to exit. */
    bool *exitGUI;

    /* Set to true if we need to tell the GUI that an update is pending. */
    bool *updateNeeded;

    /* If true, then it's okay to perform the update. */
    bool *performUpdate;

    /* Set to true to launch the client. */
    bool *execPSClient;

    CS::Threading::Mutex *mutex;
    
    /* keeps track of whether the window is visible or not. */
    bool drawScreen;

    /* Limits the frame rate either by sleeping. */
    void FrameLimit();

    /* Time when the last frame was drawn. */
    csTicks elapsed;

    /* Load the app */
    bool InitApp();

    /* Handles an event from the event handler */
    bool HandleEvent (iEvent &ev);
    
    void HandleData();
        
public:
    /* Quit the application */
    void Quit();

    void ExecClient(bool value) { *execPSClient = value; }

    psLauncherGUI(iObjectRegistry* _object_reg, bool *_exitGUI, bool *_updateNeeded, bool *_performUpdate, bool *_execPSClient, csArray<csString> *_consoleOut,  CS::Threading::Mutex *_mutex);

    // Run thread.
    void Run();
    
};

#endif // __PSLAUNCH_H__
