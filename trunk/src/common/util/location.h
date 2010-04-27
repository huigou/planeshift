/*
 * location.h
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
#ifndef __LOCATION_H__
#define __LOCATION_H__

#include <csutil/csstring.h>
#include <csgeom/vector3.h>
#include <csgeom/box.h>
#include <csutil/parray.h>
#include <iengine/sector.h>
#include <csutil/weakref.h>

#include "util/psdatabase.h"

struct iEngine;
class LocationType;

/**
 * A Location is a named place on the map, located
 * dynamically by NPCs as scripted.
 *
 * @note Location dosn't support multi sector locations yes.
 */
class Location
{
public:
    int                 id;
    csString            name;
    csVector3           pos;
    float               rot_angle;
    float               radius;
    csArray<Location*>  locs; // A number of points for regions.
    int                 id_prev_loc_in_region;
    csString            sectorName;
    csBox2              boundingBox;

    csWeakRef<iSector>  sector; ///< Cached sector
    LocationType*       type;   ///< Points back to location type

    ~Location();
    
    bool Load(iResultRow& row, iEngine *engine, iDataConnection *db);
    bool Import(iDocumentNode *node, iDataConnection *db, int typeID);
    
    bool IsRegion() { return locs.GetSize() != 0; }
    bool IsCircle() { return locs.GetSize() == 0; }
    
    /// Return cached sector or find the sector and cache it from engine.
    iSector*            GetSector(iEngine * engine);

    /** Return the bounding box for this location
     *
     * @return Bounding box of the location
     */
    const csBox2& GetBoundingBox() const;

    /** Function to calculate the bounding box for a location.
     *
     * This function should be called after the location has been
     * loaded or modified.
     */
    void CalculateBoundingBox();
    

    bool CheckWithinBounds(iEngine * engine,const csVector3& pos,const iSector* sector);

    /** Get a random position in the location
     *
     * Will return the position found. Do not relay on the
     * state of the parameters if operation failes.
     *
     * @param engine Used to find the sector
     * @param pos    The found position is returned here.
     * @param sector The found sector is returned here.
     *
     * @return True if position is found.
     */
    bool GetRandomPosition(iEngine * engine,csVector3& pos,iSector* &sector);

    static int GetSectorID(iDataConnection *db,const char* name);
    const char* GetName() { return name.GetDataSafe(); }
};

/**
 * This stores a vector of positions listing a set of 
 * points defining a common type of location, such as
 * a list of burning fires or guard stations--whatever
 * the NPCs need.
 */
class LocationType
{
public:
    int                   id;
    csString              name;
    csArray<Location*>    locs;

    ~LocationType();

    bool Load(iDocumentNode *node);
    bool Import(iDocumentNode *node, iDataConnection *db);
    bool ImportLocations(iDocumentNode *node, iDataConnection *db);
    bool Load(iResultRow& row, iEngine *engine, iDataConnection *db); 

    bool CheckWithinBounds(iEngine * engine,const csVector3& pos,const iSector* sector);
    
    /** Get a random position in the location
     *
     * Will return the position found. Do not relay on the
     * state of the parameters if operation failes.
     *
     * @param engine Used to find the sector
     * @param pos    The found position is returned here.
     * @param sector The found sector is returned here.
     *
     * @return True if position is found.
     */
    bool GetRandomPosition(iEngine * engine,csVector3& pos,iSector* &sector);

    const char* GetName() { return name.GetDataSafe(); }
};

#endif

