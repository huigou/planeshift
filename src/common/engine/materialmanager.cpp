/*
 *  materialmanager.cpp - Author: Mike Gist
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <iutil/stringarray.h>
#include <iutil/object.h>
#include <ivideo/material.h>

#include "materialmanager.h"
#include "util/strutil.h"
#include "globals.h"

MaterialManager::MaterialManager(iObjectRegistry* _object_reg, bool _keepModels) : scfImplementationType (this)
{
    object_reg = _object_reg;
    csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D> (object_reg);    
    txtmgr = g3d->GetTextureManager();
    engine = csQueryRegistry<iEngine> (object_reg);
    loader = csQueryRegistry<iThreadedLoader> (object_reg);
    vfs = csQueryRegistry<iVFS> (object_reg);
    keepModels = _keepModels;
}

iMaterialWrapper* MaterialManager::MissingMaterial(const char *name, const char *filename)
{
    iMaterialWrapper* materialWrap = engine->GetMaterialList()->FindByName(name);
    if(!materialWrap)
    {
        // Check that the texture exists.
        if (!vfs->Exists(filename))
            return NULL;

        iTextureWrapper* texture = MissingTexture(name, filename);

        csRef<iMaterial> material (engine->CreateBaseMaterial(texture));
        materialWrap = engine->GetMaterialList()->NewMaterial(material, name);
    }
    return materialWrap;
}

iTextureWrapper* MaterialManager::MissingTexture(const char *name, const char *filename)
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
        csRef<iThreadReturn> itr = loader->LoadTexture(name, filename, CS_TEXTURE_3D, txtmgr, true, false);
        itr->Wait();
        texture = scfQueryInterfaceSafe<iTextureWrapper>(itr->GetResultRefPtr());
        engine->SyncEngineListsNow(loader);
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

bool MaterialManager::LoadTextureDir(const char *dir)
{
    csRef<iDataBuffer> xpath = vfs->ExpandPath(dir);
    csRef<iStringArray> files = vfs->FindFiles( **xpath );

    if (!files)
        return false;

    for (size_t i=0; i < files->GetSize(); i++)
    {
        const char* filename = files->Get(i);
        if (strcmp (filename + strlen(filename) - 4, ".png") && 
            strcmp (filename + strlen(filename) - 4, ".tga") && 
            strcmp (filename + strlen(filename) - 4, ".gif") && 
            strcmp (filename + strlen(filename) - 4, ".bmp") && 
            strcmp (filename + strlen(filename) - 4, ".jpg") &&
            strcmp (filename + strlen(filename) - 4, ".dds"))
            continue;

        // If this is an icon type texture then not required to load as a
        // material. 
        if ( strstr( filename, "_icon" ) )
            continue;
        
        const char* name = csStrNew(filename);
        const char* onlyname = PS_GetFileName(name);

        if (!MissingMaterial(onlyname,filename))
        {
            delete[] name;
            return false;
        }
        delete[] name;
    }
    return true;
}

bool MaterialManager::PreloadTextures()
{
    // characters
    if (!LoadTextureDir("/planeshift/models/"))
        return false;

    // Load the textures for the weapons.
    if (!LoadTextureDir("/planeshift/weapons/"))
        return false;

    if (!LoadTextureDir("/planeshift/shields/"))
        return false;
        
    // Load the textures for the items.
    if (!LoadTextureDir("/planeshift/items/"))
        return false;

    // Load the textures for the spell effects
    if (!LoadTextureDir("/planeshift/art/effects/"))
        return false;

    // Load the textures for the resources items
    if (!LoadTextureDir("/planeshift/naturalres/"))
        return false;

    // Load the textures for the tools items
    if (!LoadTextureDir("/planeshift/tools/"))
        return false;

    // Load the textures for the food items
    if (!LoadTextureDir("/planeshift/food/"))
        return false;

    return true;
}

void MaterialManager::UnloadUnusedMaterials()
{
    iMaterialList *matList = engine->GetMaterialList();
    for(int i=0; i<matList->GetCount(); i++)
    {
        if(matList->Get(i)->GetRefCount() == 1)
        {
            matList->Remove(i);
            i--;
        }
    }
}

void MaterialManager::UnloadUnusedTextures()
{
    iTextureList *texList = engine->GetTextureList();
    for(int i=0; i<texList->GetCount(); i++)
    {
        if(texList->Get(i)->GetRefCount() == 1)
        {
            texList->Remove(i);
            i--;
        }
    }
}

void MaterialManager::UnloadUnusedFactories()
{
    iMeshFactoryList *factList = engine->GetMeshFactories();
    for(int i=0; i<factList->GetCount(); i++)
    {
        if(factList->Get(i)->GetRefCount() == 1)
        {
            factList->Remove(i);
            i--;
        }
    }
}
