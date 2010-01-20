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

//#ifndef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!x) printf("ART ERROR: %s\n", msg);
//#endif

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
SCF_IMPLEMENT_FACTORY(BgLoader)

BgLoader::BgLoader(iBase *p)
  : scfImplementationType (this, p), parsedShaders(false),
  loadRange(200), enabledGfxFeatures(0),
  validPosition(false), resetHitbeam(true)
{
}

BgLoader::~BgLoader()
{
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
    size_t i = 0;
    size_t count = 0;
    while(count < 10)
    {
        // True if at least one mesh finished load.
        bool finished = false;

        // Delete from delete queue (fairly expensive, so limited per update).
        if(!deleteQueue.IsEmpty())
        {
            CleanMesh(deleteQueue[0]);
            deleteQueue.DeleteIndexFast(0);
        }

        // Check if we need to reset i
        if (i == loadingMeshes.GetSize())
            i = 0;

        // Check already loading meshes.
        for(; i<(loadingMeshes.GetSize() < 20 ? loadingMeshes.GetSize() : 20); ++i)
        {
            if(LoadMesh(loadingMeshes[i]))
            {
                finished = true;
                finalisableMeshes.Push(loadingMeshes[i]);
                loadingMeshes.DeleteIndex(i);
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
        if(csVector3(lastPos - pos).Norm() < loadRange/10)
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
        if(sectors[i]->object.IsValid() && connectedSectors.Find(sectors[i]) == csArrayItemNotFound)
        {
            CleanSector(sectors[i]);
        }
    }
}

void BgLoader::FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector)
{
    if(connectedSectors.Find(sector) != csArrayItemNotFound)
    {
        return;
    }

    connectedSectors.Push(sector);

    for(size_t i=0; i<sector->activePortals.GetSize(); i++)
    {
        FindConnectedSectors(connectedSectors, sector->activePortals[i]->targetSector);
    }
}

void BgLoader::CleanSector(Sector* sector)
{
    if(!sector->object.IsValid())
        return;

    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(sector->meshes[i]->object.IsValid())
        {
            sector->meshes[i]->object->GetMovable()->ClearSectors();
            sector->meshes[i]->object->GetMovable()->UpdateMove();
            engine->GetMeshes()->Remove(sector->meshes[i]->object);
            sector->meshes[i]->object.Invalidate();
            CleanMesh(sector->meshes[i]);
            --(sector->objectCount);
        }
    }

    for(size_t i=0; i<sector->meshgen.GetSize(); i++)
    {
        CleanMeshGen(sector->meshgen[i]);
        --(sector->objectCount);
    }

    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(sector->portals[i]->mObject.IsValid())
        {
            engine->GetMeshes()->Remove(sector->portals[i]->mObject);
            sector->portals[i]->pObject = NULL;
            sector->portals[i]->mObject.Invalidate();
            sector->activePortals.Delete(sector->portals[i]);
            --(sector->objectCount);
        }
    }

    for(size_t i=0; i<sector->lights.GetSize(); i++)
    {
        if(sector->lights[i]->object.IsValid())
        {
            engine->RemoveLight(sector->lights[i]->object);
            sector->lights[i]->object.Invalidate();
            --(sector->objectCount);
        }

        for(size_t j=0; j<sector->lights[i]->sequences.GetSize(); ++j)
        {
            if(sector->lights[i]->sequences[j]->status.IsValid())
            {
                for(size_t k=0; k<sector->lights[i]->sequences[j]->triggers.GetSize(); ++k)
                {
                    if(sector->lights[i]->sequences[j]->triggers[k]->status.IsValid())
                    {
                        csRef<iSequenceTrigger> st = scfQueryInterface<iSequenceTrigger>(sector->lights[i]->sequences[j]->triggers[k]->status->GetResultRefPtr());
                        engseq->RemoveTrigger(st);
                        sector->lights[i]->sequences[j]->triggers[k]->status.Invalidate();
                    }
                }

                csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(sector->lights[i]->sequences[j]->status->GetResultRefPtr());
                engseq->RemoveSequence(sw);
                sector->lights[i]->sequences[j]->status.Invalidate();
            }
        }
    }

    // Unload all 'always loaded' meshes before destroying sector.
    if(sector->objectCount != 0)
    {
        for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
        {
            sector->alwaysLoaded[i]->object->GetMovable()->ClearSectors();
            sector->alwaysLoaded[i]->object->GetMovable()->UpdateMove();
            engine->GetMeshes()->Remove(sector->alwaysLoaded[i]->object);
            sector->alwaysLoaded[i]->object.Invalidate();
            CleanMesh(sector->alwaysLoaded[i]);
            --(sector->objectCount);
        }
    }

    // Remove sequences.
    for(size_t i=0; i<sector->sequences.GetSize(); ++i)
    {
        if(sector->sequences[i]->status.IsValid())
        {
            csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(sector->sequences[i]->status->GetResultRefPtr());
            engseq->RemoveSequence(sw);
            sector->sequences[i]->status.Invalidate();
        }
    }

    if(sector->objectCount != 0)
    {
        csString msg;
        msg.Format("Error cleaning sector. Sector still has %zu objects!", sector->objectCount);
        CS_ASSERT_MSG(msg.GetData(), false);
    }
    CS_ASSERT_MSG("Error cleaning sector. Sector is invalid!", sector->object.IsValid());

    // Remove the sector from the engine.
    sector->checked = false;
    sector->object->QueryObject()->SetObjectParent(0);
    engine->GetSectors()->Remove(sector->object);
    sector->object.Invalidate();
}

void BgLoader::CleanMesh(MeshObj* mesh)
{
    for(size_t i=0; i<mesh->sequences.GetSize(); ++i)
    {
        if(mesh->sequences[i]->status.IsValid())
        {
            for(size_t j=0; j<mesh->sequences[i]->triggers.GetSize(); ++j)
            {
                if(mesh->sequences[i]->triggers[j]->status.IsValid())
                {
                    csRef<iSequenceTrigger> st = scfQueryInterface<iSequenceTrigger>(mesh->sequences[i]->triggers[j]->status->GetResultRefPtr());
                    engseq->RemoveTrigger(st);
                    mesh->sequences[i]->triggers[j]->status.Invalidate();
                }
            }

            csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(mesh->sequences[i]->status->GetResultRefPtr());
            engseq->RemoveSequence(sw);
            mesh->sequences[i]->status.Invalidate();
        }
    }

    for(size_t i=0; i<mesh->meshfacts.GetSize(); ++i)
    {
        CleanMeshFact(mesh->meshfacts[i]);
    }

    for(size_t i=0; i<mesh->mftchecked.GetSize(); ++i)
    {
        mesh->mftchecked[i] = false;
    }

    for(size_t i=0; i<mesh->materials.GetSize(); ++i)
    {
        CleanMaterial(mesh->materials[i]);
    }

    for(size_t i=0; i<mesh->matchecked.GetSize(); ++i)
    {
        mesh->matchecked[i] = false;
    }

    for(size_t i=0; i<mesh->textures.GetSize(); ++i)
    {
        CleanTexture(mesh->textures[i]);
    }

    for(size_t i=0; i<mesh->texchecked.GetSize(); ++i)
    {
        mesh->texchecked[i] = false;
    }
}

void BgLoader::CleanMeshGen(MeshGen* meshgen)
{
  meshgen->sector->object->RemoveMeshGenerator(meshgen->name);
  meshgen->status.Invalidate();

  for(size_t i=0; i<meshgen->meshfacts.GetSize(); ++i)
  {
      CleanMeshFact(meshgen->meshfacts[i]);
  }

  for(size_t i=0; i<meshgen->mftchecked.GetSize(); ++i)
  {
      meshgen->mftchecked[i] = false;
  }

  for(size_t i=0; i<meshgen->materials.GetSize(); ++i)
  {
      CleanMaterial(meshgen->materials[i]);
  }

  for(size_t i=0; i<meshgen->matchecked.GetSize(); ++i)
  {
      meshgen->matchecked[i] = false;
  }
}

void BgLoader::CleanMeshFact(MeshFact* meshfact)
{
  if(--meshfact->useCount == 0)
  {
      csWeakRef<iMeshFactoryWrapper> mf = scfQueryInterface<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());
      if(mf->GetRefCount() == 2)
      {
          engine->GetMeshFactories()->Remove(mf);
      }

      meshfact->status.Invalidate();

      for(size_t i=0; i<meshfact->materials.GetSize(); ++i)
      {
          CleanMaterial(meshfact->materials[i]);
      }

      for(size_t i=0; i<meshfact->checked.GetSize(); ++i)
      {
          meshfact->checked[i] = false;
      }
  }
}

void BgLoader::CleanMaterial(Material* material)
{
  if(--material->useCount == 0)
  {
      if(material->mat->GetRefCount() == 2)
      {
          engine->GetMaterialList()->Remove(material->mat);
      }

      material->mat.Invalidate();

      for(size_t i=0; i<material->textures.GetSize(); ++i)
      {
          CleanTexture(material->textures[i]);
      }

      for(size_t i=0; i<material->checked.GetSize(); ++i)
      {
          material->checked[i] = false;
      }
  }
}

void BgLoader::CleanTexture(Texture* texture)
{
    if(--texture->useCount == 0)
    {
        csWeakRef<iTextureWrapper> t = scfQueryInterface<iTextureWrapper>(texture->status->GetResultRefPtr());
        if(t->GetRefCount() == 2)
        {
            engine->GetTextureList()->Remove(t);
        }

        texture->status.Invalidate();
    }
}

void BgLoader::LoadSector(const csBox3& loadBox, const csBox3& unloadBox,
                        Sector* sector, uint depth, bool force, bool loadMeshes, bool portalsOnly)
{
    sector->isLoading = true;

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
                    csVector3& transform = sector->activePortals[i]->transform;
                    wwLoadBox.SetMin(0, wwLoadBox.MinX()-transform.x);
                    wwLoadBox.SetMin(1, wwLoadBox.MinY()-transform.y);
                    wwLoadBox.SetMin(2, wwLoadBox.MinZ()-transform.z);
                    wwLoadBox.SetMax(0, wwLoadBox.MaxX()-transform.x);
                    wwLoadBox.SetMax(1, wwLoadBox.MaxY()-transform.y);
                    wwLoadBox.SetMax(2, wwLoadBox.MaxZ()-transform.z);
                    wwUnloadBox.SetMin(0, wwUnloadBox.MinX()-transform.x);
                    wwUnloadBox.SetMin(1, wwUnloadBox.MinY()-transform.y);
                    wwUnloadBox.SetMin(2, wwUnloadBox.MinZ()-transform.z);
                    wwUnloadBox.SetMax(0, wwUnloadBox.MaxX()-transform.x);
                    wwUnloadBox.SetMax(1, wwUnloadBox.MaxY()-transform.y);
                    wwUnloadBox.SetMax(2, wwUnloadBox.MaxZ()-transform.z);
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
                    sector->meshes[i]->object->GetMovable()->ClearSectors();
                    sector->meshes[i]->object->GetMovable()->UpdateMove();
                    engine->GetMeshes()->Remove(sector->meshes[i]->object);
                    sector->meshes[i]->object.Invalidate();
                    deleteQueue.Push(sector->meshes[i]);
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
                if(sector->portals[i]->mObject)
                {
                    engine->GetMeshes()->Remove(sector->portals[i]->mObject);
                    sector->portals[i]->pObject = NULL;
                    sector->portals[i]->mObject.Invalidate();
                    sector->activePortals.Delete(sector->portals[i]);
                    --(sector->objectCount);
                }

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
                    csVector3& transform = sector->portals[i]->transform;
                    wwLoadBox.SetMin(0, wwLoadBox.MinX()-transform.x);
                    wwLoadBox.SetMin(1, wwLoadBox.MinY()-transform.y);
                    wwLoadBox.SetMin(2, wwLoadBox.MinZ()-transform.z);
                    wwLoadBox.SetMax(0, wwLoadBox.MaxX()-transform.x);
                    wwLoadBox.SetMax(1, wwLoadBox.MaxY()-transform.y);
                    wwLoadBox.SetMax(2, wwLoadBox.MaxZ()-transform.z);
                    wwUnloadBox.SetMin(0, wwUnloadBox.MinX()-transform.x);
                    wwUnloadBox.SetMin(1, wwUnloadBox.MinY()-transform.y);
                    wwUnloadBox.SetMin(2, wwUnloadBox.MinZ()-transform.z);
                    wwUnloadBox.SetMax(0, wwUnloadBox.MaxX()-transform.x);
                    wwUnloadBox.SetMax(1, wwUnloadBox.MaxY()-transform.y);
                    wwUnloadBox.SetMax(2, wwUnloadBox.MaxZ()-transform.z);
                }
                LoadSector(wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1, false, loadMeshes);
            }

            sector->portals[i]->mObject = engine->CreatePortal(sector->portals[i]->name, sector->object,
                csVector3(0), sector->portals[i]->targetSector->object, sector->portals[i]->poly.GetVertices(),
                (int)sector->portals[i]->poly.GetVertexCount(), sector->portals[i]->pObject);

            if(sector->portals[i]->warp)
            {
                sector->portals[i]->pObject->SetWarp(sector->portals[i]->matrix, sector->portals[i]->wv, sector->portals[i]->ww);
            }

            if(sector->portals[i]->pfloat)
            {
                sector->portals[i]->pObject->GetFlags().SetBool(CS_PORTAL_FLOAT, true);
            }

            if(sector->portals[i]->clip)
            {
                sector->portals[i]->pObject->GetFlags().SetBool(CS_PORTAL_CLIPDEST, true);
            }

            if(sector->portals[i]->zfill)
            {
                sector->portals[i]->pObject->GetFlags().SetBool(CS_PORTAL_ZFILL, true);
            }

            sector->activePortals.Push(sector->portals[i]);
            ++(sector->objectCount);
        }
        else if(!force && sector->portals[i]->OutOfRange(unloadBox))
        {
            if(!sector->portals[i]->targetSector->isLoading)
            {
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->portals[i]->warp)
                {
                    csVector3& transform = sector->portals[i]->transform;
                    wwLoadBox.SetMin(0, wwLoadBox.MinX()-transform.x);
                    wwLoadBox.SetMin(1, wwLoadBox.MinY()-transform.y);
                    wwLoadBox.SetMin(2, wwLoadBox.MinZ()-transform.z);
                    wwLoadBox.SetMax(0, wwLoadBox.MaxX()-transform.x);
                    wwLoadBox.SetMax(1, wwLoadBox.MaxY()-transform.y);
                    wwLoadBox.SetMax(2, wwLoadBox.MaxZ()-transform.z);
                    wwUnloadBox.SetMin(0, wwUnloadBox.MinX()-transform.x);
                    wwUnloadBox.SetMin(1, wwUnloadBox.MinY()-transform.y);
                    wwUnloadBox.SetMin(2, wwUnloadBox.MinZ()-transform.z);
                    wwUnloadBox.SetMax(0, wwUnloadBox.MaxX()-transform.x);
                    wwUnloadBox.SetMax(1, wwUnloadBox.MaxY()-transform.y);
                    wwUnloadBox.SetMax(2, wwUnloadBox.MaxZ()-transform.z);
                }
                LoadSector(wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1, false, loadMeshes);
            }

            engine->GetMeshes()->Remove(sector->portals[i]->mObject);
            sector->portals[i]->pObject = NULL;
            sector->portals[i]->mObject.Invalidate();
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
                sector->lights[i]->object = engine->CreateLight(sector->lights[i]->name, sector->lights[i]->pos,
                    sector->lights[i]->radius, sector->lights[i]->colour, sector->lights[i]->dynamic);
                sector->lights[i]->object->SetAttenuationMode(sector->lights[i]->attenuation);
                sector->lights[i]->object->SetType(sector->lights[i]->type);
                sector->object->GetLights()->Add(sector->lights[i]->object);
                ++sector->objectCount;

                // Load all light sequences.
                for(size_t j=0; j<sector->lights[i]->sequences.GetSize(); ++j)
                {
                    sector->lights[i]->sequences[j]->status = tloader->LoadNodeWait(vfs->GetCwd(),
                        sector->lights[i]->sequences[j]->data);
                    for(size_t k=0; k<sector->lights[i]->sequences[j]->triggers.GetSize(); ++k)
                    {
                        sector->lights[i]->sequences[j]->triggers[k]->status = tloader->LoadNode(vfs->GetCwd(),
                            sector->lights[i]->sequences[j]->triggers[k]->data);
                    }
                }
            }
            else if(!force && sector->lights[i]->OutOfRange(unloadBox))
            {
                engine->RemoveLight(sector->lights[i]->object);
                sector->lights[i]->object.Invalidate();
                --sector->objectCount;

                for(size_t j=0; j<sector->lights[i]->sequences.GetSize(); ++j)
                {
                    if(sector->lights[i]->sequences[j]->status.IsValid())
                    {
                        for(size_t k=0; k<sector->lights[i]->sequences[j]->triggers.GetSize(); ++k)
                        {
                            if(sector->lights[i]->sequences[j]->triggers[k]->status.IsValid())
                            {
                                csRef<iSequenceTrigger> st = scfQueryInterface<iSequenceTrigger>(sector->lights[i]->sequences[j]->triggers[k]->status->GetResultRefPtr());
                                engseq->RemoveTrigger(st);
                                sector->lights[i]->sequences[j]->triggers[k]->status.Invalidate();
                            }
                        }

                        csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(sector->lights[i]->sequences[j]->status->GetResultRefPtr());
                        engseq->RemoveSequence(sw);
                        sector->lights[i]->sequences[j]->status.Invalidate();
                    }
                }
            }
        }

        if(loadMeshes && !portalsOnly)
        {
            // Load all sector sequences.
            for(size_t i=0; i<sector->sequences.GetSize(); i++)
            {
                if(!sector->sequences[i]->status.IsValid())
                {
                    sector->sequences[i]->status = tloader->LoadNode(vfs->GetCwd(),
                        sector->sequences[i]->data);
                }
            }
        }

        // Check whether this sector is empty and should be unloaded.
        if(sector->objectCount == sector->alwaysLoaded.GetSize() && sector->object.IsValid())
        {
            // Unload all 'always loaded' meshes before destroying sector.
            for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
            {
                sector->alwaysLoaded[i]->object->GetMovable()->ClearSectors();
                sector->alwaysLoaded[i]->object->GetMovable()->UpdateMove();
                engine->GetMeshes()->Remove(sector->alwaysLoaded[i]->object);
                sector->alwaysLoaded[i]->object.Invalidate();
                deleteQueue.Push(sector->meshes[i]);
                --(sector->objectCount);
            }

            // Remove sequences.
            for(size_t i=0; i<sector->sequences.GetSize(); i++)
            {
                if(sector->sequences[i]->status.IsValid())
                {
                    csRef<iSequenceWrapper> sw = scfQueryInterface<iSequenceWrapper>(sector->sequences[i]->status->GetResultRefPtr());
                    engseq->RemoveSequence(sw);
                    sector->sequences[i]->status.Invalidate();
                }
            }

            // Remove the sector from the engine.
            sector->object->QueryObject()->SetObjectParent(0);
            engine->GetSectors()->Remove(sector->object);
            sector->object.Invalidate();
        }
    }

    sector->checked = true;
    sector->isLoading = false;
}

void BgLoader::FinishMeshLoad(MeshObj* mesh)
{
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

    if(ready && !meshgen->status)
    {
        meshgen->status = tloader->LoadNode(vfs->GetCwd(), meshgen->data, 0, meshgen->sector->object);
        return false;
    }

    if(meshgen->status && meshgen->status->IsFinished())
    {
      meshgen->loading = false;
      return true;
    }

    return false;
}

bool BgLoader::LoadMesh(MeshObj* mesh)
{
    if(mesh->object.IsValid())
      return true;

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

    if(ready)
    {
        if(!mesh->status)
            mesh->status = tloader->LoadNode(mesh->path, mesh->data);

        ready = mesh->status->IsFinished();
    }

    // Load sequences.
    if(ready)
    {
        for(size_t i=0; i<mesh->sequences.GetSize(); ++i)
        {
            if(mesh->sequences[i]->loaded)
                continue;

            if(!mesh->sequences[i]->status)
            {
                mesh->sequences[i]->status = tloader->LoadNode(mesh->path, mesh->sequences[i]->data);
                ready = false;
            }
            else
            {
                if(ready && mesh->sequences[i]->status->IsFinished())
                {
                    if(!mesh->sequences[i]->status->WasSuccessful())
                    {
                        csString msg;
                        msg.Format("Sequence '%s' in mesh '%s' failed to load.\n", mesh->sequences[i]->name.GetData(), mesh->name.GetData());
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                    
                    mesh->sequences[i]->loaded = true;
                }
                else
                    ready = false;
            }
        }
    }

    // Load triggers
    if(ready)
    {
        for(size_t i=0; i<mesh->sequences.GetSize(); ++i)
        {
            for(size_t j=0; j<mesh->sequences[i]->triggers.GetSize(); ++j)
            {
                if(mesh->sequences[i]->triggers[j]->loaded)
                	continue;

                if(!mesh->sequences[i]->triggers[j]->status)
                {
                    mesh->sequences[i]->triggers[j]->status = tloader->LoadNode(mesh->path, mesh->sequences[i]->triggers[j]->data);
                    ready = false;
                }
                else
                {
                    if(ready && mesh->sequences[i]->triggers[j]->status->IsFinished())
                    {
                        if(!mesh->sequences[i]->triggers[j]->status->WasSuccessful())
                        {
                            csString msg;
                            msg.Format("Trigger '%s' in mesh '%s' failed to load.\n",
                                mesh->sequences[i]->triggers[j]->name.GetData(), mesh->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);
                        }

                        mesh->sequences[i]->triggers[j]->loaded = true;
                    }
                    else
                        ready = false;
                }
            }
        }
    }

    return ready;
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

    if(ready && !meshfact->status)
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

    if(meshfact->status && meshfact->status->IsFinished())
    {
        ++meshfact->useCount;
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

    if(texture->status->IsFinished())
    {
        ++texture->useCount;
        return true;
    }

    return false;
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
        if(!failed)
        {
            // Check success.
            csString msg;
            msg.Format("Failed to load factory '%s' path: %s filename: %s", name, (const char*) meshfact->path, (const char*) meshfact->filename);
            CS_ASSERT_MSG(msg.GetData(), meshfact->status->WasSuccessful());
        }
        else if(!meshfact->status->WasSuccessful())
        {
            *failed = true;
            return csPtr<iMeshFactoryWrapper>(0);
        }

        return scfQueryInterfaceSafe<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());
    }

    return csPtr<iMeshFactoryWrapper>(0);
}

csPtr<iMaterialWrapper> BgLoader::LoadMaterial(const char* name, bool* failed, bool wait)
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
          return csPtr<iMaterialWrapper>(0);
        }
    }

    if(LoadMaterial(material, wait))
    {
        return csPtr<iMaterialWrapper>(material->mat);
    }

    return csPtr<iMaterialWrapper>(0);
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

bool BgLoader::LoadZones(iStringArray* regions, bool loadMeshes)
{
    // Firstly, get a list of all zones that should be loaded.
    csRefArray<Zone> newLoadedZones;
    for(size_t i=0; i<regions->GetSize(); ++i)
    {
        csRef<Zone> zone = zones.Get(zStringSet.Request(regions->Get(i)), csRef<Zone>());
        if(zone.IsValid())
        {
            newLoadedZones.Push(zone);
        }
        else
        {
            return false;
        }
    }

    // Next clean all sectors which shouldn't be loaded.
    for(size_t i=0; i<loadedZones.GetSize(); ++i)
    {
        bool found = false;
        for(size_t j=0; j<newLoadedZones.GetSize(); ++j)
        {
            if(loadedZones[i] == newLoadedZones[j])
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            for(size_t j=0; j<loadedZones[i]->sectors.GetSize(); ++j)
            {
                CleanSector(loadedZones[i]->sectors[j]);
            }

            loadedZones.DeleteIndex(i);
            --i;
        }
    }

    // Now load all sectors which should be loaded.
    for(size_t i=0; i<newLoadedZones.GetSize(); ++i)
    {
        bool found = false;
        for(size_t j=0; j<loadedZones.GetSize(); ++j)
        {
            if(newLoadedZones[i] == loadedZones[j])
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            loadedZones.Push(newLoadedZones[i]);
            for(size_t j=0; j<newLoadedZones[i]->sectors.GetSize(); ++j)
            {
                LoadSector(csBox3(), csBox3(), newLoadedZones[i]->sectors[j], (uint)-1, true, loadMeshes);
            }
        }
        else
        {
            for(size_t j=0; j<newLoadedZones[i]->sectors.GetSize(); ++j)
            {
                LoadSector(csBox3(), csBox3(), newLoadedZones[i]->sectors[j], (uint)-1, true, loadMeshes, true);
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
                if(!loadedZones[i]->sectors[j]->activePortals[k]->targetSector->checked)
                {
                    CleanSector(loadedZones[i]->sectors[j]->activePortals[k]->targetSector);
                    
                    // TODO: improve this, it's a bit hacky.
                    engine->GetMeshes()->Remove(loadedZones[i]->sectors[j]->activePortals[k]->mObject);
                    loadedZones[i]->sectors[j]->activePortals[k]->pObject = NULL;
                    loadedZones[i]->sectors[j]->activePortals[k]->mObject.Invalidate();
                    loadedZones[i]->sectors[j]->activePortals.Delete(loadedZones[i]->sectors[j]->activePortals[k]);
                    --(loadedZones[i]->sectors[j]->objectCount);
                }
            }
        }
    }

    return true;
}
}
CS_PLUGIN_NAMESPACE_END(bgLoader)
