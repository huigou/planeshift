/*
 * pawsobjectview.cpp - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/weakref.h>

#include <iutil/object.h>
#include <iutil/objreg.h>
#include <imap/loader.h>
#include <iutil/vfs.h>
#include <iengine/texture.h>
#include <csutil/cscolor.h>
#include "pawsobjectview.h"
#include "pawsmanager.h"
#include "engine/psworld.h"
#include "util/log.h"
#include "util/psconst.h"

int pawsObjectView::idName = 0;

pawsObjectView::pawsObjectView()
{
    engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    object = NULL;
    ID = 0;

    idName++;
    realName.Format("NAME%d", idName);

    rotateTime = orgTime = 0;
    rotateRadians = orgRadians = 0;
    camRotate = 0.0f;
    objectPos = csVector3(0,0,0); // Center of podium
    cameraMod = csVector3(0,0,0);

    loadedMap = false;
    spinMouse = false;
    mouseControlled = false;
    doRotate = true;
    mouseDownUnlock = false;

    needToFilter = PawsManager::GetSingleton().GetGFXFeatures() != useAll;
}   

pawsObjectView::~pawsObjectView()
{
    engine->RemoveCollection(realName);
    idName--;
}

bool pawsObjectView::Setup(iDocumentNode* node )
{
    csRef<iDocumentNode> distanceNode = node->GetNode( "distance" );
    if ( distanceNode )
        distance = distanceNode->GetAttributeValueAsFloat("value");
    else
        distance = 4;

    if (!resizeToScreen)
        distance *= float(graphics2D->GetWidth())/800.0f;

    csRef<iDocumentNode> cameraModNode = node->GetNode( "cameramod" );
    if ( cameraModNode )
    {
        cameraMod = csVector3(cameraModNode->GetAttributeValueAsFloat("x"),
                              cameraModNode->GetAttributeValueAsFloat("y"),
                              cameraModNode->GetAttributeValueAsFloat("z"));
    }

    csRef<iDocumentNode> mapNode = node->GetNode( "map" );
    if ( mapNode )
    {
        csString mapFile = mapNode->GetAttributeValue("file");
        csString sector  = mapNode->GetAttributeValue("sector");

        return LoadMap( mapFile, sector );
    }
    else
    {
        Error1("pawsObjectView map failed to load because the mapNode doesn't exist!\n");
        return false;
    }
}

csRef<iDocumentNode> pawsObjectView::Filter(csRef<iDocumentNode> world)
{
    if(!(PawsManager::GetSingleton().GetGFXFeatures() & useAdvancedShaders))
    {
        csRef<iDocumentNodeIterator> sectors = world->GetNodes("sector");
        while(sectors->HasNext())
        {
            csRef<iDocumentNode> sector = sectors->Next();
            csRef<iDocumentNode> rloop = sector->GetNode("renderloop");
            if(rloop.IsValid())
            {
                csString value = rloop->GetContentsValue();
                if(value.Compare("std_rloop_diffuse"))
                {
                    sector->RemoveNode(rloop);
                }
            }
        }
    }

    return world;
}

bool pawsObjectView::LoadMap( const char* map, const char* sector )
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iThreadedLoader> loader =  csQueryRegistry<iThreadedLoader> ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iVFS> VFS =  csQueryRegistry<iVFS> ( PawsManager::GetSingleton().GetObjectRegistry());

    stage = engine->FindSector( sector );

    // Create/Get collection and clear it in case it already existed.
    iCollection* col = engine->CreateCollection(realName);

    if ( !stage )
    {
        csRef<iDocumentSystem> xml (
            csQueryRegistry<iDocumentSystem> (PawsManager::GetSingleton().GetObjectRegistry()));

        csRef<iDocument> doc = xml->CreateDocument();
        csString filename = map;
        filename.Append("/world");
        csRef<iDataBuffer> buf (VFS->ReadFile (filename, false));
        doc->Parse(buf);
        csRef<iDocumentNode> worldNode = doc->GetRoot()->GetNode("world");

        if(needToFilter)
        {
            // Filter the world file to get the correct settings.
            worldNode = Filter(worldNode);
        }

        // Now load the map into the selected region
        VFS->ChDir (map);
        VFS->SetSyncDir(VFS->GetCwd());
        engine->SetCacheManager(NULL);
        csRef<iThreadReturn> itr = loader->LoadMap(worldNode, CS_LOADER_KEEP_WORLD, col);
        itr->Wait();
        if (!itr->WasSuccessful())
            return false;

        VFS->ChDir (map);
        engine->SyncEngineListsNow(loader);

        stage = engine->FindSector( sector );
        CS_ASSERT( stage );
        col->Add( stage->QueryObject() );
        if ( !stage )
             return false;
    }

    meshSector = engine->CreateSector( realName );

    iLightList* lightList = meshSector->GetLights();
    csRef<iLight> light = engine->CreateLight(NULL, csVector3(-3,4,-3),10,
                                      csColor(0.86F,0.87F,0.6F), CS_LIGHT_STATIC);
    light->SetAttenuationMode( CS_ATTN_NONE );
    lightList->Add( light );

    meshView = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));
    meshView->GetCamera()->SetSector(meshSector);
    meshView->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));

    meshView->SetRectangle(screenFrame.xmin, screenFrame.ymin,
                       screenFrame.Width(),screenFrame.Height());

    view = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));
    view->GetCamera()->SetSector(stage);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));

    view->SetRectangle(screenFrame.xmin, screenFrame.ymin, screenFrame.Width(), screenFrame.Height());

    loadedMap = true;
    return true;
}

void pawsObjectView::View( const char* factName, const char* fileName )
{
    csRef<iMeshFactoryWrapper> meshfact = 0;
    meshfact = engine->GetMeshFactories()->FindByName (factName);

    if ( !meshfact )
    {
        csRef<iThreadedLoader> loader = csQueryRegistry<iThreadedLoader> (PawsManager::GetSingleton().GetObjectRegistry());
        csRef<iThreadReturn> itr = loader->LoadMeshObjectFactory (fileName);
        itr->Wait();
        meshfact = scfQueryInterfaceSafe<iMeshFactoryWrapper>(itr->GetResultRefPtr());
        engine->SyncEngineListsNow(loader);
    }

    if ( !meshfact )
    {
        Error2("Failed to load mesh factory from file %s", fileName);
        return;
    }

    View(meshfact);
}

void pawsObjectView::View( iMeshFactoryWrapper* wrapper )
{
    Clear();

    if(wrapper)
    {
      iSector* sector = loadedMap ? meshSector : stage;
      object = engine->CreateMeshWrapper (wrapper, "PaperDoll", sector, csVector3(0,0,0) );
    }
}

void pawsObjectView::View( iMeshWrapper* wrapper )
{
    Clear();

    if(wrapper)
    {
      View(wrapper->GetFactory());
    }
}

void pawsObjectView::Rotate(int speed,float radians)
{
    RotateTemp(speed,radians);
    orgRadians= rotateRadians;
    orgTime = rotateTime;
}

void pawsObjectView::Rotate(float radians)
{
    camRotate = radians;
}

void pawsObjectView::RotateDef()
{
    rotateTime = orgTime;
    rotateRadians = orgRadians;
}

void pawsObjectView::RotateTemp(int speed,float radians)
{
    rotateTime = speed;
    rotateRadians = radians;

    if(speed == -1)
    {
        rotateTime = 0; // Don't enter rotate code
        rotateRadians = 0;
        camRotate = 0;
    }
}

void pawsObjectView::Draw()
{
    if ( doRotate )
        DrawRotate();
    else
        DrawNoRotate();        
}

void pawsObjectView::LockCamera( csVector3 where, csVector3 at, bool mouseBreak )
{
    oldPosition = cameraPosition;
    oldLookAt = lookingAt; 
    mouseDownUnlock = mouseBreak;
    
    doRotate = false;
    cameraPosition = where;
    lookingAt = at;
}

void pawsObjectView::UnlockCamera()
{
    cameraPosition = oldPosition;
    lookingAt = oldLookAt;
    
    doRotate = true;
}

void pawsObjectView::DrawNoRotate()
{
    if(screenFrame.xmin > graphics2D->GetWidth() || screenFrame.ymin > graphics2D->GetHeight() ||
       screenFrame.xmax < 0 || screenFrame.ymax < 0)
    {
       return;
    }

    graphics2D->SetClipRect(0, 0, graphics2D->GetWidth(), graphics2D->GetHeight());
    if(!PawsManager::GetSingleton().GetGraphics3D()->BeginDraw(CSDRAW_3DGRAPHICS))
    {
        return;
    }

    if(!view)
    {
        return;
    }

    iGraphics3D* og3d = view->GetContext();

    view->SetContext(PawsManager::GetSingleton().GetGraphics3D());

    view->SetRectangle(screenFrame.xmin,
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(), screenFrame.Height());

    view->GetPerspectiveCamera()->SetPerspectiveCenter((float)(screenFrame.xmin+(screenFrame.Width() >> 1))/graphics2D->GetWidth(),
                                                       1-(float)(screenFrame.ymin+(screenFrame.Height() >> 1))/graphics2D->GetHeight());
       
    view->GetCamera()->GetTransform().SetOrigin(cameraPosition);
    view->GetCamera()->GetTransform().LookAt(lookingAt, csVector3(0, 1, 0));

    view->Draw();

    if ( loadedMap )
    {
        og3d = meshView->GetContext();

        meshView->SetContext(PawsManager::GetSingleton().GetGraphics3D());

        meshView->SetRectangle(screenFrame.xmin,
                               PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                               screenFrame.Width(), screenFrame.Height());

        meshView->GetPerspectiveCamera()->SetPerspectiveCenter((float)(screenFrame.xmin+(screenFrame.Width() >> 1))/graphics2D->GetWidth(),
                                                               1-(float)(screenFrame.ymin+(screenFrame.Height() >> 1))/graphics2D->GetHeight());

        meshView->GetCamera()->GetTransform().SetOrigin(cameraPosition);
        meshView->GetCamera()->GetTransform().LookAt(lookingAt, csVector3(0, 1, 0));
        meshView->Draw();
    }

    PawsManager::GetSingleton().GetGraphics3D()->BeginDraw( CSDRAW_2DGRAPHICS );

    view->SetContext( og3d );
    pawsWidget::Draw();
}


void pawsObjectView::DrawRotate()
{
    if(spinMouse)
    {
        // NOTE: Y isn't used, but I will do it here if we need it later
        psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
        csVector2 blur;
        blur.Set(downPos);

        // Unite pos for all reses
        pos.x = (pos.x * 800) / graphics2D->GetWidth();
        pos.y = (pos.y * 600) / graphics2D->GetHeight();

        blur.x = (blur.x * 800) / graphics2D->GetWidth();
        blur.y = (blur.y * 600) / graphics2D->GetHeight();

        // Scale down, we want blurry positions
        pos.x = pos.x / 10;
        pos.y = pos.y / 10;
        blur.x = blur.x / 10;
        blur.y = blur.y / 10;

        float newRad;
        newRad = pos.x - blur.x;
        newRad /= 100;
        if(newRad != rotateRadians)
        {
            RotateTemp(10,newRad);
        }
    }

    if(rotateTime != 0)
    {
        static unsigned int ticks = csGetTicks();
        if(csGetTicks() > ticks + rotateTime)
        {
            ticks = csGetTicks();
            camRotate += rotateRadians;

            float currentAngle = (camRotate*180)/TWO_PI;
            if(currentAngle > 180.0f)
            {
                camRotate = camRotate - TWO_PI; // (180*TWO_PI)/180 = TWO_PI
            }
        }
    }

    if ( screenFrame.xmin > graphics2D->GetWidth() ||
         screenFrame.ymin > graphics2D->GetHeight() ||
         screenFrame.xmax < 0 ||
         screenFrame.ymax < 0 )
         return;

    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    if ( !PawsManager::GetSingleton().GetGraphics3D()->BeginDraw(CSDRAW_3DGRAPHICS) )
        return;

    if ( !view )
        return;

    iGraphics3D* og3d = view->GetContext();

    view->SetContext(PawsManager::GetSingleton().GetGraphics3D());

    view->SetRectangle( screenFrame.xmin,
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(),
                       screenFrame.Height() );

    view->GetPerspectiveCamera()->SetPerspectiveCenter((float)(screenFrame.xmin+(screenFrame.Width() >> 1))/graphics2D->GetWidth(),
                                                       1-(float)(screenFrame.ymin+(screenFrame.Height() >> 1))/graphics2D->GetHeight());

    csVector3 camera;
    camera.x = objectPos.x + sin((double)camRotate)*((-distance)-1);
    camera.y = 1;
    camera.z = objectPos.z + cos((double)camRotate)*((-distance)-1);

    view->GetCamera()->GetTransform().SetOrigin(camera);
    view->GetCamera()->GetTransform().LookAt(objectPos - camera + cameraMod, csVector3(0, 1, 0));

    view->Draw();

    if ( loadedMap )
    {
        og3d = meshView->GetContext();

        meshView->SetContext(PawsManager::GetSingleton().GetGraphics3D());

        meshView->SetRectangle(screenFrame.xmin,
                               PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                               screenFrame.Width(), screenFrame.Height());

        meshView->GetPerspectiveCamera()->SetPerspectiveCenter((float)(screenFrame.xmin+(screenFrame.Width() >> 1))/graphics2D->GetWidth(),
                                                               1-(float)(screenFrame.ymin+(screenFrame.Height() >> 1))/graphics2D->GetHeight());

        meshView->GetCamera()->GetTransform().SetOrigin(camera);
        meshView->GetCamera()->GetTransform().LookAt(objectPos - camera + cameraMod, csVector3(0, 1, 0));
        meshView->Draw();
    }

    PawsManager::GetSingleton().GetGraphics3D()->BeginDraw( CSDRAW_2DGRAPHICS );

    view->SetContext( og3d );
    pawsWidget::Draw();
}

bool pawsObjectView::OnMouseDown(int button,int mod, int x, int y)
{
    if(!mouseControlled)
        return false;
    if ( !doRotate && mouseDownUnlock )
        UnlockCamera();
        
    spinMouse = true;
    downPos.Set(x,y);
    downTime = csGetTicks();
    return true;
}

bool pawsObjectView::OnMouseUp(int button,int mod, int x, int y)
{
    if(!mouseControlled)
        return false;

    // 1 sec and about the same pos
    if(csGetTicks() - downTime < 1000 && int(downPos.x / 10) == int(x/10) && int(downPos.y / 10) == int(y/10) )
    {
        downTime = 0;
        // Click == stop or begin
        if(rotateTime != 0)
            RotateTemp(-1,0);
        else
            RotateDef();

        spinMouse = false;
    }
    else if(spinMouse)
    {
        spinMouse = false;
        RotateTemp(-1,0);
    }
    return true;
}

bool pawsObjectView::OnMouseExit()
{
    if(!mouseControlled)
        return false;

    if(spinMouse)
    {
        spinMouse = false;
        RotateTemp(-1,0);
    }
    return true;
}

void pawsObjectView::Clear()
{
    if(object)
    {
        object->GetMovable()->ClearSectors();
        engine->GetMeshes()->Remove(object);
    }
}
