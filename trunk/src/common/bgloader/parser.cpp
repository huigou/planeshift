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
    void BgLoader::ParseShaders()
    {
        if(parsedShaders)
            return;

        csRef<iConfigManager> config = csQueryRegistry<iConfigManager> (object_reg);

        // Parse basic set of shaders.
        csString shaderList = config->GetStr("PlaneShift.Loading.ShaderList", "/shader/shaderlist.xml");
        if(vfs->Exists(shaderList))
        {
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
            csRef<iDocument> doc = docsys->CreateDocument();
            csRef<iDataBuffer> data = vfs->ReadFile(shaderList);
            doc->Parse(data, true);

            if(doc->GetRoot())
            {
                csRef<iDocumentNode> node = doc->GetRoot()->GetNode("shaders");
                if(node.IsValid())
                {
                    csRefArray<iThreadReturn> rets;

                    csRef<iDocumentNodeIterator> nodeItr = node->GetNodes("shader");
                    while(nodeItr->HasNext())
                    {
                        node = nodeItr->Next();

                        csRef<iDocumentNode> file = node->GetNode("file");
                        shaders.Push(file->GetContentsValue());
                        rets.Push(tloader->LoadShader(vfs->GetCwd(), file->GetContentsValue()));

                        shadersByUsageType.Put(strings->Request(node->GetNode("type")->GetContentsValue()),
                            node->GetAttributeValue("name"));
                    }

                    // Wait for shader loads to finish.
                    tman->Wait(rets);
                }
            }
        }

        parsedShaders = true;
    }

    THREADED_CALLABLE_IMPL2(BgLoader, PrecacheData, const char* path, bool recursive)
    {
        // Make sure shaders are parsed at this point.
        ParseShaders();

        // Don't parse folders.
        csString vfsPath(path);
        if(vfsPath.GetAt(vfsPath.Length()-1) == '/')
            return false;

        if(vfs->Exists(vfsPath))
        {
            // For the plugin and shader loads.
            csRefArray<iThreadReturn> rets;

            // Zone for this file.
            csRef<Zone> zone;

            // Lights and sequences in this file (for sequences and triggers).
            csHash<Light*, csStringID> lights;
            csHash<Sequence*, csStringID> sequences;

            // Restores any directory changes.
            csVfsDirectoryChanger dirchange(vfs);

            // XML doc structures.
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
            csRef<iDocument> doc = docsys->CreateDocument();
            csRef<iDataBuffer> data = vfs->ReadFile(path);
            if(!data.IsValid())
                return false;

            doc->Parse(data, true);

            // Check that it's an xml file.
            if(!doc->GetRoot())
                return false;

            if(!recursive)
            {
                dirchange.ChangeTo(vfsPath.Truncate(vfsPath.FindLast('/')+1));
            }
            else
            {
                vfsPath = vfs->GetCwd();
            }

            // Begin document parsing.
            bool realRoot = false;
            csRef<iDocumentNode> root = doc->GetRoot()->GetNode("world");
            if(!root.IsValid())
            {
                root = doc->GetRoot()->GetNode("library");
                if(!root)
                {
                    realRoot = true;
                    root = doc->GetRoot();
                }
            }
            else
            {
                csString zonen(path);
                zonen = zonen.Slice(zonen.FindLast('/')+1);
                zone = csPtr<Zone>(new Zone(zonen));
                CS::Threading::ScopedWriteLock lock(zLock);
                zones.Put(zStringSet.Request(zonen.GetData()), zone);
            }

            if(root.IsValid())
            {
                csRef<iDocumentNode> node;
                csRef<iDocumentNodeIterator> nodeItr;

                // Parse referenced libraries.
                nodeItr = root->GetNodes("library");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();
                    PrecacheDataTC(ret, false, node->GetContentsValue(), true);
                }

                // Parse needed plugins.
                node = root->GetNode("plugins");
                if(node.IsValid())
                {
                    rets.Push(tloader->LoadNode(vfs->GetCwd(), node));
                }

                // Parse referenced shaders.
                node = root->GetNode("shaders");
                if(node.IsValid())
                {
                    nodeItr = node->GetNodes("shader");
                    while(nodeItr->HasNext())
                    {
                        bool loadShader = false;
                        node = nodeItr->Next();
                        node = node->GetNode("file");

                        {
                            // Keep track of shaders that have already been parsed.
                            CS::Threading::ScopedWriteLock lock(sLock);
                            if(shaders.Contains(node->GetContentsValue()) == csArrayItemNotFound)
                            {
                                shaders.Push(node->GetContentsValue());
                                loadShader = true;
                            }
                        }

                        if(loadShader)
                        {
                            // Dispatch shader load to a thread.
                            rets.Push(tloader->LoadShader(vfs->GetCwd(), node->GetContentsValue()));
                        }
                    }
                }

                // Parse all referenced textures.
                node = root->GetNode("textures");
                if(node.IsValid())
                {
                    nodeItr = node->GetNodes("texture");
                    while(nodeItr->HasNext())
                    {
                        node = nodeItr->Next();
                        csRef<Texture> t = csPtr<Texture>(new Texture(node->GetAttributeValue("name"), vfsPath, node));
                        {
                            CS::Threading::ScopedWriteLock lock(tLock);
                            textures.Put(tStringSet.Request(t->name), t);
                        }
                    }
                }

                // Parse all referenced materials.
                node = root->GetNode("materials");
                if(node.IsValid())
                {
                    nodeItr = node->GetNodes("material");
                    while(nodeItr->HasNext())
                    {
                        node = nodeItr->Next();
                        csRef<Material> m = csPtr<Material>(new Material(node->GetAttributeValue("name")));
                        {
                            CS::Threading::ScopedWriteLock lock(mLock);
                            materials.Put(mStringSet.Request(m->name), m);
                        }

                        // Parse the texture for a material. Construct a shader variable for it.
                        if(node->GetNode("texture"))
                        {
                            node = node->GetNode("texture");
                            ShaderVar sv("tex diffuse", csShaderVariable::TEXTURE);
                            sv.value = node->GetContentsValue();
                            m->shadervars.Push(sv);

                            csRef<Texture> texture;
                            {
                                CS::Threading::ScopedReadLock lock(tLock);
                                texture = textures.Get(tStringSet.Request(node->GetContentsValue()), csRef<Texture>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid texture reference '%s' in material '%s'", node->GetContentsValue(), node->GetParent()->GetAttributeValue("name"));
                                CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                            }
                            m->textures.Push(texture);
                            m->checked.Push(false);

                            node = node->GetParent();
                        }

                        // Parse the shaders attached to this material.
                        csRef<iDocumentNodeIterator> nodeItr2 = node->GetNodes("shader");
                        while(nodeItr2->HasNext())
                        {
                            node = nodeItr2->Next();
                            m->shaders.Push(Shader(node->GetAttributeValue("type"), node->GetContentsValue()));
                            node = node->GetParent();
                        }

                        // Parse the shader variables attached to this material.
                        nodeItr2 = node->GetNodes("shadervar");
                        while(nodeItr2->HasNext())
                        {
                            node = nodeItr2->Next();

                            // Parse the different types. Currently texture, vector2 and vector3 are supported.
                            if(csString("texture").Compare(node->GetAttributeValue("type")))
                            {
                                // Ignore some shader variables if the functionality they bring is not enabled.
                                if(enabledGfxFeatures & (useHighShaders | useMediumShaders | useLowShaders | useLowestShaders))
                                {
                                    if(!strcmp(node->GetAttributeValue("name"), "tex height") ||
                                        !strcmp(node->GetAttributeValue("name"), "tex ambient occlusion"))
                                    {
                                        continue;
                                    }

                                    if(enabledGfxFeatures & (useMediumShaders | useLowShaders | useLowestShaders))
                                    {
                                        if(!strcmp(node->GetAttributeValue("name"), "tex specular"))
                                        {
                                            continue;
                                        }
                                    }

                                    if(enabledGfxFeatures & (useLowShaders | useLowestShaders))
                                    {
                                        if(!strcmp(node->GetAttributeValue("name"), "tex normal") ||
                                            !strcmp(node->GetAttributeValue("name"), "tex normal compressed"))
                                        {
                                            continue;
                                        }
                                    }
                                }

                                ShaderVar sv(node->GetAttributeValue("name"), csShaderVariable::TEXTURE);
                                sv.value = node->GetContentsValue();
                                m->shadervars.Push(sv);
                                csRef<Texture> texture;
                                {
                                    CS::Threading::ScopedReadLock lock(tLock);
                                    texture = textures.Get(tStringSet.Request(node->GetContentsValue()), csRef<Texture>());

                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid texture reference '%s' in shadervar in material '%s'.",
                                        node->GetContentsValue(), m->name.GetData());
                                    CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                                }
                                m->textures.Push(texture);
                                m->checked.Push(false);
                            }
                            else if(csString("vector2").Compare(node->GetAttributeValue("type")))
                            {
                                ShaderVar sv(node->GetAttributeValue("name"), csShaderVariable::VECTOR2);
                                csScanStr (node->GetContentsValue(), "%f,%f", &sv.vec2.x, &sv.vec2.y);
                                m->shadervars.Push(sv);
                            }
                            else if(csString("vector3").Compare(node->GetAttributeValue("type")))
                            {
                                ShaderVar sv(node->GetAttributeValue("name"), csShaderVariable::VECTOR3);
                                csScanStr (node->GetContentsValue(), "%f,%f,%f", &sv.vec3.x, &sv.vec3.y, &sv.vec3.z);
                                m->shadervars.Push(sv);
                            }
                            node = node->GetParent();
                        }
                    }
                }

                // Parse all mesh factories.
                bool once = false;
                nodeItr = root->GetNodes("meshfact");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();
                    csRef<MeshFact> mf = csPtr<MeshFact>(new MeshFact(node->GetAttributeValue("name"), vfsPath, node));

                    if(!cache)
                    {
                        if(realRoot && !once && !nodeItr->HasNext())
                        {
                            // Load this file when needed to save memory.
                            mf->data.Invalidate();
                            mf->filename = csString(path).Slice(csString(path).FindLast('/')+1);
                        }

                        // Mark that we've already loaded a meshfact in this file.
                        once = true;
                    }

                    // Read bbox data.
                    csRef<iDocumentNode> cell = node->GetNode("params")->GetNode("cells");
                    if(cell.IsValid())
                    {
                        cell = cell->GetNode("celldefault");
                        if(cell.IsValid())
                        {
                            cell = cell->GetNode("size");
                            mf->bboxvs.Push(csVector3(-1*cell->GetAttributeValueAsInt("x")/2,
                                0, -1*cell->GetAttributeValueAsInt("z")/2));
                            mf->bboxvs.Push(csVector3(cell->GetAttributeValueAsInt("x")/2,
                                cell->GetAttributeValueAsInt("y"),
                                cell->GetAttributeValueAsInt("z")/2));
                        }
                    }
                    else
                    {
                        csRef<iDocumentNodeIterator> keys = node->GetNodes("key");
                        while(keys->HasNext())
                        {
                            csRef<iDocumentNode> bboxdata = keys->Next();
                            if(csString("bbox").Compare(bboxdata->GetAttributeValue("name")))
                            {
                                csRef<iDocumentNodeIterator> vs = bboxdata->GetNodes("v");
                                while(vs->HasNext())
                                {
                                    bboxdata = vs->Next();
                                    csVector3 vtex;
                                    syntaxService->ParseVector(bboxdata, vtex);
                                    mf->bboxvs.Push(vtex);
                                }
                                break;
                            }
                        }
                    }

                    // Parse mesh params to get the materials that we depend on.
                    if(node->GetNode("params")->GetNode("material"))
                    {
                        csRef<Material> material;
                        {
                            CS::Threading::ScopedReadLock lock(mLock);
                            material = materials.Get(mStringSet.Request(node->GetNode("params")->GetNode("material")->GetContentsValue()), csRef<Material>());

                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in meshfact '%s'", node->GetNode("params")->GetNode("material")->GetContentsValue(), node->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        mf->materials.Push(material);
                        mf->checked.Push(false);
                    }

                    csRef<iDocumentNodeIterator> nodeItr3 = node->GetNode("params")->GetNodes("submesh");
                    while(nodeItr3->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr3->Next();
                        if(node2->GetNode("material"))
                        {
                            csRef<Material> material;
                            {
                                CS::Threading::ScopedReadLock lock(mLock);
                                material = materials.Get(mStringSet.Request(node2->GetNode("material")->GetContentsValue()), csRef<Material>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid material reference '%s' in meshfact '%s'", node2->GetNode("material")->GetContentsValue(), node->GetAttributeValue("name"));
                                CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                            }

                            mf->materials.Push(material);
                            mf->checked.Push(false);
                            mf->submeshes.Push(node2->GetAttributeValue("name"));
                        }
                    }

                    nodeItr3 = node->GetNode("params")->GetNodes("mesh");
                    while(nodeItr3->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr3->Next();
                        csRef<Material> material;
                        {
                            CS::Threading::ScopedReadLock lock(mLock);
                            material = materials.Get(mStringSet.Request(node2->GetAttributeValue("material")), csRef<Material>());

                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in cal3d meshfact '%s' mesh '%s'",
                                node2->GetAttributeValue("material"), node->GetAttributeValue("name"), node2->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }
                        mf->materials.Push(material);
                        mf->checked.Push(false);
                    }

                    // Parse terrain cells for materials.
                    if(node->GetNode("params")->GetNode("cells"))
                    {
                        node = node->GetNode("params")->GetNode("cells")->GetNode("celldefault")->GetNode("basematerial");
                        {
                            csRef<Material> material;
                            {    
                                CS::Threading::ScopedReadLock lock(mLock);
                                material = materials.Get(mStringSet.Request(node->GetContentsValue()), csRef<Material>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid basematerial reference '%s' in terrain mesh", node->GetContentsValue());
                                CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                            }

                            mf->materials.Push(material);
                            mf->checked.Push(false);
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
                                    csRef<Material> material;
                                    {
                                        CS::Threading::ScopedReadLock lock(mLock);
                                        material = materials.Get(mStringSet.Request(node2->GetAttributeValue("material")), csRef<Material>());

                                        // Validation.
                                        csString msg;
                                        msg.Format("Invalid alphamap reference '%s' in terrain mesh", node2->GetAttributeValue("material"));
                                        CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                                    }

                                    mf->materials.Push(material);
                                    mf->checked.Push(false);
                                }
                            }
                        }
                    }

                    CS::Threading::ScopedWriteLock lock(mfLock);
                    meshfacts.Put(mfStringSet.Request(mf->name), mf);
                }

                // Parse all sectors.
                nodeItr = root->GetNodes("sector");
                while(nodeItr->HasNext())
                {
                    node = nodeItr->Next();

                    csRef<Sector> s;
                    csString sectorName = node->GetAttributeValue("name");
                    {
                        CS::Threading::ScopedReadLock lock(sLock);
                        s = sectorHash.Get(sStringSet.Request(sectorName), csRef<Sector>());
                    }

                    // This sector may have already been created (referenced by a portal somewhere else).
                    if(!s.IsValid())
                    {
                        // But if not then create its representation.
                        s = csPtr<Sector>(new Sector(sectorName));
                        CS::Threading::ScopedWriteLock lock(sLock);
                        sectors.Push(s);
                        sectorHash.Put(sStringSet.Request(sectorName), s);
                    }

                    if(zone.IsValid())
                    {
                        s->parent = zone;
                        zone->sectors.Push(s);
                    }

                    // Get culler properties.
                    s->init = true;
                    s->culler = node->GetNode("cullerp")->GetContentsValue();

                    // Get ambient lighting.
                    if(node->GetNode("ambient"))
                    {
                        node = node->GetNode("ambient");
                        s->ambient = csColor(node->GetAttributeValueAsFloat("red"),
                            node->GetAttributeValueAsFloat("green"), node->GetAttributeValueAsFloat("blue"));
                        node = node->GetParent();
                    }

                    // Get water bodies in this sector.
                    csRef<iDocumentNodeIterator> nodeItr2 = node->GetNodes("key");
                    while(nodeItr2->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr2->Next();
                        if(csString("water").Compare(node2->GetAttributeValue("name")))
                        {
                            csRef<iDocumentNodeIterator> nodeItr3 = node2->GetNodes("area");
                            while(nodeItr3->HasNext())
                            {
                                csRef<iDocumentNode> area = nodeItr3->Next();
                                WaterArea* wa = new WaterArea();
                                s->waterareas.Push(wa);

                                csRef<iDocumentNode> colour = area->GetNode("colour");
                                if(colour.IsValid())
                                {
                                    syntaxService->ParseColor(colour, wa->colour);
                                }
                                else
                                {
                                    // Default.
                                    wa->colour = csColor4(0.0f, 0.17f, 0.49f, 0.6f);
                                }

                                csRef<iDocumentNodeIterator> vs = area->GetNodes("v");
                                while(vs->HasNext())
                                {
                                    csRef<iDocumentNode> v = vs->Next();
                                    csVector3 vector;
                                    syntaxService->ParseVector(v, vector);
                                    wa->bbox.AddBoundingVertex(vector);
                                }
                            }
                        }
                    }

                    // Get all mesh instances in this sector.
                    nodeItr2 = node->GetNodes("meshobj");
                    while(nodeItr2->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr2->Next();
                        csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(node2->GetAttributeValue("name"), vfsPath, node2));
                        m->sector = s;

                        // Get the position data for later.
                        csRef<iDocumentNode> position = node2->GetNode("move");

                        // Check for a params file and switch to use it to continue parsing.
                        if(node2->GetNode("paramsfile"))
                        {
                            csRef<iDocument> pdoc = docsys->CreateDocument();
                            csRef<iDataBuffer> pdata = vfs->ReadFile(node2->GetNode("paramsfile")->GetContentsValue());
                            CS_ASSERT_MSG("Invalid params file.\n", pdata.IsValid());
                            pdoc->Parse(pdata, true);
                            node2 = pdoc->GetRoot();
                        }

                        // Parse all materials and shader variables this mesh depends on.
                        csRef<iDocumentNodeIterator> nodeItr3 = node2->GetNode("params")->GetNodes("submesh");
                        while(nodeItr3->HasNext())
                        {
                            csRef<iDocumentNode> node3 = nodeItr3->Next();
                            if(node3->GetNode("material"))
                            {
                                csRef<Material> material;
                                {
                                    CS::Threading::ScopedReadLock lock(mLock);
                                    material = materials.Get(mStringSet.Request(node3->GetNode("material")->GetContentsValue()), csRef<Material>());

                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid material reference '%s' in meshobj '%s' submesh in sector '%s'", node3->GetNode("material")->GetContentsValue(), m->name.GetData(), s->name.GetData());
                                    CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                                }

                                m->materials.Push(material);
                                m->matchecked.Push(false);
                            }

                            csRef<iDocumentNodeIterator> nodeItr4 = node3->GetNodes("shadervar");
                            while(nodeItr4->HasNext())
                            {
                                node3 = nodeItr4->Next();
                                if(csString("texture").Compare(node3->GetAttributeValue("type")))
                                {
                                    csRef<Texture> texture;
                                    {
                                        CS::Threading::ScopedReadLock lock(tLock);
                                        texture = textures.Get(tStringSet.Request(node3->GetContentsValue()), csRef<Texture>());

                                        // Validation.
                                        csString msg;
                                        msg.Format("Invalid texture reference '%s' in meshobj shadervar", node3->GetContentsValue());
                                        CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                                    }

                                    m->textures.Push(texture);
                                    m->texchecked.Push(false);
                                }
                            }
                        }

                        node2 = node2->GetNode("params")->GetNode("factory");
                        {
                            csRef<MeshFact> meshfact;
                            {
                                CS::Threading::ScopedReadLock lock(mfLock);
                                meshfact = meshfacts.Get(mfStringSet.Request(node2->GetContentsValue()), csRef<MeshFact>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid factory reference '%s' in meshobj '%s' in sector '%s'", node2->GetContentsValue(),
                                    node2->GetParent()->GetParent()->GetAttributeValue("name"), s->name.GetData());
                                CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
                            }

                            if(meshfact.IsValid())
                            {
                                // Calc bbox data.
                                if(position.IsValid())
                                {
                                    csVector3 pos;
                                    syntaxService->ParseVector(position->GetNode("v"), pos);

                                    csMatrix3 rot;
                                    if(position->GetNode("matrix"))
                                    {
                                        syntaxService->ParseMatrix(position->GetNode("matrix"), rot);
                                    }

                                    for(size_t v=0; v<meshfact->bboxvs.GetSize(); ++v)
                                    {
                                        if(position->GetNode("matrix"))
                                        {
                                            m->bbox.AddBoundingVertex(rot*csVector3(pos+meshfact->bboxvs[v]));
                                        }
                                        else
                                        {
                                            m->bbox.AddBoundingVertex(pos+meshfact->bboxvs[v]);
                                        }
                                    }
                                }

                                m->meshfacts.Push(meshfact);
                                m->mftchecked.Push(false);

                                // Validation.
                                nodeItr3 = node2->GetParent()->GetNodes("submesh");
                                while(nodeItr3->HasNext())
                                {
                                    csRef<iDocumentNode> submesh = nodeItr3->Next();
                                    if(meshfact->submeshes.Find(submesh->GetAttributeValue("name")) == csArrayItemNotFound)
                                    {
                                        csString msg;
                                        msg.Format("Invalid submesh reference '%s' in meshobj '%s' in sector '%s'", submesh->GetAttributeValue("name"),
                                            m->name.GetData(), s->name.GetData());
                                        CS_ASSERT_MSG(msg.GetData(), false);
                                    }
                                }
                            }
                        }
                        node2 = node2->GetParent();

                        // Continue material parsing.
                        if(node2->GetNode("material"))
                        {
                            node2 = node2->GetNode("material");

                            csRef<Material> material;
                            {
                                CS::Threading::ScopedReadLock lock(mLock);
                                material = materials.Get(mStringSet.Request(node2->GetContentsValue()), csRef<Material>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid material reference '%s' in terrain '%s' object in sector '%s'", node2->GetContentsValue(), m->name.GetData(), s->name.GetData());
                                CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                            }

                            m->materials.Push(material);
                            m->matchecked.Push(false);
                            node2 = node2->GetParent();
                        }

                        // materialpalette for terrain.
                        if(node2->GetNode("materialpalette"))
                        {
                            nodeItr3 = node2->GetNode("materialpalette")->GetNodes("material");
                            while(nodeItr3->HasNext())
                            {
                                csRef<iDocumentNode> node3 = nodeItr3->Next();
                                csRef<Material> material;
                                {
                                    CS::Threading::ScopedReadLock lock(mLock);
                                    material = materials.Get(mStringSet.Request(node3->GetContentsValue()), csRef<Material>());

                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid material reference '%s' in terrain materialpalette", node3->GetContentsValue());
                                    CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                                }

                                m->materials.Push(material);
                                m->matchecked.Push(false);
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
                                            csRef<Texture> texture;
                                            {
                                                CS::Threading::ScopedReadLock lock(tLock);
                                                texture = textures.Get(tStringSet.Request(node3->GetContentsValue()), csRef<Texture>());

                                                // Validation.
                                                csString msg;
                                                msg.Format("Invalid texture reference '%s' in terrain renderproperties shadervar", node3->GetContentsValue());
                                                CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                                            }

                                            m->textures.Push(texture);
                                            m->texchecked.Push(false);
                                        }
                                    }
                                }
                            }
                        }

                        // alwaysloaded ignores range checks. If the sector is loaded then so is this mesh.
                        if(node2->GetAttributeValueAsBool("alwaysloaded"))
                        {
                            s->alwaysLoaded.Push(m);
                        }
                        else
                        {
                            s->meshes.Push(m);
                        }
                        CS::Threading::ScopedWriteLock lock(meshLock);
                        meshes.Put(meshStringSet.Request(m->name), m);
                    }

                    // Get all trimeshes in this sector.
                    nodeItr2 = node->GetNodes("trimesh");
                    while(nodeItr2->HasNext())
                    {
                        csRef<iDocumentNode> node2 = nodeItr2->Next();
                        csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(node2->GetAttributeValue("name"), vfsPath, node2));
                        m->sector = s;

                        // Always load trimesh... for now. TODO: Calculate bbox.
                        // Push to sector.
                        s->alwaysLoaded.Push(m);
                        CS::Threading::ScopedWriteLock lock(meshLock);
                        meshes.Put(meshStringSet.Request(m->name), m);
                    }

                    // Parse mesh generators (for foliage, rocks etc.)
                    if(enabledGfxFeatures & useMeshGen)
                    {
                        nodeItr2 = node->GetNodes("meshgen");
                        while(nodeItr2->HasNext())
                        {
                            csRef<iDocumentNode> meshgen = nodeItr2->Next();
                            csRef<MeshGen> mgen = csPtr<MeshGen>(new MeshGen(meshgen->GetAttributeValue("name"), meshgen));

                            mgen->sector = s;
                            s->meshgen.Push(mgen);

                            meshgen = meshgen->GetNode("samplebox");
                            {
                                csVector3 min;
                                csVector3 max;

                                syntaxService->ParseVector(meshgen->GetNode("min"), min);
                                syntaxService->ParseVector(meshgen->GetNode("max"), max);
                                mgen->bbox.AddBoundingVertex(min);
                                mgen->bbox.AddBoundingVertex(max);
                            }
                            meshgen = meshgen->GetParent();

                            {
                                CS::Threading::ScopedReadLock lock(meshLock);
                                csStringID mID = meshStringSet.Request(meshgen->GetNode("meshobj")->GetContentsValue());
                                mgen->object = meshes.Get(mID, csRef<MeshObj>());
                            }

                            csRef<iDocumentNodeIterator> geometries = meshgen->GetNodes("geometry");
                            while(geometries->HasNext())
                            {
                                csRef<iDocumentNode> geometry = geometries->Next();

                                csRef<MeshFact> meshfact;
                                {
                                    csString name(geometry->GetNode("factory")->GetAttributeValue("name"));
                                    CS::Threading::ScopedReadLock lock(mfLock);
                                    meshfact = meshfacts.Get(mfStringSet.Request(name), csRef<MeshFact>());

                                    // Validation.
                                    csString msg;
                                    msg.Format("Invalid meshfact reference '%s' in meshgen '%s'", name.GetData(), mgen->name.GetData());
                                    CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
                                }

                                mgen->meshfacts.Push(meshfact);   
                                mgen->mftchecked.Push(false);

                                csRef<iDocumentNodeIterator> matfactors = geometry->GetNodes("materialfactor");
                                while(matfactors->HasNext())
                                {
                                    csRef<Material> material;
                                    {
                                        csString name(matfactors->Next()->GetAttributeValue("material"));
                                        CS::Threading::ScopedReadLock lock(mLock);
                                        material = materials.Get(mStringSet.Request(name), csRef<Material>());

                                        // Validation.
                                        csString msg;
                                        msg.Format("Invalid material reference '%s' in meshgen '%s'", name.GetData(), mgen->name.GetData());
                                        CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                                    }

                                    mgen->materials.Push(material);
                                    mgen->matchecked.Push(false);
                                }
                            }
                        }
                    }

                    // Parse all portals.
                    nodeItr2 = node->GetNodes("portals");
                    while(nodeItr2->HasNext())
                    {
                        csRef<iDocumentNodeIterator> nodeItr3 = nodeItr2->Next()->GetNodes("portal");
                        while(nodeItr3->HasNext())
                        {
                            csRef<iDocumentNode> node2 = nodeItr3->Next();
                            csRef<Portal> p = csPtr<Portal>(new Portal(node2->GetAttributeValue("name")));

                            // Warping
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

                            // Other options.
                            if(node2->GetNode("float"))
                            {
                                p->pfloat = true;
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
                                CS::Threading::ScopedReadLock lock(sLock);
                                p->targetSector = sectorHash.Get(sStringSet.Request(targetSector), csRef<Sector>());
                            }

                            if(!p->targetSector.IsValid())
                            {
                                p->targetSector = csPtr<Sector>(new Sector(targetSector));
                                CS::Threading::ScopedWriteLock lock(sLock);
                                sectors.Push(p->targetSector);
                                sectorHash.Put(sStringSet.Request(targetSector), p->targetSector);
                            }

                            if(!p->ww_given)
                            {
                                p->ww = p->wv;
                            }
                            p->transform = p->ww - p->matrix * p->wv;
                            s->portals.Push(p);
                        }
                    }

                    // Parse all sector lights.
                    nodeItr2 = node->GetNodes("light");
                    while(nodeItr2->HasNext())
                    {
                        node = nodeItr2->Next();
                        csRef<Light> l = csPtr<Light>(new Light(node->GetAttributeValue("name")));
                        {
                            CS::Threading::ScopedWriteLock lock(sLock);
                            lights.Put(sStringSet.Request(l->name), l);
                        }

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

                // Parse the start position.
                node = root->GetNode("start");
                if(node.IsValid())
                {
                    csRef<StartPosition> startPos = csPtr<StartPosition>(new StartPosition());
                    csString zonen(path);
                    zonen = zonen.Slice(zonen.FindLast('/')+1);
                    startPos->zone = zonen;
                    startPos->sector = node->GetNode("sector")->GetContentsValue();
                    syntaxService->ParseVector(node->GetNode("position"), startPos->position);
                    startPositions.Push(startPos);
                }

                node = root->GetNode("sequences");
                if(node.IsValid())
                {
                    nodeItr = node->GetNodes("sequence");
                    while(nodeItr->HasNext())
                    {
                        node = nodeItr->Next();
                        csRef<Sequence> seq = csPtr<Sequence>(new Sequence(node->GetAttributeValue("name"), node));
                        {
                            CS::Threading::ScopedWriteLock lock(sLock);
                            sequences.Put(sStringSet.Request(seq->name), seq);
                        }

                        bool loaded = false;
                        csRef<iDocumentNodeIterator> nodes = node->GetNodes("setambient");
                        if(nodes->HasNext())
                        {
                            csRef<iDocumentNode> type = nodes->Next();
                            CS::Threading::ScopedWriteLock lock(sLock);
                            csRef<Sector> sec = sectorHash.Get(sStringSet.Request(type->GetAttributeValue("sector")), csRef<Sector>());
                            sec->sequences.Push(seq);
                            loaded = true;
                        }

                        nodes = node->GetNodes("fadelight");
                        if(nodes->HasNext())
                        {
                            csRef<iDocumentNode> type = nodes->Next();
                            CS::Threading::ScopedWriteLock lock(sLock);
                            csRef<Light> l = lights.Get(sStringSet.Request(type->GetAttributeValue("light")), csRef<Light>());
                            l->sequences.Push(seq);
                            loaded = true;
                        }

                        nodes = node->GetNodes("rotate");
                        if(nodes->HasNext())
                        {
                            csRef<iDocumentNode> type = nodes->Next();
                            CS::Threading::ScopedWriteLock lock(meshLock);
                            csRef<MeshObj> l = meshes.Get(meshStringSet.Request(type->GetAttributeValue("mesh")), csRef<MeshObj>());
                            l->sequences.Push(seq);
                            loaded = true;
                        }

                        CS_ASSERT_MSG("Unknown sequence type!", loaded);
                    }
                }

                node = root->GetNode("triggers");
                if(node.IsValid())
                {
                    nodeItr = node->GetNodes("trigger");
                    while(nodeItr->HasNext())
                    {
                        node = nodeItr->Next();
                        const char* seqname = node->GetNode("fire")->GetAttributeValue("sequence");
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Sequence> sequence = sequences.Get(sStringSet.Request(seqname), csRef<Sequence>());
                        CS_ASSERT_MSG("Unknown sequence in trigger!", sequence.IsValid());

                        csRef<Trigger> trigger = csPtr<Trigger>(new Trigger(node->GetAttributeValue("name"), node));
                        sequence->triggers.Push(trigger);
                    }
                }
            }

            // Wait for plugin and shader loads to finish.
            tman->Wait(rets);
        }

        return true;
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)
