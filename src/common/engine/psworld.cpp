/*
 * psworld.cpp - author Matze Braun <MatzeBraun@gmx.de> and
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
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstool/collider.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <imesh/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/consoleout.h"
#include "util/log.h"
#include "util/strutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psregion.h"
#include "psworld.h"

psWorld::psWorld()
{
}

psWorld::~psWorld()
{
    transarray.Empty();
}

bool psWorld::Initialize(iObjectRegistry* objectReg, uint _gfxFeatures)
{
    object_reg = objectReg;
    engine = csQueryRegistry<iEngine>(object_reg);
    gfxFeatures = _gfxFeatures;

    return true;
}

bool psWorld::CreateMap( const char* name, const char* mapfile, bool load_now, bool loadMeshes)
{    
    if (!NewRegion(mapfile,load_now, loadMeshes))
        return false;

    return true;
}

psRegion* psWorld::NewRegion(const char *mapfile,bool load, bool loadMeshes)
{
    for (unsigned i=0; i < regions.GetSize(); i++)
    {
        psRegion *rgn = regions[i];
        if (!strcmp(rgn->GetName(),mapfile))
            return rgn;
    }

    psRegion *newregion = new psRegion(object_reg, mapfile, gfxFeatures);
    if (load && !newregion->Load(loadMeshes))
    {
        delete newregion;
        return NULL;
    }
    // This must be rebuilt when the sector list changes
    BuildWarpCache();
    regions.Push(newregion);
    return newregion;
}

void psWorld::GetAllRegionNames(csString& str)
{
    str.Clear();
    for (unsigned i=0; i < regions.GetSize(); i++)
    {
        str.Append( regions[i]->GetName() );
        str.Append( "|" );
    }
}


void psWorld::FlagAllRegionsAsNotNeeded()
{
    for (unsigned i=0; i < regions.GetSize(); i++)
    {
        psRegion *rgn = regions[i];
        rgn->SetNeededFlag(psWorld::NOT_NEEDED);
    }
}

void psWorld::FlagRegionAsNeeded(const char *map)
{
    // if on the list, just retain it with the flag
    for (unsigned i=0; i < regions.GetSize(); i++)
    {
        psRegion *rgn = regions[i];
        if (rgn->GetName() == map)
        {
            rgn->SetNeededFlag(psWorld::NEEDED);
            return;
        }
    }
    // if not on the list, make a new entry
    psRegion *rgn = NewRegion(map,psWorld::DEFER_LOAD);
    rgn->SetNeededFlag(psWorld::NEEDED);
}

void ConnectPortalToSector(iEngine * engine, const char * portalName, const char * sectorName)
{
    iMeshList * meshes = engine->GetMeshes();
    iMeshWrapper * wrap = meshes->FindByName(portalName);
    if (wrap)
    {
        iMeshObject *obj=wrap->GetMeshObject();
        if (obj)
        {
            csRef<iPortalContainer> portalc =  scfQueryInterface<iPortalContainer> (obj);
            if (portalc==NULL)
                return;
            for (int pn=0; pn < portalc->GetPortalCount(); pn++)
            {
                iPortal * portal = portalc->GetPortal(pn);
                iSector * sector = engine->FindSector(sectorName);
                if (sector)
                {
                    Error3("sector %s connected to portal %s", sectorName, portalName);
                    portal->SetSector(sector);
                }
            }
        }
    }
}

int psWorld::ExecuteFlaggedRegions(bool transitional)
{
    // Load any regions on the list which are not already loaded.
    for (uint i=0; i < regions.GetSize(); i++)
    {
        psRegion *rgn = regions[i];
        if (!rgn->IsLoaded() )
        {
            if(!rgn->Load())
            {
                Error2("Loading region %s failed!", rgn->GetName());
                return 1;
            }
            // 2 signifies that a region is loaded, and that we need to refresh the screen.
            return 2;
        }
    }

    // Delete and unload regions not flagged to be saved   

    // Transitional means that we should allow whatever
    // levels are already loaded to stay loaded, but still
    // ensure that listed levels are loaded also.
    if (!transitional)
    {
        for (size_t i=regions.GetSize(); i > 0; i--)
        {
            psRegion *rgn = regions[i-1];
            if (!rgn->IsNeeded() )
            {
                Debug2(LOG_LOAD,0,"Region %s is being deleted.\n",rgn->GetName() );

                regions.DeleteIndex(i-1);
            }
        }
    }

    return 0;
}

bool psWorld::NeedsLoading(bool transitional)
{

    // Transitional means that we should allow whatever
    // levels are already loaded to stay loaded, but still
    // ensure that listed levels are loaded also.
    if (!transitional)
        for (unsigned i=0; i < regions.GetSize(); i++)
            if (!regions[i]->IsNeeded() )
                return true;

    for (unsigned i=0; i < regions.GetSize(); i++)
        if (!regions[i]->IsLoaded() )
            return true;
    return false;
}

bool psWorld::IsAllLoaded()
{
    for (unsigned i=0; i < regions.GetSize(); i++)
    {
        psRegion *rgn = regions[i];
        if (!rgn->IsLoaded() )
        {
            return false;            
        }
    }

    return true;
}

void psWorld::GetNotNeededRegions(csArray<iCollection*> & regs)
{
    for (unsigned i=0; i < regions.GetSize(); i++)
        if (!regions[i]->IsNeeded() )
            regs.Push(regions[i]->GetCollection());
}

void psWorld::BuildWarpCache()
{
    int sectorCount = engine->GetSectors()->GetCount();
    Debug2(LOG_STARTUP,0,"Building warp cache for %d sectors...",sectorCount);

    /// Clear existing entries
    transarray.Empty();

    transarray.SetSize(sectorCount);

    csReversibleTransform identity;

    for(int i=0; i<sectorCount; i++)
    {
        const csSet<csPtrKey<iMeshWrapper> >& portals = engine->GetSectors()->Get(i)->GetPortalMeshes();
        Debug3(LOG_STARTUP,0," %zu portal meshes for %s",portals.GetSize(),
            engine->GetSectors()->Get(i)->QueryObject()->GetName());

        csSet<csPtrKey<iMeshWrapper> >::GlobalIterator it = portals.GetIterator ();
        while (it.HasNext ())
        {
            iMeshWrapper* portal_mesh = it.Next ();
            iPortalContainer* pc = portal_mesh->GetPortalContainer ();
            for (int j = 0 ; j < pc->GetPortalCount () ; j++)
            {
                iPortal* portal = pc->GetPortal (j);
                if (portal->CompleteSector(0))
                {
                    if (engine->GetSectors()->Find(portal->GetSector()) != -1)
                    {
                        transarray[i].Set(portal->GetSector(), portal->GetWarp());
                    }
                }
            }
        }
    }
}

void psWorld::DumpWarpCache()
{
    for(size_t i=0; i<transarray.GetSize(); i++)
    {
        csHash<csReversibleTransform*, csPtrKey<iSector> >::GlobalIterator it =  transarray[i].GetIterator();
        iSector * fromSector = engine->GetSectors()->Get((int)i);
        CPrintf(CON_CMDOUTPUT,"%s\n",fromSector->QueryObject()->GetName());
        while (it.HasNext())
        {
            csPtrKey<iSector>  sector;
            csReversibleTransform* rt = it.Next(sector);
            CPrintf(CON_CMDOUTPUT,"  %-20s : %s\n",sector->QueryObject()->GetName(),toString(*rt).GetData());

        }
    }
}


bool psWorld::WarpSpace(const iSector* from, const iSector* to, csVector3& pos)
{
    if(from == to)
        return true; // No need to transform, pos ok.

    int i = engine->GetSectors()->Find((iSector*)from);
    if (i == -1)
    {
        return false; // Didn't find transformation, pos not ok. 
    }

    csReversibleTransform* transform = transarray[i].Get((iSector*)to);
    if(transform)
    {
        pos = *transform * pos;
        return true; // Position transformed, pos ok.
    }

    return false; // Didn't find transformation, pos not ok.
}

float psWorld::Distance(const csVector3& from_pos, const iSector* from_sector, csVector3 to_pos, const iSector* to_sector)
{
    if (from_sector == to_sector)
    {
        return (from_pos - to_pos).Norm();
    }
    else
    {
        if (WarpSpace(to_sector, from_sector, to_pos))
        {
            return (from_pos - to_pos).Norm();
        }
        else
        {
            return 9999999.99f; // No transformation found, so just set larg distance.
        }

    }
}


float psWorld::Distance(iMeshWrapper * ent1, iMeshWrapper * ent2)
{
    csVector3 pos1,pos2;
    iSector *sector1,*sector2;
    

    GetPosition(ent1,pos1,NULL,sector1);
    GetPosition(ent2,pos2,NULL,sector2);
    
    return Distance(pos1,sector1,pos2,sector2);
}



void psWorld::GetPosition(iMeshWrapper *pcmesh, csVector3& pos, float* yrot,iSector*& sector)
{
    pos = pcmesh->GetMovable()->GetPosition();

    // rotation
    if (yrot)
    {
        csMatrix3 transf = pcmesh->GetMovable()->GetTransform().GetT2O();
        *yrot = Matrix2YRot(transf);
    }

    // Sector
    if (pcmesh->GetMovable()->GetSectors()->GetCount())
    {
        sector = pcmesh->GetMovable()->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }
}


float psWorld::Matrix2YRot(const csMatrix3& mat)
{
    csVector3 vec(0,0,1);
    vec = mat * vec;

    float result = atan2(vec.z, vec.x);
    // force the result in range [0;2*pi]
    if(result < 0) result += TWO_PI;

    return result;
}
