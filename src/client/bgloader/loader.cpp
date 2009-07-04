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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/stringarray.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivideo/material.h>

#include "loader.h"

CS_IMPLEMENT_PLUGIN

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
SCF_IMPLEMENT_FACTORY(BgLoader)

BgLoader::BgLoader(iBase *p)
  : scfImplementationType (this, p), validPosition(false)
{
}

BgLoader::~BgLoader()
{
}

bool BgLoader::Initialize(iObjectRegistry* object_reg)
{
    this->object_reg = object_reg;

    engine = csQueryRegistry<iEngine> (object_reg);
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

    return true;
}

void BgLoader::Setup(uint gfxFeatures, float loadRange)
{
    this->gfxFeatures = gfxFeatures;
    this->loadRange = loadRange;

    // Parse basic set of shaders.
    csRef<iConfigManager> config = csQueryRegistry<iConfigManager> (object_reg);
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

                    shadersByUsageType.Register(node->GetAttributeValue("name"),
                        strings->Request(node->GetNode("type")->GetContentsValue()));
                }

                // Wait for shader loads to finish.
                tman->Wait(rets);
            }
        }
    }
}

csStringArray BgLoader::GetShaderName(const char* usageType) const
{
    csStringArray t;
    csStringID id = strings->Request(usageType);
    csArray<const char*> all = shadersByUsageType.RequestAll(id);

    for(size_t i=0; i<all.GetSize(); ++i)
    {
        t.Push(all[i]);
    }

    return t;
}

THREADED_CALLABLE_IMPL2(BgLoader, PrecacheData, const char* path, bool recursive)
{
    // Don't parse folders.
    csString vfsPath(path);
    if(vfsPath.GetAt(vfsPath.Length()-1) == '/')
        return false;

    if(vfs->Exists(vfsPath))
    {
        // For the plugin and shader loads.
        csRefArray<iThreadReturn> rets;

        // Restores any directory changes.
        csVfsDirectoryChanger dirchange(vfs);

        // XML doc structures.
        csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
        csRef<iDocument> doc = docsys->CreateDocument();
        csRef<iDataBuffer> data = vfs->ReadFile(path);

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
              rets.Push(tloader->LoadNode(vfs->GetCwd(), node));
            }

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
                        CS::Threading::ScopedWriteLock lock(sLock);
                        if(shaders.Contains(node->GetContentsValue()) == csArrayItemNotFound)
                        {
                            shaders.Push(node->GetContentsValue());
                            loadShader = true;
                        }
                    }

                    if(loadShader)
                    {
                        rets.Push(tloader->LoadShader(vfs->GetCwd(), node->GetContentsValue()));
                    }
                }
            }

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
                        CS::Threading::ScopedWriteLock lock(mLock);
                        materials.Put(m->name, m);
                    }

                    if(node->GetNode("texture"))
                    {
                        node = node->GetNode("texture");
                        ShaderVar sv("tex diffuse", csShaderVariable::TEXTURE);
                        sv.value = node->GetContentsValue();
                        m->shadervars.Push(sv);

                        csRef<Texture> texture;
                        {
                            CS::Threading::ScopedReadLock lock(tLock);
                            texture = textures.Get(node->GetContentsValue(), csRef<Texture>());

                            // Validation.
                            csString msg;
                            msg.Format("Invalid texture reference '%s' in material '%s'", node->GetContentsValue(), node->GetParent()->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
                        }
                        m->textures.Push(texture);
                        m->checked.Push(false);

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
                            if(gfxFeatures & (useHighShaders | useMediumShaders | useLowShaders | useLowestShaders))
                            {
                                if(!strcmp(node->GetAttributeValue("name"), "tex height") ||
                                   !strcmp(node->GetAttributeValue("name"), "tex ambient occlusion"))
                                {
                                    continue;
                                }

                                if(gfxFeatures & (useMediumShaders | useLowShaders | useLowestShaders))
                                {
                                    if(!strcmp(node->GetAttributeValue("name"), "tex specular"))
                                    {
                                        continue;
                                    }
                                }

                                if(gfxFeatures & (useLowShaders | useLowestShaders))
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
                                texture = textures.Get(node->GetContentsValue(), csRef<Texture>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid texture reference '%s' in shadervar", node->GetContentsValue());
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
                        node = node->GetParent();
                    }
                }
            }

            nodeItr = root->GetNodes("meshfact");
            while(nodeItr->HasNext())
            {
                node = nodeItr->Next();
                csRef<MeshFact> mf = csPtr<MeshFact>(new MeshFact(node->GetAttributeValue("name"), vfsPath, node));

                if(node->GetNode("params")->GetNode("material"))
                {
                    csRef<Material> material;
                    {
                        CS::Threading::ScopedReadLock lock(mLock);
                        material = materials.Get(node->GetNode("params")->GetNode("material")->GetContentsValue(), csRef<Material>());

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
                            material = materials.Get(node2->GetNode("material")->GetContentsValue(), csRef<Material>());

                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in meshfact '%s'", node2->GetNode("material")->GetContentsValue(), node->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        mf->materials.Push(material);
                        mf->checked.Push(false);
                    }
                }

                nodeItr3 = node->GetNode("params")->GetNodes("mesh");
                while(nodeItr3->HasNext())
                {
                    csRef<iDocumentNode> node2 = nodeItr3->Next();
                    csRef<Material> material;
                    {
                        CS::Threading::ScopedReadLock lock(mLock);
                        material = materials.Get(node2->GetAttributeValue("material"), csRef<Material>());

                        // Validation.
                        csString msg;
                        msg.Format("Invalid material reference '%s' in cal3d meshfact '%s'", node2->GetAttributeValue("material"), node->GetAttributeValue("name"));
                        CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                    }
                    mf->materials.Push(material);
                    mf->checked.Push(false);
                }

                if(node->GetNode("params")->GetNode("cells"))
                {
                    node = node->GetNode("params")->GetNode("cells")->GetNode("celldefault")->GetNode("basematerial");
                    {
                        csRef<Material> material;
                        {    
                            CS::Threading::ScopedReadLock lock(mLock);
                            material = materials.Get(node->GetContentsValue(), csRef<Material>());

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
                                    material = materials.Get(node2->GetAttributeValue("material"), csRef<Material>());

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
                meshfacts.Put(mf->name, mf);
            }

            nodeItr = root->GetNodes("sector");
            while(nodeItr->HasNext())
            {
                node = nodeItr->Next();

                csRef<Sector> s;
                csString sectorName = node->GetAttributeValue("name");
                sectorName.Downcase();
                {
                    CS::Threading::ScopedReadLock lock(sLock);
                    s = sectortree.Get(sectorName, csRef<Sector>());
                }

                if(!s.IsValid())
                {
                    s = csPtr<Sector>(new Sector(sectorName));
                    CS::Threading::ScopedWriteLock lock(sLock);
                    sectors.Push(s);
                    sectortree.Put(sectorName, s);
                }

                s->init = true;
                s->culler = node->GetNode("cullerp")->GetContentsValue();
                if(node->GetNode("ambient"))
                {
                    node = node->GetNode("ambient");
                    s->ambient = csColor(node->GetAttributeValueAsFloat("red"),
                        node->GetAttributeValueAsFloat("green"), node->GetAttributeValueAsFloat("blue"));
                    node = node->GetParent();
                }

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

                nodeItr2 = node->GetNodes("meshobj");
                while(nodeItr2->HasNext())
                {
                    csRef<iDocumentNode> node2 = nodeItr2->Next();
                    csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(node2->GetAttributeValue("name"), vfsPath, node2));
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
                            csRef<Material> material;
                            {
                                CS::Threading::ScopedReadLock lock(mLock);
                                material = materials.Get(node3->GetNode("material")->GetContentsValue(), csRef<Material>());

                                // Validation.
                                csString msg;
                                msg.Format("Invalid material reference '%s' in meshobj submesh", node3->GetNode("material")->GetContentsValue());
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
                                    texture = textures.Get(node3->GetContentsValue(), csRef<Texture>());

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
                            meshfact = meshfacts.Get(node2->GetContentsValue(), csRef<MeshFact>());

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

                        csRef<iDocumentNode> cell = meshfact->data->GetNode("params")->GetNode("cells");
                        if(cell.IsValid())
                        {
                          cell = cell->GetNode("celldefault");
                          if(cell.IsValid())
                          {
                            m->hasBBox = true;
                            cell = cell->GetNode("size");
                            m->bbox.AddBoundingVertex(m->pos.x-(cell->GetAttributeValueAsInt("x")/2),
                              m->pos.y,
                              m->pos.z-(cell->GetAttributeValueAsInt("z")/2));
                            m->bbox.AddBoundingVertex(m->pos.x+(cell->GetAttributeValueAsInt("x")/2),
                              m->pos.y+cell->GetAttributeValueAsInt("y"),
                              m->pos.z+(cell->GetAttributeValueAsInt("z")/2));
                          }
                        }

                        m->meshfacts.Push(meshfact);
                        m->mftchecked.Push(false);
                    }
                    node2 = node2->GetParent();

                    if(node2->GetNode("material"))
                    {
                        node2 = node2->GetNode("material");

                        csRef<Material> material;
                        {
                            CS::Threading::ScopedReadLock lock(mLock);
                            material = materials.Get(node2->GetContentsValue(), csRef<Material>());

                            // Validation.
                            csString msg;
                            msg.Format("Invalid material reference '%s' in terrain object", node2->GetContentsValue());
                            CS_ASSERT_MSG(msg.GetData(), material.IsValid());
                        }

                        m->materials.Push(material);
                        m->matchecked.Push(false);
                        node2 = node2->GetParent();
                    }


                    if(node2->GetNode("materialpalette"))
                    {
                        nodeItr3 = node2->GetNode("materialpalette")->GetNodes("material");
                        while(nodeItr3->HasNext())
                        {
                            csRef<iDocumentNode> node3 = nodeItr3->Next();
                            csRef<Material> material;
                            {
                                CS::Threading::ScopedReadLock lock(mLock);
                                material = materials.Get(node3->GetContentsValue(), csRef<Material>());

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
                                            texture = textures.Get(node3->GetContentsValue(), csRef<Texture>());

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

                    s->meshes.Push(m);
                    CS::Threading::ScopedWriteLock lock(meshLock);
                    meshes.Put(m->name, m);
                }

                if(gfxFeatures & useMeshGen)
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
                            mgen->object = meshes.Get(meshgen->GetNode("meshobj")->GetContentsValue(), csRef<MeshObj>());
                        }

                        csRef<iDocumentNodeIterator> geometries = meshgen->GetNodes("geometry");
                        while(geometries->HasNext())
                        {
                            csRef<iDocumentNode> geometry = geometries->Next();

                            csRef<MeshFact> meshfact;
                            {
                                csString name(geometry->GetNode("factory")->GetAttributeValue("name"));
                                CS::Threading::ScopedReadLock lock(mfLock);
                                meshfact = meshfacts.Get(name, csRef<MeshFact>());

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
                                    material = materials.Get(name, csRef<Material>());

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
                        targetSector.Downcase();
                        {
                            CS::Threading::ScopedReadLock lock(sLock);
                            p->targetSector = sectortree.Get(targetSector, csRef<Sector>());
                        }

                        if(!p->targetSector.IsValid())
                        {
                            p->targetSector = csPtr<Sector>(new Sector(targetSector));
                            CS::Threading::ScopedWriteLock lock(sLock);
                            sectors.Push(p->targetSector);
                            sectortree.Put(targetSector, p->targetSector);
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

        // Wait for plugin and shader loads to finish.
        tman->Wait(rets);
    }

    return true;
}

void BgLoader::ContinueLoading(bool waiting)
{
    bool finishLoading = true;
    
    // Limit even while waiting - we want some frames.
    size_t count = 0;
    while(finishLoading && count < 20)
    {
        // Delete from delete queue (fairly expensive, so limited per update).
        if(!deleteQueue.IsEmpty())
        {
            CleanMesh(deleteQueue[0]);
            deleteQueue.DeleteIndexFast(0);
        }

        // Check already loading meshes.
        for(size_t i=0; i<(loadingMeshes.GetSize() < 10 ? loadingMeshes.GetSize() : 10); ++i)
        {
            if(LoadMesh(loadingMeshes[i]))
            {
                finalisableMeshes.Push(loadingMeshes[i]);
                loadingMeshes.DeleteIndex(i);
            }
        }

        // Finalise loaded meshes (expensive, so limited per update).
        if(!finalisableMeshes.IsEmpty())
        {
            FinishMeshLoad(finalisableMeshes[0]);
            finalisableMeshes.DeleteIndexFast(0);
        }

        // Load meshgens.
        for(size_t i=0; i<loadingMeshGen.GetSize(); ++i)
        {
            if(LoadMeshGen(loadingMeshGen[i]))
            {
                loadingMeshGen.DeleteIndex(i);
            }
        }

        ++count;
        if(!waiting || GetLoadingCount() == 0)
            finishLoading = false;
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
        sector = sectortree.Get(csString(sectorName).Downcase(), csRef<Sector>());
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
        LoadSector(pos, loadBox, unloadBox, sector, 0);

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
    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(sector->meshes[i]->object.IsValid())
        {
            sector->meshes[i]->object->GetMovable()->ClearSectors();
            sector->meshes[i]->object->GetMovable()->UpdateMove();
            engine->GetMeshes()->Remove(sector->meshes[i]->object);
            sector->meshes[i]->object.Invalidate();
            CleanMesh(sector->meshes[i]);
            --sector->objectCount;
        }
    }

    for(size_t i=0; i<sector->meshgen.GetSize(); i++)
    {
        CleanMeshGen(sector->meshgen[i]);
        --sector->objectCount;
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
        msg.Format("Error cleaning sector. Sector still has %zu objects!", sector->objectCount);
        CS_ASSERT_MSG(msg.GetData(), false);
    }
    CS_ASSERT_MSG("Error cleaning sector. Sector is invalid!", sector->object.IsValid());

    engine->GetSectors()->Remove(sector->object);
    sector->object.Invalidate();
}

void BgLoader::CleanMesh(MeshObj* mesh)
{
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

void BgLoader::LoadSector(const csVector3& pos, const csBox3& loadBox, const csBox3& unloadBox,
                        Sector* sector, uint depth)
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

            LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->activePortals[i]->targetSector, depth+1);
        }
    }

    for(size_t i=0; i<sector->meshes.GetSize(); i++)
    {
        if(!sector->meshes[i]->loading)
        {
            if(sector->meshes[i]->InRange(pos, loadBox, loadRange))
            {
                sector->meshes[i]->loading = true;
                loadingMeshes.Push(sector->meshes[i]);
                ++sector->objectCount;
            }
            else if(sector->meshes[i]->OutOfRange(pos, unloadBox, loadRange))
            {
                sector->meshes[i]->object->GetMovable()->ClearSectors();
                sector->meshes[i]->object->GetMovable()->UpdateMove();
                engine->GetMeshes()->Remove(sector->meshes[i]->object);
                sector->meshes[i]->object.Invalidate();
                deleteQueue.Push(sector->meshes[i]);
                --sector->objectCount;
            }
        }
    }

    for(size_t i=0; i<sector->meshgen.GetSize(); i++)
    {
        if(!sector->meshgen[i]->loading)
        {
            if(sector->meshgen[i]->InRange(loadBox))
            {
                sector->meshgen[i]->loading = true;
                loadingMeshGen.Push(sector->meshgen[i]);
                ++sector->objectCount;
            }
            else if(sector->meshgen[i]->OutOfRange(unloadBox))
            {
                CleanMeshGen(sector->meshgen[i]);
                --sector->objectCount;
            }
        }
    }

    for(size_t i=0; i<sector->portals.GetSize(); i++)
    {
        if(depth < maxPortalDepth && sector->portals[i]->InRange(loadBox))
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
                LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1);
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
                LoadSector(wwPos, wwLoadBox, wwUnloadBox, sector->portals[i]->targetSector, depth+1);
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

void BgLoader::FinishMeshLoad(MeshObj* mesh)
{
  mesh->object = scfQueryInterface<iMeshWrapper>(mesh->status->GetResultRefPtr());
  mesh->status.Invalidate();
  engine->SyncEngineListsNow(tloader);

  // Mark the mesh as being realtime lit depending on graphics setting.
  if(gfxFeatures & (useMediumShaders | useLowShaders | useLowestShaders))
  {
      mesh->object->GetFlags().Set(CS_ENTITY_NOLIGHTING);
  }

  // Set world position.
  mesh->object->GetMovable()->SetSector(mesh->sector->object);
  mesh->object->GetMovable()->UpdateMove();

  // Init collision data. TODO: Load simpler CD meshes.
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

    if(ready && !mesh->status)
    {
        mesh->status = tloader->LoadNode(mesh->path, mesh->data);
    }

    return (mesh->status && mesh->status->IsFinished());
}

bool BgLoader::LoadMeshFact(MeshFact* meshfact)
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
            meshfact->checked[i] = LoadMaterial(meshfact->materials[i]);
            ready &= meshfact->checked[i];
        }
    }

    if(ready && !meshfact->status)
    {
        meshfact->status = tloader->LoadNode(meshfact->path, meshfact->data);
        return false;
    }

    if(meshfact->status && meshfact->status->IsFinished())
    {
        ++meshfact->useCount;
        return true;
    }

    return false;
}

bool BgLoader::LoadMaterial(Material* material)
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
            material->checked[i] = LoadTexture(material->textures[i]);
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

        ++material->useCount;
        return true;
    }

    return false;
}

bool BgLoader::LoadTexture(Texture* texture)
{
    if(texture->useCount != 0)
    {
        ++texture->useCount;
        return true;
    }

    if(!texture->status.IsValid())
    {
        texture->status = tloader->LoadNode(texture->path, texture->data);
        return false;
    }

    if(texture->status->IsFinished())
    {
        ++texture->useCount;
        return true;
    }

    return false;
}

csPtr<iMeshFactoryWrapper> BgLoader::LoadFactory(const char* name, bool* failed)
{
    csRef<MeshFact> meshfact = meshfacts.Get(name, csRef<MeshFact>());
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

    if(LoadMeshFact(meshfact))
    {
        return scfQueryInterface<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());
    }

    return csPtr<iMeshFactoryWrapper>(0);
}

csPtr<iMaterialWrapper> BgLoader::LoadMaterial(const char* name, bool* failed)
{
    csRef<Material> material = materials.Get(name, csRef<Material>());
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

    if(LoadMaterial(material))
    {
        return csPtr<iMaterialWrapper>(material->mat);
    }

    return csPtr<iMaterialWrapper>(0);
}

bool BgLoader::InWaterArea(const char* sector, csVector3* pos, csColor4** colour) const
{
    // Hack to work around the weird sector stuff we do.
    if(!strcmp("SectorWhereWeKeepEntitiesResidingInUnloadedMaps", sector))
        return false;

    csRef<Sector> s = sectortree.Get(csString(sector).Downcase(), csRef<Sector>());
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

}
CS_PLUGIN_NAMESPACE_END(bgLoader)
