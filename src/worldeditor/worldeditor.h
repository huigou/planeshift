/*
* worldeditor.h - Author: Mike Gist
*
* Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/ref.h>
#include "util/genericevent.h"

class pawsMainWidget;
class PawsManager;
struct iObjectRegistry;
struct iSceneManipulate;
struct iView;

class WorldEditor
{
public:
    WorldEditor(int argc, char* argv[]);
    ~WorldEditor();

    void Run();

private:
    /* Handles an event from the event handler */
    bool HandleEvent (iEvent &ev);

    /* Init plugins, paws, world etc. */
    bool Init();

    // CS
    iObjectRegistry* objectReg;
    csRef<iView> view;
    DeclareGenericEventHandler(EventHandler, WorldEditor, "worldeditor");

    // PS
    PawsManager* paws;
    pawsMainWidget* mainWidget;
    csRef<iSceneManipulate> sceneManipulator;

    // Event ids.
    csEventID MouseMove;
    csEventID MouseDown;
    csEventID FrameEvent;
};
