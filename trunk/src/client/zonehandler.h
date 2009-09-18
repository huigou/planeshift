/*
 * zonehandler.h    Keith Fulton <keith@paqrat.com>
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
#ifndef ZONEHANDLER_H
#define ZONEHANDLER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
class MsgHandler;
class psWorld;
class PawsManager;
class psCelClient;
class pawsLoadWindow;
class pawsProgressBar;

/**
 * The ZoneHandler keeps a tree of these structures, loaded
 * from an XML file, which specifies which regions are to
 * be loaded when in that particular sector.  Every sector
 * in the world will need an entry in that xml file, because
 * the client will use this list also when the game loads
 * to make sure all relevant pieces are loaded no matter where
 * the player's starting point is.
 */
class ZoneLoadInfo
{
public:
    csString inSector;
    csString    loadImage;
    bool        transitional;
    
    csRef<iStringArray> regions;

    ZoneLoadInfo(iDocumentNode *node);

    bool operator==(ZoneLoadInfo& other) const
    {
        return inSector == other.inSector;
    }
    bool operator<(ZoneLoadInfo& other) const
    {
        return (strcmp(inSector,other.inSector)<0);
    }
};


/**
 *  This class listens for crossing sector boundaries
 *  and makes sure that we have all the right stuff
 *  loaded in each zone.
 */
class ZoneHandler : public psClientNetSubscriber
{
public:
    ZoneHandler(MsgHandler* mh,iObjectRegistry* object_reg, psCelClient *cc);
    virtual ~ZoneHandler();

    void HandleMessage(MsgEntry* me);

    void LoadZone(csVector3 pos, const char* sector);

    /** Call this after drawing on screen finished.
        It checks if player just crossed boundary between sectors and loads/unloads needed maps */
    void OnDrawingFinished();

    /** Moves player to given location */
    void MovePlayerTo(const csVector3 & newPos, const csString & newSector);

    bool IsLoading() const { return loading; }
   
protected:
    csHash<ZoneLoadInfo *, const char*> zonelist;
    csArray<csString>       alllist;
    iObjectRegistry*        object_reg;
    csRef<MsgHandler>        msghandler;
    psCelClient             *celclient;

    bool valid;
    bool needsToLoadMaps;
    csString sectorToLoad;
    csVector3 newPos;
    bool haveNewPos;
    int rgnsLoaded;
    bool initialRefreshScreen;
    
    pawsLoadWindow* loadWindow;
    pawsProgressBar* loadProgressBar;
    bool FindLoadWindow();

    bool LoadZoneInfo();
    ZoneLoadInfo * FindZone(const char* sector);
    bool loading;

    /** Tells "world" to (un)load flagged maps, then hides LoadingWindow */
    bool ExecuteFlaggedRegions(const csString & sector);
};


#endif
