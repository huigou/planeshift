/*
 *  loader.h - Author: Mike Gist
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

#ifndef __LOADER_H__
#define __LOADER_H__

#include <csgfx/shadervar.h>
#include <csutil/scf_implementation.h>
#include <csutil/threadmanager.h>

#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <iengine/texture.h>
#include <imap/loader.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include "util/singleton.h"

struct iCollideSystem;
struct iObjectRegistry;

class Loader : public Singleton<Loader>, public ThreadedCallable<Loader>
{
public:
    void Init(iObjectRegistry* _object_reg, bool _keepModels, uint gfxFeatures, float loadRange);

    iMaterialWrapper* LoadMaterial (const char* name, const char* filename);

    iTextureWrapper* LoadTexture (const char* name, const char* filename, const char* className = 0);

    THREADED_CALLABLE_DECL2(Loader, PrecacheData, csThreadReturn, const char*, path, bool, recursive, THREADEDL, false, false);
    void UpdatePosition(const csVector3& pos, const char* sectorName, bool force);

    bool PreloadTextures();
    bool KeepModels() { return keepModels; }

    iThreadedLoader* GetLoader() { return tloader; }

    size_t GetLoadingCount() { return loadingMeshes.GetSize(); }

    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

private:
    class Texture;
    class Material;
    class MeshFact;
    class Sector;
    class MeshObj;
    class Portal;
    class Light;

    void CleanDisconnectedSectors(Sector* sector);
    void FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector);
    void CleanSector(Sector* sector);
    void LoadSector(const csVector3& pos, Sector* sector);
    void LoadMesh(MeshObj* mesh);
    bool LoadMeshFact(MeshFact* meshfact);
    bool LoadMaterial(Material* material);
    bool LoadTexture(Texture* texture);

    bool LoadTextureDir(const char *dir);
    bool keepModels;
    float loadRange;

    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iTextureManager> txtmgr;
    csRef<iThreadedLoader> tloader;
    csRef<iVFS> vfs;
    csRef<iShaderVarStringSet> svstrings;
    csRef<iStringSet> strings;
    csRef<iCollideSystem> cdsys;
    uint gfxFeatures;

    csRef<Sector> lastSector;
    csVector3 lastPos;

    csRefArray<MeshObj> loadingMeshes;

    csRefArray<Texture> textures;
    csRefArray<Material> materials;
    csRefArray<MeshFact> meshfacts;
    csRefArray<Sector> sectors;

    CS::Threading::Mutex tLock;
    CS::Threading::Mutex mLock;
    CS::Threading::Mutex mfLock;
    CS::Threading::Mutex sLock;

    struct Shader
    {
        csString type;
        csString name;

        Shader(const char* type, const char* name)
            : type(type), name(name)
        {
        }
    };

    struct ShaderVar
    {
        csString name;
        csShaderVariable::VariableType type;
        csString value;
        csVector2 vec2;
        
        ShaderVar(const char* name, csShaderVariable::VariableType type)
            : name(name), type(type)
        {
        }
    };

    class Texture : public CS::Utility::FastRefCount<Texture>
    {
    public:
        Texture(const char* name = "")
            : name(name), loaded(false)
        {
        }

        csString name;
        bool loaded;
        csRef<iThreadReturn> status;
        csRef<iDocumentNode> data;
    };

    class Material : public CS::Utility::FastRefCount<Material>
    {
    public:
        Material(const char* name = "")
            : name(name), loaded(false)
        {
        }

        csString name;
        bool loaded;
        csArray<Shader> shaders;
        csArray<ShaderVar> shadervars;
        csRefArray<Texture> textures;
    };

    class MeshFact : public CS::Utility::FastRefCount<MeshFact>
    {
    public:
        MeshFact(const char* name, iDocumentNode* data) : name(name), loaded(false), data(data)
        {
        }

        csString name;
        bool loaded;
        csRef<iThreadReturn> status;
        csRef<iDocumentNode> data;
        csRefArray<Material> materials;
    };

    class Sector : public CS::Utility::FastRefCount<Sector>
    {
    public:
        Sector(const char* name) : name(name), isLoading(false), objectCount(0)
        {
            ambient = csColor(0.0f);
        }

        csString name;
        bool isLoading;
        csString culler;
        csColor ambient;
        size_t objectCount;
        csRef<iSector> object;
        csRefArray<MeshObj> meshes;
        csRefArray<Portal> portals;
        csRefArray<Portal> activePortals;
        csRefArray<Light> lights;
    };

    class MeshObj : public CS::Utility::FastRefCount<MeshObj>
    {
    public:
        MeshObj(const char* name, iDocumentNode* data) : name(name), data(data), loading(false)
        {
        }

        csString name;
        csRef<iDocumentNode> data;
        csVector3 pos;

        bool loading;
        csRef<iThreadReturn> status;
        csRef<iMeshWrapper> object;
        csRefArray<Texture> textures;
        csRefArray<Material> materials;
        csRefArray<MeshFact> meshfacts;
        Sector* sector;
    };

    class Portal : public CS::Utility::FastRefCount<Portal>
    {
    public:
        Portal(const char* name) : name(name)
        {
        }

        bool InRange(const csVector3& pos)
        {
            for(int i=0; i<num_vertices; i++)
            {
                if(csVector3(vertices[i] - pos).Norm() <= Loader::GetSingleton().loadRange)
                {
                    return true;
                }
            }

            return false;
        }

        bool OutOfRange(const csVector3& pos)
        {
            for(int i=0; i<num_vertices; i++)
            {
                if(csVector3(vertices[i] - pos).Norm() <= Loader::GetSingleton().loadRange*1.5)
                {
                    return false;
                }
            }

            return true;
        }

        csString name;
        csVector3* vertices;
        csVector3 ww;
        int num_vertices;

        csRef<Sector> targetSector;
        iPortal* pObject;
        csRef<iMeshWrapper> mObject;
    };

    class Light : public CS::Utility::FastRefCount<Light>
    {
    public:
        Light(const char* name) : name(name)
        {
        }

        csRef<iLight> object;
        csString name;
        csVector3 pos;
        float radius;
        csColor colour;
        csLightDynamicType dynamic;
        csLightAttenuationMode attenuation;
        csLightType type;
    };
};

#endif // __LOADER_H__
