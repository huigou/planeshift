/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008-10 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/enginetools.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <csutil/scfstringarray.h>
#include <iengine/camera.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivaria/engseq.h>
#include <ivideo/graph2d.h>
#include <ivideo/material.h>

#include "util/psconst.h"
#include "loader.h"

#ifdef CS_DEBUG
#define LOADER_DEBUG_MESSAGE(...) csPrintf(__VA_ARGS__)
#else
#define LOADER_DEBUG_MESSAGE(...)
#endif

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
SCF_IMPLEMENT_FACTORY(BgLoader)

BgLoader::BgLoader(iBase *p)
  : scfImplementationType (this, p), parsedShaders(false),
  loadRange(500), enabledGfxFeatures(0), loadingOffset(0),
  validPosition(false), resetHitbeam(true)
{
}

BgLoader::~BgLoader()
{
// hard disabled for now as it causes issues
/*#ifdef CS_DEBUG
    csRef<iStringArray> zone = csPtr<iStringArray>(new scfStringArray());
    LoadPriorityZones(zone);
    CleanDisconnectedSectors(NULL);
    size_t leakCount;

    leakCount = 0;
    for(csRefArray<Sector>::Iterator it = sectors.GetIterator(); it.HasNext();)
    {
        Sector* t = it.Next();
        if(t->object.IsValid())
        {
            LOADER_DEBUG_MESSAGE("detected leaking instance of sector %s\n", t->name.GetDataSafe());
            leakCount += t->object->GetRefCount();
            while(t->object.IsValid()) CleanSector(t);
        }
    }
    LOADER_DEBUG_MESSAGE("detected %u leaking sectors\n", leakCount);

    leakCount = 0;
    for(csHash<csRef<MeshObj>, csStringID>::GlobalIterator it = meshes.GetIterator(); it.HasNext();)
    {
        MeshObj* t = it.Next();
        if(t->object.IsValid())
        {
            LOADER_DEBUG_MESSAGE("detected leaking instance of meshobj %s\n", t->name.GetDataSafe());
            leakCount += t->object->GetRefCount();
            while(t->object.IsValid()) CleanMesh(t);
        }
    }
    LOADER_DEBUG_MESSAGE("detected %u leaking meshes\n", leakCount);

    leakCount = 0;
    for(csHash<csRef<MeshFact>, csStringID>::GlobalIterator it = meshfacts.GetIterator(); it.HasNext();)
    {
        MeshFact* t = it.Next();
        if(t->useCount != 0)
        {
            LOADER_DEBUG_MESSAGE("detected %u leaking instances of factory %s\n", t->useCount, t->name.GetDataSafe());
            leakCount += t->useCount;
            while(t->useCount) CleanMeshFact(t);
        }
    }
    LOADER_DEBUG_MESSAGE("detected %u leaking factories\n", leakCount);

    leakCount = 0;
    for(csHash<csRef<Material>, csStringID>::GlobalIterator it = materials.GetIterator(); it.HasNext();)
    {
        Material* t = it.Next();
        if(t->useCount != 0)
        {
            LOADER_DEBUG_MESSAGE("detected %u leaking instances of material %s\n", t->useCount, t->name.GetDataSafe());
            leakCount += t->useCount;
            while(t->useCount) CleanMaterial(t);
        }
    }
    LOADER_DEBUG_MESSAGE("detected %u leaking materials\n", leakCount);

    leakCount = 0;
    for(csHash<csRef<Texture>, csStringID>::GlobalIterator it = textures.GetIterator(); it.HasNext();)
    {
        Texture* t = it.Next();
        if(t->useCount != 0)
        {
            LOADER_DEBUG_MESSAGE("detected %u leaking instances of texture %s\n", t->useCount, t->name.GetDataSafe());
            leakCount += t->useCount;
            while(t->useCount) CleanTexture(t);
        }
    }
    LOADER_DEBUG_MESSAGE("detected %u leaking textures\n", leakCount);

    fflush(stdout);
#endif*/
}

bool BgLoader::Initialize(iObjectRegistry* object_reg)
{
    this->object_reg = object_reg;

    engine = csQueryRegistry<iEngine> (object_reg);
    engseq = csQueryRegistry<iEngineSequenceManager> (object_reg);
    g2d = csQueryRegistry<iGraphics2D> (object_reg);
    tloader = csQueryRegistry<iThreadedLoader> (object_reg);
    tman = csQueryRegistry<iThreadManager> (object_reg);
    vfs = csQueryRegistry<iVFS> (object_reg);
    svstrings = csQueryRegistryTagInterface<iShaderVarStringSet>(object_reg, "crystalspace.shader.variablenameset");
    strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
    cdsys = csQueryRegistry<iCollideSystem> (object_reg); 

    syntaxService = csQueryRegistryOrLoad<iSyntaxService>(object_reg, "crystalspace.syntax.loader.service.text");

    csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D>(object_reg);
    txtmgr = g3d->GetTextureManager();

    engine->SetClearZBuf(true);

    csRef<iConfigManager> config = csQueryRegistry<iConfigManager> (object_reg);
    
    // Check whether we're caching files for performance.    
    cache = config->GetBool("PlaneShift.Loading.Cache", true);

    // Check the level of shader use.
    csString shader("Highest");
    if(shader.CompareNoCase(config->GetStr("PlaneShift.Graphics.Shaders")))
    {
      enabledGfxFeatures |= useHighestShaders;
    }
    shader = "High";
    if(shader.CompareNoCase(config->GetStr("PlaneShift.Graphics.Shaders")))
    {
      enabledGfxFeatures |= useHighShaders;
    }
    shader = "Medium";
    if(shader.CompareNoCase(config->GetStr("PlaneShift.Graphics.Shaders")))
    {
      enabledGfxFeatures |= useMediumShaders;
    }
    shader = "Low";
    if(shader.CompareNoCase(config->GetStr("PlaneShift.Graphics.Shaders")))
    {
      enabledGfxFeatures |= useLowShaders;
    }
    shader = "Lowest";
    if(shader.CompareNoCase(config->GetStr("PlaneShift.Graphics.Shaders")))
    {
      enabledGfxFeatures |= useLowestShaders;
    }

    // Check if we're using real time shadows.
    if(config->GetBool("PlaneShift.Graphics.Shadows"))
    {
      enabledGfxFeatures |= useShadows;
    }

    // Check if we're using meshgen.
    if(config->GetBool("PlaneShift.Graphics.EnableGrass", true))
    {
      enabledGfxFeatures |= useMeshGen;
    }

    return true;
}

csPtr<iStringArray> BgLoader::GetShaderName(const char* usageType) const
{
    csRef<iStringArray> t = csPtr<iStringArray>(new scfStringArray());
    csStringID id = strings->Request(usageType);
    csArray<csString> all = shadersByUsageType.GetAll(id);

    for(size_t i=0; i<all.GetSize(); ++i)
    {
        t->Push(all[i]);
    }

    return csPtr<iStringArray>(t);
}

void BgLoader::ContinueLoading(bool waiting)
{  
    // Limit even while waiting - we want some frames.
    size_t count = 0;
    while(count < 10)
    {
        // True if at least one mesh finished load.
        bool finished = false;

        // Check if we need to reset i
        if(loadingOffset >= loadingMeshes.GetSize())
            loadingOffset = 0;

        // Check already loading meshes.
        size_t endOffset = loadingOffset + 20;
        if(endOffset > loadingMeshes.GetSize())
            endOffset = loadingMeshes.GetSize();

        for(; loadingOffset < endOffset; ++loadingOffset)
        {
            if(LoadMesh(loadingMeshes[loadingOffset]))
            {
                finished = true;
                finalisableMeshes.Push(loadingMeshes[loadingOffset]);
                loadingMeshes.DeleteIndex(loadingOffset);
            }
        }

        // Finalise loaded meshes (expensive, so limited per update).
        if(!finalisableMeshes.IsEmpty())
        {
            if(finished)
                engine->SyncEngineListsNow(tloader);

            FinishMeshLoad(finalisableMeshes[0]);
            finalisableMeshes.DeleteIndexFast(0);
        }

        // Load meshgens.
        for(size_t j=0; j<loadingMeshGen.GetSize(); ++j)
        {
            if(LoadMeshGen(loadingMeshGen[j]))
            {
                loadingMeshGen.DeleteIndex(j);
            }
        }

        ++count;
        if(!waiting || GetLoadingCount() == 0)
            break;
    }
}

void BgLoader::UpdatePosition(const csVector3& pos, const char* sectorName, bool force)
{
    validPosition = true;

    if(GetLoadingCount() != 0)
    {
        ContinueLoading(false);
    }

    if(!force)
    {
        // Check if we've moved.
        if(csVector3(lastPos - pos).Norm() < loadRange/10 && (!lastSector.IsValid() || lastSector->name == sectorName))
        {
            return;
        }
    }

    csRef<Sector> sector;
    // Hack to work around the weird sector stuff we do.
    if(csString("SectorWhereWeKeepEntitiesResidingInUnloadedMaps").Compare(sectorName))
    {
        sector = lastSector;
    }

    if(!sector.IsValid())
    {
        sector = sectorHash.Get(sStringSet.Request(sectorName), csRef<Sector>());
    }

    if(sector.IsValid())
    {
        // Calc bbox.
        csBox3 loadBox;
        loadBox.AddBoundingVertex(pos.x+loadRange, pos.y+loadRange, pos.z+loadRange);
        loadBox.AddBoundingVertexSmart(pos.x-loadRange, pos.y-loadRange, pos.z-loadRange);

        csBox3 unloadBox;
        unloadBox.AddBoundingVertex(pos.x+loadRange*1.5, pos.y+loadRange*1.5, pos.z+loadRange*1.5);
        unloadBox.AddBoundingVertexSmart(pos.x-loadRange*1.5, pos.y-loadRange*1.5, pos.z-loadRange*1.5);

        // Check.
        LoadSector(loadBox, unloadBox, sector, 0, false, true);

        if(force)
        {
            // Make sure we start the loading now.
            engine->SyncEngineListsNow(tloader);
            for(size_t i=0; i<loadingMeshes.GetSize(); i++)
            {
                if(LoadMesh(loadingMeshes[i]))
                {
                    FinishMeshLoad(loadingMeshes[i]);
                    loadingMeshes.DeleteIndex(i);
                }
            }

            for(size_t i=0; i<loadingMeshGen.GetSize(); ++i)
            {
                if(LoadMeshGen(loadingMeshGen[i]))
                {
                    loadingMeshGen.DeleteIndex(i);
                }
            }
        }

        if(lastSector != sector)
        {
            CleanDisconnectedSectors(sector);
        }
        lastPos = pos;
        lastSector = sector;
    }

    for(size_t i=0; i<sectors.GetSize(); i++)
    {
        sectors[i]->checked = false;
    }
}

void BgLoader::CleanDisconnectedSectors(Sector* sector)
{
    // Create a list of connectedSectors;
    csRefArray<Sector> connectedSectors;
    FindConnectedSectors(connectedSectors, sector);

    // Check for disconnected sectors.
    for(size_t i=0; i<sectors.GetSize(); i++)
    {
        if(sectors[i]->object.IsValid() && connectedSectors.Find(sectors[i]) == csArrayItemNotFound && !sectors[i]->priority)
        {
            CleanSector(sectors[i]);
        }
    }
}

void BgLoader::FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector)
{
    if(connectedSectors.Find(sector) != csArrayItemNotFound || sector == NULL)
    {
        return;
    }

    connectedSectors.Push(sector);

    for(size_t i=0; i<sector->activePortals.GetSize(); i++)
    {
        if(sector->activePortals[i]->mObject.IsValid()) // count not reachable sectors as disconnected
            FindConnectedSectors(connectedSectors, sector->activePortals[i]->targetSector);
    }
}

void BgLoader::CleanSector(Sector* sector)
{
    if(!sector->object.IsValid())
    {
        return;
    }

    for(size_t i=0; i<sector->lights.GetSize(); i++)
    {
        // finish partially loaded light
        if(!sector->lights[i]->loaded && sector->lights[i]->object.IsValid())
        {
            if(LoadLight(sector->lights[i], sector, true))
            {
                ++(sector->objectCount);
            }
        }

        if(sector->lights[i]->loaded)
        {
            sector->object->GetLights()->Remove(sector->lights[i]->object);
            CleanLight(sector->lights[i]);
            --(sector->objectCount);
        }
    }

    // Remove sequences.
    for(size_t i=0; i<sector->sequences.GetSize(); ++i)
    {
        // finish partially loaded sequence
        if(!sector->sequences[i]->loaded && sector->sequences[i]->status.IsValid())
        {
            LoadSequence(sector->sequences[i], true);
        }

        if(sector->sequences[i]->loaded)
        {
            CleanSequence(sector->sequences[i]);
        }
    }

    for(size_t i=0; i<sector->meshgen.GetSize(); i++)
    {
        if(sector->meshgen[i]->status.IsValid())
        {
            CleanMeshGen(sector->meshgen[i]);
            --(sector->objectCount);
        }
    }

    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(sector->meshes[i]->object.IsValid())
        {
            CleanMesh(sector->meshes[i]);
            --(sector->objectCount);
        }
    }

    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(sector->portals[i]->mObject.IsValid())
        {
            CleanPortal(sector->portals[i]);
            sector->activePortals.Delete(sector->portals[i]);
            --(sector->objectCount);
        }
    }

    for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
    {
        if(sector->alwaysLoaded[i]->object.IsValid())
        {
            CleanMesh(sector->alwaysLoaded[i]);
            --(sector->objectCount);
        }
    }

    if(sector->objectCount > 0)
    {
        // could not unload all items, some may still be loading
        // note that this shall never occur with blocked loading,
        // because in that case this would mean there's a memleak
        return;
    }

    CS_ASSERT_MSG("Error cleaning sector. Sector is invalid!", sector->object.IsValid());

    // Remove the sector from the engine.
    sector->checked = false;
    csWeakRef<iSector> w(sector->object);
    engine->RemoveObject(w);
    sector->object.Invalidate();
    if(w.IsValid())
    {
        LOADER_DEBUG_MESSAGE("detected leaking sector %u %s\n", w->GetRefCount(), sector->name.GetDataSafe());
    }
}

void BgLoader::CleanMesh(MeshObj* mesh)
{
    csWeakRef<iMeshWrapper> m(mesh->object);
    if(m.IsValid())
    {
        engine->RemoveObject(m);
        mesh->object.Invalidate();
    }
    else
    {
        LOADER_DEBUG_MESSAGE("double free of mesh object %s\n", mesh->name.GetDataSafe());
        return;
    }
    CS_ASSERT(!mesh->loading);

    for(size_t i=0; i<mesh->sequences.GetSize(); ++i)
    {
        CleanSequence(mesh->sequences[i]);
    }

    for(size_t i=0; i<mesh->meshfacts.GetSize(); ++i)
    {
        CleanMeshFact(mesh->meshfacts[i]);
        mesh->mftchecked[i] = false;
    }

    for(size_t i=0; i<mesh->materials.GetSize(); ++i)
    {
        CleanMaterial(mesh->materials[i]);
        mesh->matchecked[i] = false;
    }

    for(size_t i=0; i<mesh->textures.GetSize(); ++i)
    {
        CleanTexture(mesh->textures[i]);
        mesh->texchecked[i] = false;
    }

    if(m.IsValid())
    {
        LOADER_DEBUG_MESSAGE("detected leaking mesh %u %s\n", m->GetRefCount(), mesh->name.GetDataSafe());
    }
}

void BgLoader::CleanMeshGen(MeshGen* meshgen)
{
  if(!meshgen->status.IsValid())
  {
      LOADER_DEBUG_MESSAGE("double-free of meshgen %s\n", meshgen->name.GetDataSafe());
      return;
  }

  meshgen->sector->object->RemoveMeshGenerator(meshgen->name);
  meshgen->status.Invalidate();

  for(size_t i=0; i<meshgen->meshfacts.GetSize(); ++i)
  {
      CleanMeshFact(meshgen->meshfacts[i]);
      meshgen->mftchecked[i] = false;
  }

  for(size_t i=0; i<meshgen->materials.GetSize(); ++i)
  {
      CleanMaterial(meshgen->materials[i]);
      meshgen->matchecked[i] = false;
  }

  CleanMesh(meshgen->object);
}

void BgLoader::CleanPortal(Portal* portal)
{
    if(!portal->mObject.IsValid())
    {
        LOADER_DEBUG_MESSAGE("double-free of portal %s\n", portal->name.GetDataSafe());
    }

    csWeakRef<iMeshWrapper> w(portal->mObject);
    engine->RemoveObject(w);
    portal->pObject = NULL;
    portal->mObject.Invalidate();
    if(w.IsValid())
    {
        LOADER_DEBUG_MESSAGE("detected leaking portal %u %s\n", w->GetRefCount(), portal->name.GetDataSafe());
    }
}

void BgLoader::CleanMeshFact(MeshFact* meshfact)
{
  if(!meshfact->useCount)
  {
      LOADER_DEBUG_MESSAGE("double-free of factory %s\n", meshfact->name.GetDataSafe());
      return;
  }

  if(--meshfact->useCount == 0)
  {
      csWeakRef<iMeshFactoryWrapper> mfc(meshfact->object);
      engine->RemoveObject(mfc);
      meshfact->object.Invalidate();
      if(mfc.IsValid())
      {
          LOADER_DEBUG_MESSAGE("detected leaking factory: %u %s\n", mfc->GetRefCount(), meshfact->name.GetDataSafe());
      }

      for(size_t i=0; i<meshfact->materials.GetSize(); ++i)
      {
          CleanMaterial(meshfact->materials[i]);
          meshfact->checked[i] = false;
      }
  }
}

void BgLoader::CleanMaterial(Material* material)
{
  if(!material->useCount)
  {
      LOADER_DEBUG_MESSAGE("double-free of material %s\n", material->name.GetDataSafe());
      return;
  }

  if(--material->useCount == 0)
  {
      engine->RemoveObject(material->mat);
      csWeakRef<iMaterialWrapper> m(material->mat);
      material->mat.Invalidate();
      if(m.IsValid())
      {
          LOADER_DEBUG_MESSAGE("detected leaking material: %u %s\n", m->GetRefCount(), material->name.GetDataSafe());
      }

      for(size_t i=0; i<material->textures.GetSize(); ++i)
      {
          CleanTexture(material->textures[i]);
          material->checked[i] = false;
      }
  }
}

void BgLoader::CleanTexture(Texture* texture)
{
    if(!texture->useCount)
    {
        LOADER_DEBUG_MESSAGE("double-free of texture %s\n", texture->name.GetDataSafe());
        return;
    }

    if(--texture->useCount == 0)
    {
        csRef<iTextureWrapper> t = scfQueryInterface<iTextureWrapper>(texture->status->GetResultRefPtr());
        csWeakRef<iTextureWrapper> tc(t);
        engine->RemoveObject(t);
        t.Invalidate();
        texture->status.Invalidate();
        if(tc.IsValid())
        {
            LOADER_DEBUG_MESSAGE("detected leaking texture: %u %s\n", tc->GetRefCount(), texture->name.GetDataSafe());
        }
    }
}

void BgLoader::CleanLight(Light* light)
{
    if(!light->object.IsValid())
    {
        LOADER_DEBUG_MESSAGE("double-free of light %s\n", light->name.GetDataSafe());
        return;
    }

    for(size_t i=0; i<light->sequences.GetSize(); ++i)
    {
        CleanSequence(light->sequences[i]);
    }

    csWeakRef<iLight> w(light->object);
    engine->RemoveObject(w);
    light->object.Invalidate();
    if(w.IsValid())
    {
        LOADER_DEBUG_MESSAGE("detected leaking light %u %s\n", w->GetRefCount(), light->name.GetDataSafe());
    }

    light->loaded = false;
}

void BgLoader::CleanSequence(Sequence* sequence)
{
    if(!sequence->loaded)
    {
        LOADER_DEBUG_MESSAGE("double-free of sequence %s\n", sequence->name.GetDataSafe());
        return;
    }

    for(size_t i=0; i<sequence->triggers.GetSize(); ++i)
    {
        Trigger* trigger = sequence->triggers[i];
        if(!trigger->loaded)
        {
            continue;
        }
        else if(trigger->status.IsValid())
        {
            csRef<iSequenceTrigger> st;
            // @@@ would be nice to know why the result can be null
            if(trigger->status->GetResultRefPtr().IsValid())
            {
                st = scfQueryInterface<iSequenceTrigger>(trigger->status->GetResultRefPtr());
                //engseq->RemoveTrigger(st);
                engine->RemoveObject(st);
            }
            trigger->status.Invalidate();
            if(st.IsValid() && st->GetRefCount() > 1)
            {
                LOADER_DEBUG_MESSAGE("detected leaking trigger %u %s\n", st->GetRefCount() - 1, trigger->name.GetDataSafe());
            }
        }
        trigger->loaded = false;
    }

    csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(sequence->status->GetResultRefPtr());
    //engseq->RemoveSequence(sw);
    engine->RemoveObject(sw);
    sequence->status.Invalidate();
    if(sw->GetRefCount() > 1)
    {
        LOADER_DEBUG_MESSAGE("detected leaking sequence %u %s\n", sw->GetRefCount() - 1, sequence->name.GetDataSafe());
    }

    sequence->loaded = false;
}

void BgLoader::LoadSector(const csBox3& loadBox, const csBox3& unloadBox,
                        Sector* sector, uint depth, bool force, bool loadMeshes, bool portalsOnly)
{
    sector->isLoading = true;
    force |= sector->priority;

    if(!sector->object.IsValid())
    {
        {
            csString msg;
            msg.AppendFmt("Attempting to load uninit sector %s!\n", sector->name.GetData());
            CS_ASSERT_MSG(msg.GetData(), sector->init);
            if(!sector->init) return;
        }
        sector->object = engine->CreateSector(sector->name);
        sector->object->SetDynamicAmbientLight(sector->ambient);
        sector->object->SetVisibilityCullerPlugin(sector->culler);
        sector->object->QueryObject()->SetObjectParent(sector->parent);
    }

    if(!force && depth < maxPortalDepth)
    {
        // Check other sectors linked to by active portals.
        for(size_t i=0; i<sector->activePortals.GetSize(); i++)
        {
            if(!sector->activePortals[i]->targetSector->isLoading && !sector->activePortals[i]->targetSector->checked)
            {
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->activePortals[i]->warp)
                {
                    wwLoadBox *= sector->activePortals[i]->transform;
                    wwUnloadBox *= sector->activePortals[i]->transform;
                }

                LoadSector(wwLoadBox, wwUnloadBox, sector->activePortals[i]->targetSector, depth+1, false, loadMeshes);
            }
        }
    }

    if(loadMeshes && !portalsOnly)
    {
        // Load all meshes which should always be loaded in this sector.
        for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
        {
            if(!sector->alwaysLoaded[i]->loading &&
               !sector->alwaysLoaded[i]->object.IsValid())
            {
                sector->alwaysLoaded[i]->loading = true;
                loadingMeshes.Push(sector->alwaysLoaded[i]);
                ++(sector->objectCount);
            }
        }

        // Check all meshes in this sector.
        for(size_t i=0; i<sector->meshes.GetSize(); i++)
        {
            if(!sector->meshes[i]->loading)
            {
                if(sector->meshes[i]->InRange(loadBox, force))
                {
                    sector->meshes[i]->loading = true;
                    loadingMeshes.Push(sector->meshes[i]);
                    ++(sector->objectCount);
                }
                else if(!force && sector->meshes[i]->OutOfRange(unloadBox))
                {
                    CleanMesh(sector->meshes[i]);
                    --(sector->objectCount);
                }
            }
        }

        // Check all meshgen in this sector.
        for(size_t i=0; i<sector->meshgen.GetSize(); i++)
        {
            if(!sector->meshgen[i]->loading)
            {
                if(sector->meshgen[i]->InRange(loadBox, force))
                {
                    sector->meshgen[i]->loading = true;
                    loadingMeshGen.Push(sector->meshgen[i]);
                    ++(sector->objectCount);
                }
                else if(!force && sector->meshgen[i]->OutOfRange(unloadBox))
                {
                    CleanMeshGen(sector->meshgen[i]);
                    --(sector->objectCount);
                }
            }
        }
    }

    // Check all portals in this sector... and recurse into the sectors they lead to.
    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(sector->portals[i]->InRange(loadBox, force))
        {
            bool recurse = true;
            if(!force && depth >= maxPortalDepth)
            {
                // If we've reached the recursion limit then check if the
                // target sector is valid. If so then create a portal to it.
                if(sector->portals[i]->targetSector->object.IsValid())
                {
                    recurse = false;
                }
                else // Else check the next portal.
                {
                    continue;
                }
            }

            if(force)
            {
                if(!sector->portals[i]->targetSector->object.IsValid())
                {
                    {
                        csString msg;
                        msg.AppendFmt("Attempting to load uninit sector %s!\n", sector->portals[i]->targetSector->name.GetData());
                        CS_ASSERT_MSG(msg.GetData(), sector->portals[i]->targetSector->init);
                        if(!sector->portals[i]->targetSector->init) return;
                    }
                    sector->portals[i]->targetSector->object = engine->CreateSector(sector->portals[i]->targetSector->name);
                    sector->portals[i]->targetSector->object->SetDynamicAmbientLight(sector->portals[i]->targetSector->ambient);
                    sector->portals[i]->targetSector->object->SetVisibilityCullerPlugin(sector->portals[i]->targetSector->culler);
                    sector->portals[i]->targetSector->object->QueryObject()->SetObjectParent(sector->portals[i]->targetSector->parent);
                }
            }
            else if(!sector->portals[i]->targetSector->isLoading && !sector->portals[i]->targetSector->checked && recurse)
            {
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->portals[i]->warp)
                {
                    wwLoadBox *= sector->portals[i]->transform;
                    wwUnloadBox *= sector->portals[i]->transform;
                }

                LoadSector(wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1, false, loadMeshes);
            }

            if(!sector->portals[i]->mObject.IsValid() && LoadPortal(sector->portals[i], sector))
            {
                sector->activePortals.Push(sector->portals[i]);
                ++(sector->objectCount);
            }
        }
        else if(!force && sector->portals[i]->OutOfRange(unloadBox))
        {
            if(!sector->portals[i]->targetSector->isLoading)
            {
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->portals[i]->warp)
                {
                    wwLoadBox *= sector->portals[i]->transform;
                    wwUnloadBox *= sector->portals[i]->transform;
                }

                LoadSector(wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1, false, loadMeshes);
            }

            CleanPortal(sector->portals[i]);
            sector->activePortals.Delete(sector->portals[i]);
            --(sector->objectCount);
        }
    }

    if(loadMeshes && !portalsOnly)
    {
        // Check all sector lights.
        for(size_t i=0; i<sector->lights.GetSize(); i++)
        {
            if(sector->lights[i]->InRange(loadBox, force))
            {
                if(LoadLight(sector->lights[i], sector))
                {
                    ++(sector->objectCount);
                }
            }
            else if(!force && sector->lights[i]->OutOfRange(unloadBox))
            {
                sector->object->GetLights()->Remove(sector->lights[i]->object);
                CleanLight(sector->lights[i]);
                --(sector->objectCount);
            }
        }

        if(loadMeshes && !portalsOnly)
        {
            // Load all sector sequences.
            for(size_t i=0; i<sector->sequences.GetSize(); i++)
            {
                if(!sector->sequences[i]->loaded)
                {
                    LoadSequence(sector->sequences[i]);
                }
            }
        }

        // Check whether this sector is empty and should be unloaded.
        if(sector->objectCount == sector->alwaysLoaded.GetSize() && sector->object.IsValid())
        {
            CleanSector(sector);
        }
    }

    sector->checked = true;
    sector->isLoading = false;
}

void BgLoader::FinishMeshLoad(MeshObj* mesh)
{
    if(!mesh->status.IsValid())
    {
        if(!mesh->object.IsValid())
        {
            printf("Mesh '%s' failed to load. (no data found)\n", mesh->name.GetData());
        }
        return;
    }

    if(!mesh->status->WasSuccessful())
    {
        printf("Mesh '%s' failed to load.\n", mesh->name.GetData());
        return;
    }

    mesh->object = scfQueryInterface<iMeshWrapper>(mesh->status->GetResultRefPtr());
    mesh->status.Invalidate();

    // Mark the mesh as being realtime lit depending on graphics setting.
    if(enabledGfxFeatures & (useMediumShaders | useLowShaders | useLowestShaders))
    {
        mesh->object->GetFlags().Set(CS_ENTITY_NOLIGHTING);
    }

    // Set world position.
    mesh->object->GetMovable()->SetSector(mesh->sector->object);
    mesh->object->GetMovable()->UpdateMove();

    // Init collision data.
    csColliderHelper::InitializeCollisionWrapper(cdsys, mesh->object);

    // Get the correct path for loading heightmap data.
    vfs->PushDir(mesh->path);
    engine->PrecacheMesh(mesh->object);
    vfs->PopDir();

    mesh->loading = false;
}

bool BgLoader::LoadMeshGen(MeshGen* meshgen)
{
    bool ready = true;
    for(size_t i=0; i<meshgen->meshfacts.GetSize(); i++)
    {
        if(!meshgen->mftchecked[i])
        {
            meshgen->mftchecked[i] = LoadMeshFact(meshgen->meshfacts[i]);
            ready &= meshgen->mftchecked[i];
        }
    }

    if(!ready)
      return false;

    for(size_t i=0; i<meshgen->materials.GetSize(); i++)
    {
        if(!meshgen->matchecked[i])
        {
            meshgen->matchecked[i] = LoadMaterial(meshgen->materials[i]);
            ready &= meshgen->matchecked[i];
        }
    }

    if(!ready || !LoadMesh(meshgen->object))
      return false;

    FinishMeshLoad(meshgen->object);

    if(ready && !meshgen->status.IsValid())
    {
        meshgen->status = tloader->LoadNode(vfs->GetCwd(), meshgen->data, 0, meshgen->sector->object);
        return false;
    }

    if(meshgen->status.IsValid() && meshgen->status->IsFinished())
    {
        meshgen->loading = false;
        return true;
    }

    return false;
}

bool BgLoader::LoadMesh(MeshObj* mesh)
{
    if(mesh->object.IsValid())
    {
      return true;
    }

    bool ready = true;
    for(size_t i=0; i<mesh->meshfacts.GetSize(); i++)
    {
        if(!mesh->mftchecked[i])
        {
            mesh->mftchecked[i] = LoadMeshFact(mesh->meshfacts[i]);
            ready &= mesh->mftchecked[i];
        }
    }

    if(!ready)
      return false;

    for(size_t i=0; i<mesh->materials.GetSize(); i++)
    {
        if(!mesh->matchecked[i])
        {
            mesh->matchecked[i] = LoadMaterial(mesh->materials[i]);
            ready &= mesh->matchecked[i];
        }
    }

    if(!ready)
      return false;

    for(size_t i=0; i<mesh->textures.GetSize(); i++)
    {
        if(!mesh->texchecked[i])
        {
            mesh->texchecked[i] = LoadTexture(mesh->textures[i]);
            ready &= mesh->texchecked[i];
        }
    }

    if(ready && !mesh->status)
    {
        mesh->status = tloader->LoadNode(mesh->path, mesh->data);
        ready = false;
    }

    if(ready)
    {
        ready = mesh->status->IsFinished();
    }

    // Load sequences.
    if(ready)
    {
        for(size_t i=0; i<mesh->sequences.GetSize(); ++i)
        {
            if(!mesh->sequences[i]->loaded)
            {
                ready &= LoadSequence(mesh->sequences[i]);
            }
        }
    }

    return ready;
}

bool BgLoader::LoadPortal(Portal* portal, Sector* sector)
{
    if(portal->mObject.IsValid())
    {
        return true;
    }

    iSector* target = portal->targetSector->object;

    if(portal->autoresolve)
    {
        target = 0;
    }

    portal->mObject = engine->CreatePortal(portal->name, sector->object, csVector3(0),
                 target, portal->poly.GetVertices(), (int)portal->poly.GetVertexCount(),
                 portal->pObject);

    if(portal->warp)
    {
        portal->pObject->SetWarp(portal->matrix, portal->wv, portal->ww);
    }

    if(portal->pfloat)
    {
        portal->pObject->GetFlags().SetBool(CS_PORTAL_FLOAT, true);
    }

    if(portal->clip)
    {
        portal->pObject->GetFlags().SetBool(CS_PORTAL_CLIPDEST, true);
    }

    if(portal->zfill)
    {
        portal->pObject->GetFlags().SetBool(CS_PORTAL_ZFILL, true);
    }

    if(!target)
    {
        csRef<Portal::MissingSectorCallback> cb;
        cb.AttachNew(new Portal::MissingSectorCallback(portal->targetSector, portal->autoresolve));
        portal->pObject->SetMissingSectorCallback(cb);
    }

    return true;
}

bool BgLoader::LoadMeshFact(MeshFact* meshfact, bool wait)
{
    if(meshfact->useCount != 0)
    {
        ++meshfact->useCount;
        return true;
    }

    bool ready = true;
    for(size_t i=0; i<meshfact->materials.GetSize(); i++)
    {
        if(!meshfact->checked[i])
        {
            meshfact->checked[i] = LoadMaterial(meshfact->materials[i], wait);
            ready &= meshfact->checked[i];
        }
    }

    if(ready && !meshfact->status.IsValid())
    {
        if(meshfact->data)
        {
            if(wait)
            {
                meshfact->status = tloader->LoadNodeWait(meshfact->path, meshfact->data);
            }
            else
            {
                meshfact->status = tloader->LoadNode(meshfact->path, meshfact->data);
                return false;
            }
        }
        else
        {
            if(wait)
            {
                meshfact->status = tloader->LoadMeshObjectFactoryWait(meshfact->path, meshfact->filename);
            }
            else
            {
                meshfact->status = tloader->LoadMeshObjectFactory(meshfact->path, meshfact->filename);
                return false;
            }
        }
    }
    else if(ready && wait)
    {
        meshfact->status->Wait();
    }

    if(meshfact->status && meshfact->status->IsFinished())
    {
        ++meshfact->useCount;
        meshfact->object = scfQueryInterfaceSafe<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());
        meshfact->status.Invalidate();
        return true;
    }

    return false;
}

bool BgLoader::LoadMaterial(Material* material, bool wait)
{
    if(material->useCount != 0)
    {
        ++material->useCount;
        return true;
    }

    bool ready = true;
    for(size_t i=0; i<material->textures.GetSize(); i++)
    {
        if(!material->checked[i])
        {
            material->checked[i] = LoadTexture(material->textures[i], wait);
            ready &= material->checked[i];
        }
    }

    if(ready)
    {
        csRef<iMaterial> mat (engine->CreateBaseMaterial(0));
        material->mat = engine->GetMaterialList()->NewMaterial(mat, material->name);

        for(size_t i=0; i<material->shaders.GetSize(); i++)
        {
            csRef<iShaderManager> shaderMgr = csQueryRegistry<iShaderManager> (object_reg);
            iShader* shader = shaderMgr->GetShader(material->shaders[i].name);
            csStringID type = strings->Request(material->shaders[i].type);
            mat->SetShader(type, shader);
        }

        for(size_t i=0; i<material->shadervars.GetSize(); i++)
        {
            csShaderVariable* var = mat->GetVariableAdd(svstrings->Request(material->shadervars[i].name));
            var->SetType(material->shadervars[i].type);

            if(material->shadervars[i].type == csShaderVariable::TEXTURE)
            {
                for(size_t j=0; j<material->textures.GetSize(); j++)
                {
                    if(material->textures[j]->name.Compare(material->shadervars[i].value))
                    {
                        csRef<iTextureWrapper> tex = scfQueryInterface<iTextureWrapper>(material->textures[j]->status->GetResultRefPtr());
                        var->SetValue(tex);
                        break;
                    }
                }
            }
            else if(material->shadervars[i].type == csShaderVariable::VECTOR2)
            {
                var->SetValue(material->shadervars[i].vec2);
            }
            else if(material->shadervars[i].type == csShaderVariable::VECTOR3)
            {
                var->SetValue(material->shadervars[i].vec3);
            }
        }

        ++material->useCount;
        return true;
    }

    return false;
}

bool BgLoader::LoadTexture(Texture* texture, bool wait)
{
    if(texture->useCount != 0)
    {
        ++texture->useCount;
        return true;
    }

    if(!texture->status.IsValid())
    {
        if(wait)
        {
            texture->status = tloader->LoadNodeWait(texture->path, texture->data);
        }
        else
        {
            texture->status = tloader->LoadNode(texture->path, texture->data);
            return false;
        }
    }
    else if(wait)
    {
        texture->status->Wait();
    }

    if(texture->status->IsFinished())
    {
        ++texture->useCount;
        return true;
    }

    return false;
}

bool BgLoader::LoadLight(Light* light, Sector* sector, bool wait)
{
    if(light->loaded)
    {
        return true;
    }

    if(!light->object.IsValid())
    {
        light->object = engine->CreateLight(light->name, light->pos, light->radius, light->colour, light->dynamic);
        light->object->SetAttenuationMode(light->attenuation);
        light->object->SetType(light->type);
	light->object->GetMovable()->SetSector(sector->object);
	sector->object->GetLights()->Add(light->object);
    }

    // Load all light sequences.
    bool ready = true;
    for(size_t i=0; i<light->sequences.GetSize(); ++i)
    {
        if(!light->sequences[i]->loaded)
            ready &= LoadSequence(light->sequences[i], wait);
    }

    light->loaded = ready;
    return ready;
}

bool BgLoader::LoadSequence(Sequence* sequence, bool wait)
{
    if(sequence->loaded)
    {
        return true;
    }

    if(!sequence->status.IsValid())
    {
        if(wait)
        {
            sequence->status = tloader->LoadNodeWait(vfs->GetCwd(), sequence->data);
        }
        else
        {
            sequence->status = tloader->LoadNode(vfs->GetCwd(), sequence->data);
        }
    }

    if(!sequence->status->IsFinished())
    {
        if(wait)
        {
            sequence->status->Wait();
        }
        else
        {
            return false;
        }
    }

    if(!sequence->status->WasSuccessful())
    {
        LOADER_DEBUG_MESSAGE("sequence %s failed to load\n", sequence->name.GetDataSafe());
        sequence->loaded = true;
        sequence->status.Invalidate();
        return true;
    }

    bool ready = true;
    for(size_t i=0; i<sequence->triggers.GetSize(); ++i)
    {
        Trigger* trigger = sequence->triggers[i];
        if(trigger->loaded)
        {
            continue;
        }

        if(!trigger->status.IsValid())
        {
            if(wait)
            {
                trigger->status = tloader->LoadNodeWait(vfs->GetCwd(), trigger->data);
            }
            else
            {
                trigger->status = tloader->LoadNode(vfs->GetCwd(), trigger->data);
            }
        }
        else if(!trigger->status->IsFinished() && wait)
        {
            trigger->status->Wait();
        }
        else
        {
            ready = false;
            continue;
        }

        if(!trigger->status->WasSuccessful())
        {
            LOADER_DEBUG_MESSAGE("trigger %s failed to load\n", sequence->name.GetDataSafe());
            trigger->status.Invalidate();
        }

        trigger->loaded = true;
    }

    sequence->loaded = ready;

    return ready;
}

csPtr<iMeshFactoryWrapper> BgLoader::LoadFactory(const char* name, bool* failed, bool wait)
{
    csRef<MeshFact> meshfact = meshfacts.Get(mfStringSet.Request(name), csRef<MeshFact>());
    {
        if(!failed)
        {
            // Validation.
            csString msg;
            msg.Format("Invalid factory reference '%s'", name);
            CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
        }
        else if(!meshfact.IsValid())
        {
            *failed = true;
            return csPtr<iMeshFactoryWrapper>(0);
        }
    }

    if(LoadMeshFact(meshfact, wait))
    {
        csRef<iMeshFactoryWrapper> mfw(meshfact->object);
        if(!failed)
        {
            // Check success.
            csString msg;
            msg.Format("Failed to load factory '%s' path: %s filename: %s", name, (const char*) meshfact->path, (const char*) meshfact->filename);
            CS_ASSERT_MSG(msg.GetData(), mfw.IsValid());
        }
        else if(!mfw.IsValid())
        {
            *failed = true;
            return csPtr<iMeshFactoryWrapper>(0);
        }

        return csPtr<iMeshFactoryWrapper>(mfw);
    }

    return csPtr<iMeshFactoryWrapper>(0);
}

bool BgLoader::FreeFactory(const char* name)
{
    csRef<MeshFact> meshfact = meshfacts.Get(mfStringSet.Request(name), csRef<MeshFact>());
    if(!meshfact.IsValid())
    {
        return false;
    }

    CleanMeshFact(meshfact);
    return true;
}

void BgLoader::CloneFactory(const char* name, const char* newName, bool* failed)
{
    // Find meshfact to clone.
    csRef<MeshFact> meshfact = meshfacts.Get(mfStringSet.Request(name), csRef<MeshFact>());
    {
        if(!failed)
        {
            // Validation.
            csString msg;
            msg.Format("Invalid factory reference '%s' passed for cloning.", name);
            CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
        }
        else if(!meshfact.IsValid())
        {
            *failed = true;
            return;
        }
    }

    // check whether newName already exists
    csRef<MeshFact> newMeshFact = meshfacts.Get(mfStringSet.Request(newName), csRef<MeshFact>());
    if(newMeshFact.IsValid())
    {
        if(!failed)
        {
            // validation
            csString msg;
            msg.Format("Cloning factory '%s' to '%s' that already exists and doesn't match.", name, newName);
            CS_ASSERT_MSG(msg.GetData(), *meshfact == *newMeshFact);
        }
        else if(!(*newMeshFact == *meshfact))
        {
            *failed = true;
            return;
        }
    }

    // Create a clone.
    if(!newMeshFact.IsValid())
    {
        newMeshFact = meshfact->Clone(newName);
        meshfacts.Put(mfStringSet.Request(newName), newMeshFact);
    }
}

bool BgLoader::FreeMaterial(const char* name)
{
    csRef<Material> material = materials.Get(mStringSet.Request(name), csRef<Material>());
    if(!material.IsValid())
        return false;

    CleanMaterial(material);
    return true;
}

iMaterialWrapper* BgLoader::LoadMaterial(const char* name, bool* failed, bool wait)
{
    csRef<Material> material = materials.Get(mStringSet.Request(name), csRef<Material>());
    {
        if(!failed)
        {
          // Validation.
          csString msg;
          msg.Format("Invalid material reference '%s'", name);
          CS_ASSERT_MSG(msg.GetData(), material.IsValid());
        }
        else if(!material.IsValid())
        {
          *failed = true;
          return 0;
        }
    }

    if(LoadMaterial(material, wait))
    {
        return material->mat;
    }

    return 0;
}

bool BgLoader::InWaterArea(const char* sector, csVector3* pos, csColor4** colour)
{
    // Hack to work around the weird sector stuff we do.
    if(!strcmp("SectorWhereWeKeepEntitiesResidingInUnloadedMaps", sector))
        return false;

    csRef<Sector> s = sectorHash.Get(sStringSet.Request(sector), csRef<Sector>());
    CS_ASSERT_MSG("Invalid sector passed to InWaterArea().", s.IsValid());

    for(size_t i=0; i<s->waterareas.GetSize(); ++i)
    {
        if(s->waterareas[i]->bbox.In(*pos))
        {
            *colour = &s->waterareas[i]->colour;
            return true;
        }
    }

    return false;
}

bool BgLoader::LoadZones(iStringArray* regions, bool loadMeshes, bool priority)
{
    // Firstly, get a list of all zones that should be loaded.
    csRefArray<Zone> newLoadedZones;
    for(size_t i=0; i<regions->GetSize(); ++i)
    {
        csRef<Zone> zone = zones.Get(zStringSet.Request(regions->Get(i)), csRef<Zone>());
        if(zone.IsValid())
        {
            zone->priority = priority;
            newLoadedZones.Push(zone);
        }
        else
        {
            return false;
        }
    }

    // Next clean all zones which shouldn't be loaded.
    for(size_t i=0; i<loadedZones.GetSize(); ++i)
    {
        bool found = false;
        for(size_t j=0; j<newLoadedZones.GetSize(); ++j)
        {
            if(loadedZones[i] == newLoadedZones[j])
            {
                found = true;
                loadedZones[i]->priority |= priority;
                break;
            }
        }

        if(!found && loadedZones[i]->priority == priority) // only unload zones of same priority
        {
            for(size_t j=0; j<loadedZones[i]->sectors.GetSize(); ++j)
            {
                CleanSector(loadedZones[i]->sectors[j]);
            }

            loadedZones.DeleteIndex(i);
            --i;
        }
    }

    // Now load all zones which should be loaded.
    for(size_t i=0; i<newLoadedZones.GetSize(); ++i)
    {
        bool found = false;
        for(size_t j=0; j<loadedZones.GetSize(); ++j)
        {
            if(newLoadedZones[i] == loadedZones[j])
            {
                found = true;
                loadedZones[j]->priority |= priority; // upgrade priority
                break;
            }
        }

        if(!found)
        {
            loadedZones.Push(newLoadedZones[i]);
            for(size_t j=0; j<newLoadedZones[i]->sectors.GetSize(); ++j)
            {
                LoadSector(csBox3(), csBox3(), newLoadedZones[i]->sectors[j], (uint)-1, true, loadMeshes);
                newLoadedZones[i]->sectors[j]->priority |= priority; // upgrade priority
            }
        }
        else
        {
            for(size_t j=0; j<newLoadedZones[i]->sectors.GetSize(); ++j)
            {
                LoadSector(csBox3(), csBox3(), newLoadedZones[i]->sectors[j], (uint)-1, true, loadMeshes, true);
                newLoadedZones[i]->sectors[j]->priority |= priority; // upgrade priority
            }
        }
    }

    // Finally, clean up all sectors which were created but not checked for loading.
    for(size_t i=0; i<loadedZones.GetSize(); ++i)
    {
        for(size_t j=0; j<loadedZones[i]->sectors.GetSize(); ++j)
        {
            for(size_t k=0; k<loadedZones[i]->sectors[j]->activePortals.GetSize(); ++k)
            {
                Portal* portal = loadedZones[i]->sectors[j]->activePortals[k];
                if(!portal->targetSector->checked)
                {
                    CleanSector(portal->targetSector);
                    if(portal->mObject.IsValid())
                    {
                        CleanPortal(portal);
                        loadedZones[i]->sectors[j]->activePortals.Delete(portal);
                        --(loadedZones[i]->sectors[j]->objectCount);
                    }
                }
            }
        }
    }

    return true;
}

bool BgLoader::LoadPriorityZones(iStringArray* regions)
{
    return LoadZones(regions, true, true);
}
}
CS_PLUGIN_NAMESPACE_END(bgLoader)
