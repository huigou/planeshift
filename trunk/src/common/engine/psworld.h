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

struct iBgLoader;
struct iCollection;
struct iEngine;

/**
 * psWorld is in charge of managing all regions (zone map files)
 * and loading/unloading them as needed.  The main user of this
 * class is EntityManager.
 */
class psWorld 
{
protected:
    csRef<iStringArray> regions;
    iObjectRegistry *object_reg;
    csWeakRef<iEngine> engine;
    csRef<iBgLoader> loader;

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
    bool Initialize(iObjectRegistry* object_reg);

    /// Create a new psRegion entry and load it if specified
    bool NewRegion(const char *mapfile, bool loadMeshes = true);

    /// This makes a string out of all region names, separated by | chars.
    void GetAllRegionNames(csString& str);

    /// Changes pos according to the warp portal between adjacent sectors from and to.
    bool WarpSpace(const iSector* from, const iSector* to, csVector3& pos);

    /// Calculate the distance between two to points either in same or different sectors.
    float Distance(const csVector3& from_pos, const iSector* from_sector, csVector3 to_pos, const iSector* to_sector);
    
    /// Calculate the distance between two meshes either in same or different sectors.
    float Distance(iMeshWrapper * ent1, iMeshWrapper * ent2);

    /// Return an enties position
    void GetPosition(iMeshWrapper *entity, csVector3& pos, float* yrot, iSector*& sector);

    static float Matrix2YRot(const csMatrix3& mat);

    void DumpWarpCache();
};


#endif

