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
#include <iutil/object.h>
#include <iutil/objreg.h>
#include <imap/loader.h>
#include <iutil/vfs.h>
#include <iengine/texture.h>
#include <csutil/cscolor.h>
#include "pawsobjectview.h"
#include "pawsmanager.h"
#include "util/log.h"
#include "util/psconst.h"

int pawsObjectView::idName = 0;

pawsObjectView::pawsObjectView()
{
    engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    object = NULL;
    ID = 0;

    idName++;

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
}   

pawsObjectView::~pawsObjectView()
{
    idName--;
}

bool pawsObjectView::Setup(iDocumentNode* node )
{
    csRef<iDocumentNode> distanceNode = node->GetNode( "distance" );
    if ( distanceNode )
        distance = distanceNode->GetAttributeValueAsFloat("value");
    else
        distance = 4;

    float width = (float)defaultFrame.Width();
    float height = (float)defaultFrame.Height();

    distance *= distance / (width/267.5 + height/393);

    csRef<iDocumentNode> mapNode = node->GetNode( "map" );
    if ( mapNode )
    {
        csString mapFile = mapNode->GetAttributeValue("file");
        csString sector  = mapNode->GetAttributeValue("sector");

        return LoadMap( mapFile, sector );
    }
    else
    {
        return CreateMap();
    }
}

bool pawsObjectView::LoadMap( const char* map, const char* sector )
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iLoader> loader =  csQueryRegistry<iLoader > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iVFS> VFS =  csQueryRegistry<iVFS> ( PawsManager::GetSingleton().GetObjectRegistry());

    char newName[10];
    sprintf(newName, "NAME%d\n", idName );

    stage = engine->FindSector( sector );
    if ( !stage )
    {
        iRegion* cur_region = engine->CreateRegion (newName);

        // Clear it out if it already existed
        cur_region->DeleteAll ();

        // Now load the map into the selected region
        VFS->ChDir (map);
        engine->SetCacheManager(NULL);
        if ( !loader->LoadMapFile("world", CS_LOADER_KEEP_WORLD, cur_region, CS_LOADER_ACROSS_REGIONS) )
            return false;

        stage = engine->FindSector( sector );
        CS_ASSERT( stage );
        cur_region->Add( stage->QueryObject() );
        if ( !stage )
             return false;

        cur_region->Prepare();
    }

    meshSector = engine->CreateSector( newName );

    iLightList* lightList = meshSector->GetLights();
    csRef<iLight> light = engine->CreateLight(NULL, csVector3(-3,4,-3),10,
                                      csColor(0.86F,0.87F,0.6F), CS_LIGHT_STATIC);
    light->SetAttenuationMode( CS_ATTN_NONE );
    lightList->Add( light );

    meshSector->ShineLights();

    meshView = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));
    meshView->GetCamera()->SetSector(meshSector);
    meshView->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));

    meshView->SetRectangle(screenFrame.xmin, screenFrame.ymin,
                       screenFrame.Width(),screenFrame.Height());

    view = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));
    view->GetCamera()->SetSector(stage);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));


    view->SetRectangle(screenFrame.xmin, screenFrame.ymin,
                       screenFrame.Width(),screenFrame.Height());

    loadedMap = true;
    return true;
}

bool pawsObjectView::CreateMap()
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iLoader> loader =  csQueryRegistry<iLoader > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iVFS> VFS =  csQueryRegistry<iVFS> ( PawsManager::GetSingleton().GetObjectRegistry());

    char newName[10];
    sprintf(newName, "NAME%d\n", idName );
    iRegion* cur_region = engine->CreateRegion (newName);

    // Clear it out if it already existed
    cur_region->DeleteAll ();
       iTextureWrapper* txt = loader->LoadTexture("stone", "/lib/std/stone4.gif");

    if ( !txt )
    {
         Error1("Error loading the object view texture");
        return false;
    }
    cur_region->Add( txt->QueryObject () );

    iMaterialWrapper * matWrap;
    matWrap = engine->GetMaterialList()->FindByName( "stone" );

    cur_region->Add( matWrap->QueryObject() );

    stage = engine->CreateSector( "stage" );
    cur_region->Add( stage->QueryObject() );


    csRef<iMeshWrapper> walls = engine->CreateSectorWallsMesh( stage, "stagewalls" );
    cur_region->Add( walls->QueryObject() );

    csRef<iThingFactoryState> walls_state = scfQueryInterface<iThingFactoryState> (walls->GetMeshObject ()->GetFactory()); 
    walls_state->AddInsideBox(csVector3 (-5, 0, -5), csVector3 (5, 10, 5));
    walls_state->SetPolygonMaterial(CS_POLYRANGE_LAST, matWrap);
    walls_state->SetPolygonTextureMapping (CS_POLYRANGE_LAST, 3);

    // Create some lights. Later this can be added using the .def file or
    // a world file.
    iLightList* lightList = stage->GetLights();
    csRef<iLight> light = engine->CreateLight(NULL, csVector3(-3,4,-3),10,
                                      csColor(0.86F,0.87F,0.6F), CS_LIGHT_STATIC);
    light->SetAttenuationMode( CS_ATTN_NONE );
    lightList->Add( light );
    cur_region->Add( light->QueryObject() );

    light = engine->CreateLight(NULL, csVector3 (3, 4, 2), 10,
                                csColor (0, .125F, .85F), CS_LIGHT_STATIC);
    lightList->Add (light);
    cur_region->Add( light->QueryObject() );

    light = engine->CreateLight(NULL, csVector3(3,1,-3),10,
                                csColor(0.45F,0.45F,0.45F), CS_LIGHT_STATIC);
    lightList->Add( light );
    cur_region->Add( light->QueryObject() );

    // Cannot Prepare() entire engine more than once
    cur_region->Add( stage->QueryObject() );
    cur_region->Prepare();
    cur_region->ShineLights();

    view = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));
    view->GetCamera()->SetSector(stage);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));

    view->SetRectangle(screenFrame.xmin, screenFrame.ymin,
                       screenFrame.Width(),screenFrame.Height());

    view->GetCamera()->GetTransform().SetOrigin(csVector3(0,1,-distance));

    return true;
}

void pawsObjectView::View( const char* factName, const char* fileName )
{
    csRef<iMeshFactoryWrapper> meshfact = 0;
    meshfact = engine->GetMeshFactories()->FindByName (factName);

    if ( !meshfact )
    {
        csRef<iLoader> loader =  csQueryRegistry<iLoader> (PawsManager::GetSingleton().GetObjectRegistry());
        meshfact = loader->LoadMeshObjectFactory (fileName);
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
    if ( !wrapper )
        return;

    Clear();

    iSector* sector =  loadedMap ? meshSector : stage;
    object = engine->CreateMeshWrapper (wrapper, "PaperDoll", sector, csVector3(0,0,0) );
}

void pawsObjectView::View( iMeshWrapper* wrapper )
{
    if (wrapper) View(wrapper->GetFactory());
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
    if ( screenFrame.xmin > graphics2D->GetWidth() ||
         screenFrame.ymin > graphics2D->GetHeight() ||
         screenFrame.xmax < 0 ||
         screenFrame.ymax < 0 )
         return;

    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    if ( !PawsManager::GetSingleton().GetGraphics3D()->BeginDraw( engine->GetBeginDrawFlags() | CSDRAW_3DGRAPHICS) )
        return;

    if ( !view )
        return;

    iGraphics3D* og3d = view->GetContext();

    view->SetContext(PawsManager::GetSingleton().GetGraphics3D());

    view->SetRectangle( screenFrame.xmin,
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(),
                       screenFrame.Height() );

    view->GetCamera()->SetPerspectiveCenter(
                       screenFrame.xmin + (screenFrame.Width() >> 1),
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.Height() -
                       screenFrame.ymin + (screenFrame.Height() >> 1) );

    view->GetCamera()->SetFOV( view->GetCamera()->GetFOV(), screenFrame.Width() );

       
    view->GetCamera()->GetTransform().SetOrigin(cameraPosition);
    view->GetCamera()->GetTransform().LookAt(
        lookingAt,
        csVector3(0,1,0)
        );

    view->Draw();

    if ( loadedMap )
    {
        og3d = meshView->GetContext();

        meshView->SetContext(PawsManager::GetSingleton().GetGraphics3D());

        meshView->SetRectangle( screenFrame.xmin,
                                PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                                screenFrame.Width(),
                                screenFrame.Height() );

        meshView->GetCamera()->SetPerspectiveCenter(
                                screenFrame.xmin + (screenFrame.Width() >> 1),
                                PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.Height() -
                                screenFrame.ymin + (screenFrame.Height() >> 1) );


        meshView->GetCamera()->SetFOV( view->GetCamera()->GetFOV(), screenFrame.Width() );

        meshView->GetCamera()->GetTransform().SetOrigin(cameraPosition);
        meshView->GetCamera()->GetTransform().LookAt(
            lookingAt,
            csVector3(0,1,0)
            );
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
    if ( !PawsManager::GetSingleton().GetGraphics3D()->BeginDraw( engine->GetBeginDrawFlags() | CSDRAW_3DGRAPHICS) )
        return;

    if ( !view )
        return;

    iGraphics3D* og3d = view->GetContext();

    view->SetContext(PawsManager::GetSingleton().GetGraphics3D());

    view->SetRectangle( screenFrame.xmin,
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(),
                       screenFrame.Height() );

    view->GetCamera()->SetPerspectiveCenter(
                       screenFrame.xmin + (screenFrame.Width() >> 1),
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.Height() -
                       screenFrame.ymin + (screenFrame.Height() >> 1) );

    view->GetCamera()->SetFOV( view->GetCamera()->GetFOV(), screenFrame.Width() );

    csBox3 bbox;
    if(object)
        bbox = object->GetWorldBoundingBox();

    csVector3 camera;
    camera.x = objectPos.x + sin((double)camRotate)*((-distance)-1);
    camera.y = 1;
    camera.z = objectPos.z + cos((double)camRotate)*((-distance)-1);

    view->GetCamera()->GetTransform().SetOrigin(camera);
    view->GetCamera()->GetTransform().LookAt(
        objectPos + csVector3(0,bbox.GetCenter().y,0) - camera + cameraMod,
        csVector3(0,1,0)
        );

    view->Draw();

    if ( loadedMap )
    {
        og3d = meshView->GetContext();

        meshView->SetContext(PawsManager::GetSingleton().GetGraphics3D());

        meshView->SetRectangle( screenFrame.xmin,
                                PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                                screenFrame.Width(),
                                screenFrame.Height() );

        meshView->GetCamera()->SetPerspectiveCenter(
                                screenFrame.xmin + (screenFrame.Width() >> 1),
                                PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.Height() -
                                screenFrame.ymin + (screenFrame.Height() >> 1) );


        meshView->GetCamera()->SetFOV( view->GetCamera()->GetFOV(), screenFrame.Width() );

        meshView->GetCamera()->GetTransform().SetOrigin(camera);
        meshView->GetCamera()->GetTransform().LookAt(
            objectPos + csVector3(0,bbox.GetCenter().y,0) - camera,
            csVector3(0,1,0)
            );
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
