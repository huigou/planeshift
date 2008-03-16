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

// CS
#include <iengine/rview.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <csutil/snprintf.h>
#include <csutil/sysfunc.h>
#include <iengine/region.h>
#include <iutil/vfs.h>
#include <csutil/csstring.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include <igeom/trimesh.h>
#include <imesh/objmodel.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <imesh/thing.h>
#include <csutil/databuf.h>
#include <iengine/renderloop.h>
#include <csutil/xmltiny.h>
#include <iengine/movable.h>
#include <iutil/objreg.h>

// PS
#include "engine/materialmanager.h"
#include "engine/psworld.h"
#include "util/psconst.h"
#include "util/log.h"
#include "util/strutil.h"
#include "util/consoleout.h"
#include "globals.h"

#define SHARED_REGION_NAME "SharedDataRegion"

psWorld::psWorld()
{
}

psWorld::~psWorld()
{
    transarray.Empty();
}

bool psWorld::Initialize(iObjectRegistry* objectReg, bool unloadingLast, uint _gfxFeatures)
{
    object_reg = objectReg;
    engine = csQueryRegistry<iEngine>(object_reg);
    startLoading = unloadingLast;
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

    psRegion *newregion = new psRegion(object_reg, this, mapfile, gfxFeatures);
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

int psWorld::ExecuteFlaggedRegions(bool transitional, bool unloadingLast)
{
    if(startLoading)
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
        // Check if any factories, materials or textures can be freed.
        if(!MaterialManager::GetSingletonPtr()->KeepModels())
        {
            MaterialManager::GetSingletonPtr()->UnloadUnusedFactories();
            MaterialManager::GetSingletonPtr()->UnloadUnusedMaterials();
            MaterialManager::GetSingletonPtr()->UnloadUnusedTextures();
        }
    }

    // Mark that we should start loading maps.
    if(!startLoading)
    {
        startLoading = true;
        return 2;
    }

    // Reset loading flag.
    startLoading = unloadingLast;

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

void psWorld::GetNotNeededRegions(csArray<iRegion*> & regs)
{
    for (unsigned i=0; i < regions.GetSize(); i++)
        if (!regions[i]->IsNeeded() )
            regs.Push(regions[i]->GetRegion());
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
    vec.Normalize();

    return GetAngle (vec.z, vec.x);
}

float psWorld::GetAngle(float x, float y)
{
    if ( x > 1.0 )  x = 1.0;
    if ( x < -1.0 ) x = -1.0;

    float angle = acos(x);
    if (y < 0)
        angle = 2*PI - angle;

    return angle;
}

//--------------------------------------------------------------------------

psRegion::psRegion(iObjectRegistry *obj_reg, psWorld * world, const char *file, uint _gfxFeatures)
{
    object_reg = obj_reg;
    this->world = world;

    worlddir.Format("/planeshift/world/%s", file);
    colldetworlddir.Format("/planeshift/world/cd_%s", file);
    worldfile  = "world";
    regionname = file;
    loaded     = false;
    gfxFeatures = _gfxFeatures;
    needToFilter = !(gfxFeatures & useNormalMaps);
}

psRegion::~psRegion()
{
    if (loaded)
        Unload();
}


bool psRegion::Load(bool loadMeshes)
{
    if (loaded)
        return true;

    bool using3D;

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (object_reg);

    // Find out if we are ever going to render 3D
    csRef<iGraphics3D> g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    csRef<iFactory> factory = 
        scfQueryInterface<iFactory> (g3d);

    const char* g3Dname = factory->QueryClassID();
    using3D = (strcmp("crystalspace.graphics3d.null", g3Dname)? true: false);

    csString target;
    target.Format("%s/world", worlddir.GetData());
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( object_reg);

    csRef<iDocumentSystem> xml (
        csQueryRegistry<iDocumentSystem> (object_reg));

    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iDataBuffer> buf (vfs->ReadFile (target.GetData()));
    if (!buf || !buf->GetSize ())
    {
        Error2("Error loading world file. %s\n", target.GetData());
        return false;
    }

    const char* error = doc->Parse( buf );

    if( error )
    {
        Error3("Error %s loading file to be cleaned. %s\n",error, target.GetData());
        return false;
    }

    csRef<iDocumentNode> worldNode = doc->GetRoot()->GetNode("world");

    if(!loadMeshes)
    {
        // Clean the world file to remove all textures/meshes/models
        Debug1(LOG_LOAD, 0,"Cleaning map file.");
        worldNode = Clean(worldNode);
    }
    else if(needToFilter)
    {
        // Filter the world file to get the correct settings.
        worldNode = Filter(worldNode);
    }

    // Create a new region with the given name, or select it if already there
    iRegion* cur_region = engine->CreateRegion (regionname);

    // Clear it out if it already existed
    cur_region->DeleteAll ();

    // Now load the map into the selected region
    csRef<iLoader> loader ( csQueryRegistry<iLoader> (object_reg));
    CS_ASSERT (loader != NULL);
    csRef<iVFS> VFS ( csQueryRegistry<iVFS> (object_reg));
    CS_ASSERT (VFS != NULL);
    VFS->ChDir (worlddir);
    engine->SetCacheManager(NULL);

    csTicks start = csGetTicks();
    Debug2(LOG_LOAD, 0,"Loading map file %s", worlddir.GetData());

    if (!loader->LoadMap(worldNode, CS_LOADER_KEEP_WORLD,cur_region, CS_LOADER_ACROSS_REGIONS, true, 0, MaterialManager::GetSingletonPtr()))
    {
        Error3("loader->LoadMapFile(%s,%s) failed.",worlddir.GetData(),worldfile.GetData() );
        Error2("Region name was: %s", regionname.GetData() );
        return false;
    }
    Debug2(LOG_LOAD, 0,"After LoadMapFile, %dms elapsed", csGetTicks()-start);

    // Successfully loaded.  Now get textures ready, etc. and return.
    if (using3D)
    {
        cur_region->Prepare ();
        Debug2(LOG_LOAD, 0,"After Prepare, %dms elapsed", csGetTicks()-start);
    }

    if (using3D)
    {
        engine->PrecacheDraw (cur_region);
        Debug2(LOG_LOAD, 0,"After Precache, %dms elapsed", csGetTicks()-start);
    }

    if (loadMeshes)
    {
        csRef<iCollideSystem> cdsys = csQueryRegistry<iCollideSystem> (object_reg);

        csString target;
        target.Format("%s/%s", colldetworlddir.GetData(), worldfile.GetData());

        csRef<iDataBuffer> buf = vfs->ReadFile(target.GetData());
        if (!buf || !buf->GetSize())
        {
            SetupWorldColliders(engine, cur_region);
        }
        else
        {
            const char* error = doc->Parse(buf);
            if(error)
            {
                Error3("Error %s while loading colldet world file: %s.\nFalling back to normal colldet, please report this error.\n", error, target.GetData());
                SetupWorldColliders(engine, cur_region);
            }
            else
            {

                csRef<iDocumentNode> worldNodeCD = doc->GetRoot()->GetNode("world");

                iRegion* colldetRegion = engine->CreateRegion("colldetPS");
                colldetRegion->DeleteAll ();

                vfs->ChDir(colldetworlddir);

                if(!loader->LoadMap(worldNodeCD, CS_LOADER_KEEP_WORLD, colldetRegion, CS_LOADER_ACROSS_REGIONS, true))
                {
                    Error3("LoadMap failed: %s, %s.\n", colldetworlddir.GetData(), worldfile.GetData() );
                    Error2("Region name was: %s\nFalling back to normal colldet, please report this error.\n", regionname.GetData());
                    SetupWorldColliders(engine, cur_region);
                }
                else
                {
                    SetupWorldCollidersCD(engine, cur_region, colldetRegion);
                }

                engine->GetRegions()->Remove(colldetRegion);
            }
        }
        Debug2(LOG_LOAD, 0,"After SetupWorldColliders, %dms elapsed\n", csGetTicks()-start);
    }

    loaded = true;

    VFS->ChDir("/planeshift");
    engine->SetCacheManager(NULL);
    engine->GetCacheManager();

    printf("Map %s loaded successfully in %dms\n", (const char *)regionname, csGetTicks()-start);
    return true;
}

csRef<iDocumentNode> psRegion::Clean(csRef<iDocumentNode> world)
{
    csRef<iDocumentSystem> xml (
        csQueryRegistry<iDocumentSystem> (object_reg));

    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> node = doc->CreateRoot();

    // Copy the world node
    csRef<iDocumentNode> cleanedWorld = node->CreateNodeBefore(CS_NODE_ELEMENT);

    cleanedWorld->SetValue("world");

    // Copy the sector node
    csRef<iDocumentNodeIterator> sectors = world->GetNodes("sector");
    while ( sectors->HasNext() )
    {
        csRef<iDocumentNode> sector = sectors->Next();

        csRef<iDocumentNode> cleanedSector = cleanedWorld->CreateNodeBefore(CS_NODE_ELEMENT);
        cleanedSector->SetValue(sector->GetValue());

        // Copy the sector attributes
        csRef<iDocumentAttributeIterator> attrs = sector->GetAttributes();
        while(attrs->HasNext())
        {
            csRef<iDocumentAttribute> attr = attrs->Next();
            cleanedSector->SetAttribute(attr->GetName(), attr->GetValue());
        }

        // Copy the portal
        csRef<iDocumentNodeIterator> nodes = sector->GetNodes("portal");

        while(nodes->HasNext())
        {
            csRef<iDocumentNode> portal = nodes->Next();
            csRef<iDocumentNode> cleanedportal = cleanedSector->CreateNodeBefore(CS_NODE_ELEMENT);
            CloneNode(portal, cleanedportal);
        }

        // Copy the portals
        csRef<iDocumentNodeIterator> portalsItr = sector->GetNodes("portals");

        while(portalsItr->HasNext())
        {
            csRef<iDocumentNode> portals = portalsItr->Next();
            csRef<iDocumentNode> cleanedportals = cleanedSector->CreateNodeBefore(CS_NODE_ELEMENT);
            CloneNode(portals, cleanedportals);
        }


    }

    // Copy the start node
    csRef<iDocumentNodeIterator> startLocations = world->GetNodes("start");
    while (startLocations->HasNext())
    {
        csRef<iDocumentNode> start = startLocations->Next();
        csRef<iDocumentNode> cleanedStart = cleanedWorld->CreateNodeBefore(CS_NODE_ELEMENT);
        CloneNode(start, cleanedStart);
    }

    return cleanedWorld;
}

csRef<iDocumentNode> psRegion::Filter(csRef<iDocumentNode> world)
{
    // Filter the various features.
    if(!(gfxFeatures & useNormalMaps))
    {
        csRef<iDocumentNodeIterator> sectors = world->GetNodes("sector");
        while(sectors->HasNext())
        {
            csRef<iDocumentNode> sector = sectors->Next();
            csRef<iDocumentNode> rloop = sector->GetNode("renderloop");
            if(rloop.IsValid())
            {
                csString value = rloop->GetContentsValue();
                if(value.Compare("std_rloop_diffuse"))
                {
                    sector->RemoveNode(rloop);
                }
            }
        }
    }

    return world;
}

void psRegion::CloneNode (iDocumentNode* from, iDocumentNode* to)
{
    to->SetValue (from->GetValue ());
    csRef<iDocumentNodeIterator> it = from->GetNodes ();
    while (it->HasNext ())
    {
        csRef<iDocumentNode> child = it->Next ();
        csRef<iDocumentNode> child_clone = to->CreateNodeBefore (
            child->GetType (), 0);
        CloneNode (child, child_clone);
    }
    csRef<iDocumentAttributeIterator> atit = from->GetAttributes ();
    while (atit->HasNext ())
    {
        csRef<iDocumentAttribute> attr = atit->Next ();
        to->SetAttribute (attr->GetName (), attr->GetValue ());
    }
}

void psRegion::Unload()
{
    if (!loaded)
        return;
    loaded = false;

    // The engine needs to not be in the region when it is unloaded!

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (object_reg);

    iRegion* cur_region = engine->CreateRegion (regionname);

    // Array to point to RL objects.
    csWeakRefArray<iRenderLoop> rls;

    // Copy pointers to objects into a vector.
    csWeakRefArray<iObject> copy (1024, 256);
    csRef<iObjectIterator> iter = cur_region->QueryObject()->GetIterator ();
    while (iter->HasNext ())
    {
        csWeakRef<iObject> o = iter->Next ();
        csWeakRef<iSector> sec = scfQueryInterface<iSector>(o);
        csWeakRef<iLight> light = scfQueryInterface<iLight>(o);
        // If the object is a sector then remove it now, else copy the object.
        if(sec)
        {
            // If the sector uses a non-default renderloop, make note of it for later.
            csWeakRef<iRenderLoop> rl = sec->GetRenderLoop();
            if(rl)
                rls.PushSmart(rl);

            // Remove any mesh generators and meshes.
            sec->RemoveMeshGenerators();

            // Remove sector.
            engine->RemoveObject(o);
        }
        else if(!light)
            copy.Push (o);
    }

    size_t i;

    struct rcStruct { csString n; csWeakRef<iBase> weakb; };
    rcStruct* rc = new rcStruct[copy.GetSize()];

    // Remove all objects from the region.
    cur_region->QueryObject()->ObjRemoveAll();

    // Go through all objects and remove them if they're not being used.
    // Loop because references will be removed as objects are deleted,
    // so we need to check multiple times.
    bool doClean;
    do
    {
        doClean = false;
        i = 0;
        while (i < copy.GetSize ())
        {
            if(!copy[i]) 
            {
                printf("Removing null object!\n");
                copy.DeleteIndex (i);
                continue;
            }

            csWeakRef<iBase> b = scfQueryInterface<iBase>(copy[i]);
            if(b->GetRefCount() == 1)
            {
                if (engine->RemoveObject (b))
                {
                    copy.DeleteIndex (i);
                    doClean = true;
                    continue;
                }
            }
            i++;
        }
    }
    while (doClean);

    // Check for any objects that weren't deleted.
    for (i = 0 ; i < copy.GetSize () ; i++)
        if (rc[i].weakb != 0)
            printf ("Not Deleted %p '%s' ref=%d\n",
            (iBase*)rc[i].weakb, (const char*)rc[i].n,
            rc[i].weakb->GetRefCount ());
    fflush (stdout);
    delete[] rc;

    // Check all the renderloops that were being used by sectors to see if they can be removed.
    for(uint i=0; i<rls.GetSize(); i++)
    {
        if(rls.Get(i)->GetRefCount() == 2)
            engine->GetRenderLoopManager()->Unregister(rls.Get(i));
    }

    engine->GetRegions()->Remove(cur_region);
}

void psRegion::SetupWorldColliders(iEngine *engine,iRegion* cur_region)
{
    csRef<iCollideSystem> cdsys =
        csQueryRegistry<iCollideSystem> (object_reg);
    csRef<iObjectIterator> iter = cur_region->QueryObject()->GetIterator();

    iObject *curr;
    while ( iter->HasNext() )
    {
        curr = iter->Next();
        // regions hold many objects, but only meshes are collide-able
        csRef<iMeshWrapper> sp = scfQueryInterface<iMeshWrapper> (curr);
        if (sp && sp->GetMeshObject() )
        {
            csColliderHelper::InitializeCollisionWrapper(cdsys, sp);

            csRef<iThingState> thing = scfQueryInterface<iThingState> (sp->GetMeshObject());
            if (thing)
                thing->Prepare ();
        }
    }
}

void psRegion::SetupWorldCollidersCD(iEngine *engine, iRegion *cur_region, iRegion *cd_region)
{
    csRef<iCollideSystem> cdsys =
        csQueryRegistry<iCollideSystem> (object_reg);
    csRef<iObjectIterator> iter = cd_region->QueryObject()->GetIterator();

    iObject *curr;
    while (iter->HasNext())
    {
        curr = iter->Next();
        // regions hold many objects, but only meshes are collide-able
        csRef<iMeshWrapper> sp = scfQueryInterface<iMeshWrapper> (curr);
        if (sp && sp->GetMeshObject())
        {
            csRef<csColliderWrapper> cw = csColliderHelper::InitializeCollisionWrapper(cdsys, sp);
            if(cw)
            {
              csRef<iMeshWrapper> mw = cur_region->FindMeshObject(cw->GetObjectParent()->GetName());
              if(mw)
              {
                  mw->QueryObject()->ObjAdd(cw);
                  cw->SetObjectParent(mw->QueryObject());
              }
              else
              {
                  printf("Mesh Wrapper %s doesn't exist for collision!!\n", cw->GetObjectParent()->GetName());
              }
            }

            csRef<iThingState> thing = scfQueryInterface<iThingState> (sp->GetMeshObject());
            if (thing)
                thing->Prepare ();
        }
    }
}

iRegion * psRegion::GetRegion()
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (object_reg);
    return engine->GetRegions()->FindByName(regionname);
}

