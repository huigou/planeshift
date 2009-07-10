/*
 * psworld.h - author Matze Braun <MatzeBraun@gmx.de> and
 *              Keith Fulton <keith@paqrat.com>
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
 */
#ifndef __PSWORLD_H__
#define __PSWORLD_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/transfrm.h>
#include <csutil/parray.h>
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

struct iCollection;
struct iEngine;
class psRegion;

/**
 * psWorld is in charge of managing all regions (zone map files)
 * and loading/unloading them as needed.  The main users
 * of this class are EntityManager and on the client, ZoneHandler.
 */
class psWorld 
{
protected:
    csPDelArray<psRegion> regions;
    iObjectRegistry *object_reg;
    csWeakRef<iEngine> engine;

    class sectorTransformation
    {
        // Transformation
        csHash<csReversibleTransform*, csPtrKey<iSector> > trans;
        
    public:
        csHash<csReversibleTransform*, csPtrKey<iSector> >::GlobalIterator GetIterator()
            {
                return trans.GetIterator();
            }
        
        
        void Set(iSector* sector, csReversibleTransform transform)
        {
            csReversibleTransform* transf = trans.Get(csPtrKey<iSector> (sector), NULL);
            
            if(!transf)
            {
                transf = new csReversibleTransform();
                trans.Put(csPtrKey<iSector> (sector), transf);
            }

            *transf = transform;
        }

        csReversibleTransform* Get(iSector* sector)
        {
            csReversibleTransform* got = trans.Get(csPtrKey<iSector> (sector), NULL);

            return got;
        }

        ~sectorTransformation()
        {
            csHash<csReversibleTransform*, csPtrKey<iSector> >::GlobalIterator iter = trans.GetIterator();
            while(iter.HasNext())
                delete iter.Next();
            
            trans.Empty();
        }

    };

    csArray<sectorTransformation> transarray;

    void BuildWarpCache();
public:
    psWorld();
    ~psWorld();
 
    /// Initialize psWorld
    bool Initialize(iObjectRegistry* object_reg, uint gfxFeatures = 0);
    bool CreateMap(const char* name, const char* mapFile, bool loadNow, bool loadMeshes = true); 
    enum
    {
        DEFER_LOAD = 0,
        LOAD_NOW   = 1
    };

    /// Create a new psRegion entry and load it if specified
    psRegion *NewRegion(const char *mapfile,bool load_now, bool loadMeshes = true);

    /// This makes a string out of all region names, separated by | chars.
    void GetAllRegionNames(csString& str);

    /**
     * Mark all regions to be unloaded initially.  The ZoneHandler
     * clears the list whenever a sector is crossed and verifies
     * that all the right zones are loaded and unloaded.  The algorithm
     * is:
     *     1) Flag everything as unload
     *     2) Flag the things that should be loaded or retained based
     *        on the zone information ZoneHandler has.  If the region
     *        exists, it is just flagged to be retained.  If it doesn't
     *        exist yet, it is added to the list but not loaded yet.
     *     3) When this loop is through, regions that were not touched
     *        are still flagged to be unloaded per step 1.
     *     4) ExecuteFlaggedRegions first unloads any entries flagged
     *        as unloaded, deleting their entries, then loads any entries
     *        flagged to be loaded which are not already in memory.
     */
    void FlagAllRegionsAsNotNeeded();
    enum 
    {
        NOT_NEEDED = 0,
        NEEDED = 1
    };
    /**
     * Flag a region to be kept, or add a new entry to the list if not found.
     * The new entry will not be loaded immediately, but only by ExecuteFlaggedRegions().
     */
    void FlagRegionAsNeeded(const char *map);

    void GetNotNeededRegions(csArray<iCollection*> & regions);

    /// Unload all regions to be unloaded, and load any regions to be kept but
    /// which are not loaded already.
    int ExecuteFlaggedRegions(bool transitional);

    bool NeedsLoading(bool transitional);

    bool IsAllLoaded();

    /// Changes pos according to the warp portal between adjacent sectors from and to.
    bool WarpSpace(const iSector* from, const iSector* to, csVector3& pos);

    /// Calculate the distance between two to points either in same or different sectors.
    float Distance(const csVector3& from_pos, const iSector* from_sector, csVector3 to_pos, const iSector* to_sector);
    
    /// Calculate the distance between two meshes either in same or different sectors.
    float Distance(iMeshWrapper * ent1, iMeshWrapper * ent2);

    /// Return an enties position
    void GetPosition(iMeshWrapper *entity, csVector3& pos, float* yrot, iSector*& sector);

    static float Matrix2XRot(const csMatrix3& mat);
    static float Matrix2YRot(const csMatrix3& mat);
    static float Matrix2ZRot(const csMatrix3& mat);

    void DumpWarpCache();
private:
    uint gfxFeatures;
};


#endif

