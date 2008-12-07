/*
 *  materialmanager.h - Author: Mike Gist
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

#ifndef __MATERIAL_MANAGER_H__
#define __MATERIAL_MANAGER_H__

#include <csutil/scf_implementation.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/texture.h>
#include <imap/loader.h>
#include <iutil/vfs.h>
#include "util/singleton.h"

struct iObjectRegistry;

class MaterialManager : public scfImplementation1<MaterialManager, iMissingLoaderData>, public Singleton<MaterialManager>
{
public:
    MaterialManager(iObjectRegistry* _object_reg, bool _keepModels);

    virtual iMaterialWrapper* MissingMaterial (const char* name, const char* filename);

    virtual iTextureWrapper* MissingTexture (const char* name, const char* filename);

    virtual iShader* MissingShader (const char* name) { return 0; }

    virtual iMeshFactoryWrapper* MissingFactory (const char* name) { return 0; }

    virtual iMeshWrapper* MissingMesh (const char* name){ return 0; }

    virtual iSector* MissingSector (const char* name) { return 0; }

    virtual iLight* MissingLight (const char* name) { return 0; }

    bool PreloadTextures();
    bool KeepModels() { return keepModels; }

    void UnloadUnusedMaterials();
    void UnloadUnusedTextures();
    void UnloadUnusedFactories();

private:
    bool LoadTextureDir(const char *dir);
    bool keepModels;
    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iTextureManager> txtmgr;
    csRef<iThreadedLoader> loader;
    csRef<iVFS> vfs;
};

#endif // __MATERIAL_MANAGER_H__
