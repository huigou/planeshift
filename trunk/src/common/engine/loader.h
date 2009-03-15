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

#include <csgeom/poly3d.h>
#include <csgfx/shadervar.h>
#include <csutil/redblacktree.h>
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
struct iSyntaxService;

class Loader : public Singleton<Loader>, public ThreadedCallable<Loader>
{
public:
    void Init(iObjectRegistry* _object_reg, uint gfxFeatures, float loadRange);

    iTextureWrapper* LoadTexture(const char* name, const char* filename, const char* className = 0);
    iMaterialWrapper* LoadMaterial(const char* name, const char* filename);
    csPtr<iMeshFactoryWrapper> LoadFactory(const char* name);

    THREADED_CALLABLE_DECL2(Loader, PrecacheData, csThreadReturn, const char*, path, bool, recursive, THREADEDL, false, false);
    void UpdatePosition(const csVector3& pos, const char* sectorName, bool force);

    inline iThreadedLoader* GetLoader() { return tloader; }

    inline size_t GetLoadingCount() { return loadingMeshes.GetSize() + finalisableMeshes.GetSize(); }

    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

    inline void SetLoadRange(float r) { loadRange = r; }

    inline bool HasValidPosition() const { return validPosition; }

private:
  class MeshObj;
  class Portal;
  class Light;

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
        Sector(const char* name) : name(name), isLoading(false), checked(false), objectCount(0), alwaysLoadedCount(0)
        {
            ambient = csColor(0.0f);
        }

        csString name;
        bool isLoading;
        bool checked;
        csString culler;
        csColor ambient;
        size_t objectCount;
        size_t alwaysLoadedCount;
        csRef<iSector> object;
        csRefArray<MeshObj> meshes;
        csRefArray<Portal> portals;
        csRefArray<Portal> activePortals;
        csRefArray<Light> lights;
    };

    class MeshObj : public CS::Utility::FastRefCount<MeshObj>
    {
    public:
        MeshObj(const char* name, iDocumentNode* data) : name(name), data(data),
            loading(false), alwaysLoaded(false), hasBBox(false)
        {
        }

        inline bool InRange(const csVector3& curpos, const csBox3& curBBox)
        {
            return !object.IsValid() && (alwaysLoaded ||
                (hasBBox ? curBBox.Overlap(bbox) : csVector3(pos - curpos).Norm() <= Loader::GetSingleton().loadRange));
        }

        inline bool OutOfRange(const csVector3& curpos, const csBox3& curBBox)
        {
            return !alwaysLoaded && object.IsValid() &&
                (hasBBox ? !curBBox.Overlap(bbox) : csVector3(pos - curpos).Norm() > Loader::GetSingleton().loadRange*1.5);
        }

        csString name;
        csRef<iDocumentNode> data;
        csVector3 pos;

        bool loading;
        bool alwaysLoaded;
        bool hasBBox;
        csBox3 bbox;
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
        Portal(const char* name) : name(name), wv(0), ww_given(false), ww(0), transform(0), clip(false), zfill(false), warp(false)
        {
        }

        inline bool InRange(const csVector3& pos, const csBox3& curBBox)
        {
            return !mObject.IsValid() && curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csVector3& pos, const csBox3& curBBox)
        {
            return mObject.IsValid() && !curBBox.Overlap(bbox);
        }

        csString name;
        csMatrix3 matrix;
        csVector3 wv;
        bool ww_given;
        csVector3 ww;
        csVector3 transform;
        bool clip;
        bool zfill;
        bool warp;
        csPoly3D poly;
        csBox3 bbox;

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

    void CleanDisconnectedSectors(Sector* sector);
    void FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector);
    void CleanSector(Sector* sector);
    void LoadSector(const csVector3& pos, const csBox3& bbox, Sector* sector);
    void FinishMeshLoad(MeshObj* mesh);
    bool LoadMesh(MeshObj* mesh);
    bool LoadMeshFact(MeshFact* meshfact);
    bool LoadMaterial(Material* material);
    bool LoadTexture(Texture* texture);

    float loadRange;

    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iTextureManager> txtmgr;
    csRef<iLoader> loader;
    csRef<iThreadedLoader> tloader;
    csRef<iVFS> vfs;
    csRef<iShaderVarStringSet> svstrings;
    csRef<iStringSet> strings;
    csRef<iCollideSystem> cdsys;
    csRef<iSyntaxService> syntaxService;
    uint gfxFeatures;
    bool validPosition;

    csRef<Sector> lastSector;
    csVector3 lastPos;

    csRefArray<MeshObj> loadingMeshes;
    csRefArray<MeshObj> finalisableMeshes;

    csRedBlackTreeMap<csString, csRef<Texture> > textures;
    csRedBlackTreeMap<csString, csRef<Material> > materials;
    csRedBlackTreeMap<csString, csRef<MeshFact> > meshfacts;
    csRefArray<Sector> sectors;

    CS::Threading::Mutex tLock;
    CS::Threading::Mutex mLock;
    CS::Threading::Mutex mfLock;
    CS::Threading::Mutex sLock;
};

#endif // __LOADER_H__
