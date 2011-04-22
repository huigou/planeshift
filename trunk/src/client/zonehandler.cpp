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
#include <csutil/scfstringarray.h>
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

#include "engine/psworld.h"

#include <ibgloader.h>

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

#define LOADING_SECTOR "SectorWhereWeKeepEntitiesResidingInUnloadedMaps"

ZoneHandler::ZoneHandler(MsgHandler* mh, psCelClient *cc) :
                        msghandler(mh),
                        celclient(cc),
                        loading(false),
                        forcedLoadingEndTime(0),
                        loadWindow(0),
                        loadProgressBar(0)
{
    msghandler->Subscribe(this,MSGTYPE_NEWSECTOR);

    valid = LoadZoneInfo();
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

bool ZoneHandler::FindLoadWindow(bool force, const char* widgetName)
{
    if(loadProgressBar != NULL && !force)
        return true;  // Already found

    loadWindow = static_cast <pawsLoadWindow*> (PawsManager::GetSingleton().FindWidget(widgetName));
    if(loadWindow == NULL)
        return false;

    loadProgressBar = static_cast <pawsProgressBar*> (loadWindow->FindWidget("Progress"));
    if(loadProgressBar == NULL)
        return false;

    return true;
}

bool ZoneHandler::ForceLoadWindowWidget(bool enable, csString loadWindowName)
{
    if(enable)
    {
        //check first if the window name is valid. shouldn't end up here
        if(loadWindowName.IsEmpty())
        {
            return false;
        }
        else //if it's valid we load it and set it as load window for now
        {
            PawsManager::GetSingleton().LoadWidget(loadWindowName.GetData());

            //request the load window to update the new window
            if(loadWindow)
                loadWindow->PublishMOTD();

            return FindLoadWindow(true, loadWindowName.GetData());
        }
    }
    else //restore the pointers to the normal window and removes the specified window from the memory
    {
        //restore the ties to the default load window
        bool result = FindLoadWindow(true);
        //as we have reloaded the load window we can remove the widget we don't need anymore
        if(loadWindowName.Length())
            PawsManager::GetSingleton().RemoveWidget(loadWindowName.GetData(), false);

        return result;
    }

    return true;
}

ZoneLoadInfo* ZoneHandler::FindZone(const char* sector) const
{
    ZoneLoadInfo* zone = zonelist.Get(sector, NULL);

    if(zone == NULL)
        Error2("Error: Could not find zone info for sector %s!\n",sector);

    return zone;
}

bool ZoneHandler::LoadZoneInfo()
{
    iVFS* vfs = psengine->GetVFS();
    if(!vfs)
        return false;
        
    iDocumentSystem* xml = psengine->GetXMLParser();
    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iFile> file = vfs->Open("/planeshift/data/zoneinfo.xml", VFS_FILE_READ);
    if(!file)
        return false;

    const char* error = doc->Parse(file);
    if(error)
    {
        Error2("Error Loading Zone Data: %s\n", error);
        return false;
    }

    csRef<iDocumentNodeIterator> zoneIter = doc->GetRoot()->GetNode("zonelist")->GetNodes("zone");

    while(zoneIter->HasNext())
    {
        csRef<iDocumentNode> zoneNode = zoneIter->Next();
        ZoneLoadInfo *zone = new ZoneLoadInfo(zoneNode);
        zonelist.Put(zone->inSector, zone);
    }

    return true;
}


void ZoneHandler::HandleMessage(MsgEntry* me)
{
    psNewSectorMessage msg(me);

    Notify3(LOG_LOAD, "Crossed from sector %s to sector %s.", msg.oldSector.GetData(), msg.newSector.GetData());
        
    LoadZone(msg.pos, msg.newSector);
}

void ZoneHandler::LoadZone(csVector3 pos, const char* sector, bool force)
{
    if((loading || !strcmp(sector, LOADING_SECTOR)) && !force)
        return;
    newPos = pos;
    csString sectorBackup = sectorToLoad; // cache old sector
    sectorToLoad = sector;
    bool connected = true;

    ZoneLoadInfo* zone = FindZone(sectorToLoad);
    if (zone == NULL)
    {
        Error1("Unable to find the sector you have entered in zoneinfo data.\nPlease check zoneinfo.xml");
        return;
    }

    // Move player to the loading sector.
    MovePlayerTo(csVector3(0.0f), LOADING_SECTOR);

    // load target location
    if(!psengine->BackgroundWorldLoading())
    {
        // Load the world.
        if(!psengine->GetLoader()->LoadZones(zone->regions))
        {
            Error2("Unable to load zone '%s'\n", zone->inSector.GetData());
            return;
        }
    }
    else
    {
        // perform extra checks whether blocked loading is necessary
        if(sectorToLoad != sectorBackup)
        {
            iSector * newsector = psengine->GetEngine()->FindSector(sectorToLoad.GetDataSafe());
            iSector * oldsector = psengine->GetEngine()->FindSector(sectorBackup.GetDataSafe());

            if (oldsector && newsector && celclient->GetWorld())
            {
                celclient->GetWorld()->BuildWarpCache(); // we need an up-to-date warp cache here
                connected = celclient->GetWorld()->Connected(oldsector, newsector);
            }
            else
            {
                connected = false;
            }
        }

        psengine->GetLoader()->UpdatePosition(pos, sectorToLoad, true);
    }

    // Set load screen if required.
    loadCount = psengine->GetLoader()->GetLoadingCount();
    if (FindLoadWindow() && (loadCount != 0 || !psengine->HasLoadedMap() || !connected || forcedLoadingEndTime != 0))
    {
        
        loading = true;

        if(psengine->HasLoadedMap())
            loadWindow->Clear();

        loadWindow->AddText(PawsManager::GetSingleton().Translate("Loading world"));
        loadWindow->SetAlwaysOnTop(true);
        loadWindow->Show();

        //If a background is defined in script
        if(!forcedBackgroundImg.IsEmpty())
        {
            Debug2(LOG_LOAD, 0, "Setting background from script %s", forcedBackgroundImg.GetData());
            loadWindow->SetBackground(forcedBackgroundImg);
        }
        else
        // If the area has its own loading image, use it
        if (zone->loadImage)
        {
            Debug2(LOG_LOAD, 0, "Setting background %s", zone->loadImage.GetData());
            loadWindow->SetBackground(zone->loadImage.GetData());
        }

        loadProgressBar->SetTotalValue(1.0f);
        loadProgressBar->SetCurrentValue(0.0f);

        psengine->ForceRefresh();
    }

    // do move the player in *any* case to make sure we won't end up looping to death
    // move player to new pos
    MovePlayerTo(newPos, sectorToLoad);
}

void ZoneHandler::MovePlayerTo(const csVector3 & newPos, const csString & newSector)
{
    if(!celclient->IsReady())
        return;

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
}

void ZoneHandler::OnDrawingFinished()
{
    if (loading)
    {
        if(psengine->GetLoader()->GetLoadingCount() == 0 && csGetTicks() >= forcedLoadingEndTime)
        {
            // move player to new pos
            MovePlayerTo(newPos, sectorToLoad);

            if(psengine->HasLoadedMap())
                loadWindow->Hide();

            loading = false;
            loadProgressBar->Completed();
            psengine->SetLoadedMap(true);

            // Move all entities which belong in these new sectors to them.
            psengine->GetCelClient()->OnMapsLoaded();

            // Reset camera clip distance.
            psCamera* cam = psengine->GetPSCamera();
            if(cam && !cam->GetDistanceCfg().adaptive)
                cam->UseFixedDistanceClipping(cam->GetFixedDistClip());
            psengine->GetPSCamera()->ResetCameraPositioning();

            // Update the lighting.
            psengine->GetModeHandler()->FinishLightFade();
            psengine->GetModeHandler()->DoneLoading(sectorToLoad);

            //Update delay data
            forcedBackgroundImg.Empty();
            forcedLoadingEndTime = 0;

            //restore the previous load window if any
            if(forcedWidgetName.Length())
                ForceLoadWindowWidget(false, forcedWidgetName);
        }
        else
        {
            float timeProgress = 1.0f;
            if(forcedLoadingEndTime)
            {
                timeProgress = (float)(csGetTicks() - forcedLoadingStartTime)/(forcedLoadingEndTime-forcedLoadingStartTime);
            }
            
            float loadProgress = 1.0f;
            if(loadCount)
            {
                loadProgress = (float)(loadCount-psengine->GetLoader()->GetLoadingCount())/loadCount;
                psengine->GetLoader()->ContinueLoading(false);
            }
            loadProgressBar->SetCurrentValue(csMin(loadProgress,timeProgress));
        }
    }
}

ZoneLoadInfo::ZoneLoadInfo(iDocumentNode *node)
{
    inSector = node->GetAttributeValue("sector");
    loadImage = node->GetAttributeValue("loadimage");
    regions.AttachNew(new scfStringArray());

    csRef<iDocumentNodeIterator> regionIter = node->GetNodes("region");
    while (regionIter->HasNext())
    {
        // Parse attributes from tag
        csRef<iDocumentNode> regionNode = regionIter->Next();
        regions->Push(regionNode->GetAttributeValue("map"));
    }    
}

void ZoneHandler::ForceLoadScreen(csString backgroundImage, uint32_t length, csString widgetName)
{
    forcedBackgroundImg = backgroundImage;
    forcedLoadingStartTime = csGetTicks();
    forcedLoadingEndTime = forcedLoadingStartTime + (length * 1000);
    forcedWidgetName = widgetName;
    if(forcedWidgetName.Length())
        ForceLoadWindowWidget(true, forcedWidgetName);
}

void ZoneHandler::HandleDelayAndAnim(int32_t loadDelay, csVector2 start, csVector2 dest, csString background, csString widgetName)
{
    if(loadDelay > 0)
    {
        ForceLoadScreen(background, loadDelay, widgetName);

        if(start != 0 || dest != 0)
            loadWindow->InitAnim(start, dest, loadDelay);
    }
}
