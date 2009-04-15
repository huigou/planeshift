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
#include <csutil/threading/rwmutex.h>
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

    csPtr<iMaterialWrapper> LoadMaterial(const char* name, bool* failed = NULL);
    csPtr<iMeshFactoryWrapper> LoadFactory(const char* name);

    THREADED_CALLABLE_DECL2(Loader, PrecacheData, csThreadReturn, const char*, path, bool, recursive, THREADEDL, false, false);
    void UpdatePosition(const csVector3& pos, const char* sectorName, bool force);

    void ContinueLoading(bool waiting);

    inline iThreadedLoader* GetLoader() { return tloader; }

    inline size_t GetLoadingCount() { return loadingMeshes.GetSize() + finalisableMeshes.GetSize(); }

    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

    inline void SetLoadRange(float r) { loadRange = r; }

    inline bool HasValidPosition() const { return validPosition; }

private:
  class MeshGen;
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
            : name(name), type(type), vec2(0.0f)
        {
        }
    };

    class Texture : public CS::Utility::FastRefCount<Texture>
    {
    public:
        Texture(const char* name, const char* path, iDocumentNode* data)
            : name(name), path(path), useCount(0), data(data)
        {
        }

        csString name;
        csString path;
        uint useCount;
        csRef<iThreadReturn> status;
        csRef<iDocumentNode> data;
    };

    class Material : public CS::Utility::FastRefCount<Material>
    {
    public:
        Material(const char* name = "")
            : name(name), useCount(0)
        {
        }

        csString name;
        uint useCount;
        csRef<iMaterialWrapper> mat;
        csArray<Shader> shaders;
        csArray<ShaderVar> shadervars;
        csRefArray<Texture> textures;
        csArray<bool> checked;
    };

    class MeshFact : public CS::Utility::FastRefCount<MeshFact>
    {
    public:
        MeshFact(const char* name, const char* path, iDocumentNode* data) : name(name),
          path(path), useCount(0), data(data)
        {
        }

        csString name;
        csString path;
        uint useCount;
        csRef<iThreadReturn> status;
        csRef<iDocumentNode> data;
        csRefArray<Material> materials;
        csArray<bool> checked;
    };

    class Sector : public CS::Utility::FastRefCount<Sector>
    {
    public:
        Sector(const char* name) : name(name), init(false), isLoading(false), checked(false),
          objectCount(0), alwaysLoadedCount(0)
        {
            ambient = csColor(0.0f);
        }

        csString name;
        bool init;
        bool isLoading;
        bool checked;
        csString culler;
        csColor ambient;
        size_t objectCount;
        size_t alwaysLoadedCount;
        csRef<iSector> object;
        csRefArray<MeshGen> meshgen;
        csRefArray<MeshObj> meshes;
        csRefArray<Portal> portals;
        csRefArray<Portal> activePortals;
        csRefArray<Light> lights;
    };

    class MeshGen : public CS::Utility::FastRefCount<MeshObj>
    {
    public:
        MeshGen(const char* name, iDocumentNode* data) : name(name), data(data),
            loading(false)
        {
        }

        inline bool InRange(const csBox3& curBBox)
        {
            return !status.IsValid() && curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csBox3& curBBox)
        {
            return status.IsValid() && !curBBox.Overlap(bbox);
        }

        csString name;
        csRef<iDocumentNode> data;
        bool loading;
        csBox3 bbox;
        csRef<iThreadReturn> status;
        csRef<MeshObj> object;
        csRefArray<Material> materials;
        csArray<bool> matchecked;
        csRefArray<MeshFact> meshfacts;
        csArray<bool> mftchecked;
        Sector* sector;
    };

    class MeshObj : public CS::Utility::FastRefCount<MeshObj>
    {
    public:
        MeshObj(const char* name, const char* path, iDocumentNode* data) : name(name), path(path), data(data),
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
        csString path;
        csRef<iDocumentNode> data;
        csVector3 pos;

        bool loading;
        bool alwaysLoaded;
        bool hasBBox;
        csBox3 bbox;
        csRef<iThreadReturn> status;
        csRef<iMeshWrapper> object;
        csRefArray<Texture> textures;
        csArray<bool> texchecked;
        csRefArray<Material> materials;
        csArray<bool> matchecked;
        csRefArray<MeshFact> meshfacts;
        csArray<bool> mftchecked;
        Sector* sector;
    };

    class Portal : public CS::Utility::FastRefCount<Portal>
    {
    public:
        Portal(const char* name) : name(name), wv(0), ww_given(false), ww(0), transform(0), clip(false), zfill(false), warp(false)
        {
        }

        inline bool InRange(const csBox3& curBBox)
        {
            return !mObject.IsValid() && curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csBox3& curBBox)
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

        inline bool InRange(const csBox3& curBBox)
        {
            return !object.IsValid() && curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csBox3& curBBox)
        {
            return object.IsValid() && !curBBox.Overlap(bbox);
        }

        csRef<iLight> object;
        csString name;
        csVector3 pos;
        float radius;
        csColor colour;
        csLightDynamicType dynamic;
        csLightAttenuationMode attenuation;
        csLightType type;
        csBox3 bbox;
    };

    void CleanDisconnectedSectors(Sector* sector);
    void FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector);
    void CleanSector(Sector* sector);
    void CleanMesh(MeshObj* mesh);
    void CleanMeshGen(MeshGen* meshgen);
    void CleanMeshFact(MeshFact* meshfact);
    void CleanMaterial(Material* material);
    void CleanTexture(Texture* texture);
    void LoadSector(const csVector3& pos, const csBox3& loadBox, const csBox3& unloadBox,
      Sector* sector, uint depth);
    void FinishMeshLoad(MeshObj* mesh);
    bool LoadMeshGen(MeshGen* meshgen);
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
    csRef<iThreadManager> tman;
    csRef<iVFS> vfs;
    csRef<iShaderVarStringSet> svstrings;
    csRef<iStringSet> strings;
    csRef<iCollideSystem> cdsys;
    csRef<iSyntaxService> syntaxService;
    uint gfxFeatures;
    bool validPosition;

    // Limit on how many portals deep we load.
    static const int maxPortalDepth = 3;

    csRef<Sector> lastSector;
    csVector3 lastPos;

    csRefArray<MeshGen> loadingMeshGen;
    csRefArray<MeshObj> loadingMeshes;
    csRefArray<MeshObj> finalisableMeshes;
    csRefArray<MeshObj> deleteQueue;

    csRedBlackTreeMap<csString, csRef<Texture> > textures;
    csRedBlackTreeMap<csString, csRef<Material> > materials;
    csRedBlackTreeMap<csString, csRef<MeshFact> > meshfacts;
    csRedBlackTreeMap<csString, csRef<MeshObj> > meshes;
    csRefArray<Sector> sectors;

    CS::Threading::ReadWriteMutex tLock;
    CS::Threading::ReadWriteMutex mLock;
    CS::Threading::ReadWriteMutex mfLock;
    CS::Threading::ReadWriteMutex meshLock;
    CS::Threading::ReadWriteMutex sLock;
};

#endif // __LOADER_H__
