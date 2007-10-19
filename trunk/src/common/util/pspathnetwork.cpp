/*
* pspathnetwork.cpp
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
#include <psconfig.h>

#include <iengine/engine.h>
#include <iengine/sector.h>

#include "pspathnetwork.h"
#include "util/psutil.h"
#include "util/log.h"
#include "util/psdatabase.h"
#include "util/psstring.h"
#include "engine/psworld.h"
#include "util/waypoint.h"
#include "util/consoleout.h"

#ifndef INFINITY
#define INFINITY 999999999.0F
#endif

bool psPathNetwork::Load(iEngine *engine, iDataConnection *db,psWorld * world)
{
    Result rs(db->Select("select wp.*,s.name as sector from sc_waypoints wp, sectors s where wp.loc_sector_id = s.id"));

    if (!rs.IsValid())
    {
        Error2("Could not load waypoints from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs.Count(); i++)
    {
        Waypoint *wp = new Waypoint();

        if (wp->Load(rs[i],engine))
        {
           waypoints.Push(wp);
        }
        else
        {
            Error2("Could not load waypoint: %s",db->GetLastError() );            
            delete wp;
            return false;
        }
        
    }

    Result rs1(db->Select("select * from sc_waypoint_links"));

    if (!rs1.IsValid())
    {
        Error2("Could not load waypoint links from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs1.Count(); i++)
    {
        Waypoint * wp1 = FindWaypoint(rs1[i].GetInt("wp1"));
        Waypoint * wp2 = FindWaypoint(rs1[i].GetInt("wp2"));
        psString flagstr(rs1[i]["flags"]);

        bool oneWay = flagstr.FindSubString("ONEWAY",0,true)!=-1;
        bool preventWander = flagstr.FindSubString("NO_WANDER",0,true)!=-1;


        int pathId = rs1[i].GetInt("id");
        
        csString pathType = rs1[i]["type"];
        
        psPath * path;
        if (strcasecmp(pathType,"linear") == 0)
        {
            path = new psLinearPath(pathId,rs1[i]["name"]);
        } else
        {
            path = new psLinearPath(pathId,rs1[i]["name"]); // For now
        }

        path->AddPoint(&wp1->loc);
        path->Load(db,engine);
        path->AddPoint(&wp2->loc);
        path->start = wp1;
        path->end = wp2;
        paths.Push(path);

        float dist = path->GetLength(world,engine);
                                    
        wp1->links.Push(wp2);
        wp1->paths.Push(path);
        wp1->pathDir.Push(psPath::FORWARD);
        wp1->dists.Push(dist);
        wp1->prevent_wander.Push(preventWander);
        
        if (!oneWay)
        {
            wp2->links.Push(wp1);  // bi-directional link is implied
            wp2->paths.Push(path);
            wp1->pathDir.Push(psPath::REVERSE);
            wp2->dists.Push(dist);
            wp2->prevent_wander.Push(preventWander);
        }
        
    }
    
    
    return true;
}


Waypoint *psPathNetwork::FindWaypoint(int id)
{
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    while (iter.HasNext())
    {
        wp = iter.Next();

        if (wp->loc.id == id)
        {
            return wp;
        }
    }
    
    return NULL;
}

Waypoint *psPathNetwork::FindWaypoint(const char * name)
{
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    while (iter.HasNext())
    {
        wp = iter.Next();

        if (strcasecmp(wp->GetName(),name)==0)
        {
            return wp;
        }
    }
    
    return NULL;
}

Waypoint *psPathNetwork::FindNearestWaypoint(psWorld * world,iEngine *engine, csVector3& v,iSector *sector, float range, float * found_range)
{
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    float min_range = range;

    Waypoint *min_wp = NULL;

    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (min_range < 0 || dist2 < min_range)
        {
            min_range = dist2;
            min_wp = wp;
        }
    }
    if (min_wp && found_range) *found_range = min_range;

    return min_wp;
}

Waypoint *psPathNetwork::FindRandomWaypoint(psWorld *world,iEngine *engine,csVector3& v,iSector *sector, float range, float * found_range)
{
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    csArray<Waypoint*> nearby;
    csArray<float> dist;
 
    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (range < 0 || dist2 < range)
        {
            nearby.Push(wp);
            dist.Push(dist2);
        }
    }

    if (nearby.GetSize()>0)  // found one or more closer than range
    {
        size_t pick = psGetRandom((uint32)nearby.GetSize());
        
        if (found_range) *found_range = sqrt(dist[pick]);
        
        return nearby[pick];
    }


    return NULL;
}

csList<Waypoint*> psPathNetwork::FindWaypointRoute(Waypoint * start, Waypoint * end)
{
    csList<Waypoint*> waypoint_list;
    csList<Waypoint*> priority; // Should have been a priority queue
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;
    
    // Using Dijkstra's algorithm to find shortest way

    // Initialize
    while (iter.HasNext())
    {
        wp = iter.Next();

        wp->distance = INFINITY;
        wp->pi = NULL;
        
        // Insert WP into priority queue
        priority.PushBack(wp);
    }
    start->distance = 0;

    while (!priority.IsEmpty())
    {
        Waypoint *wp_u = NULL, *pri_wp = NULL;
        // Extract min from priority queue
        csList<Waypoint*>::Iterator pri(priority);
        csList<Waypoint*>::Iterator pri_loc;
        while (pri.HasNext())
        {
            pri_wp = pri.Next();

            if (!wp_u || pri_wp->distance < wp_u->distance)
            {
                wp_u = pri_wp;
                pri_loc = pri;
            }
        }
        priority.Delete(pri_loc);
        size_t v;
        for (v = 0; v < wp_u->links.GetSize(); v++)
        {
            Waypoint * wp_v = wp_u->links[v];
            // Relax
            if (wp_v->distance > wp_u->distance + wp_u->dists[v])
            {
                wp_v->distance = wp_u->distance + wp_u->dists[v];
                wp_v->pi = wp_u;
            }
        }
        // if wp == end, we should be done!!!!!!
    }

    wp = end;

    if (end->pi)
    {
        wp = end;
        while (wp)
        {
            waypoint_list.PushFront(wp);
            wp = wp->pi;
        }
    }

    return waypoint_list;
}


void psPathNetwork::ListWaypoints(const char * pattern)
{
    csArray<Waypoint*>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    CPrintf(CON_CMDOUTPUT, "%9s %-30s %-40s %-10s %10s %s\n", "WP id", "Name", "Position","Radius","Dist","PI");
    while (iter.HasNext())
    {
        wp = iter.Next();

        if (!pattern || strstr(wp->GetName(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%9d %-30s (%9.3f,%9.3f,%9.3f, %s) %9.3f %9.3f %s" ,
                    wp->loc.id,wp->GetName(),wp->loc.pos.x,wp->loc.pos.y,wp->loc.pos.z,
                    wp->loc.sectorName.GetDataSafe(),wp->loc.radius,wp->distance,
                    (wp->pi?wp->pi->GetName():""));

            for (size_t i = 0; i < wp->links.GetSize(); i++)
            {
                CPrintf(CON_CMDOUTPUT," %s%s(%.1f)",(wp->prevent_wander[i]?"#":""),
                        wp->links[i]->GetName(),wp->dists[i]);
            }

            CPrintf(CON_CMDOUTPUT,"\n");
        }
    }
}


void psPathNetwork::ListPaths(const char *name)
{
    csArray<psPath*>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();

        CPrintf(CON_CMDOUTPUT,"%9d %6d %6d %-30s\n",path->id,(path->start?path->start->loc.id:0),
                (path->end?path->end->loc.id:0),path->name.GetDataSafe());

        for (int i = 0; i < path->GetNumPoints();i++)
        {
            psPathPoint * pp = path->points[i];
            CPrintf(CON_CMDOUTPUT, "%9d (%9.3f,%9.3f,%9.3f, %s) %6.3f %6.3f\n" ,
                    pp->id,pp->pos.x,pp->pos.y,pp->pos.z,
                    pp->sectorName.GetDataSafe(),pp->startDistance[psPath::FORWARD],pp->startDistance[psPath::REVERSE]);
        }
    }
}

psPath   *psPathNetwork::FindPath(const char *name)
{
    csArray<psPath*>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();
        if (strcmp ( path->GetName(),name) == 0)
        {
            return path;
        }
    }

    return NULL;
}

psPath   *psPathNetwork::FindPath(const Waypoint * wp1, const Waypoint * wp2, psPath::Direction & direction)
{
    csArray<psPath*>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();

        if (path->start == wp1 && path->end == wp2)
        {
            // Forward path
            direction = psPath::FORWARD;
            return path;
        }

        if (path->start == wp2 && path->end == wp1)
        {
            // Reverse path
            direction = psPath::REVERSE;
            return path;
        }
    }

    return NULL;
}
