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

//#ifdef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!x) printf("ART ERROR: %s\n", msg)
//#endif

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
    void BgLoader::ParseShaders()
    {
        if(!parseShaders || parsedShaders)
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
                        if(blockShaderLoad)
                        {
                            rets.Push(tloader->LoadShaderWait(vfs->GetCwd(), file->GetContentsValue()));
                        }
                        else
                        {
                            // Dispatch shader load to a thread.
                            rets.Push(tloader->LoadShader(vfs->GetCwd(), file->GetContentsValue()));
                        }

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

    BgLoader::ShaderVar* BgLoader::ParseShaderVar(const csString& name, const csString& type, const csString& value, csRef<Texture>& tex, bool doChecks)
    {
        tex.Invalidate();
        ShaderVar* sv = 0;
        // Parse the different types. Currently texture, vector2 and vector3 are supported.
        if(type == "texture")
        {
            // Ignore some shader variables if the functionality they bring is not enabled.
            if(doChecks && enabledGfxFeatures & (useHighShaders | useMediumShaders | useLowShaders | useLowestShaders))
            {
                if(name == "tex height" || name == "tex ambient occlusion")
                {
                    return 0;
                }

                if(enabledGfxFeatures & (useMediumShaders | useLowShaders | useLowestShaders))
                {
                    if(name == "tex specular")
                    {
                        return 0;
                    }

                    if(enabledGfxFeatures & (useLowShaders | useLowestShaders))
                    {
                        if(name == "tex normal" || name == "tex normal compressed")
                        {
                            return 0;
                        }
                    }
                }
            }

            sv = new ShaderVar(name.GetData(), csShaderVariable::TEXTURE);
            sv->value = value;

            {
                CS::Threading::ScopedReadLock lock(tLock);
                tex = textures.Get(tStringSet.Request(value.GetData()), csRef<Texture>());
            }
        }
        else if(type == "float")
        {
            sv = new ShaderVar(name.GetData(), csShaderVariable::FLOAT);
            csScanStr (value.GetData(), "%f", &sv->vec1);
        }
        else if(type == "vector2")
        {
            sv = new ShaderVar(name.GetData(), csShaderVariable::VECTOR2);
            csScanStr (value.GetData(), "%f,%f", &sv->vec2.x, &sv->vec2.y);
        }
        else if(type == "vector3")
        {
            sv = new ShaderVar(name.GetData(), csShaderVariable::VECTOR3);
            csScanStr (value.GetData(), "%f,%f,%f", &sv->vec3.x, &sv->vec3.y, &sv->vec3.z);
        }
        else if(type == "vector4")
        {
            sv = new ShaderVar(name.GetData(), csShaderVariable::VECTOR4);
            csScanStr (value.GetData(), "%f,%f,%f,%f", &sv->vec4.x, &sv->vec4.y, &sv->vec4.z, &sv->vec4.w);
        }
        else
        {
            // unknown type
            csString msg;
            msg.Format("Unknown variable type in shadervar %s: %s", name.GetData(), type.GetData());
            CS_ASSERT_MSG(msg.GetData(), false);
        }
        return sv;
    }

    void BgLoader::ParseMaterials(iDocumentNode* materialsNode)
    {
        csRef<iDocumentNodeIterator> it = materialsNode->GetNodes("material");
        while(it->HasNext())
        {
            csRef<iDocumentNode> materialNode = it->Next();
            csRef<Material> m = csPtr<Material>(new Material(materialNode->GetAttributeValue("name")));
            if(m.IsValid())
            {
                CS::Threading::ScopedWriteLock lock(mLock);
                materials.Put(mStringSet.Request(m->name), m);
            }
            else
            {
                csString msg;
                msg.Format("Failed to create material '%s'", materialNode->GetAttributeValue("name"));
                CS_ASSERT_MSG(msg.GetData(), false);
                continue;
            }

            // Parse the texture for a material. Construct a shader variable for it.
            csRef<iDocumentNodeIterator> nodeIt = materialNode->GetNodes();
            while(nodeIt->HasNext())
            {
                csRef<iDocumentNode> node(nodeIt->Next());
                csStringID id = xmltokens.Request(node->GetValue());
                switch(id)
                {
                    case PARSERTOKEN_TEXTURE:
                    case PARSERTOKEN_SHADERVAR:
                    {
                        csString varName;
                        csString varType;
                        csString varValue(node->GetContentsValue());
                        csRef<Texture> texture;

                        if(id == PARSERTOKEN_TEXTURE)
                        {
                            varName = "tex diffuse";
                            varType = "texture";
                        }
                        else
                        {
                            varName = node->GetAttributeValue("name");
                            varType = node->GetAttributeValue("type");
                        }

                        ShaderVar* sv = ParseShaderVar(varName, varType, varValue, texture, id != PARSERTOKEN_TEXTURE);
                        if(!sv)
                        {
                            // no result probably means it wasn't validated for the shaders we use
                            // if it was a parse error, the function already throws an error
                            csString msg;
                            msg.Format("failed to parse shadervar '%s' in material '%s'!", varName.GetData(), m->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);
                            continue;
                        }
                        else if(sv->type == csShaderVariable::TEXTURE && !texture.IsValid())
                        {
                            csString msg;
                            msg.Format("Invalid texture reference '%s' in shadervar '%s' in material '%s'!",
                                        varValue.GetData(), varName.GetData(), m->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);
                        }
                        else
                        {
                            m->shadervars.Push(*sv);
                            if(sv->type == csShaderVariable::TEXTURE && texture.IsValid())
                            {
                                m->textures.Push(texture);
                                m->checked.Push(false);
                            }
                        }
                        delete sv;
                    }
                    break;

                    case PARSERTOKEN_SHADER:
                    {
                        m->shaders.Push(Shader(node->GetAttributeValue("type"), node->GetContentsValue()));
                    }
                    break;
                }
            }
        }
    }

    void BgLoader::ParseMaterialReference(const char* name, csRefArray<Material>& materialArray, csArray<bool>& checked, const char* parent, const char* type)
    {
        csRef<Material> material;
        {
            CS::Threading::ScopedReadLock lock(mLock);
            material = materials.Get(mStringSet.Request(name), csRef<Material>());
        }

        if(material.IsValid())
        {
            materialArray.Push(material);
            checked.Push(false);
        }
        else
        {
            // Validation.
            csString msg;
            msg.Format("Invalid material reference '%s' in %s", name, type);
            if(parent)
            {
                msg.AppendFmt(" '%s'", parent);
            }
            CS_ASSERT_MSG(msg.GetData(), false);
        }
    }

    void BgLoader::ParsePortal(iDocumentNode* portalNode, ParserData& parserData)
    {
        csRef<iDocumentNodeIterator> portalsIt(portalNode->GetNodes("portal"));
        while(portalsIt->HasNext())
        {
            csRef<iDocumentNode> portalNode(portalsIt->Next());
            csRef<Portal> p = csPtr<Portal>(new Portal(portalNode->GetAttributeValue("name")));

            csRef<iDocumentNodeIterator> nodeIt(portalNode->GetNodes());
            while(nodeIt->HasNext())
            {
                csRef<iDocumentNode> node(nodeIt->Next());
                csStringID id = xmltokens.Request(node->GetValue());
                switch(id)
                {
                    case PARSERTOKEN_MATRIX:
                    {
                        p->warp = true;
                        syntaxService->ParseMatrix(node, p->matrix);
                    }
                    break;

                    case PARSERTOKEN_WV:
                    {
                        p->warp = true;
                        syntaxService->ParseVector(node, p->wv);
                    }
                    break;

                    case PARSERTOKEN_WW:
                    {
                        p->warp = true;
                        p->ww_given = true;
                        syntaxService->ParseVector(node, p->ww);
                    }
                    break;

                    case PARSERTOKEN_FLOAT:
                    {
                        p->pfloat = true;
                    }
                    break;

                    case PARSERTOKEN_CLIP:
                    {
                        p->clip = true;
                    }
                    break;

                    case PARSERTOKEN_ZFILL:
                    {
                        p->zfill = true;
                    }
                    break;

                    case PARSERTOKEN_AUTORESOLVE:
                    {
                        p->autoresolve = true;
                    }
                    break;

                    case PARSERTOKEN_SECTOR:
                    {
                        csString targetSector = node->GetContentsValue();
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
                    }
                    break;

                    case PARSERTOKEN_V:
                    {
                        csVector3 vertex;
                        syntaxService->ParseVector(node, vertex);
                        p->poly.AddVertex(vertex);
                        p->bbox.AddBoundingVertex(vertex);
                    }
                    break;
                }
            }

            if(!p->ww_given)
            {
                p->ww = p->wv;
            }

            p->transform = csReversibleTransform(p->matrix.GetInverse(), p->ww - p->matrix * p->wv);
            {
                CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
                parserData.currentSector->portals.Push(p);
            }
        }
    }

    void BgLoader::ParseLight(iDocumentNode* lightNode, ParserData& parserData)
    {
        csRef<Light> l = csPtr<Light>(new Light(lightNode->GetAttributeValue("name")));
        {
            CS::Threading::ScopedWriteLock lock(sLock);
            parserData.lights.Put(sStringSet.Request(l->name), l);
        }

        // set default values
        l->attenuation = CS_ATTN_LINEAR;
        l->dynamic = CS_LIGHT_DYNAMICTYPE_STATIC;
        l->type = CS_LIGHT_POINTLIGHT;

        csRef<iDocumentNodeIterator> nodeIt(lightNode->GetNodes());
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_ATTENUATION:
                {
                    csStringID type = xmltokens.Request(node->GetContentsValue());
                    switch(type)
                    {
                        case PARSERTOKEN_NONE:
                        {
                            l->attenuation = CS_ATTN_NONE;
                        }
                        break;

                        case PARSERTOKEN_LINEAR:
                        {
                            l->attenuation = CS_ATTN_LINEAR;
                        }
                        break;

                        case PARSERTOKEN_INVERSE:
                        {
                            l->attenuation = CS_ATTN_INVERSE;
                        }
                        break;

                        case PARSERTOKEN_REALISTIC:
                        {
                            l->attenuation = CS_ATTN_REALISTIC;
                        }
                        break;

                        case PARSERTOKEN_CLQ:
                        {
                            l->attenuation = CS_ATTN_CLQ;
                        }
                        break;
                    }
                }
                break;

                case PARSERTOKEN_DYNAMIC:
                {
                    l->dynamic = CS_LIGHT_DYNAMICTYPE_PSEUDO;
                }
                break;

                case PARSERTOKEN_CENTER:
                {
                    syntaxService->ParseVector(node, l->pos);
                    l->bbox.SetCenter(l->pos);
                }
                break;
                
                case PARSERTOKEN_RADIUS:
                {
                    csVector3 size(node->GetContentsValueAsFloat());
                    l->bbox.SetSize(size);
                }
                break;

                case PARSERTOKEN_COLOR:
                {
                    syntaxService->ParseColor(node, l->colour);
                }
                break;
            }
        }

        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->lights.Push(l);
        }
    }

    void BgLoader::ParseSector(iDocumentNode* sectorNode, ParserData& parserData)
    {
        csRef<Sector> s;
        csString sectorName = sectorNode->GetAttributeValue("name");
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

        if(parserData.zone.IsValid())
        {
            s->parent = parserData.zone;
            parserData.zone->sectors.Push(s);
        }

        // let the other parser functions know about us
        parserData.currentSector = s;

        // set this sector as initialized
        s->init = true;

        csRef<iDocumentNodeIterator> it(sectorNode->GetNodes());
        while(it->HasNext())
        {
            csRef<iDocumentNode> node(it->Next());
            csStringID id = xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_CULLERP:
                {
                    s->culler = node->GetContentsValue();
                }
                break;

                case PARSERTOKEN_AMBIENT:
                {
                    s->ambient = csColor(node->GetAttributeValueAsFloat("red"),
                                         node->GetAttributeValueAsFloat("green"),
                                         node->GetAttributeValueAsFloat("blue"));
                }
                break;

                case PARSERTOKEN_KEY:
                {
                    if(csString("water").Compare(node->GetAttributeValue("name")))
                    {
                        csRef<iDocumentNodeIterator> areasIt(node->GetNodes("area"));
                        while(areasIt->HasNext())
                        {
                            csRef<iDocumentNode> areaNode(areasIt->Next());
                            WaterArea area;
                            csRef<iDocumentNode> colourNode = areaNode->GetNode("colour");
                            if(colourNode.IsValid())
                            {
                                syntaxService->ParseColor(colourNode, area.colour);
                            }
                            else
                            {
                                area.colour = csColor4(0.0f, 0.17f, 0.49f, 0.6f);
                            }

                            csRef<iDocumentNodeIterator> vs = areaNode->GetNodes("v");
                            while(vs->HasNext())
                            {
                                csRef<iDocumentNode> v(vs->Next());
                                csVector3 vector;
                                syntaxService->ParseVector(v, vector);
                                area.bbox.AddBoundingVertex(vector);
                            }

                            {
                                CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
                                parserData.currentSector->waterareas.Push(area);
                            }
                        }
                    }
                }
                break;

                case PARSERTOKEN_PORTALS:
                {
                    ParsePortal(node, parserData);
                }
                break;

                case PARSERTOKEN_LIGHT:
                {
                    ParseLight(node, parserData);
                }
                break;

                case PARSERTOKEN_MESHOBJ:
                {
                    ParseMeshObj(node, parserData);
                }
                break;

                case PARSERTOKEN_TRIMESH:
                {
                    ParseTriMesh(node, parserData);
                }
                break;

                case PARSERTOKEN_MESHGEN:
                {
                    ParseMeshGen(node, parserData);
                }
                break;
            }
        }
    }

    void BgLoader::ParseSequences(iDocumentNode* sequencesNode, ParserData& parserData)
    {
        // do a 2 pass parsing of sequences
        // first create all sequences.
        csRef<iDocumentNodeIterator> sequenceIt(sequencesNode->GetNodes("sequence"));
        while(sequenceIt->HasNext())
        {
            csRef<iDocumentNode> sequenceNode(sequenceIt->Next());
            csRef<Sequence> seq = csPtr<Sequence>(new Sequence(sequenceNode->GetAttributeValue("name"), sequenceNode));
            {
                CS::Threading::ScopedWriteLock lock(sLock);
                parserData.sequences.Put(sStringSet.Request(seq->name), seq);
            }
            CS_ASSERT_MSG("Created invalid sequence!", seq.IsValid());
        }

        // now actually parse them.
        sequenceIt = sequencesNode->GetNodes("sequence");
        while(sequenceIt->HasNext())
        {
            csRef<iDocumentNode> sequenceNode(sequenceIt->Next());
            csRef<Sequence> seq;
            {
                CS::Threading::ScopedWriteLock lock(sLock);
                seq = parserData.sequences.Get(sStringSet.Request(sequenceNode->GetAttributeValue("name")), csRef<Sequence>());
            }
            CS_ASSERT_MSG("Created invalid sequence!", seq.IsValid());

            bool loaded = false;
            csRef<iDocumentNodeIterator> nodeIt(sequenceNode->GetNodes());
            while(nodeIt->HasNext())
            {
                csRef<iDocumentNode> node(nodeIt->Next());
                csStringID id = xmltokens.Request(node->GetValue());
                bool parsed = true;
                switch(id)
                {
                    // sequence types operating on a sector.
                    case PARSERTOKEN_SETAMBIENT:
                    case PARSERTOKEN_FADEAMBIENT:
                    case PARSERTOKEN_SETFOG:
                    case PARSERTOKEN_FADEFOG:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Sector> s = sectorHash.Get(sStringSet.Request(node->GetAttributeValue("sector")), csRef<Sector>());
                        s->sequences.Push(seq);
                    }
                    break;

                    // sequence types operating on a mesh.
                    case PARSERTOKEN_SETCOLOR:
                    case PARSERTOKEN_FADECOLOR:
                    case PARSERTOKEN_MATERIAL:
                    case PARSERTOKEN_ROTATE:
                    case PARSERTOKEN_MOVE:
                    {
                        CS::Threading::ScopedWriteLock lock(meshLock);
                        csRef<MeshObj> m = meshes.Get(meshStringSet.Request(node->GetAttributeValue("mesh")), csRef<MeshObj>());
                        m->sequences.Push(seq);
                    }
                    break;

                    // sequence types operating on a light.
                    case PARSERTOKEN_ROTATELIGHT:
                    case PARSERTOKEN_MOVELIGHT:
                    case PARSERTOKEN_FADELIGHT:
                    case PARSERTOKEN_SETLIGHT:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Light> l = parserData.lights.Get(sStringSet.Request(node->GetAttributeValue("light")), csRef<Light>());
                        l->sequences.Push(seq);
                    }
                    break;

                    // sequence types operating on a sequence.
                    case PARSERTOKEN_RUN:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Sequence> dep = parserData.sequences.Get(sStringSet.Request(node->GetAttributeValue("sequence")), csRef<Sequence>());
                        dep->sequences.Push(seq);
                    }
                    break;

                    // sequence types operating on a trigger.
                    case PARSERTOKEN_ENABLE:
                    case PARSERTOKEN_DISABLE:
                    case PARSERTOKEN_CHECK:
                    case PARSERTOKEN_TEST:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Trigger> t = parserData.triggers.Get(sStringSet.Request(node->GetAttributeValue("trigger")), csRef<Trigger>());
                        if(!t.IsValid())
                        {
                            // trigger doesn't exist, yet - create it
                            t = csPtr<Trigger>(new Trigger(node->GetAttributeValue("trigger"), 0));
                            csString msg;
                            msg.Format("created trigger %s that is referenced by a sequence, but didn't exist, yet!",
                                        t->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);

                            parserData.triggers.Put(sStringSet.Request(t->name), t);
                        }
                        seq->triggers.Push(t);
                    }
                    break;

                    // miscallenous sequence types.
                    case PARSERTOKEN_DELAY:
                    case PARSERTOKEN_SETVAR:
                    default:
                    {
                        parsed = false;
                    }
                    break;
                }
                loaded |= parsed;
            }

            CS_ASSERT_MSG("Unknown sequence type!", loaded);
        }
    }

    void BgLoader::ParseTriggers(iDocumentNode* triggers, ParserData& parserData)
    {
        csRef<iDocumentNodeIterator> triggerIt(triggers->GetNodes("trigger"));
        while(triggerIt->HasNext())
        {
            csRef<iDocumentNode> triggerNode(triggerIt->Next());
            csRef<Trigger> trigger;
            {
                const char* triggerName = triggerNode->GetAttributeValue("name");
                CS::Threading::ScopedWriteLock lock(sLock);
                trigger = parserData.triggers.Get(sStringSet.Request(triggerName), csRef<Trigger>());
                if(!trigger.IsValid())
                {
                    trigger = csPtr<Trigger>(new Trigger(triggerName, triggerNode));
                    parserData.triggers.Put(sStringSet.Request(triggerName), trigger);
                }
                else
                {
                    trigger->data = triggerNode;
                }
            }
            bool loaded = false;

            csRef<iDocumentNodeIterator> nodeIt(triggerNode->GetNodes());
            while(nodeIt->HasNext())
            {
                csRef<iDocumentNode> node(nodeIt->Next());
                csStringID id = xmltokens.Request(node->GetValue());
                bool parsed = true;
                switch(id)
                {
                    // triggers fired by a sector.
                    case PARSERTOKEN_SECTORVIS:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Sector> s = sectorHash.Get(sStringSet.Request(node->GetAttributeValue("sector")), csRef<Sector>());
                        s->triggers.Push(trigger);
                    }
                    break;

                    // triggers fired by a mesh.
                    case PARSERTOKEN_ONCLICK:
                    {
                        CS::Threading::ScopedWriteLock lock(meshLock);
                        csRef<MeshObj> m = meshes.Get(meshStringSet.Request(node->GetAttributeValue("mesh")), csRef<MeshObj>());
                        m->triggers.Push(trigger);
                    }
                    break;

                    // triggers fired by a light.
                    case PARSERTOKEN_LIGHTVALUE:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Light> l = parserData.lights.Get(sStringSet.Request(node->GetAttributeValue("light")), csRef<Light>());
                        l->triggers.Push(trigger);
                    }
                    break;

                    // triggers fired by a sequence.
                    case PARSERTOKEN_FIRE:
                    {
                        CS::Threading::ScopedWriteLock lock(sLock);
                        csRef<Sequence> seq = parserData.sequences.Get(sStringSet.Request(node->GetAttributeValue("sequence")), csRef<Sequence>());
                        seq->triggers.Push(trigger);
                    }
                    break;

                    // triggers fired by a miscallenous operation.
                    case PARSERTOKEN_MANUAL:
                    default:
                    {
                        parsed = false;
                    }
                    break;
                }

                loaded |= parsed;
            }

            CS_ASSERT_MSG("Unknown trigger type!", loaded);
        }
    }

    THREADED_CALLABLE_IMPL2(BgLoader, PrecacheData, const char* path, bool recursive)
    {
        // Make sure shaders are parsed at this point.
        ParseShaders();

        ParserData data;
        data.path = path;

        // Don't parse folders.
        data.vfsPath = path;
        if(data.vfsPath.GetAt(data.vfsPath.Length()-1) == '/')
            return false;

        if(vfs->Exists(data.vfsPath))
        {
            // Restores any directory changes.
            csVfsDirectoryChanger dirchange(vfs);

            csRef<iDocumentNode> root;
            {
                // XML doc structures.
                csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
                csRef<iDocument> doc = docsys->CreateDocument();
                csRef<iDataBuffer> buffer = vfs->ReadFile(data.path);
                if(!buffer.IsValid())
                    return false;

                doc->Parse(buffer, true);

                // Check that it's an xml file.
                if(!doc->GetRoot())
                    return false;

                if(!recursive)
                {
                    dirchange.ChangeTo(data.vfsPath.Truncate(data.vfsPath.FindLast('/')+1));
                }
                else
                {
                    data.vfsPath = vfs->GetCwd();
                }

                // Begin document parsing.
                data.realRoot = false;
                root = doc->GetRoot()->GetNode("world");
                if(!root.IsValid())
                {
                    root = doc->GetRoot()->GetNode("library");
                    if(!root)
                    {
                        data.realRoot = true;
                        root = doc->GetRoot();
                    }
                }
                else
                {
                    csString zonen(path);
                    zonen = zonen.Slice(zonen.FindLast('/')+1);
                    data.zone = csPtr<Zone>(new Zone(zonen));

                    CS::Threading::ScopedWriteLock lock(zLock);
                    zones.Put(zStringSet.Request(zonen.GetData()), data.zone);
                }
            }

            if(root.IsValid())
            {
                csRef<iDocumentNodeIterator> nodeIt(root->GetNodes());
                while(nodeIt->HasNext())
                {
                    csRef<iDocumentNode> node(nodeIt->Next());
                    csStringID id = xmltokens.Request(node->GetValue());
                    switch(id)
                    {
                        // Parse referenced libraries.
                        case PARSERTOKEN_LIBRARY:
                        {
                            csString target(node->GetContentsValue());
                            PrecacheDataTC(ret, false, target, target.FindFirst('/') == SIZET_NOT_FOUND);
                        }
                        break;

                        // Parse needed plugins.
                        case PARSERTOKEN_PLUGINS:
                        {
                            data.rets.Push(tloader->LoadNode(vfs->GetCwd(), node));
                        }
                        break;

                        // Parse referenced shaders.
                        case PARSERTOKEN_SHADERS:
                        {
                            csRef<iDocumentNodeIterator> shaderIt(node->GetNodes("shader"));
                            while(shaderIt->HasNext())
                            {
                                csRef<iDocumentNode> shader(shaderIt->Next());

                                csRef<iDocumentNode> fileNode(shader->GetNode("file"));
                                if(fileNode.IsValid())
                                {
                                    bool loadShader = false;
                                    const char* fileName = fileNode->GetContentsValue();
                                    {
                                        // Keep track of shaders that have already been parsed.
                                        CS::Threading::ScopedWriteLock lock(sLock);
                                        if(shaders.Contains(fileName) == csArrayItemNotFound)
                                        {
                                            shaders.Push(fileName);
                                            loadShader = true;
                                        }
                                    }

                                    if(loadShader && parseShaders)
                                    {
                                        // Dispatch shader load to a thread.
                                        csRef<iThreadReturn> shaderRet = tloader->LoadShader(vfs->GetCwd(), fileName);
                                        if(blockShaderLoad)
                                        {
                                            shaderRet->Wait();
                                        }
                                        else
                                        {
                                            data.rets.Push(shaderRet);
                                        }
                                    }
                                }
                            }
                        }
                        break;

                        // Parse all referenced textures.
                        case PARSERTOKEN_TEXTURES:
                        {
                            csRef<iDocumentNodeIterator> textureIt(node->GetNodes("texture"));
                            while(textureIt->HasNext())
                            {
                                csRef<iDocumentNode> textureNode(textureIt->Next());
                                csRef<Texture> t = csPtr<Texture>(new Texture(textureNode->GetAttributeValue("name"), data.vfsPath, textureNode));
                                {
                                    CS::Threading::ScopedWriteLock lock(tLock);
                                    textures.Put(tStringSet.Request(t->name), t);
                                }
                            }
                        }
                        break;

                        // Parse all referenced materials.
                        case PARSERTOKEN_MATERIALS:
                        {
                            ParseMaterials(node);
                        }
                        break;

                        // Parse mesh factory.
                        case PARSERTOKEN_MESHFACT:
                        {
                            ParseMeshFact(node, data);
                        }
                        break;

                        // Parse sector.
                        case PARSERTOKEN_SECTOR:
                        {
                            ParseSector(node, data);
                        }
                        break;

                        // Parse start position.
                        case PARSERTOKEN_START:
                        {
                            csRef<StartPosition> startPos = csPtr<StartPosition>(new StartPosition());
                            csString zonen(path);
                            zonen = zonen.Slice(zonen.FindLast('/')+1);
                            startPos->zone = zonen;
                            startPos->sector = node->GetNode("sector")->GetContentsValue();
                            syntaxService->ParseVector(node->GetNode("position"), startPos->position);
                            startPositions.Push(startPos);
                        }
                        break;

                        case PARSERTOKEN_SEQUENCES:
                        {
                            ParseSequences(node, data);
                        }
                        break;

                        case PARSERTOKEN_TRIGGERS:
                        {
                            ParseTriggers(node, data);
                        }
                        break;
                    }
                }
            }

            // Wait for plugin and shader loads to finish.
            tman->Wait(data.rets);
        }
        
        return true;
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)

