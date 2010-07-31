/*
 * loader.h - Author: Mike Gist
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csutil/scf_implementation.h>
#include <csutil/hash.h>
#include <csutil/threading/rwmutex.h>
#include <csutil/threadmanager.h>
#include <csutil/refcount.h>

#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <iengine/texture.h>
#include <iengine/movable.h>
#include <imesh/objmodel.h>
#include <imap/loader.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include <iclient/ibgloader.h>
#include <iclient/iscenemanipulate.h>

struct iCollideSystem;
struct iEngineSequenceManager;
struct iSyntaxService;

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
class BgLoader : public ThreadedCallable<BgLoader>,
                 public scfImplementation3<BgLoader,
                                           iBgLoader,
                                           iSceneManipulate,
                                           iComponent>
{
public:
    BgLoader(iBase *p);
    virtual ~BgLoader();

   /**
    * Plugin initialisation.
    */
    bool Initialize(iObjectRegistry* _object_reg);

   /**
    * Start loading a material into the engine. Returns 0 if the material is not yet loaded.
    * @param failed Pass a boolean to be able to manually handle a failed load.
    */
    iMaterialWrapper* LoadMaterial(const char* name, bool* failed = NULL, bool wait = false);

   /**
    * Start loading a mesh factory into the engine. Returns 0 if the factory is not yet loaded.
    * @param failed Pass a boolean to be able to manually handle a failed load.
    */
    csPtr<iMeshFactoryWrapper> LoadFactory(const char* name, bool* failed = NULL, bool wait = false);

   /**
    * Free your instance of a material.
    * @return true upon success, false otherwise.
    */
    bool FreeMaterial(const char* name);

   /**
    * Free your instance of a factory.
    * @return true upon success, false otherwise.
    */
    bool FreeFactory(const char* name);

    /**
    * Clone a mesh factory.
    * @param name The name of the mesh factory to clone.
    * @param newName The name of the new cloned mesh factory.
    * @param load Begin loading the cloned mesh factory.
    * @param failed Pass a boolean to be able to manually handle a failed clone.
    */
    void CloneFactory(const char* name, const char* newName, bool* failed = NULL);

   /**
    * Pass a data file to be cached. This method will parse your data and add it to it's
    * internal world representation. You may then request that these objects are loaded.
    * @param recursive Mark true if this is a recursive call (no vfs chdir needed).
    * If you don't know, set this to false.
    * This call will be dispatched to a thread, so it will return immediately.
    * You should wait for parsing to finish before calling UpdatePosition().
    */
    THREADED_CALLABLE_DECL2(BgLoader, PrecacheData, csThreadReturn, const char*, path, bool, recursive, THREADEDL, false, false);

   /**
    * Update your position in the world.
    * Calling this will trigger per-object checks and initiate (un)loading if the object
    * is within a given threshold (loadRange).
    * @param pos Your world space position.
    * @param sectorName The name of the sector that you are currently in.
    * @param force Forces the checks to be done (normally they won't if you e.g. haven't moved).
    */
    void UpdatePosition(const csVector3& pos, const char* sectorName, bool force);

   /**
    * Call this function to finalise a number of loading objects.
    * Useful when you are waiting for a load to finish (load into the world, teleport),
    * but want to continue rendering while you wait.
    * Will return after processing a number of objects.
    * @param waiting Set as 'true' if you're waiting for the load to finish.
    * This will make it process more before returning (lower overhead).
    * Note that you should process a frame after each call, as spawned threads
    * may depend on the main thread to handle requests.
    */
    void ContinueLoading(bool waiting);

   /**
    * Returns a pointer to the Crystal Space threaded loader.
    */
    iThreadedLoader* GetLoader() { return tloader; }

   /**
    * Returns the number of objects currently loading.
    */
    size_t GetLoadingCount() { return loadingMeshes.GetSize()*2 + finalisableMeshes.GetSize(); }

   /**
    * Returns a pointer to the object registry.
    */
    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

   /**
    * Update the load range initially passed to the loader in Setup().
    */
    void SetLoadRange(float r) { loadRange = r; if (lastSector.IsValid()) UpdatePosition(lastPos, lastSector->name, true); }

   /**
    * Request to know whether the current world position stored by the loader is valid.
    * Returns false until the first call of UpdatePosition().
    */
    bool HasValidPosition() const { return validPosition; }

   /**
    * Request to know whether you are currently positioned in a water body.
    * @param sector The sector that you are checking.
    * @param pos The world space position that you are checking.
    * @param colour Will contain the colour of the water that you are positioned in.
    */
    bool InWaterArea(const char* sector, csVector3* pos, csColor4** colour);

   /**
    * Load zones given by name.
    */
    bool LoadZones(iStringArray* regions, bool loadMeshes = true, bool priority = false);

   /**
    * Load high priority zones given by name.
    * High priority zones are not unloaded by UpdatePosition or LoadZones.
    */
    bool LoadPriorityZones(iStringArray* regions);

   /**
    * Returns an array of the available shaders for a given type.
    * @param usageType The type of shader you wish to have.
    * E.g. 'default_alpha' to get an array of all default world alpha shaders.
    */
    csPtr<iStringArray> GetShaderName(const char* usageType) const;

   /**
    * Creates a new instance of the given factory at the given screen space coordinates.
    * @param factName The name of the factory to be used to create the mesh.
    * @param matName The optional name of the material to set on the mesh. Pass NULL to set none.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    iMeshWrapper* CreateAndSelectMesh(const char* factName, const char* matName,
        iCamera* camera, const csVector2& pos);

   /**
    * Selects the closest mesh at the given screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    iMeshWrapper* SelectMesh(iCamera* camera, const csVector2& pos);

   /**
    * Translates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param vertical True if you want to translate vertically (along y-axis).
    * False to translate snapped to the mesh at the screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    bool TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos);

   /**
    * Rotates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param pos The screen space coordinates to use to base the rotation, relative to the last saved coordinates.
    */
    void RotateSelected(const csVector2& pos);

   /**
    * Set the axes to rotate around with RotateSelected.
    * @param flags_h bitflag with axes for horizontal mouse movement
    * @param flags_v bitflag with axes for vertical mouse movement
    * @see PS_MANIPULATE
    */
    void SetRotation(int flags_h, int flags_v);

   /**
    * Sets the previous position (e.g. in case you warped the mouse)
    */
    void SetPosition(const csVector2& pos) { previousPosition = pos; };

   /**
    * Removes the currently selected mesh from the scene.
    */
    void RemoveSelected();

    void GetPosition(csVector3 & pos, csVector3 & rot, const csVector2& screenPos);

   /**
    * Returns an array of start positions in the world.
    */
    csRefArray<StartPosition>* GetStartPositions()
    {
        return &startPositions;
    }

private:
    class MeshGen;
    class MeshObj;
    class Portal;
    class Light;
    class Sequence;
    class Zone;

    // The various gfx feature options we have.
    enum gfxFeatures
    {
      useLowestShaders = 0x1,
      useLowShaders = 0x2,
      useMediumShaders = 0x4,
      useHighShaders = 0x8,
      useHighestShaders = 0x10,
      useShadows = 0x20,
      useMeshGen = 0x40,
      useAll = (useHighestShaders | useShadows | useMeshGen)
    };

    /********************************************************
     * Data structures representing components of the world.
     *******************************************************/

    struct WaterArea
    {
        csBox3 bbox;
        csColor4 colour;
    };

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
        float vec1;
        csVector2 vec2;
        csVector3 vec3;
        csVector4 vec4;

        ShaderVar(const char* name, csShaderVariable::VariableType type)
            : name(name), type(type), vec2(0.0f)
        {
        }
    };

    class Texture : public CS::Utility::AtomicRefCount
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

    class Material : public CS::Utility::AtomicRefCount
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

    class MeshFact : public CS::Utility::AtomicRefCount
    {
    public:
        MeshFact(const char* name, const char* path, iDocumentNode* data) : name(name),
          path(path), useCount(0), data(data)
        {
        }

        csPtr<MeshFact> Clone(const char* clonedName)
        {
            csRef<MeshFact> meshfact;
            meshfact.AttachNew(new MeshFact(clonedName, path, data));

            meshfact->filename = filename;
            meshfact->materials = materials;
            meshfact->checked.SetSize(checked.GetSize(), false);
            meshfact->bboxvs = bboxvs;
            meshfact->submeshes = submeshes;

            return csPtr<MeshFact>(meshfact);
        }

        bool operator==(const MeshFact& other)
        {
            if (filename != other.filename)
                return false;
            if ((iDocumentNode*)data != (iDocumentNode*)other.data)
                return false;

            return true;
        }

        csString name;
        csString filename;
        csString path;
        uint useCount;
        csRef<iMeshFactoryWrapper> object;
        csRef<iThreadReturn> status;
        csRef<iDocumentNode> data;
        csRefArray<Material> materials;
        csArray<bool> checked;
        csArray<csVector3> bboxvs;
        csStringArray submeshes;
    };

    class Sector : public CS::Utility::AtomicRefCount
    {
    public:
        Sector(const char* name) : name(name), init(false), isLoading(false), checked(false),
          objectCount(0), priority(false)
        {
            ambient = csColor(0.0f);
        }

        ~Sector()
        {
            while(!waterareas.IsEmpty())
            {
                delete waterareas.Pop();
            }
        }

        csString name;
        bool init;
        bool isLoading;
        bool checked;
        csString culler;
        csColor ambient;
        int objectCount;
        csRef<iSector> object;
        csWeakRef<Zone> parent;
        csRefArray<MeshGen> meshgen;
        csRefArray<MeshObj> alwaysLoaded;
        csRefArray<MeshObj> meshes;
        csRefArray<Portal> portals;
        csRefArray<Portal> activePortals;
        csRefArray<Light> lights;
        csRefArray<Sequence> sequences;
        csArray<WaterArea*> waterareas;
        CS::Threading::ReadWriteMutex lock; // used during precaching
        bool priority;
    };

    class MeshGen : public CS::Utility::AtomicRefCount
    {
    public:
        MeshGen(const char* name, iDocumentNode* data) : name(name), data(data),
            loading(false)
        {
        }

        inline bool InRange(const csBox3& curBBox, bool force)
        {
            return !status.IsValid() && (force || curBBox.Overlap(bbox)) && !loading;
        }

        inline bool OutOfRange(const csBox3& curBBox)
        {
            return status.IsValid() && !curBBox.Overlap(bbox) && !loading;
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

    class MeshObj : public CS::Utility::AtomicRefCount
    {
    public:
        MeshObj(const char* name, const char* path, iDocumentNode* data) : name(name), path(path), data(data),
            loading(false)
        {
        }

        inline bool InRange(const csBox3& curBBox, bool force)
        {
            return !object.IsValid() && (force || curBBox.Overlap(bbox)) && !loading;
        }

        inline bool OutOfRange(const csBox3& curBBox)
        {
            return object.IsValid() && !curBBox.Overlap(bbox) && !loading;
        }

        csString name;
        csString path;
        csRef<iDocumentNode> data;

        bool loading;
        csBox3 bbox;
        csRef<iThreadReturn> status;
        csRef<iMeshWrapper> object;
        csArray<ShaderVar> shadervars;
        csRefArray<Texture> textures;
        csArray<bool> texchecked;
        csRefArray<Material> materials;
        csArray<bool> matchecked;
        csRefArray<MeshFact> meshfacts;
        csArray<bool> mftchecked;
        Sector* sector;
        csRefArray<Sequence> sequences;
    };

    class Portal : public CS::Utility::AtomicRefCount
    {
    public:
        Portal(const char* name) : name(name), wv(0), ww_given(false),
            ww(0), pfloat(false), clip(false), zfill(false), warp(false)
        {
        }

        inline bool InRange(const csBox3& curBBox, bool force)
        {
            return force || (!mObject.IsValid() && curBBox.Overlap(bbox));
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
        csReversibleTransform transform;
        bool pfloat;
        bool clip;
        bool zfill;
        bool warp;
	bool autoresolve;
        csPoly3D poly;
        csBox3 bbox;

        csRef<Sector> targetSector;
        iPortal* pObject;
        csRef<iMeshWrapper> mObject;

        class MissingSectorCallback : public scfImplementation1<MissingSectorCallback, iPortalCallback>
        {
        private:
            csRef<Sector> targetSector;
            bool autoresolve;

        public:
            MissingSectorCallback(Sector* target, bool resolve) : scfImplementationType(this),targetSector(target),autoresolve(resolve)
            {
            }

            virtual ~MissingSectorCallback()
            {
            }

            virtual bool Traverse(iPortal* p, iBase* /*context*/)
            {
                if(targetSector->object.IsValid())
                {
                    p->SetSector(targetSector->object);
                }
                else
                {
                    return false;
                }

                if(!autoresolve)
                {
                    p->RemoveMissingSectorCallback(this);
                }

                return true;
            }
        };
    };

    class Light : public CS::Utility::AtomicRefCount
    {
    public:
        Light(const char* name) : name(name), loaded(false)
        {
        }

        inline bool InRange(const csBox3& curBBox, bool force)
        {
            return !loaded && (force || curBBox.Overlap(bbox));
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
        csRefArray<Sequence> sequences;
        bool loaded;
    };

    class Trigger : public CS::Utility::AtomicRefCount
    {
    public:
        Trigger(const char* name, iDocumentNode* data) : name(name), loaded(false),
            data(data)
        {
        }

        csString name;
        bool loaded;
        csRef<iDocumentNode> data;
        csRef<iThreadReturn> status;
    };

    class Sequence : public CS::Utility::AtomicRefCount
    {
    public:
        Sequence(const char* name, iDocumentNode* data) : name(name), loaded(false),
            data(data)
        {
        }

        csString name;
        bool loaded;
        csRef<iDocumentNode> data;
        csRefArray<Trigger> triggers;
        csRef<iThreadReturn> status;
    };

    /***********************************************************************/

    /* Shader parsing */
    void ParseShaders();
    ShaderVar* ParseShaderVar(const csString& name, const csString& type, const csString& value, csRef<Texture>& tex, bool doChecks = true);

    /* Internal unloading methods. */
    void CleanDisconnectedSectors(Sector* sector);
    void FindConnectedSectors(csRefArray<Sector>& connectedSectors, Sector* sector);
    void CleanSector(Sector* sector);
    void CleanPortal(Portal* sequence);
    void CleanMesh(MeshObj* mesh);
    void CleanMeshGen(MeshGen* meshgen);
    void CleanMeshFact(MeshFact* meshfact);
    void CleanMaterial(Material* material);
    void CleanTexture(Texture* texture);
    void CleanLight(Light* light);
    void CleanSequence(Sequence* sequence);

    /* Internal loading methods. */
    void LoadSector(const csBox3& loadBox, const csBox3& unloadBox,
      Sector* sector, uint depth, bool force, bool loadMeshes, bool portalsOnly = false);
    void FinishMeshLoad(MeshObj* mesh);
    bool LoadMeshGen(MeshGen* meshgen);
    bool LoadMesh(MeshObj* mesh);
    bool LoadPortal(Portal* portal, Sector* sector);
    bool LoadMeshFact(MeshFact* meshfact, bool wait = false);
    bool LoadMaterial(Material* material, bool wait = false);
    bool LoadTexture(Texture* texture, bool wait = false);
    bool LoadLight(Light* light, Sector* sector, bool wait = false);
    bool LoadSequence(Sequence* sequence, bool wait = false);

    // Pointers to other needed plugins.
    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iEngineSequenceManager> engseq;
    csRef<iGraphics2D> g2d;
    csRef<iTextureManager> txtmgr;
    csRef<iThreadedLoader> tloader;
    csRef<iThreadManager> tman;
    csRef<iVFS> vfs;
    csRef<iShaderVarStringSet> svstrings;
    csRef<iStringSet> strings;
    csRef<iCollideSystem> cdsys;
    csRef<iSyntaxService> syntaxService;

    // Whether we've parsed shaders.
    bool parsedShaders;

    // Our load range ^_^
    float loadRange;

    // Currently enabled graphics features.
    uint enabledGfxFeatures;

    // Whether or not we're caching.
    bool cache;

    // Whether the current position is valid.
    bool validPosition;

    // Limit on how many portals deep we load.
    static const uint maxPortalDepth = 3;

    // The last valid sector.
    csRef<Sector> lastSector;

    // The last valid position.
    csVector3 lastPos;

    // The last offset used for mesh loading
    size_t loadingOffset;

    // Shader store.
    csHash<csString, csStringID> shadersByUsageType;

    // Stores world representation.
    class Zone : public csObject
    {
    public:
        bool loading;
        csRefArray<Sector> sectors;
        bool priority;

        Zone(const char* name) : loading(false), priority(false)
        {
            SetName(name);
        }
    };

    csRefArray<Zone> loadedZones;
    csHash<csRef<Zone>, csStringID> zones;

    csHash<csRef<Texture>, csStringID> textures;
    csHash<csRef<Material>, csStringID> materials;
    csHash<csRef<MeshFact>, csStringID> meshfacts;
    csHash<csRef<MeshObj>, csStringID> meshes;
    csHash<csRef<Sector>, csStringID> sectorHash;
    csRefArray<Sector> sectors;
    csRefArray<StartPosition> startPositions;

    csStringArray shaders;
    csRefArray<MeshGen> loadingMeshGen;
    csRefArray<MeshObj> loadingMeshes;
    csRefArray<MeshObj> finalisableMeshes;

    // Locks on the resource hashes.
    CS::Threading::ReadWriteMutex tLock;
    CS::Threading::ReadWriteMutex mLock;
    CS::Threading::ReadWriteMutex mfLock;
    CS::Threading::ReadWriteMutex meshLock;
    CS::Threading::ReadWriteMutex sLock;
    CS::Threading::ReadWriteMutex zLock;

    // Resource string sets.
    csStringSet tStringSet;
    csStringSet mStringSet;
    csStringSet mfStringSet;
    csStringSet meshStringSet;
    csStringSet sStringSet;
    csStringSet zStringSet;

    // For world manipulation.
    csRef<iMeshWrapper> selectedMesh;
    csString selectedFactory;
    csString selectedMaterial;
    csVector2 previousPosition;
    csVector3 origTrans;
    csVector3 rotBase;
    csVector3 origRot;
    csVector3 currRot_h;
    csVector3 currRot_v;
    bool resetHitbeam;
};
}
CS_PLUGIN_NAMESPACE_END(bgLoader)

#endif // __LOADER_H__
