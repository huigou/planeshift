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
    csPDelArray<Waypoint> waypoints;
    csPDelArray<psPath> paths;
    
    csArray<csString> waypointGroupNames;
    csArray< csList<Waypoint*> > waypointGroups;

    csWeakRef<iEngine> engine;
    csWeakRef<iDataConnection> db;
    psWorld * world;
    
    
    /**
     * Load all waypoins and paths from db
     */
    bool Load(iEngine *engine, iDataConnection *db, psWorld * world);

    /**
     *
     */
    int AddWaypointToGroup(csString group, Waypoint * wp);

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
     * @param range Find only waypoints within range from waypoint, -1 if range dosn't matter
     */
    Waypoint *FindNearestWaypoint(csVector3& v,iSector *sector, float range, float * found_range = NULL);

    /**
     * Find random waypoint within a given range to a point in the world
     * @param range Find only waypoints within range from waypoint, -1 if range dosn't matter
     */
    Waypoint *FindRandomWaypoint(csVector3& v, iSector *sector, float range, float * found_range = NULL);

    /**
     * Find waypoint nearest to a point in the world in the given group.
     * @param range Find only waypoints within range from waypoint, -1 if range dosn't matter
     */
    Waypoint *FindNearestWaypoint(int group, csVector3& v,iSector *sector, float range, float * found_range = NULL);

    /**
     * Find random waypoint within a given range to a point in the world
     * @param range Find only waypoints within range from waypoint, -1 if range dosn't matter
     */
    Waypoint *FindRandomWaypoint(int group, csVector3& v, iSector *sector, float range, float * found_range = NULL);

    /**
     * Find the index for the given group name, return -1 if no group is found.
     */
    int FindWaypointGroup(const char * groupName);
    
    /**
     * Find the path nearest to a point in the world in the given group.
     * @ param Set an maximum range for points to considere.
     */
    psPath *FindNearestPath(csVector3& v, iSector *sector, float range, float * found_range = NULL, int * index = NULL);
    
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


    /**
     * Create a new waypoint
     */
    Waypoint* CreateWaypoint(csString& name, csVector3& pos, csString& sectorName, float radius, csString& flags);

    /**
     * Create a new path/connection/link between two waypoints
     */
    psPath* CreatePath(const csString& name, Waypoint* wp1, Waypoint* wp2, const csString& flags);

    /**
     * Create a new path/connection/link between two waypoints from an external created path object
     */
    psPath* CreatePath(psPath * path);

    /**
     * Get next unique number for waypoint checking.
     */
    int GetNextWaypointCheck();

    /**
     * Delete the given path from the db.
     */
    bool Delete(psPath * path);
};

#endif
