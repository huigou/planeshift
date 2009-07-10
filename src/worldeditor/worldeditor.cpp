/*
* worldeditor.cpp - Author: Mike Gist
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

#include <psconfig.h>

#include <cstool/csview.h>
#include <cstool/enginetools.h>
#include <cstool/genmeshbuilder.h>
#include <cstool/initapp.h>
#include <iengine/sector.h>
#include <iutil/cfgmgr.h>
#include <iutil/eventq.h>
#include <ivaria/view.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include "iclient/ibgloader.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "util/log.h"

#include "worldeditor.h"

#define APPNAME "PlaneShift World Editor"
#define WEDIT_CONFIG_FILENAME "/this/worldeditor.cfg"

WorldEditor::WorldEditor(int argc, char* argv[]) : paws(0)
{
    objectReg = csInitializer::CreateEnvironment(argc, argv);
    csInitializer::SetupConfigManager(objectReg, WEDIT_CONFIG_FILENAME);
    csInitializer::RequestPlugins(objectReg, CS_REQUEST_FONTSERVER,
        CS_REQUEST_IMAGELOADER, CS_REQUEST_OPENGL3D, CS_REQUEST_END);
}

WorldEditor::~WorldEditor()
{
    view.Invalidate();
    csInitializer::DestroyApplication(objectReg);
}

void WorldEditor::Run()
{
    if(Init())
    {
        // Hand over control to CS.
        csDefaultRunLoop(objectReg);
    }

    if(paws)
    {
        // Delete paws.
        delete paws;
        paws = 0;
    }

    // Close window.
    csInitializer::CloseApplication(objectReg);
}

bool WorldEditor::Init()
{
    pslog::Initialize(objectReg);

    csRef<iVFS> vfs = csQueryRegistry<iVFS> (objectReg);
    if (!vfs)
    {
        printf("vfs failed to Init!\n");
        return false;
    }

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (objectReg);
    if (!configManager)
    {
        printf("configManager failed to Init!\n");
        return false;
    }

    csRef<iEventQueue> queue = csQueryRegistry<iEventQueue> (objectReg);
    if (!queue)
    {
        printf("No iEventQueue plugin!\n");
        return false;
    }

    csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D> (objectReg);
    if (!g3d)
    {
        printf("iGraphics3D failed to Init!\n");
        return false;
    }

    csRef<iGraphics2D> g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        printf("GetDriver2D failed to Init!\n");
        return false;
    }

    csRef<iEngine> engine = csQueryRegistry<iEngine> (objectReg);
    if (!engine)
    {
        printf("iEngine failed to Init!\n");
        return false;
    }

    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(objectReg);
    if(!loader.IsValid())
    {
        printf("Failed to load iBgLoader!\n");
        return false;
    }

    if(!csInitializer::OpenApplication(objectReg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    iNativeWindow *nw = g2d->GetNativeWindow();
    if (nw)
      nw->SetTitle(APPNAME);

    // Paws initialization
    vfs->Mount("/planeshift/", "$^");
    csString skinPath = configManager->GetStr("PlaneShift.GUI.Skin.Base","/planeshift/art/skins/base/client_base.zip");
    paws = new PawsManager(objectReg, skinPath);
    if (!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Load and assign a default button click sound for pawsbutton
    paws->LoadSound("/planeshift/art/sounds/gui/next.wav","sound.standardButtonClick");

    // Set mouse image.
    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    csRef<EventHandler> event_handler = csPtr<EventHandler> (new EventHandler (this));
    csEventID esub[] = 
    {
        csevFrame (objectReg),
        csevMouseEvent (objectReg),
        csevKeyboardEvent (objectReg),
        csevQuit (objectReg),
        CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    // Set up view.
    view.AttachNew(new csView(engine, g3d));
    view->SetRectangle(0, 0, g2d->GetWidth(), g2d->GetHeight());

    // Create blackbox world.
    iSector* blackbox = engine->CreateSector("BlackBox");
    blackbox->SetDynamicAmbientLight(csColor(1.0f, 1.0f, 1.0f));
    view->GetCamera()->SetSector(blackbox);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0.0f, 1.0f, 0.0f));

    // Create the base geometry.
    using namespace CS::Geometry;
    TesselatedBox box (csVector3 (-CS_BOUNDINGBOX_MAXVALUE, 0, -CS_BOUNDINGBOX_MAXVALUE),
                       csVector3 (CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE));
    box.SetFlags (Primitives::CS_PRIMBOX_INSIDE);

    // Now we make a factory and a mesh at once.
    csRef<iMeshWrapper> bbox = GeneralMeshBuilder::CreateFactoryAndMesh (
        engine, blackbox, "blackbox", "blackbox_factory", &box);

    // Create and set material.
    bbox->GetMeshObject ()->SetMaterialWrapper(engine->CreateMaterial("black", engine->CreateBlackTexture("black", 2, 2, 0, 0)));

    // Prepare engine.
    engine->Prepare();

    return true;
}

bool WorldEditor::HandleEvent (iEvent &ev)
{
    if(ev.Name == csevFrame (objectReg))
    {
        view->Draw();
    }

    if(ev.Name == csevMouseDown (PawsManager::GetSingleton().GetEventNameRegistry(), 0))
    {
      psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
      printf("Point: %i, %i\n", p.x, p.y);
      csScreenTargetResult result = csEngineTools::FindScreenTarget(csVector2(p.x, p.y), 1000, view->GetCamera());
      printf("Hit at: %s\n", result.isect.Description().GetData());
    }
    
    return paws->HandleEvent(ev);
}
