/*
 * gem.h - author Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 * This is the cel access class for PS.
 */

#ifndef __GEM_H__
#define __GEM_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <csutil/csobject.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"

#include "util/gameevent.h"
#include "util/consoleout.h"

#include "net/npcmessages.h"  // required for psNPCCommandsMessage::PerceptionType

//=============================================================================
// Local Space Includes
//=============================================================================
#include "msgmanager.h"
#include "deathcallback.h"

struct iMeshWrapper;

class ProximityList;
class ServerCharManager;
class EntityManager;
class gemObject;
class PlayerGroup;
class psDatabase;
class psItem;
class csMatrix3;
class NPCManager;
class psGlyphList;
class FactionSet;
class ProgressionManager;
class psNPCDialog;
class psAllEntityPosMessage;
class psActionLocation;
class MathScript;
class gemItem;
class gemActor;
class gemNPC;
class gemPet;
class ClientConnectionSet;
class PublishVector;
class psLinearMovement;
class gemMesh;

#define BUFF_INDICATOR          "+"
#define DEBUFF_INDICATOR        "-"

#define UNSTICK_TIME 15000


//-----------------------------------------------------------------------------

/** Helper class to attach a PlaneShift gem object to a particular mesh.
  */
class psGemServerMeshAttach : public scfImplementationExt1<psGemServerMeshAttach,
                                                           csObject,
                                                           scfFakeInterface<psGemServerMeshAttach> >
{
public:
    SCF_INTERFACE(psGemServerMeshAttach, 0, 0, 1);

    /** @brief Setup this helper with the object we want to attach with.
     *  @param object  The gemObject we want to attach to a mesh.
     */
    psGemServerMeshAttach(gemObject* object);

    /** @brief Get the gemObject that the mesh has attached.
     */
    gemObject* GetObject() { return object; }

private:
    gemObject* object;          ///< The object that is attached to a iMeshWrapper object.
};

//-----------------------------------------------------------------------------

/**
* This class holds the refs to the core factories, etc in CEL.
*/
class GEMSupervisor : public MessageManager, public Singleton<GEMSupervisor>
{
public:
    iObjectRegistry*        object_reg;                     ///< The Crystal Space Object Registry.
    psDatabase             *database;                       ///< The main PlaneShift database object.
    NPCManager             *npcmanager;                     ///< NPC controller.

    /** @brief Create a new singleton of the GEM Supervisor object.
      * @param objreg The Crystal Space Object Registry.
      * @param db The main PlaneShift database.
      */
    GEMSupervisor(iObjectRegistry *objreg, psDatabase *db);

    /** @brief Destroy this singleton.
     *
     *  This will only be ever called on a server shutdown.
     */
    virtual ~GEMSupervisor();

    /** @brief Get a hash of all the current entities on the server.
     *
     *  @return a csHash of all the gemObjects.
     */
    csHash<gemObject*, EID> & GetAllGEMS() { return entities_by_eid; }

    // Search functions
    /** @brief Find an entity ID for an item.
     *
     *  @param item The psItem that we want to find the entity ID for.
     *
     *  @return the EID of that item if it was found. 0 if no id could be found.
     */
    EID        FindItemID(psItem *item);
    gemObject *FindObject(EID cel_id);
    gemObject *FindObject(const csString& name);


    gemActor  *FindPlayerEntity(PID player_id);
    gemNPC    *FindNPCEntity(PID npc_id);
    gemItem   *FindItemEntity(uint32 item_id);

    EID  CreateEntity(gemObject *obj);
    void RemoveEntity(gemObject *which);
    void AddActorEntity(gemActor *actor);
    void RemoveActorEntity(gemActor *actor);
    void AddItemEntity(gemItem *item);
    void RemoveItemEntity(gemItem *item);

    void RemoveClientFromLootables(int cnum);

    void UpdateAllDR();
    void UpdateAllStats();

    void GetAllEntityPos(psAllEntityPosMessage& msg);
    int  CountManagedNPCs(AccountID superclientID);
    void FillNPCList(MsgEntry *msg, AccountID superclientID);
    void StopAllNPCs(AccountID superclientID);

    /** @brief Gets a list of all the 'live' entities that this player has ownership of.
      *
      * This can be things like items in containers or work items.
      *
      * @param playerID The character owner ID we are looking for.
      * @param list The populated list of items that are active in game.
      */
    void GetPlayerObjects(PID playerID, csArray<gemObject*> &list);

    /** @brief Teleport a player to a location.
     *
     *  @param object The player to move
     *  @param x,y,z  Location to move to
     *  @param rot    The rotation to use.
     *  @param sector The sector name to move to.
     */
    void Teleport( gemObject* object, float x, float y, float z, float rot, const char* sectorname );

    void HandleMessage(MsgEntry *me, Client *client);


   /** @brief Attach a server gemObject to a Crystal Space object.
     *
     * In most cases this will be a mesh wrapper.
     *
     * @param object The Crystal Space object we want to attach our gemObject to.
     * @param gobject The PlaneShift object that we want to attach.
     */
    void AttachObject( iObject* object, gemObject* gobject);


    /** @brief Unattach a gemObject from a Crystal Space object.
      *
      * In most cases the Crystal Space object is a meshwrapper.
      *
      * @param object The Crystal Space object we want to unattach our client object from.
      * @param gobject The gem object we want to unattach.
      */
    void UnattachObject( iObject* object, gemObject* gobject);


    /** @brief See if there is a gemObject attached to a given Crystal Space object.
      *
      * @param object The Cyrstal Space object we want to see if there is an object attached to.
      *
      * @return A gemObject if it exists that is attached to the Crystal Space object.
      */
    gemObject* FindAttachedObject (iObject* object);


    /** @brief Create a list of all nearby gem objects.
      *
      * @param sector The sector to check in.
      * @param pos The starting position
      * @param radius The distance around the starting point to check.
      * @param doInvisible If true check invisible meshes otherwise ignore them.
      *
      * @return A csArray<> of all the objects in the given radius.
      */
    csArray<gemObject*> FindNearbyEntities (iSector* sector, const csVector3& pos, float radius, bool doInvisible = false);

protected:
    /** @brief Get the next ID for an object.
      *
      * @return The next ID available that can be assigned to an object.
      */
    EID GetNextID();

    csHash<gemObject*, EID> entities_by_eid; ///< A list of all the entities stored by EID (entity/gem ID).
    csHash<gemItem*, uint32> items_by_uid;   ///< A list of all the items stored by UID (psItem ID).
    csHash<gemActor*,  PID> actors_by_pid;   ///< A list of all the actors stored by PID (player/character ID).

    int                 count_players;       ///< Total Number of players

    uint32              nextEID;             ///< The next ID available for an object.

};

//-----------------------------------------------------------------------------

/**
* A gemObject is any solid, graphical object visible in PS with normal physics
* and normal collision detection.
*/
class gemObject : public iDeleteNotificationObject, public CS::Utility::WeakReferenced
{

public:
    gemObject(const char* name, const char* factname,const char* filename,InstanceID myinstance,iSector* room,
        const csVector3& pos,float rotangle,int clientnum);

    /// This ctor is only for use in making keys for the BinaryTree
    gemObject(const char *name);

    virtual ~gemObject();

    EID GetEID() { return eid; }

    /// Called when a client disconnects
    virtual void Disconnect();

    virtual bool IsValid(void) { return eid.IsValid(); }

    /// Returns whether the object is alive.
    bool IsAlive() const { return is_alive; }
    void SetAlive(bool flag);

    uint32 GetClientID();

    virtual const char* GetObjectType() { return "Object"; }

    virtual psItem *GetItem() { return NULL; }
    virtual gemActor* GetActorPtr() { return NULL; }
    virtual gemNPC* GetNPCPtr()  { return NULL; }
    virtual gemPet* GetPetPtr() { return NULL; }

    virtual psCharacter *GetCharacterData() { return NULL; }
    virtual Client* GetClient() const { return NULL; }

    const char *GetName();
    void SetName(const char* n);

    void SetInstance(InstanceID newInstance) { worldInstance = newInstance; }
    InstanceID  GetInstance()                { return worldInstance; }

    void RegisterCallback(iDeleteObjectCallback * receiver) { receivers.Push(receiver); }
    void UnregisterCallback(iDeleteObjectCallback * receiver) { receivers.Delete(receiver); }

    // Mesh related functions
    iMeshWrapper *GetMeshWrapper();
    csString GetMesh() { return factname; };
    void Move(const csVector3& pos,float rotangle,iSector* room);
    bool IsNear(gemObject *obj,float radius);
    void GetPosition(csVector3& pos, float& yrot,iSector*& sector);
    void GetPosition(csVector3& pos, iSector*& sector);
    float GetAngle();
    iSector* GetSector();
    int FindAnimIndex(const char *name);

    csArray<gemObject*> *GetObjectsInRange( float range );

    // Proxlist related functions
    ProximityList *GetProxList() { return proxlist; };
    csArray<PublishDestination>& GetMulticastClients();

    /** Generates proxlist if needed (or forced)
      * Then removes entities of nearby objects at clients, if needed */
    void UpdateProxList( bool force = false);
    void RemoveFromAllProx();

    float RangeTo(gemObject *obj, bool ignoreY = false);

    virtual bool IsUpdateReq (csVector3 const &pos,csVector3 const &oldPos);

    /** This value indicates the range that this entity would become visible
     *  to other entities if no other modifiers were taken into consideration. */
    virtual float GetBaseAdvertiseRange() { return DEF_PROX_DIST; };

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);
    /** Dump debug information. Used in the com_print function
     */
    virtual void Dump();

    // Networking functions
    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool to_superclient) {}
    virtual void SendGroupMessage(MsgEntry *me) { };

    // Overridden functions in child classes
    virtual PSCHARACTER_MODE GetMode() { return PSCHARACTER_MODE_UNKNOWN; }
    virtual void SetMode(PSCHARACTER_MODE mode) { }
    virtual PID GetPID() { return 0; }
    virtual int GetGuildID() { return 0; }
    virtual psGuildInfo* GetGuild() { return 0; }
    virtual bool UpdateDR() { return false; }
    virtual void BroadcastTargetStatDR(ClientConnectionSet *clients) { }
    virtual void SendTargetStatDR(Client *client) { }
    virtual psNPCDialog *GetNPCDialogPtr() { return 0; }
    virtual void GetLastSuperclientPos(csVector3& pos, InstanceID& instance) const { }
    virtual void SetLastSuperclientPos(const csVector3& pos, InstanceID instance) { }
    virtual void AddLootableClient(int cnum) { }
    virtual void RemoveLootableClient(int cnum) { }
    virtual bool IsLootableClient(int cnum) { return false; }
    virtual Client *GetRandomLootClient(int range) { return NULL; }
    virtual AccountID GetSuperclientID() { return 0; }
    virtual void SetSuperclientID(AccountID id) { }

    virtual bool GetVisibility() { return true; }
    virtual bool SeesObject(gemObject * object, float range) { return false; }

    virtual gemObject* GetOwner() { return NULL; }

protected:
    bool valid;                                 ///< Is object fully loaded
//  csRef<gemObjectSafe> self_reference;        ///< Placeholder for ref 1 of saferef

    gemMesh* pcmesh;                            ///< link to mesh class
    ProximityList *proxlist;                    ///< Proximity List for this object
    csString name;                              ///< Name of this object, used mostly for debugging
    static GEMSupervisor *cel;                  ///< Static ptr back to main collection of all objects
    InstanceID worldInstance;                  ///< Only objects which match instances can see each other
    csVector3 pos;                              ///< Position in 3d space
    float yRot;                                 ///< Left-Right rotation, in radians
    iSector *sector;                            ///< Ptr to the CS sector inhabited
    bool is_alive;                              ///< Flag indicating whether object is alive or not
    csString factname;                          ///< Name of CS Mesh Factory used to create this object
    csString filename;                          ///< VFS Filename of mesh
    EID eid;                                    ///< Entity ID (unique identifier for object)

    csArray<iDeleteObjectCallback*> receivers;  ///< List of objects which are to be notified when this object is deleted.

    float prox_distance_desired;                ///< What is the maximum range of proxlist we want
    float prox_distance_current;                ///< What is the current actual range for proxlists (they adjust when the # of objects gets too high)

    bool InitProximityList(float radius,int clientnum);

    bool InitMesh(const char *name,const char *factname,const char *filename,
        const csVector3& pos,const float rotangle,iSector* room);
};

//-----------------------------------------------------------------------------

/*
* Any PS Object with which a player may have interaction (i.e. clickable).
*/
class gemActiveObject : public gemObject
{
public:
    gemActiveObject( const char *name );
    gemActiveObject( const char* name,
                    const char* factname,
                    const char* filename,
                    InstanceID myInstance,
                    iSector* room,
                    const csVector3& pos,
                    float rotangle,
                    int clientnum);

    virtual const char* GetObjectType() { return "Active object"; }

    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool to_superclient) { }

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);

    //@@@ should probably add tests for other actions (usable? examinable?)
    // default "interaction" objects are not pick-uppable and cannot be locked
    virtual bool IsPickable() { return false; }
    virtual bool IsLockable() { return false; }
    virtual bool IsLocked() { return false; }
    virtual bool IsSecutityLocked() { return false; }
    virtual bool IsContainer() { return false; }
};

//-----------------------------------------------------------------------------

class gemItem : public gemActiveObject
{
protected:
    csWeakRef<psItem> itemdata;
    csString itemType;

public:
    gemItem(csWeakRef<psItem> item,
        const char* factname,
        const char* filename,
        InstanceID myInstance,
        iSector* room,
        const csVector3& pos,
        float rotangle,
        int clientnum);

    virtual const char* GetObjectType() { return itemType.GetData(); }
    virtual psItem *GetItem();

    virtual float GetBaseAdvertiseRange();

    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool super_clients);

    virtual void SetPosition(const csVector3& pos,float angle, iSector* sector, InstanceID instance);

    virtual bool IsPickable();
    virtual bool IsLockable();
    virtual bool IsLocked();
    virtual bool IsSecurityLocked();
    virtual bool IsContainer();
    virtual bool IsUsingCD();

    virtual bool GetCanTransform();
    virtual bool GetVisibility();
};

//-----------------------------------------------------------------------------

/**
 * gemContainers are the public containers in the world for crafting, like
 * forges or ovens.  Regular containers in inventory, like sacks, are simulated
 * by psCharInventory.
 */
class gemContainer : public gemItem
{
protected:
    csArray<psItem*> itemlist;
    bool AddToContainer(psItem *item, Client *fromClient,int slot, bool test);

public:
    gemContainer(csWeakRef<psItem> item,
        const char* factname,
        const char* filename,
        InstanceID myInstance,
        iSector* room,
        const csVector3& pos,
        float rotangle,
        int clientnum);

    bool CanAdd(unsigned short amountToAdd, psItem *item, int slot=-1);
    bool AddToContainer(psItem *item,Client *fromClient, int slot=-1) { return AddToContainer(item, fromClient, slot, false); }
    bool RemoveFromContainer(psItem *item,Client *fromClient);

    /** @brief Checks if client is allowed to remove an item from the container.
      *
      * @param client A pointer to the client viewing the container
      * @param item  Item being viewed or taken
      *
      * @return boolean indicating if client is allowed to take the item
      */
    bool CanTake(Client *client, psItem* item);

    /** @brief Remove an item from the container.
      *
      * @param itemStack A pointer to the complete stack of the items we are looking at.
      * @param fromslot  Where in the container the items are removed from.
      * @param fromClient The client that is removing the items.
      * @param stackCount The amount of items we want to remove.
      *
      * @return An item pointer that is the removed items from container.  If itemStack == the item returned
      *         then the entire stack has been removed.  Otherwise it will be a new item instance.
      */
    psItem* RemoveFromContainer(psItem *itemStack, int fromSlot, Client *fromClient, int stackCount);

    psItem *FindItemInSlot(int slot, int stackCount = -1);
    int SlotCount() const { return PSITEM_MAX_CONTAINER_SLOTS; }
    size_t CountItems() { return itemlist.GetSize(); }
    psItem *GetIndexItem(size_t i) { return itemlist[i]; }

    class psContainerIterator;

    class psContainerIterator
    {
        size_t current;
        gemContainer *container;

    public:

        psContainerIterator(gemContainer *containerItem);
        bool HasNext();
        psItem *Next();
        psItem *RemoveCurrent(Client *fromClient);
        void UseContainerItem(gemContainer *containerItem);
    };
};

//-----------------------------------------------------------------------------

class gemActionLocation : public gemActiveObject
{
private:
    psActionLocation *action;
    bool visible;

public:
    gemActionLocation(psActionLocation *action, iSector *isec, int clientnum);

    virtual const char* GetObjectType() { return "ActionLocation"; }
    virtual psActionLocation *GetAction() { return action; }

    virtual float GetBaseAdvertiseRange();
    virtual bool SeesObject(gemObject * object, float range);

    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool super_clients);

    virtual bool GetVisibility() { return visible; };
    virtual void SetVisibility(bool vis) { visible = vis; };
};

//-----------------------------------------------------------------------------

/*
    Struct for damage history
*/
struct DamageHistory
{
    csWeakRef<gemActor> attacker_ref;
    float damage;
    float damageRate;
    int hp;
    unsigned int timestamp;
};

//-----------------------------------------------------------------------------

/*
* Any semi-autonomous object, either a player or an NPC.
*/
class gemActor :  public gemObject, public iDeathNotificationObject
{
protected:
    psCharacter *psChar;
    FactionSet *factions;
    PID pid; ///< Player ID (also known as character ID or PID)
    csRef<PlayerGroup> group;

    csVector3 top, bottom, offset;
    csVector3 last_production_pos;

    csWeakRef<Client> clientRef;

    uint8_t DRcounter;  ///< increments in loop to prevent out of order packet overwrites of better data
    csTicks lastDR;
    csVector3 lastV;

    /** Production Start Pos is used to record the place where people started digging. */
    csVector3 productionStartPos;

    csVector3 lastSentSuperclientPos;
    unsigned int lastSentSuperclientInstance;

    csArray<iDeathCallback*> deathReceivers;  ///< List of objects which are to be notified when this actor dies.

    struct DRstate
    {
        csVector3 pos;
        iSector* sector;
        float yrot;
        float vel_y;
    } valid_location;
    DRstate newvalid_location;
    DRstate last_location;
    DRstate prev_teleport_location;

    // To be used for the /report command.
    // NumReports says how many /report commands that haven't expired yet were applied on our object.
    // Each /report command increments this variable. After some time, it is decremented back.
    int numReports;
    csRef<iFile> logging_chat_file;
    /// Chat history for last CHAT_HISTORY_SIZE number of messages.
    csPDelArray<csString> chatHistory;
    uint32_t reportTargetId;

    bool InitLinMove(const csVector3& pos,float angle, iSector* sector);
    bool InitCharData(Client* c);

    /// What commands the actor can access. Normally only for a player controlled object.
    int securityLevel;
    int masqueradeLevel;

    bool isFalling;           ///< is this object falling down ?
    csVector3 fallStartPos;   ///< the position where the fall began
    iSector* fallStartSector; ///< the sector where the fall began
    csTicks fallStartTime;

    bool invincible;          ///< cannot be attacked
    bool visible;             ///< is visible to clients ?
    bool viewAllObjects;      ///< can view invisible objects?

    csString meshcache;

    csPDelArray<DamageHistory> dmgHistory;
    csArray<csString> onAttackScripts, onDamageScripts;

    int FindCategorySlot(const csString & categoryName);
    csArray<csString> active_spell_categories;

    void ApplyStaminaCalculations(const csVector3& velocity, float times);

    /// Set initial attributes for GMs
    void SetGMDefaults();

    uint8_t movementMode; ///< Actor movement mode from DB table movement_modes
    bool isAllowedToMove; ///< Is a movement lockout in effect?
    bool atRest;          ///< Is this character stationary or moving?

public:
    psLinearMovement* pcmove;

    gemActor(psCharacter *chardata, const char* factname,const char* filename,
        InstanceID myInstance,iSector* room,const csVector3& pos,float rotangle,int clientnum);

    virtual ~gemActor();

    virtual const char* GetObjectType() { return "Actor"; }
    virtual gemActor* GetActorPtr() { return this; }
    virtual psCharacter *GetCharacterData() { return psChar; }
    virtual Client* GetClient() const;

    virtual PID GetPID() { return pid; }

    bool SetupCharData();

    void SetTextureParts(const char *parts);
    void SetEquipment(const char *equip);

    void SetMode(PSCHARACTER_MODE mode) { psChar->SetMode(mode, GetClientID()); }
    PSCHARACTER_MODE GetMode() { return psChar->GetMode(); }
    bool IsAllowedToMove() { return isAllowedToMove; }  ///< Covers sitting, death, and out-of-stamina
    void SetAllowedToMove(bool newvalue);

    /**
     * Call this to ask the actor to set allowed to disconnect in the connected client.
     */
    void SetAllowedToDisconnect(bool allowed);

    void SetSecurityLevel(int level);
    void SetMasqueradeLevel(int level);
    int GetSecurityLevel() { return(securityLevel); }
    int GetMasqueradeLevel() { return(masqueradeLevel); }

    // Last Production Pos is used to require people to move around while /digging
    void SetLastProductionPos(csVector3& pos) { last_production_pos = pos; }
    void GetLastProductionPos(csVector3& pos) { pos = last_production_pos; }

    /** @brief Returns the place where the player last started digging.
      *
      * @return The location where the player last started digging. */
    const csVector3& GetProductionStartPos(void) const { return productionStartPos; }
    /** @brief Sets the place where the player started digging.
      *
      * @param pos The location where the player started digging. */
    void SetProductionStartPos(const csVector3& pos) { productionStartPos = pos; }

    // To be used for the /report command.
    void AddChatReport(gemActor *target);
    void RemoveChatReport();
    bool IsLoggingChat() const { return numReports > 0; }
    uint32_t GetReportTargetId() const { return reportTargetId; }

    /**
     * @brief Adds the chat message to the history and optionally to the log file
     * @param[in] who The name of the character who sent this message
     * @param[in] msg The chat message
     * @return Returns true if the message was written to the log file
     */
    bool LogMessage(const char *who, const psChatMessage &msg);

    void UpdateStats();
    void ProcessStamina();
    void ProcessStamina(const csVector3& velocity, bool force=false);
    virtual float DrainMana(float adjust, bool absolute);

    void SetPosition(const csVector3& pos,float angle, iSector* sector);
    void SetInstance(InstanceID worldInstance);

    void UpdateValidLocation(const csVector3& pos, float vel_y, float yrot, iSector* sector, bool force = false);

    bool SetDRData(psDRMessage& drmsg);
    void MulticastDRUpdate(MsgEntry *resend = NULL);

    using gemObject::RegisterCallback;
    using gemObject::UnregisterCallback;
    void RegisterCallback(iDeathCallback * receiver) { deathReceivers.Push(receiver); }
    void UnregisterCallback(iDeathCallback * receiver) { deathReceivers.Delete(receiver); }
    void HandleDeath();

    float GetRelativeFaction(gemActor *speaker);
    FactionSet *GetFactions() { return factions; }

    csPtr<PlayerGroup> GetGroup();
    void SetGroup(PlayerGroup *group);
    bool InGroup() const;
    bool IsGroupedWith(gemActor *other) const;
    int GetGroupID();
    void RemoveFromGroup();

    bool IsMyPet(gemActor *other) const;

    const char *GetGuildName();
    psGuildInfo *GetGuild() { return psChar->GetGuild(); }
    psGuildLevel *GetGuildLevel() { return psChar->GetGuildLevel(); }

    void DoDamage(gemActor *attacker, float damage, float damageRate = 0.0f, csTicks duration=0);
    void AddAttackerHistory(gemActor * attacker, float damage, float damageRate = 0.0f, csTicks duration = 0 );
    void RemoveAttackerHistory(gemActor * attacker);
    bool CanBeAttackedBy(gemActor *attacker, gemActor ** lastAttacker) const;
    void Kill(gemActor *attacker) { DoDamage(attacker, psChar->GetHP() ); }
    void Resurrect();

    virtual bool UpdateDR();
    virtual void GetLastSuperclientPos(csVector3& pos, InstanceID& instance) const;
    virtual void SetLastSuperclientPos(const csVector3& pos, InstanceID instance);

    virtual void BroadcastTargetStatDR(ClientConnectionSet *clients);
    virtual void SendTargetStatDR(Client *client);
    virtual void SendGroupStats();

    void SetAction(const char *anim,csTicks& timeDelay);
    void ActionCommand(bool actionMy, bool actionNarrate, const char *actText,int destClientID,csTicks& timeDelay);

    virtual void Broadcast(int clientnum, bool control);
    /**
     * /param control  Set to true when sent to the controling client.
     */
    virtual void Send( int clientnum, bool control, bool to_superclient );
    virtual void SendGroupMessage(MsgEntry *me);

    /// Used by chat manager to identify an npc you are near if you talk to one
    gemObject *FindNearbyActorName(const char *name);

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);

    /** Called when the object began falling - 'fallBeginning' tells where the fall started
        displaceY is the displacement that needs to be added to due to passing through
        warping portals
        portalSector is the final sector of the player after passing through the warping
        portals.
    */
    void FallBegan(const csVector3& pos, iSector* sector);
    /** Called when the object has fallen down - sets its falling status to false */
    float FallEnded(const csVector3& pos, iSector* sector);
    /** Checks if the object is falling */
    bool IsFalling() { return isFalling; }
    csTicks GetFallStartTime() { return fallStartTime; }

    bool AtRest() const { return atRest; }

    virtual bool GetVisibility() { return visible; }
    virtual void SetVisibility(bool visible);
    virtual bool SeesObject(gemObject * object, float range);

    virtual bool GetInvincibility() { return invincible; }
    virtual void SetInvincibility(bool invincible);

    /// Flag to determine of this player can see all objects
    bool GetViewAllObjects() { return viewAllObjects; }
    void SetViewAllObjects(bool v);

    void StopMoving(bool worldVel = false);

    /// Moves player to his spawn position
    bool MoveToSpawnPos();
    bool GetSpawnPos(csVector3& pos, float& yrot, iSector*& sector);

    /// Restores actor to his last valid position
    bool MoveToValidPos(bool force = false);
    void GetValidPos(csVector3& pos, float& yrot, iSector*& sector);

    /// Get the last reported location this actor was at
    void GetLastLocation(csVector3& pos, float& vel_y, float& yrot, iSector*& sector);
    /// Moves player to his last reported location
    void MoveToLastPos();

    /// Record the location of this actor before he was teleported
    void SetPrevTeleportLocation(const csVector3& pos, float yrot, iSector* sector);
    /// Get the location of the player before he was teleported
    void GetPrevTeleportLocation(csVector3& pos, float& yrot, iSector*& sector);

    const DamageHistory* GetDamageHistory(int pos) const { return dmgHistory.Get(pos); }
    DamageHistory* GetDamageHistory(int pos) { return dmgHistory.Get(pos); }
    size_t GetDamageHistoryCount() const { return dmgHistory.GetSize(); }
    void ClearDamageHistory() { dmgHistory.Empty(); }

    int  AttachAttackScript(const csString & scriptName);
    void DetachAttackScript(int scriptID);
    int  AttachDamageScript(const csString & scriptName);
    void DetachDamageScript(int scriptID);
    void InvokeAttackScripts(gemActor *target, psItem *item);
    void InvokeDamageScripts(gemActor *attacker, psItem *item);

    bool AddActiveMagicCategory(const csString & category);
    bool RemoveActiveMagicCategory(const csString & category);
    bool IsMagicCategoryActive(const csString & category);
    csArray<csString> GetActiveMagicCategories() { return active_spell_categories; }

    /** These flags are for GM/debug abilities */
    bool nevertired;        ///< infinite stamina
    bool infinitemana;      ///< infinite mana
    bool instantcast;       ///< cast spells instantly
    bool safefall;          ///< no fall damage
    bool questtester;       ///< no quest lockouts
    bool givekillexp;       ///< give exp if killed

    bool SetMesh(const char* meshname);
    bool ResetMesh() { return SetMesh(meshcache); }

    bool GetFiniteInventory() { return GetCharacterData()->Inventory().GetDoRestrictions(); }
    void SetFiniteInventory(bool v) { GetCharacterData()->Inventory().SetDoRestrictions(v); }
};

//-----------------------------------------------------------------------------
class NpcDialogMenu;

class gemNPC : public gemActor
{
protected:
    psNPCDialog *npcdialog;
    AccountID superClientID;
    csWeakRef<gemObject>  target;
    csWeakRef<gemObject>  owner;

    csTicks nextVeryShortRangeAvail; ///< When can npc respond to very short range prox trigger again
    csTicks nextShortRangeAvail;     ///< When can npc respond to short range prox trigger again
    csTicks nextLongRangeAvail;      ///< When can npc respond to long range prox trigger again

    /// Array of client id's allowed to loot this char
    csArray<int> lootable_clients;

    struct DialogCounter
    {
        csString said;
        csString trigger;
        int      count;
        csTicks  when;
        static int Compare( DialogCounter * const & first,  DialogCounter * const & second)
        {
            if (first->count != second->count)
                return first->count - second->count;
            return first->when - second->when;
//            if (first.count != second.count)
//                return first.count - second.count;
//            return first.when - second.when;
        }
    };

    csPDelArray<DialogCounter> badText;

    NpcDialogMenu *initial_triggers;

public:
    gemNPC(psCharacter *chardata, const char* factname,const char* filename,
           InstanceID myInstance,iSector* room,const csVector3& pos,float rotangle,int clientnum);

    virtual ~gemNPC();

    virtual const char* GetObjectType()    { return "NPC";     }
    virtual gemNPC* GetNPCPtr()            { return this;      }
    virtual psNPCDialog *GetNPCDialogPtr() { return npcdialog; }
    virtual Client* GetClient() const      { return NULL;      }

    virtual AccountID GetSuperclientID()        { return superClientID; }
    virtual void SetSuperclientID(AccountID id) { superClientID = id; }

    void SetupDialog(PID npcID,bool force=false);
    void ReactToPlayerApproach(psNPCCommandsMessage::PerceptionType type,gemActor *player);

    virtual void AddLootableClient(int cnum);
    virtual void RemoveLootableClient(int cnum);
    bool IsLootableClient(int cnum);
    const csArray<int>& GetLootableClients() const {return lootable_clients; }
    Client *GetRandomLootClient(int range);

    /// Used to allow a NPC to communicate to its environment
    void Say(const char *strsay,Client *who,bool saypublic,csTicks& timeDelay);

    void AddBadText(const char *playerSaid,const char *trigger);
    void GetBadText(size_t first,size_t last, csStringArray& saidArray, csStringArray& trigArray);

    virtual void Send( int clientnum, bool control, bool to_superclient );
    virtual void Broadcast(int clientnum, bool control );

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);
    void ShowPopupMenu(Client *client);

    virtual void SetTarget(gemObject* target)
    {
        this->target = target;
    };
    virtual gemObject* GetTarget()
    {
        return this->target;
    };

    virtual void SetOwner(gemObject* owner)
    {
        if ( owner )
        {
            this->owner = owner;
            this->GetCharacterData()->SetOwnerID(owner->GetCharacterData()->GetPID());
        }
        else
            this->owner = NULL;
    };
    virtual gemObject* GetOwner()
    {
        return this->owner;
    };

    virtual void SetPosition(const csVector3& pos, float angle, iSector* sector);
};

//-----------------------------------------------------------------------------

class gemPet : public gemNPC
{
public:

    gemPet(psCharacter *chardata, const char* factname,const char* filename,InstanceID instance,iSector* room,
        const csVector3& pos,float rotangle,int clientnum,uint32 id) : gemNPC(chardata,factname,filename,instance,room,pos,rotangle,clientnum)
    {
        this->persistanceLevel = "Temporary";
    };

    virtual const char* GetObjectType() { return "PET"; }
    virtual gemPet* GetPetPtr()  { return this; }


    void SetPersistanceLevel( const char *level )   { this->persistanceLevel = level; };
    const char* SetPersistanceLevel( void )         { return persistanceLevel.GetData(); };
    bool IsFamiliar( void )                         { return this->persistanceLevel.CompareNoCase( "Permanent" ); };

private:
    csString persistanceLevel;
};

//-----------------------------------------------------------------------------

/**
 * This class automatically implements timed events which depend
 * on the existence and validity of a gemObject of any kind.  It
 * will make sure this event is cancelled and skipped when the
 * gemObject is deleted before the event is fired.
 */
class psGEMEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    csWeakRef<gemObject> dependency;

    psGEMEvent(csTicks ticks,int offsetticks,gemObject *depends, const char* newType)
        : psGameEvent(ticks,offsetticks,newType)
    {
        dependency = NULL;

        // Register for disconnect events
        if ( depends )
        {
            dependency = depends;
            depends->RegisterCallback(this);
        }
    }

    virtual ~psGEMEvent()
    {
        // If DeleteObjectCallback() has not been called normal operation
        // this object have to unregister to prevent the
        // object from calling DeleteObjectCallback() later when destroyed.
        if (dependency.IsValid())
        {
            dependency->UnregisterCallback(this);
            dependency = NULL;
        }
    }

    virtual void DeleteObjectCallback(iDeleteNotificationObject * object)
    {
        SetValid(false); // Prevent the Trigger from beeing called.

        if (dependency.IsValid())
        {
            dependency->UnregisterCallback(this);
            dependency = NULL;
        }
    }
};

//-----------------------------------------------------------------------------

class psResurrectEvent : public psGameEvent // psGEMEvent
{
protected:
    csWeakRef<gemObject> who;

public:
    psResurrectEvent(csTicks ticks,int offsetticks,gemActor *actor)
        : psGameEvent(ticks,offsetticks,"psResurrectEvent")
    {
        who = actor;
    }

    void Trigger()
    {
        if (who.IsValid())
        {
            gemActor *actor = dynamic_cast<gemActor*> ((gemObject *) who);
            actor->Resurrect();
        }
    }
};


#endif
