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
    void BgLoader::ParseMeshFact(iDocumentNode* meshfactNode, ParserData& parserData)
    {
         const char* meshfactName = meshfactNode->GetAttributeValue("name");
         csRef<MeshFact> mf = csPtr<MeshFact>(new MeshFact(meshfactName, parserData.vfsPath, meshfactNode));

         if(!cache)
         {
             if(parserData.realRoot && !parserData.parsedMeshFact)
             {
                 // Load this file when needed to save memory.
                 mf->data.Invalidate();
                 mf->filename = csString(parserData.path).Slice(csString(parserData.path).FindLast('/')+1);
             }

             // Mark that we've already loaded a meshfact in this file.
             parserData.parsedMeshFact = true;
         }

         csRef<iDocumentNodeIterator> it(meshfactNode->GetNodes());
         while(it->HasNext())
         {
             csRef<iDocumentNode> node(it->Next()); 
             csStringID id = xmltokens.Request(node->GetValue());
             switch(id)
             {
                 case PARSERTOKEN_PARAMS:
                 {
                     csRef<iDocumentNodeIterator> paramsIt(node->GetNodes());
                     while(paramsIt->HasNext())
                     {
                         csRef<iDocumentNode> paramNode(paramsIt->Next());
                         csStringID param = xmltokens.Request(paramNode->GetValue());
                         switch(param)
                         {
                             case PARSERTOKEN_CELLS:
                             {
                                 csRef<iDocumentNodeIterator> cellsIt(paramNode->GetNodes());
                                 while(cellsIt->HasNext())
                                 {
                                     csRef<iDocumentNode> cellNode(cellsIt->Next());
                                     csStringID cell = xmltokens.Request(cellNode->GetValue());
                                     switch(cell)
                                     {
                                         case PARSERTOKEN_CELLDEFAULT:
                                         {
                                             csRef<iDocumentNode> cellSize = cellNode->GetNode("size");
                                             if(cellSize.IsValid())
                                             {
                                                 csVector3 bound(cellSize->GetAttributeValueAsInt("x")/2,
                                                                 cellSize->GetAttributeValueAsInt("y"),
                                                                 cellSize->GetAttributeValueAsInt("z")/2);
                                                 mf->bboxvs.Push(bound);
                                                 bound *= -1;
                                                 bound.y = 0;
                                                 mf->bboxvs.Push(bound);
                                             }

                                             csRef<iDocumentNode> baseMat = cellNode->GetNode("basematerial");
                                             if(baseMat.IsValid())
                                             {
                                                 ParseMaterialReference(baseMat->GetContentsValue(), mf->materials, mf->checked,
                                                                        "terrain mesh", 0);
                                             }
                                         }
                                         break;

                                         case PARSERTOKEN_CELL:
                                         {
                                             csRef<iDocumentNode> feederProps = cellNode->GetNode("feederproperties");
                                             if(feederProps.IsValid())
                                             {
                                                 csRef<iDocumentNode> alphamap = feederProps->GetNode("alphamap");
                                                 if(alphamap.IsValid())
                                                 {
                                                     ParseMaterialReference(alphamap->GetAttributeValue("material"), mf->materials, mf->checked,
                                                                            "terrain mesh", 0);
                                                 }
                                             }
                                         }
                                         break;
                                     }
                                 }
                             }
                             break;

                             case PARSERTOKEN_MATERIAL:
                             {
                                 ParseMaterialReference(paramNode->GetContentsValue(), mf->materials, mf->checked,
                                                        "meshfact", meshfactName);
                             }
                             break;

                             case PARSERTOKEN_SUBMESH:
                             {
                                 const char* submeshName = paramNode->GetAttributeValue("name");
                                 csRef<iDocumentNode> materialNode = paramNode->GetNode("material");
                                 if(materialNode.IsValid())
                                 {
                                     csString description("meshfact '");
                                     description.Append(meshfactName);
                                     description.Append("' submesh");
                                     ParseMaterialReference(materialNode->GetContentsValue(), mf->materials, mf->checked,
                                                            description.GetData(), submeshName);
                                 }
                                 mf->submeshes.Push(submeshName);
                             }
                             break;

                             case PARSERTOKEN_MESH:
                             {
                                 csString description("cal3d meshfact '");
                                 description.Append(meshfactName);
                                 description.Append("' mesh");
                                 ParseMaterialReference(paramNode->GetAttributeValue("material"), mf->materials, mf->checked,
                                                        description.GetData(), paramNode->GetAttributeValue("name"));
                             }
                             break;
                         }
                     }
                 }
                 break;

                 case PARSERTOKEN_KEY:
                 {
                     if(csString("bbox").Compare(node->GetAttributeValue("name")))
                     {
                         csRef<iDocumentNodeIterator> vs = node->GetNodes("v");
                         while(vs->HasNext())
                         {
                             csRef<iDocumentNode> v = vs->Next();
                             csVector3 vtex;
                             syntaxService->ParseVector(v, vtex);
                             mf->bboxvs.Push(vtex);
                         }
                         break;
                     }
                 }
                 break;
             }
         }

         CS::Threading::ScopedWriteLock lock(mfLock);
         meshfacts.Put(mfStringSet.Request(mf->name), mf);
    }

    void BgLoader::ParseMeshObj(iDocumentNode* meshNode, ParserData& parserData)
    {
        csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(meshNode->GetAttributeValue("name"), parserData.vfsPath, meshNode));
        m->sector = parserData.currentSector;
        bool alwaysLoaded = false;

        // Check for a params file and switch to use it to continue parsing.
        csRef<iDocumentNode> params(meshNode);
        csRef<iDocumentNode> paramfileNode(meshNode->GetNode("paramsfile"));
        if(paramfileNode.IsValid())
        {
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
            csRef<iDocument> pdoc = docsys->CreateDocument();
            csRef<iDataBuffer> pdata = vfs->ReadFile(paramfileNode->GetContentsValue());
            CS_ASSERT_MSG("Invalid params file.\n", pdata.IsValid());
            pdoc->Parse(pdata, true);
            params = pdoc->GetRoot();
        }
        params = params->GetNode("params");
        if(!params.IsValid())
        {
            return;
        }

        csRef<iDocumentNodeIterator> paramIt(params->GetNodes());
        while(paramIt->HasNext())
        {
            csRef<iDocumentNode> paramNode(paramIt->Next());
            csStringID id = xmltokens.Request(paramNode->GetValue());
            switch(id)
            {
                case PARSERTOKEN_SUBMESH:
                {
                    const char* submeshName = paramNode->GetAttributeValue("name");

                    csRef<iDocumentNode> materialNode(paramNode->GetNode("material"));
                    if(materialNode.IsValid())
                    {
                        csString description("meshobj '");
                        description.Append(m->name.GetData());
                        description.Append("' submesh in sector");
                        ParseMaterialReference(materialNode->GetContentsValue(), m->materials, m->matchecked,
                                               description.GetData(), parserData.currentSector->name.GetData());
                    }

                    csRef<iDocumentNodeIterator> shaderVarIt(paramNode->GetNodes("shadervar"));
                    while(shaderVarIt->HasNext())
                    {
                        csRef<iDocumentNode> shaderVar(shaderVarIt->Next());
                        csString name = shaderVar->GetAttributeValue("name");
                        csString type = shaderVar->GetAttributeValue("type");
                        csString value = shaderVar->GetContentsValue();
                        csRef<Texture> texture;
                        ShaderVar* sv = ParseShaderVar(name, type, value, texture, false);
                        if(!sv)
                        {
                            // no result probably means it wasn't validated for the shaders we use
                            // if it was a parse error, the function already throws an error
                            /*csString msg;
                            msg.Format("failed to parse renderproperty shadervar '%s' for meshobj '%s'",
                            name.GetData(), m->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);*/
                            continue;
                        }
                        else if(sv->type == csShaderVariable::TEXTURE && !texture.IsValid())
                        {
                            csString msg;
                            msg.Format("Invalid texture reference '%s' in terrain renderproperty shadervar for meshobj '%s'",
                                        value.GetData(), m->name.GetData());
                            CS_ASSERT_MSG(msg.GetData(), false);
                        }
                        else
                        {
                            // we don't add the variable here because CS' loader does it for us on loading
                            if(sv->type == csShaderVariable::TEXTURE && texture.IsValid())
                            {
                                //m->shadervars.Push(*sv);
                                m->textures.Push(texture);
                                m->texchecked.Push(false);
                            }
                        }
                        delete sv;
                    }

                    bool found = false;
                    for(size_t i = 0; i < m->meshfacts.GetSize(); i++)
                    {
                        if(m->meshfacts[i]->submeshes.Find(submeshName) != csArrayItemNotFound)
                        {
                            found = true;
                            break;
                        }
                    }
                    
                    if(!found)
                    {
                        csString msg;
                        msg.Format("Invalid submesh reference '%s' in meshobj '%s' in sector '%s'",
                                    submeshName, m->name.GetData(), parserData.currentSector->name.GetData());
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                }
                break;

                case PARSERTOKEN_FACTORY:
                {
                    csRef<MeshFact> meshfact;
                    {
                        CS::Threading::ScopedReadLock lock(mfLock);
                        meshfact = meshfacts.Get(mfStringSet.Request(paramNode->GetContentsValue()), csRef<MeshFact>());
                    }
                    if(meshfact.IsValid())
                    {
                        csRef<iDocumentNode> moveNode = meshNode->GetNode("move");
                        if(moveNode.IsValid())
                        {
                            csRef<iDocumentNode> posNode(moveNode->GetNode("v"));
                            if(posNode.IsValid())
                            {
                                csVector3 pos;
                                csMatrix3 rot;
                                syntaxService->ParseVector(posNode, pos);

                                csRef<iDocumentNode> matrixNode(moveNode->GetNode("matrix"));
                                if(matrixNode.IsValid())
                                {
                                    syntaxService->ParseMatrix(matrixNode, rot);
                                    rot.Invert();
                                }

                                csReversibleTransform t(rot, pos);
                                for(size_t i = 0; i < meshfact->bboxvs.GetSize(); i++)
                                {
                                    m->bbox.AddBoundingVertex(t.This2Other(meshfact->bboxvs[i]));
                                }
                            }
                        }

                        m->meshfacts.Push(meshfact);
                        m->mftchecked.Push(false);
                    }
                    else
                    {
                        // Validation.
                        csString msg;
                        msg.Format("Invalid factory reference '%s' in meshobj '%s' in sector '%s'", paramNode->GetContentsValue(),
                                    m->name.GetData(), parserData.currentSector->name.GetData());
                        CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
                    }
                }
                break;

                case PARSERTOKEN_MATERIAL:
                {
                    csString description("terrain '");
                    description.Append(m->name.GetData());
                    description.Append("' object in sector");
                    ParseMaterialReference(paramNode->GetContentsValue(), m->materials, m->matchecked,
                                           description.GetData(), parserData.currentSector->name.GetData());
                }
                break;

                case PARSERTOKEN_MATERIALPALETTE:
                {
                    csRef<iDocumentNodeIterator> materialIt(paramNode->GetNodes("material"));
                    while(materialIt->HasNext())
                    {
                        csRef<iDocumentNode> materialNode(materialIt->Next());
                        ParseMaterialReference(materialNode->GetContentsValue(), m->materials, m->matchecked,
                                               "terrain materialpalette", 0);
                    }
                }
                break;

                case PARSERTOKEN_CELLS:
                {
                    csRef<iDocumentNodeIterator> cellIt(paramNode->GetNodes("cell"));
                    while(cellIt->HasNext())
                    {
                        csRef<iDocumentNode> cellNode(cellIt->Next());
                        csRef<iDocumentNode> renderProps(cellNode->GetNode("renderproperties"));
                        if(renderProps.IsValid())
                        {
                            csRef<iDocumentNodeIterator> shaderVarIt(renderProps->GetNodes("shadervar"));
                            while(shaderVarIt->HasNext())
                            {
                                csRef<iDocumentNode> shaderVar(shaderVarIt->Next());
                                csString name = shaderVar->GetAttributeValue("name");
                                csString type = shaderVar->GetAttributeValue("type");
                                csString value = shaderVar->GetContentsValue();
                                csRef<Texture> texture;

                                ShaderVar* sv = ParseShaderVar(name, type, value, texture, false);
                                if(!sv)
                                {
                                    // no result probably means it wasn't validated for the shaders we use
                                    // if it was a parse error, the function already throws an error
                                    /*csString msg;
                                    msg.Format("failed to parse renderproperty shadervar '%s' for meshobj '%s'",
                                    name.GetData(), m->name.GetData());
                                    CS_ASSERT_MSG(msg.GetData(), false);*/
                                    continue;
                                }
                                else if(sv->type == csShaderVariable::TEXTURE && !texture.IsValid())
                                {
                                    csString msg;
                                    msg.Format("Invalid texture reference '%s' in terrain renderproperty shadervar for meshobj '%s'",
                                                value.GetData(), m->name.GetData());
                                    CS_ASSERT_MSG(msg.GetData(), false);
                                }
                                else
                                {
                                    m->shadervars.Push(*sv);
                                    if(sv->type == csShaderVariable::TEXTURE && texture.IsValid())
                                    {
                                        m->textures.Push(texture);
                                        m->texchecked.Push(false);
                                    }
                                }
                                delete sv;
                            }
                        }
                    }
                }
                break;

                case PARSERTOKEN_ALWAYSLOADED:
                {
                    alwaysLoaded = true;
                }
                break;
            }
        }

        // alwaysloaded ignores range checks. If the sector is loaded then so is this mesh.
        if(alwaysLoaded || m->bbox.Empty() || m->bbox.IsNaN())
        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->alwaysLoaded.Push(m);
        }
        else
        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->meshes.Push(m);
        }

        CS::Threading::ScopedWriteLock lock(meshLock);
        meshes.Put(meshStringSet.Request(m->name), m);
    }

    void BgLoader::ParseTriMesh(iDocumentNode* meshNode, ParserData& parserData)
    {
        csRef<MeshObj> m = csPtr<MeshObj>(new MeshObj(meshNode->GetAttributeValue("name"), parserData.vfsPath, meshNode));
        m->sector = parserData.currentSector;

        bool alwaysLoaded = true;
        csRef<iDocumentNodeIterator> nodeIt = meshNode->GetNodes();
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_MESH:
                {
                    csRef<iDocumentNodeIterator> vertexIt(node->GetNodes("v"));
                    while(vertexIt->HasNext())
                    {
                        csRef<iDocumentNode> vertex(vertexIt->Next());
                        csVector3 v;
                        syntaxService->ParseVector(vertex, v);
                        m->bbox.AddBoundingVertex(v);
                    }
                    alwaysLoaded = false;
                }
                break;

                case PARSERTOKEN_BOX:
                {
                    syntaxService->ParseBox(node, m->bbox);
                    alwaysLoaded = false;
                }
                break;
            }
        }

        if(alwaysLoaded || m->bbox.Empty() || m->bbox.IsNaN())
        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->alwaysLoaded.Push(m);
        }
        else
        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->meshes.Push(m);
        }

        CS::Threading::ScopedWriteLock lock(meshLock);
        meshes.Put(meshStringSet.Request(m->name), m);
    }

    void BgLoader::ParseMeshGen(iDocumentNode* meshNode, ParserData& parserData)
    {
        csRef<MeshGen> mgen = csPtr<MeshGen>(new MeshGen(meshNode->GetAttributeValue("name"), meshNode));

        mgen->sector = parserData.currentSector;
        {
            CS::Threading::ScopedWriteLock lock(parserData.currentSector->lock);
            parserData.currentSector->meshgen.Push(mgen);
        }

        csRef<iDocumentNode> objNode(meshNode->GetNode("meshobj"));
        if(objNode.IsValid())
        {
            CS::Threading::ScopedReadLock lock(meshLock);
            csStringID mID = meshStringSet.Request(objNode->GetContentsValue());
            mgen->object = meshes.Get(mID, csRef<MeshObj>());
        }

        csRef<iDocumentNode> boxNode(meshNode->GetNode("samplebox"));
        if(boxNode.IsValid())
        {
            csVector3 min;
            csVector3 max;

            syntaxService->ParseVector(boxNode->GetNode("min"), min);
            syntaxService->ParseVector(boxNode->GetNode("max"), max);

            mgen->bbox = csBox3(min,max);
        }

        csRef<iDocumentNodeIterator> geometryIt(meshNode->GetNodes("geometry"));
        while(geometryIt->HasNext())
        {
            csRef<iDocumentNode> geometry(geometryIt->Next());

            csRef<MeshFact> meshfact;
            {
                csString name(geometry->GetNode("factory")->GetAttributeValue("name"));
                CS::Threading::ScopedReadLock lock(mfLock);
                meshfact = meshfacts.Get(mfStringSet.Request(name), csRef<MeshFact>());
            }

            if(meshfact.IsValid())
            {
                mgen->meshfacts.Push(meshfact);
                mgen->mftchecked.Push(false);
            }
            else
            {
                // Validation.
                csString msg;
                msg.Format("Invalid meshfact reference '%s' in meshgen '%s'", mgen->name.GetData(), mgen->name.GetData());
                CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
            }

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

                if(material.IsValid())
                {
                    mgen->materials.Push(material);
                    mgen->matchecked.Push(false);
                }
            }
        }
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)

