/*
 * zonehandler.cpp    Keith Fulton <keith@paqrat.com>
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/object.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <imesh/partsys.h>
#include <csutil/cscolor.h>
#include <iutil/vfs.h>
#include <csutil/sysfunc.h>
#include <ivaria/engseq.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/psscf.h"
#include "util/log.h"

#include "paws/pawsmanager.h"
#include "paws/pawsprogressbar.h"

#include "gui/pawsloading.h"

#include "engine/loader.h"
#include "engine/psworld.h"

#include "iclient/isoundmngr.h"

#include "psclientdr.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "zonehandler.h"
#include "pscamera.h"
#include "pscelclient.h"
#include "modehandler.h"
#include "pscharcontrol.h"
#include "globals.h"

ZoneHandler::ZoneHandler(MsgHandler* mh,iObjectRegistry* obj_reg, psCelClient *cc)
{
    msghandler     = mh;
    object_reg     = obj_reg;
    world          = NULL;
    celclient      = cc;
    haveNewPos     = false;

    msghandler->Subscribe(this,MSGTYPE_NEWSECTOR);

    valid = LoadZoneInfo();
    needsToLoadMaps = false;
    initialRefreshScreen = true;

    loadWindow = NULL;
    loadProgressBar = NULL;
}

ZoneHandler::~ZoneHandler()
{
    if (msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_NEWSECTOR);
    }
    
    csHash<ZoneLoadInfo *, const char*>::GlobalIterator it(zonelist.GetIterator());
    
    while(it.HasNext())
        delete it.Next();
        
}

bool ZoneHandler::FindLoadWindow()
{
    if (loadProgressBar != NULL)
        return true;  // Already found

    loadWindow = static_cast <pawsLoadWindow*> (PawsManager::GetSingleton().FindWidget("LoadWindow"));
    if (loadWindow == NULL)
        return false;

    loadProgressBar = static_cast <pawsProgressBar*> (loadWindow->FindWidget("Progress"));
    if (loadProgressBar == NULL)
        return false;

    return true;
}

ZoneLoadInfo * ZoneHandler::FindZone(const char* sector)
{
    ZoneLoadInfo* zone = zonelist.Get(sector, NULL);

    if (zone == NULL)
        Error2("Error: Could not find zone info for sector %s!\n",sector);

    return zone;
}

bool ZoneHandler::LoadZoneInfo()
{
    iVFS* vfs = psengine->GetVFS ();
    if (!vfs)
        return false;
        
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iFile> file = vfs->Open("/planeshift/data/zoneinfo.xml", VFS_FILE_READ);
    if (!file)
        return false;

    const char* error = doc->Parse(file);
    if (error)
    {
        Error2("Error Loading Zone Data: %s\n", error);
        return false;
    }

    csRef<iDocumentNodeIterator> zoneIter = doc->GetRoot()->GetNode("zonelist")->GetNodes("zone");

    while (zoneIter->HasNext())
    {
        csRef<iDocumentNode> zoneNode = zoneIter->Next();

        csRef<iDocumentNodeIterator> regionIter = zoneNode->GetNodes("region");
        while (regionIter->HasNext())
        {
            // Parse attributes from tag
            csRef<iDocumentNode> regionNode = regionIter->Next();
            csString mapname = regionNode->GetAttributeValue("map");

            bool found = false;
            for(size_t i =0;i<alllist.GetSize();i++)
            {
                if(mapname == alllist[i])
                {
                    found = true;
                    break;
                }
            }

            if(!found)
                alllist.Push(mapname);
        }

        ZoneLoadInfo *zone = new ZoneLoadInfo(zoneNode);
        zonelist.Put(zone->inSector, zone);
    }

    return true;
}


void ZoneHandler::HandleMessage(MsgEntry* me)
{
    psNewSectorMessage msg(me);

    Notify3(LOG_LOAD, "Crossed from sector %s to sector %s.", msg.oldSector.GetData(), msg.newSector.GetData() );

    if (IsMapLoadNeeded()) 
    {
        Warning2(LOG_LOAD, "Still loading maps, ignoring crossing to sector %s.", msg.newSector.GetData());
        return;
    }

    // We don't load the maps here: we just remember that we need to do it and we show LoadingWindow
    // If we began to load the maps immediately at this place, the LoadingWindow would not have a chance to be drawn

    ZoneLoadInfo* zone = FindZone(msg.newSector);
    if (zone == NULL)
    {
        Error1("The sector you have entered couldn't be loaded.\nPlease check your logs for details.");
        return;
    }

    if(!psengine->ThreadedWorldLoading())
    {
        FlagRegions(zone);
    }

    bool catchUp = psengine->ThreadedWorldLoading();
    if(catchUp)
    {
      csVector3 pos;
      float yrot;
      iSector* sector;

      celclient->GetMainPlayer()->GetLastPosition (pos, yrot, sector);

      pos -= msg.pos;
      catchUp = msg.oldSector.IsEmpty() || msg.oldSector.Compare("SectorWhereWeKeepEntitiesResidingInUnloadedMaps") ||
        (abs(pos.x) > 1.0f || abs(pos.x) > 1.0f || abs(pos.x) > 1.0f);
    }

    if (catchUp || (world && world->NeedsLoading(zone->transitional)))
    {
        SetMapLoadNeeded(true);
        sectorToLoad = msg.newSector;
        haveNewPos = true;
        newPos = msg.pos;

        if(catchUp)
        {
          Loader::GetSingleton().UpdatePosition(newPos, sectorToLoad, true);
        }

        if(catchUp && Loader::GetSingleton().GetLoadingCount() == 0)
        {
            haveNewPos = false;
        }
        else if(psengine->IsGameLoaded() && FindLoadWindow())
        {
            loadWindow->SetAlwaysOnTop(true);
            loadWindow->Clear();
            // If the area has its own loading image, use it
            if (zone->loadImage) {
                Debug2(LOG_LOAD, 0, "Setting background %s", zone->loadImage.GetData());
                loadWindow->SetBackground(zone->loadImage.GetData());
            }
            loadWindow->AddText("Loading region");
            loadWindow->Show();
            psengine->ForceRefresh();
        }
    }
    else // If we don't need to load map, set player's position immediately
        MovePlayerTo(msg.pos, msg.newSector);

    // Reset camera clip distance.
    psCamera* cam = psengine->GetPSCamera();
    if(cam && !cam->GetDistanceCfg().adaptive)
        cam->UseFixedDistanceClipping(cam->GetFixedDistClip());

    if(!IsMapLoadNeeded())
        psengine->GetModeHandler()->DoneLoading(msg.newSector);
}

void ZoneHandler::MovePlayerTo(const csVector3 & newPos, const csString & newSector)
{
    csVector3 pos;
    float yrot;
    iSector* sector;
    
    celclient->GetMainPlayer()->GetLastPosition (pos, yrot, sector);            // retrieve last yrot
    
    sector = psengine->GetEngine()->FindSector(newSector);
    if (sector != NULL)
    {
        Notify5(LOG_LOAD, "Setting position of player %f %f %f in sector '%s'", newPos.x, newPos.y, newPos.z, newSector.GetData());
        celclient->GetMainPlayer()->SetPosition(newPos, yrot, sector);          // set new position
    }
    else
    {
        Error2("Couldn't find sector '%s'", newSector.GetData());
    }

    if (FindLoadWindow())
        loadProgressBar->Completed();
}

void ZoneHandler::SetMapLoadNeeded(bool needed)
{
    needsToLoadMaps = needed;
    //inform server about status change
    psClientDR* clientDr = celclient->GetClientDR(); 
    if (clientDr)
        clientDr->CheckDeadReckoningUpdate();
}

void ZoneHandler::OnDrawingFinished()
{
    if (IsMapLoadNeeded())
    {
        if(ExecuteFlaggedRegions(sectorToLoad))
        {
            SetMapLoadNeeded(false);

            psengine->SetLoadedMap(true);
            psengine->GetModeHandler()->FinishLightFade();  // make sure new map gets relit for time of day

            if (haveNewPos) 
            {
                MovePlayerTo(newPos, sectorToLoad);
                psengine->GetPSCamera()->ResetCameraPositioning();
            }
        }

        if (FindLoadWindow())
        {
            loadProgressBar->SetCurrentValue(loadProgressBar->GetCurrentValue() + 1);
            psengine->ForceRefresh();
        }
    }
}

void ZoneHandler::FlagRegions(ZoneLoadInfo* zone)
{
    if (!zone)
        return;

    if(!keepMapsLoaded)
        world->FlagAllRegionsAsNotNeeded();

    csArray<csString> loading;
    for (size_t i=0; i < zone->regions.GetSize(); i++)
    {
        csString *map = zone->regions[i];
        world->FlagRegionAsNeeded(*map);
        loading.Push(*map);
    }

    if(loadAllMaps)
    {
        Debug1(LOG_LOAD,0, "Flagging all maps to be loaded");
        for (size_t i=0; i<alllist.GetSize(); i++)
        {
            bool found = false;
            for (size_t j=0; j<loading.GetSize(); j++)
            {
                if(loading[j] == alllist[i])
                {
                    found = true;
                    break;
                }
            }
            if(found)
                continue;

            // Flag
            world->FlagRegionAsNeeded(alllist[i]);
        }
    }

    if (FindLoadWindow())
    {
        if (loadAllMaps)
        {
            loadProgressBar->SetTotalValue(alllist.GetSize());
        }
        else
        {
            csArray<iCollection*> deletedRegions;
            world->GetNotNeededRegions(deletedRegions);
            loadProgressBar->SetTotalValue(zone->regions.GetSize() + deletedRegions.GetSize());
        }

        loadProgressBar->SetCurrentValue(0);
        psengine->ForceRefresh();
    }
}

void ZoneHandler::LoadZone(const char* sector)
{
    sectorToLoad = sector;
    SetMapLoadNeeded(true);
    FlagRegions( FindZone(sectorToLoad) );
}

bool ZoneHandler::ExecuteFlaggedRegions(const csString & sector)
{
    ZoneLoadInfo* found = FindZone(sector);
    bool background = true;
    if(!psengine->ThreadedWorldLoading())
    {
        background = false;
    }

    if (found)
    {
        if(!background)
        {
            // If the sector has a loading screen, display it
            if (found->loadImage && FindLoadWindow())
            {
                loadWindow->Clear();
                Debug2(LOG_LOAD, 0, "Setting background %s", found->loadImage.GetData());
                loadWindow->SetBackground(found->loadImage.GetData());
                loadWindow->AddText("Loading region");
                loadWindow->Show();
                psengine->ForceRefresh();
            }

            if(!found->transitional)
            {
                csArray<iCollection*> deletedRegions;
                world->GetNotNeededRegions(deletedRegions);
                celclient->OnRegionsDeleted(deletedRegions);
            }
        }

        // Before the first map is loaded, we want to refresh the screen to get the loading background.
        if(initialRefreshScreen)
        {
            initialRefreshScreen = false;
            return false;
        }

        // Load a map.
        int executed = 2;
        if(background)
        {
            if(Loader::GetSingleton().HasValidPosition() && Loader::GetSingleton().GetLoadingCount() == 0)
            {
                executed = 0;
                psengine->GetEngine()->PrecacheDraw();
            }
        }
        else
        {
            executed = world->ExecuteFlaggedRegions(found->transitional);
        }

        switch(executed)
        {
            case 1:
                psengine->FatalError("Loading region failed!");
            case 2:
                return false;
            default:
                initialRefreshScreen = true;
        }

        celclient->OnMapsLoaded();

        // If this is the first time, don't hide the loading (we still got NPCs to load)
        if (psengine->IsGameLoaded())
        {
            if (FindLoadWindow())
                loadWindow->Hide();
        }
        else
        {
            // Else we need to call this since it's not a "new" sector
            psengine->GetModeHandler()->DoneLoading(sector);
        }
    }
    return true;
}

ZoneLoadInfo::ZoneLoadInfo(const char *sector, const char *image)
{
    inSector = sector;
    loadImage = image;
}

ZoneLoadInfo::ZoneLoadInfo(iDocumentNode *node)
{
    csString trans;

    inSector = node->GetAttributeValue("sector");
    loadImage = node->GetAttributeValue("loadimage");
    trans = node->GetAttributeValue("transitional");

    transitional = (trans=="yes");

    csRef<iDocumentNodeIterator> regionIter = node->GetNodes("region");

    while (regionIter->HasNext())
    {
        // Parse attributes from tag
        csRef<iDocumentNode> regionNode = regionIter->Next();
        csString *mapname = new csString(regionNode->GetAttributeValue("map"));
        regions.Push(mapname);
    }    
}

