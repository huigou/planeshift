/*
 * pspath.h
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
#ifndef __PSPATH_H__
#define __PSPATH_H__

#include <csutil/weakref.h>
#include <igeom/path.h>

#include "iserver/idal.h"

class Waypoint;
class psPathAnchor;
class psWorld;
struct iMovable;
class Location;

class psPathPoint
{
public:
    int                    id;
    int                    prevPointId;
    csVector3              pos;
    csString               sectorName;       /// Should realy only be the pointer, but
                                             /// since sector might not be available
                                             /// when loaded we use the name for now.

    // Internal data
    csWeakRef<iSector>     sector;           /// Cached sector
    float                  startDistance[2]; /// Start distance for FORWARD and REVERS

    bool Load(iResultRow& row, iEngine *engine);
    
    iSector * GetSector(iEngine *engine);
};

class psPath
{
    friend class psPathAnchor;
public:
    typedef enum {
        FORWARD,
        REVERSE
    } Direction;
            
    int                    id;
    csString               name;
    Waypoint              *start,*end; /// This path start and end waypoint
    csArray<psPathPoint*>  points;

    bool                   precalculationValid;
    float                  totalDistance;
    
    psPath(int pathID, csString name);
    
    virtual ~psPath() {}

    /// Load the path from the db
    bool Load(iDataConnection * db, iEngine *engine);

    /// Add a new point to the path
    void AddPoint(Location * loc);

    /// Precalculate values needed for anchors
    virtual void Precalculate(psWorld * world, iEngine *engine);


    /// Get the end point
    csVector3 GetEndPos(Direction direction);
    /// Get the end rotation
    float GetEndRot(Direction direction);
    /// Get the end sector
    iSector* GetEndSector(iEngine * engine, Direction direction);
    
    /// Return a path anchor to this path
    virtual psPathAnchor* CreatePathAnchor();

    /// Get number of points in path
    virtual int GetNumPoints () { return (int)points.GetSize(); }

    /// Get name of the path
    virtual const char* GetName() { return name.GetDataSafe(); }

    /// Get the total length of all path segments.
    virtual float GetLength(psWorld * world, iEngine *engine);

    /// Utility function to calcualte angle to point between to points
    float CalculateIncidentAngle(csVector3& pos, csVector3& dest);
    
protected:
    /// Do the actual precalculate work
    virtual void PrecalculatePath(psWorld * world, iEngine *engine) = 0;

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (int index, float fraction, csVector3& pos) = 0;

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (int index, float fraction, csVector3& up) = 0;
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (int index, float fraction, csVector3& forward) = 0;
};

class psLinearPath: public psPath
{
public:
    psLinearPath(int pathID, csString name);
    virtual ~psLinearPath(){};

protected:
    // Do the actual precalculate work
    virtual void PrecalculatePath(psWorld * world, iEngine *engine);

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (int index, float fraction, csVector3& pos);

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (int index, float fraction, csVector3& up);
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (int index, float fraction, csVector3& forward);
    
private:
    csArray<float> dx;
    csArray<float> dy;
    csArray<float> dz;
};

class psPathAnchor
{
public:
    psPathAnchor(psPath * path);
    virtual ~psPathAnchor() {}

    /// Calculate internal values for this path given some distance value.
    virtual bool CalculateAtDistance(psWorld * world, iEngine *engine, float distance, psPath::Direction direction);

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (csVector3& pos);

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (csVector3& up);
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (csVector3& forward);

    /// Extrapolate the movable delta distance along the path
    virtual bool Extrapolate(psWorld * world, iEngine *engine, float delta, psPath::Direction direction, iMovable * movable);


    /// Get the current distance this anchor has moved along the path
    float GetDistance(){ return pathDistance; }
    /// Get the index to the current path point
    int GetCurrentAtIndex() { return currentAtIndex; }
    /// Get the current direction
    psPath::Direction GetCurrentAtDirection() { return currentAtDirection; }
    /// Get the current fraction of the current path segment
    float GetCurrentAtFraction(){ return currentAtFraction; }
    
private:
    psPath * path;
    // Internal non reentrant data leagal after calcuateAt operation
    int                    currentAtIndex;
    psPath::Direction      currentAtDirection;
    float                  currentAtFraction;

    float                  pathDistance;    
};

#endif
