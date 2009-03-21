/*
 * npcclient.h
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
#ifndef __NPCCLIENT_H__
#define __NPCCLIENT_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/ref.h>
#include <csutil/list.h>

struct iObjectRegistry;
struct iDocumentNode;
struct iConfigManager;
struct iCollideSystem;

//=============================================================================
// Library Includes
//=============================================================================
#include "net/pstypes.h"

#include "util/prb.h"
#include "util/psconst.h"
#include "util/serverconsole.h"
#include "util/pspath.h"

class  psDatabase;
class  MsgHandler;
class  psNetConnection;
class  EventManager;
class  NetworkManager;
class  NPCType;
class  NPC;
class  gemNPCActor;
class  gemNPCObject;
class  gemNPCItem;
struct iVFS;
class  Perception;
class  psWorld;
class  Location;
class  LocationType;
class  Waypoint;
class  psPFMaps;
class  psTribe;
class  psPath;
class  psPathNetwork;

struct RaceInfo_t
{
    csString name;
    float    walkSpeed;
    float    runSpeed;

    const char * GetName() { return name.GetDataSafe(); }
};

/**
 * The main NPC Client class holding references to important superclient objects
 */
class psNPCClient : public iCommandCatcher
{
public:

    struct DeferredNPC
    {
        PID id;
        csString name;
    };

    
    psNPCClient();
    /**
     * Cleans up all allocated memory and removes all the players from the world.
     */
    ~psNPCClient();

    /**
     * Initialize the superclient.
     * Starts a thread for the Network, Message Handler. Also 
     * binds the status socket for the network.
     */
    bool Initialize(iObjectRegistry* object_reg,const char *host, const char *user, const char *pass, int port);

    /**
     * Just make iObjectRegistry available to other classes.
     */
    iObjectRegistry *GetObjectReg()
    { return objreg; }

    /**
     * Load and fork off a new thread for the Server Console. Then start
     * EventManager's main loop, processing all network and server events.
     */
    void MainLoop ();

    virtual void CatchCommand(const char *cmd);

    /**
     * Used to load the log settings
     */
    void LoadLogSettings();

    /**
     * Used to save the log settings
     */
    void SaveLogSettings();

    /**
     * To be called when the the load process is completed.
     */
    void LoadCompleted();
    
    /**
     * If a world is ready and loaded this returns true.
     */
    bool IsReady();
    
    /**
    * This does 1 AI calc per Tribe then returns.
    */
    void Tick();
   
    /**
     * Load a map into a region.  This is a copy of code
     * from entitymanager->CreateRoom().
     */
    bool LoadMap(const char *mapfile);
   
    /**
     * This function handles the searching for the specified region name
     * so that other functions can refer to the region directly.
     */
    LocationType *FindRegion(const char *regname);

    /**
     * This function handles the searching for the specified location type
     * so that other functions can refer to the location type directly.
     */
    LocationType *FindLocation(const char *locname);
    
    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the <locate> script command.
     */
    Location *FindLocation(const char *loctype, const char *name);
    
    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the <locate> script command.
     */
    Location *FindNearestLocation(const char *loctype, csVector3& pos, iSector* sector, float range = -1, float *found_range = NULL);

    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the <locate> script command.
     */
    Location *FindRandomLocation(const char *loctype, csVector3& pos, iSector* sector, float range = -1, float *found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindNearestWaypoint(csVector3& v, iSector* sector, float range = -1, float * found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindRandomWaypoint(csVector3& v, iSector* sector, float range = -1, float * found_range = NULL);
    
    /**
     * This iterates over all waypoints and finds the nearest one with the given group.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindNearestWaypoint(const char *group, csVector3& v, iSector* sector, float range = -1, float * found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one with the given group.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindRandomWaypoint(const char *group, csVector3& v, iSector* sector, float range = -1, float * found_range = NULL);
    
    /**
     * This iterates over all waypoints and finds the one with the given id.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindWaypoint(int id);

    /**
     * This iterates over all waypoints and finds the one with the given name.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint *FindWaypoint(const char * name);
    
    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Waypoint*> FindWaypointRoute(Waypoint * start, Waypoint * end);

    /**
     * This function handles the searching for the specified NPCType name
     * so that other NPCTypes can subclass an existing one.
     */
    NPCType *FindNPCType(const char *npctype_name);

    /**
     * Add a race info to the race info list
     */
    void AddRaceInfo(csString &name, float walkSpeed, float runSpeed);

    /**
     * Get pointer to a race info to the given race name.
     */
    RaceInfo_t* GetRaceInfo(const char* name);

    /**
     * Dump all race infos
     */
    bool DumpRace(const char * pattern);
        
    /**
     * Find the walk velocity for a given race
     */
    float GetWalkVelocity(csString &race);
    
    /**
     * Find the run velocity for a given race
     */
    float GetRunVelocity(csString &race);

    /**
     * Returns the network handler so script ops can invoke the network.
     */
    NetworkManager *GetNetworkMgr()
    { return network; }

    /**
     * Returns the network handler so script ops can invoke the network.
     */
    psNetConnection *GetNetConnection()
    { return connection; }

    /**
     * Sends a perception to a specified NPC, or broadcasts to all npcs, or to all within a range of a position.
     *
     * @param npc The npc to percept or NULL it all NPCs is to be percepted. If NPC is null max_range
                  can be set to null to broadast to all or both
     *            base_pos and base_sector has to be set to broadcast only to items within the given range.
     */
    void TriggerEvent(NPC *npc, Perception *pcpt, float max_range=-1.0, csVector3 *base_pos=NULL, iSector *base_sector=NULL);

    EventManager *GetEventMgr()
    { return eventmanager; }

    /**
     * Disconnect from server nicely, before quitting.
     */
    void Disconnect();

    /**
     * SetEntityPos finds the given ID entity, and updates
     * its position in mesh and linmove.
     */
    void SetEntityPos(EID id, csVector3& pos, iSector* sector, InstanceID instance);

    /**
     * Find the NPC* attached to the entity with the specified character ID
     */
    NPC *FindNPCByPID(PID character_id);
    
    /**
     * Find the NPC* attached to the entity with the specified EID
     */
    NPC *FindNPC(EID entid);

    /**
     * Find a given path.
     */
    psPath *FindPath(const char *name);

    /**
     * Find a given path.
     */
    psPath *FindPath(const Waypoint * wp1, const Waypoint * wp2, psPath::Direction & direction);

    /**
     * List all NPCs matching pattern to console.
     */
    void ListAllNPCs(const char *pattern);

    /**
     * Find one npc and print its current state info.
     */
    bool DumpNPC(const char *pattern);
    
    /**
     * List all known entities on superclient.
     */
    void ListAllEntities(const char *pattern, bool onlyCharacters = false);

    /**
     * List all tribes matching pattern to console.
     */
    void ListTribes(const char *pattern);
    
    /**
     * List all waypoints matching pattern to console.
     */
    void ListWaypoints(const char *pattern);

    /**
     * List all paths matching pattern to console.
     */
    void ListPaths(const char *pattern);
    
    /**
     * List all locations matching pattern to console.
     */
    void ListLocations(const char *pattern);
    
    /**
     * Special handling for death notifications from the server
     * is required, to stop behavior scripting, delete hate lists,
     * etc.
     */
    void HandleDeath(NPC *who);

    void Add( gemNPCObject* object );
    void Remove ( gemNPCObject* object );
    void RemoveAll();
    
    gemNPCObject *FindCharacterID(PID pid);
    gemNPCObject *FindEntityID(EID eid);

    /**
     * Loop through every tribe and check if this npc is a member.
     */
    void CheckAttachTribes( NPC* npc);

    void AttachNPC( gemNPCActor* actor, uint8_t DRcounter ); 

    iCollideSystem *GetCollDetSys() { return cdsys; }

    NPC *ReadSingleNPC(PID char_id);

    /**
     * Update with time from server in order to start timed events.
     */
    void UpdateTime(int minute, int hour, int day, int month, int year);
    
    psPFMaps * GetMaps() { return PFMaps; }

    psWorld * GetWorld() { return world; }

    iEngine * GetEngine() { return engine; }
    
    iVFS *  GetVFS() { return vfs; }

    int GetGameTODMinute() { return gameMinute;}
    int GetGameTODHour() { return gameHour; }
    int GetGameTODDay() { return gameDay;}
    int GetGameTODMonth() { return gameMonth;} 
    int GetGameTODYear(){ return gameYear;} 

    /** Attach a server gemObject to a Crystal Space object.
     * In most cases this will be a mesh wrapper.
     *
     * @param object The Crystal Space object we want to attach our gemObject to.
     * @param gobject The PlaneShift object that we want to attach.
     */ 
    void AttachObject( iObject* object, gemNPCObject* gobject);

    /** Unattach a gemObject from a Crystal Space object.
      * In most cases the Crystal Space object is a meshwrapper.
      *
      * @param object The Crystal Space object we want to unattach our object from.
      * @param gobject The gem object we want to unattach.
      */
    void UnattachObject( iObject* object, gemNPCObject* gobject); 

    /** See if there is a gemObject attached to a given Crystal Space object.
      * 
      * @param object The Cyrstal Space object we want to see if there is an object attached to.
      *
      * @return A gemNPCObject if it exists that is attached to the Crystal Space object.
      */
    gemNPCObject* FindAttachedObject (iObject* object);
    
    
    /** Create a list of all nearby gem objects.
      * @param sector The sector to check in.
      * @param pos The starting position
      * @param radius The distance around the starting point to check.
      * @param doInvisible If true check invisible meshes otherwise ignore them.     
      *
      * @return A csArray<> of all the objects in the given radius.
      */
    csArray<gemNPCObject*> FindNearbyEntities (iSector* sector, const csVector3& pos, float radius, bool doInvisible = false);

    csRef<iDocumentNode> GetRootNode(const char *xmlfile);
    
    bool LoadNPCTypes(iDocumentNode* root);
    
protected:

    bool ReadNPCsFromDatabase();


protected:
    bool LoadLocations();
    bool LoadPathNetwork();
    
    /** Load Tribes from db */
    bool LoadTribes();


protected:
    /**
     * Find all items that are close to NPC's and percept the
     * NPC.
     */
    void PerceptProximityItems();
    
    /**
     * Find all locations that are close to NPC's and percept the
     * NPC.
     */
    void PerceptProximityLocations();
    
    psNetConnection           *connection;
    iObjectRegistry*           objreg;
    csRef<iEngine>             engine;
    csRef<iConfigManager >     configmanager;
    csRef<MsgHandler>          msghandler;
    ServerConsole             *serverconsole;
    EventManager              *eventmanager;
    NetworkManager            *network;
    psDatabase                *database;
    csRef<iVFS>                vfs;

    csHash<NPCType*, const char*> npctypes;
    BinaryRBTree<LocationType> loctypes;
    psPathNetwork             *pathNetwork;
    csArray<NPC*>              npcs;
    csArray<DeferredNPC>       npcsDeferred;
    csArray<psTribe*>          tribes;
    csHash<gemNPCObject*, EID> all_gem_objects_by_eid;
    csHash<gemNPCObject*, PID> all_gem_objects_by_pid;
    csArray<gemNPCObject*>     all_gem_objects;
    csArray<gemNPCItem*>       all_gem_items;
    csHash<RaceInfo_t,csString> raceInfos;

    csRef<iCollideSystem>      cdsys;

    psWorld                   *world;
    
    psPFMaps                  *PFMaps;

    /// Counter used to start events at every nth client tick
    unsigned int               tick_counter;

    int                        current_long_range_perception_index;

    // Game Time
    int     gameMinute;
    int     gameHour;
    int     gameDay;
    int     gameMonth;
    int     gameYear;
    csTicks gameTimeUpdated;
    
};


#endif

