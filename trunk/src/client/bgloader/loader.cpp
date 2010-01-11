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

#ifndef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!x) printf("%s\n", msg);
#endif

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
SCF_IMPLEMENT_FACTORY(BgLoader)

BgLoader::BgLoader(iBase *p)
  : scfImplementationType (this, p), loadRange(200), enabledGfxFeatures(0),
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
                                //CS_ASSERT_MSG(msg.GetData(), texture.IsValid());
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
                                    //CS_ASSERT_MSG(msg.GetData(), false);
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
    for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
    {
        sector->alwaysLoaded[i]->object->GetMovable()->ClearSectors();
        sector->alwaysLoaded[i]->object->GetMovable()->UpdateMove();
        engine->GetMeshes()->Remove(sector->alwaysLoaded[i]->object);
        sector->alwaysLoaded[i]->object.Invalidate();
        CleanMesh(sector->alwaysLoaded[i]);
        --(sector->objectCount);
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

        // Load all meshes which should always be loaded in this sector.
        for(size_t i=0; i<sector->alwaysLoaded.GetSize(); i++)
        {
            sector->alwaysLoaded[i]->loading = true;
            loadingMeshes.Push(sector->alwaysLoaded[i]);
            ++(sector->objectCount);
        }
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

                    // Load all meshes which should always be loaded in this sector.
                    for(size_t j=0; j<sector->portals[i]->targetSector->alwaysLoaded.GetSize(); j++)
                    {
                        sector->portals[i]->targetSector->alwaysLoaded[j]->loading = true;
                        loadingMeshes.Push(sector->portals[i]->targetSector->alwaysLoaded[j]);
                        ++(sector->portals[i]->targetSector->objectCount);
                    }
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

    return true;
}

iMeshWrapper* BgLoader::CreateAndSelectMesh(const char* factName, iCamera* camera, const csVector2& pos)
{
    // Check that requested mesh is valid.
    csRef<MeshFact> meshfact = meshfacts.Get(mfStringSet.Request(factName), csRef<MeshFact>());
    if(!meshfact.IsValid())
        return 0;

    // Get WS position.
    csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

    // If there's no hit then we can't create.
    if(result.mesh == 0)
        return 0;

    // Load meshfactory.
    while(!LoadMeshFact(meshfact));
    csRef<iMeshFactoryWrapper> factory = scfQueryInterface<iMeshFactoryWrapper>(meshfact->status->GetResultRefPtr());

    // Update stored position.
    previousPosition = pos;

    // Create new mesh.
    if(selectedMesh && resetHitbeam)
    {
        selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
    }
    resetHitbeam = false;
    selectedMesh = factory->CreateMeshWrapper();
    selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    selectedMesh->GetMovable()->SetPosition(camera->GetSector(), result.isect);
    selectedMesh->GetMovable()->UpdateMove();

    return selectedMesh;
}

iMeshWrapper* BgLoader::SelectMesh(iCamera* camera, const csVector2& pos)
{
    // Get WS position.
    csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

    // Reset flags.
    if(selectedMesh && resetHitbeam)
    {
        selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
    }

    // Get new selected mesh.
    selectedMesh = result.mesh;
    if(selectedMesh)
    {
        resetHitbeam = !selectedMesh->GetFlags().Check(CS_ENTITY_NOHITBEAM);
        selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    }

    // Update stored position.
    previousPosition = pos;

    return selectedMesh;
}

bool BgLoader::TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos)
{
    if(selectedMesh.IsValid())
    {
        if(vertical)
        {
            float d = 5 * float(previousPosition.y - pos.y) / g2d->GetWidth();
            csVector3 position = selectedMesh->GetMovable()->GetPosition();
            selectedMesh->GetMovable()->SetPosition(position + csVector3(0.0f, d, 0.0f));
            previousPosition = pos;
        }
        else
        {
            // Get WS position.
            csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);
            if(result.mesh)
            {
                selectedMesh->GetMovable()->SetPosition(result.isect);
                return true;
            }
        }
    }

    return false;
}

void BgLoader::RotateSelected(const csVector2& pos)
{
    if(selectedMesh.IsValid())
    {
        float d = 6 * PI * ((float)previousPosition.x - pos.x) / g2d->GetWidth();
        csYRotMatrix3 rotation(d);

        selectedMesh->GetMovable()->GetTransform().SetO2T(origRotation*rotation);
    }
}

void BgLoader::RemoveSelected()
{
    if(selectedMesh.IsValid())
    {
        selectedMesh->GetMovable()->SetSector(0);
        selectedMesh.Invalidate();
    }
}

}
CS_PLUGIN_NAMESPACE_END(bgLoader)
