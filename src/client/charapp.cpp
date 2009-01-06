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
#include "util/log.h"
#include "util/psstring.h"
#include "effects/pseffect.h"
#include "effects/pseffectmanager.h"
#include "engine/loader.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "charapp.h"
#include "psengine.h"
#include "clientcachemanager.h"
#include "pscelclient.h"
#include "psclientchar.h"
#include "globals.h"


psCharAppearance::psCharAppearance(iObjectRegistry* objectReg)
{
    stringSet = csQueryRegistryTagInterface<iShaderVarStringSet>(objectReg, "crystalspace.shader.variablenameset");
    engine = csQueryRegistry<iEngine>(objectReg);
    vfs    = csQueryRegistry<iVFS>(objectReg);
    g3d    = csQueryRegistry<iGraphics3D>(objectReg);
    txtmgr = g3d->GetTextureManager();
    xmlparser =  csQueryRegistry<iDocumentSystem> (objectReg);

    eyeMesh = "Eyes";
    hairMesh = "Hair";
    beardMesh = "Beard";

    eyeColorSet = false;
    hairAttached = true;
    hairColorSet = false;
}

psCharAppearance::~psCharAppearance()
{
    ClearEquipment();
    psengine->UnregisterDelayedLoader(this);
}

void psCharAppearance::SetMesh(iMeshWrapper* mesh)
{
    state = scfQueryInterface<iSpriteCal3DState>(mesh->GetMeshObject());
    stateFactory = scfQueryInterface<iSpriteCal3DFactoryState>(mesh->GetMeshObject()->GetFactory());

    baseMesh = mesh;
}


csString psCharAppearance::ParseStrings(const char* part, const char* str) const
{
    psString result(str);

    const char* factname = baseMesh->GetFactory()->QueryObject()->GetName();

    result.ReplaceAllSubString("$F", factname);
    result.ReplaceAllSubString("$P", part);

    return result;
}


void psCharAppearance::FaceTexture(csString& faceMaterial, csString& faceTexture)
{
    csString materialParsed = ParseStrings("", faceMaterial);
    csString textureParsed  = ParseStrings("", faceTexture);

    iMaterialWrapper* material = Loader::GetSingleton().LoadMaterial(materialParsed, textureParsed);

    if ( !material )
    {
            Notify3(LOG_CHARACTER, "Failed to load texture ( %s, %s )", faceMaterial.GetData(), faceTexture.GetData());
            return;
    }
    else
    {
        if ( state )
        {
            state->SetMaterial("Head", material);
        }
    }

}

void psCharAppearance::BeardMesh(csString& subMesh)
{
    beardMesh = subMesh;

    if ( beardMesh.Length() == 0 )
    {
        for ( int idx=0; idx < stateFactory->GetMeshCount(); idx++)
        {
            const char* meshName = stateFactory->GetMeshName(idx);

            if ( strstr(meshName, "Beard") )
            {
                state->DetachCoreMesh(meshName);
            }
        }
        return;
    }

    csString newPartParsed = ParseStrings("Beard", beardMesh);

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

    if ( hairColorSet )
        HairColor(hairShader);
}

void psCharAppearance::HairMesh(csString& subMesh)
{
    hairMesh = subMesh;

    if ( hairMesh.Length() == 0 )
    {
        hairMesh = "Hair";
    }

    csString newPartParsed = ParseStrings("Hair", hairMesh);

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
        hairAttached = true;
        hairMesh = newPartParsed;
    }

    if ( hairColorSet )
        HairColor(hairShader);
}


void psCharAppearance::HairColor(csVector3& color)
{
    if ( hairMesh.Length() == 0 )
    {
        return;
    }
    else
    {
        hairShader = color;
        iShaderVariableContext* context_hair = state->GetCoreMeshShaderVarContext(hairMesh);
        iShaderVariableContext* context_beard = state->GetCoreMeshShaderVarContext(beardMesh);

        if ( context_hair )
        {
            CS::ShaderVarStringID varName = stringSet->Request("color modulation");
            csShaderVariable* var = context_hair->GetVariableAdd(varName);

            if ( var )
            {
                var->SetValue(hairShader);
            }
        }

        if ( context_beard )
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
}

void psCharAppearance::EyeColor(csVector3& color)
{
    eyeShader = color;
    iShaderVariableContext* context_eyes = state->GetCoreMeshShaderVarContext(eyeMesh);

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

void psCharAppearance::ShowHair(bool show)
{
    if (show)
    {
        if (hairAttached)
            return;

        state->AttachCoreMesh(hairMesh);

        if (hairColorSet)
            HairColor(hairShader);

        hairAttached = true;
    }
    else
    {
        state->DetachCoreMesh(hairMesh);
        hairAttached = false;
    }
}

void psCharAppearance::SetSkinTone(csString& part, csString& material, csString& texture)
{
    if (!baseMesh || !part || !material || !texture)
    {
        return;
    }
    else
    {
        SkinToneSet s;
        s.part = part;
        s.material = material;
        s.texture = texture;

        skinToneSet.Push(s);

        csString materialNameParsed    = ParseStrings(part, material);
        csString textureNameParsed     = ParseStrings(part, texture);

        iMaterialWrapper* material = Loader::GetSingleton().LoadMaterial(materialNameParsed, textureNameParsed );
        if ( !material )
        {
            // Not necisarily an error; this texture may just not exist for this character, yet
            Notify3(LOG_CHARACTER,"Failed to load texture \"%s\" for part \"%s\"",textureNameParsed.GetData(),part.GetData());
            return;
        }

        if ( !state->SetMaterial(part,material) )
        {
            csString left,right;
            left.Format("Left %s",part.GetData());
            right.Format("Right %s",part.GetData());

            // Try mirroring
            if ( !state->SetMaterial(left,material) || !state->SetMaterial(right,material) )
            {
                Error3("Failed to set material \"%s\" on part \"%s\"",materialNameParsed.GetData(),part.GetData());
                return;
            }
        }
    }
}


void psCharAppearance::ApplyEquipment(csString& equipment)
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

    // Do the helm check.
    csRef<iDocumentNode> helmNode = doc->GetRoot()->GetNode("equiplist")->GetNode("helm");
    csString helmGroup(helmNode->GetContentsValue());
    if ( helmGroup.Length() == 0 )
        helmGroup = baseMesh->GetFactory()->QueryObject()->GetName();

    csRef<iDocumentNodeIterator> equipIter = doc->GetRoot()->GetNode("equiplist")->GetNodes("equip");

    while (equipIter->HasNext())
    {
        csRef<iDocumentNode> equipNode = equipIter->Next();
        csString slot = equipNode->GetAttributeValue( "slot" );
        csString mesh = equipNode->GetAttributeValue( "mesh" );
        csString part = equipNode->GetAttributeValue( "part" );
        csString partMesh = equipNode->GetAttributeValue("partMesh");
        csString texture = equipNode->GetAttributeValue( "texture" );

        //If the mesh has a $H it means it's an helm so search for replacement
        mesh.ReplaceAll("$H",helmGroup);

        Equip(slot, mesh, part, partMesh, texture);
    }

    return;
}


void psCharAppearance::Equip( csString& slotname,
                              csString& mesh,
                              csString& part,
                              csString& subMesh,
                              csString& texture
                             )
{

    if ( slotname == "helm" )
    {
        ShowHair(false);
    }

    // If it's a new mesh attach that mesh.
    if ( mesh.Length() )
    {
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
            ChangeMaterial( ParseStrings(part,subMesh),texture, texture);
        }
    }
    else if ( part.Length() )
    {
        ChangeMaterial(part, texture, texture);
    }
}


bool psCharAppearance::Dequip(csString& slotname,
                              csString& mesh,
                              csString& part,
                              csString& subMesh,
                              csString& texture)
{
    if ( slotname == "helm" )
    {
         ShowHair(true);
    }

    if ( mesh.Length() )
    {
        Detach(slotname);
    }

    // This is a part mesh (ie Mesh) set default mesh for that part.

    if ( subMesh.Length() )
    {
        DefaultMesh(part);
    }

    if ( part.Length() )
    {
        if ( texture.Length() )
        {
            ChangeMaterial(part, texture, texture);
        }
        else
        {
            DefaultMaterial(part);
        }
        DefaultMaterial(part);
    }

    ClearEquipment(slotname);

    return true;
}


void psCharAppearance::DefaultMesh(const char* part)
{
    const char * defaultPart = NULL;
    /* First we detach every mesh that match the partPattern */
    for (int idx=0; idx < stateFactory->GetMeshCount(); idx++)
    {
        const char * meshName = stateFactory->GetMeshName( idx );
        if (strstr(meshName, part))
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


bool psCharAppearance::ChangeMaterial(const char* part, const char* meshName, const char* textureName )
{
    if ( !part || !meshName || !textureName)
        return false;

    csString meshNameParsed    = ParseStrings(part, meshName);
    csString textureNameParsed = ParseStrings(part, textureName);

    iMaterialWrapper* material = Loader::GetSingleton().LoadMaterial( meshNameParsed, textureNameParsed );
    if ( !material )
    {
        // Not necisarily an error; this texture may just not exist for this character, yet
        Notify3(LOG_CHARACTER,"Failed to load texture \"%s\" for part \"%s\"",textureNameParsed.GetData(),part);
        return false;
    }

    if ( !state->SetMaterial(part,material) )
    {
        csString left,right;
        left.Format("Left %s",part);
        right.Format("Right %s",part);

        // Try mirroring
        if ( !state->SetMaterial(left,material) || !state->SetMaterial(right,material) )
        {
             Error3("Failed to set material \"%s\" on part \"%s\"",meshNameParsed.GetData(),part);
             return false;
        }
    }

    return true;
}


bool psCharAppearance::ChangeMesh(const char* partPattern, const char* newPart)
{
    csString newPartParsed = ParseStrings(partPattern, newPart);

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
    return true;
}


bool psCharAppearance::Attach(const char* socketName, const char* meshFactName)
{
    if (!socketName || !meshFactName)
        return false;


    csRef<iSpriteCal3DSocket> socket = state->FindSocket( socketName );
    if ( !socket )
    {
        Notify2(LOG_CHARACTER, "Socket %s not found.", socketName );
        return false;
    }

    csRef<iMeshFactoryWrapper> factory = engine->GetMeshFactories()->FindByName (meshFactName);
    if ( !factory )
    {
        // Try loading the mesh again
        csString filename;
        if (!psengine->GetFileNameByFact(meshFactName, filename))
        {
            Error2("Mesh Factory %s not found", meshFactName );
            return false;
        }

        // Check for factory and set up callback if not loaded yet.
        FactoryIndexEntry* fie = psengine->GetCacheManager()->GetFactoryEntry(filename);
        if(!fie)
        {
            Attachment attach;
            attach.filename = filename;
            attach.socket = socket;
            if(delayedAttach.IsEmpty())
            {
                psengine->RegisterDelayedLoader(this);
            }
            delayedAttach.PushBack(attach);
        }
        else
        {
            factory = fie->factory;
            ProcessAttach(factory, meshFactName, socket);
        }
    }
    else
    {
        ProcessAttach(factory, meshFactName, socket);
    }

    return true;
}

void psCharAppearance::ProcessAttach(csRef<iMeshFactoryWrapper> factory, const char* meshFactName, csRef<iSpriteCal3DSocket> socket)
{
     csRef<iMeshWrapper> meshWrap = engine->CreateMeshWrapper( factory, meshFactName );
     const char* socketName = socket->GetName();

    // Given a socket name of "righthand", we're looking for a key in the form of "socket_righthand"
    csString keyName = "socket_";
    keyName += socketName;

    // Variables for transform to be specified
    float trans_x = 0, trans_y = 0.0, trans_z = 0, rot_x = -PI/2, rot_y = 0, rot_z = 0;
    csRef<iObjectIterator> it = factory->QueryObject()->GetIterator();

    while ( it->HasNext() )
    {
        csRef<iKeyValuePair> key ( scfQueryInterface<iKeyValuePair> (it->Next()));
        if (key && keyName == key->GetKey())
        {
            sscanf(key->GetValue(),"%f,%f,%f,%f,%f,%f",&trans_x,&trans_y,&trans_z,&rot_x,&rot_y,&rot_z);
        }
    }

    meshWrap->QuerySceneNode()->SetParent( baseMesh->QuerySceneNode ());
    socket->SetMeshWrapper( meshWrap );
    socket->SetTransform( csTransform(csZRotMatrix3(rot_z)*csYRotMatrix3(rot_y)*csXRotMatrix3(rot_x), csVector3(trans_x,trans_y,trans_z)) );

    usedSlots.PushSmart(socketName);

    psengine->GetCelClient()->HandleItemEffect(factory->QueryObject()->GetName(), socket->GetMeshWrapper(), false, socketName, &effectids, &lightids);
}

void psCharAppearance::CheckMeshLoad()
{
    if(!delayedAttach.IsEmpty())
    {
        Attachment attach = delayedAttach.Front();
        FactoryIndexEntry* fie = psengine->GetCacheManager()->GetFactoryEntry(attach.filename);
        if(fie)
        {
            ProcessAttach(fie->factory, fie->factname, attach.socket);
            delayedAttach.PopFront();
        }
    }
    else
    {
        psengine->UnregisterDelayedLoader(this);
    }
}

void psCharAppearance::ApplyTraits(csString& traitString)
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
                SetSkinTone(trait->mesh, trait->material, trait->texture);
                break;
            }

            case PSTRAIT_LOCATION_FACE:
            {
                FaceTexture(trait->material, trait->texture );
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
            ChangeMaterial(part, skinToneSet[z].material, skinToneSet[z].texture);
        }
    }

    // Set stateFactory defaults if no skinToneSet found.
    if ( !skinToneSetFound )
    {
        ChangeMaterial(part, stateFactory->GetDefaultMaterial(part), stateFactory->GetDefaultMaterial(part));
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

    csArray<csString> deleteList = usedSlots;

    for ( size_t z = 0; z < deleteList.GetSize(); z++ )
    {
        Detach(deleteList[z]);
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
            psengine->GetEffectManager()->DeleteEffect(lightItr.Next());
        }
        lightids.Empty();
    }
}


bool psCharAppearance::Detach(const char* socketName )
{
    if (!socketName)
    {
        return false;
    }


    csRef<iSpriteCal3DSocket> socket = state->FindSocket( socketName );
    if ( !socket )
    {
        Notify2(LOG_CHARACTER, "Socket %s not found.", socketName );
        return false;
    }

    csRef<iMeshWrapper> meshWrap = socket->GetMeshWrapper();
    if ( !meshWrap )
    {
        Notify2(LOG_CHARACTER, "No mesh in socket: %s.", socketName );
    }
    else
    {
        meshWrap->QuerySceneNode ()->SetParent (0);
        socket->SetMeshWrapper( NULL );
        engine->RemoveObject( meshWrap );
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
    this->hairAttached  = clone->hairAttached;
    this->hairColorSet  = clone->hairColorSet;
    this->effectids     = clone->effectids;
}
