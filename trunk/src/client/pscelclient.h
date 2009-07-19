/*
 * psnetmanager.cpp by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __psCelClient_H__
#define __psCelClient_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/cscolor.h>
#include <csutil/parray.h>
#include <csutil/refarr.h>
#include <csutil/list.h>
#include <csutil/hash.h>
#include <csutil/redblacktree.h>
#include <csutil/strhashr.h>
#include <iengine/collection.h>
#include <iengine/sector.h>

//=============================================================================
// Project Includes
//=============================================================================

#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psengine.h"

struct Trait;
struct iVFS;
struct iSpriteCal3DState;
struct iMeshObject;

class MsgHandler;
class ZoneHandler;
class psClientDR;
class psNetPersist;
class psClientVitals;
class psCharAppearance;
class psEntityLabels;
class psShadowManager;
class psWorld;
class psPersistActor;
class psPersistItem;
class GEMClientObject;
class GEMClientActor;
class GEMClientItem;
class GEMClientActionLocation;
class psEffect;
class psSolid;

class psLinearMovement;

/** This is information about an entity with unresolved position (=sector not found)
  * This happens when some entity is located in map that is not currently loaded.
  * We are trying to resolve the position every time new maps are loaded */
class UnresolvedPos
{
public:
    GEMClientObject * entity;  ///< our object ..

    csVector3 pos;             ///< .. and its position that could not be set
    float rot;
    csString sector;

    UnresolvedPos(GEMClientObject * entity, const csVector3 & pos, float rot, const csString & sector)
    {
        this->entity  = entity;
        this->pos     = pos;
        this->rot     = rot;
        this->sector  = sector;
    }
};

struct InstanceObject : public CS::Utility::FastRefCount<InstanceObject>
{
    csRef<iMeshWrapper> pcmesh;
    csBox3 bbox;
};

/**
 * Client version of the Cel Manager
 */
class psCelClient : public psClientNetSubscriber
{
private:
    csRef<iObjectRegistry> object_reg;
    csPDelArray<GEMClientObject> entities;
    csHash<GEMClientObject*, EID> entities_hash;
    csRefArray<MsgEntry> newActorQueue;
    csRefArray<MsgEntry> newItemQueue;

    // Keep seperate for speedups
    csArray<GEMClientActionLocation*> actions;

    struct Effect
    {
        csString effectname;
        csVector3 effectoffset;
        bool rotateWithMesh;
    };

    struct Light
    {
        csColor colour;
        csVector3 lightoffset;
        float radius;
    };

    struct ItemEffect
    {
        bool activeOnGround;
        csPDelArray<Effect> effects;
        csPDelArray<Light> lights;
    };

    csHash<ItemEffect*, csString> effectItems;

    /// Load items that have effects attached to them into the effectItems array.
    void LoadEffectItems();

    bool ignore_others;

    csRedBlackTreeMap<csString, csRef<InstanceObject> > instanceObjects;

    csRef<iMeshFactoryWrapper> nullfact;

public:

    psCelClient();
    virtual ~psCelClient();

    bool Initialize (iObjectRegistry* object_reg, MsgHandler* msghandler,
                     ZoneHandler *zonehndlr);

    void RequestServerWorld();
    bool IsReady();

    GEMClientObject* FindObject(EID eid);

    void SetMainActor(GEMClientActor* object);
    void SetPlayerReady(bool flag);

    void RemoveObject(GEMClientObject* entity);

    psClientDR* GetClientDR()       { return clientdr; }
    const csPDelArray<GEMClientObject>& GetEntities () const { return entities; }
    bool IsMeshSubjectToAction(const char* sector,const char* mesh);
    GEMClientActor * GetActorByName(const char * name, bool trueName = true) const;

    virtual void HandleMessage(MsgEntry* me);

    void SetupWorldColliders();

    psEntityLabels * GetEntityLabels() { return entityLabels; }
    psShadowManager * GetShadowManager() { return shadowManager; }

    GEMClientActor* GetMainPlayer() { return local_player; }

    ZoneHandler* GetZoneHandler() { return zonehandler; }

    /** Check if the item has an effect attached to it and process it if so. */
    void HandleItemEffect( const char* factName, csRef<iMeshWrapper> mw, bool onGround = true, const char* slot = 0,
                           csHash<int, csString> *effectids = 0, csHash<int, csString> *lightids = 0 );

    /** Called when new world maps were loaded
        CelClient looks for GEM Objects which have sectors with unknown name and checks if this name is known now */
    void OnMapsLoaded();

    /** Called when a region of the world is deleted from the client (because we don't need it loaded now)
        CelClient removes all GEMClientObjects that are in this region */
    void OnRegionsDeleted(csArray<iCollection *>& regions);

    psWorld* GetWorld() { return gameWorld; }

    /** This is called when position of some entity could not be resolved (see the UnresolvedPos struct)
      * It adds this position to list of unresolved positions which we will attempt to resolve later
      * and moves the entity to special sector that keeps these unfortunate entities.
      */
    void HandleUnresolvedPos(GEMClientObject * entity, const csVector3 & pos, float rot, const csString & sector);

    void PruneEntities();

    bool IsUnresSector(iSector* sector) { return unresSector == sector;}

    int GetRequestStatus() { return requeststatus; }

    void IgnoreOthers(bool state) { ignore_others = state; }

    bool IsIgnoringOthers() { return ignore_others; }

    /// Add one new actor or item entity from the queue.
    void CheckEntityQueues();
    /// Add all new entities on the queue.
    void ForceEntityQueues();

    void Update(bool loaded);


    /** Attach a client object to a Crystal Space object.
      * In most cases the Crystal Space object is a meshwrapper.
      *
      * @param object The Crystal Space object we want to attach our client object to.
      * @param clientObject The client object we want to attach.
      */
    void AttachObject( iObject* object, GEMClientObject* clientObject);

    /** Unattach a client object from a Crystal Space object.
      * In most cases the Crystal Space object is a meshwrapper.
      *
      * @param object The Crystal Space object we want to unattach our client object from.
      * @param clientObject The client object we want to unattach.
      */
    void UnattachObject( iObject* object, GEMClientObject* clientObject);

    /** See if there is a client object attached to a given object.
      *
      * @param object The Cyrstal Space object we want to see if there is a client object attached to.
      *
      * @return A GEMClientObject if it exists that is attached to the Crystal Space object.
      */
    GEMClientObject* FindAttachedObject (iObject* object);


    /** Create a list of all nearby GEM objects.
      * @param sector The sector to check in.
      * @param pos The starting position
      * @param radius The distance around the starting point to check.
      * @param doInvisible If true check invisible meshes otherwise ignore them.
      *
      * @return A csArray<> of all the objects in the given radius.
      */
    csArray<GEMClientObject*> FindNearbyEntities (iSector* sector, const csVector3& pos, float radius, bool doInvisible = false);

    /**
     * Search for an instance object and return it if existing. Else return 0 csPtr.
     */
    csPtr<InstanceObject> FindInstanceObject(const char* name) const;

    /**
     * Add an instance object to the tree.
     */
    void AddInstanceObject(const char* name, csRef<InstanceObject> object);

    /**
     * Returns the nullmesh factory.
     */
    iMeshFactoryWrapper* GetNullFact() const
    {
        return nullfact;
    }

protected:
    friend class GEMClientActor;

    /** Finds given entity in list of unresolved entities */
    csList<UnresolvedPos*>::Iterator FindUnresolvedPos(GEMClientObject * entity);

    int requeststatus;
    csRef<iVFS>         vfs;
    csRef<MsgHandler>   msghandler;
    psClientDR* clientdr;
    ZoneHandler *zonehandler;

    psEntityLabels * entityLabels;
    psShadowManager * shadowManager;

    psWorld* gameWorld;

    void HandleWorld( MsgEntry* me );
    void HandleActor( MsgEntry* me );
    void HandleItem( MsgEntry* me );
    void HandleActionLocation( MsgEntry* me );
    void HandleObjectRemoval( MsgEntry* me );
    void HandleNameChange( MsgEntry* me );
    void HandleGuildChange( MsgEntry* me );
    void HandleGroupChange( MsgEntry* me );

    /** Handles a stats message from the server.
      * This basically just publishes the data to PAWS so various widgets can be updated.
      */
    void HandleStats( MsgEntry* me );

    void RequestActor();

    GEMClientActor* local_player;

    /// Handle a change in the main actor
    void HandleMainActor( psPersistActor& mesg );
    csRef<iMeshObject>          local_player_defaultMesh;
    csRef<iMeshFactoryWrapper>  local_player_defaultFact;
    csString                    local_player_defaultFactName;

    csList<UnresolvedPos*> unresPos;   ///< list of entities with unresolved location
    iSector * unresSector;             ///< sector where we keep these entities
};

enum GEMOBJECT_TYPE
{
    GEM_OBJECT = 0,
    GEM_ACTOR,
    GEM_ITEM,
    GEM_ACTION_LOC,
    GEM_TYPE_COUNT
};

/** An object that the client knows about. This is the base object for any
  * 'entity' that the client can be sent.
  */
class GEMClientObject : public DelayedLoader
{
public:
    GEMClientObject();
    GEMClientObject(psCelClient* cel, EID id);
    virtual ~GEMClientObject();

    virtual GEMOBJECT_TYPE GetObjectType() { return GEM_OBJECT; }

    /** Setup the mesh for this object.
    *
    *   @return true if the mesh was set correctly, false if mesh failed to be set or created.
    */
    bool InitMesh();

    /** Set the position of mesh 
     * @param pos the coordinates of the mesh
     * @param rotangle the y axis rotation of the mesh
     * @param room the sector in which the mesh is moved to
     */
    void Move(const csVector3& pos,float rotangle, const char* room);

    /** Set the rotation of mesh 
     * @param xRot the variable used to set the x rotation of the item
     * @param yRot the variable used to set the x rotation of the item
     * @param zRot the variable used to set the z rotation of the item
     */
	void Rotate(float xRot, float yRot, float zRot);

    /** Set position of entity */
    virtual bool SetPosition(const csVector3 & pos, float rot, iSector * sector);

    /** Get position of entity */
    virtual csVector3 GetPosition();

    /** Get rotation of entity */
    virtual float GetRotation();

    /** Get sector of entity */
    virtual iSector* GetSector() const;

    /** Get list of sectors that entity is in */
    virtual iSectorList* GetSectors() const;

    /** Return the bounding box of this entity. */
    virtual const csBox3& GetBBox() const;

    EID GetEID() { return eid; }
    csRef<iMeshWrapper> pcmesh;

    virtual int GetMasqueradeType();

    int GetType() { return type; }

    virtual const char* GetName() { return name; }
    virtual void ChangeName(const char* name);

    const char* GetFactName() { return factName; }

    psEffect* GetEntityLabel() { return entitylabel; }
    void      SetEntityLabel(psEffect* el) { entitylabel = el; }

    psEffect * GetShadow() { return shadow; }
    void SetShadow(psEffect * shadow) { this->shadow = shadow; }

    /**
     * Indicate if this object is alive
     */
    virtual bool IsAlive() { return false; }

    /** Get the flag bit field.
      * @return The bit field that contains the flags on this actor.
      */
    int Flags() { return flags; }

     psCharAppearance* charApp;

     /** Get the mesh that this object has.
      * @return The iMeshWrapper or 0 if no mesh.
      */
     csRef<iMeshWrapper> GetMesh() const;

     virtual void Update();

     float RangeTo(GEMClientObject * obj, bool ignoreY);

     bool HasShadow() { return hasShadow; }

     /**
      * Delayed mesh loading.
      */
     virtual void CheckLoadStatus() {}

     /**
      * Delayed load 'post-process'.
      */
     virtual void PostLoad(bool nullmesh) { }

protected:
    friend class psCelClient;

    static psCelClient *cel;

    csString name;
    csString factName;
    EID eid;
    int type;

    int flags;                      ///< Various flags on the entity.
    psEffect* entitylabel;
    psEffect * shadow;
    bool hasShadow;

    csRef<InstanceObject> instance;
    csRef<csShaderVariable> position;
};

class psDRMessage;


//---------------------------------------------------------------------------



//--------------------------------------------------------------------------

/** This is a player or another 'alive' entity on the client. */
class GEMClientActor : public GEMClientObject
{
public:

    GEMClientActor( psCelClient* cel, psPersistActor& mesg );
    virtual ~GEMClientActor();

    virtual GEMOBJECT_TYPE GetObjectType() { return GEM_ACTOR; }


    /** Get the last position of this object.
      *
      * @param pos The x,y,z location of the object. [CHANGED]
      * @param yrot The Y-Axis rotation of the object. [CHANGED]
      * @param sector The sector of the object is in [CHANGED]
      */
    void GetLastPosition (csVector3& pos, float& yrot, iSector*& sector);

    /** Get the object velocity.
      *
      * @return The velocity of the object as a vector.
      */
    const csVector3 GetVelocity () const;

    virtual bool SetPosition(const csVector3 & pos, float rot, iSector * sector);

    void SetAlive( bool aliveFlag, bool newactor );
    virtual bool IsAlive() { return alive; }
    virtual int GetMasqueradeType() { return masqueradeType; }

    /** Get the condition manager on this actor.
      */
    psClientVitals* GetVitalMgr() { return vitalManager; }

    csVector3 Pos() const;
    csVector3 Rot() const;
    iSector *GetSector() const;

    virtual const char* GetName(bool realName = true);

    const char* GetGuildName() { return guildName; }
    void SetGuildName(const char* guild) { guildName = guild; }

    bool NeedDRUpdate(unsigned char& priority);
    void SendDRUpdate(unsigned char priority,csStringHashReversible* msgstrings);
    void SetDRData(psDRMessage& drmsg);
    void StopMoving(bool worldVel = false);

    psLinearMovement * linmove;

    /// The Vital of the player with regards to his health/mana/fatigue/etc.
    psClientVitals *vitalManager;

    void SetMode(uint8_t mode, bool newactor = false);
    uint8_t GetMode() { return serverMode; }
    void SetIdleAnimation(const char* anim);
    void SetAnimationVelocity(const csVector3& velocity);
    bool SetAnimation(const char* anim, int duration=0);
    void RefreshCal3d();  ///< Reloads iSpriteCal3DState

    void SetChatBubbleID(unsigned int chatBubbleID);
    unsigned GetChatBubbleID() const;

    csRef<iSpriteCal3DState> cal3dstate;
    bool control;

    /**
     * This optimal routine tries to get the animation index given an
     * animation csStringID.
     */
    int GetAnimIndex (csStringHashReversible* msgstrings, csStringID animid);

    // The following hash is used by GetAnimIndex().
    csHash<int,csStringID> anim_hash;

    /**
      * Delayed mesh loading.
      */
     virtual void CheckLoadStatus();

    csString race;
    csString helmGroup;
    csString BracerGroup;
    csString BeltGroup;
    csString CloakGroup;
    csString equipment;
    csString traits;
    csVector3 lastSentVelocity,lastSentRotation;
    bool stationary,path_sent;
    csTicks lastDRUpdateTime;
    bool ready;
    unsigned short gender;

    // Access functions for the group var
    bool IsGroupedWith(GEMClientActor* actor);
    unsigned int GetGroupID() { return groupID; }
    void SetGroupID(unsigned int id) { groupID = id; }
    EID  GetOwnerEID()       { return ownerEID; }
    void SetOwnerEID(EID id) { ownerEID = id; }

    csPDelArray<Trait> traitList;

    /** Get the movment system this object is using.
      */
    psLinearMovement * GetMovement();

    virtual void Update();

protected:

    unsigned int chatBubbleID;
    unsigned int groupID;
    EID ownerEID;
    csString guildName;
    uint8_t  DRcounter;  ///< increments in loop to prevent out of order packet overwrites of better data
    bool DRcounter_set;

    virtual void PostLoad(bool nullmesh);

    bool InitLinMove(const csVector3& pos,float angle, const char* sector,
                     csVector3 top, csVector3 bottom, csVector3 offset);

    bool InitCharData(const char* textures, const char* equipment);

    bool alive;

    int masqueradeType;

    void SetCharacterMode(size_t id);
    size_t movementMode;
    uint8_t serverMode;

    /// Post load data.
    struct PostLoadData
    {
        csVector3 pos;
        float yrot;
        csString sectorName;
        iSector* sector;
        csVector3 top;
        csVector3 bottom;
        csVector3 offset;
        bool on_ground;
        csVector3 vel;
        csVector3 angularVelocity;
        csVector3 worldVel;
        float ang_vel;
        csString texParts;
    };

    PostLoadData* post_load;
};

/** An item on the client. */
class GEMClientItem : public GEMClientObject
{
public:
    GEMClientItem( psCelClient* cel, psPersistItem& mesg );
    virtual ~GEMClientItem();

    virtual GEMOBJECT_TYPE GetObjectType() { return GEM_ITEM; }
    
    void UpdateItem( psPersistItem& mesg );

    /**
      * Delayed mesh loading.
      */
     virtual void CheckLoadStatus();

protected:
    virtual void PostLoad(bool nullmesh);

    psSolid* solid;

private:
    /// Post load data.
    struct PostLoadData
    {
        csVector3 pos;
        float xRot;
        float yRot;
        float zRot;
        csString sector;
        uint32_t flags;
    };

    PostLoadData* post_load;
};

/** An action location on the client. */
class GEMClientActionLocation : public GEMClientObject
{
public:
    GEMClientActionLocation( psCelClient* cel, psPersistActionLocation& mesg );

    virtual GEMOBJECT_TYPE GetObjectType() { return GEM_ACTION_LOC; }

    const char* GetMeshName() { return meshname; }

protected:
    csString meshname;
};

#endif

