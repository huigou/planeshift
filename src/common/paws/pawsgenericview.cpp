/*
 * Author: Andrew Craig
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

#include <iengine/campos.h>
#include <iengine/collection.h>
#include <imap/loader.h>
#include <iutil/object.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include "pawsgenericview.h"
#include "pawsmanager.h"
#include "util/log.h"
#include "util/psconst.h"

int pawsGenericView::idName = 0;

pawsGenericView::pawsGenericView()
{
    engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    idName++;  

    char newName[10];
    sprintf(newName, "NAME%d\n", idName );
    collection = engine->CreateCollection(newName);

    loadedMap = false;
}   

pawsGenericView::~pawsGenericView()
{
    idName--;
}

bool pawsGenericView::Setup(iDocumentNode* node )
{
    csRef<iDocumentNode> mapNode = node->GetNode( "map" ); 
    if ( mapNode )
    {
        csString mapFile = mapNode->GetAttributeValue("file");
        csString sector  = mapNode->GetAttributeValue("sector");
        
        LoadMap(mapFile, sector);
        return true;
        //return LoadMap( mapFile, sector );
    }
    return false;
}

bool pawsGenericView::LoadMap( const char* map, const char* sector )
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iThreadedLoader> loader =  csQueryRegistry<iThreadedLoader> ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iVFS> VFS =  csQueryRegistry<iVFS> ( PawsManager::GetSingleton().GetObjectRegistry());
             
    mapName = map;

    csString sectorName;

    // Clear out the collection.
    collection->ReleaseAllObjects();

    // Now load the map into the selected region
    VFS->ChDir (map);
    VFS->SetSyncDir(VFS->GetCwd());
    engine->SetCacheManager(NULL);
    csRef<iThreadReturn> itr = loader->LoadMapFile(map, "world", CS_LOADER_KEEP_WORLD, collection);
    itr->Wait();
    if(!itr->WasSuccessful())
    {
        return false;
    }
    engine->SyncEngineListsNow(loader);
    VFS->ChDir (map);

    if (sector)
        sectorName = sector;
    else if (engine->GetCameraPositions()->GetCount() > 0)
        sectorName = engine->GetCameraPositions()->Get(0)->GetSector();
    else
        return false;

    stage = engine->FindSector(sectorName);
    CS_ASSERT( stage );
    collection->Add( stage->QueryObject() );
    if ( !stage )
            return false;

    view = csPtr<iView> (new csView( engine, PawsManager::GetSingleton().GetGraphics3D() ));

    if (engine->GetCameraPositions()->GetCount() > 0)
    {
        iCameraPosition * cp = engine->GetCameraPositions()->Get(0);
        view->GetCamera()->SetSector(engine->FindSector(cp->GetSector()));
        view->GetCamera()->GetTransform().SetOrigin(cp->GetPosition());
        view->GetCamera()->GetTransform().LookAt(cp->GetForwardVector(), cp->GetUpwardVector());
    }
    else
    {
        view->GetCamera()->SetSector(stage);
        view->GetCamera()->GetTransform().SetOrigin(csVector3(-33,1,-198));
        view->GetCamera()->GetTransform().LookAt(csVector3(0,0,4), csVector3(0,1,0));
    }
        
    view->SetRectangle(screenFrame.xmin, screenFrame.ymin,
                       screenFrame.Width(),screenFrame.Height());
   
    loadedMap = true;        
    return true;        
}

void pawsGenericView::Draw()
{
    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    // tell CS to render the scene
    if (!PawsManager::GetSingleton().GetGraphics3D()->BeginDraw(CSDRAW_3DGRAPHICS))
        return;

    view->SetRectangle( screenFrame.xmin, 
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.ymax ,
                       screenFrame.Width(),
                       screenFrame.Height() );

    view->GetPerspectiveCamera()->SetPerspectiveCenter(
                       screenFrame.xmin + (screenFrame.Width() >> 1),
                       PawsManager::GetSingleton().GetGraphics3D()->GetHeight() - screenFrame.Height() - 
                       screenFrame.ymin + (screenFrame.Height() >> 1) );

    view->GetPerspectiveCamera()->SetFOV( view->GetPerspectiveCamera()->GetFOV(), screenFrame.Width() );
    
    view->Draw();

    PawsManager::GetSingleton().GetGraphics3D()->BeginDraw( CSDRAW_2DGRAPHICS );
    pawsWidget::Draw();
}
