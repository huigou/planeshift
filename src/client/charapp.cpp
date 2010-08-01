/*
 * charapp.cpp
 *
 * Copyright (C) 2002-2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/scenenode.h>
#include <imap/loader.h>
#include <imesh/object.h>
#include <iutil/object.h>
#include <ivaria/keyval.h>
#include <ivideo/shader/shader.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "iclient/ibgloader.h"
#include "util/log.h"
#include "util/psstring.h"
#include "effects/pseffect.h"
#include "effects/pseffectmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "charapp.h"
#include "psengine.h"
#include "pscelclient.h"
#include "psclientchar.h"
#include "globals.h"

#include <csutil/callstack.h>

static const unsigned int meshCount = 9;
static const char* meshNames[meshCount] = { "Head", "Torso", "Hand", "Legs", "Foot", "Arm", "Eyes", "Hair", "Beard" };
static const unsigned int bracersSlotCount = 2;
static csString BracersSlots[bracersSlotCount] = { "rightarm", "leftarm" };

psCharAppearance::psCharAppearance(iObjectRegistry* objectReg)
{
    stringSet = csQueryRegistryTagInterface<iShaderVarStringSet>(objectReg, "crystalspace.shader.variablenameset");
    engine = csQueryRegistry<iEngine>(objectReg);
    vfs    = csQueryRegistry<iVFS>(objectReg);
    g3d    = csQueryRegistry<iGraphics3D>(objectReg);
    txtmgr = g3d->GetTextureManager();
    xmlparser =  csQueryRegistry<iDocumentSystem> (objectReg);

    //NOTE: THESE MUST BE VALID AT ALL TIMES!
    eyeMesh = "Eyes";
    hairMesh = "Hair";
    beardMesh = "Beard";

    eyeColorSet = false;
    hairColorSet = false;
    sneak = false;
}

void psCharAppearance::Free()
{
    while(!delayedAttach.IsEmpty())
    {
        // free ressources allocated by delayed loading
        // that won't be used
        Attachment & attach = delayedAttach.Front();
        if(attach.materialPtr.IsValid())
        {
            attach.materialPtr.Invalidate();
            psengine->GetLoader()->FreeMaterial(attach.materialName);
        }

        if(attach.factoryPtr.IsValid())
        {
            attach.factoryPtr.Invalidate();
            psengine->GetLoader()->FreeFactory(attach.factName);
        }

        delayedAttach.PopFront();
    }

    ClearEquipment();
    csHash<csString,csString>::GlobalIterator it = materials.GetIterator();
    csString part;
    while(it.HasNext())
    {
        // reset all materials
        it.Next(part);
        DefaultMaterial(part);
    }

    while(CheckLoadStatus())
    {
        continue; // wait for material resets to finish
    }

    state.Invalidate();
    stateFactory.Invalidate();
    animeshObject.Invalidate();
    animeshFactory.Invalidate();
    baseMesh.Invalidate();

    it.Reset();
    while(it.HasNext())
    {
        // reset all materials
        csString& value = it.Next();
        psengine->GetLoader()->FreeMaterial(value);
    }
    materials.DeleteAll();
    removedMeshes.DeleteAll();

    // reset traits
    skinToneSet.DeleteAll();
    faceMaterial.Clear();
    eyeColorSet = false;
    hairColorSet = false;
    beardAttached = false;
    eyeMesh = "Eyes";
    hairMesh = "Hair";
    beardMesh = "Beard";
}

psCharAppearance::~psCharAppearance()
{
    Free();
}

void psCharAppearance::SetMesh(iMeshWrapper* mesh)
{
    if(baseMesh == mesh)
        return;

    Free();

    state = scfQueryInterface<iSpriteCal3DState>(mesh->GetMeshObject());
    stateFactory = scfQueryInterface<iSpriteCal3DFactoryState>(mesh->GetMeshObject()->GetFactory());

    animeshObject = scfQueryInterface<iAnimatedMesh>(mesh->GetMeshObject());
    animeshFactory = scfQueryInterface<iAnimatedMeshFactory>(mesh->GetMeshObject()->GetFactory());

    baseMesh = mesh;
}


csString psCharAppearance::ParseStrings(const char* part, const char* str) const
{
    csString result(str);

    const char* factname = baseMesh->GetFactory()->QueryObject()->GetName();

    result.ReplaceAll("$F", factname);
    result.ReplaceAll("$P", part);

    return result;
}


void psCharAppearance::FaceTexture(csString& faceMaterial)
{
    ChangeMaterial("Head", faceMaterial);
}

void psCharAppearance::BeardMesh(csString& subMesh)
{
    beardMesh = subMesh;

    if ( beardMesh.Length() == 0 )
    {
        if(state && stateFactory)
        {
            for ( int idx=0; idx < stateFactory->GetMeshCount(); idx++)
            {
                const char* meshName = stateFactory->GetMeshName(idx);

                if ( strstr(meshName, "Beard") )
                {
                    state->DetachCoreMesh(meshName);
                    beardAttached = false;
                }
            }
        }
        else if(animeshObject && animeshFactory)
        {
            for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
            {
                const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
                if(meshName)
                {
                    if ( strstr(meshName, "Beard") )
                    {
                        animeshObject->GetSubMesh(idx)->SetRendering(false);
                        beardAttached = false;
                    }
                }
            }
        }
        return;
    }

    csString newPartParsed = ParseStrings("Beard", beardMesh);

    if(state && stateFactory)
    {
        int newMeshAvailable = stateFactory->FindMeshName(newPartParsed);
        if ( newMeshAvailable == -1 )
        {
            return;
        }
        else
        {
            for ( int idx=0; idx < stateFactory->GetMeshCount(); idx++)
            {
                const char* meshName = stateFactory->GetMeshName(idx);

                if ( strstr(meshName, "Beard") )
                {
                    state->DetachCoreMesh(meshName);
                }
            }

            state->AttachCoreMesh(newPartParsed);
            beardAttached = true;
            beardMesh = newPartParsed;
        }
    }
    else if(animeshObject && animeshFactory)
    {
        for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
        {
            const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
            if(meshName)
            {
                if (strstr(meshName, "Beard"))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(false);
                }

                if (!strcmp(meshName, newPartParsed))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(true);
                    beardAttached = true;
                    beardMesh = newPartParsed;
                }
            }
        }
    }

    if ( hairColorSet )
        HairColor(hairShader);
}

void psCharAppearance::HairMesh(csString& subMesh)
{
    csString newPartParsed = subMesh.Length() ? ParseStrings("Hair", subMesh) : hairMesh;

    if(state && stateFactory)
    {
        int newMeshAvailable = stateFactory->FindMeshName(newPartParsed);
        if ( newMeshAvailable == -1 )
        {
            return;
        }
        else
        {
            for ( int idx=0; idx < stateFactory->GetMeshCount(); idx++)
            {
                const char* meshName = stateFactory->GetMeshName(idx);

                if ( strstr(meshName, "Hair") )
                {
                    state->DetachCoreMesh(meshName);
                }
            }

            state->AttachCoreMesh(newPartParsed);
            hairMesh = newPartParsed;
        }
    }
    else if(animeshObject && animeshFactory)
    {
        for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
        {
            const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
            if(meshName)
            {
                if (strstr(meshName, "Hair"))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(false);
                }

                if (!strcmp(meshName, newPartParsed))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(true);
                    hairMesh = newPartParsed;
                }
            }
        }
    }
    if ( hairColorSet )
        HairColor(hairShader);
}


void psCharAppearance::HairColor(csVector3& color)
{
    hairShader = color;
    csRef<iShaderVariableContext> context_hair;
    csRef<iShaderVariableContext> context_beard;

    if(state)
    {
        context_hair = state->GetCoreMeshShaderVarContext(hairMesh);
        context_beard = state->GetCoreMeshShaderVarContext(beardMesh);
    }
    else if(animeshObject && animeshFactory)
    {
        for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
        {
            const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
            if(meshName)
            {
                if (!strcmp(meshName, hairMesh))
                {
                    context_hair = animeshObject->GetSubMesh(idx)->GetShaderVariableContext(0);
                }

                if (!strcmp(meshName, beardMesh))
                {
                    context_beard = animeshObject->GetSubMesh(idx)->GetShaderVariableContext(0);
                }
            }
        }
    }

    if ( context_hair.IsValid() )
    {
        CS::ShaderVarStringID varName = stringSet->Request("color modulation");
        csShaderVariable* var = context_hair->GetVariableAdd(varName);

        if ( var )
        {
            var->SetValue(hairShader);
        }
    }

    if ( context_beard.IsValid() )
    {
        CS::ShaderVarStringID varName = stringSet->Request("color modulation");
        csShaderVariable* var = context_beard->GetVariableAdd(varName);

        if ( var )
        {
            var->SetValue(hairShader);
        }
    }
    hairColorSet = true;
}

void psCharAppearance::EyeColor(csVector3& color)
{
    eyeShader = color;
    iShaderVariableContext* context_eyes;


    if(state)
    {
        context_eyes = state->GetCoreMeshShaderVarContext(eyeMesh);
    }
    else if(animeshObject && animeshFactory)
    {
        size_t idx = animeshFactory->FindSubMesh(eyeMesh);
        if(idx != (size_t)-1)
        {
            context_eyes = animeshObject->GetSubMesh(idx)->GetShaderVariableContext(0);
        }
    }

    if ( context_eyes )
    {
        CS::ShaderVarStringID varName = stringSet->Request("color modulation");
        csShaderVariable* var = context_eyes->GetVariableAdd(varName);

        if ( var )
        {
            var->SetValue(eyeShader);
        }
    }

    eyeColorSet = true;
}

void psCharAppearance::ShowMeshes(csString& slot, csString& meshList, bool show)
{
    if(meshList.IsEmpty() || slot.IsEmpty())
        return;

    //allows substituiters for replaceable meshes.
    meshList.ReplaceAll("$H", hairMesh);
    meshList.ReplaceAll("$E", eyeMesh);
    meshList.ReplaceAll("$B", beardMesh);

    //check if we have still meshes to work on
    if(!meshList.Length())
        return;

    //first split the meshlist
    csStringArray meshes;
    meshes.SplitString(meshList, ",");

    for(size_t i = 0; i < meshes.GetSize(); i++)
    {
        csString meshName = meshes.Get(i);
        uint meshKey = csHashCompute(meshName);
        if (show) //in this case we are removing a restrain from hiding this mesh.
        {
            //get the list of items blocking this item
            csArray<csString> removingItems = removedMeshes.GetAll(meshKey);

            //if the list is empty it means it's not blocked, shouldn't happen as how it works.
            if(!removingItems.GetSize())
                continue;

            //if the list is > 1 it means another item is locking this mesh from being shown
            //so we just remove this blocker
            if(removingItems.GetSize() > 1)
            {
                removedMeshes.Delete(meshKey, slot);
                continue;
            }

            //in the other cases we restore the mesh.
            removedMeshes.DeleteAll(meshKey);

            if(state)
            {
                state->AttachCoreMesh(meshName);
            }
            else if(animeshObject && animeshFactory)
            {
                size_t idx = animeshFactory->FindSubMesh(meshName);
                if(idx != (size_t)-1)
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(true);
                }
            }

            //temporary hack for hair to be removed. Teorically all meshes should be passed through shader.
            if(meshName == hairMesh)
            {
                if (hairColorSet)
                    HairColor(hairShader);
            }
        }
        else //in this case we are adding a restrain to hiding this mesh
        {
            //there is already something here?
            csArray<csString> removingItems = removedMeshes.GetAll(meshKey);

            //add ourselves as a restrain.
            removedMeshes.Put(meshKey, slot);

            //this mesh was already removed so just bail out.
            //Note: this array is at the status of before adding ourselves
            if(removingItems.GetSize() > 0)
                continue;

            //This mesh wasn't already removed so let's remove it
            if(state)
            {
                state->DetachCoreMesh(meshName);
            }
            else if(animeshObject && animeshFactory)
            {
                size_t idx = animeshFactory->FindSubMesh(meshName);
                if(idx != (size_t)-1)
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(false);
                }
            }
        }
    }
}

void psCharAppearance::UpdateMeshShowStatus(csString& meshName)
{
    uint meshKey = csHashCompute(meshName);
    
    //check if the requested mesh was already removed
    if(removedMeshes.Contains(meshKey))
    {
        //in the case it is remove it. Useful when a mesh is substituited to update it's status correctly.
        if(state)
        {
            state->DetachCoreMesh(meshName);
        }
        else if(animeshObject && animeshFactory)
        {
            size_t idx = animeshFactory->FindSubMesh(meshName);
            if(idx != (size_t)-1)
            {
                animeshObject->GetSubMesh(idx)->SetRendering(false);
            }
        }
    }
}

void psCharAppearance::SetSkinTone(csString& part, csString& material)
{
    if (!baseMesh || !part || !material)
    {
        return;
    }
    else
    {
        SkinToneSet s;
        s.part = part;
        s.material = material;
        skinToneSet.Push(s);

        ChangeMaterial(part, material);
    }
}

void psCharAppearance::ApplyRider(csRef<iMeshWrapper> mesh)
{
    baseMesh->GetFlags().Set(CS_ENTITY_NODECAL);
    csString keyName = "socket_back";

    // Variables for transform to be specified
    float trans_x = 0, trans_y = 0.0, trans_z = 0, rot_x = -PI/2, rot_y = 0, rot_z = 0;
    csRef<iObjectIterator> it = baseMesh->GetFactory()->QueryObject()->GetIterator();

    while ( it->HasNext() )
    {
        csRef<iKeyValuePair> key ( scfQueryInterface<iKeyValuePair> (it->Next()));
        if (key && keyName == key->GetKey())
        {
            sscanf(key->GetValue(),"%f,%f,%f,%f,%f,%f",&trans_x,&trans_y,&trans_z,&rot_x,&rot_y,&rot_z);
        }
    }

    baseMesh->QuerySceneNode()->SetParent(mesh->QuerySceneNode());

    csRef<iSpriteCal3DState> mountState = scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
    csRef<iAnimatedMesh> mountObject = scfQueryInterface<iAnimatedMesh> (mesh->GetMeshObject());
    csRef<iAnimatedMeshFactory> mountFactory = scfQueryInterface<iAnimatedMeshFactory>(mesh->GetMeshObject()->GetFactory());

    csReversibleTransform transform(csZRotMatrix3(rot_z)*csYRotMatrix3(rot_y)*csXRotMatrix3(rot_x), csVector3(trans_x,trans_y,trans_z));
    if (mountState.IsValid())
    {
        csRef<iSpriteCal3DSocket> socket = mountState->FindSocket("back");
        if (socket.IsValid())
        {
            socket->SetMeshWrapper(baseMesh);
            socket->SetTransform(transform);
        }
    }
    else if (mountObject.IsValid() && mountFactory.IsValid())
    {
        size_t idx = mountFactory->FindSocket("back");
        if (idx != (size_t)-1)
        {
            csRef<iAnimatedMeshSocket> socket = mountObject->GetSocket(idx);
            socket->SetSceneNode(baseMesh->QuerySceneNode());
            socket->SetTransform(transform);
        }
    }
}

void psCharAppearance::ApplyEquipment(const csString& equipment)
{
    if ( equipment.Length() == 0 )
    {
        return;
    }

    csRef<iDocument> doc = xmlparser->CreateDocument();

    const char* error = doc->Parse(equipment);
    if ( error )
    {
        Error2("Error in XML: %s", error );
        return;
    }

    csString BaseGroup = baseMesh->GetFactory()->QueryObject()->GetName();

    // Do the helm check.
    csRef<iDocumentNode> helmNode = doc->GetRoot()->GetNode("equiplist")->GetNode("helm");
    csString helmGroup(helmNode->GetContentsValue());
    if ( helmGroup.Length() == 0 )
        helmGroup = BaseGroup;

    // Do the bracer check.
    csRef<iDocumentNode> BracerNode = doc->GetRoot()->GetNode("equiplist")->GetNode("bracer");
    csString BracerGroup(BracerNode->GetContentsValue());
    if ( BracerGroup.Length() == 0 )
        BracerGroup = BaseGroup;

    // Do the belt check.
    csRef<iDocumentNode> BeltNode = doc->GetRoot()->GetNode("equiplist")->GetNode("belt");
    csString BeltGroup(BeltNode->GetContentsValue());
    if ( BeltGroup.Length() == 0 )
        BeltGroup = BaseGroup;

    // Do the cloak check.
    csRef<iDocumentNode> CloakNode = doc->GetRoot()->GetNode("equiplist")->GetNode("cloak");
    csString CloakGroup(CloakNode->GetContentsValue());
    if ( CloakGroup.Length() == 0 )
        CloakGroup = BaseGroup;

    csRef<iDocumentNodeIterator> equipIter = doc->GetRoot()->GetNode("equiplist")->GetNodes("equip");

    while (equipIter->HasNext())
    {
        csRef<iDocumentNode> equipNode = equipIter->Next();
        csString slot = equipNode->GetAttributeValue( "slot" );
        csString mesh = equipNode->GetAttributeValue( "mesh" );
        csString part = equipNode->GetAttributeValue( "part" );
        csString partMesh = equipNode->GetAttributeValue("partMesh");
        csString texture = equipNode->GetAttributeValue( "texture" );
        csString removedMesh = equipNode->GetAttributeValue("removedMesh");

        //If the mesh has a $H it means it's an helm so search for replacement
        mesh.ReplaceAll("$H",helmGroup);
        //If the mesh has a $B it means it's a bracer so search for replacement
        mesh.ReplaceAll("$B",BracerGroup);
        //If the mesh has a $E it means it's a belt so search for replacement
        mesh.ReplaceAll("$E", BeltGroup);
        //If the mesh has a $C it means it's a cloak so search for replacement
        mesh.ReplaceAll("$C", CloakGroup);

        Equip(slot, mesh, part, partMesh, texture, removedMesh);
    }

    return;
}


void psCharAppearance::Equip( csString& slotname,
                              csString& mesh,
                              csString& part,
                              csString& subMesh,
                              csString& texture,
                              csString& removedMesh
                             )
{

    //Bracers must be managed separately as we have two slots in cal3d but only one slot here
    //To resolve the problem we call recursively this same function with the "corrected" slot names
    //which are rightarm leftarm

    if (slotname == "bracers")
    {
        for(unsigned int position = 0; position < bracersSlotCount; position++)
            Equip(BracersSlots[position], mesh, part, subMesh, texture, removedMesh);
        return;
    }

    /*if ( slotname == "helm" )
    {
        ShowHair(false);
    }*/

    ShowMeshes(slotname, removedMesh, false);

    // If it's a new mesh attach that mesh.
    if ( mesh.Length() )
    {
        if( texture.Length() && !subMesh.Length() )
            Attach(slotname, mesh, texture);
        else
            Attach(slotname, mesh);
    }

    // This is a subMesh on the model change so change the mesh for that part.
    if ( subMesh.Length() )
    {
        // Change the mesh on the part of the model.
        ChangeMesh(part, subMesh);

        // If there is also a new material ( texture ) then place that on as well.
        if ( texture.Length() )
        {
            ChangeMaterial( ParseStrings(part,subMesh), texture);
        }
    }
    else if ( part.Length() )
    {
        ChangeMaterial(part, texture);
    }
}


bool psCharAppearance::Dequip(csString& slotname,
                              csString& mesh,
                              csString& part,
                              csString& subMesh,
                              csString& texture,
                              csString& removedMesh)
{

    //look Equip() for more informations on this: bracers must be managed separately

    if (slotname == "bracers")
    {
        for(unsigned int position = 0; position < bracersSlotCount; position++)
            Dequip(BracersSlots[position], mesh, part, subMesh, texture, removedMesh);
        return true;
    }

    /*if ( slotname == "helm" )
    {
         ShowHair(true);
    }*/

    ShowMeshes(slotname, removedMesh, true);

    if ( mesh.Length() )
    {
        ClearEquipment(slotname);
        Detach(slotname);
    }

    // This is a part mesh (ie Mesh) set default mesh for that part.

    if ( subMesh.Length() )
    {
        DefaultMesh(part, subMesh);
    }

    if ( part.Length() )
    {
        if ( texture.Length() )
        {
            ChangeMaterial(part, texture);
        }
        else
        {
            DefaultMaterial(part);
        }
    }

    ClearEquipment(slotname);

    return true;
}


void psCharAppearance::DefaultMesh(const char* part, const char *subMesh)
{
    const char * defaultPart = NULL;

    if(stateFactory && state)
    {
        /* First we detach every mesh that match the partPattern */
        for (int idx=0; idx < stateFactory->GetMeshCount(); idx++)
        {
            const char * meshName = stateFactory->GetMeshName( idx );
            if (strstr(meshName, part) || (subMesh != NULL && strstr(meshName,subMesh)))
            {
                state->DetachCoreMesh( meshName );
                if (stateFactory->IsMeshDefault(idx))
                {
                    defaultPart = meshName;
                }
            }
        }

        if (!defaultPart)
        {
            return;
        }

        state->AttachCoreMesh( defaultPart );
    }
    else if(animeshFactory && animeshObject)
    {
        if (!defaultPart)
        {
            return;
        }

        for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
        {
            const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
            if(meshName)
            {
                if (strstr(meshName, part))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(false);
                }

                if (!strcmp(meshName, defaultPart))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(true);
                }
            }
        }
    }

    csString oldPart = defaultPart;
    UpdateMeshShowStatus(oldPart);

}


bool psCharAppearance::ChangeMaterial(const char* part, const char* materialName)
{
    if (!part || !materialName)
        return false;

    csString materialNameParsed = ParseStrings(part, materialName);

    bool failed = false;
    csRef<iMaterialWrapper> material = psengine->GetLoader()->LoadMaterial(materialNameParsed, &failed);
    if(!failed)
    {
        if(!material.IsValid())
        {
            Attachment attach(false);
            attach.materialName = materialNameParsed;
            attach.partName = part;
            attach.materialPtr = material;
            if(delayedAttach.IsEmpty())
            {
                psengine->RegisterDelayedLoader(this);
            }
            delayedAttach.PushBack(attach);

            return true;
        }

        ProcessAttach(material, materialNameParsed, part);
        return true;
    }

    // The material isn't available to load.
    csReport(psengine->GetObjectRegistry(), CS_REPORTER_SEVERITY_NOTIFY,
        "planeshift.character.appearance", "Attempted to change to material %s and failed; material not found.",
        materialNameParsed.GetData());
    return false;
}


bool psCharAppearance::ChangeMesh(const char* partPattern, const char* newPart)
{
    csString newPartParsed = ParseStrings(partPattern, newPart);

    if(stateFactory && state)
    {
        // If the new mesh cannot be found then do nothing.
        int newMeshAvailable = stateFactory->FindMeshName(newPartParsed);
        if ( newMeshAvailable == -1 )
            return false;

        /* First we detach every mesh that match the partPattern */
        for (int idx=0; idx < stateFactory->GetMeshCount(); idx++)
        {
            const char * meshName = stateFactory->GetMeshName( idx );
            if (strstr(meshName,partPattern))
            {
                state->DetachCoreMesh( meshName );
            }
        }

        state->AttachCoreMesh( newPartParsed.GetData() );
    }
    else if(animeshFactory && animeshObject)
    {
        // If the new mesh cannot be found then do nothing.
        size_t newMeshAvailable = animeshFactory->FindSubMesh(newPartParsed);
        if ( newMeshAvailable == (size_t)-1 )
            return false;

        /* First we detach every mesh that match the partPattern */
        for ( size_t idx=0; idx < animeshFactory->GetSubMeshCount(); idx++)
        {
            const char* meshName = animeshFactory->GetSubMesh(idx)->GetName();
            if(meshName)
            {
                if (strstr(meshName, partPattern))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(false);
                }

                if (!strcmp(meshName, newPartParsed))
                {
                    animeshObject->GetSubMesh(idx)->SetRendering(true);
                }
            }
        }
    }

    UpdateMeshShowStatus(newPartParsed);

    return true;
}


bool psCharAppearance::Attach(const char* socketName, const char* meshFactName, const char* materialName)
{
    if (!socketName || !meshFactName || !(state.IsValid() && !animeshObject.IsValid()))
    {
        return false;
    }

    bool failed = false;
    csRef<iMeshFactoryWrapper> factory = psengine->GetLoader()->LoadFactory(meshFactName, &failed);
    if(failed)
    {
        Notify2(LOG_CHARACTER, "Mesh factory %s not found.", meshFactName );
        return false;
    }

    csRef<iMaterialWrapper> material;
    if(materialName != NULL)
    {
        psengine->GetLoader()->LoadMaterial(materialName, &failed);
        if(failed)
        {
            Notify2(LOG_CHARACTER, "Material %s not found.", materialName);
            return false;
        }
    }
    
    Detach(socketName); // detach anything previously attached (free ressources)

    if(!factory.IsValid() || (materialName != NULL && !material.IsValid()))
    {
        Attachment attach(true);
        attach.factName = meshFactName;
        attach.factoryPtr = factory;
        attach.materialName = materialName;
        attach.materialPtr = material;
        // using socket name as the mesh may change while loading the factory/materials
        attach.socket = socketName;
        if(delayedAttach.IsEmpty())
        {
            psengine->RegisterDelayedLoader(this);
        }
        delayedAttach.PushBack(attach);
    }
    else
    {
        ProcessAttach(factory, material, meshFactName, socketName);
        if(materialName != NULL)
        {
            csString & mat = materials.GetOrCreate(socketName);
            if(!mat.IsEmpty()) // free previously allocated loader ressource
            {
                psengine->GetLoader()->FreeMaterial(materialName);
            }
            mat = materialName;
        }
    }

    return true;
}

void psCharAppearance::ProcessAttach(iMeshFactoryWrapper* factory, iMaterialWrapper* material, const char* meshFactName, const char* socket)
{
     csRef<iMeshWrapper> meshWrap = engine->CreateMeshWrapper( factory, meshFactName );

     if(material != NULL)
     {
         meshWrap->GetMeshObject()->SetMaterialWrapper(material);
     }

    ProcessAttach(meshWrap, socket);

    psengine->GetCelClient()->HandleItemEffect(factory->QueryObject()->GetName(), meshWrap, false, socket, &effectids, &lightids);
}

void psCharAppearance::ProcessAttach(csRef<iMeshWrapper> meshWrap, const char* socket)
{
    meshWrap->GetFlags().Set(CS_ENTITY_NODECAL);

    // Given a socket name of "righthand", we're looking for a key in the form of "socket_righthand"
    csString keyName = "socket_";
    keyName += socket;

    // Variables for transform to be specified
    float trans_x = 0, trans_y = 0.0, trans_z = 0, rot_x = -PI/2, rot_y = 0, rot_z = 0;
    csRef<iObjectIterator> it = meshWrap->GetFactory()->QueryObject()->GetIterator();

    while ( it->HasNext() )
    {
        csRef<iKeyValuePair> key ( scfQueryInterface<iKeyValuePair> (it->Next()));
        if (key && keyName == key->GetKey())
        {
            sscanf(key->GetValue(),"%f,%f,%f,%f,%f,%f",&trans_x,&trans_y,&trans_z,&rot_x,&rot_y,&rot_z);
        }
    }

    csReversibleTransform transform(csZRotMatrix3(rot_z)*csYRotMatrix3(rot_y)*csXRotMatrix3(rot_x), csVector3(trans_x,trans_y,trans_z));

    if (state.IsValid())
    {
        csRef<iSpriteCal3DSocket> cal3DSocket = state->FindSocket(socket);
        if (cal3DSocket.IsValid())
        {
            meshWrap->QuerySceneNode()->SetParent( baseMesh->QuerySceneNode ());
            cal3DSocket->SetMeshWrapper(meshWrap);
            cal3DSocket->SetTransform(transform);
        }
    }
    else if (animeshObject.IsValid() && animeshFactory.IsValid())
    {
        size_t idx = animeshFactory->FindSocket(socket);
        if (idx != (size_t)-1)
        {
            meshWrap->QuerySceneNode()->SetParent( baseMesh->QuerySceneNode ());
            csRef<iAnimatedMeshSocket> animeshSocket = animeshObject->GetSocket(idx);
            animeshSocket->SetSceneNode(meshWrap->QuerySceneNode());
            animeshSocket->SetTransform(transform);
        }
    }

    usedSlots.PushSmart(socket);
}

bool psCharAppearance::ProcessMaterial(csRef<iMaterialWrapper> material, const char* materialName, const char* partName)
{
    bool success = false;
    if (state.IsValid())
    {
        success = state->SetMaterial(partName, material);
    }
    else if (animeshObject.IsValid() && animeshFactory.IsValid())
    {
        size_t idx = animeshFactory->FindSubMesh(partName);
        if (idx != (size_t)-1)
        {
            animeshObject->GetSubMesh(idx)->SetMaterial(material);
            success = true;
        }
    }

    return success;
}

void psCharAppearance::ProcessAttach(csRef<iMaterialWrapper> material, const char* materialName, const char* partName)
{
    if (ProcessMaterial(material, materialName, partName))
    {
        csString & mat = materials.GetOrCreate(partName);
        if (!mat.IsEmpty())
        {
            // free loader ressources
            psengine->GetLoader()->FreeMaterial(mat);
        }
        mat = materialName;
    }
    else
    {
        // Try mirroring
        csString left, right;
        left.Format("Left %s", partName);
        right.Format("Right %s", partName);

        size_t success = 0;

        if (ProcessMaterial(material, materialName, partName))
        {
            ++success;
            csString & mat = materials.GetOrCreate(left);
            if (!mat.IsEmpty())
            {
                // free loader ressources
                psengine->GetLoader()->FreeMaterial(mat);
            }
            mat = materialName;
        }

        if (ProcessMaterial(material, materialName, partName))
        {
            ++success;
            csString & mat = materials.GetOrCreate(right);
            if (!mat.IsEmpty())
            {
                // free loader ressources
                psengine->GetLoader()->FreeMaterial(mat);
            }
            mat = materialName;
        }

        if (success < 2)
        {
            Error3("Failed to set material \"%s\" on part \"%s\"", materialName, partName);
        }

        if (success < 1)
        {
            // free loader ressources
            psengine->GetLoader()->FreeMaterial(materialName);
        }
        else if (success > 1)
        {
            // we use it twice, but only got 1 reference, therefore we get another one
            psengine->GetLoader()->LoadMaterial(materialName);
        }
    }
}

bool psCharAppearance::CheckLoadStatus()
{
    if(!delayedAttach.IsEmpty())
    {
        Attachment & attach = delayedAttach.Front();

        if(attach.factory)
        {
            csRef<iMeshFactoryWrapper> & factory = attach.factoryPtr;
            if(!factory.IsValid())
            {
                factory = psengine->GetLoader()->LoadFactory(attach.factName);
            }

            if(factory.IsValid())
            {
                if(!attach.materialName.IsEmpty())
                {
                    csRef<iMaterialWrapper> & material = attach.materialPtr;

                    if(!material.IsValid())
                    {
                        material = psengine->GetLoader()->LoadMaterial(attach.materialName);
                    }

                    if(material.IsValid())
                    {
                        ProcessAttach(factory, material, attach.factName, attach.socket);
                        csString & mat = materials.GetOrCreate(attach.socket);
                        if(!mat.IsEmpty()) // free previously allocated loader ressource
                        {
                            psengine->GetLoader()->FreeMaterial(attach.materialName);
                        }
                        mat = attach.materialName;
                        delayedAttach.PopFront();
                    }
                }
                else
                {
                    ProcessAttach(factory, NULL, attach.factName, attach.socket);
                    delayedAttach.PopFront();
                }
            }
        }
        else
        {
            csRef<iMaterialWrapper> & material = attach.materialPtr;

            if(!material.IsValid())
            {
                material = psengine->GetLoader()->LoadMaterial(attach.materialName);
            }

            if(material.IsValid())
            {
                ProcessAttach(material, attach.materialName, attach.partName);
                delayedAttach.PopFront();
            }
        }

        return true;
    }
    else
    {
        psengine->UnregisterDelayedLoader(this);
        return false;
    }
}

void psCharAppearance::ApplyTraits(const csString& traitString)
{
    if ( traitString.Length() == 0 )
    {
        return;
    }

    csRef<iDocument> doc = xmlparser->CreateDocument();

    const char* traitError = doc->Parse(traitString);
    if ( traitError )
    {
        Error2("Error in XML: %s", traitError );
        return;

    }

    csRef<iDocumentNodeIterator> traitIter = doc->GetRoot()->GetNode("traits")->GetNodes("trait");

    csPDelArray<Trait> traits;

    // Build traits table
    while ( traitIter->HasNext() )
    {
        csRef<iDocumentNode> traitNode = traitIter->Next();

        Trait * trait = new Trait;
        trait->Load(traitNode);
        traits.Push(trait);
    }

    // Build next and prev pointers for trait sets
    csPDelArray<Trait>::Iterator iter = traits.GetIterator();
    while (iter.HasNext())
    {
        Trait * trait = iter.Next();

        csPDelArray<Trait>::Iterator iter2 = traits.GetIterator();
        while (iter2.HasNext())
        {
            Trait * trait2 = iter2.Next();
            if (trait->next_trait_uid == trait2->uid)
            {
                trait->next_trait = trait2;
                trait2->prev_trait = trait;
            }
        }
    }

    // Find top traits and set them on mesh
    csPDelArray<Trait>::Iterator iter3 = traits.GetIterator();
    while (iter3.HasNext())
    {
        Trait * trait = iter3.Next();
        if (trait->prev_trait == NULL)
        {
            if (!SetTrait(trait))
            {
                Error2("Failed to set trait %s for mesh.", traitString.GetData());
            }
        }
    }
    return;

}


bool psCharAppearance::SetTrait(Trait * trait)
{
    bool result = true;

    while (trait)
    {
        switch (trait->location)
        {
            case PSTRAIT_LOCATION_SKIN_TONE:
            {
                SetSkinTone(trait->mesh, trait->material);
                break;
            }

            case PSTRAIT_LOCATION_FACE:
            {
                FaceTexture(trait->material);
                break;
            }


            case PSTRAIT_LOCATION_HAIR_STYLE:
            {
                HairMesh(trait->mesh);
                break;
            }


            case PSTRAIT_LOCATION_BEARD_STYLE:
            {
                BeardMesh(trait->mesh);
                break;
            }


            case PSTRAIT_LOCATION_HAIR_COLOR:
            {
                HairColor(trait->shader);
                break;
            }

            case PSTRAIT_LOCATION_EYE_COLOR:
            {
                EyeColor(trait->shader);
                break;
            }


            default:
            {
                Error3("Trait(%d) unknown trait location %d",trait->uid,trait->location);
                result = false;
                break;
            }
        }
        trait = trait->next_trait;
    }

    return true;
}

void psCharAppearance::DefaultMaterial(csString& part)
{
    bool skinToneSetFound = false;

    for ( size_t z = 0; z < skinToneSet.GetSize(); z++ )
    {
        if ( part == skinToneSet[z].part )
        {
            skinToneSetFound = true;
            ChangeMaterial(part, skinToneSet[z].material);
        }
    }

    // Set stateFactory defaults if no skinToneSet found.
    if (!skinToneSetFound)
    {
        if(stateFactory.IsValid())
        {
            ChangeMaterial(part, stateFactory->GetDefaultMaterial(part));
        }
        else if(animeshFactory.IsValid())
        {
            ChangeMaterial(part, 0);
        }
    }
}


void psCharAppearance::ClearEquipment(const char* slot)
{
    if(slot)
    {
        psengine->GetEffectManager()->DeleteEffect(effectids.Get(slot, 0));
        psengine->GetEffectManager()->DetachLight(lightids.Get(slot, 0));
        effectids.DeleteAll(slot);
        lightids.DeleteAll(slot);
        return;
    }

    if(psengine->GetEffectManager())
    {
        csHash<int, csString>::GlobalIterator effectItr = effectids.GetIterator();
        while(effectItr.HasNext())
        {
            psengine->GetEffectManager()->DeleteEffect(effectItr.Next());
        }
        effectids.Empty();

        csHash<int, csString>::GlobalIterator lightItr = lightids.GetIterator();
        while(lightItr.HasNext())
        {
            psengine->GetEffectManager()->DetachLight(lightItr.Next());
        }
        lightids.Empty();
    }

    csArray<csString> deleteList = usedSlots;
    for ( size_t z = 0; z < deleteList.GetSize(); z++ )
    {
        if(!Detach(deleteList[z]))
        {
            Error2("failed to detach slot %s", deleteList[z].GetData());
        }
    }
}


bool psCharAppearance::Detach(const char* socketName, bool removeItem )
{
    if (!socketName || usedSlots.Find(socketName) == csArrayItemNotFound || (!state.IsValid() && !animeshObject.IsValid()))
    {
        return false;
    }

    csRef<iMeshWrapper> meshWrap;
    if (state.IsValid())
    {
        csRef<iSpriteCal3DSocket> socket = state->FindSocket(socketName);
        if (socket.IsValid())
        {
            meshWrap = socket->GetMeshWrapper();
            socket->SetMeshWrapper(0);
        }
        else
        {
            Notify2(LOG_CHARACTER, "Socket %s not found.", socketName);
        }
    }
    else if (animeshObject.IsValid() && animeshFactory.IsValid())
    {
        size_t idx = animeshFactory->FindSocket(socketName);
        if(idx != (size_t)-1)
        {
            csRef<iAnimatedMeshSocket> socket = animeshObject->GetSocket(idx);
            meshWrap = socket->GetSceneNode()->QueryMesh();
            socket->SetSceneNode(0);
        }
        else
        {
            Notify2(LOG_CHARACTER, "Socket %s not found.", socketName);
        }
    }

    if (!meshWrap)
    {
        Notify2(LOG_CHARACTER, "No mesh in socket: %s.", socketName );
    }
    else
    {
        meshWrap->QuerySceneNode()->SetParent (0);

        csString material = materials.Get(socketName, "");
        csString factory;

        iMeshFactoryWrapper* fact = meshWrap->GetFactory();

        // check whether we have a factory to free
        if(fact)
        {
            factory = fact->QueryObject()->GetName();
        }

        if(removeItem)
        {
            engine->RemoveObject(meshWrap);
        }
        meshWrap.Invalidate();

        // free loader ressources
        if(factory)
        {
            psengine->GetLoader()->FreeFactory(factory);
        }

        if(!material.IsEmpty())
        {
            psengine->GetLoader()->FreeMaterial(material);
            materials.DeleteAll(socketName);
        }
    }

    usedSlots.Delete(socketName);
    return true;
}


void psCharAppearance::Clone(psCharAppearance* clone)
{
    this->eyeMesh       = clone->eyeMesh;
    this->hairMesh      = clone->hairMesh;
    this->beardMesh     = clone->beardMesh;

    this->eyeShader     = clone->eyeShader;
    this->hairShader    = clone->hairShader;
    this->faceMaterial  = clone->faceMaterial;
    this->skinToneSet   = clone->skinToneSet;
    this->eyeColorSet   = clone->eyeColorSet;
    this->hairColorSet  = clone->hairColorSet;
    this->effectids     = clone->effectids;
}

void psCharAppearance::SetSneak(bool sneaking)
{
    if(sneak != sneaking)
    {
        sneak = sneaking;

        // Apply to base/core mesh.
        if(sneaking)
        {
            baseMesh->SetRenderPriority(engine->GetRenderPriority("alpha"));
        }
        else
        {
            baseMesh->SetRenderPriority(engine->GetRenderPriority("object"));
        }

        CS::ShaderVarStringID varName = stringSet->Request("alpha factor");
        for(uint i=0; i<meshCount; i++)
        {
            iShaderVariableContext* context = NULL;
            if(state.IsValid())
            {
                context = state->GetCoreMeshShaderVarContext(meshNames[i]);
            }
            else if(animeshObject.IsValid() && animeshFactory.IsValid())
            {
                size_t idx = animeshFactory->FindSubMesh(meshNames[i]);
                if(idx != (size_t)-1)
                {
                    context = animeshObject->GetSubMesh(idx)->GetShaderVariableContext(0);
                }
            }

            if(context)
            {
                csShaderVariable* var = context->GetVariableAdd(varName);
                if(var)
                {
                  if(sneaking)
                  {
                      var->SetValue(0.5f);
                  }
                  else
                  {
                      var->SetValue(1.0f);
                  }
                }
            }
        }

        // Apply to equipped meshes too.
        for(size_t i=0; i < usedSlots.GetSize(); ++i)
        {
            csRef<iMeshWrapper> meshWrap;
            if(state.IsValid())
            {
                iSpriteSocket *socket = state->FindSocket(usedSlots[i]);
                if(socket)
                    meshWrap = socket->GetMeshWrapper();
            }
            else if(animeshObject.IsValid() && animeshFactory.IsValid())
            {
                size_t idx = animeshFactory->FindSocket(usedSlots[i]);
                if(idx != (size_t)-1)
                {
                    iSceneNode* node = animeshObject->GetSocket(idx)->GetSceneNode();
                    if(node)
                    {
                        meshWrap = node->QueryMesh();
                    }
                }
            }

            if(meshWrap.IsValid())
            {
                csShaderVariable* var = meshWrap->GetSVContext()->GetVariableAdd(varName);

                if(sneaking)
                {
                    meshWrap->SetRenderPriority(engine->GetRenderPriority("alpha"));
                    var->SetValue(0.5f);
                }
                else
                {
                    meshWrap->SetRenderPriority(engine->GetRenderPriority("object"));
                    var->SetValue(1.0f);
                }
            }
        }
    }
}
