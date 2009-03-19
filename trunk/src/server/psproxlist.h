/*
 * psproxlist.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 * 
 */
#ifndef __PS_IMP_PROXIMITYLIST__
#define __PS_IMP_PROXIMITYLIST__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstypes.h>
#include <iutil/comp.h>
#include <csutil/scf.h>
#include <net/netbase.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

//=============================================================================
// Local Includes
//=============================================================================

class gemObject;
struct iObjectRegistry;

/* ProximityList must maintain following properties:
 *    - values in objectsThatWatchMe are unique
 *    - values in objectsThatIWatch  are unique
 *    - object X is in objectsThatWatchMe of object Y <===> object Y must be in objectsThatIWatch of X
 *    - objects with GetClientID()==0 have empty objectsThatIWatch
 *    - correspondence between objectsThatWatchMe and objectsThatWatchMe_touched
 *                             objectsThatIWatch  and objectsThatIWatch_touched
 */

class ProximityList
{
protected:
    gemObject *self;

    csArray<PublishDestination> objectsThatWatchMe;   ///< What players are subscribed to my updates?
    csArray<gemObject*>  objectsThatIWatch;           ///< What objects am I subscribed to myself?

    csArray<bool> objectsThatWatchMe_touched;
    csArray<bool> objectsThatIWatch_touched;

    int          clientnum;
    bool         firstFrame;
    csVector3    oldPos;
    InstanceID  oldInstance;

    /** Adds 'interestedobject' to 'objectsThatWatchMe' */
    void AddWatcher(gemObject *interestedobject, float range);

    /** Removes 'interestedobject' from 'objectsThatWatchMe' */
    void RemoveWatcher(gemObject *object);

    bool IsNear(iSector *sector,csVector3& pos,gemObject *object,float radius);
    bool FindObject(gemObject *object);
    PublishDestination *FindObjectThatWatchesMe(gemObject *object);
    bool FindObjectThatIWatch(gemObject *object);

    void TouchObjectThatWatchesMe(gemObject *object,float newrange);
    void UpdatePublishDestRange(PublishDestination *pd, 
                                gemObject *myself, 
                                gemObject *object, 
                                float newrange);

public:
    ProximityList( iObjectRegistry* object_reg, gemObject *parent );
    ~ProximityList();

    bool Initialize(int cnum,gemObject *parent);

    /** Creates relation watcher --> watched between our object and given object
      * Returns: true if new relation was added */
    bool StartWatching(gemObject * object, float range);

    /** Deletes relation watcher --> watched between our object and given object */
    void EndWatching(gemObject * object);

    /** Deletes all relations where our object plays role of "watched" object
      * The only exception is the relation where our object watches itself */
    //void UnsubscribeAllWatchers();

    /** Creates relation watcher --> watched in both directions between our object and given object */
    bool StartMutualWatching(int othercnum, gemObject *otherObject,float range);

    /** Deletes relation watcher --> watched in both directions between our object and given object */
    bool EndMutualWatching(gemObject *fromobject);

    csArray<PublishDestination>& GetClients() { return objectsThatWatchMe; }
    int GetClientID() { return clientnum; }

    bool FindClient(int cnum);
    gemObject *FindObjectName(const char *name);

    /**
     * Check if an update is required for this object based on moved range and changed instance.
     */
    bool CheckUpdateRequired();
    float RangeTo( gemObject* object, bool ignoreY = false, bool ignoreInstance = false );
    void DebugDumpContents(csString& out);

    void ClearTouched();
    bool GetUntouched_ObjectThatWatchesMe(gemObject* &object);
    bool GetUntouched_ObjectThatIWatch(gemObject* &object);
};

#endif 

