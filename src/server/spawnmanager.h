/*
 * spawnmanager.h by Keith Fulton <keith@paqrat.com>
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
 */

#ifndef __SPAWNMANAGER_H__
#define __SPAWNMANAGER_H__

class EntityManager;
class gemObject;
class psCharacter;
class psDatabase;
class psScheduledItem;
class psSectorInfo;

#include <csgeom/vector3.h>
#include <csutil/hash.h>
#include "util/location.h"
#include "util/gameevent.h"
#include "msgmanager.h"


/**
 * This class is used to store respawn ranges for NPCs. They are intended
 * to be gathered into a SpawnRule.
 */
class SpawnRange
{
protected:
    csRandomGen* randomgen;

    /// Unique ID used for ident
    int   id;

    char  type;  /// A = Area (rect) or L = Line Segment

    /// Corresponding spawn rule ID
    int   npcspawnruleid;

    /// Spawn sector name
    csString spawnsector;

private:
    /// Spawn range rectangular volume with corners (x1,y1,z1) and (x2,y2,z2)
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;

    /// Pre-computed area to avoid overhead
    float area;

public:
    /// Ctor clears all to zero
    SpawnRange();

    /// Setup variables and compute area
    void Initialize(int idval,
                    int spawnruleid,
                    const char *type_code,
                    float rx1, float ry1, float rz1,
                    float rx2, float ry2, float rz2,
                    const char *sectorname);

    void SetID(int idval) { id = idval; };
    int GetID() { return id; }

    /// Get range's XZ area
    float GetArea() { return area; };

    /// Get spawn sector name
    const csString& GetSector() { return spawnsector; }

    /// Randomly pick a position within the range
    const csVector3 PickPos();
};

class LootEntrySet;
 
/**
 * This class is used to store respawn rules for NPCs. They are loaded
 * from the database at startup and used only in RAM thereafter.
 */
class SpawnRule
{
protected:
    
    csRandomGen* randomgen;

    /// Unique Id used for ident and tree sorting
    int   id;

    /// Minimum respawn delay in ticks (msec)
    int   minspawntime;

    /// Maximum respawn delay in ticks
    int   maxspawntime;

    /// Odds of substitute spawn triggering instead of respawn of same entity
    float substitutespawnodds;

    /// Player id to respawn if substitution spawn odds are met
    PID   substituteplayer;

    /// If ranges are populated, these are ignored
    float    fixedspawnx;
    float    fixedspawny;
    float    fixedspawnz;
    float    fixedspawnrot;
    csString fixedspawnsector;
    InstanceID fixedinstance;

    /// Spawn ranges for the current rule
    csHash<SpawnRange*> ranges;
    
    /// Rules for generating loot when this npc is killed.
    LootEntrySet *loot;

    int dead_remain_time;

public:

    /// Ctor clears all to zero
    SpawnRule();
    ~SpawnRule();

    /// Setup variables
    void Initialize(int idval,
                    int minspawn,
                    int maxspawn,
                    float substodds,
                    int substplayer,
                    float x,float y,float z,float angle,
                    const char *sector,
                    LootEntrySet *loot_id,
                    int dead_time,
                    InstanceID instance);

    int  GetID() { return id; };
    void SetID(int idval) { id = idval; };

    /// Get random value between min and max spawn time, in ticks/msecs.
    int GetRespawnDelay();

    /// Determine if substitute player should be spawned.  Returns either original or substitute.
    PID CheckSubstitution(PID originalplayer);

    /// Pick a spot for the entity to respawn
    void DetermineSpawnLoc(psCharacter *ch, csVector3& pos, float& angle, csString& sectorname, InstanceID& instance);

    /// Add a spawn range to current rule
    void AddRange(SpawnRange *range);

    /// Get the Loot Rule set to generate loot
    LootEntrySet *GetLootRules() { return loot; }

    int GetDeadRemainTime() { return dead_remain_time; }
};

class psItemStats;
class psCharacter;
class LootRandomizer;
/**
 * This class holds one loot possibility for a killed npc.
 * The npc_spawn_rule has a ptr to an array of these.
 */
struct LootEntry
{
    psItemStats *item;
    float probability;
    int   min_money;
    int   max_money;
    bool  randomize;
};

/**
 * This class stores an array of LootEntry and calculates
 * required loot on a newly dead mob.
 */
class LootEntrySet
{
protected:
    int id;
    csArray<LootEntry*> entries;
    float total_prob;
    LootRandomizer *lootRandomizer;

    void CreateSingleLoot(psCharacter *chr);
    void CreateMultipleLoot(psCharacter *chr, size_t numModifiers = 0);

public:
    LootEntrySet(int idx) { id=idx; total_prob=0;}
    ~LootEntrySet();

    /// This adds another item to the entries array
    void AddLootEntry( LootEntry *entry );

    /** This calculates the loot for the character, given the 
     *  current set of loot entries, and adds them to the 
     *  character as lootable inventory.
     */
    void CreateLoot( psCharacter *chr, size_t numModifiers = 0 );

    /**
     * Set the randomizer to use for random loot
     */
    void SetRandomizer( LootRandomizer *rnd ) { lootRandomizer = rnd; }
};

/**
 * This class holds one loot modifier
 * The lootRandomizer contions arrays of these
 */
struct LootModifier
{
    csString modifier_type;
    csString name;
    csString effect;
    csString equip_script;
    float    probability;
    csString stat_req_modifier;
    float    cost_modifier;
    csString mesh;
    csString not_usable_with;
};

class MathScript;
/**
 * This class stores an array of LootModifiers and randomizes
 * loot stats.
 */
class LootRandomizer
{
protected:
    csArray<LootModifier*> prefixes;
    csArray<LootModifier*> suffixes;
    csArray<LootModifier*> adjectives;

    // precalculated max values for probability. min is always 0
    float prefix_max;
    float adjective_max;
    float suffix_max;

public:
    LootRandomizer();
    ~LootRandomizer();

    /// This adds another item to the entries array
    void AddLootModifier( LootModifier *entry );

    /// This randomizes the current loot item stats and returns the new item stats
    psItemStats* RandomizeItem( psItemStats* itemstats,
                                float cost,
                                bool lootTesting = false,
                                size_t numModifiers = 0 );

    float CalcModifierCostCap(psCharacter *chr);

protected:
    MathScript* modifierCostCalc;

private:
    void AddModifier( LootModifier *oper1, LootModifier *oper2 );
    void ApplyModifier(psItemStats *loot, LootModifier *mod);
};

class PendingLootPrompt;

/**
 *  This class is periodically called by the engine to ensure that
 *  monsters (and other NPCs) are respawned appropriately.
 */
class SpawnManager : public MessageManager
{
protected:
    psDatabase             *database;
    csHash<SpawnRule*> rules;
    csHash<LootEntrySet*>   looting;
    LootRandomizer         *lootRandomizer;

    void HandleLootItem(MsgEntry *me,Client *client);
    void HandleDeathEvent(MsgEntry *me,Client *notused);

public:

    SpawnManager(psDatabase *db);
    virtual ~SpawnManager();

    /** Returns the loot randomizer.
     *
     * @return Returns a reference to the current loot randomizer.
     */
    LootRandomizer* GetLootRandomizer() { return lootRandomizer; }

    /**
     * Load all rules into memory for future use.
     */
    void PreloadDatabase();

    /**
     * Load all loot categories into the manager for use in spawn rules.
     */
    void PreloadLootRules();

#if 0
    /**
     * Load NPC Waypoint paths as named linear spawn ranges.
     */
    LoadWaypointsAsSpawnRanges(iDocumentNode *topNode);
#endif

    /**
     * Load all loot mosifiers into the manager for use in spawn rules.
     */
    void PreloadLootModifiers();

    /**
     * Load all ranges for a rule.
     */
    void LoadSpawnRanges(SpawnRule *rule);

    /** Load hunt location
     *  @param sectorinfo The sector to load in. NULL means all sectors.
     */
    void LoadHuntLocations(psSectorInfo *sectorinfo = 0);

    /**
     * Called at server startup to create all creatures currently marked as
     * "living" in the database.  This will restore the server to its last
     * known NPC population if it crashes.
     *
     *  @param sectorinfo The sector we want to repopulate.
     *                    If NULL then respawn all sectors.
     */
    void RepopulateLive(psSectorInfo *sectorinfo = 0);

    /**
     * This function receives inbound net messages from the client.
     */
    void HandleMessage(MsgEntry *me,Client *client) { };

    /**
     * This function is called periodically by the server and will respawn
     * NPCs as appropriate.
     */
    void Respawn(InstanceID instance, csVector3& where, float rot, csString& sector, PID playerID);
    
    /// Adds all items to the world.
    /** Called at the server startup to add all the items to the game. 
     * @param sectorinfo The sector to respawn the items into. If NULL it will
     *                   respawn all items in all sectors.
     */
    void RepopulateItems(psSectorInfo *sectorinfo = 0);

    /**
     * Sets the NPC as dead, plays the death animation and triggers the
     * loot generator.  Also queues the removal of the corpse.
     */
    void KillNPC(gemObject *npc, gemActor * killer);

    /**
     * Kills the specified NPC, updates the database that he is "dead"
     * and queues him for respawn according to the spawn rules.
     */
    void RemoveNPC(gemObject *obj);
};


/**
 * When an NPC or mob is killed in the spawn manager, its respawn event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 */
class psRespawnGameEvent : public psGameEvent
{
protected:
    SpawnManager *spawnmanager;
    int ticks;
    csVector3 where;
    float     rot;
    csString  sector;
    PID       playerID;
    InstanceID instance;

public:
    psRespawnGameEvent(SpawnManager *mgr,
               int delayticks,
               csVector3& pos,
               float angle,
               csString& sector,
               PID newplayer,
               InstanceID newinstance);

    virtual void Trigger();  // Abstract event processing function
};

/**
 * When an NPC or mob is killed in the spawn manager, its respawn event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 */
class psDespawnGameEvent : public psGameEvent
{
protected:
    SpawnManager *spawnmanager;
    int ticks;
    EID entity;

public:
    psDespawnGameEvent(SpawnManager *mgr,
                       int delayticks,
                       gemObject *obj);

    virtual void Trigger();  // Abstract event processing function
};

class psItemSpawnEvent : public psGameEvent
{
public:
    psItemSpawnEvent (psScheduledItem* item);
    virtual ~psItemSpawnEvent();
    
    void Trigger();  // Abstract event processing function    

protected:
    psScheduledItem* schedule;
};


#endif
