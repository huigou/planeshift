/*
 * psmeshutil.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "util/psmeshutil.h"
#include "util/psstring.h"
#include "util/log.h"

#include <iutil/object.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/scenenode.h>
#include <iengine/texture.h>
#include <iengine/region.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <imap/loader.h>
#include <ivaria/keyval.h>
#include <ivideo/texture.h>
#include "clientcachemanager.h"
#include "globals.h"

psMeshUtil::psMeshUtil(iObjectRegistry *obj_reg)
{
    engine =  csQueryRegistry<iEngine> (obj_reg);
    loader =  csQueryRegistry<iLoader> (obj_reg);
    vfs    =  csQueryRegistry<iVFS> (obj_reg);
    g3d    =  csQueryRegistry<iGraphics3D> (obj_reg);
    txtmgr = g3d->GetTextureManager();
}

iMaterialWrapper* psMeshUtil::LoadMaterial(const char* name, const char* texture, iRegion* region)
{
    if (!name || !texture)
        return NULL;

    iMaterialWrapper* mat_wrapper = engine->FindMaterial(name);
    if ( mat_wrapper )
        return mat_wrapper;

    // Check to see if we even have this texture
    if ( !vfs->Exists(texture) )
        return NULL;

    csRef<iTextureHandle> th = loader->LoadTexture(texture);
    iTextureList *txlist = engine->GetTextureList();
    if ( !th || !txlist )
        return NULL;

    // Add the texture to the engine's list of textures
    iTextureWrapper* tex_wrapper = txlist->NewTexture(th);
    if ( !tex_wrapper )
        return NULL;

    // Create the material and register it with the engine
    mat_wrapper = engine->CreateMaterial(name,tex_wrapper);

    if (region && mat_wrapper)
        region->Add(mat_wrapper->QueryObject());

    return mat_wrapper;
}

csString psMeshUtil::ParseStrings(iMeshWrapper* mesh, const char * part, const char* str) const
{
    psString result(str);
    const char* factname = mesh->GetFactory()->QueryObject()->GetName();

    result.ReplaceAllSubString("$F",factname);
    result.ReplaceAllSubString("$P",part);

    return result;
}

bool psMeshUtil::ChangeMaterial( iMeshWrapper* mesh, const char* part, const char* meshName, const char* textureName )
{
    if (!mesh || !part || !meshName || !textureName)
        return false;

    csRef<iSpriteCal3DState> state =  scfQueryInterface<iSpriteCal3DState > ( mesh->GetMeshObject());
    if ( !state )
        return false;

    csString meshNameParsed    = ParseStrings(mesh, part, meshName);
    csString textureNameParsed = ParseStrings(mesh, part, textureName);

    iMaterialWrapper* material = LoadMaterial( meshNameParsed, textureNameParsed );
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

bool psMeshUtil::DefaultMaterial( iMeshWrapper* mesh, const char* part )
{
    if (!mesh || !part)
        return false;

    csRef<iSpriteCal3DFactoryState> state =  scfQueryInterface<iSpriteCal3DFactoryState > ( mesh->GetMeshObject()->GetFactory());
    if ( !state )
        return false;

    const char* materialName = state->GetDefaultMaterial( part );

    if (materialName)
    {
        return ChangeMaterial( mesh, part, materialName, materialName );
    }
    else // Try mirroring
    {
        csString left,right;
        left.Format("Left %s",part);
        right.Format("Right %s",part);

        const char* leftMaterialName = state->GetDefaultMaterial( left );
        const char* rightMaterialName = state->GetDefaultMaterial( right );

        bool L = ChangeMaterial( mesh, left, leftMaterialName, leftMaterialName );
        bool R = ChangeMaterial( mesh, right, rightMaterialName, rightMaterialName );

        return (L && R);
    }
}

bool psMeshUtil::ChangeMesh( iMeshWrapper* mesh, const char* partPattern, const char* newPart)
{
    csRef<iSpriteCal3DFactoryState> statefac =  scfQueryInterface<iSpriteCal3DFactoryState > ( mesh->GetMeshObject()->GetFactory());
    if ( !statefac )
    {
        return false;
    }

    csRef<iSpriteCal3DState> state =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
    if ( !state )
    {
        return false;
    }

    csString newPartParsed = ParseStrings(mesh, partPattern, newPart);

    // If the new mesh cannot be found then do nothing.   
    int newMeshAvailable = statefac->FindMeshName(newPartParsed);
    if ( newMeshAvailable == -1 )
        return false;
    
    /* First we detach every mesh that match the partPattern */
    for (int idx=0; idx < statefac->GetMeshCount(); idx++)
    {
        const char * meshName = statefac->GetMeshName( idx );
        if (strstr(meshName,partPattern))
        {     
            state->DetachCoreMesh( meshName );
        }
    }
    
    state->AttachCoreMesh( newPartParsed );

    return true;
}

bool psMeshUtil::DefaultMesh( iMeshWrapper* mesh, const char* partPattern )
{
    csRef<iSpriteCal3DFactoryState> statefac =  scfQueryInterface<iSpriteCal3DFactoryState > ( mesh->GetMeshObject()->GetFactory());
    if ( !statefac )
    {
        return false;
    }

    csRef<iSpriteCal3DState> state =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
    if ( !state )
    {
        return false;
    }

    const char * defaultPart = NULL;
    /* First we detach every mesh that match the partPattern */
    for (int idx=0; idx < statefac->GetMeshCount(); idx++)
    {
        const char * meshName = statefac->GetMeshName( idx );
        if (strstr(meshName,partPattern))
        {
            state->DetachCoreMesh( meshName );
            if (statefac->IsMeshDefault(idx))
            {
                defaultPart = meshName;
            }
        }
    }

    if (!defaultPart) 
    {
        return false;
    }
    
    state->AttachCoreMesh( defaultPart );

    return true;
}


bool psMeshUtil::Attach( iMeshWrapper* mesh, const char* socketName, const char* meshFactName )
{
    if (!mesh || !socketName || !meshFactName)
        return false;

    csRef<iSpriteCal3DState> state =  scfQueryInterface<iSpriteCal3DState > ( mesh->GetMeshObject());
    if ( !state )
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
        psengine->GetCacheManager()->LoadNewFactory(filename);
        factory = psengine->GetEngine()->GetMeshFactories()->FindByName (meshFactName);      
        if (!factory)
        {
            Error2("Mesh Factory %s not found", meshFactName );
            return false;
        }
    }

    csRef<iMeshWrapper> meshWrap = engine->CreateMeshWrapper( factory, meshFactName );

    // Given a socket name of "righthand", we're looking for a key in the form of "socket_righthand"
    char * keyName = (char *)malloc(strlen(socketName)+strlen("socket_")+1);
    strcpy(keyName,"socket_");
    strcat(keyName,socketName);

    // Variables for transform to be specified
    float trans_x = 0, trans_y = 0.0, trans_z = 0, rot_x = -PI/2, rot_y = 0, rot_z = 0;
    csRef<iObjectIterator> it = factory->QueryObject()->GetIterator();

    while ( it->HasNext() )
    {
        csRef<iKeyValuePair> key ( scfQueryInterface<iKeyValuePair> (it->Next()));
        if (key && strcmp(key->GetKey(),keyName) == 0)
        {
            sscanf(key->GetValue(),"%f,%f,%f,%f,%f,%f",&trans_x,&trans_y,&trans_z,&rot_x,&rot_y,&rot_z);
        }
    }

    free(keyName);
    keyName = NULL;

    meshWrap->QuerySceneNode()->SetParent( mesh->QuerySceneNode ());
    socket->SetMeshWrapper( meshWrap );
    socket->SetTransform( csTransform(csZRotMatrix3(rot_z)*csYRotMatrix3(rot_y)*csXRotMatrix3(rot_x), csVector3(trans_x,trans_y,trans_z)) );

    return true;
}

bool psMeshUtil::Detach( iMeshWrapper* mesh, const char* socketName )
{
    if (!mesh || !socketName)
        return false;

    csRef<iSpriteCal3DState> state =  scfQueryInterface<iSpriteCal3DState > ( mesh->GetMeshObject());
    if ( !state )
        return false;

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

    return true;
}
