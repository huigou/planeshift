/*
 * pscharacter.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PSCHARACTER_H__
#define __PSCHARACTER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/psconst.h"
#include "util/scriptvar.h"
#include "util/skillcache.h"
#include "util/slots.h"

#include "net/charmessages.h"

#include "../icachedobject.h"
#include "../playergroup.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "buffable.h"
#include "psskills.h"
#include "psstdint.h"
#include "pscharinventory.h"
#include "psinventorycachesvr.h"
#include "psitemstats.h"
#include "servervitals.h"

class psServerVitals;
class MsgEntry;
class psItemStats;
class psItem;
class psQuest;
class psGuildInfo;

struct Result;
struct Faction;

/** "Normalizes" name of character i.e. makes the first letter uppercase and all the rest downcase */
csString NormalizeCharacterName(const csString & name);


////////////////////////////////////////////////////////////////////////////////

enum PSCHARACTER_GENDERRACE
{
    PSCHARACTER_GENRACE_FXACHA = 0,
    PSCHARACTER_GENRACE_MXACHA = 1,
    PSCHARACTER_GENRACE_FYLIAN = 2,
    PSCHARACTER_GENRACE_MYLIAN = 3,
    PSCHARACTER_GENRACE_FNOLTHRIR = 4,
    PSCHARACTER_GENRACE_MNOLTHRIR = 5,
    PSCHARACTER_GENRACE_FDERMORIAN = 6,
    PSCHARACTER_GENRACE_MDERMORIAN = 7,
    PSCHARACTER_GENRACE_FSTONEBREAKER = 8,
    PSCHARACTER_GENRACE_MSTONEBREAKER = 9,
    PSCHARACTER_GENRACE_FHAMMERWIELDER = 10,
    PSCHARACTER_GENRACE_MHAMMERWIELDER = 11,
    PSCHARACTER_GENRACE_FLEMUR = 12,
    PSCHARACTER_GENRACE_MLEMUR = 13,
    PSCHARACTER_GENRACE_FDIABOLI = 14,
    PSCHARACTER_GENRACE_MDIABOLI = 15,
    PSCHARACTER_GENRACE_FENKIDUKAI = 16,
    PSCHARACTER_GENRACE_MENKIDUKAI = 17,
    PSCHARACTER_GENRACE_FKLYROS = 18,
    PSCHARACTER_GENRACE_MKLYROS = 19,
    PSCHARACTER_GENRACE_FYNNWN = 20,
    PSCHARACTER_GENRACE_MYNNWN = 21,
    PSCHARACTER_GENRACE_NKRAN = 22, ///< Oddball race - Nuetral/No gender
    PSCHARACTER_GENRACE_COUNT
};

enum PSCHARACTER_TYPE
{
    PSCHARACTER_TYPE_PLAYER = 0,
    PSCHARACTER_TYPE_NPC    = 1,
    PSCHARACTER_TYPE_PET    = 2,
    PSCHARACTER_TYPE_COUNT  = 3,
    PSCHARACTER_TYPE_UNKNOWN = ~0
} ;

#define PSCHARACTER_BULK_COUNT INVENTORY_BULK_COUNT
#define PSCHARACTER_BANK_BULK_COUNT 16

/// Base class for several other classes which hold character attributes of different sorts
class CharacterAttribute
{
protected:
    psCharacter *self;
public:
    CharacterAttribute(psCharacter *character) : self(character) { }
};


/**
 * Advantages/Disadvantages
 *
 *  A character either has or does not have an advantage/disadvantage.  Each advantage/disadvantage is numbered.
 *  A series of bitfields are created to store the actual values.
 *
*/

enum PSCHARACTER_ADVANTAGE
{
    PSCHARACTER_ADVANTAGE_6TH_SENSE = 0,
    PSCHARACTER_ADVANTAGE_HEART_OF_VENGEANCE,
    PSCHARACTER_DISADVANTAGE_BALDING,
    PSCHARACTER_DISADVANTAGE_DROPPED_ON_HEAD_AS_CHILD,
    // More to come
    PSCHARACTER_ADVANTAGE_COUNT
};

#define PSCHARACTER_ADVANTAGE_32BIT_BITFIELDS (PSCHARACTER_ADVANTAGE_COUNT-1)/32+1

// Remember to update the translation table player_mode_to_str when
// adding new modes.
enum PSCHARACTER_MODE
{
    PSCHARACTER_MODE_UNKNOWN = 0,
    PSCHARACTER_MODE_PEACE,
    PSCHARACTER_MODE_COMBAT,
    PSCHARACTER_MODE_SPELL_CASTING,
    PSCHARACTER_MODE_WORK,
    PSCHARACTER_MODE_DEAD,
    PSCHARACTER_MODE_SIT,
    PSCHARACTER_MODE_OVERWEIGHT,
    PSCHARACTER_MODE_EXHAUSTED,
    PSCHARACTER_MODE_DEFEATED,
    PSCHARACTER_MODE_COUNT
};

enum PSCHARACTER_CUSTOM
{
    PSCHARACTER_CUSTOM_EYES = 0,
    PSCHARACTER_CUSTOM_HAIR,
    PSCHARACTER_CUSTOM_BEARD,
    PSCHARACTER_CUSTOM_COLOUR,
    PSCHARACTER_CUSTOM_SKIN,
    PSCHARACTER_CUSTOM_COUNT
};

#define PSQUEST_DELETE   'D'
#define PSQUEST_ASSIGNED 'A'
#define PSQUEST_COMPLETE 'C'

/**
 * This structure tracks assigned quests.
 */
struct QuestAssignment
{
    /// Character id of player who assigned quest to this player.  This is used to make sure you cannot get two quests from the same guy at the same time.
    PID assigner_id;
    /// This status determines whether the quest was assigned, completed, or is marked for deletion.
    char status;
    /// Dirty flag determines minimal save on exit
    bool dirty;
    /// When a quest is completed, often it cannot immediately be repeated.  This indicate the time when it can be started again.
    unsigned long lockout_end;
    /// To avoid losing a chain of responses in a quest, last responses are stored per assigned quest.
    int last_response;
    /// To avoid losing a chain of responses in a quest, last responses are stored per assigned quest.
    PID last_response_from_npc_pid;

    /// Since "quest" member can be nulled without notice, this accessor function attempts to refresh it if NULL
    csWeakRef<psQuest>& GetQuest();
    void SetQuest(psQuest *q);
protected:

    /// Quest ID saved in case quest gets nulled out from us
    int quest_id;
    /// Weak pointer to the underlying quest relevant here
    csWeakRef<psQuest> quest;
};

/**
 * Structure for assigned GM Events.
 */
struct GMEventsAssignment
{
    /// GM controlling running GM Event.
    int runningEventIDAsGM;
    /// Player Running GM Event.
    int runningEventID;
    /// completed GM events as the GM.
    csArray<int> completedEventIDsAsGM;
    /// completed GM events
    csArray<int> completedEventIDs;
};

/**
 * This enumeration and structure tracks
 * the players trade skill efforts
 */
enum PSCHARACTER_WORKSTATE
{
    PSCHARACTER_WORKSTATE_HALTED = 0,
    PSCHARACTER_WORKSTATE_STARTED,
    PSCHARACTER_WORKSTATE_COMPLETE,
    PSCHARACTER_WORKSTATE_OVERDONE
};


/// Set if this slot should continuously attack while in combat
#define PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK       0x00000001
/// Set if this slot can attack when the client specifically requests (and only when the client specifically requests)
#define PSCHARACTER_EQUIPMENTFLAG_SINGLEATTACK     0x00000002
/// Set if this slot can attack even when empty - requires that a default psItem be set in default_if_empty
#define PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY    0x00000004

// Dirty bits for STATDRDATA
//#define PSCHARACTER_STATDRDATA_DIRTY_HITPOINTS      0x00000001
//#define PSCHARACTER_STATDRDATA_DIRTY_HITPOINTS_MAX  0x00000002
//#define PSCHARACTER_STATDRDATA_DIRTY_HITPOINTS_RATE 0x00000004
//#define PSCHARACTER_STATDRDATA_DIRTY_MANA           0x00000008
//#define PSCHARACTER_STATDRDATA_DIRTY_MANA_MAX       0x00000010
//#define PSCHARACTER_STATDRDATA_DIRTY_MANA_RATE      0x00000020
//#define PSCHARACTER_STATDRDATA_DIRTY_STAMINA        0x00000040
//#define PSCHARACTER_STATDRDATA_DIRTY_STAMINA_MAX    0x00000080
//#define PSCHARACTER_STATDRDATA_DIRTY_STAMINA_RATE   0x00000100


struct psRaceInfo;
class psSectorInfo;

class ExchangeManager;
class MathScript;
class NpcResponse;
class gemActor;
struct psGuildLevel;
class psLootMessage;
class psMerchantInfo;
class psQuestListMessage;
class psSpell;
class psSpellCastGameEvent;
class psTradeTransformations;
class psTradeProcesses;
class psTrainerInfo;
struct psTrait;
class psWorkGameEvent;

//-----------------------------------------------------------------------------

struct Buddy
{
    csString name;
    PID playerID;
};

// Need to recalculate Max HP/Mana/Stamina and inventory limits whenever stats
// change (whether via buffs or base value changes).
class SkillStatBuffable : public ClampedPositiveBuffable<int>
{
public:
    void Initialize(psCharacter *c) { chr = c; }
protected:
    virtual void OnChange();
    psCharacter *chr;
};

// When base stats change, we also need to recalculate skill training costs.
// However, since training ignores buffs, we don't need to for all changes.
class CharStat : public SkillStatBuffable
{
public:
    void SetBase(int x);
};

typedef SkillStatBuffable SkillRank;

class StatSet : public CharacterAttribute
{
public:
    StatSet(psCharacter *self);

    CharStat & Get(PSITEMSTATS_STAT attrib);
    CharStat & operator [] (PSITEMSTATS_STAT which) { return Get(which); }

protected:
    CharStat stats[PSITEMSTATS_STAT_COUNT];
};

/** A structure that holds the knowledge/practice/rank of each player skill.
 */
struct Skill
{
    unsigned short z;        ///< Practice value
    unsigned short y;        ///< Knowledge Level
    SkillRank rank;          ///< Skill rank (buffable)

    unsigned short zCost;    ///< Cost in Z points.
    unsigned short yCost;    ///< cost in y points.
    unsigned short zCostNext;///< Cost in Z points of next level.
    unsigned short yCostNext;///< cost in y points of next level.
    bool dirtyFlag;          ///< Flag if this was changed after load from database

    psSkillInfo *info;       ///< Database information about the skill.

    Skill() { Clear(); }
    void Clear() { z=y=0; zCost=yCost=0; info = NULL; dirtyFlag = false;}

    void CalculateCosts(psCharacter* user);

    /** Checks to see if this skill can be trained any more at the current rank.
     */
    bool CanTrain() { return y < yCost; }

    /** @brief Train a skill by a particular amount.
      *
      * This does range checking on the training level and will cap it at the
      * max allowable level.
      * @param yIncrease The amount to try to increase the skill by.
      */
    void Train( int yIncrease );

    /** @brief Practice this skill.
      *
      * This checks a couple of things.
      * 1) If the player has the required knowledge to allow for training.
      * 2) If the amount of practice causes a rank change it will increase
      *    the rank of the skill and reset the knowledge/practice levels.
      *
      * @param amount The amount of practice on this skill.
      * @param actuallyAdded [CHANGES] If the amount added causes a rank change
      *                       only the amount required is added and this variable
      *                       stores that.
      * @param user The character this was for.
      *
      * @return True if the practice causes a rank change, false if not.
      */
    bool Practice( unsigned int amount, unsigned int& actuallyAdded,psCharacter* user  );
};


/** A list of skills.
  * This maintains a list of all the skills and the player's current levels
  * in them.
  */
class SkillSet : public CharacterAttribute
{
protected:
    Skill skills[PSSKILL_COUNT];

public:
    SkillSet(psCharacter *self) : CharacterAttribute(self)
    {
        for (int i=0; i<PSSKILL_COUNT; i++)
        {
            skills[i].Clear();
            skills[i].rank.Initialize(self);
        }
    }

    /** @brief Sets the common skill info for this skill ( data from the database )
      * @param which  The skill we want to set
      * @param info   The info structure to assign to this skill.
      * @param user   The owner character of this skill.
      * @param recalculatestats   if true, stats of player will be recalculated taking into account the new skill
      */
    void SetSkillInfo( PSSKILL which, psSkillInfo* info, bool recalculatestats = true);

    /** @brief Sets the practice level for the skill.
      *
      * Does no error or range checking on the z value.  Simply assigns.
      *
      * @param which The skill we want to set.
      * @param z_value The practice level of that skill.
      */
    void SetSkillPractice(PSSKILL which,int z_value);

    /** @brief Set a skill knowledge level.
     *
     *  Sets a skill to a particular knowledge level. This does no checking
     *  of allowed values. It just sets it to a particular value.
     *
     *  @param which  Skill name. One of the PSSKILL enum values.
     *  @param y_value    The value to set this skill knowledge at.
     */
    void SetSkillKnowledge(PSSKILL which,int y_value);

    /** @brief Set a skill rank level.
     *
     *  Sets a skill to a particular rank level. This does no checking
     *  of allowed values. It just sets it to a particular value.
     *
     *  @param which  Skill name. One of the PSSKILL enum values.
     *  @param rank    The value to set this skill rank at.
     *  @param recalculatestats   if true, stats of player will be recalculated taking into account the new skill rank
     */
    void SetSkillRank( PSSKILL which, unsigned int rank, bool recalculatestats = true);

    /** Update the costs for all the skills.
     */
    void Calculate();

    /** @brief Figure out if this skill can be trained.
      *
      * Checks the current knowledge of the skill. If it is already maxed out then
      * can train no more.
      *
      * @param skill The skill we want to train.
      * @return  True if the skill still requires Y credits before it is fully trained.
      */
    bool CanTrain( PSSKILL skill );

    /** @brief Trains a skill.
     *
     *  It will only train up to the cost of the next rank. So the yIncrease is
     *  capped by the cost and anything over will be lost.
     *
     *  @param skill The skill we want to train.
     *  @param yIncrease  The amount we want to train this skill by.
     */
    void Train( PSSKILL skill, int yIncrease );


    /** @brief Get the current rank of a skill.
     *
     *  @param which The skill that we want the rank for.
     *  @return The rank of the requested skill.
     */
    SkillRank & GetSkillRank(PSSKILL which);

    /** @brief Get the current knowledge level of a skill.
     *
     *  @param skill the enum of the skill that we want.
     *  @return The Y value of that skill.
     */
    unsigned int GetSkillKnowledge( PSSKILL skill );

    /** @brief Get the current practice level of a skill.
     *
     *  @param skill the enum of the skill that we want.
     *  @return The Z value of that skill.
     */
    unsigned int GetSkillPractice(PSSKILL skill);

    /** @brief Add some practice to a skill.
      *
      * @param skill The skill we want to practice
      * @param val The amount we want to practice the skill by. This value could
      *            be capped if the amount of practice causes a rank up.
      * @param added [CHANGES] The amount the skill changed by ( if any )
      *
      * @return True if practice caused a rank up.
      */
    bool  AddToSkillPractice(PSSKILL skill, unsigned int val, unsigned int& added );

    int AddSkillPractice(PSSKILL skill, unsigned int val);

    /** Gets a players best skill rank **/
    unsigned int GetBestSkillValue( bool withBuff );

    /** @brief Get the slot that is the best skill in the set.
     *
     *  @param withBuff   Apply any skill buffs?
     *  @return The slot the best skill is in.
     */
    unsigned int GetBestSkillSlot( bool withBuff );

    Skill & Get(PSSKILL skill);
};

#define ALWAYS_IMPERVIOUS      1
#define TEMPORARILY_IMPERVIOUS 2


//-----------------------------------------------------------------------------


#define ANY_BULK_SLOT        -1
#define ANY_EMPTY_BULK_SLOT  -2


//-----------------------------------------------------------------------------

struct Stance
{
    unsigned int stance_id;
    csString stance_name;
    float stamina_drain_P;
    float stamina_drain_M;
    float attack_speed_mod;
    float attack_damage_mod;
    float defense_avoid_mod;
    float defense_absorb_mod;
};

//-----------------------------------------------------------------------------

class psCharacter : public iScriptableVar, public iCachedObject
{
protected:
    psCharacterInventory      inventory;                    ///< Character's inventory handler.
    psMoney money;                                          ///< Current cash set on player.
    psMoney bankMoney;                                      ///< Money stored in the players bank account.

    psGuildInfo*              guildinfo;
    csArray<psSpell*>         spellList;
    csArray<QuestAssignment*> assigned_quests;
    StatSet                   attributes, modifiers;
    SkillSet                  skills;
    psSkillCache              skillCache;
    GMEventsAssignment        assigned_events;
    csArray<PID>              explored_areas;
    bool guildNotified; ///< If true the player will get notifications about his guild members logging in and out

    bool LoadSpells(PID use_id);
    bool LoadAdvantages(PID use_id);
    bool LoadSkills(PID use_id);
    bool LoadTraits(PID use_id);
    bool LoadQuestAssignments();
    bool LoadRelationshipInfo(PID pid);
    bool LoadBuddies( Result& myBuddy, Result& buddyOf);
    bool LoadMarriageInfo( Result& result);
    bool LoadFamiliar( Result& pet, Result& owner);
    bool LoadExploration(Result& exploration);
    bool LoadGMEvents();

    float override_max_hp,override_max_mana;  ///< These values are loaded from base_hp_max,base_mana_max in the db and
                                              ///< should prevent normal HP calculations from taking place

    static const char *characterTypeName[];
    unsigned int characterType;

    /// Array of items waiting to be looted.
    csArray<psItemStats *> loot_pending;
    /// Last response of an NPC to this character (not saved)
    int  lastResponse;
    /// Amount of money ready to be looted
    int  loot_money;

public:
    psCharacterInventory& Inventory() { return inventory; }

    psMoney Money() { return money; }
    psMoney& BankMoney() { return bankMoney; }
    void SetMoney(psMoney m);
    /** @brief Add a money object to the current wallet.
      *
      * This is used when money is picked up and will destory the object passed in.
      *
      * @param moneyObject The money object that was just picked up.
      */
    void SetMoney( psItem *& moneyObject );
    void AdjustMoney(psMoney m, bool bank);
    void SaveMoney(bool bank);

    void ResetStats();

//    WorkInformation* workInfo;
    PID pid;
    AccountID accountid;

    csString name;
    csString lastname;
    csString fullname;
    csString oldlastname;

    csString spouseName;
    bool     isMarried;

    csArray<Buddy> buddyList;
    csArray<PID> buddyOfList;
    csSet<PID> acquaintances;

    psRaceInfo *raceinfo;
    PSCHARACTER_MODE player_mode;
    static const char * player_mode_to_str[];
    Stance combat_stance;
    const Stance& getStance(csString name);
    csString faction_standings;
    csString progressionScriptText; ///< flat string loaded from the DB.
    int     impervious_to_attack;
    /// Bitfield for which help events a character has already encountered.
    unsigned int     help_event_flags;

    /// Checks the bit field for a bit flag from the enum in TutorialManager.h
    bool NeedsHelpEvent(int which) { return (help_event_flags & (1 << which))==0; }

    /// Sets a bit field complete for a specified flag from the enum in tutorialmanager.h
    void CompleteHelpEvent(int which) { help_event_flags |= (1 << which); }

    /// Set the active guild for the character.
    void SetGuild(psGuildInfo *g) { guildinfo = g; }
    /// Return the active guild, if any for this character.
    psGuildInfo  *GetGuild() { return guildinfo; }
    /// Return the guild level for this character, if any.
    psGuildLevel *GetGuildLevel();

    ///Returns if the client should receive notifications about guild members logging in
    bool IsGettingGuildNotifications() { return guildNotified; }
    ///Sets if the client should receive notifications about guild members logging in
    void SetGuildNotifications(bool enabled) { guildNotified = enabled; }

    StatSet  & Stats()  { return attributes; }
    SkillSet & Skills() { return skills;     }

    /**
     * Returns a pointer to the skill cache for this character
     */
    psSkillCache *GetSkillCache() { return &skillCache; }

    /// Set the work transformation state for the character.
    void SetWorkState(PSCHARACTER_WORKSTATE state) { work_state = state; }
    /// Return the work transformation state
    PSCHARACTER_WORKSTATE GetWorkState() { return work_state; }
    /// Assign trade work event so it can be accessed
    void SetTradeWork(psWorkGameEvent * event);
    /// Return trade work event so it can be stopped
    psWorkGameEvent *GetTradeWork() { return workEvent; }

    // iCachedObject Functions below
    virtual void ProcessCacheTimeout() {};  ///< required for iCachedObject but not used here
    virtual void *RecoverObject() { return this; }  ///< Turn iCachedObject ptr into psCharacter
    virtual void DeleteSelf() { delete this; }  ///< Delete must come from inside object to handle operator::delete overrides.


    struct st_bank
    {
        psItem *bulk[PSCHARACTER_BANK_BULK_COUNT];
        psMoney money;
    } bank;

    struct st_location
    {
        psSectorInfo *loc_sector;
        csVector3 loc;
        float loc_yrot;
        InstanceID worldInstance;
    } location;

    unsigned int advantage_bitfield[PSCHARACTER_ADVANTAGE_32BIT_BITFIELDS];

    psSpellCastGameEvent *spellCasting; ///< Hold a pointer to the game event
                                        ///< for the spell currently cast.

    psServerVitals* vitals;

    psTrait *traits[PSTRAIT_LOCATION_COUNT];

    // NPC specific data.  Should this go here?
    int npc_spawnruleid;
    int npc_masterid;

    /// Id of Loot category to use if this char has extra loot
    int  loot_category_id;

    csString animal_affinity;
    PID owner_id;
    PID familiar_id;
    Buffable<int> canSummonFamiliar;

public:
    psCharacter();
    virtual ~psCharacter();

    bool Load(iResultRow& row);

    /// Load the bare minimum to know what this character is looks like
    bool QuickLoad(iResultRow& row, bool noInventory);

    void LoadIntroductions();

    void LoadActiveSpells();
    void AddSpell(psSpell * spell);
    bool Store(const char *location,const char *slot,psItem *what);

    void SetPID(PID characterID) { pid = characterID; }
    PID GetPID() const { return pid; }

    PID GetMasterNPCID() const { return npc_masterid ? npc_masterid : pid; }

    void SetAccount(AccountID id) { accountid = id; }
    AccountID GetAccount() const { return accountid; }

    void SetName(const char* newName) { SetFullName(newName,lastname.GetData()); }
    void SetLastName(const char* newLastName) { SetFullName(name.GetData(),newLastName); }
    void SetFullName(const char* newFirstName, const char* newLastName);
    void SetOldLastName( const char* oldLastName ) { this->oldlastname = oldLastName; };

    const char *GetCharName() const     { return name.GetData(); }
    const char *GetCharLastName() const { return lastname.GetData(); }
    const char *GetCharFullName() const { return fullname.GetData(); }
    const char *GetOldLastName() const  { return oldlastname.GetData(); }

    // Introductions
    /// Answers whether this character knows the given character or not.
    bool Knows(PID charid);
    bool Knows(psCharacter *c) { return (c ? Knows(c->GetPID()) : false); }
    /// Introduces this character to the given character; answers false if already introduced.
    bool Introduce(psCharacter *c);
    /// Unintroduces this character to the given character; answers false if not introduced.
    bool Unintroduce(psCharacter *c);

    unsigned int GetCharType() const { return characterType; }
    void SetCharType(unsigned int v) { CS_ASSERT(v < PSCHARACTER_TYPE_COUNT); characterType = v; }
    const char *GetCharTypeName() { return psCharacter::characterTypeName[ characterType ]; }

    void SetLastLoginTime( const char* last_login = NULL, bool save = true);
    csString GetLastLoginTime() const { return lastlogintime; }

    void SetSpouseName(const char* name);
    /** @brief Gets Spouse Name of a character.
     *
     *  @return  SpouseName or "" if not married.
     */
    const char *GetSpouseName() const { return spouseName.GetData(); }
    void SetIsMarried( bool married ) { isMarried = married; }
    bool GetIsMarried() const { return isMarried; }

    void SetRaceInfo(psRaceInfo *rinfo);
    psRaceInfo *GetRaceInfo() { return raceinfo; }

    /** @brief Remove the player with a certain Player ID from this character buddy list
     *
     *  @param buddyID the Player ID which we are going to remove from the character buddy list
     */
    void RemoveBuddy(PID buddyID);

    /** @brief Checks if a playerID is a buddy of this character
     *
     *  @param buddyID the Player ID of which we are checking the presence in the character buddy list
     *  @return  true if the provided PID was found in this character.
     */
    bool IsBuddy(PID buddyID);

    /** @brief Add the player with a certain Player ID to this character buddy list
     *
     *  @param buddyID the Player ID which we are going to add to the character buddy list
     */
    bool AddBuddy(PID buddyID, csString & name);

    void BuddyOf(PID buddyID);
    void NotBuddyOf(PID buddyID);

    /** @brief Add a new explored area.
     *
     *  @param explored The PID of the area npc.
     */
    bool AddExploredArea(PID explored);

    /** @brief Check if an area has already been explored.
     *
     *  @param explored The PID of the area npc.
     */
    bool HasExploredArea(PID explored);

    const char *GetFactionStandings() { return faction_standings; }

    void SetLootCategory(int id) { loot_category_id = id; }
    int  GetLootCategory() const { return loot_category_id; }
    bool RemoveLootItem(int id);

    void AddInventoryToLoot();
    void AddLootItem(psItemStats *item);
    void AddLootMoney(int money) { loot_money += money; }
    size_t GetLootItems(psLootMessage& msg, EID entity, int cnum);

    /// Gets and zeroes the loot money
    int  GetLootMoney();

    /// Clears the pending loot items array and money
    void ClearLoot();

    QuestAssignment *IsQuestAssigned(int id);
    QuestAssignment *AssignQuest(psQuest *quest, PID assigner_id);
    bool CompleteQuest(psQuest *quest);
    void DiscardQuest(QuestAssignment *q, bool force = false);
    bool SetAssignedQuestLastResponse(psQuest *quest, int response, gemObject *npc);
    size_t GetNumAssignedQuests() { return assigned_quests.GetSize(); }
    int GetAssignedQuestLastResponse(size_t i);
    /// The last_response given by an npc to this player.
    int GetLastResponse() { return lastResponse; }
    void SetLastResponse(int response) { lastResponse=response; }

    /**
     * @brief Sync dirty Quest Assignemnt to DB
     *
     * @param force_update Force every QuestAssignment to be dirty
     * @return True if succesfully updated DB.
     */
    bool UpdateQuestAssignments(bool force_update = false);

    size_t  GetAssignedQuests(psQuestListMessage& quests,int cnum);
    csArray<QuestAssignment*>& GetAssignedQuests() { return assigned_quests; }

    bool CheckQuestAssigned(psQuest *quest);
    bool CheckQuestCompleted(psQuest *quest);
    bool CheckQuestAvailable(psQuest *quest, PID assigner_id);
    /**
     * @brief Check if all prerequisites are valid for this response
     * for this character.
     *
     * @param resp The Response to check
     * @return True if all prerequisites are ok for the response
     */
    bool CheckResponsePrerequisite(NpcResponse *resp);
    int NumberOfQuestsCompleted(csString category);

    void AddAdvantage( PSCHARACTER_ADVANTAGE advantage);
    void RemoveAdvantage( PSCHARACTER_ADVANTAGE advantage);
    bool HasAdvantage ( PSCHARACTER_ADVANTAGE advantage);

    void CombatDrain(int);

    unsigned int GetExperiencePoints(); // W
    void SetExperiencePoints(unsigned int W);
    unsigned int AddExperiencePoints(unsigned int W);
    unsigned int CalculateAddExperience(PSSKILL skill, unsigned int awardedPoints, float modifier = 1);
    unsigned int GetProgressionPoints(); // X
    void SetProgressionPoints(unsigned int X,bool save);
    void UseProgressionPoints(unsigned int X);

    void SetSpellCasting(psSpellCastGameEvent * event) { spellCasting = event; }
    bool IsSpellCasting() { return spellCasting != NULL; }
    void InterruptSpellCasting();
    /// Get the maximum realm the caster can cast with given skill
    int GetMaxAllowedRealm( PSSKILL skill );
    /// Checks if this character has enough knowledge to cast spell
    /// of given way and realm
    bool CheckMagicKnowledge( PSSKILL skill, int realm );
    SkillRank & GetSkillRank(PSSKILL skill) { return skills.GetSkillRank(skill); }

    PSCHARACTER_MODE GetMode() { return player_mode; }
    const char* GetModeStr(); ///< Return a string name of the mode
    bool CanSwitchMode(PSCHARACTER_MODE from, PSCHARACTER_MODE to);
    void SetMode(PSCHARACTER_MODE newmode, uint32_t clientnum, uint32_t extraData = 0);

    /**
     * Reset modes for NPCs
     */
    void ResetMode();

    void KilledBy(psCharacter* attacker) { deaths++; if(!attacker) suicides++; }
    void Kills(psCharacter* target) { kills++; }

    unsigned int GetKills() const { return kills; }
    unsigned int GetDeaths() const { return deaths; }
    unsigned int GetSuicides() const { return suicides; }

    /** @brief Drops an item into the world (one meter from this character's position)
      *
      * @param Item to be dropped
      * @param Transient flag (decay?) (default=true)
      */
    void DropItem(psItem *&item, csVector3 pos = 0, bool guarded = true, bool transient = true, bool inplace = false);

    float GetHP();
    float GetMana();
    float GetStamina(bool pys);

    void SetHitPoints(float v);
    void SetMana(float v);
    void SetStamina(float v, bool pys);

    void AdjustHitPoints(float adjust);
    void AdjustMana(float adjust);
    void AdjustStamina(float adjust, bool pys);

    VitalBuffable & GetMaxHP();
    VitalBuffable & GetMaxMana();
    VitalBuffable & GetMaxPStamina();
    VitalBuffable & GetMaxMStamina();

    VitalBuffable & GetHPRate();
    VitalBuffable & GetManaRate();
    VitalBuffable & GetPStaminaRate();
    VitalBuffable & GetMStaminaRate();

    void SetStaminaRegenerationNone(bool physical = true, bool mental = true);
    void SetStaminaRegenerationWalk(bool physical = true, bool mental = true);
    void SetStaminaRegenerationSitting();
    void SetStaminaRegenerationStill(bool physical = true, bool mental = true);
    void SetStaminaRegenerationWork(int skill);

    void CalculateMaxStamina();

    const char* GetHelmGroup() { return helmGroup.GetData(); }

    size_t GetAssignedGMEvents(psGMEventListMessage& gmevents, int clientnum);
    void AssignGMEvent(int id, bool playerIsGM);
    void CompleteGMEvent(bool playerIsGM);
    void RemoveGMEvent(int id, bool playerIsGM=false);

    /** Update a npc's default spawn position with given data.
    */
    void UpdateRespawn(csVector3 pos, float yrot, psSectorInfo *sector, InstanceID instance);


    /**
     * Update this faction for this player with delta value.
     */
    bool UpdateFaction(Faction * faction, int delta);

    /**
     * Check player for given faction.
     */
    bool CheckFaction(Faction * faction, int value);

    /* Check if the character is a banker */
    bool IsBanker() const { return banker; }

private:
    int FindGlyphSlot(const csArray<glyphSlotInfo>& slots, psItemStats * glyphType, int purifyStatus);

    csString helmGroup;                 // Some races share helms so this tells which
                                        // group it's in. If empty assume in racial group.
    /* Whether or not the character is a banker */
    bool banker;
public:
    void RecalculateStats();

    bool IsNPC() { return characterType == PSCHARACTER_TYPE_NPC; };

    /// Used to determine if this NPC is a pet
    bool IsPet() { return characterType == PSCHARACTER_TYPE_PET; };
    PID  GetFamiliarID() { return familiar_id; };
    void SetFamiliarID(PID v);
    bool CanSummonFamiliar() { return GetFamiliarID() != 0 && canSummonFamiliar.Current() > 0; }
    Buffable<int> & GetCanSummonFamiliar() { return canSummonFamiliar; }
    const char *GetAnimalAffinity() { return animal_affinity.GetDataSafe(); };
    void SetAnimialAffinity( const char* v ) { animal_affinity = v; };
    PID  GetOwnerID() { return owner_id; };
    void SetOwnerID(PID v) { owner_id = v; };

    bool UpdateStatDRData(csTicks now);
    bool SendStatDRMessage(uint32_t clientnum, EID eid, int flags, csRef<PlayerGroup> group = NULL);

    /** Returns true if the character is able to attack with the current slot.
     *  This could be true even if the slot is empty (as in fists).
     *  It could also be false due to effects or other properties.
     */
    bool GetSlotAttackable(INVENTORY_SLOT_NUMBER slot);

    bool GetSlotAutoAttackable(INVENTORY_SLOT_NUMBER slot);
    bool GetSlotSingleAttackable(INVENTORY_SLOT_NUMBER slot);
    void ResetSwings(csTicks timeofattack);
    void TagEquipmentObject(INVENTORY_SLOT_NUMBER slot,int eventId);
    int GetSlotEventId(INVENTORY_SLOT_NUMBER slot);

    void SetCombatStance(const Stance& stance);
    const Stance& GetCombatStance() { return combat_stance; }

    /// Retrieves the calculated Attack Value for the given weapon-slot
    //float GetAttackValue(psItem *slotitem);
    //float GetAttackValueForWeaponInSlot(int slot);
    float GetTargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetUntargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetTotalTargetedBlockValue();
    float GetTotalUntargetedBlockValue();
    float GetCounterBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetDodgeValue();

    Multiplier & AttackModifier()  { return attackModifier;  }
    Multiplier & DefenseModifier() { return defenseModifier; }

    /// Practice skills for armor and weapons
    void PracticeArmorSkills(unsigned int practice, INVENTORY_SLOT_NUMBER attackLocation);
    void PracticeWeaponSkills(unsigned int practice);
    void PracticeWeaponSkills(psItem * weapon, unsigned int practice);

    void SetTraitForLocation(PSTRAIT_LOCATION location,psTrait *trait);
    psTrait *GetTraitForLocation(PSTRAIT_LOCATION location);

    void GetLocationInWorld(InstanceID &instance,psSectorInfo *&sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot);
    void SetLocationInWorld(InstanceID instance,psSectorInfo *sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot);
    void SaveLocationInWorld();

    /// Construct an XML format string of the player's texture choices.
    void MakeTextureString( csString& textureString );

    /// Construct an XML format string of the player's equipment.
    void MakeEquipmentString( csString& equipmentString );

    /// Returns a level of character based on his 6 base stats.
    unsigned int GetCharLevel();

    bool IsMerchant() { return (merchantInfo != NULL); }
    psMerchantInfo *GetMerchantInfo() { return merchantInfo; }
    bool IsTrainer() { return (trainerInfo != NULL); }
    psTrainerInfo *GetTrainerInfo() { return trainerInfo; }
    psCharacter *GetTrainer() { return trainer; }
    void SetTrainer(psCharacter *trainer) { this->trainer = trainer; }

    /** @brief Figure out if this skill can be trained.
      *
      * Checks the current knowledge of the skill. If it is already maxed out then
      * can train no more.
      *
      * @param skill The skill we want to train.
      * @return  True if the skill still requires Y credits before it is fully trained.
      */
    bool CanTrain( PSSKILL skill );

    /** @brief Trains a skill.
     *
     *  It will only train up to the cost of the next rank. So the yIncrease is
     *  capped by the cost and anything over will be lost.
     *
     *  @param skill The skill we want to train.
     *  @param yIncrease  The amount we want to train this skill by.
     */
    void Train( PSSKILL skill, int yIncrease );

    /** Directly sets rank of given skill. It completely bypasses the skill logic,
       it is used for testing only. */
    void SetSkillRank( PSSKILL which, unsigned int rank);

    psSpell * GetSpellByName(const csString& spellName);
    psSpell * GetSpellByIdx(int index);
    csArray<psSpell*>& GetSpellList() { return spellList; }

    typedef enum
        { NOT_TRADING, SELLING, BUYING} TradingStatus;

    psCharacter* GetMerchant() { return merchant; }
    TradingStatus GetTradingStatus() { return tradingStatus; }
    void SetTradingStatus(TradingStatus trading, psCharacter *merchant)
        { tradingStatus = trading; this->merchant = merchant; }

    gemActor *GetActor() { return actor; }
    void SetActor(gemActor *actor);

    bool SetTradingStopped(bool stopped);

    bool ReadyToExchange();

    /// Number of seconds online this session in seconds.
    unsigned int GetOnlineTimeThisSession() { return (csGetTicks() - startTimeThisSession)/1000; }

    /// Number of seconds online ever including this session in seconds.
    unsigned int GetTotalOnlineTime() { return timeconnected + GetOnlineTimeThisSession(); }

    /// Total number of seconds online.  Updated at logoff.
    unsigned int timeconnected;
    csTicks startTimeThisSession;

    unsigned int GetTimeConnected() { return timeconnected; }

    /** @brief This is used to get the stored player description.
     *  @return Returns a pointer to the stored player description.
     */
    const char* GetDescription();

    /** @brief This is used to store the player description.
     *  @param newValue: A pointer to the new string to set as player description.
     */
    void SetDescription(const char* newValue);

    /** @brief This is used to get the stored player OOC description.
     *  @return Returns a pointer to the stored player OOC description.
     */
    const char* GetOOCDescription();

    /** @brief This is used to store the player OOC description.
     *  @param newValue: A pointer to the new string to set as player OOC description.
     */
    void SetOOCDescription(const char* newValue);

    /** @brief This is used to get the stored informations from the char creation.
     *  @return Returns a pointer to the stored informations from the char creation.
     */
    const char* GetCreationInfo();

    /** @brief This is used to store the informations from the char creation. Players shouldn't be able to edit this.
     *  @param newValue: A pointer to the new string to set as char creation data.
     */
    void SetCreationInfo(const char* newValue);

    /** @brief Gets dynamic life events generated from the factions of the character.
     *  @param factionDescription: where to store the dynamically generated data.
     *  @return Returns true if there were some dynamic life events founds else false.
     */
    bool GetFactionEventsDescription(csString & factionDescription);

    /** @brief This is used to get the stored informations from the custom life events made by players.
     *  @return Returns a pointer to the stored informations from the custom life events made by players.
     */
    const char* GetLifeDescription();

    /** @brief This is used to store the informations sent by players for their custom life events.
     *  @param newValue: A pointer to the new string to set as custom life events.
     */
    void SetLifeDescription(const char* newValue);

    /// This is used by the math scripting engine to get various values.
    double GetProperty(const char *ptr);
    double CalcFunction(const char * functionName, const double * params);
    const char* ToString() { return fullname.GetData(); }

    /// The exp to be handed out when this actor dies
    int GetKillExperience() { return kill_exp; }
    void SetKillExperience(int newValue) { kill_exp=newValue; }

    void SetImperviousToAttack(int newValue) { impervious_to_attack=newValue; }
    int GetImperviousToAttack() { return impervious_to_attack; }

    void CalculateEquipmentModifiers();
    float GetStatModifier(PSITEMSTATS_STAT attrib);

    csString& GetLastError() { return lastError; }

    csString lastError;

    // State information for merchants
    csRef<psMerchantInfo>  merchantInfo;
    bool tradingStopped;
    TradingStatus tradingStatus;
    psCharacter* merchant; ///< The merchant this charcter trade with
    gemActor * actor;
    //
    csRef<psTrainerInfo>  trainerInfo;
    psCharacter* trainer;

//    psTradeTransformations * transformation;
    PSCHARACTER_WORKSTATE work_state;
    psWorkGameEvent * workEvent;

    csString description;     ///<Player description
    csString oocdescription;  ///<Player OOC description
    csString creationinfo;    ///<Creation manager informations
    csString lifedescription; ///<Custom life events informations

    int kill_exp; ///< Kill Exp

    Multiplier attackModifier;  ///< Attack  value is multiplied by this
    Multiplier defenseModifier; ///< Defense value is multiplied by this

    MathScript* powerScript, *maxRealmScript; ///< The PowerLevel math script

    MathScript* staminaCalc;  ///< The stamina calc script
    MathScript* expSkillCalc; ///< The exp calc script to assign experience on skill ranking


protected:
    csString lastlogintime;///< String value copied from the database containing the last login time

    //Stats for this character
    unsigned int deaths;
    unsigned int kills;
    unsigned int suicides;


public:
    // NPC based functions - should these go here?
    int NPC_GetSpawnRuleID() { return npc_spawnruleid; }
    void NPC_SetSpawnRuleID(int v) { npc_spawnruleid=v; }

    st_location spawn_loc;

    bool AppendCharacterSelectData(psAuthApprovedMessage& auth);

    ///  The new operator is overriden to call PoolAllocator template functions
    void *operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void *);

private:
    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psCharacter> characterpool;

public:
    bool loaded;
};

#endif


