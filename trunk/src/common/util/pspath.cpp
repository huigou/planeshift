/*
* pspath.cpp
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

// CS
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/movable.h>

#include "util/pspath.h"
#include "util/location.h"
#include "util/log.h"
#include "util/psdatabase.h"
#include "engine/psworld.h"

bool psPathPoint::Load(iResultRow& row, iEngine *engine)
{
    id          = row.GetInt("id");
    prevPointId = row.GetInt("prev_point");
    pos.x       = row.GetFloat("x");
    pos.y       = row.GetFloat("y");
    pos.z       = row.GetFloat("z");
    sectorName  = row["sector_name"];
    sector      = engine->FindSector(sectorName);

    return true;
}

iSector* psPathPoint::GetSector(iEngine * engine)
{
    if (sector) return sector;
    
    sector = engine->FindSector(sectorName.GetDataSafe());
    return sector;
}


//---------------------------------------------------------------------------

psPath::psPath(int pathID, csString name)
    :id(pathID),name(name),start(NULL),end(NULL),precalculationValid(false)
{
}

void psPath::AddPoint(Location * loc)
{
    psPathPoint * pp = new psPathPoint();

    pp->id = -1;
    pp->pos = loc->pos;
    pp->sectorName = loc->sectorName;

    points.Push(pp);
}

void psPath::Precalculate(psWorld * world, iEngine *engine)
{
    if (precalculationValid) return;

    PrecalculatePath(world,engine);
    
    precalculationValid = true;
}

                               
csVector3 psPath::GetEndPos(Direction direction)
{
    if (direction == FORWARD)
    {
        return points[points.GetSize()-1]->pos;
    }
    else
    {
        return points[0]->pos;
    }
}

float psPath::GetEndRot(Direction direction)
{
    if (direction == FORWARD)
    {
        return CalculateIncidentAngle(points[points.GetSize()-2]->pos,points[points.GetSize()-1]->pos);
    }
    else
    {
        return CalculateIncidentAngle(points[1]->pos,points[0]->pos);
    }    
}

iSector* psPath::GetEndSector(iEngine * engine, Direction direction)
{
    if (direction == FORWARD)
    {
        return points[points.GetSize()-1]->GetSector(engine);
    }
    else
    {
        return points[0]->GetSector(engine);
    }
}

psPathAnchor* psPath::CreatePathAnchor()
{
    return new psPathAnchor(this);
}

float psPath::GetLength(psWorld * world, iEngine *engine)
{ 
    Precalculate(world,engine);
    return totalDistance;
}


bool psPath::Load(iDataConnection * db, iEngine *engine)
{
    Result rs1(db->Select("select pp.*,s.name AS sector_name from sc_path_points pp, sectors s where pp.loc_sector_id = s.id and path_id='%d'",id));
 
    if (!rs1.IsValid())
    {
        Error2("Could not load path points from db: %s",db->GetLastError() );
        return false;
    }
    csArray<psPathPoint*>  tempPoints;
    for (int i=0; i<(int)rs1.Count(); i++)
    {
        psPathPoint * pp = new psPathPoint();
        
        pp->Load(rs1[i],engine);

        tempPoints.Push(pp);
    }

    int currPoint = 0, prevPoint=0;
    size_t limit = tempPoints.GetSize()*tempPoints.GetSize();
    while (tempPoints.GetSize() && limit)
    {
        if (tempPoints[currPoint]->prevPointId == prevPoint)
        {
            psPathPoint * pp = tempPoints[currPoint];
            tempPoints.DeleteIndexFast(currPoint);
            
            points.Push(pp);
            prevPoint = pp->id;
            currPoint--;
        }
        currPoint++;

        if (currPoint >= (int)tempPoints.GetSize())
        {
            currPoint = 0;
        }

        limit--;
    }
    if (limit == 0)
    {
        Error2("Could not assamble path %d",id);
        return false;
    }
    
    return true;
}

float psPath::CalculateIncidentAngle(csVector3& pos, csVector3& dest)
{
    csVector3 diff = dest-pos;  // Get vector from player to desired position

    if (!diff.x)
        diff.x = .000001F; // div/0 protect

    float angle = atan2(-diff.x,-diff.z);

    return angle;
}


//---------------------------------------------------------------------------

psLinearPath::psLinearPath(int pathID, csString name)
    :psPath(pathID,name)
{
}

void psLinearPath::PrecalculatePath(psWorld * world, iEngine *engine)
{
    totalDistance = 0;

    for (size_t ii=0;ii<points.GetSize()-1;ii++)
    {
        csVector3 pos1(points[ii]->pos),pos2(points[ii+1]->pos);

        world->WarpSpace(points[ii+1]->GetSector(engine),points[ii]->GetSector(engine),pos2);

        float dist = (pos2-pos1).Norm();

        points[ii]->startDistance[FORWARD] = totalDistance;

        totalDistance += dist;
        
        dx.Push( pos2.x - pos1.x);
        dy.Push( pos2.y - pos1.y);
        dz.Push( pos2.z - pos1.z);        
    }    
    points[points.GetSize()-1]->startDistance[FORWARD] = totalDistance;

    for (size_t ii=0;ii<points.GetSize();ii++)
    {
        points[ii]->startDistance[REVERSE] =  totalDistance - points[ii]->startDistance[FORWARD];
    }
}

void psLinearPath::GetInterpolatedPosition (int index, float fraction, csVector3& pos)
{
    psPathPoint * pp = points[index];
    pos.x = pp->pos.x + dx[index]* fraction;
    pos.y = pp->pos.y + dy[index]* fraction;
    pos.z = pp->pos.z + dz[index]* fraction;
}

void psLinearPath::GetInterpolatedUp (int index, float fraction, csVector3& up)
{
    up = csVector3(0,1,0);
}
    
void psLinearPath::GetInterpolatedForward (int index, float fraction, csVector3& forward)
{
    forward.x = dx[index];
    forward.y = dy[index];
    forward.z = dz[index];
}

//---------------------------------------------------------------------------

psPathAnchor::psPathAnchor(psPath * path)
    :path(path),pathDistance(0.0)
{
}

bool psPathAnchor::CalculateAtDistance(psWorld * world, iEngine *engine, float distance, psPath::Direction direction)
{
    float start = 0.0 ,end = 0.0; // Start and end distance of currentAtIndex

    // Check if we are at the end of the path. GetLength will indirect precalculate the path
    if (distance < 0.0 || distance > path->GetLength(world,engine))
    {
        return false;
    }
    
    if (direction == psPath::FORWARD)
    {
        // First find the current index.
        for (currentAtIndex = 0; currentAtIndex < (int)path->points.GetSize() -1 ; currentAtIndex++)
        {
            start = path->points[currentAtIndex]->startDistance[direction];
            end   = path->points[currentAtIndex+1]->startDistance[direction];
            if (distance >= start && distance <= end) break; // Found segment
        }
        currentAtFraction = (distance - start) / (end - start);
    } else
    {
        // First find the current index.
        for (currentAtIndex = 0; currentAtIndex < (int)path->points.GetSize() -1 ; currentAtIndex++)
        {
            start = path->points[currentAtIndex+1]->startDistance[direction];
            end   = path->points[currentAtIndex]->startDistance[direction];
            if (distance >= start && distance <= end) break; // Found segment
        }
        currentAtFraction = (end - distance) / (end - start);
    }

    currentAtDirection = direction;
    
    return true;
}

void psPathAnchor::GetInterpolatedPosition (csVector3& pos)
{
    path->GetInterpolatedPosition(currentAtIndex,currentAtFraction,pos);
}

void psPathAnchor::GetInterpolatedUp (csVector3& up)
{
    path->GetInterpolatedUp(currentAtIndex,currentAtFraction,up);
}

void psPathAnchor::GetInterpolatedForward (csVector3& forward)
{
    path->GetInterpolatedForward(currentAtIndex,currentAtFraction,forward);

    if (currentAtDirection != psPath::REVERSE)
    {
        forward = -forward;
    }
}

bool psPathAnchor::Extrapolate(psWorld * world, iEngine *engine, float delta, psPath::Direction direction, iMovable * movable)
{
    pathDistance += delta;

    if (!CalculateAtDistance (world, engine, pathDistance, direction))
    {
        return false; // At end of path
    }
    

    csVector3 pos, look, up;
    
    GetInterpolatedPosition (pos);
    GetInterpolatedUp (up);
    GetInterpolatedForward (look);

    movable->GetTransform().SetOrigin (pos);
    movable->GetTransform().LookAt(
    	look.Unit (), up.Unit ());
    movable->UpdateMove ();

    return true;
}


