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

#include <iutil/vfs.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/weakreferenced.h>

//#include "engine/celbase.h"
#include "bulkobjects/pscharacter.h"
//#include "playergroup.h"
#include "util/gameevent.h"
#include "util/consoleout.h"
#include "msgmanager.h"
#include "deathcallback.h"
//#include "groupmanager.h"

//#include <physicallayer/entity.h>

struct iCelPlLayer;
struct iCelEntityList;
struct iCelEntity;
class ProximityList;
struct iPcLinearMovement;
struct iPcNPCDialog;
class psServerCharManager;
class EntityManager;
class gemObject;
class PlayerGroup;
class psDatabase;
class psItem;
class csMatrix3;
struct iPcCharacterData;
class NPCManager;
class psGlyphList;
class FactionSet;
class ProgressionManager;
class psNPCDialog;
class psAllEntityPosMessage;
class psActionLocation;
class MathScript;
class gemItem;

#include <net/npcmessages.h>  // required for psNPCCommandsMessage::PerceptionType

#define BUFF_INDICATOR          "+"
#define DEBUFF_INDICATOR        "-"

#define UNSTICK_TIME 15000

class gemActor;
class gemNPC;
class gemItem;

/**
* This class holds the refs to the core factories, etc in CEL.
*/
class GEMSupervisor : public MessageManager, public Singleton<GEMSupervisor>
{
protected:
    csHash<gemObject *> entities_by_cel_id;
    csHash<gemObject *> entities_by_ps_id;
    int                 count_players;

    PS_ID               nextEID;
    csList<PS_ID>       freeListEID;
    unsigned int        freeListEIDLength;
    
    void FreeEID(PS_ID eid);
    PS_ID GetEID();

public:
    iObjectRegistry*        object_reg;
    csRef<iCelPlLayer>      pl;
    psDatabase             *database;
    NPCManager             *npcmanager;

public:

    GEMSupervisor(iObjectRegistry *objreg,
        iCelPlLayer *player,
        psDatabase *db);

    virtual ~GEMSupervisor();

    csHash<gemObject *>& GetAllGEMS() { return entities_by_cel_id; }

    // Search functions
    PS_ID      FindItemID(psItem *item);
    gemObject *FindObject(PS_ID cel_id);
    gemObject *FindObject(const csString& name);
    gemObject *GetObjectFromEntityList(iCelEntityList *list,size_t i);

    gemActor  *FindPlayerEntity(int player_id);
    gemNPC    *FindNPCEntity(int npc_id);
    gemItem   *FindItemEntity(int item_id);

    csPtr<iCelEntity> CreateEntity(gemObject *obj,uint32 gemID);
    void RemoveEntity(gemObject *which,uint32 gemID);
    
    void RemoveClientFromLootables(int cnum);
    
    csPtr<iCelEntity> CreateProxActorList(uint32_t clientnum,iCelEntity *all_actors);

    void UpdateAllDR();
    void UpdateAllStats();

    void GetAllEntityPos(psAllEntityPosMessage& msg);
    int  CountManagedNPCs(int superclientID);
    void FillNPCList(MsgEntry *msg,int superclientID);
    void StopAllNPCs(int superclientID);

    /** Gets a list of all the 'live' entities that this player has ownership of.
      * This can be things like items in containers or work items.
      *
      * @param playerID The character owner ID we are looking for.
      * @param list The populated list of items that are active in game.
      */
    void GetPlayerObjects(unsigned int playerID, csArray<gemObject*> &list);

    /** Teleport a player to a location.
     *
     *  @param object The player to move
     *  @param x,y,z  Location to move to
     *  @param rot    The rotation to use.
     *  @param sector The sector name to move to.
     */
    void Teleport( gemObject* object, float x, float y, float z, float rot, const char* sectorname );

    void HandleMessage(MsgEntry *me,Client *client);
};


class gemActor;
class gemNPC;
class ClientConnectionSet;
class PublishVector;
struct iPcMesh;
struct iMeshWrapper;

/**
* A gemObject is any solid, graphical object visible in PS with normal physics
* and normal collision detection.
*/
class gemObject : public iDeleteNotificationObject, public CS::Utility::WeakReferenced
{
protected:
    bool valid;                             // Is object fully loaded
//    csRef<gemObjectSafe> self_reference;    // Placeholder for ref 1 of saferef
    csRef<iCelEntity> entity;               // link to CEL entity for CEL's purposes
    csRef<iPcMesh> pcmesh;                  // link to CEL mesh class
    ProximityList *proxlist;                // Proximity List for this object
    csString name;                          // Name of this object, used mostly for debugging
    static GEMSupervisor *cel;              // Static ptr back to main collection of all objects
    int worldInstance;                      // Only objects which match instances can see each other
    csVector3 pos;                          // Position in 3d space
    float yRot;                             // Left-Right rotation, in radians
    iSector *sector;                        // Ptr to the CS sector inhabited
    bool is_alive;                          // Flag indicating whether object is alive or not
    csString factname;                      // Name of CS Mesh Factory used to create this object
    csString filename;                      // VFS Filename of mesh
    uint32 gemID;                           // Unique identifier for object

    csArray<iDeleteObjectCallback*> receivers;  // List of objects which are to be notified when this object is deleted.

    float prox_distance_desired;        // What is the maximum range of proxlist we want
    float prox_distance_current;        // What is the current actual range for proxlists (they adjust when the # of objects gets too high)

    bool InitProximityList(float radius,int clientnum);

    bool InitMesh(const char *name,const char *factname,const char *filename,
        const csVector3& pos,const float rotangle,iSector* room, const char *action);

    float Matrix2YRot(const csMatrix3& mat);
    float GetAngle(float x, float y);

public:
    gemObject(const char* name, const char* factname,const char* filename,int myinstance,iSector* room,
        const csVector3& pos,float rotangle,int clientnum,uint32 id);

    /// This ctor is only for use in making keys for the BinaryTree
    gemObject(const char *name);

    virtual ~gemObject();

    uint32 GetGemID() { return gemID; }

    /// Called when a client disconnects
    virtual void Disconnect();

    virtual bool IsValid(void) { return (entity!=NULL); }

    /// Returns whether the object is alive.
    bool IsAlive() const { return is_alive; }
    void SetAlive(bool flag);

    uint32 GetClientID();

    virtual const char* GetObjectType() { return "Object"; }
    iCelEntity *GetEntity() { return entity; }

    virtual psItem *GetItem() { return NULL; }
    virtual gemActor* GetActorPtr() { return NULL; }
    virtual gemNPC* GetNPCPtr()  { return NULL; }
    virtual psCharacter *GetCharacterData() { return NULL; }
    virtual Client* GetClient() { return NULL; }

    const char *GetName();
    void SetName(const char* n);

    void SetInstance(int newInstance) { worldInstance = newInstance; }
    int  GetInstance()                { return worldInstance; }

    void RegisterCallback(iDeleteObjectCallback * receiver) { receivers.Push(receiver); }
    void UnregisterCallback(iDeleteObjectCallback * receiver) { receivers.Delete(receiver); }

    // Mesh related functions
    iMeshWrapper *GetMeshWrapper();
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

    virtual bool IsUpdateReq (csVector3 const &pos,csVector3 const &oldPos)
        { return (pos-oldPos).SquaredNorm() >= DEF_UPDATE_DIST*DEF_UPDATE_DIST; }

    /** This value indicates the range that this entity would become visible
     *  to other entities if no other modifiers were taken into consideration. */
    virtual float GetBaseAdvertiseRange() { return DEF_PROX_DIST; };

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);
    /** Dump debug in formation. Used in the com_print function
     */
    virtual void Dump();
    
    // Networking functions
    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool to_superclient) {}
    virtual void SendGroupMessage(MsgEntry *me) { };

    // Overridden functions in child classes
    virtual PSCHARACTER_MODE GetMode() { return PSCHARACTER_MODE_UNKNOWN; }
    virtual void SetMode(PSCHARACTER_MODE mode) { }
    virtual int GetPlayerID() { return 0; }
    virtual int GetGuildID() { return 0; }
    virtual psGuildInfo* GetGuild() { return 0; }
    virtual bool UpdateDR() { return false; }
    virtual void BroadcastTargetStatDR(ClientConnectionSet *clients) { }
    virtual void SendTargetStatDR(Client *client) { }
    virtual psNPCDialog *GetNPCDialogPtr() { return 0; }
    virtual void GetLastSuperclientPos(csVector3& pos) { }
    virtual void SetLastSuperclientPos(csVector3& pos) { }
    virtual void AddLootableClient(int cnum) { }
    virtual void RemoveLootableClient(int cnum) { }
    virtual bool IsLootableClient(int cnum) { return false; }
    virtual Client *GetRandomLootClient(int range) { return NULL; }
    virtual int  GetSuperclientID() { return 0; }
    virtual void SetSuperclientID(int id) { }

    virtual bool GetVisibility() { return true; }
    virtual bool SeesObject(gemObject * object, float range) { return false; }
};

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
                    int myInstance,
                    iSector* room,
                    const csVector3& pos,
                    float rotangle,
                    int clientnum,uint32 id);

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

class gemItem : public gemActiveObject
{
protected:
    csWeakRef<psItem> itemdata;
    csString itemType;

public:
    gemItem(csWeakRef<psItem> item,
        const char* factname,
        const char* filename,
        int myInstance,
        iSector* room,
        const csVector3& pos,
        float rotangle,
        int clientnum,uint32 id);

    virtual const char* GetObjectType() { return itemType.GetData(); }
    virtual psItem *GetItem();

    virtual float GetBaseAdvertiseRange();

    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool super_clients);

    virtual void SetPosition(const csVector3& pos,float angle, iSector* sector, int instance);

    virtual bool IsPickable();
    virtual bool IsLockable();
    virtual bool IsLocked();
    virtual bool IsSecurityLocked();
    virtual bool IsContainer();

    virtual bool GetCanTransform();
    virtual bool GetVisibility();
};

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
        int myInstance,
        iSector* room,
        const csVector3& pos,
        float rotangle,
        int clientnum,uint32 id);

    bool CanAdd(unsigned short amountToAdd, psItem *item, int slot=-1);
    bool AddToContainer(psItem *item,Client *fromClient, int slot=-1) { return AddToContainer(item, fromClient, slot, false); }
    bool RemoveFromContainer(psItem *item,Client *fromClient);
    psItem *FindItemInSlot(int slot);
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

class gemActionLocation : public gemActiveObject
{
private:
    psActionLocation *action;
    bool visible;

public:
    gemActionLocation( GEMSupervisor *cel, psActionLocation *action, iSector *isec, int clientnum, uint32 id );

    virtual const char* GetObjectType() { return "ActionLocation"; }
    virtual psActionLocation *GetAction() { return action; }
    
    virtual float GetBaseAdvertiseRange();
    virtual bool SeesObject(gemObject * object, float range);

    virtual void Broadcast(int clientnum, bool control);
    virtual void Send( int clientnum, bool control, bool super_clients);

    virtual bool GetVisibility() { return visible; };
    virtual void SetVisibility(bool vis) { visible = vis; };
};

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

/*
* Any semi-autonomous object, either a player or an NPC.
*/
class gemActor :  public gemObject, public iDeathNotificationObject
{
protected:
    psCharacter *psChar;
    FactionSet *factions;
    int playerID;
    csRef<PlayerGroup> group;

    csVector3 top, bottom, offset;
    csVector3 last_production_pos;

    csWeakRef<Client> clientRef;

    uint8_t DRcounter;  /// increments in loop to prevent out of order packet overwrites of better data
    csTicks lastDR;

    /** Production Start Pos is used to record the place where people started digging. */
    csVector3 productionStartPos;

    csVector3 last_sent_superclient_pos;

    csArray<iDeathCallback*> deathReceivers;  // List of objects which are to be notified when this actor dies.

    struct DRstate
    {
        csVector3 pos;
        iSector* sector;
        float yrot;
        float vel_y;
    } valid_location;
    DRstate newvalid_location;
    DRstate last_location;

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

    bool isFalling;         ///< is this object falling down ?
    csVector3 fallStartPos; ///< the position where the fall began
    iSector* fallStartSector; ///< the sector where the fall began
    csTicks fallStartTime;

    bool invincible;        ///< cannot be attacked
    bool visible;           ///< is visible to clients ?
    bool viewAllObjects;    ///< can view invisible objects?

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
    csRef<iPcLinearMovement> pcmove;

    gemActor(psCharacter *chardata, const char* factname,const char* filename,
        int myInstance,iSector* room,const csVector3& pos,float rotangle,int clientnum,uint32 id);

    virtual ~gemActor();

    virtual const char* GetObjectType() { return "Actor"; }
    virtual gemActor* GetActorPtr() { return this; }
    virtual psCharacter *GetCharacterData() { return psChar; }
    virtual Client* GetClient();
    
    virtual int GetPlayerID() { return playerID; }

    bool SetupCharData();

    void SetTextureParts(const char *parts);
    void SetEquipment(const char *equip);
    
    void SetMode(PSCHARACTER_MODE mode) { psChar->SetMode(mode, GetClientID()); }
    PSCHARACTER_MODE GetMode() { return psChar->GetMode(); }
    bool IsAllowedToMove() { return isAllowedToMove; }  ///< Covers sitting, death, and out-of-stamina
    void SetAllowedToMove(bool newvalue);

    void SetSecurityLevel(int level);
    void SetMasqueradeLevel(int level);
    int GetSecurityLevel() { return(securityLevel); }
    int GetMasqueradeLevel() { return(masqueradeLevel); }

    // Last Production Pos is used to require people to move around while /digging
    void SetLastProductionPos(csVector3& pos) { last_production_pos = pos; }
    void GetLastProductionPos(csVector3& pos) { pos = last_production_pos; }

    /** Returns the place where the player last started digging.
      * @return The location where the player last started digging. */
    const csVector3& GetProductionStartPos(void) const { return productionStartPos; }
    /** Sets the place where the player started digging.
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
    void ProcessStamina(const csVector3& velocity, bool force=false);
    virtual float DrainMana(float adjust, bool absolute);

    void SetPosition(const csVector3& pos,float angle, iSector* sector);
    void SetInstance(int worldInstance);

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

    const char *GetGuildName();
    psGuildInfo *GetGuild() { return psChar->GetGuild(); }
    psGuildLevel *GetGuildLevel() { return psChar->GetGuildLevel(); }

    void DoDamage(gemActor *attacker, float damage, float damageRate = 0.0f, csTicks duration=0);
    void AddAttackerHistory(gemActor * attacker, float damage, float damageRate = 0.0f, csTicks duration = 0 );
    void RemoveAttackerHistory(gemActor * attacker);
    void Kill(gemActor *attacker) { DoDamage(attacker, psChar->GetHP() ); }
    void Resurrect();

    virtual bool UpdateDR();
    virtual void GetLastSuperclientPos(csVector3& pos) { pos = last_sent_superclient_pos; }
    virtual void SetLastSuperclientPos(csVector3& pos) { last_sent_superclient_pos = pos; }

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
    void MoveToLastPos(); 

    DamageHistory* GetDamageHistory(int pos) { return dmgHistory.Get(pos); }
    size_t GetDamageHistoryCount() { return dmgHistory.GetSize(); }
    void ClearDamageHistory() { dmgHistory.Empty(); }

    int  AttachAttackScript(const csString & scriptName);
    void DetachAttackScript(int scriptID);
    int  AttachDamageScript(const csString & scriptName);
    void DetachDamageScript(int scriptID);
    void InvokeAttackScripts(gemActor *target);
    void InvokeDamageScripts(gemActor *attacker);

    bool AddActiveMagicCategory(const csString & category);
    bool RemoveActiveMagicCategory(const csString & category);
    bool IsMagicCategoryActive(const csString & category);
    csArray<csString> GetActiveMagicCategories() { return active_spell_categories; }

    /** These flags are for GM/debug abilities */
    bool nevertired;        // infinite stamina
    bool safefall;          // no fall damage
    bool questtester;       // no quest lockouts

    bool SetMesh(const char* meshname);
    bool ResetMesh() { return SetMesh(meshcache); }
    
    bool GetFiniteInventory() { return GetCharacterData()->Inventory().GetDoRestrictions(); }
    void SetFiniteInventory(bool v) { GetCharacterData()->Inventory().SetDoRestrictions(v); }
};

class gemNPC : public gemActor
{
protected:
    psNPCDialog *npcdialog;
    int superClientID;
    csWeakRef<gemObject>  target;
    csWeakRef<gemObject>  owner;

    csTicks nextVeryShortRangeAvail; /// When can npc respond to very short range prox trigger again
    csTicks nextShortRangeAvail;     /// When can npc respond to short range prox trigger again
    csTicks nextLongRangeAvail;      /// When can npc respond to long range prox trigger again

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

public:
    gemNPC(psCharacter *chardata, const char* factname,const char* filename,
           int myInstance,iSector* room,const csVector3& pos,float rotangle,int clientnum,uint32 id);

    virtual ~gemNPC();

    virtual const char* GetObjectType()    { return "NPC";     }
    virtual gemNPC* GetNPCPtr()            { return this;      }
    virtual psNPCDialog *GetNPCDialogPtr() { return npcdialog; }
    virtual Client* GetClient()            { return NULL;      }

    virtual int GetSuperclientID()         { return superClientID; }
    virtual void SetSuperclientID(int id)  { superClientID=id; }

    void SetupDialog(int NPCID);
    void ReactToPlayerApproach(psNPCCommandsMessage::PerceptionType type,gemActor *player);

    virtual void AddLootableClient(int cnum);
    virtual void RemoveLootableClient(int cnum);
    bool IsLootableClient(int cnum);
    Client *GetRandomLootClient(int range);
    bool AdjustMoneyLootClients(const psMoney &m);

    /// Used to allow a NPC to communicate to its environment
    /// void NPCTalk(const csString & text);

    void Say(const char *strsay,Client *who,bool saypublic,csTicks& timeDelay);
    void AddBadText(const char *playerSaid,const char *trigger);
    void GetBadText(size_t first,size_t last, csStringArray& saidArray, csStringArray& trigArray);

    virtual void Send( int clientnum, bool control, bool to_superclient );
    virtual void Broadcast(int clientnum, bool control );

    virtual void SendBehaviorMessage(const csString & str, gemObject *obj);
    virtual csString GetDefaultBehavior(const csString & dfltBehaviors);

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
            this->GetCharacterData()->SetOwnerID( owner->GetCharacterData()->GetCharacterID() );
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

class gemPet : public gemNPC
{
public:

    gemPet(psCharacter *chardata, const char* factname,const char* filename,int instance,iSector* room,
        const csVector3& pos,float rotangle,int clientnum,uint32 id) : gemNPC(chardata,factname,filename,instance,room,pos,rotangle,clientnum,id) 
    {
        this->persistanceLevel = "Temporary";
    };

    virtual const char* GetObjectType() { return "PET"; }

    void SetPersistanceLevel( const char *level )   { this->persistanceLevel = level; };
    const char* SetPersistanceLevel( void )         { return persistanceLevel.GetData(); };
    bool IsFamiliar( void )                         { return this->persistanceLevel.CompareNoCase( "Permanent" ); };

private:
    csString persistanceLevel;
};

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
