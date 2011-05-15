/*
 * waypoint.h
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
#ifndef __WAYPOINT_H__
#define __WAYPOINT_H__

#include <csutil/csstring.h>
#include <csgeom/vector3.h>
#include <iengine/sector.h>

#include "util/psdatabase.h"
#include "util/location.h"
#include "util/pspath.h"
#include "util/edge.h"

/**
 * A waypoint is a specified circle on the map with a name,
 * location, and a list of waypoints it is connected to.  With
 * this class, a network of nodes can be created which will
 * allow for pathfinding and path wandering by NPCs.
 */
class Waypoint
{
public:
    Location                   loc;            ///< Id and position
    csString                   group;          ///< Hold group name for this waypoint if any.
    csArray<csString>          aliases;        ///< Hold aliases for this waypoint

    csArray<Waypoint*>         links;          ///< Links to other waypoinst connected with paths from this node.
    csArray<float>             dists;          ///< Distances of each link.
    csPDelArray<Edge>          edges;          ///< Edges for each link.
    csArray<psPath*>           paths;          ///< Path object for each of the links
    
    bool                       allowReturn;    ///< This prevents the link back to the prior waypoint
                                               ///< from being chosen, if true.
    bool                       underground;    ///< True if this waypoint is underground
    bool                       underwater;     ///< True if this waypoint is underwater
    bool                       priv;           ///< True if this waypoint is private
    bool                       pub;            ///< True if this waypoint is public
    bool                       city;           ///< True if this waypoint is in a city
    bool                       indoor;         ///< True if this waypoint is indoor
    bool                       path;           ///< True if this waypoint is a path
    bool                       road;           ///< True if this waypoint is a road/street
    bool                       ground;         ///< True if this waypoint is ground

    Waypoint();
    Waypoint(const char *name);
    Waypoint(csString& name, csVector3& pos, csString& sectorName, float radius, csString& flags);
    
    
    bool operator==(Waypoint& other) const
    { return loc.name==other.loc.name; }
    bool operator<(Waypoint& other) const
    { return (strcmp(loc.name,other.loc.name)<0); }

    bool Load(iDocumentNode *node, iEngine *engine);
    bool Import(iDocumentNode *node, iEngine *engine, iDataConnection *db);
    bool Load(iResultRow& row, iEngine *engine); 

    void AddLink(psPath * path, Waypoint * wp, psPath::Direction direction, float distance);
    void RemoveLink(psPath * path);

    Edge* GetRandomEdge(const psPathNetwork::RouteFilter* routeFilter);


    /// Add a new alias to this waypoint
    void AddAlias(csString alias);
    /// Remove a alias from this waypoint
    void RemoveAlias(csString alias);

    /// Get the id of this waypoint
    int          GetID(){ return loc.id; }
    
    /// Get the name of this waypoint
    const char * GetName(){ return loc.name.GetDataSafe(); }

    /// Rename the waypoint and update the db
    bool Rename(iDataConnection * db,const char* name);
    /// Rename the waypoint
    void Rename(const char* name);

    /// Get the group name of this waypoint
    const char* GetGroup(){ return group.GetDataSafe(); }
    
    /// Get a string with aliases for this waypoint
    csString GetAliases();
    
    /// Get the sector from the location.
    iSector*     GetSector(iEngine * engine) { return loc.GetSector(engine); }
    
    csString     GetFlags();

    float GetRadius() { return loc.radius; }
    bool SetRadius(float radius);
    bool SetRadius(iDataConnection * db, float radius);
    void RecalculateEdges(psWorld * world, iEngine *engine);

    const csVector3& GetPosition() { return loc.pos; }

    bool CheckWithin(iEngine * engine, const csVector3& pos, const iSector* sector);

    int Create(iDataConnection * db);
    bool CreateAlias(iDataConnection * db, csString alias);
    bool RemoveAlias(iDataConnection * db, csString alias);
    bool Adjust(iDataConnection * db, csVector3 & pos, csString sector);
    
    /// Set all flags based on the string.
    void SetFlags(const csString& flagStr);

    /** Enable/Disable the listed flags
     *
     * Will search the flagstr input to find the flag
     * to enable or disable.
     *
     * @param db      Need a pointer to the db to update flags
     * @param flagstr The flag to modify.
     * @param enable  Set to true to enable the flag.
     * @return        True if flags where set.
     */
    bool SetFlag(iDataConnection * db, const csString &flagstr, bool enable);

    /// Data used in the dijkstra's algorithm to find waypoint path
    float distance; /// Hold current shortest distance to the start WP.
    Waypoint * pi;  /// Predecessor WP to track shortest way back to start.
    bool excluded;  /// Set to true if the waypoint is filtered out.

};

#endif
