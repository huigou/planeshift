/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cstool/collider.h>
#include <csutil/scanstr.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/stringarray.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivideo/material.h>

#include "loader.h"
#include "util/strutil.h"
#include "globals.h"

void Loader::Init(iObjectRegistry* object_reg, uint gfxFeatures, float loadRange)
{
    this->object_reg = object_reg;
    this->gfxFeatures = gfxFeatures;
    this->loadRange = loadRange;
    
    validPosition = false;

    engine = csQueryRegistry<iEngine> (object_reg);
    loader = csQueryRegistry<iLoader> (object_reg);
    tloader = csQueryRegistry<iThreadedLoader> (object_reg);
    vfs = csQueryRegistry<iVFS> (object_reg);
    svstrings = csQueryRegistryTagInterface<iShaderVarStringSet>(object_reg, "crystalspace.shader.variablenameset");
    strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
    cdsys = csQueryRegistry<iCollideSystem> (object_reg);

    syntaxService = csQueryRegistryOrLoad<iSyntaxService>(object_reg, "crystalspace.syntax.loader.service.text");

    csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D>(object_reg);
    txtmgr = g3d->GetTextureManager();

    engine->SetClearZBuf(true);
}

THREADED_CALLABLE_IMPL2(Loader, PrecacheData, const char* path, bool recursive)
{
    // Don't parse folders.
    csString folder(path);
    if(folder.GetAt(folder.Length()-1) == '/')
        return false;

    if(vfs->Exists(path))
    {
        csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
        csRef<iDocument> doc = docsys->CreateDocument();
        csRef<iDataBuffer> data = vfs->ReadFile(path);

        if(!recursive)
        {
            vfs->ChDir(csString(path).Truncate(csString(path).FindLast('/')));
        }

        doc->Parse(data, true);

        // Check that it's an xml file.
        if(!doc->GetRoot())
            return false;

        csRef<iDocumentNode> root = doc->GetRoot()->GetNode("library");
        if(!root.IsValid())
        {
            root = doc->GetRoot()->GetNode("world");
        }

        if(root.IsValid())
        {
            csRef<iDocumentNode> node;
            csRef<iDocumentNodeIterator> nodeItr;

            nodeItr = root->GetNodes("library");
            while(nodeItr->HasNext())
            {
                node = nodeItr->Next();
                PrecacheDataTC(ret, false, node->GetContentsValue(), true);
            }

            node = root->GetNode("plugins");
            if(node.IsValid())
            {
                loader->Load(node);
            }

            node = root->GetNode("shaders");
            if(node.IsValid())
            {
                nodeItr = root->GetNodes("shader");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();
                    node = node->GetNode("file");
                    tloader->LoadShader(vfs->GetCwd(), node->GetContentsValue());
                }
            }

            node = root->GetNode("textures");
            if(node.IsValid())
            {
                nodeItr = node->GetNodes("texture");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();
                    csRef<Texture> t = csPtr<Texture>(new Texture());
                    {
                        t->name = node->GetAttributeValue("name");
                        t->data = node;
                        CS::Threading::MutexScopedLock lock(tLock);
                        textures.Put(t->name, t);
                    }
                }
            }

            node = root->GetNode("materials");
            if(node.IsValid())
            {
                nodeItr = node->GetNodes("material");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();
                    csRef<Material> m = csPtr<Material>(new Material(node->GetAttributeValue("name")));
                    {
                        CS::Threading::MutexScopedLock lock(mLock);
                        materials.Put(m->name, m);
                    }

                    if(node->GetNode("texture"))
                    {
                        node = node->GetNode("texture");
                        ShaderVar sv("tex diffuse", csShaderVariable::TEXTURE);
                        sv.value = node->GetContentsValue();
                        m->shadervars.Push(sv);

                        CS::Threading::MutexScopedLock lock(tLock);
                        csRef<Texture> texture = textures.Get(node->GetContentsValue(), csRef<Texture>());
                        {
                            // Validation.
                            csString msg;
                            msg.Format("Invalid texture reference '%s' in material '%s'", node->GetContentsValue(), node->GetParent()->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                        }
                        m->textures.Push(texture);

                        node = node->GetParent();
                    }

                    csRef<iDocumentNodeIterator> nodeItr2 = node->GetNodes("shader");
                    while(nodeItr2->HasNext())
                    {
                        node = nodeItr2->Next();
                        m->shaders.Push(Shader(node->GetAttributeValue("type"), node->GetContentsValue()));
                        node = node->GetParent();
                    }

                    nodeItr2 = node->GetNodes("shadervar");
                    while(nodeItr2->HasNext())
                    {
                        node = nodeItr2->Next();
                        if(csString("texture").Compare(node->GetAttributeValue("type")))
                        {
                            ShaderVar sv(node->GetAttributeValue("name"), csShaderVariable::TEXTURE);
                            sv.value = node->GetContentsValue();
                            m->shadervars.Push(sv);
                            CS::Threading::MutexScopedLock lock(tLock);
                            csRef<Texture> texture = textures.Get(node->GetContentsValue(), csRef<Texture>());
                            {
                                // Validation.
                                csString msg;
                                msg.Format("Invalid texture reference '%s' in shadervar", node->GetContentsValue());
                                CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                            }
                            m->textures.Push(texture);
                        }
                        else if(csString("vector2").Compare(node->GetAttributeValue("type")))
                        {
                            ShaderVar sv(node->GetAttributeValue("name"), csShaderVariable::VECTOR2);
                            csScanStr (node->GetContentsValue(), "%f,%f", &sv.vec2.x, &sv.vec2.y);
                            m->shadervars.Push(sv);
                        }
                        node = node->GetParent();
                    }
                }
            }

            nodeItr = root->GetNodes("meshfact");
            while(nodeItr->HasNext())
            {
                node = nodeItr->Next();
                csRef<MeshFact> mf = csPtr<MeshFact>(new MeshFact(node->GetAttributeValue("name"), node));
                csRef<iDocumentNodeIterator> nodeItr3 = node->GetNode("params")->GetNodes("submesh");
                while(nodeItr3->HasNext())
                {
                    csRef<iDocumentNode> node2 = nodeItr3->Next();
                    if(node2->GetNode("material"))
                    {
                        CS::Threading::MutexScopedLock lock(mLock);
                        csRef<Material> material = materials.Get(node2->GetNode("material")->GetContentsValue(), csRef<Material>());
                        {
                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in meshfact '%s'", node2->GetNode("material")->GetContentsValue(), node->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        mf->materials.Push(material);
                    }
                }

                nodeItr3 = node->GetNode("params")->GetNodes("mesh");
                while(nodeItr3->HasNext())
                {
                    csRef<iDocumentNode> node2 = nodeItr3->Next();
                    CS::Threading::MutexScopedLock lock(mLock);
                    csRef<Material> material = materials.Get(node2->GetAttributeValue("material"), csRef<Material>());
                    {
                        // Validation.
                        csString msg;
                        msg.Format("Invalid material reference '%s' in cal3d meshfact '%s'", node2->GetAttributeValue("material"), node->GetAttributeValue("name"));
                        CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                    }
                    mf->materials.Push(material);
                }

                if(node->GetNode("params")->GetNode("cells"))
                {
                    node = node->GetNode("params")->GetNode("cells")->GetNode("celldefault")->GetNode("basematerial");
                    {
                        CS::Threading::MutexScopedLock lock(mLock);
                        csRef<Material> material = materials.Get(node->GetContentsValue(), csRef<Material>());
                        {
                            // Validation.
                            csString msg;
                            msg.Format("Invalid basematerial reference '%s' in terrain mesh", node->GetContentsValue());
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        mf->materials.Push(material);
                    }
                    node = node->GetParent()->GetParent();

                    nodeItr3 = node->GetNodes("cell");
                    while(nodeItr3->HasNext())
                    {
                        node = nodeItr3->Next();
                        node = node->GetNode("feederproperties");
                        if(node)
                        {
                            csRef<iDocumentNodeIterator> nodeItr4 = node->GetNodes("alphamap");
                            while(nodeItr4->HasNext())
                            {
                                csRef<iDocumentNode> node2 = nodeItr4->Next();
                                CS::Threading::MutexScopedLock lock(mLock);
                                csRef<Material> material = materials.Get(node2->GetAttributeValue("material"), csRef<Material>());
                                {
                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid alphamap reference '%s' in terrain mesh", node2->GetAttributeValue("material"));
                                    CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                                }

                                mf->materials.Push(material);
                            }
                        }
                    }
                }

                CS::Threading::MutexScopedLock lock(mfLock);
                meshfacts.Put(mf->name, mf);
            }

            nodeItr = root->GetNodes("sector");
            while(nodeItr->HasNext())
            {
                node = nodeItr->Next();

                csRef<Sector> s;
                csString sectorName = node->GetAttributeValue("name");
                {
                    CS::Threading::MutexScopedLock lock(sLock);
                    for(size_t i=0; i<sectors.GetSize(); i++)
                    {
                        if(sectors[i]->name.Compare(sectorName))
                        {
                            s = sectors[i];
                            break;
                        }
                    }
                }

                if(!s.IsValid())
                {
                    s = csPtr<Sector>(new Sector(sectorName));
                    CS::Threading::MutexScopedLock lock(sLock);
                    sectors.Push(s);
                }

                s->culler = node->GetNode("cullerp")->GetContentsValue();
                if(node->GetNode("ambient"))
                {
                    node = node->GetNode("ambient");
                    s->ambient = csColor(node->GetAttributeValueAsFloat("red"),
                        node->GetAttributeValueAsFloat("green"), node->GetAttributeValueAsFloat("blue"));
                    node = node->GetParent();
                }

                csRef<iDocumentNodeIterator> nodeItr2 = node->GetNodes("meshobj");
                while(nodeItr2->HasNext())
                {
                    csRef<iDocumentNode> node2 = nodeItr2->Next();
                    csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(node2->GetAttributeValue("name"), node2));
                    m->sector = s;

                    if(node2->GetAttributeValueAsBool("alwaysloaded"))
                    {
                        ++s->alwaysLoadedCount;
                        m->alwaysLoaded = true;
                    }

                    csRef<iDocumentNode> move = node2->GetNode("move");
                    if(move.IsValid())
                    {
                        move = move->GetNode("v");
                        m->pos = csVector3(move->GetAttributeValueAsFloat("x"),
                            move->GetAttributeValueAsFloat("y"), move->GetAttributeValueAsFloat("z"));
                        move = move->GetParent();
                    }

                    if(node2->GetNode("paramsfile"))
                    {
                        csRef<iDocument> pdoc = docsys->CreateDocument();
                        csRef<iDataBuffer> pdata = vfs->ReadFile(node2->GetNode("paramsfile")->GetContentsValue());
                        pdoc->Parse(pdata, true);
                        node2 = pdoc->GetRoot();
                    }

                    csRef<iDocumentNodeIterator> nodeItr3 = node2->GetNode("params")->GetNodes("submesh");
                    while(nodeItr3->HasNext())
                    {
                        csRef<iDocumentNode> node3 = nodeItr3->Next();
                        if(node3->GetNode("material"))
                        {
                            CS::Threading::MutexScopedLock lock(mLock);
                            csRef<Material> material = materials.Get(node3->GetNode("material")->GetContentsValue(), csRef<Material>());
                            {
                                // Validation.
                                csString msg;
                                msg.Format("Invalid material reference '%s' in meshobj submesh", node3->GetNode("material")->GetContentsValue());
                                CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                            }

                            m->materials.Push(material);
                        }

                        csRef<iDocumentNodeIterator> nodeItr4 = node3->GetNodes("shadervar");
                        while(nodeItr4->HasNext())
                        {
                            node3 = nodeItr4->Next();
                            if(csString("texture").Compare(node3->GetAttributeValue("type")))
                            {
                                CS::Threading::MutexScopedLock lock(tLock);
                                csRef<Texture> texture = textures.Get(node3->GetContentsValue(), csRef<Texture>());
                                {
                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid texture reference '%s' in meshobj shadervar", node3->GetContentsValue());
                                    CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                                }

                                m->textures.Push(texture);
                            }
                        }
                    }

                    node2 = node2->GetNode("params")->GetNode("factory");
                    {
                        CS::Threading::MutexScopedLock lock(mfLock);
                        csRef<MeshFact> meshfact = meshfacts.Get(node2->GetContentsValue(), csRef<MeshFact>());
                        {
                            // Validation.
                            csString msg;
                            msg.Format("Invalid factory reference '%s' in meshobj '%s'", node2->GetContentsValue(),
                                node2->GetParent()->GetParent()->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
                        }

                        // Calc bbox data.
                        csRef<iDocumentNodeIterator> keys = meshfact->data->GetNodes("key");
                        while(keys->HasNext())
                        {
                            csRef<iDocumentNode> bboxdata = keys->Next();
                            if(csString("bbox").Compare(bboxdata->GetAttributeValue("name")))
                            {
                                csRef<iDocumentNode> position = move;
                                if(position.IsValid())
                                {
                                    m->hasBBox = true;
                                    csVector3 pos;
                                    syntaxService->ParseVector(position->GetNode("v"), pos);

                                    csMatrix3 rot;
                                    if(position->GetNode("matrix"))
                                    {
                                        syntaxService->ParseMatrix(position->GetNode("matrix"), rot);
                                    }

                                    csRef<iDocumentNodeIterator> vs = bboxdata->GetNodes("v");
                                    while(vs->HasNext())
                                    {
                                        bboxdata = vs->Next();
                                        csVector3 bPos;
                                        syntaxService->ParseVector(bboxdata, bPos);
                                        if(position->GetNode("matrix"))
                                        {
                                            m->bbox.AddBoundingVertex(rot*csVector3(pos+bPos));
                                        }
                                        else
                                        {
                                            m->bbox.AddBoundingVertex(pos+bPos);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        m->meshfacts.Push(meshfact);
                    }
                    node2 = node2->GetParent();

                    if(node2->GetNode("material"))
                    {
                        node2 = node2->GetNode("material");
                        CS::Threading::MutexScopedLock lock(mLock);
                        csRef<Material> material = materials.Get(node2->GetContentsValue(), csRef<Material>());
                        {
                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in terrain object", node2->GetContentsValue());
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        m->materials.Push(material);
                        node2 = node2->GetParent();
                    }


                    if(node2->GetNode("materialpalette"))
                    {
                        nodeItr3 = node2->GetNode("materialpalette")->GetNodes("material");
                        while(nodeItr3->HasNext())
                        {
                            csRef<iDocumentNode> node3 = nodeItr3->Next();
                            CS::Threading::MutexScopedLock lock(mLock);
                            csRef<Material> material = materials.Get(node3->GetContentsValue(), csRef<Material>());
                            {
                                // Validation.
                                csString msg;
                                msg.Format("Invalid material reference '%s' in terrain materialpalette", node3->GetContentsValue());
                                CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                            }

                            m->materials.Push(material);
                        }
                    }

                    if(node2->GetNode("cells"))
                    {
                        nodeItr3 = node2->GetNode("cells")->GetNodes("cell");
                        while(nodeItr3->HasNext())
                        {
                            node2 = nodeItr3->Next();
                            if(node2->GetNode("renderproperties"))
                            {
                                csRef<iDocumentNodeIterator> nodeItr4 = node2->GetNode("renderproperties")->GetNodes("shadervar");
                                while(nodeItr4->HasNext())
                                {
                                    csRef<iDocumentNode> node3 = nodeItr4->Next();
                                    if(csString("texture").Compare(node3->GetAttributeValue("type")))
                                    {
                                        CS::Threading::MutexScopedLock lock(tLock);
                                        csRef<Texture> texture = textures.Get(node3->GetContentsValue(), csRef<Texture>());
                                        {
                                            // Validation.
                                            csString msg;
                                            msg.Format("Invalid texture reference '%s' in terrain renderproperties shadervar", node3->GetContentsValue());
                                            CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                                        }

                                        m->textures.Push(texture);
                                    }
                                }
                            }
                        }
                    }

                    s->meshes.Push(m);
                }

                nodeItr2 = node->GetNodes("portals");
                while(nodeItr2->HasNext())
                {
                    csRef<iDocumentNodeIterator> nodeItr3 = nodeItr2->Next()->GetNodes("portal");
                    while(nodeItr3->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr3->Next();
                        csRef<Portal> p = csPtr<Portal>(new Portal(node2->GetAttributeValue("name")));

                        if(node2->GetNode("matrix"))
                        {
                            p->warp = true;
                            syntaxService->ParseMatrix(node2->GetNode("matrix"), p->matrix);
                        }

                        if(node2->GetNode("wv"))
                        {
                            p->warp = true;
                            syntaxService->ParseVector(node2->GetNode("wv"), p->wv);
                        }

                        if(node2->GetNode("ww"))
                        {
                            p->warp = true;
                            p->ww_given = true;
                            syntaxService->ParseVector(node2->GetNode("ww"), p->ww);
                        }

                        if(node2->GetNode("clip"))
                        {
                            p->clip = true;
                        }

                        if(node2->GetNode("zfill"))
                        {
                            p->zfill = true;
                        }

                        csRef<iDocumentNodeIterator> nodeItr3 = node2->GetNodes("v");
                        while(nodeItr3->HasNext())
                        {
                            csVector3 vec;
                            syntaxService->ParseVector(nodeItr3->Next(), vec);
                            p->poly.AddVertex(vec);
                            p->bbox.AddBoundingVertex(vec);
                        }

                        csString targetSector = node2->GetNode("sector")->GetContentsValue();
                        {
                            CS::Threading::MutexScopedLock lock(sLock);
                            for(size_t i=0; i<sectors.GetSize(); i++)
                            {
                                if(targetSector.CompareNoCase(sectors[i]->name))
                                {
                                    p->targetSector = sectors[i];
                                    break;
                                }
                            }
                        }

                        if(!p->targetSector.IsValid())
                        {
                            p->targetSector = csPtr<Sector>(new Sector(targetSector));
                            CS::Threading::MutexScopedLock lock(sLock);
                            sectors.Push(p->targetSector);
                        }

                        if(!p->ww_given)
                        {
                            p->ww = p->wv;
                        }
                        p->transform = p->ww - p->matrix * p->wv;
                        s->portals.Push(p);
                    }
                }

                nodeItr2 = node->GetNodes("light");
                while(nodeItr2->HasNext())
                {
                    node = nodeItr2->Next();
                    csRef<Light> l = csPtr<Light>(new Light(node->GetAttributeValue("name")));

                    if(node->GetNode("attenuation"))
                    {
                        node = node->GetNode("attenuation");
                        if(csString("none").Compare(node->GetContentsValue()))
                        {
                            l->attenuation = CS_ATTN_NONE;
                        }
                        else if(csString("linear").Compare(node->GetContentsValue()))
                        {
                            l->attenuation = CS_ATTN_LINEAR;
                        }
                        else if(csString("inverse").Compare(node->GetContentsValue()))
                        {
                            l->attenuation = CS_ATTN_INVERSE;
                        }
                        else if(csString("realistic").Compare(node->GetContentsValue()))
                        {
                            l->attenuation = CS_ATTN_REALISTIC;
                        }
                        else if(csString("clq").Compare(node->GetContentsValue()))
                        {
                            l->attenuation = CS_ATTN_CLQ;
                        }

                        node = node->GetParent();
                    }
                    else
                    {
                        l->attenuation = CS_ATTN_LINEAR;
                    }

                    if(node->GetNode("dynamic"))
                    {
                        l->dynamic = CS_LIGHT_DYNAMICTYPE_PSEUDO;
                    }
                    else
                    {
                        l->dynamic = CS_LIGHT_DYNAMICTYPE_STATIC;
                    }

                    l->type = CS_LIGHT_POINTLIGHT;

                    syntaxService->ParseVector(node->GetNode("center"), l->pos);
                    l->radius = node->GetNode("radius")->GetContentsValueAsFloat();
                    syntaxService->ParseColor(node->GetNode("color"), l->colour);

                    l->bbox.AddBoundingVertex(l->pos.x - l->radius, l->pos.y - l->radius, l->pos.z - l->radius);
                    l->bbox.AddBoundingVertex(l->pos.x + l->radius, l->pos.y + l->radius, l->pos.z + l->radius);

                    s->lights.Push(l);
                    node = node->GetParent();
                }
            }
        }
    }

    return true;
}

void Loader::UpdatePosition(const csVector3& pos, const char* sectorName, bool force)
{
    validPosition = true;

    // Check already loading meshes.
    for(size_t i=0; i<(loadingMeshes.GetSize() < 50 ? loadingMeshes.GetSize() : 50); ++i)
    {
        if(LoadMesh(loadingMeshes[i]))
        {
          finalisableMeshes.Push(loadingMeshes[i]);
          loadingMeshes.DeleteIndex(i);
        }
    }

    // Finalise loaded meshes (expensive, so limited per check).
    for(size_t i=0; i<(finalisableMeshes.GetSize() < 10 ? finalisableMeshes.GetSize() : 10); ++i)
    {
      FinishMeshLoad(finalisableMeshes[i]);
      finalisableMeshes.DeleteIndex(i);
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

    for(size_t i=0; !sector.IsValid() && i<sectors.GetSize(); i++)
    {
        if(sectors[i]->name.Compare(sectorName))
        {
            sector = sectors[i];
            break;
        }
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
        LoadSector(pos, loadBox, unloadBox, sector);

        if(force)
        {
          // Make sure we start the loading now.
          for(size_t i=0; i<loadingMeshes.GetSize(); i++)
          {
            if(LoadMesh(loadingMeshes[i]))
            {
              FinishMeshLoad(loadingMeshes[i]);
              loadingMeshes.DeleteIndex(i);
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

void Loader::CleanDisconnectedSectors(Sector* sector)
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

void Loader::FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector)
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

void Loader::CleanSector(Sector* sector)
{
    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(sector->meshes[i]->object.IsValid())
        {
            sector->meshes[i]->object->GetMovable()->ClearSectors();
            sector->meshes[i]->object->GetMovable()->UpdateMove();
            engine->GetMeshes()->Remove(sector->meshes[i]->object);
            sector->meshes[i]->object.Invalidate();
            --sector->objectCount;
        }
    }

    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(sector->portals[i]->mObject.IsValid())
        {
            engine->GetMeshes()->Remove(sector->portals[i]->mObject);
            sector->portals[i]->pObject = NULL;
            sector->portals[i]->mObject.Invalidate();
            sector->activePortals.Delete(sector->portals[i]);
            --sector->objectCount;
        }
    }

    for(size_t i=0; i<sector->lights.GetSize(); i++)
    {
        if(sector->lights[i]->object.IsValid())
        {
            engine->RemoveLight(sector->lights[i]->object);
            sector->lights[i]->object.Invalidate();
            --sector->objectCount;
        }
    }

    if(sector->objectCount != 0)
    {
        csString msg;
        msg.Format("Error cleaning sector. Sector still has %u objects!", sector->objectCount);
        CS_ASSERT_MSG(msg.GetData(), false);
    }
    CS_ASSERT_MSG("Error cleaning sector. Sector is invalid!", sector->object.IsValid());

    engine->GetSectors()->Remove(sector->object);
    sector->object.Invalidate();
}

void Loader::LoadSector(const csVector3& pos, const csBox3& loadBox, const csBox3& unloadBox, Sector* sector)
{
    sector->isLoading = true;

    if(!sector->object.IsValid())
    {
        sector->object = engine->CreateSector(sector->name);
        sector->object->SetDynamicAmbientLight(sector->ambient);
        sector->object->SetVisibilityCullerPlugin(sector->culler);
    }

    // Check other sectors linked to by active portals.
    for(size_t i=0; i<sector->activePortals.GetSize(); i++)
    {
        if(!sector->activePortals[i]->targetSector->isLoading && !sector->activePortals[i]->targetSector->checked)
        {
            csVector3 wwPos = pos;
            csBox3 wwLoadBox = loadBox;
            csBox3 wwUnloadBox = unloadBox;
            if(sector->activePortals[i]->warp)
            {
                csVector3& transform = sector->activePortals[i]->transform;
                wwPos -= transform;
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

            LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->activePortals[i]->targetSector);
        }
    }

    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(!sector->meshes[i]->loading)
        {
            if(sector->meshes[i]->InRange(pos, loadBox))
            {
                sector->meshes[i]->loading = true;
                loadingMeshes.Push(sector->meshes[i]);
                ++sector->objectCount;
            }
            else if(sector->meshes[i]->OutOfRange(pos, unloadBox))
            {
                sector->meshes[i]->object->GetMovable()->ClearSectors();
                sector->meshes[i]->object->GetMovable()->UpdateMove();
                engine->GetMeshes()->Remove(sector->meshes[i]->object);
                sector->meshes[i]->object.Invalidate();
                --sector->objectCount;
            }
        }
    }

    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(sector->portals[i]->InRange(loadBox))
        {
            if(!sector->portals[i]->targetSector->isLoading && !sector->portals[i]->targetSector->checked)
            {
                csVector3 wwPos = pos;
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->portals[i]->warp)
                {
                    csVector3& transform = sector->portals[i]->transform;
                    wwPos -= transform;
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
                LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector);
            }

            sector->portals[i]->mObject = engine->CreatePortal(sector->portals[i]->name, sector->object,
                csVector3(0), sector->portals[i]->targetSector->object, sector->portals[i]->poly.GetVertices(),
                (int)sector->portals[i]->poly.GetVertexCount(), sector->portals[i]->pObject);

            if(sector->portals[i]->warp)
            {
                sector->portals[i]->pObject->SetWarp(sector->portals[i]->matrix, sector->portals[i]->wv, sector->portals[i]->ww);
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
            ++sector->objectCount;
        }
        else if(sector->portals[i]->OutOfRange(unloadBox))
        {
            if(!sector->portals[i]->targetSector->isLoading)
            {
                csVector3 wwPos = pos;
                csBox3 wwLoadBox = loadBox;
                csBox3 wwUnloadBox = unloadBox;
                if(sector->portals[i]->warp)
                {
                    csVector3& transform = sector->portals[i]->transform;
                    wwPos -= transform;
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
                LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector);
            }

            engine->GetMeshes()->Remove(sector->portals[i]->mObject);
            sector->portals[i]->pObject = NULL;
            sector->portals[i]->mObject.Invalidate();
            sector->activePortals.Delete(sector->portals[i]);
            --sector->objectCount;
        }
    }

    for(size_t i=0; i<sector->lights.GetSize(); i++)
    {
        if(sector->lights[i]->InRange(loadBox))
        {
            sector->lights[i]->object = engine->CreateLight(sector->lights[i]->name, sector->lights[i]->pos,
                sector->lights[i]->radius, sector->lights[i]->colour, sector->lights[i]->dynamic);
            sector->lights[i]->object->SetAttenuationMode(sector->lights[i]->attenuation);
            sector->lights[i]->object->SetType(sector->lights[i]->type);
            sector->object->AddLight(sector->lights[i]->object);
            ++sector->objectCount;
        }
        else if(sector->lights[i]->OutOfRange(unloadBox))
        {
            engine->RemoveLight(sector->lights[i]->object);
            sector->lights[i]->object.Invalidate();
            --sector->objectCount;
        }
    }

    if(sector->objectCount == sector->alwaysLoadedCount && sector->object.IsValid())
    {
        for(size_t i=0; i<sector->meshes.GetSize(); i++)
        {
            if(sector->meshes[i]->alwaysLoaded)
            {
                sector->meshes[i]->object->GetMovable()->ClearSectors();
                sector->meshes[i]->object->GetMovable()->UpdateMove();
                engine->GetMeshes()->Remove(sector->meshes[i]->object);
                sector->meshes[i]->object.Invalidate();
                --sector->objectCount;
            }
        }

        engine->GetSectors()->Remove(sector->object);
        sector->object.Invalidate();
    }

    sector->checked = true;
    sector->isLoading = false;
}

void Loader::FinishMeshLoad(MeshObj* mesh)
{
  mesh->object = scfQueryInterface<iMeshWrapper>(mesh->status->GetResultRefPtr());
  engine->SyncEngineListsNow(tloader);
  mesh->object->GetMovable()->SetSector(mesh->sector->object);
  mesh->object->GetMovable()->UpdateMove();
  csColliderHelper::InitializeCollisionWrapper(cdsys, mesh->object);
  engine->PrecacheMesh(mesh->object);

  mesh->loading = false;
}

bool Loader::LoadMesh(MeshObj* mesh)
{
    bool ready = true;
    for(size_t i=0; i<mesh->meshfacts.GetSize(); i++)
    {
        ready &= LoadMeshFact(mesh->meshfacts[i]);
    }

    if(!ready)
      return false;

    for(size_t i=0; i<mesh->materials.GetSize(); i++)
    {
        ready &= LoadMaterial(mesh->materials[i]);
    }

    if(!ready)
      return false;

    for(size_t i=0; i<mesh->textures.GetSize(); i++)
    {
        ready &= LoadTexture(mesh->textures[i]);
    }

    if(ready && !mesh->status)
    {
        mesh->status = tloader->LoadNode(vfs->GetCwd(), mesh->data);
    }

    return (mesh->status && mesh->status->IsFinished());
}

bool Loader::LoadMeshFact(MeshFact* meshfact)
{
    if(meshfact->loaded)
    {
        return true;
    }

    bool ready = true;
    for(size_t i=0; i<meshfact->materials.GetSize(); i++)
    {
        ready &= LoadMaterial(meshfact->materials[i]);
    }

    if(ready && !meshfact->status)
    {
        meshfact->status = tloader->LoadNode(vfs->GetCwd(), meshfact->data);
        return false;
    }

    if(meshfact->status && meshfact->status->IsFinished())
    {
        meshfact->loaded = true;
        return true;
    }

    return false;
}

bool Loader::LoadMaterial(Material* material)
{
    if(material->loaded)
    {
        return true;
    }

    bool ready = true;
    for(size_t i=0; i<material->textures.GetSize(); i++)
    {
        ready &= LoadTexture(material->textures[i]);
    }

    if(ready)
    {
        csArray<csStringID> shadertypes;
        csArray<iShader*> shaderptrs;
        csRefArray<csShaderVariable> shadervars;

        csRef<iMaterial> mat (engine->CreateBaseMaterial(0));
        engine->GetMaterialList()->NewMaterial(mat, material->name);

        for(size_t i=0; i<material->shaders.GetSize(); i++)
        {
            csRef<iShaderManager> shaderMgr = csQueryRegistry<iShaderManager> (object_reg);
            iShader* shader = shaderMgr->GetShader(material->shaders[i].name);
            csStringID type = strings->Request(material->shaders[i].type);
            mat->SetShader(type, shader);
        }

        for(size_t i=0; i<material->shadervars.GetSize(); i++)
        {
            if(material->shadervars[i].type == csShaderVariable::TEXTURE)
            {
                for(size_t j=0; j<material->textures.GetSize(); j++)
                {
                    if(material->textures[j]->name.Compare(material->shadervars[i].value))
                    {
                        csShaderVariable* var = mat->GetVariableAdd(svstrings->Request(material->shadervars[i].name));
                        var->SetType(material->shadervars[i].type);
                        csRef<iTextureWrapper> tex = scfQueryInterface<iTextureWrapper>(material->textures[j]->status->GetResultRefPtr());
                        var->SetValue(tex);
                        break;
                    }
                }
            }
            else if(material->shadervars[i].type == csShaderVariable::VECTOR2)
            {
                csShaderVariable* var = mat->GetVariableAdd(svstrings->Request(material->shadervars[i].name));
                var->SetType(material->shadervars[i].type);
                var->SetValue(material->shadervars[i].vec2);
            }
        }

        material->loaded = true;
        return true;
    }

    return false;
}

bool Loader::LoadTexture(Texture* texture)
{
    if(texture->loaded)
    {
        return true;
    }

    if(!texture->status.IsValid())
    {
        texture->status = tloader->LoadNode(vfs->GetCwd(), texture->data);
        return false;
    }

    if(!texture->status->IsFinished())
    {
        return false;
    }

    texture->loaded = true;
    return true;
}

csPtr<iMeshFactoryWrapper> Loader::LoadFactory(const char* name)
{
    csRef<MeshFact> meshfact = meshfacts.Get(name, csRef<MeshFact>());
    {
        // Validation.
        csString msg;
        msg.Format("Invalid factory reference '%s'", name);
        CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
    }

    if(LoadMeshFact(meshfact))
    {
        return scfQueryInterface<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());
    }

    return csPtr<iMeshFactoryWrapper>(0);
}

iMaterialWrapper* Loader::LoadMaterial(const char *name, const char *filename)
{
    iMaterialWrapper* materialWrap = engine->GetMaterialList()->FindByName(name);
    if(!materialWrap)
    {
        // Check that the texture exists.
        if(!vfs->Exists(filename))
            return NULL;

        // Load base texture.
        iTextureWrapper* texture = LoadTexture(name, filename);

        // Load base material.
        csRef<iMaterial> material (engine->CreateBaseMaterial(texture));
        materialWrap = engine->GetMaterialList()->NewMaterial(material, name);

        // Check for shader maps.
        if(gfxFeatures & useAdvancedShaders)
        {
            csString shadermapBase = filename;
            shadermapBase.Truncate(shadermapBase.Length()-4);

            // Normal map
            csString shadermap = shadermapBase;
            shadermap.Append("_n.dds");
            if(vfs->Exists(shadermap))
            {
                iTextureWrapper* t = LoadTexture(shadermap, shadermap, "normalmap");
                csShaderVariable* shadervar = new csShaderVariable();
                shadervar->SetName(svstrings->Request("tex normal compressed"));
                shadervar->SetValue(t);
                material->AddVariable(shadervar);
            }

            // Height map
            shadermap = shadermapBase;
            shadermap.Append("_h.dds");
            if(vfs->Exists(shadermap))
            {
                iTextureWrapper* t = LoadTexture(shadermap, shadermap);
                csShaderVariable* shadervar = new csShaderVariable();
                shadervar->SetName(svstrings->Request("tex height"));
                shadervar->SetValue(t);
                material->AddVariable(shadervar);
            }

            // Spec map
            shadermap = shadermapBase;
            shadermap.Append("_s.dds");
            if(vfs->Exists(shadermap))
            {
                iTextureWrapper* t = LoadTexture(shadermap, shadermap);
                csShaderVariable* shadervar = new csShaderVariable();
                shadervar->SetName(svstrings->Request("tex specular"));
                shadervar->SetValue(t);
                material->AddVariable(shadervar);
            }

            // Gloss map
            shadermap = shadermapBase;
            shadermap.Append("_g.dds");
            if(vfs->Exists(shadermap))
            {
                iTextureWrapper* t = LoadTexture(shadermap, shadermap);
                csShaderVariable* shadervar = new csShaderVariable();
                shadervar->SetName(svstrings->Request("tex gloss"));
                shadervar->SetValue(t);
                material->AddVariable(shadervar);
            }

            // AO map
            shadermap = shadermapBase;
            shadermap.Append("_ao.dds");
            if(vfs->Exists(shadermap))
            {
                iTextureWrapper* t = LoadTexture(shadermap, shadermap);
                csShaderVariable* shadervar = new csShaderVariable();
                shadervar->SetName(svstrings->Request("tex ambient occlusion"));
                shadervar->SetValue(t);
                material->AddVariable(shadervar);
            }
        }
    }
    return materialWrap;
}

iTextureWrapper* Loader::LoadTexture(const char *name, const char *filename, const char* className)
{
    // name is the material name; blah.dds
    // filename will be /planeshift/blah/blah.dds
    csString tempName;
    if(!name)
    {
        tempName = filename;
        size_t last = tempName.FindLast('/');
        tempName.DeleteAt(0, last+1);
        name = tempName.GetData();
    }

    csRef<iTextureWrapper> texture = engine->GetTextureList()->FindByName(name);

    if(!texture)
    {
        texture = loader->LoadTexture(name, filename, CS_TEXTURE_3D, txtmgr, true, false);
        if(className)
        {
            texture->SetTextureClass(className);
        }
        engine->SyncEngineListsNow(tloader);
    }

    if (!texture)
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
            "planeshift.engine.celbase",
            "Error loading texture '%s'!",
            name);
        return false;
    }
    return texture;
}
