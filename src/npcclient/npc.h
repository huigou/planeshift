/*
* npc.h
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
*/

/* This file holds definitions for ALL global variables in the planeshift
* server, normally you should move global variables into the psServer class
*/
#ifndef __NPC_H__
#define __NPC_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>

struct iMovable;

#include "util/psutil.h"
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
//#include "npcbehave.h"
#include "tribe.h"
#include "gem.h"

class  Behavior;
class  EventManager;
class  HateList;
class  LocationType;
class  NPCType;
class  NetworkManager;
class  Tribe;
class  Waypoint;
class  gemNPCActor;
class  gemNPCObject;
class  iResultRow;
class  psLinearMovement;
class  psNPCClient;
class  psNPCTick;
struct iCollideSystem;
struct HateListEntry;
struct RaceInfo_t;

#define NPC_BRAIN_TICK 200

/**
* This object represents the entities which have attacked or 
* hurt the NPC and prioritizes them.
*/
class HateList
{
protected:
    csHash<HateListEntry*, EID> hatelist;

public:
    HateList(psNPCClient* npcclient, iEngine* engine, psWorld* world) { this->npcclient = npcclient; this->engine = engine; this->world = world; }

    void AddHate(EID entity_id, float delta);
    
    /** Find the most hated entity within range of given position
     *
     *  Check the hate list and retrive most hated entity within range.
     *
     *  @param  pos               The position
     *  @param  sector            The sector of the position
     *  @param  range             The range to search for hated entities.
     *  @param  includeInvisible  Include invisible entities in the search.
     *  @param  includeInvincible Include invincible entities in the search.
     *  @param  hate              If diffrent from NULL, set upon return to the hate of the hated.
     *  @return The hated entity
     */
    gemNPCActor *GetMostHated(csVector3& pos, iSector *sector, float range, LocationType * region, bool includeInvisible, bool includeInvincible, float* hate);
    bool Remove(EID entity_id);
    void DumpHateList(const csVector3& myPos, iSector *mySector);
    void Clear();
    float GetHate(EID ent);
    
private:
    psNPCClient* npcclient;
    iEngine* engine;
    psWorld* world;
};


/**
* This object represents each NPC managed by this superclient.
*/
class NPC : private ScopedTimerCB
{
public:
    /** Structure to hold located positions */
    typedef struct
    {
        csVector3               pos;        ///< The position of the located object
        iSector*                sector;     ///< The sector for the located object
        float                   angle;      ///< The angle of the located object
        Waypoint*               wp;         ///< The nearest waypoint to the located object
        float                   radius;     ///< The radius of the located object
        csWeakRef<gemNPCObject> target;     ///< The located target
    } Locate;
    typedef csHash<Locate*,csString> LocateHash;

    enum
    {
        LOCATION_NONE = 0,
        LOCATION_POS = 1,
        LOCATION_SECTOR = 2,
        LOCATION_ANGLE = 4,
        LOCATION_WP = 8,
        LOCATION_RADIUS = 16,
        LOCATION_TARGET = 32,
        LOCATION_ALL = -1
    };

protected:

    typedef csHash<csString,csString> BufferHash;

    NPCType           *brain;
    csString           type;
    PID                pid;
    csString           name;
    csTicks            last_update;
    gemNPCActor       *npcActor;
    iMovable          *movable;
    uint8_t            DRcounter;

    Locate*            activeLocate;   ///< The current "Active" locate
    LocateHash         storedLocates;  ///< List of stored locate locations
    
    float              ang_vel,vel;
    float              walkVelocity;
    float              runVelocity;
    csString           region_name;          ///< Region name as loaded from db
    LocationType*      region;               ///< Cached pointer to the region
    bool               insideRegion;         ///< State variable for inside outside region checks.
    Perception*        last_perception;
    int                debugging;       /// The current debugging level for this npc
    bool               alive;
    EID                owner_id;
    EID                target_id;

    Tribe*             tribe;
    csString           tribeMemberType;      ///< What type/class is this NPC in the tribe. 
    bool               insideTribeHome;      ///< State variable for inside outside tribe home checks.
        
    csVector3          spawnPosition;        ///< The stored position that this NPC where spawned
    iSector*           spawnSector;          ///< The stored sector that this NPC where spawned

    RaceInfo_t        *raceInfo;

    csArray< csWeakRef<gemNPCActor> > controlledActors; ///< Actors that are dragged/pushed around by this NPC.

    // Initial position checks
    csVector3          checkedPos;
    iSector*           checkedSector;
    bool               checked;
    bool               checkedResult;
    bool               disabled;

    int                fallCounter; // Incremented if the NPC fall off the map
    
    void Advance(csTicks when);

public:
    HateList           hatelist;
    
    NPC(psNPCClient* npcclient, NetworkManager* networkmanager, psWorld* world, iEngine* engine, iCollideSystem* cdsys);
    virtual ~NPC();

    
    void Tick();
    
    PID                   GetPID() { return pid; }
    /**
     * Return the entity ID if an entity exist else 0.
     */
    EID                   GetEID();
    iMovable             *GetMovable()   { return movable; }
    psLinearMovement     *GetLinMove();
    uint8_t               GetDRCounter() { return ++DRcounter;}
    void                  SetDRCounter(uint8_t counter) { DRcounter = counter;}

    bool Load(iResultRow& row,csHash<NPCType*, const char*>& npctypes, EventManager* eventmanager, PID usePID);
    void Load(const char* name, PID pid, NPCType* type, const char* region_name, int debugging, bool disabled, EventManager* eventmanager);

    bool InsertCopy(PID use_char_id, PID ownerPID);

    void SetActor(gemNPCActor * actor);
    gemNPCActor * GetActor() { return npcActor; }
    const char* GetName() {return name.GetDataSafe();}
    void SetAlive(bool a);
    bool IsAlive() const { return alive; }
    void Disable(bool disable = true);
    bool IsDisabled() { return disabled; }

    Behavior *GetCurrentBehavior();
    NPCType  *GetBrain();

    /**
     * Sets a new brain (npctype)  to this npc.
     * @param type The new type to assign to this npc.
     * @param eventmanager A pointer to the npcclient eventmanager.
     */
    void SetBrain(NPCType *type, EventManager* eventmanager);

    /** Callback for debug scope timers
     */
    void ScopedTimerCallback(const ScopedTimer* timer);

    /** Provide info about the NPC.
     *
     *  This funcion is both used by the "/info" in the client
     *  and the "info <pid> npcclient console command.
     */
    csString Info();
    
    /** Dump all information for one NPC to the console.
     *
     *  The main use of this fuction is the "print <pid>"
     *  npcclient console command.
     */
    void Dump();

    /**
     * Dump all state information for npc.
     */
    void DumpState();
    /**
     * Dump all behaviors for npc.
     */
    void DumpBehaviorList();
    /**
     * Dump all reactions for npc.
     */
    void DumpReactionList();
    /**
     * Dump all hated entities for npc.
     */
    void DumpHateList();

    /**
     * Dump the debug log for npc
     */
    void DumpDebugLog();

    void ClearState();

    /** Send a perception to this NPC
     *
     * This will send the perception to this npc.
     * If the maxRange has been set to something greater than 0.0
     * a range check will be applied. Only if within the range
     * from the base position and sector will this perception
     * be triggered.
     *
     * @param pcpt       Perception to be sent.
     * @param maxRange   If greater then 0.0 then max range apply
     * @param basePos    The base position for range checks.
     * @param baseSector The base sector for range checks.
     * @param sameSector Only trigger if in same sector
     */    
    void TriggerEvent(Perception *pcpt, float maxRange=-1.0,
                      csVector3* basePos=NULL, iSector* baseSector=NULL,
                      bool sameSector=false);

    /** Send a perception to this NPC
     *
     * Uses the above method, but acts as a wrapper.
     * Receives only the name of the perception and uses default values
     * for the rest of the arguments
     */
    void TriggerEvent(const char* pcpt);
    
    void SetLastPerception(Perception *pcpt);
    Perception *GetLastPerception() { return last_perception; }

    /** Find the most hated entity within range of the NPC
     *
     *  Check the hate list and retrive most hated entity within range.
     *
     *  @param  range             The range to search for hated entities.
     *  @param  includeInvisible  Include invisible entities in the search.
     *  @param  includeInvincible Include invincible entities in the search.
     *  @param  hate              If diffrent from NULL, set upon return to the hate of the hated.
     *  @return The hated entity
     */
    gemNPCActor* GetMostHated(float range, bool includeInvisible, bool includeInvincible, float* hate=NULL);

    /** Find the most hated entity within range of a given position
     *
     *  Check the hate list and retrive most hated entity within range.
     *
     *  @param  pos               The position
     *  @param  sector            The sector of the position
     *  @param  range             The range to search for hated entities.
     *  @param  includeInvisible  Include invisible entities in the search.
     *  @param  includeInvincible Include invincible entities in the search.
     *  @param  hate              If diffrent from NULL, set upon return to the hate of the hated.
     *  @return The hated entity
     */
    gemNPCActor* GetMostHated(csVector3& pos, iSector *sector, float range, LocationType * region, bool includeInvisible, bool includeInvincible, float* hate);

    
    float GetEntityHate(gemNPCActor *ent);
    void AddToHateList(gemNPCActor *attacker,float delta);
    void RemoveFromHateList(EID who);

    /** Set the NPCs locate.
     */
    void SetLocate(const csString& destination, const NPC::Locate& locate );

    /** Get the NPCs current active locate.
     */
    void GetActiveLocate(csVector3& pos, iSector*& sector, float& rot);

    /** Return the wp of the current active locate.
     *
     * @param wp will be set with the wp currently in the active locate.
     */
    void GetActiveLocate(Waypoint*& wp);
    
    /** Get the radius of the last locate operatoins
     *
     * @return The radius of the last locate
     */
    float GetActiveLocateRadius() const;

    /** Copy locates
     *
     * Typically used to take backups of "Active" locate.
     */
    bool CopyLocate(csString source, csString destination, unsigned int flags);

    /** Replace $LOCATION[<location>.<attribute>]
     */
    void ReplaceLocations(csString& result);

    /** Switch the debuging state of this NPC.
     */
    bool SwitchDebugging();
    
    /** Set a new debug level for this NPC.
     * @param debug New debug level, 0 is no debugging
     */
    void SetDebugging(int debug);
    
    /** Add a client to receive debug information
     * @param clentnum The client to add.
     */
    void AddDebugClient(uint clientnum);
    
    /** Remove client from list of debug receivers.
     * @param clentnum The client to remove.
     */
    void RemoveDebugClient(uint clientnum);
    
    float GetAngularVelocity();
    float GetVelocity();
    float GetWalkVelocity();
    float GetRunVelocity();

    csString& GetRegionName() { return region_name; }
    LocationType *GetRegion();

    /** Check the inside region state of the npc.
     *
     */
    bool IsInsideRegion() { return insideRegion; }
    
    /** Set the inside region state
     *
     * Keep track of last perception for inbound our, outbound
     */
    void SetInsideRegion(bool inside) { insideRegion = inside; }


    /** Return the nearest actor within the given range.
     *  @param range The search range to look for players
     *  @param destPosition Return the position of the target
     *  @param destSector Return the sector of the target
     *  @param destRange Return the range to the target
     *  @return The actor of the nearest player or NPC or NULL if none within range
     */
    gemNPCActor* GetNearestActor(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    /** Return the nearest NPC within the given range.
     *  @param range The search range to look for players
     *  @param destPosition Return the position of the target
     *  @param destSector Return the sector of the target
     *  @param destRange Return the range to the target
     *  @return The actor of the nearest NPC or NULL if none within range
     */
    gemNPCActor* GetNearestNPC(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    /** Return the nearest player within the given range.
     *  @param range The search range to look for players
     *  @param destPosition Return the position of the target
     *  @param destSector Return the sector of the target
     *  @param destRange Return the range to the target
     *  @return The actor of the nearest player or NULL if none within range
     */
    gemNPCActor* GetNearestPlayer(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    gemNPCActor* GetNearestVisibleFriend(float range);
    
    gemNPCActor* GetNearestDeadActor(float range);

    void Printf(const char *msg,...);
    void Printf(int debug, const char *msg,...);
    void VPrintf(int debug, const char *msg,va_list arg);

    gemNPCObject *GetTarget();
    void SetTarget(gemNPCObject *t);

    gemNPCObject *GetOwner();
    const char* GetOwnerName();
    
    /** Sets the owner of this npc. The server will send us the owner of
     *  the entity connected to it so we can follow it's directions.
     *  @param owner_EID the eid of the entity who owns this npc.
     */
    void SetOwner(EID owner_EID);

    /** Set a new tribe for this npc */
    void SetTribe(Tribe * new_tribe);
    
    /** Get the tribe this npc belongs to.
     *
     * @return Null if not part of a tribe
     */
    Tribe * GetTribe();

    /** Set the type/class for this npc in a tribe.
     */
    void SetTribeMemberType( const char* tribeMemberType );

    /** Return the type/class for this NPC's tribe membership if any.
     */
    const csString& GetTribeMemberType() const;

    /** Check the inside tribe home state of the npc.
     *
     */
    bool IsInsideTribeHome() { return insideTribeHome; }
    
    /** Set the inside tribe home state
     *
     * Keep track of last perception for inbound our, outbound
     */
    void SetInsideTribeHome(bool inside) { insideTribeHome = inside; }

    /** Get the npc race info
     */
    RaceInfo_t * GetRaceInfo();
    
    /** Take control of another entity.
     */
    void TakeControl(gemNPCActor* actor);

    /** Release control of another controlled entity.
     */
    void ReleaseControl(gemNPCActor* actor);

    /**
     */
    void UpdateControlled();

    bool IsDebugging() { return (debugging > 0);};
    bool IsDebugging(int debug) { return (debugging > 0 && debug <= debugging);};

    void CheckPosition();

    /** Store the start position.
     *
     * Store the current position as spawn position so that NPCs can locate
     * spawn position.
     */
    void StoreSpawnPosition();
    
    /** Return the position part of the spawn position */
    const csVector3& GetSpawnPosition() const;

    /** Return the sector part of the spawn position */
    iSector* GetSpawnSector() const;


    /** Increment the fall counter 
    *
    * Fall counter is used for debugging.
    */
    void IncrementFallCounter()  { ++fallCounter; }

    /** Return the fall counter 
    *
    * Fall counter is used for debugging.
    */
    int GetFallCounter()  { return fallCounter; }

    /** Return a named buffer from the NPC.
     *
     * Function used to get data from buffers containing information.
     *
     * @param The buffer name.
     * @return The content of the named buffer.
     */
    csString GetBuffer(const csString& bufferName);
    /** Set/Update the value of a named buffer.
     *
     * @param The buffer name.
     * @param The value to put in the buffer.
     */
    void SetBuffer(const csString& bufferName, const csString& value);
    /** Replace $NBUFFER[x] with values from the NPC buffer.
     *
     * @param result String to replace buffers in.
     */
    void ReplaceBuffers(csString& result);

    Tribe::Memory* GetBufferMemory() { return bufferMemory; }
    void SetBufferMemory(Tribe::Memory* memory);

    /** Set a building spot for this NPC
     */
    void SetBuildingSpot(Tribe::Asset* buildingSpot);
    
    /** Get the stored building spot for this NPC
     */
    Tribe::Asset* GetBuildingSpot();

private:
    psNPCTick*        tick;
    psNPCClient*      npcclient;
    NetworkManager*   networkmanager;
    psWorld*          world;
    iCollideSystem*   cdsys;

    BufferHash        npcBuffer;        ///< Used to store dynamic data
    Tribe::Memory*    bufferMemory;     ///< Used to store location data
    Tribe::Asset*     buildingSpot;     ///< Used to store current building spot.
    
    friend class psNPCTick;

    csArray<csString> debugLog;          ///< Local debug log of last n print statments for this NPC.
    int               nextDebugLogEntry; ///< The next entry to use.
    csArray<uint>     debugClients;      ///< List of clients doing debugging
};

// The event that makes the NPC brain go TICK.
class psNPCTick : public psGameEvent
{
protected:
    NPC *npc;

public:
	psNPCTick(int offsetticks, NPC *npc): psGameEvent(0,offsetticks,"psNPCTick"), npc(npc) {};

    virtual void Trigger()
    {
    	npc->tick = NULL;
    	npc->Tick();
    }

    virtual csString ToString() const { return "psNPCTick"; } 
};

struct HateListEntry
{
    EID   entity_id;
    float hate_amount;
};


#endif

