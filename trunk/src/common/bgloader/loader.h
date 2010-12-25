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
#include <csutil/typetraits.h>

#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/meshgen.h>
#include <iengine/sector.h>
#include <iengine/texture.h>
#include <iengine/movable.h>
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <imap/loader.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>

#include <ibgloader.h>
#include <iscenemanipulate.h>

#ifdef CS_DEBUG
#define LOADER_DEBUG_MESSAGE(...) csPrintf(__VA_ARGS__)
#else
#define LOADER_DEBUG_MESSAGE(...)
#endif

//#ifdef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!(x)) printf("ART ERROR: %s\n", msg)
//#endif

struct iCollideSystem;
struct iEngineSequenceManager;
struct iSyntaxService;

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{

// string literals for usage as template parameter
namespace ObjectNames
{
    extern const char texture[8];;
    extern const char material[9];
    extern const char trigger[8];
    extern const char sequence[9];
    extern const char meshobj[5];
    extern const char meshfact[13];
    extern const char meshgen[8];
    extern const char light[6];
    extern const char portal[7];
    extern const char sector[7];
}

class BgLoader : public ThreadedCallable<BgLoader>,
                 public scfImplementation3<BgLoader,
                                           iBgLoader,
                                           iSceneManipulate,
                                           iComponent>
{
private:
    // forward declaration
    class Loadable;

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
    csPtr<iMaterialWrapper> LoadMaterial(const char* name, bool* failed = NULL, bool wait = false);

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
    * Clears all temporary data that is only required parse time.
    * calls to PrecacheData mustn't occur after this function has been called
    */
    void ClearTemporaryData()
    {
        parserData.xmltokens.Empty();
        parserData.textures.Clear();
        parserData.meshes.Clear();
        parserData.svstrings.Invalidate();
        parserData.syntaxService.Invalidate();
        tman.Invalidate();
    }

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
    size_t GetLoadingCount() { return loadCount; }

   /**
    * Returns a pointer to the object registry.
    */
    iObjectRegistry* GetObjectRegistry() const { return object_reg; }

   /**
    * Update the load range initially passed to the loader in Setup().
    */
    void SetLoadRange(float r) { loadRange = r; if (lastSector.IsValid()) UpdatePosition(lastPos, lastSector->GetName(), true); }

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
    bool LoadZones(iStringArray* regions, bool priority = false);

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
    csPtr<iStringArray> GetShaderName(const char* usageType);

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
    csRefArray<StartPosition> GetStartPositions()
    {
        csRefArray<StartPosition> array;
        CS::Threading::ScopedReadLock lock(parserData.positions.lock);
        LockedType<StartPosition>::HashType::GlobalIterator it(parserData.positions.hash.GetIterator());
        while(it.HasNext())
        {
            array.Push(it.Next());
        }
        return array;
    }

    // internal accessors
    iEngine* GetEngine() const
    {
        return engine;
    }

    iVFS* GetVFS()
    {
        vfsLock.Lock();
        return vfs;
    }

    void ReleaseVFS()
    {
        vfsLock.Unlock();
    }

    iThreadedLoader* GetLoader() const
    {
        return tloader;
    }
    
    iCollideSystem* GetCDSys() const
    {
        return cdsys;
    }

    // increase load count - to be used by loadables only
    void RegisterPendingObject(Loadable* obj)
    {
        ++loadCount;
        loadList.Push(obj);
    }

    // decrease load count - to be used by loadables only
    void UnregisterPendingObject(Loadable* obj)
    {
        --loadCount;
        loadList.Delete(obj);
    }

private:
    // The various gfx feature options we have.
    enum gfxFeatures
    {
      useLowestShaders = 0x1,
      useLowShaders = 0x2 | useLowestShaders,
      useMediumShaders = 0x4 | useLowShaders,
      useHighShaders = 0x8 | useMediumShaders,
      useHighestShaders = 0x10 | useHighShaders,
      useShadows = 0x20,
      useMeshGen = 0x40,
      useAll = (useHighestShaders | useShadows | useMeshGen)
    };

    /********************************************************
     * Data structures representing components of the world.
     *******************************************************/

    // forward declarations
    class Sector;
    class Zone;
    class Texture;
    struct ParserData;
    struct GlobalParserData;

    // helper clases used with object classes that represent the world
    template<typename T> struct CheckedLoad
    {
        csRef<T> obj;
        bool checked;

        CheckedLoad(const csRef<T>& obj) : obj(obj), checked(false)
        {
        }

        CheckedLoad(const CheckedLoad& other) : obj(other.obj), checked(false)
        {
        }
    };

    template<typename T, bool check = true> struct LockedType
    {
    public:
        typedef csHash<csRef<T>, csStringID> HashType;
        HashType hash;
        csStringSet stringSet;
        CS::Threading::ReadWriteMutex lock;

        csPtr<T> Get(const csString& name)
        {
            csRef<T> object;
            CS::Threading::ScopedReadLock scopedLock(lock);
            if(stringSet.Contains(name))
            {
                csStringID objectID = stringSet.Request(name);
                object = hash.Get(objectID, csRef<T>());
            }
            return csPtr<T>(object);
        }

        void Put(const csRef<T>& obj, const char* name = 0)
        {
            CS::Threading::ScopedWriteLock scopedLock(lock);
            csStringID objectID;
            if(name)
            {
                objectID = stringSet.Request(name);
            }
            else
            {
                objectID = stringSet.Request(obj->GetName());
            }

            // check for duplicates
            if(check && hash.Contains(objectID))
            {
                //LOADER_DEBUG_MESSAGE("detected name conflict for object '%s'\n", name ? name : obj->GetName());
            }
            else
            {
                hash.Put(objectID, obj);
            }
        }

        void Delete(const csString& name)
        {
            CS::Threading::ScopedWriteLock scopedLock(lock);
            if(stringSet.Contains(name))
            {
                csStringID id = stringSet.Request(name);
                hash.DeleteAll(id);
                stringSet.Delete(id);
            }
        }

        void Clear()
        {
            stringSet.Empty();
            hash.Empty();
        }
    };

    class RangeBased
    {
    protected:
        csBox3 bbox;

    public:
        inline bool InRange(const csBox3& curBBox) const
        {
            return curBBox.Overlap(bbox);
        }

        inline bool OutOfRange(const csBox3& curBBox) const
        {
            return !curBBox.Overlap(bbox);
        }
    };

    class AlwaysLoaded
    {
    public:
        inline bool InRange(const csBox3& curBBox) const
        {
            return true;
        }

        inline bool OutOfRange(const csBox3& curBBox) const
        {
            return false;
        }
    };

    class Loadable : public csObject
    {
    public:
        Loadable(BgLoader* parent) : parent(parent), loading(false)
        {
            // we want to start with an initial ref count of 0
            useCount.DecRef();
        }

        Loadable(const Loadable& other) : parent(other.parent)
        {
            SetName(other.GetName());

            // we want to start with an initial ref count of 0
            useCount.DecRef();
        }

        virtual ~Loadable()
        {
        }

        bool Load(bool wait = false)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = true;
            if(useCount.GetRefCount() == 0)
            {
                lingerCount = 0;

                if(!loading)
                {
                    loading = true;
                    GetParent()->RegisterPendingObject(this);
                }

                if(LoadObject(wait))
                {
                    loading = false;
                    GetParent()->UnregisterPendingObject(this);
                    useCount.IncRef();

                    FinishObject();
                }

                return !loading;
            }
            else
            {
                useCount.IncRef();
                return true;
            }
        }

        void AbortLoad()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(loading)
            {
                loading = false;
                checked = false;
                GetParent()->UnregisterPendingObject(this);
            }
        }

        void Unload()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(!useCount.GetRefCount())
            {
                LOADER_DEBUG_MESSAGE("tried to free not loaded object '%s'\n", GetName());
                return;
            }

            CS_ASSERT_MSG("unloading currently loading object!", !loading);

            useCount.DecRef();
            if(useCount.GetRefCount() == 0)
            {
                UnloadObject();
            }
        }

        virtual bool LoadObject(bool wait) = 0;
        virtual void UnloadObject() = 0;
        virtual void FinishObject() {}

        bool IsLoaded() const
        {
            return useCount.GetRefCount() > 0;
        }

        inline BgLoader* GetParent() const
        {
            return parent;
        }

        void ResetChecked()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = false;
        }

        bool IsChecked() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return checked;
        }

        size_t GetLingerCount() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return lingerCount;
        }

        void IncLingerCount()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            ++lingerCount;
        }

    protected:
        void MarkChecked()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            checked = true;
        }

        template<typename T, const char* TypeName> void CheckRemove(csRef<T>& ref)
        {
            csWeakRef<T> check(ref);
            parent->GetEngine()->RemoveObject(ref);
            ref.Invalidate();
            if(check.IsValid())
            {
                LOADER_DEBUG_MESSAGE("detected leaking %s: %u %s\n", TypeName, check->GetRefCount(), GetName());
            }
        }

    private:
        mutable CS::Threading::RecursiveMutex busy;
        class UsageCounter : public CS::Utility::AtomicRefCount
        {
        private:
            void Delete()
            {
            }
        } useCount;

        BgLoader* parent;
        bool loading;
        bool checked;
        size_t lingerCount;
    };

    // trivial loadable (e.g. triggers, textures, ...)
    template<typename T, const char* TypeName> class TrivialLoadable : public Loadable
    {
    public:
        TrivialLoadable(BgLoader* parent) : Loadable(parent)
        {
        }

        TrivialLoadable(const TrivialLoadable& other)
            : Loadable(other.GetParent()), path(other.path),
              data(other.data)
        {
        }

        bool LoadObject(bool wait)
        {
            if(!status.IsValid())
            {
                status = GetParent()->GetLoader()->LoadNode(path,data);
            }

            if(wait)
            {
                status->Wait();
            }

            if(status->IsFinished())
            {
                if(!status->WasSuccessful())
                {
                    LOADER_DEBUG_MESSAGE("%s %s failed to load\n", TypeName, GetName());
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        void UnloadObject()
        {
            csRef<T> ref = scfQueryInterface<T>(status->GetResultRefPtr());
            status.Invalidate();
            Loadable::CheckRemove<T,TypeName>(ref);
        }

        csPtr<T> GetObject()
        {
            csRef<T> obj = scfQueryInterface<T>(status->GetResultRefPtr());
            return csPtr<T>(obj);
        }

        void SetData(iDocumentNode* newData)
        {
            data = newData;
        }

    protected:
        // parse results
        csString path;
        csRef<iDocumentNode> data;

        // load data
        csRef<iThreadReturn> status;
    };

    // helper class that allows a specific dependency type for an object
    template<typename T> class ObjectLoader
    {
    public:
        typedef CheckedLoad<T> ObjectType;
        typedef csHash<ObjectType, csString> HashType;

        ObjectLoader() : objectCount(0)
        {
        }

        ObjectLoader(const ObjectLoader& other) : objectCount(0)
        {
            CS::Threading::RecursiveMutexScopedLock lock(other.busy);
            typename HashType::ConstGlobalIterator it(other.objects.GetIterator());
            while(it.HasNext())
            {
                csString key;
                ObjectType obj(it.Next(key));
                objects.Put(key, obj);
            }
        }

        ~ObjectLoader()
        {
            UnloadObjects();
        }

        size_t GetObjectCount() const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            return objectCount;
        }

        // load all dependencies of this type
        // return true if all are ready
        bool LoadObjects(bool wait = false)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(objectCount == objects.GetSize())
            {
                // nothing to be done
                return true;
            }

            bool ready = true;
            typename HashType::GlobalIterator it(objects.GetIterator());
            while(it.HasNext())
            {
                ObjectType& ref = it.Next();
                if(!ref.checked)
                {
                    ref.checked = ref.obj->Load(wait);
                    if(ref.checked)
                    {
                        ++objectCount;
                    }
                    else
                    {
                        ready = false;
                    }
                }
            }
            return ready;
        }

        // unloads all dependencies of this type
        void UnloadObjects()
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(!objectCount)
            {
                // nothing to be done
                return;
            }

            typename HashType::GlobalIterator it(objects.GetIterator());
            while(it.HasNext())
            {
                ObjectType& ref = it.Next();
                if(ref.checked)
                {
                    ref.obj->Unload();
                    ref.checked = false;
                    --objectCount;
                }
            }
        }

        int UpdateObjects(const csBox3& loadBox, const csBox3& keepBox)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            int oldObjectCount = objectCount;
            if(CS::Meta::IsBaseOf<RangeBased,T>::value)
            {
                typename HashType::GlobalIterator it(objects.GetIterator());
                while(it.HasNext())
                {
                    ObjectType& ref = it.Next();
                    if(ref.checked)
                    {
                        if(ref.obj->OutOfRange(keepBox))
                        {
                            ref.obj->Unload();
                            ref.checked = false;
                            --objectCount;
                        }
                    }
                    else if(ref.obj->InRange(loadBox))
                    {
                        ref.checked = ref.obj->Load(false);
                        if(ref.checked)
                        {
                            ++objectCount;
                        }
                    }
                }
            }
            else
            {
                LoadObjects(false);
            }
            return (int)objectCount - oldObjectCount;
        }

        void AddDependency(T* obj)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            if(!objects.Contains(obj->GetName()))
            {
                objects.Put(obj->GetName(), csRef<T>(obj));
            }
        }

        void RemoveDependency(const T* obj)
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            const ObjectType& ref = objects.Get(obj->GetName(), ObjectType(csRef<T>()));
            if(ref.checked)
            {
                ref.obj->Unload();
                --objectCount;
            }
            objects.DeleteAll(obj->GetName());
        }

        const csRef<T>& GetDependency(const csString& name, const csRef<T>& fallbackobj = csRef<T>()) const
        {
            CS::Threading::RecursiveMutexScopedLock lock(busy);
            ObjectType fallback(fallbackobj);
            const ObjectType& ref = objects.Get(name,fallbackobj);
            return ref.obj;
        }

        HashType& GetDependencies()
        {
            return objects;
        }

        const HashType& GetDependencies() const
        {
            return objects;
        }

    protected:
        mutable CS::Threading::RecursiveMutex busy;

        HashType objects;
        size_t objectCount;
    };

    struct WaterArea
    {
        csBox3 bbox;
        csColor4 colour;
    };

    class ShaderVar : public CS::Utility::AtomicRefCount
    {
    public:
        ShaderVar(BgLoader* parent, ObjectLoader<Texture>* object) : parent(parent), object(object)
        {
        }

        ~ShaderVar()
        {
        }

        template<bool check> bool Parse(iDocumentNode* node, GlobalParserData& data);
        void LoadObject(csShaderVariable* var);
        CS::ShaderVarStringID GetID() const
        {
            return nameID;
        }

    private:
        CS::ShaderVarStringID nameID;
        csShaderVariable::VariableType type;
        csString value;
        float vec1;
        csVector2 vec2;
        csVector3 vec3;
        csVector4 vec4;

        BgLoader* parent;
        ObjectLoader<Texture>* object;
    };

    class Texture : public TrivialLoadable<iTextureWrapper,ObjectNames::texture>
    {
    public:
        Texture(BgLoader* parent) : TrivialLoadable<iTextureWrapper,ObjectNames::texture>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);
    };

    class Material : public Loadable, public ObjectLoader<Texture>
    {
    public:
        using ObjectLoader<Texture>::AddDependency;

        Material(BgLoader* parent) : Loadable(parent)
        {
        }

        bool Parse(iDocumentNode* node, GlobalParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();
        csPtr<iMaterialWrapper> GetObject()
        {
            csRef<iMaterialWrapper> wrapper(materialWrapper);
            return csPtr<iMaterialWrapper>(wrapper);
        }

    private:
        // dependencies
        struct Shader
        {
            csRef<iShader> shader;
            csStringID type;
        };
        csArray<Shader> shaders;
        csRefArray<ShaderVar> shadervars;

        // load data
        csRef<iMaterialWrapper> materialWrapper;
    };

    class MaterialLoader : public ObjectLoader<Material>
    {
    public:
        void ParseMaterialReference(GlobalParserData& data, const char* name, const char* parentName, const char* type);
    };

    class Trigger : public TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>, public AlwaysLoaded
    {
    public:
        Trigger(BgLoader* parent) : TrivialLoadable<iSequenceTrigger,ObjectNames::trigger>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);
    };

    class Sequence : public ObjectLoader<Sequence>, public ObjectLoader<Trigger>,
                     public TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>,
                     public AlwaysLoaded
    {
    public:
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;

        Sequence(BgLoader* parent) : TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait)
        {
            bool ready = true;
            ready &= ObjectLoader<Trigger>::LoadObjects(wait);
            ready &= ObjectLoader<Sequence>::LoadObjects(wait);

            return ready && TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>::LoadObject(wait);
        }

        void UnloadObject()
        {
            ObjectLoader<Trigger>::UnloadObjects();
            ObjectLoader<Sequence>::UnloadObjects();
            TrivialLoadable<iSequenceWrapper,ObjectNames::sequence>::UnloadObject();
        }
    };

    class Light : public Loadable, public RangeBased, public ObjectLoader<Sequence>,
                  public ObjectLoader<Trigger>
    {
    public:
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;

        Light(BgLoader* parent) : Loadable(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

    private:
        // parse results
        csVector3 pos;
        float radius;
        csColor colour;
        csLightDynamicType dynamic;
        csLightAttenuationMode attenuation;
        csLightType type;

        // dependencies
        Sector* sector;

        // load data
        csRef<iLight> light;
    };

    class MeshFact : public TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>,
                     public MaterialLoader
    {
    public:
        using ObjectLoader<Material>::AddDependency;

        MeshFact(BgLoader* parent) : TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        csPtr<MeshFact> Clone(const char* name)
        {
            csRef<MeshFact> clone/*(this)*/;
            clone.AttachNew(new MeshFact(*this));
            clone->SetName(name);
            return csPtr<MeshFact>(clone);
        }

        bool operator==(const MeshFact& other)
        {
            if (filename != other.filename)
                return false;
            if ((iDocumentNode*)data != (iDocumentNode*)other.data)
                return false;

            return true;
        }

        bool LoadObject(bool wait);
        void UnloadObject();

        bool FindSubmesh(const csString& name) const
        {
            return submeshes.Find(name) != csArrayItemNotFound;
        }

        const csArray<csVector3>& GetVertices() const
        {
            return bboxvs;
        }

    private:
        // parser results
        csString filename;
        csArray<csVector3> bboxvs;
        csStringArray submeshes;
    };

    class MeshObj : public TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>,
                    public RangeBased, public ObjectLoader<Texture>,
                    public MaterialLoader, public ObjectLoader<MeshFact>,
                    public ObjectLoader<Sequence>, public ObjectLoader<Trigger>
    {
    public:
        using ObjectLoader<Texture>::AddDependency;
        using ObjectLoader<Material>::AddDependency;
        using ObjectLoader<MeshFact>::AddDependency;
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;

        MeshObj(BgLoader* parent) : TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>(parent), finished(false)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data, bool& alwaysLoaded);
        bool ParseTriMesh(iDocumentNode* node, ParserData& data, bool& alwaysLoaded);

        bool LoadObject(bool wait);
        void UnloadObject();
        void FinishObject();

        bool FindSubmesh(const csString& name) const
        {
            typedef ObjectLoader<MeshFact>::HashType HashType;
            const HashType& factories = ObjectLoader<MeshFact>::GetDependencies();
            HashType::ConstGlobalIterator it(factories.GetIterator());
            bool found = false;
            while(it.HasNext() && !found)
            {
                const csRef<MeshFact>& factory = it.Next().obj;
                found |= factory->FindSubmesh(name);
            }
            return found;
        }

    private:
        // parse results
        bool dynamicLighting;

        // dependencies
        Sector* sector;
        csRefArray<ShaderVar> shadervars;

        // load data
        csRef<iThreadReturn> status;
        bool finished;
    };

    class MeshGen : public TrivialLoadable<iMeshGenerator,ObjectNames::meshgen>, public RangeBased, public MaterialLoader,
                    public ObjectLoader<MeshFact>
    {
    public:
        using ObjectLoader<MeshFact>::AddDependency;
        using ObjectLoader<Material>::AddDependency;

        MeshGen(BgLoader* parent) : TrivialLoadable<iMeshGenerator,ObjectNames::meshgen>(parent)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

        // parser data
        csString name;
        csRef<iDocumentNode> data;
        Sector* sector;

        // dependencies
        csRef<MeshObj> mesh;

        // load data
        csRef<iThreadReturn> status;
    };

    class Portal : public Loadable, public RangeBased
    {
    public:
        Portal(BgLoader* parent) : Loadable(parent), flags(0), warp(false)
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();

        // parser results
        csString name;
        csMatrix3 matrix;
        csVector3 wv;
        bool ww_given;
        csVector3 ww;
        csReversibleTransform transform;
        uint32 flags;
        bool warp;
	bool autoresolve;
        csPoly3D poly;

        csRef<Sector> targetSector;
        Sector* sector;

        // load data
        iPortal* pObject;
        csRef<iMeshWrapper> mObject;

        // sector callback
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

    class Sector : public Loadable,
                   public ObjectLoader<MeshGen>, public ObjectLoader<MeshObj>,
                   public ObjectLoader<Portal>, public ObjectLoader<Light>,
                   public ObjectLoader<Sequence>, public ObjectLoader<Trigger>
    {
    public:
        using ObjectLoader<MeshGen>::AddDependency;
        using ObjectLoader<MeshObj>::AddDependency;
        using ObjectLoader<Portal>::AddDependency;
        using ObjectLoader<Light>::AddDependency;
        using ObjectLoader<Sequence>::AddDependency;
        using ObjectLoader<Trigger>::AddDependency;

        Sector(BgLoader* parent) : Loadable(parent), ambient(0.0f), objectCount(0),
                                   init(false), isLoading(false)
        {
        }

        ~Sector()
        {
        }

        bool Parse(iDocumentNode* node, ParserData& data);

        bool LoadObject(bool wait);
        void UnloadObject();
        int UpdateObjects(const csBox3& loadBox, const csBox3& keepBox, size_t recursions);

        bool Initialize();
        void ForceUpdateObjectCount();
        void FindConnectedSectors(csSet<csPtrKey<Sector> >& connectedSectors);

        void AddPortal(Portal* p)
        {
            activePortals.Add(p);
        }

        void RemovePortal(Portal* p)
        {
            activePortals.Delete(p);
        }

        void AddAlwaysLoaded(MeshObj* mesh)
        {
            alwaysLoaded.AddDependency(mesh);
        }

        // parser data
        CS::Threading::ReadWriteMutex lock;

        // parser results
        csString culler;
        csColor ambient;
        size_t objectCount;
        Zone* parent;

        // dependencies
        csArray<WaterArea> waterareas;
        ObjectLoader<MeshObj> alwaysLoaded;

        // load data
        csRef<iSector> object;
        csSet<csPtrKey<Portal> > activePortals;
        bool init;
        bool isLoading;
    };

    // Stores world representation.
    class Zone : public Loadable, public ObjectLoader<Sector>
    {
    public:
        using ObjectLoader<Sector>::AddDependency;

        Zone(BgLoader* parent, const char* name)
            : Loadable(parent), loading(false), priority(false)
        {
            Loadable::SetName(name);
        }

        bool LoadObject(bool wait)
        {
            return ObjectLoader<Sector>::LoadObjects(wait);
        }

        void UnloadObject()
        {
            ObjectLoader<Sector>::UnloadObjects();
        }

        void UpdatePriority(bool newPriority)
        {
            if(priority != newPriority)
            {
                priority = newPriority;

                // upgrade sector priorities
                /*ObjectLoader<Sector>::HashType::GlobalIterator it(ObjectLoader<Sector>::objects.GetIterator());
                while(it.HasNext())
                {
                    it.Next().obj->priority = newPriority;
                }*/
            }
        }
        
        bool GetPriority() const
        {
            return priority;
        }

        // loader data
        bool loading;
        bool priority;
    };

    struct GlobalParserData
    {
        // token lookup table
        csStringHash xmltokens;

        // temporary parse-time data
        LockedType<Texture> textures;
        LockedType<MeshObj> meshes;
        csRef<iShaderVarStringSet> svstrings;

        // plugin references
        csRef<iSyntaxService> syntaxService;
        iObjectRegistry* object_reg;

        // persistent data
        csRef<iStringSet> strings;
        LockedType<Material> materials;
        LockedType<Sector> sectors;
        LockedType<MeshFact> factories;
        LockedType<Zone> zones;
        LockedType<StartPosition,false> positions;
        CS::Threading::ReadWriteMutex shaderLock;
        csHash<csString, csStringID> shadersByUsage;
        csStringArray shaders;

        // config
        struct ParserConfig
        {
            bool cache;
            uint enabledGfxFeatures;
            bool portalsOnly;
            bool meshesOnly;
            bool parseShaders;
            bool blockShaderLoad;
            bool parsedShaders;
            bool parseShaderVars;
        } config;
    } parserData;

    struct ParserData
    {
        ParserData(GlobalParserData& data) : data(data)
        {
        }

        // global data
        GlobalParserData& data;

        // temporary data used on a per-library basis
        csRefArray<iThreadReturn> rets;
        Zone* zone;
        csRef<Sector> currentSector;
        LockedType<Light> lights;
        LockedType<Sequence> sequences;
        LockedType<Trigger> triggers;
        csString vfsPath;
        csString path;
        bool realRoot;
        bool parsedMeshFact;
    };

    /***********************************************************************/

    /* Parsing Methods */
    // parser tokens
#define CS_TOKEN_ITEM_FILE "src/common/bgloader/parser.tok"
#define CS_TOKEN_LIST_TOKEN_PREFIX PARSERTOKEN_
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
#undef CS_TOKEN_LIST_TOKEN_PREFIX

    void ParseMaterials(iDocumentNode* materialsNode);

    /* shader methods */
    void ParseShaders();

    /* Internal unloading methods. */
    void CleanDisconnectedSectors(Sector* sector);

    // Pointers to other needed plugins.
    iObjectRegistry* object_reg;
    csRef<iEngine> engine;
    csRef<iGraphics2D> g2d;
    csRef<iThreadedLoader> tloader;
    csRef<iThreadManager> tman;
    csRef<iVFS> vfs;
    csRef<iCollideSystem> cdsys;
    CS::Threading::RecursiveMutex vfsLock;

    // currently loaded zones - used by zone-based loading
    ObjectLoader<Zone> loadedZones;

    // currently loading objects
    csArray<csPtrKey<Loadable> > loadList;

    // number of objects currently loading
    size_t loadCount;

    // Our load range ^_^
    float loadRange;

    // Whether the current position is valid.
    bool validPosition;

    // Limit on how many portals deep we load.
    uint maxPortalDepth;

    // Number of checks an object may be lingering
    // without aborting the load
    size_t maxLingerCount;

    // The last valid sector.
    csRef<Sector> lastSector;

    // The last valid position.
    csVector3 lastPos;

    // current load step - use for ContinueLoading
    size_t loadStep;

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

