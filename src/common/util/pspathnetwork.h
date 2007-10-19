/*
 * pspathnetwork.h
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
#ifndef __PSPATHNETWORK_H__
#define __PSPATHNETWORK_H__

#include <csutil/array.h>
#include <csutil/list.h>

#include "iserver/idal.h"

#include "util/pspath.h"

class Waypoint;
class psWorld;
class psPath;

class psPathNetwork
{
public:
    csArray<Waypoint*> waypoints;
    csArray<psPath*> paths;

    /**
     * Load all waypoins and paths from db
     */
    bool Load(iEngine *engine, iDataConnection *db,psWorld * world);

    /**
     * Find waypoint by id
     */
    Waypoint *FindWaypoint(int id);

    /**
     * Find waypoint by name
     */
    Waypoint *FindWaypoint(const char * name);
    
    /**
     * Find waypoint nearest to a point in the world
     */
    Waypoint *FindNearestWaypoint(psWorld *world,iEngine *engine,csVector3& v,iSector *sector, float range, float * found_range);

    /**
     * Find random waypoint within a given range to a point in the world
     */
    Waypoint *FindRandomWaypoint(psWorld *world,iEngine *engine,csVector3& v,iSector *sector, float range, float * found_range);

    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Waypoint*> FindWaypointRoute(Waypoint * start, Waypoint * end);
    
    /**
     * List all waypoints matching pattern to console.
     */
    void ListWaypoints(const char *pattern);
    
    /**
     * List all paths matching pattern to console.
     */
    void ListPaths(const char *pattern);

    /**
     * Find the named path.
     */
    psPath   *FindPath(const char *name);
    
    /**
     * Find a given path.
     */
    psPath *FindPath(const Waypoint * wp1, const Waypoint * wp2, psPath::Direction & direction);

};

#endif
