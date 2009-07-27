/*
* pscharacter.cpp
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/virtclk.h>
#include <csutil/databuf.h>
#include <ctype.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "../psserver.h"
#include "../entitymanager.h"
#include "../cachemanager.h"
#include "../clients.h"
#include "../psserverchar.h"
#include "../exchangemanager.h"
#include "../spellmanager.h"
#include "../workmanager.h"
#include "../marriagemanager.h"
#include "../npcmanager.h"
#include "../playergroup.h"
#include "../events.h"
#include "../progressionmanager.h"
#include "../chatmanager.h"
#include "../commandmanager.h"
#include "../gmeventmanager.h"
#include "../progressionmanager.h"
#include "../client.h"
#include "../globals.h"


#include "util/psdatabase.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/log.h"

#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacterloader.h"
#include "psglyph.h"
#include "psquest.h"
#include "dictionary.h"
#include "psraceinfo.h"
#include "psguildinfo.h"
#include "psmerchantinfo.h"
#include "pstrainerinfo.h"
#include "servervitals.h"
#include "pssectorinfo.h"
#include "pstrait.h"
#include "adminmanager.h"


// The sizes and scripts need balancing.  For now, maxSize is disabled.
#define ENABLE_MAX_CAPACITY 0


const char *psCharacter::characterTypeName[] = { "player", "npc", "pet", "mount" };

MathScript *psCharacter::maxRealmScript    = NULL;
MathScript *psCharacter::staminaCalc       = NULL;
MathScript *psCharacter::expSkillCalc      = NULL;
MathScript *psCharacter::staminaRatioWalk  = NULL;
MathScript *psCharacter::staminaRatioStill = NULL;
MathScript *psCharacter::staminaRatioSit   = NULL;
MathScript *psCharacter::staminaRatioWork  = NULL;

//-----------------------------------------------------------------------------


// Definition of the itempool for psCharacter(s)
PoolAllocator<psCharacter> psCharacter::characterpool;


void *psCharacter::operator new(size_t allocSize)
{
// Debug3(LOG_CHARACTER,"%i %i", allocSize,sizeof(psCharacter));
//    CS_ASSERT(allocSize<=sizeof(psCharacter));
    return (void *)characterpool.CallFromNew();
}

void psCharacter::operator delete(void *releasePtr)
{
    characterpool.CallFromDelete((psCharacter *)releasePtr);
}

psCharacter::psCharacter() : inventory(this),
    guildinfo(NULL), attributes(this), modifiers(this),
    skills(this), acquaintances(101, 10, 101), npc_masterid(0), deaths(0), kills(0), suicides(0), loaded(false)
{
    characterType = PSCHARACTER_TYPE_UNKNOWN;

    helmGroup.Clear();
    BracerGroup.Clear();
    BeltGroup.Clear();
    CloakGroup.Clear();
    help_event_flags = 0;
    memset(advantage_bitfield,0,sizeof(advantage_bitfield));
    accountid = 0;
    pid = 0;
    familiar_id = 0;
    owner_id = 0;

    override_max_hp = 0.0f;
    override_max_mana = 0.0f;

    name = lastname = fullname = " ";
    SetSpouseName( "" );
    isMarried = false;

    raceinfo = NULL;
    vitals = new psServerVitals(this);
//    workInfo = new WorkInformation();

    loot_category_id = 0;
    loot_money = 0;

    location.loc_sector = NULL;
    location.loc.Set(0.0f);
    location.loc_yrot = 0.0f;
    spawn_loc = location;

    for (int i=0;i<PSTRAIT_LOCATION_COUNT;i++)
        traits[i] = NULL;

    npc_spawnruleid = -1;

    tradingStopped = false;
    tradingStatus = psCharacter::NOT_TRADING;
    merchant = NULL;
    trainer = NULL;
    actor = NULL;
    merchantInfo = NULL;
    trainerInfo = NULL;

    timeconnected = 0;
    startTimeThisSession = csGetTicks();

//    transformation = NULL;
    kill_exp = 0;
    impervious_to_attack = 0;

    faction_standings.Clear();

    lastResponse = -1;

    banker = false;
    isStatue = false;
}

psCharacter::~psCharacter()
{
    if (guildinfo)
        guildinfo->Disconnect(this);

    // First force and update of the DB of all QuestAssignments before deleting
    // every assignment.
    UpdateQuestAssignments(true);
    assigned_quests.DeleteAll();

    delete vitals;
    vitals = NULL;

//    delete workInfo;
}

void psCharacter::SetActor( gemActor* newActor )
{
    actor = newActor;
    if (actor)
    {
        inventory.RunEquipScripts();
        inventory.CalculateLimits();
    }
}

bool psCharacter::Load(iResultRow& row)
{
    // Load the math scripts
    if (!staminaCalc)
    {
        staminaCalc = psserver->GetMathScriptEngine()->FindScript("StaminaBase");
        if (!staminaCalc)
        {
            Error1("Can't find math script StaminaBase! Character loading failed.");
            return false;
        }
    }

    if (!staminaRatioWalk)
    {
        staminaRatioWalk = psserver->GetMathScriptEngine()->FindScript("StaminaRatioWalk");
        if (!staminaRatioWalk)
        {
            Error1("Can't find math script StaminaRatioWalk! Character loading failed.");
            return false;
        }
    }
    
    if (!staminaRatioStill)
    {
        staminaRatioStill = psserver->GetMathScriptEngine()->FindScript("StaminaRatioStill");
        if (!staminaRatioStill)
        {
            Error1("Can't find math script StaminaRatioStill! Character loading failed.");
            return false;
        }
    }
    
    if (!staminaRatioSit)
    {
        staminaRatioSit = psserver->GetMathScriptEngine()->FindScript("StaminaRatioSit");
        if (!staminaRatioSit)
        {
            Error1("Can't find math script StaminaRatioSit! Character loading failed.");
            return false;
        }
    }
    
    if (!staminaRatioWork)
    {
        staminaRatioWork = psserver->GetMathScriptEngine()->FindScript("StaminaRatioWork");
        if (!staminaRatioWork)
        {
            Error1("Can't find math script StaminaRatioWork! Character loading failed.");
            return false;
        }
    }

    if (!expSkillCalc)
    {
        expSkillCalc = psserver->GetMathScriptEngine()->FindScript("Calculate Skill Experience");
        if (!expSkillCalc)
        {
            Error1("Can't find 'Calculate Skill Experience' math script. Character loading failed.");
            return false;
        }
    }

    if (!maxRealmScript)
    {
        maxRealmScript = psserver->GetMathScriptEngine()->FindScript("MaxRealm");
        if (!maxRealmScript)
        {
            Error1("Can't find math script MaxRealm! Character loading failed.");
            return false;
        }
    }

    // TODO:  Link in account ID?
    csTicks start = csGetTicks();
    pid = row.GetInt("id");
    accountid = AccountID(row.GetInt("account_id"));
    SetCharType( row.GetUInt32("character_type") );

    SetFullName(row["name"], row["lastname"]);
    SetOldLastName( row["old_lastname"] );

    unsigned int raceid = row.GetUInt32("racegender_id");
    psRaceInfo *raceinfo = CacheManager::GetSingleton().GetRaceInfoByID(raceid);
    if (!raceinfo)
    {
        Error3("Character ID %s has unknown race id %s. Character loading failed.",row["id"],row["racegender_id"]);
        return false;
    }
    SetRaceInfo(raceinfo);

    //Assign the Helm/bracer/belt/cloak Group
    Result GroupsResult(db->Select("SELECT helm, bracer, belt, cloak FROM race_info WHERE id=%d", raceid));

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    helmGroup = GroupsResult[0]["helm"];

    BracerGroup = GroupsResult[0]["bracer"];

    BeltGroup = GroupsResult[0]["belt"];

    CloakGroup = GroupsResult[0]["cloak"];

    SetDescription(row["description"]);

    SetOOCDescription(row["description_ooc"]); //loads the ooc description of the player

    SetCreationInfo(row["creation_info"]); //loads the info gathered in the creation in order to allow the player
                                           //to review them

    SetLifeDescription(row["description_life"]); //Loads the life events added by players

    attributes[PSITEMSTATS_STAT_STRENGTH] .   SetBase((int) row.GetFloat("base_strength"));
    attributes[PSITEMSTATS_STAT_AGILITY]   .  SetBase((int) row.GetFloat("base_agility"));
    attributes[PSITEMSTATS_STAT_ENDURANCE]  . SetBase((int) row.GetFloat("base_endurance"));
    attributes[PSITEMSTATS_STAT_INTELLIGENCE].SetBase((int) row.GetFloat("base_intelligence"));
    attributes[PSITEMSTATS_STAT_WILL]       . SetBase((int) row.GetFloat("base_will"));
    attributes[PSITEMSTATS_STAT_CHARISMA]  .  SetBase((int) row.GetFloat("base_charisma"));

    // NPC fields here
    npc_spawnruleid = row.GetUInt32("npc_spawn_rule");
    npc_masterid    = row.GetUInt32("npc_master_id");

    // This substitution allows us to make 100 orcs which are all copies of the stats, traits and equipment
    // from a single master instance.
    PID use_id = npc_masterid ? npc_masterid : pid;

    GetMaxHP().SetBase(row.GetFloat("base_hitpoints_max"));
    override_max_hp = GetMaxHP().Base();
    GetMaxMana().SetBase(row.GetFloat("base_mana_max"));
    override_max_mana = GetMaxMana().Base();

    if (!LoadSkills(use_id))
    {
        Error2("Cannot load skills for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    RecalculateStats();

    // If mod_hp or mod_mana are set < 0 in the db, then that means
    // to use whatever is calculated as the max, so npc's spawn at 100%.
    float mod = row.GetFloat("mod_hitpoints");
    SetHitPoints(mod < 0 ? GetMaxHP().Base() : mod);
    mod = row.GetFloat("mod_mana");
    SetMana(mod < 0 ? GetMaxMana().Base() : mod);
    SetStamina(row.GetFloat("stamina_physical"),true);
    SetStamina(row.GetFloat("stamina_mental"),false);

    vitals->SetOrigVitals(); // This saves them as loaded state for restoring later without hitting db, npc death resurrect.

    lastlogintime = row["last_login"];
    faction_standings = row["faction_standings"];
    progressionScriptText = row["progression_script"];

    // Set on-hand money.
    money.Set(
        row.GetInt("money_circles"),
        row.GetInt("money_octas"),
        row.GetInt("money_hexas"),
        row.GetInt("money_trias"));

    // Set bank money.
    bankMoney.Set(
        row.GetInt("bank_money_circles"),
        row.GetInt("bank_money_octas"),
        row.GetInt("bank_money_hexas"),
        row.GetInt("bank_money_trias"));

    psSectorInfo *sectorinfo=CacheManager::GetSingleton().GetSectorInfoByID(row.GetUInt32("loc_sector_id"));
    if (sectorinfo==NULL)
    {
        Error3("Character %s has unresolvable sector id %lu.", ShowID(pid), row.GetUInt32("loc_sector_id"));
        return false;
    }

    SetLocationInWorld(row.GetUInt32("loc_instance"),
                       sectorinfo,
                       row.GetFloat("loc_x"),
                       row.GetFloat("loc_y"),
                       row.GetFloat("loc_z"),
                       row.GetFloat("loc_yrot") );
    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    spawn_loc = location;

    // Guild fields here
    guildinfo = CacheManager::GetSingleton().FindGuild(row.GetUInt32("guild_member_of"));
    if (guildinfo)
        guildinfo->Connect(this);

    SetGuildNotifications(row.GetInt("guild_notifications") == 1);

    // Loot rule here
    loot_category_id = row.GetInt("npc_addl_loot_category_id");

    impervious_to_attack = (row["npc_impervious_ind"][0]=='Y') ? ALWAYS_IMPERVIOUS : 0;

    // Familiar Fields here
    animal_affinity  = row[ "animal_affinity" ];
    //owner_id         = row.GetUInt32( "owner_id" );
    help_event_flags = row.GetUInt32("help_event_flags");

    if (!LoadTraits(use_id))
    {
        Error2("Cannot load traits for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if (!LoadAdvantages(use_id))
    {
        Error2("Cannot load advantages for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // This data is loaded only if it's a player, not an NPC
    if ( !IsNPC() && !IsPet() )
    {
        if (!LoadQuestAssignments())
        {
            Error2("Cannot load quest assignments for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }

        if (!LoadGMEvents())
        {
            Error2("Cannot load GM Events for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }

    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if (use_id != pid)
    {
        // This has a master npc template, so load character specific items
        // from the master npc.
        if (!inventory.Load(use_id))
        {
            Error2("Cannot load character specific items for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }
    }
    else
    {
        inventory.Load();
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if (!LoadRelationshipInfo(pid)) // Buddies, Marriage Info, Familiars
    {
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // Load merchant info
    csRef<psMerchantInfo> merchant = csPtr<psMerchantInfo>(new psMerchantInfo());
    if (merchant->Load(use_id))
    {
        merchantInfo = merchant;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // Load trainer info
    csRef<psTrainerInfo> trainer = csPtr<psTrainerInfo>(new psTrainerInfo());
    if (trainer->Load(use_id))
    {
        trainerInfo = trainer;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if (!LoadSpells(use_id))
    {
        Error2("Cannot load spells for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    timeconnected        = row.GetUInt32("time_connected_sec");
    startTimeThisSession = csGetTicks();

    // Load Experience Points W and Progression Points X
    SetExperiencePoints(row.GetUInt32("experience_points"));
    SetProgressionPoints(row.GetUInt32("progression_points"),false);

    // Load the kill exp
    kill_exp = row.GetUInt32("kill_exp");

    // Load if the character/npc is a banker
    if(row.GetInt("banker") == 1)
        banker = true;

     // Load if the character is a statue
    if(row.GetInt("statue") == 1)
        isStatue = true;

    loaded = true;
    return true;
}

bool psCharacter::QuickLoad(iResultRow& row, bool noInventory)
{
    pid = row.GetInt("id");
    SetFullName(row["name"], row["lastname"]);

    unsigned int raceid = row.GetUInt32("racegender_id");
    psRaceInfo *raceinfo = CacheManager::GetSingleton().GetRaceInfoByID(raceid);
    if (!raceinfo)
    {
        Error3("Character ID %s has unknown race id %s.",row["id"],row["racegender_id"]);
        return false;
    }

    if (!noInventory)
    {
        SetRaceInfo(raceinfo);

        helmGroup = raceinfo->helmGroup;
        BracerGroup = raceinfo->BracerGroup;
        BeltGroup = raceinfo->BeltGroup;
        CloakGroup = raceinfo->CloakGroup;

        if (!LoadTraits(pid))
        {
            Error2("Cannot load traits for Character %s.", ShowID(pid));
            return false;
        }

        // Load equipped items
        inventory.QuickLoad(pid);
    }

    return true;
}

bool psCharacter::LoadRelationshipInfo(PID pid)
{
    Result has_a(db->Select("SELECT a.*, b.name AS 'buddy_name' FROM character_relationships a, characters b WHERE a.character_id = %u AND a.related_id = b.id", pid.Unbox()));
    Result of_a(db->Select("SELECT a.*, b.name AS 'buddy_name' FROM character_relationships a, characters b WHERE a.related_id = %u AND a.character_id = b.id ", pid.Unbox()));

    if ( !LoadFamiliar( has_a, of_a ) )
    {
        Error2("Cannot load familiar info for Character %s.", ShowID(pid));
        return false;
    }

    if ( !LoadMarriageInfo( has_a ) )
    {
      Error2("Cannot load Marriage Info for Character %s.", ShowID(pid));
      return false;
    }

    if ( !LoadBuddies( has_a, of_a ) )
    {
        Error2("Cannot load buddies for Character %s.", ShowID(pid));
        return false;
    }

    if ( !LoadExploration( has_a ) )
    {
        Error2("Cannot load exploration points for Character %s.", ShowID(pid));
        return false;
    }

    return true;
}

void psCharacter::LoadIntroductions()
{
    Result r = db->Select("SELECT * FROM introductions WHERE charid=%d", pid.Unbox());
    if (r.IsValid())
    {
        for (unsigned long i = 0; i < r.Count(); i++)
        {
            unsigned int charID = r[i].GetUInt32("introcharid");
            // Safe to skip test because the DB disallows duplicate rows
            acquaintances.AddNoTest(charID);
        }
    }
}


bool psCharacter::LoadBuddies( Result& myBuddies, Result& buddyOf )
{
    unsigned int x;

    if ( !myBuddies.IsValid() )
      return true;

    for ( x = 0; x < myBuddies.Count(); x++ )
    {

        if ( strcmp( myBuddies[x][ "relationship_type" ], "buddy" ) == 0 )
        {
            Buddy newBud;
            newBud.name = myBuddies[x][ "buddy_name" ];
            newBud.playerID = PID(myBuddies[x].GetUInt32("related_id"));

            buddyList.Insert( 0, newBud );
        }
    }


    // Load all the people that I am a buddy of. This is used to inform these people
    // of when I log in/out.

    for (x = 0; x < buddyOf.Count(); x++ )
    {
        if ( strcmp( buddyOf[x][ "relationship_type" ], "buddy" ) == 0 )
        {
            buddyOfList.Insert( 0, buddyOf[x].GetUInt32( "character_id" ) );
        }
    }

    return true;
}

bool psCharacter::LoadMarriageInfo( Result& result)
{
    if ( !result.IsValid() )
    {
        Error3("Could not load marriage info for %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    for ( unsigned int x = 0; x < result.Count(); x++ )
    {

        if ( strcmp( result[x][ "relationship_type" ], "spouse" ) == 0 )
        {
            const char* spouseName = result[x]["spousename"];
            if ( spouseName == NULL )
                return true;

            SetSpouseName( spouseName );

            Notify2( LOG_MARRIAGE, "Successfully loaded marriage info for %s", name.GetData() );
            break;
        }
    }

    return true;
}

bool psCharacter::LoadFamiliar( Result& pet, Result& owner )
{
    familiar_id = 0;
    owner_id = 0;

    if ( !pet.IsValid() )
    {
        Error3("Could not load pet info for %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    if ( !owner.IsValid() )
    {
        Error3("Could not load owner info for character %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    unsigned int x;
    for ( x = 0; x < pet.Count(); x++ )
    {

        if ( strcmp( pet[x][ "relationship_type" ], "familiar" ) == 0 )
        {
            familiar_id = pet[x].GetInt( "related_id" );
            Notify2( LOG_MARRIAGE, "Successfully loaded familair for %s", name.GetData() );
            break;
        }
    }

    for ( x = 0; x < owner.Count(); x++ )
    {

        if ( strcmp( owner[x][ "relationship_type" ], "familiar" ) == 0 )
        {
            owner_id = owner[x].GetInt( "character_id" );
            Notify2( LOG_MARRIAGE, "Successfully loaded owner for %s", name.GetData() );
            break;
        }
    }

    return true;
}

bool psCharacter::LoadExploration(Result& exploration)
{
    if(!exploration.IsValid())
    {
        Error3("Could not load exploration info for character %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    for(uint i=0; i<exploration.Count(); ++i)
    {
        if(strcmp(exploration[i]["relationship_type"], "exploration") == 0)
        {
            explored_areas.Push(exploration[i].GetInt("related_id"));
        }
    }

    return true;
}

/// Load GM events for this player from GMEventManager
bool psCharacter::LoadGMEvents(void)
{
    assigned_events.runningEventID =
        psserver->GetGMEventManager()->GetAllGMEventsForPlayer(pid,
                                                               assigned_events.completedEventIDs,
                                                               assigned_events.runningEventIDAsGM,
                                                               assigned_events.completedEventIDsAsGM);
    return true;  // cant see how this can fail, but keep convention for now.
}

void psCharacter::SetLastLoginTime(const char *last_login, bool save )
{
    csString timeStr;

    if ( !last_login )
    {
        time_t curr=time(0);
        tm* gmtm = gmtime(&curr);

        timeStr.Format("%d-%02d-%02d %02d:%02d:%02d",
                        gmtm->tm_year+1900,
                        gmtm->tm_mon+1,
                        gmtm->tm_mday,
                        gmtm->tm_hour,
                        gmtm->tm_min,
                        gmtm->tm_sec);
    }
    else
    {
        timeStr = last_login;
    }

    this->lastlogintime = timeStr;

    if ( save )
    {
        //Store in database
        if (!db->CommandPump("UPDATE characters SET last_login='%s' WHERE id='%d'", timeStr.GetData(), pid.Unbox()))
        {
             Error2( "Last login storage: DB Error: %s\n", db->GetLastError() );
             return;
        }
    }
}

bool psCharacter::LoadSpells(PID use_id)
{
    // Load spells in asc since we use push to create the spell list.
    Result spells(db->Select("SELECT * FROM player_spells WHERE player_id=%u ORDER BY spell_slot ASC", use_id.Unbox()));
    if (spells.IsValid())
    {
        int i,count=spells.Count();

        for (i=0;i<count;i++)
        {
            psSpell * spell = CacheManager::GetSingleton().GetSpellByID(spells[i].GetInt("spell_id"));
            if (spell != NULL)
                AddSpell(spell);
            else
            {
                Error2("Spell id=%i not found in cachemanager", spells[i].GetInt("spell_id"));
            }
        }
        return true;
    }
    else
        return false;
}

bool psCharacter::LoadAdvantages(PID use_id)
{
    // Load advantages/disadvantages
    Result adv(db->Select("SELECT * FROM character_advantages WHERE character_id=%u", use_id.Unbox()));
    if (adv.IsValid())
    {
        unsigned int i;
        for (i=0;i<adv.Count();i++)
        {
            AddAdvantage((PSCHARACTER_ADVANTAGE)(adv[i].GetInt("advantage_id")));
        }
       return true;
    }
    else
        return false;
}

bool psCharacter::LoadSkills(PID use_id)
{
    // Load skills
    Result skillResult(db->Select("SELECT * FROM character_skills WHERE character_id=%u", use_id.Unbox()));

    for ( int z = 0; z < PSSKILL_COUNT; z++ )
    {
        skills.SetSkillInfo( (PSSKILL)z, CacheManager::GetSingleton().GetSkillByID((PSSKILL)z), false );
    }

    if (skillResult.IsValid())
    {
        unsigned int i;
        for (i=0;i<skillResult.Count();i++)
        {
            if (skillResult[i]["skill_id"]!=NULL)
            {
                PSSKILL skill = (PSSKILL)skillResult[i].GetInt("skill_id");
                skills.SetSkillPractice(  skill, skillResult[i].GetInt("skill_Z") );
                skills.SetSkillKnowledge( skill, skillResult[i].GetInt("skill_Y") );
                skills.SetSkillRank(      skill, skillResult[i].GetInt("skill_Rank"),false );
                skills.Get(skill).dirtyFlag = false;
            }
        }
        skills.Calculate();
    }
    else
        return false;

    // Set stats ranks
    skills.SetSkillRank(PSSKILL_AGI,  attributes[PSITEMSTATS_STAT_AGILITY].Base()     , false);
    skills.SetSkillRank(PSSKILL_CHA,  attributes[PSITEMSTATS_STAT_CHARISMA].Base()    , false);
    skills.SetSkillRank(PSSKILL_END,  attributes[PSITEMSTATS_STAT_ENDURANCE].Base()   , false);
    skills.SetSkillRank(PSSKILL_INT,  attributes[PSITEMSTATS_STAT_INTELLIGENCE].Base(), false);
    skills.SetSkillRank(PSSKILL_WILL, attributes[PSITEMSTATS_STAT_WILL].Base()        , false);
    skills.SetSkillRank(PSSKILL_STR,  attributes[PSITEMSTATS_STAT_STRENGTH].Base()    , false);

    skills.Get(PSSKILL_AGI).dirtyFlag = false;
    skills.Get(PSSKILL_CHA).dirtyFlag = false;
    skills.Get(PSSKILL_END).dirtyFlag = false;
    skills.Get(PSSKILL_INT).dirtyFlag = false;
    skills.Get(PSSKILL_WILL).dirtyFlag = false;
    skills.Get(PSSKILL_STR).dirtyFlag = false;

    return true;
}

bool psCharacter::LoadTraits(PID use_id)
{
    // Load traits
    Result traits(db->Select("SELECT * FROM character_traits WHERE character_id=%u", use_id.Unbox()));
    if (traits.IsValid())
    {
        unsigned int i;
        for (i=0;i<traits.Count();i++)
        {
            psTrait *trait=CacheManager::GetSingleton().GetTraitByID(traits[i].GetInt("trait_id"));
            if (!trait)
            {
                Error3("%s has unknown trait id %s.", ShowID(pid), traits[i]["trait_id"]);
            }
            else
                SetTraitForLocation(trait->location,trait);
        }
        return true;
    }
    else
        return false;
}

void psCharacter::AddSpell(psSpell * spell)
{
    spellList.Push(spell);
}

void psCharacter::SetFullName(const char* newFirstName, const char* newLastName)
{
    if ( !newFirstName )
    {
        Error1( "Null passed as first name..." );
        return;
    }

    // Error3( "SetFullName( %s, %s ) called...", newFirstName, newLastName );
    // Update fist, last & full name
    if ( strlen(newFirstName) )
    {
        name = newFirstName;
        fullname = name;
    }
    if ( newLastName )
    {
        lastname = newLastName;

        if ( strlen(newLastName) )
        {
            fullname += " ";
            fullname += lastname;
        }
    }

    //Error2( "New fullname is now: %s", fullname.GetData() );
}

void psCharacter::SetRaceInfo(psRaceInfo *rinfo)
{
    raceinfo=rinfo;

    if ( !rinfo )
        return;

    attributes[PSITEMSTATS_STAT_STRENGTH] .   SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_STRENGTH)));
    attributes[PSITEMSTATS_STAT_AGILITY]   .  SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_AGILITY)));
    attributes[PSITEMSTATS_STAT_ENDURANCE]  . SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_ENDURANCE)));
    attributes[PSITEMSTATS_STAT_INTELLIGENCE].SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_INTELLIGENCE)));
    attributes[PSITEMSTATS_STAT_WILL]       . SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_WILL)));
    attributes[PSITEMSTATS_STAT_CHARISMA]  .  SetBase(int(rinfo->GetBaseAttribute(PSITEMSTATS_STAT_CHARISMA)));
}

void psCharacter::SetFamiliarID(PID v)
{
    csString sql;

    familiar_id = v;
    sql.Format("INSERT INTO character_relationships VALUES (%u, %d, 'familiar', '')", pid.Unbox(), familiar_id.Unbox());
    if( !db->Command( sql ) )
    {
        Error3("Couldn't execute SQL %s!, %s's pet relationship is not saved.", sql.GetData(), ShowID(pid));
    }

};

void psCharacter::LoadActiveSpells()
{
    if (progressionScriptText.IsEmpty())
        return;

    ProgressionScript *script = ProgressionScript::Create(fullname, progressionScriptText);
    if (!script)
    {
        Error3("Saved progression script for >%s< is invalid:\n%s", fullname.GetData(), progressionScriptText.GetData());
        return;
    }

    CS_ASSERT(actor);

    MathEnvironment env;
    env.Define("Actor", actor);
    script->Run(&env);

    delete script;
}

void psCharacter::UpdateRespawn(csVector3 pos, float yrot, psSectorInfo *sector, InstanceID instance)
{
    spawn_loc.loc_sector = sector;
    spawn_loc.loc = pos;
    spawn_loc.loc_yrot   = yrot;
    spawn_loc.worldInstance = instance;

    // Save to database
    st_location & l = spawn_loc;
    psString sql;

    sql.AppendFmt("update characters set loc_x=%10.2f, loc_y=%10.2f, loc_z=%10.2f, loc_yrot=%10.2f, loc_sector_id=%u, loc_instance=%u where id=%u",
                     l.loc.x, l.loc.y, l.loc.z, l.loc_yrot, l.loc_sector->uid, l.worldInstance, pid.Unbox());
    if (db->CommandPump(sql) != 1)
    {
        Error3 ("Couldn't save character's position to database.\nCommand was "
            "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}

void psCharacter::AddAdvantage( PSCHARACTER_ADVANTAGE advantage)
{
    if (advantage<0 || advantage>=PSCHARACTER_ADVANTAGE_COUNT)
        return;
    // Get the index into the advantages array.  32 bits per entry.
    int advantage_index=(int)advantage/32;
    // Get the bit offset in the advantage entry, and use it to generate a single bit bitmask.
    unsigned int advantage_bitmask=1 << ((unsigned int)advantage % 32);

    // Set the bit
    advantage_bitfield[advantage_index]|=advantage_bitmask;
}

void psCharacter::RemoveAdvantage( PSCHARACTER_ADVANTAGE advantage)
{
    if (advantage<0 || advantage>=PSCHARACTER_ADVANTAGE_COUNT)
        return;
    // Get the index into the advantages array.  32 bits per entry.
    int advantage_index=(int)advantage/32;
    // Get the bit offset in the advantage entry, and use it to generate an inverse bitmask.
    unsigned int advantage_bitmask=~( 1 << ((unsigned int)advantage % 32));

    // Clear the bit
    advantage_bitfield[advantage_index]&=advantage_bitmask;
}


bool psCharacter::HasAdvantage ( PSCHARACTER_ADVANTAGE advantage)
{
    if (advantage<0 || advantage>=PSCHARACTER_ADVANTAGE_COUNT)
        return false;
    // Get the index into the advantages array.  32 bits per entry.
    int advantage_index=(int)advantage/32;
    // Get the bit offset in the advantage entry, and use it to generate a single bit bitmask.
    unsigned int advantage_bitmask=1 << ((unsigned int)advantage % 32);

    // This will return a value other than 0 if the bit is set.
    return advantage_bitfield[advantage_index] & advantage_bitmask ? true : false;
}


unsigned int psCharacter::GetExperiencePoints() // W
{
    return vitals->GetExp();
}

void psCharacter::SetExperiencePoints(unsigned int W)
{
    vitals->SetExp(W);
}

/*
* Will adde W to the experience points. While the number
* of experience points are greater than needed points
* for progression points the experience points are transformed
* into  progression points.
* @return Return the number of progression points gained.
*/
unsigned int psCharacter::AddExperiencePoints(unsigned int W)
{
    unsigned int pp = 0;
    unsigned int exp = vitals->GetExp();
    unsigned int progP = vitals->GetPP();

    exp += W;
    bool updatedPP = false;

    while (exp >= 200)
    {
        exp -= 200;
        if(progP != UINT_MAX) //don't allow overflow
            progP++;
        pp++;
        updatedPP = true;
    }

    vitals->SetExp(exp);
    if(updatedPP)
    {
        SetProgressionPoints(progP,true);
    }

    return pp;
}

unsigned int psCharacter::AddExperiencePointsNotify(unsigned int experiencePoints)
{
    if(experiencePoints > 0)
    {
        unsigned int PP = AddExperiencePoints(experiencePoints);
        if(GetActor() && GetActor()->GetClientID())
        {
            if(PP > 0)
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(), "You gained some experience points and %d progression points!", PP);
            }
            else
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(),"You gained some experience points");
            }
        }
        return PP;
    }
    return 0;
}

unsigned int psCharacter::CalculateAddExperience(PSSKILL skill, unsigned int practicePoints, float modifier)
{
    if(practicePoints > 0)
    {
        MathEnvironment env;
        env.Define("ZCost", skills.Get(skill).zCost);
        env.Define("YCost", skills.Get(skill).yCost);
        env.Define("ZCostNext", skills.Get(skill).zCostNext);
        env.Define("YCostNext", skills.Get(skill).yCostNext);
        env.Define("Character", this);
        env.Define("PracticePoints", practicePoints);
        env.Define("Modifier", modifier);
        expSkillCalc->Evaluate(&env);
        unsigned int experiencePoints = env.Lookup("Exp")->GetValue();

        AddExperiencePointsNotify(experiencePoints);

        if (CacheManager::GetSingleton().GetSkillByID((PSSKILL)skill)) //check if skill is valid
        {
            Skills().AddSkillPractice(skill, practicePoints);
        }
        return experiencePoints;
    }
    return 0;
}

void psCharacter::SetSpouseName( const char* name )
{
    if ( !name )
        return;

    spouseName = name;

    if ( !strcmp(name,"") )
        isMarried = false;
    else
        isMarried = true;

}

unsigned int psCharacter::GetProgressionPoints() // X
{
    return vitals->GetPP();
}

void psCharacter::SetProgressionPoints(unsigned int X,bool save)
{
    unsigned int exp = vitals->GetExp();
    if (save)
    {
        Debug3(LOG_SKILLXP, pid.Unbox(), "Updating PP points and Exp to %u and %u\n", X, exp);
        // Update the DB
        csString sql;
        sql.Format("UPDATE characters SET progression_points = '%u', experience_points = '%u' WHERE id ='%u'", X, exp, pid.Unbox());
        if(!db->CommandPump(sql))
        {
            Error3("Couldn't execute SQL %s!, %s's PP points are NOT saved", sql.GetData(), ShowID(pid));
        }
    }

    vitals->SetPP( X );
}

void psCharacter::UseProgressionPoints(unsigned int X)
{

    SetProgressionPoints(vitals->GetPP()-X,true);
}

int psCharacter::GetMaxAllowedRealm( PSSKILL skill )
{
    unsigned int waySkillRank = skills.GetSkillRank(skill).Current();

    if (waySkillRank == 0)  // zero skill = no casting
        return 0;

    MathEnvironment env;
    env.Define("WaySkill", waySkillRank);

    maxRealmScript->Evaluate(&env);

    MathVar *maxRealm = env.Lookup("MaxRealm");
    if (!maxRealm)
    {
        Error1("Failed to evaluate MathScript >MaxRealm<.");
        return 0;
    }
    return (int) maxRealm->GetValue();
}

bool psCharacter::CheckMagicKnowledge( PSSKILL skill, int realm )
{
    if (GetMaxAllowedRealm(skill) >= realm)
        return true;

    // Special case for rank 0 people just starting.
    return (realm == 1 && skills.GetSkillRank(skill).Base() == 0 && !skills.Get(skill).CanTrain());
}

void psCharacter::DropItem(psItem *&item, csVector3 suggestedPos, float yrot, bool guarded, bool transient, bool inplace)
{
    if (!item)
        return;

    if (item->IsInUse())
    {
        psserver->SendSystemError(actor->GetClientID(),"You cannot drop an item while using it.");
        return;
    }

    // Handle position...
    if (suggestedPos != 0)
    {
        // User-specified position...check if it's close enough to the character.

        csVector3 delta;
        delta = location.loc - suggestedPos;
        float dist = delta.Norm();

        // Future: Could make it drop in the direction specified, if not at the
        //         exact location...
        if (dist > 2 && actor->GetClient()->GetSecurityLevel() < GM_DEVELOPER) // max drop distance is 15m
            suggestedPos = 0;
    }

    if (suggestedPos == 0 && !inplace)
    {
        // No position specified or it was invalid.
        suggestedPos.x = location.loc.x - (DROP_DISTANCE * sinf(location.loc_yrot));
        suggestedPos.y = location.loc.y;
        suggestedPos.z = location.loc.z - (DROP_DISTANCE * cosf(location.loc_yrot));
    }
    else if (inplace)
    {
        suggestedPos.x = location.loc.x;
        suggestedPos.y = location.loc.y;
        suggestedPos.z = location.loc.z;
    }

    // Play the drop item sound for this item
    psserver->GetCharManager()->SendOutPlaySoundMessage(actor->GetClientID(), item->GetSound(), "drop");

    // Announce drop (in the future, there should be a drop animation)
    psSystemMessage newmsg(actor->GetClientID(), MSG_INFO_BASE, "%s dropped %s.", fullname.GetData(), item->GetQuantityName().GetData());
    newmsg.Multicast(actor->GetMulticastClients(), 0, RANGE_TO_SELECT);

    // If we're dropping from inventory, we should properly remove it.
    // No need to check the return value: we're removing the whole item and
    // already have a pointer.  Plus, well get NULL and crash if it isn't...
    inventory.RemoveItemID(item->GetUID());

    if(guarded) //if we want to guard the item assign the guarding pid
        item->SetGuardingCharacterID(pid);

    gemObject* obj = EntityManager::GetSingleton().MoveItemToWorld(item,
                             location.worldInstance, location.loc_sector,
                             suggestedPos.x, suggestedPos.y, suggestedPos.z,
                             yrot, this, transient);

    if (obj)
    {
        // Assign new object to replace the original object
        item = obj->GetItem();
    }

    psMoney money;

    psDropEvent evt(pid,
                    item->GetUID(),
                    item->GetStackCount(),
                    (int)item->GetCurrentStats()->GetQuality(),
                    0);
    evt.FireEvent();

    // If a container, move its contents as well...
    gemContainer *cont = dynamic_cast<gemContainer*> (obj);
    if (cont)
    {
        for (size_t i=0; i < Inventory().GetInventoryIndexCount(); i++)
        {
            psItem *item = Inventory().GetInventoryIndexItem(i);
            if (item->GetContainerID() == cont->GetItem()->GetUID())
            {
                // This item is in the dropped container
                size_t slot = item->GetLocInParent() - PSCHARACTER_SLOT_BULK1;
                Inventory().RemoveItemIndex(i);
                if (!cont->AddToContainer(item, actor->GetClient(), (int)slot))
                {
                    Error2("Cannot add item into container slot %zu.\n", slot);
                    return;
                }
                if(guarded) //if we want to guard the item assign the guarding pid
                    item->SetGuardingCharacterID(pid);
                i--; // indexes shift when we remove one.
                item->Save(false);
            }
        }
    }
}

// This is lame, but we need a key for these special "modifier" things
#define MODIFIER_FAKE_ACTIVESPELL ((ActiveSpell*) 0xf00)
void psCharacter::CalculateEquipmentModifiers()
{
    // In this method we potentially call Equip and Unequip scripts that modify stats.
    // The stat effects that these scripts have are indistinguishable from magic effects on stats,
    // and magical changes to stats also need to call this method (weapon may need to be set
    // inactive when strength spell expires). This could result in an
    // endless loop, hence this method locks itself against being called recursively.
    static bool lock_me = false;
    if(lock_me) {
      return;
    }
    lock_me = true;

    csList<psItem*> itemlist;

    for (int i = 0; i < PSITEMSTATS_STAT_COUNT; i++)
    {
        modifiers[(PSITEMSTATS_STAT) i].Cancel(MODIFIER_FAKE_ACTIVESPELL);
    }

    psItem *currentitem = NULL;

    // Loop through every holding item adding it to list of items to check
    for(int i = 0; i < PSCHARACTER_SLOT_BULK1; i++)
    {
        currentitem=inventory.GetInventoryItem((INVENTORY_SLOT_NUMBER)i);

        // checking the equipment array is necessary since the item could be just unequipped
        // (this method is also called by Unequip)
        if(!currentitem || inventory.GetEquipmentObject(currentitem->GetLocInParent()).itemIndexEquipped == 0)
            continue;

        itemlist.PushBack(currentitem);
    }
    // go through list and make items active whose requirements are fulfilled and remove item from list.
    // stop when a complete loop has been made without making a change.
    bool hasChanged;
    do
    {
        hasChanged = false;
        csList<psItem*>::Iterator it(itemlist);
        while (it.HasNext())
        {
            currentitem = it.Next();

            csString response;
            if (!currentitem->CheckRequirements(this, response))
            {
                continue;
            }
            if (!currentitem->IsActive())
            {
                currentitem->RunEquipScript(actor);
            }
            // Check for attr bonuses
            for (int i = 0; i < PSITEMSTATS_STAT_BONUS_COUNT; i++)
            {
                modifiers[currentitem->GetWeaponAttributeBonusType(i)].Buff(MODIFIER_FAKE_ACTIVESPELL, (int) currentitem->GetWeaponAttributeBonusMax(i));
            }
            hasChanged = true;
            itemlist.Delete(it);
            break;
        }
    } while (hasChanged);

    // go through list of items whose requirements are not fulfilled and deactivate them
    csList<psItem*>::Iterator i(itemlist);
    while( i.HasNext() )
    {
        currentitem = i.Next();
        if (currentitem->IsActive())
        {
            currentitem->CancelEquipScript();
        }
    }
    itemlist.DeleteAll();
    lock_me = false;
}

void psCharacter::AddLootItem(psItemStats *item)
{
    if (!item)
    {
        Error2("Attempted to add 'null' loot item to character %s, ignored.",fullname.GetDataSafe());
        return;
    }
    loot_pending.Push(item);
}

size_t psCharacter::GetLootItems(psLootMessage& msg, EID entity, int cnum)
{
    // adds inventory to loot. TEMPORARLY REMOVED. see KillNPC()
    //if ( loot_pending.GetSize() == 0 )
    //    AddInventoryToLoot();

    if (loot_pending.GetSize() )
    {
        csString loot;
        loot.Append("<loot>");

        for (size_t i=0; i<loot_pending.GetSize(); i++)
        {
            if (!loot_pending[i]) {
              printf("Potential ERROR: why this happens?");
              continue;
            }
            csString item;
            csString escpxml_imagename = EscpXML(loot_pending[i]->GetImageName());
            csString escpxml_name = EscpXML(loot_pending[i]->GetName());
            item.Format("<li><image icon=\"%s\" count=\"1\" /><desc text=\"%s\" /><id text=\"%u\" /></li>",
                  escpxml_imagename.GetData(),
                  escpxml_name.GetData(),
                  loot_pending[i]->GetUID());
            loot.Append(item);
        }
        loot.Append("</loot>");
        Debug3(LOG_COMBAT, pid.Unbox(), "Loot was %s for %s\n", loot.GetData(), name.GetData());
        msg.Populate(entity,loot,cnum);
    }
    return loot_pending.GetSize();
}

bool psCharacter::RemoveLootItem(int id)
{
    size_t x;
    for (x=0; x<loot_pending.GetSize(); x++)
    {
        if (!loot_pending[x]) {
          printf("Potential ERROR: why this happens?");
          loot_pending.DeleteIndex(x);
          continue;
        }

        if (loot_pending[x]->GetUID() == (uint32) id)
        {
            loot_pending.DeleteIndex(x);
            return true;
        }
    }
    return false;
}
int psCharacter::GetLootMoney()
{
    int val = loot_money;
    loot_money = 0;
    return val;
}

void psCharacter::ClearLoot()
{
    loot_pending.DeleteAll();
    loot_money = 0;
}

void psCharacter::SetMoney(psMoney m)
{
    money          = m;
    SaveMoney(false);
}

void psCharacter::SetMoney( psItem *& itemdata )
{
    /// Check to see if the item is a money item and treat as a special case.
    if ( itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_TRIA )
        money.AdjustTrias( itemdata->GetStackCount() );

    if ( itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_HEXA )
        money.AdjustHexas( itemdata->GetStackCount() );

    if ( itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_OCTA )
        money.AdjustOctas( itemdata->GetStackCount() );

    if ( itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_CIRCLE )
        money.AdjustCircles( itemdata->GetStackCount() );

    CacheManager::GetSingleton().RemoveInstance(itemdata);
    SaveMoney(false);
}



void psCharacter::AdjustMoney(psMoney m, bool bank)
{
    psMoney *mon;
    if(bank)
        mon = &bankMoney;
    else
        mon = &money;
    mon->Adjust( MONEY_TRIAS, m.GetTrias() );
    mon->Adjust( MONEY_HEXAS, m.GetHexas() );
    mon->Adjust( MONEY_OCTAS, m.GetOctas() );
    mon->Adjust( MONEY_CIRCLES, m.GetCircles() );
    SaveMoney(bank);
}

void psCharacter::SaveMoney(bool bank)
{
    if(!loaded)
        return;

    psString sql;

    if(bank)
    {
        sql.AppendFmt("update characters set bank_money_circles=%d, bank_money_trias=%d, bank_money_hexas=%d, bank_money_octas=%d where id=%u",
                      bankMoney.GetCircles(), bankMoney.GetTrias(), bankMoney.GetHexas(), bankMoney.GetOctas(), pid.Unbox());
    }
    else
    {
        sql.AppendFmt("update characters set money_circles=%d, money_trias=%d, money_hexas=%d, money_octas=%d where id=%u",
                      money.GetCircles(), money.GetTrias(), money.GetHexas(), money.GetOctas(), pid.Unbox());
    }

    if (db->CommandPump(sql) != 1)
    {
        Error3 ("Couldn't save character's money to database.\nCommand was "
            "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}

void psCharacter::ResetStats()
{
    vitals->ResetVitals();
    inventory.CalculateLimits();
}

void psCharacter::CombatDrain(int slot)
{
    static MathScript *script = NULL;
    if (!script)
        script = psserver->GetMathScriptEngine()->FindScript("StaminaCombat");
    if (!script)
        return;

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("Weapon", inventory.GetItem(NULL, (INVENTORY_SLOT_NUMBER) slot));

    script->Evaluate(&env);

    MathVar *phyDrain = env.Lookup("PhyDrain");
    MathVar *mntDrain = env.Lookup("MntDrain");
    if (!phyDrain || !mntDrain)
    {
        Error1("Failed to evaluate MathScript >StaminaCombat<.");
        return;
    }

    AdjustStamina(-phyDrain->GetValue(), true);
    AdjustStamina(-mntDrain->GetValue(), false);
}

void psCharacter::AdjustHitPoints(float delta)
{
    vitals->AdjustVital(VITAL_HITPOINTS, DIRTY_VITAL_HP, delta);
}

void psCharacter::SetHitPoints(float v)
{
    vitals->SetVital(VITAL_HITPOINTS, DIRTY_VITAL_HP,v);
}

float psCharacter::GetHP()
{
    return vitals->GetHP();
}

float psCharacter::GetMana()
{
    return vitals->GetMana();
}
float psCharacter::GetStamina(bool pys)
{
    return pys ? vitals->GetPStamina() : vitals->GetMStamina();
}

bool psCharacter::UpdateStatDRData(csTicks now)
{
    bool res = vitals->Update(now);

    // if HP dropped to zero, provoke the killing process
    if (GetHP() == 0   &&   actor != NULL   &&   actor->IsAlive())
        actor->Kill(NULL);
    return res;
}

bool psCharacter::SendStatDRMessage(uint32_t clientnum, EID eid, int flags, csRef<PlayerGroup> group)
{
    return vitals->SendStatDRMessage(clientnum, eid, flags, group);
}

VitalBuffable & psCharacter::GetMaxHP()
{
    return vitals->GetVital(VITAL_HITPOINTS).max;
}
VitalBuffable & psCharacter::GetMaxMana()
{
    return vitals->GetVital(VITAL_MANA).max;
}
VitalBuffable & psCharacter::GetMaxPStamina()
{
    return vitals->GetVital(VITAL_PYSSTAMINA).max;
}
VitalBuffable & psCharacter::GetMaxMStamina()
{
    return vitals->GetVital(VITAL_MENSTAMINA).max;
}

VitalBuffable & psCharacter::GetHPRate()
{
    return vitals->GetVital(VITAL_HITPOINTS).drRate;
}
VitalBuffable & psCharacter::GetManaRate()
{
    return vitals->GetVital(VITAL_MANA).drRate;
}
VitalBuffable & psCharacter::GetPStaminaRate()
{
    return vitals->GetVital(VITAL_PYSSTAMINA).drRate;
}
VitalBuffable & psCharacter::GetMStaminaRate()
{
    return vitals->GetVital(VITAL_MENSTAMINA).drRate;
}

void psCharacter::AdjustMana(float adjust)
{
    vitals->AdjustVital(VITAL_MANA, DIRTY_VITAL_MANA,adjust);
}

void psCharacter::SetMana(float v)
{
    vitals->SetVital(VITAL_MANA, DIRTY_VITAL_MANA, v);
}

void psCharacter::AdjustStamina(float delta, bool pys)
{
    if (pys)
        vitals->AdjustVital(VITAL_PYSSTAMINA, DIRTY_VITAL_PYSSTAMINA, delta);
    else
        vitals->AdjustVital(VITAL_MENSTAMINA, DIRTY_VITAL_MENSTAMINA, delta);
}

void psCharacter::SetStamina(float v,bool pys)
{
    if(pys)
        vitals->SetVital(VITAL_PYSSTAMINA, DIRTY_VITAL_PYSSTAMINA,v);
    else
        vitals->SetVital(VITAL_MENSTAMINA, DIRTY_VITAL_MENSTAMINA,v);
}

void psCharacter::SetStaminaRegenerationNone(bool physical,bool mental)
{
    if(physical) GetPStaminaRate().SetBase(0.0);
    if(mental)   GetMStaminaRate().SetBase(0.0);
}

void psCharacter::SetStaminaRegenerationWalk(bool physical,bool mental)
{
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_WALK]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_WALK]);
    
    staminaRatioWalk->Evaluate(&env);
    
    MathVar *ratePhy = env.Lookup("PStaminaRate");
    MathVar *rateMen = env.Lookup("MStaminaRate");
    
    if(physical && ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(mental   && rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
    
    //if(physical) GetPStaminaRate().SetBase(GetMaxPStamina().Current()/100 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_WALK]);
    //if(mental)   GetMStaminaRate().SetBase(GetMaxMStamina().Current()/100 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_WALK]);
}

void psCharacter::SetStaminaRegenerationSitting()
{
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);
    
    staminaRatioSit->Evaluate(&env);
    
    MathVar *ratePhy = env.Lookup("PStaminaRate");
    MathVar *rateMen = env.Lookup("MStaminaRate");
    
    if(ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
    
    //GetPStaminaRate().SetBase(GetMaxPStamina().Current() * 0.015 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    //GetMStaminaRate().SetBase(GetMaxMStamina().Current() * 0.015 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);
}

void psCharacter::SetStaminaRegenerationStill(bool physical,bool mental)
{
    
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);
    
    staminaRatioStill->Evaluate(&env);
    
    MathVar *ratePhy = env.Lookup("PStaminaRate");
    MathVar *rateMen = env.Lookup("MStaminaRate");
    
    if(physical && ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(mental   && rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());

    //if(physical) GetPStaminaRate().SetBase(GetMaxPStamina().Current()/100 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    //if(mental)   GetMStaminaRate().SetBase(GetMaxMStamina().Current()/100 * GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);
}

void psCharacter::SetStaminaRegenerationWork(int skill)
{
    //Gms don't want to lose stamina when testing
    if (actor->nevertired)
        return;

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);
    
    // Need real formula for this. Shouldn't be hard coded anyway.
    // Stamina drain needs to be set depending on the complexity of the task.
    psSkillInfo* skillInfo = CacheManager::GetSingleton().GetSkillByID(skill);
    //if skill is none (-1) we set zero here
    int factor = skillInfo? skillInfo->mental_factor : 100;

    env.Define("SkillMentalFactor",  factor);
    
    staminaRatioWork->Evaluate(&env);
    
    MathVar *ratePhy = env.Lookup("PStaminaRate");
    MathVar *rateMen = env.Lookup("MStaminaRate");
    
    if(ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());

    //GetPStaminaRate().SetBase(GetPStaminaRate().Base()-6.0*(100-factor)/100);
    //GetMStaminaRate().SetBase(GetMStaminaRate().Base()-6.0*(100-factor)/100);
}

void psCharacter::CalculateMaxStamina()
{
    MathEnvironment env;
    // Set all the skills vars
    env.Define("STR",  attributes[PSITEMSTATS_STAT_STRENGTH].Current());
    env.Define("END",  attributes[PSITEMSTATS_STAT_ENDURANCE].Current());
    env.Define("AGI",  attributes[PSITEMSTATS_STAT_AGILITY].Current());
    env.Define("INT",  attributes[PSITEMSTATS_STAT_INTELLIGENCE].Current());
    env.Define("WILL", attributes[PSITEMSTATS_STAT_WILL].Current());
    env.Define("CHA",  attributes[PSITEMSTATS_STAT_CHARISMA].Current());

    // Calculate
    staminaCalc->Evaluate(&env);

    MathVar *basePhy = env.Lookup("BasePhy");
    MathVar *baseMen = env.Lookup("BaseMen");
    if (!basePhy || !baseMen)
    {
        Error1("Failed to evaluate MathScript >StaminaBase<.");
        return;
    }
    // Set the max values
    GetMaxPStamina().SetBase(basePhy->GetValue());
    GetMaxMStamina().SetBase(baseMen->GetValue());
}

void psCharacter::ResetSwings(csTicks timeofattack)
{
    psItem *Weapon;

    for (int slot = 0; slot < PSCHARACTER_SLOT_BULK1; slot++)
    {
        Weapon = Inventory().GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);
        if (Weapon !=NULL)
        {
            inventory.GetEquipmentObject((INVENTORY_SLOT_NUMBER)slot).eventId=0;
        }
    }
}

void psCharacter::TagEquipmentObject(INVENTORY_SLOT_NUMBER slot,int eventId)
{
    psItem *Weapon;

    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return;

    // TODO: Reduce ammo if this is an ammunition using weapon

    // Reset next attack time
    Weapon=Inventory().GetEffectiveWeaponInSlot(slot);

    if (Weapon!=NULL)
        inventory.GetEquipmentObject(slot).eventId = eventId;

    //drain stamina on player attacks
    if(actor->GetClientID() && !actor->nevertired)
        CombatDrain(slot);
}

int psCharacter::GetSlotEventId(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return 0;

    return inventory.GetEquipmentObject(slot).eventId;
}

// AVPRO= attack Value progression (this is to change the progression of all the calculation in AV for now it is equal to 1)
#define AVPRO 1

float psCharacter::GetTargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem *weapon=Inventory().GetEffectiveWeaponInSlot(slot);
    if (weapon==NULL)
        return 0.0f;

    return weapon->GetTargetedBlockValue();
}

float psCharacter::GetUntargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem *weapon=Inventory().GetEffectiveWeaponInSlot(slot);
    if (weapon==NULL)
        return 0.0f;

    return weapon->GetUntargetedBlockValue();
}


float psCharacter::GetTotalTargetedBlockValue()
{
    float blockval=0.0f;
    int slot;

    for (slot=0;slot<PSCHARACTER_SLOT_BULK1;slot++)
        blockval+=GetTargetedBlockValueForWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);

    return blockval;
}

float psCharacter::GetTotalUntargetedBlockValue()
{
    float blockval=0.0f;
    int slot;

    for (slot=0;slot<PSCHARACTER_SLOT_BULK1;slot++)
        blockval+=GetUntargetedBlockValueForWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);

    return blockval;
}


float psCharacter::GetCounterBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem *weapon=inventory.GetEffectiveWeaponInSlot(slot);
    if (weapon==NULL)
        return 0.0f;

    return weapon->GetCounterBlockValue();
}

#define ARMOR_USES_SKILL(slot,skill) inventory.GetInventoryItem(slot)==NULL?inventory.GetEquipmentObject(slot).default_if_empty->GetArmorType()==skill:inventory.GetInventoryItem(slot)->GetArmorType()==skill

#define CALCULATE_ARMOR_FOR_SLOT(slot) \
    if (ARMOR_USES_SKILL(slot,PSITEMSTATS_ARMORTYPE_LIGHT)) light_p+=1.0f/6.0f; \
    if (ARMOR_USES_SKILL(slot,PSITEMSTATS_ARMORTYPE_MEDIUM)) med_p+=1.0f/6.0f; \
    if (ARMOR_USES_SKILL(slot,PSITEMSTATS_ARMORTYPE_HEAVY)) heavy_p+=1.0f/6.0f;


float psCharacter::GetDodgeValue()
{
    float heavy_p,med_p,light_p;
    float asdm;

    // hold the % of each type of armor worn
    heavy_p=med_p=light_p=0.0f;

    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_HELM);
    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_TORSO);
    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_ARMS);
    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_GLOVES);
    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_LEGS);
    CALCULATE_ARMOR_FOR_SLOT(PSCHARACTER_SLOT_BOOTS);

    // multiplies for skill
    heavy_p *= skills.GetSkillRank(PSSKILL_HEAVYARMOR).Current();
    med_p   *= skills.GetSkillRank(PSSKILL_MEDIUMARMOR).Current();
    light_p *= skills.GetSkillRank(PSSKILL_LIGHTARMOR).Current();

    // armor skill defense mod
    asdm=heavy_p+med_p+light_p;

    // if total skill is 0, give a little chance anyway to defend himself
    if (asdm==0)
        asdm=0.2F;

    // for now return just asdm
    return asdm;

    /*
    // MADM= Martial Arts Defense Mod=martial arts skill-(weight carried+ AGI malus of the armor +DEX malus of the armor) min 0
    // TODO: fix this to use armor agi malus
    madm=GetSkillRank(PSSKILL_MARTIALARTS).Current()-inventory.weight;
    if (madm<0.0f)
        madm=0.0f;

    // Active Dodge Value =ADV = (ASDM + agiDmod + MADM)*ADVP)^0.4
    // Out of Melee Defense Value=OMDV = (agiDmod+(ASDM\10)*PDVP)^0.4

    // TODO:  Use passive dv here when the conditions are finalized that it should be used (archery, casting, more?)

    dv=asdm + AgiMod  + madm;
    dv=pow(dv,(float)0.6);

    return dv;
    */
}


void psCharacter::PracticeArmorSkills(unsigned int practice, INVENTORY_SLOT_NUMBER attackLocation)
{

    psItem *armor = inventory.GetEffectiveArmorInSlot(attackLocation);

    switch (armor->GetArmorType())
    {
        case PSITEMSTATS_ARMORTYPE_LIGHT:
            skills.AddSkillPractice(PSSKILL_LIGHTARMOR,practice);
            break;
        case PSITEMSTATS_ARMORTYPE_MEDIUM:
            skills.AddSkillPractice(PSSKILL_MEDIUMARMOR,practice);
            break;
        case PSITEMSTATS_ARMORTYPE_HEAVY:
            skills.AddSkillPractice(PSSKILL_HEAVYARMOR,practice);
            break;
        default:
            break;
    }

}

void psCharacter::PracticeWeaponSkills(unsigned int practice)
{
    int slot;

    for (slot=0;slot<PSCHARACTER_SLOT_BULK1;slot++)
    {
        psItem *weapon=inventory.GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);
        if (weapon!=NULL)
            PracticeWeaponSkills(weapon,practice);
    }

}

void psCharacter::PracticeWeaponSkills(psItem * weapon, unsigned int practice)
{
    for (int index = 0; index < PSITEMSTATS_WEAPONSKILL_INDEX_COUNT; index++)
    {
        PSSKILL skill = weapon->GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)index);
        if (skill != PSSKILL_NONE)
            skills.AddSkillPractice(skill,practice);
    }
}

void psCharacter::SetTraitForLocation(PSTRAIT_LOCATION location,psTrait *trait)
{
    if (location<0 || location>=PSTRAIT_LOCATION_COUNT)
        return;

    traits[location]=trait;
}

psTrait *psCharacter::GetTraitForLocation(PSTRAIT_LOCATION location)
{
    if (location<0 || location>=PSTRAIT_LOCATION_COUNT)
        return NULL;

    return traits[location];
}


void psCharacter::GetLocationInWorld(InstanceID &instance,psSectorInfo *&sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot)
{
    sectorinfo=location.loc_sector;
    loc_x=location.loc.x;
    loc_y=location.loc.y;
    loc_z=location.loc.z;
    loc_yrot=location.loc_yrot;
    instance = location.worldInstance;
}

void psCharacter::SetLocationInWorld(InstanceID instance, psSectorInfo *sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot)
{
    psSectorInfo *oldsector = location.loc_sector;
    InstanceID oldInstance = location.worldInstance;

    location.loc_sector=sectorinfo;
    location.loc.x=loc_x;
    location.loc.y=loc_y;
    location.loc.z=loc_z;
    location.loc_yrot=loc_yrot;
    location.worldInstance = instance;

    if (oldInstance != instance || (oldsector && oldsector != sectorinfo))
    {
        if ( dynamic_cast<gemNPC*>(actor) == NULL ) // NOT an NPC so it's ok to save location info
            SaveLocationInWorld();
    }
}

void psCharacter::SaveLocationInWorld()
{
    if(!loaded)
        return;

    st_location & l = location;
    psString sql;

    sql.AppendFmt("update characters set loc_x=%10.2f, loc_y=%10.2f, loc_z=%10.2f, loc_yrot=%10.2f, loc_sector_id=%u, loc_instance=%u where id=%u",
                     l.loc.x, l.loc.y, l.loc.z, l.loc_yrot, l.loc_sector->uid, l.worldInstance, pid.Unbox());
    if (db->CommandPump(sql) != 1)
    {
        Error3 ("Couldn't save character's position to database.\nCommand was "
            "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}




psSpell * psCharacter::GetSpellByName(const csString& spellName)
{
    for (size_t i=0; i < spellList.GetSize(); i++)
    {
        if (spellList[i]->GetName().CompareNoCase(spellName)) return spellList[i];
    }
    return NULL;
}

psSpell * psCharacter::GetSpellByIdx(int index)
{
    if (index < 0 || (size_t)index >= spellList.GetSize())
        return NULL;
    return spellList[index];
}

bool psCharacter::SetTradingStopped(bool stopped)
{
    bool old = tradingStopped;
    tradingStopped=stopped;
    return old;
}

// Check if player and target is ready to do a exchange
//       - Not fighting
//       - Not casting spell
//       - Not exchanging with a third player
//       - Not player stopped trading
//       - Not trading with a merchant
bool psCharacter::ReadyToExchange()
{
    return (//TODO: Test for fighting &&
        //TODO: Test for casting spell
       //  !exchangeMgr.IsValid() &&
        !tradingStopped &&
        tradingStatus == NOT_TRADING);

}

void psCharacter::MakeTextureString( csString& traits)
{
    // initialize string
    traits = "<traits>";

    // cycle through and add entries for each part
    for (unsigned int i=0;i<PSTRAIT_LOCATION_COUNT;i++)
    {
        psTrait *trait;
        trait = GetTraitForLocation((PSTRAIT_LOCATION)i);
        while (trait != NULL)
        {
            csString buff = trait->ToXML(true);
            traits.Append(buff);
            trait = trait->next_trait;
        }
    }

    // terminate string
    traits.Append("</traits>");

    Notify2( LOG_CHARACTER, "Traits string: %s", (const char*)traits );
}

void psCharacter::MakeEquipmentString( csString& equipment )
{
    equipment = "<equiplist>";
    equipment.AppendFmt("<helm>%s</helm><bracer>%s</bracer><belt>%s</belt><cloak>%s</cloak>", EscpXML(helmGroup).GetData(), EscpXML(BracerGroup).GetData(), EscpXML(BeltGroup).GetData(), EscpXML(CloakGroup).GetData());

    for (int i=0; i<PSCHARACTER_SLOT_BULK1; i++)
    {
        psItem* item = inventory.GetInventoryItem((INVENTORY_SLOT_NUMBER)i);
        if (item == NULL)
            continue;

        csString slot = EscpXML( CacheManager::GetSingleton().slotNameHash.GetName(i) );
        csString mesh = EscpXML( item->GetMeshName() );
        csString part = EscpXML( item->GetPartName() );
        csString texture = EscpXML( item->GetTextureName() );
        csString partMesh = EscpXML( item->GetPartMeshName() );

        equipment.AppendFmt("<equip slot=\"%s\" mesh=\"%s\" part=\"%s\" texture=\"%s\" partMesh=\"%s\"  />",
                              slot.GetData(), mesh.GetData(), part.GetData(), texture.GetData(), partMesh.GetData() );
    }

    equipment.Append("</equiplist>");

    Notify2( LOG_CHARACTER, "Equipment string: %s", equipment.GetData() );
}


bool psCharacter::AppendCharacterSelectData(psAuthApprovedMessage& auth)
{
    csString traits;
    csString equipment;

    MakeTextureString( traits );
    MakeEquipmentString( equipment );

    auth.AddCharacter(fullname, raceinfo->name, raceinfo->mesh_name, traits, equipment);
    return true;
}

QuestAssignment *psCharacter::IsQuestAssigned(int id)
{
    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        if (assigned_quests[i]->GetQuest().IsValid() && assigned_quests[i]->GetQuest()->GetID() == id &&
            assigned_quests[i]->status != PSQUEST_DELETE)
            return assigned_quests[i];
    }

    return NULL;
}

int psCharacter::GetAssignedQuestLastResponse(size_t i)
{
    if (i<assigned_quests.GetSize())
    {
        return assigned_quests[i]->last_response;
    }
    else
    {
        //could use error message
        return -1; //return no response
    }
}


//if (parent of) quest id is an assigned quest, set its last response
bool psCharacter::SetAssignedQuestLastResponse(psQuest *quest, int response, gemObject *npc)
{
    int id = 0;

    if (quest)
    {
        while (quest->GetParentQuest()) //get highest parent
            quest= quest->GetParentQuest();
        id = quest->GetID();
    }
    else
        return false;


    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        if (assigned_quests[i]->GetQuest().IsValid() && assigned_quests[i]->GetQuest()->GetID() == id &&
            assigned_quests[i]->status == PSQUEST_ASSIGNED && !assigned_quests[i]->GetQuest()->GetParentQuest())
        {
            assigned_quests[i]->last_response = response;
            assigned_quests[i]->last_response_from_npc_pid = npc->GetPID();
            assigned_quests[i]->dirty = true;
            UpdateQuestAssignments();
            return true;
        }
    }
    return false;
}

size_t  psCharacter::GetAssignedQuests(psQuestListMessage& questmsg,int cnum)
{
    if (assigned_quests.GetSize() )
    {
        csString quests;
        quests.Append("<quests>");
        csArray<QuestAssignment*>::Iterator iter = assigned_quests.GetIterator();

        while(iter.HasNext())
        {
            QuestAssignment* assigned_quest = iter.Next();
            // exclude deleted
            if (assigned_quest->status == PSQUEST_DELETE || !assigned_quest->GetQuest().IsValid())
                continue;
            // exclude substeps
            if (assigned_quest->GetQuest()->GetParentQuest())
                continue;

            csString item;
            csString escpxml_image = EscpXML(assigned_quest->GetQuest()->GetImage());
            csString escpxml_name = EscpXML(assigned_quest->GetQuest()->GetName());
            item.Format("<q><image icon=\"%s\" /><desc text=\"%s\" /><id text=\"%d\" /><status text=\"%c\" /></q>",
                        escpxml_image.GetData(),
                        escpxml_name.GetData(),
                        assigned_quest->GetQuest()->GetID(),
                        assigned_quest->status );
            quests.Append(item);
        }
        quests.Append("</quests>");
        Debug2(LOG_QUESTS, pid.Unbox(), "QuestMsg was %s\n", quests.GetData());
        questmsg.Populate(quests,cnum);
    }
    return assigned_quests.GetSize();
}

QuestAssignment *psCharacter::AssignQuest(psQuest *quest, PID assigner_id)
{
    CS_ASSERT( quest );  // Must not be NULL

    // Shame on Kayden for cutting and pasting those code from CheckQuestAvailable instead of making a distinct function for it. :)
    /*********************************
    //first check if there is not another assigned quest with the same NPC
    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        if (assigned_quests[i]->GetQuest().IsValid() &&
            assigned_quests[i]->assigner_id == assigner_id &&
            assigned_quests[i]->GetQuest()->GetID() != quest->GetID() &&
            quest->GetParentQuest() == NULL &&
            assigned_quests[i]->GetQuest()->GetParentQuest() == NULL &&
            assigned_quests[i]->status == PSQUEST_ASSIGNED)
        {
            Debug3(LOG_QUESTS, pid.Unbox(), "Did not assign %s quest to %s because (s)he already has a quest assigned with this npc.\n", quest->GetName(), GetCharName());
            return false; // Cannot have multiple quests from the same guy
        }
    }
    ********************/

    QuestAssignment *q = IsQuestAssigned(quest->GetID() );
    if (!q)  // make new entry if needed, reuse if old
    {
        q = new QuestAssignment;
        q->SetQuest(quest);
        q->status = PSQUEST_DELETE;

        assigned_quests.Push(q);
    }

    if (q->status != PSQUEST_ASSIGNED)
    {
        q->dirty  = true;
        q->status = PSQUEST_ASSIGNED;
        q->lockout_end = 0;
        q->assigner_id = assigner_id;
        //set last response to current response only if this is the top parent
        q->last_response = quest->GetParentQuest() ? -1 : this->lastResponse;    //This should be the response given when starting this quest

        // assign any skipped parent quests
        if (quest->GetParentQuest() && !IsQuestAssigned(quest->GetParentQuest()->GetID()))
                AssignQuest(quest->GetParentQuest(),assigner_id );

        // assign any skipped sub quests
        csHash<psQuest*>::GlobalIterator it = CacheManager::GetSingleton().GetQuestIterator();
        while (it.HasNext())
        {
            psQuest * q = it.Next();
            if (q->GetParentQuest())
            {
                if (q->GetParentQuest()->GetID() == quest->GetID())
                    AssignQuest(q,assigner_id);
            }
        }

        q->GetQuest()->SetQuestLastActivatedTime( csGetTicks() / 1000 );

        Debug3(LOG_QUESTS, pid.Unbox(), "Assigned quest '%s' to player '%s'\n", quest->GetName(), GetCharName());
        UpdateQuestAssignments();
    }
    else
    {
        Debug3(LOG_QUESTS, pid.Unbox(), "Did not assign %s quest to %s because it was already assigned.\n", quest->GetName(), GetCharName());
    }

    return q;
}

bool psCharacter::CompleteQuest(psQuest *quest)
{
    CS_ASSERT( quest );  // Must not be NULL

    QuestAssignment *q = IsQuestAssigned( quest->GetID() );
    QuestAssignment *parent = NULL;

    // substeps are not assigned, so the above check fails for substeps.
    // in this case we check if the parent quest is assigned
    if (!q && quest->GetParentQuest()) {
      parent = IsQuestAssigned(quest->GetParentQuest()->GetID() );
    }

    // create an assignment for the substep if parent is valid
    if (parent) {
      q = AssignQuest(quest,parent->assigner_id);
    }

    if (q)
    {
        if (q->status == PSQUEST_DELETE || q->status == PSQUEST_COMPLETE)
        {
            Debug3(LOG_QUESTS, pid.Unbox(), "Player '%s' has already completed quest '%s'.  No credit.\n", GetCharName(), quest->GetName());
            return false;  // already completed, so no credit here
        }

        q->dirty  = true;
        q->status = PSQUEST_COMPLETE; // completed
        q->lockout_end = GetTotalOnlineTime() +
                         q->GetQuest()->GetPlayerLockoutTime();
        q->last_response = -1; //reset last response for this quest in case it is restarted

        // Complete all substeps if this is the parent quest
        if (!q->GetQuest()->GetParentQuest())
        {
            // assign any skipped sub quests
            csHash<psQuest*>::GlobalIterator it = CacheManager::GetSingleton().GetQuestIterator();
            while (it.HasNext())
            {
                psQuest * currQuest = it.Next();
                if (currQuest->GetParentQuest())
                {
                    if (currQuest->GetParentQuest()->GetID() == quest->GetID())
                    {
                        QuestAssignment* currAssignment = IsQuestAssigned(currQuest->GetID());
                        if (currAssignment && currAssignment->status == PSQUEST_ASSIGNED)
                            DiscardQuest(currAssignment, true);
                    }
                }
            }
        }

        Debug3(LOG_QUESTS, pid.Unbox(), "Player '%s' just completed quest '%s'.\n", GetCharName(), quest->GetName());
        UpdateQuestAssignments();
        return true;
    }

    return false;
}

void psCharacter::DiscardQuest(QuestAssignment *q, bool force)
{
    CS_ASSERT( q );  // Must not be NULL

    if (force || (q->status != PSQUEST_DELETE && !q->GetQuest()->HasInfinitePlayerLockout()) )
    {
        q->dirty = true;
        q->status = PSQUEST_DELETE;  // discarded
        if (q->GetQuest()->HasInfinitePlayerLockout())
            q->lockout_end = 0;
        else
            q->lockout_end = GetTotalOnlineTime() +
                             q->GetQuest()->GetPlayerLockoutTime();
            // assignment entry will be deleted after expiration

        Debug3(LOG_QUESTS, pid.Unbox(), "Player '%s' just discarded quest '%s'.\n",
               GetCharName(),q->GetQuest()->GetName() );

        UpdateQuestAssignments();
    }
    else
    {
        Debug3(LOG_QUESTS, pid.Unbox(),
               "Did not discard %s quest for player %s because it was already discarded or was a one-time quest.\n",
               q->GetQuest()->GetName(),GetCharName() );
        // Notify the player that he can't discard one-time quests
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(),
            "You can't discard this quest, since it can be done just once!");
    }
}

bool psCharacter::CheckQuestAssigned(psQuest *quest)
{
    CS_ASSERT( quest );  // Must not be NULL
    QuestAssignment* questAssignment = IsQuestAssigned( quest->GetID() );
    if ( questAssignment )
    {
        if ( questAssignment->status == PSQUEST_ASSIGNED)
            return true;
    }
    return false;
}

bool psCharacter::CheckQuestCompleted(psQuest *quest)
{
    CS_ASSERT( quest );  // Must not be NULL
    QuestAssignment* questAssignment = IsQuestAssigned( quest->GetID());
    if ( questAssignment )
    {
        if ( questAssignment->status == PSQUEST_COMPLETE)
            return true;
    }
    return false;
}

//This incorrectly named function checks if the npc (assigner_id) is supposed to answer
// in the (parent)quest at this moment.
bool psCharacter::CheckQuestAvailable(psQuest *quest, PID assigner_id)
{
    CS_ASSERT( quest );  // Must not be NULL

    unsigned int now = csGetTicks() / 1000;

    if (quest->GetParentQuest())
    {
        quest = quest->GetParentQuest();
    }

    bool notify = false;
    if (GetActor()->GetClient())
    {
        notify = CacheManager::GetSingleton().GetCommandManager()->Validate(GetActor()->GetClient()->GetSecurityLevel(), "quest notify");
    }

    //NPC should always answer, if the quest is assigned, no matter who started the quest.
    QuestAssignment *q = IsQuestAssigned(quest->GetID());
    if (q && q->status == PSQUEST_ASSIGNED)
    {
        return true;
    }

    //Since the quest is not assigned, this conversation will lead to starting the quest.
    //Check all assigned quests, to make sure there is no other quest already started by this NPC
    /*****
    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        if (assigned_quests[i]->GetQuest().IsValid() && assigned_quests[i]->assigner_id == assigner_id &&
            assigned_quests[i]->GetQuest()->GetID() != quest->GetID() &&
            assigned_quests[i]->GetQuest()->GetParentQuest() == NULL &&
            assigned_quests[i]->status == PSQUEST_ASSIGNED)
        {
            if (notify)
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(),
                                         "GM NOTICE: Quest found, but you already have one assigned from same NPC");
            }

            return false; // Cannot have multiple quests from the same guy
        }
    }
    ********/

    if (q) //then quest in assigned list, but not PSQUEST_ASSIGNED
    {
        // Character has this quest in completed list. Check if still in lockout
        if ( q->GetQuest()->HasInfinitePlayerLockout() ||
             q->lockout_end > GetTotalOnlineTime() )
        {
            if (notify)
            {
                if (GetActor()->questtester) // GM flag
                {
                    psserver->SendSystemInfo(GetActor()->GetClientID(),
                        "GM NOTICE: Quest (%s) found and player lockout time has been overridden.",
                        quest->GetName());
                    return true; // Quest is available for GM
                }
                else
                {
                    if (q->GetQuest()->HasInfinitePlayerLockout())
                    {
                        psserver->SendSystemInfo(GetActor()->GetClientID(),
                            "GM NOTICE: Quest (%s) found but quest has infinite player lockout.",
                            quest->GetName());
                    }
                    else
                    {
                        psserver->SendSystemInfo(GetActor()->GetClientID(),
                            "GM NOTICE: Quest (%s) found but player lockout time hasn't elapsed yet. %d seconds remaining.",
                            quest->GetName(), q->lockout_end - GetTotalOnlineTime() );
                    }

                }

            }

            return false; // Cannot have the same quest while in player lockout time.
        }
    }

    // If here, quest is not in assigned_quests, or it is completed and not in player lockout time
    // Player is allowed to start this quest, now check if quest has a lockout
    if (quest->GetQuestLastActivatedTime() &&
        (quest->GetQuestLastActivatedTime() + quest->GetQuestLockoutTime() > now))
    {
        if (notify)
        {
            if (GetActor()->questtester) // GM flag
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(),
                                         "GM NOTICE: Quest(%s) found; quest lockout time has been overrided",
                                         quest->GetName());
                return true; // Quest is available for GM
            }
            else
                psserver->SendSystemInfo(GetActor()->GetClientID(),
                                         "GM NOTICE: Quest(%s) found, but quest lockout time hasn't elapsed yet. %d seconds remaining.",
                                         quest->GetName(),quest->GetQuestLastActivatedTime()+quest->GetQuestLockoutTime() - now);
        }

        return false; // Cannot start this quest while in quest lockout time.
    }

    return true; // Quest is available
}

bool psCharacter::CheckResponsePrerequisite(NpcResponse *resp)
{
    CS_ASSERT( resp );  // Must not be NULL

    return resp->CheckPrerequisite(this);
}

int psCharacter::NumberOfQuestsCompleted(csString category)
{
    int count=0;
    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        // Character have this quest
        if (assigned_quests[i]->GetQuest().IsValid() && assigned_quests[i]->GetQuest()->GetParentQuest() == NULL &&
            assigned_quests[i]->status == PSQUEST_COMPLETE &&
            assigned_quests[i]->GetQuest()->GetCategory() == category)
        {
            count++;
        }
    }
    return count;
}

bool psCharacter::UpdateQuestAssignments(bool force_update)
{
    for (size_t i=0; i<assigned_quests.GetSize(); i++)
    {
        QuestAssignment *q = assigned_quests[i];
        if (q->GetQuest().IsValid() && (q->dirty || force_update))
        {
            int r;

            // will delete the quest only after the expiration time, so the player cannot get it again immediately
            // If it's a step, We can delete it even though it has inf lockout
            if (q->status == PSQUEST_DELETE &&
                ((!q->GetQuest()->HasInfinitePlayerLockout() &&
                (!q->GetQuest()->GetPlayerLockoutTime() || !q->lockout_end ||
                 (q->lockout_end < GetTotalOnlineTime()))) ||
                 q->GetQuest()->GetParentQuest()))   // delete
            {
                r = db->CommandPump("DELETE FROM character_quests WHERE player_id=%d AND quest_id=%d",
                                    pid.Unbox(), q->GetQuest()->GetID());

                delete assigned_quests[i];
                assigned_quests.DeleteIndex(i);
                i--;  // reincremented in loop
                continue;
            }

            // Update or create a new entry in DB

            if(!q->dirty)
                continue;


            db->CommandPump("insert into character_quests "
                            "(player_id, assigner_id, quest_id, "
                            "status, remaininglockout, last_response, last_response_npc_id) "
                            "values (%d, %d, %d, '%c', %d, %d, %d) "
                            "ON DUPLICATE KEY UPDATE "
                            "status='%c',remaininglockout=%ld,last_response=%ld,last_response_npc_id=%ld;",
                            pid.Unbox(),
                            q->assigner_id.Unbox(),
                            q->GetQuest()->GetID(),
                            q->status,
                            q->lockout_end,
                            q->last_response,
                            q->last_response_from_npc_pid.Unbox(),
                            q->status,
                            q->lockout_end,
                            q->last_response,
                            q->last_response_from_npc_pid.Unbox());
            Debug3(LOG_QUESTS, pid.Unbox(), "Updated quest info for player %d, quest %d.\n", pid.Unbox(), assigned_quests[i]->GetQuest()->GetID());
            assigned_quests[i]->dirty = false;
        }
    }
    return true;
}


bool psCharacter::LoadQuestAssignments()
{
    Result result(db->Select("SELECT * FROM character_quests WHERE player_id=%u", pid.Unbox()));
    if (!result.IsValid())
    {
        Error3("Could not load quest assignments for character %u. Error was: %s", pid.Unbox(), db->GetLastError());
        return false;
    }

    unsigned int age = GetTotalOnlineTime();

    for (unsigned int i=0; i<result.Count(); i++)
    {
        QuestAssignment *q = new QuestAssignment;
        q->dirty = false;
        q->SetQuest(CacheManager::GetSingleton().GetQuestByID( result[i].GetInt("quest_id") ) );
        q->status        = result[i]["status"][0];
        q->lockout_end   = result[i].GetInt("remaininglockout");
        q->assigner_id   = PID(result[i].GetInt("assigner_id"));
        q->last_response = result[i].GetInt("last_response");
        q->last_response_from_npc_pid = PID(result[i].GetInt("last_response_npc_id"));

        if (!q->GetQuest())
        {
            Error3("Quest %d for player %d not found!", result[i].GetInt("quest_id"), pid.Unbox());
            delete q;
            return false;
        }

        // Sanity check to see if time for completion is withing
        // lockout time.
        if (q->lockout_end > age + q->GetQuest()->GetPlayerLockoutTime())
            q->lockout_end = age + q->GetQuest()->GetPlayerLockoutTime();

        Debug6(LOG_QUESTS, pid.Unbox(), "Loaded quest %-40.40s, status %c, lockout %lu, last_response %d, for player %s.\n",
               q->GetQuest()->GetName(),q->status,
               ( q->lockout_end > age ? q->lockout_end-age:0),q->last_response, GetCharFullName());
        assigned_quests.Push(q);
    }
    return true;
}

psGuildLevel * psCharacter::GetGuildLevel()
{
    if (guildinfo == NULL)
        return 0;

    psGuildMember * membership = guildinfo->FindMember(pid);
    if (membership == NULL)
        return 0;

    return membership->guildlevel;
}

psGuildMember * psCharacter::GetGuildMembership()
{
    if (guildinfo == NULL)
        return 0;

    return guildinfo->FindMember(pid);
}

bool psCharacter::Knows(PID charID)
{
    // Introduction system is currently disabled - it's trivially worked
    // around, and simply alienating players.
    //return acquaintances.Contains(charID);
    return true;
}

bool psCharacter::Introduce(psCharacter *c)
{
    if (!c) return false;
    PID theirID = c->GetPID();

    if (!acquaintances.Contains(theirID))
    {
        acquaintances.AddNoTest(theirID);
        db->CommandPump("insert into introductions values(%d, %d)", this->pid.Unbox(), theirID.Unbox());
        return true;
    }
    return false;
}

bool psCharacter::Unintroduce(psCharacter *c)
{
    if (!c) return false;
    PID theirID = c->GetPID();

    if (acquaintances.Contains(theirID))
    {
        acquaintances.Delete(theirID);
        db->CommandPump("delete from introductions where charid=%d and introcharid=%d", this->pid.Unbox(), theirID.Unbox());
        return true;
    }
    return false;
}

void psCharacter::RemoveBuddy(PID buddyID)
{
    for ( size_t x = 0; x < buddyList.GetSize(); x++ )
    {
        if ( buddyList[x].playerID == buddyID )
        {
            buddyList.DeleteIndex(x);
            return;
        }
    }
}

void psCharacter::BuddyOf(PID buddyID)
{
    if ( buddyOfList.Find( buddyID )  == csArrayItemNotFound )
    {
        buddyOfList.Push( buddyID );
    }
}

void psCharacter::NotBuddyOf(PID buddyID)
{
    buddyOfList.Delete( buddyID );
}

bool psCharacter::IsBuddy(PID buddyID)
{
    for ( size_t x = 0; x < buddyList.GetSize(); x++ )
    {
        if ( buddyList[x].playerID == buddyID )
        {
            return true;
        }
    }
    return false;
}

bool psCharacter::AddBuddy(PID buddyID, csString & buddyName)
{
    // Cannot addself to buddy list
    if (buddyID == pid)
        return false;

    for ( size_t x = 0; x < buddyList.GetSize(); x++ )
    {
        if ( buddyList[x].playerID == buddyID )
        {
            return true;
        }
    }

    Buddy b;
    b.name = buddyName;
    b.playerID = buddyID;

    buddyList.Push( b );

    return true;
}

bool psCharacter::AddExploredArea(PID explored)
{
    int rows = db->Command("INSERT INTO character_relationships (character_id, related_id, relationship_type) VALUES (%u, %u, 'exploration')",
        pid.Unbox(), explored.Unbox());

    if (rows != 1)
    {
        psserver->GetDatabase()->SetLastError(psserver->GetDatabase()->GetLastSQLError());
        return false;
    }

    explored_areas.Push(explored);

    return true;
}

bool psCharacter::HasExploredArea(PID explored)
{
    for(size_t i=0; i<explored_areas.GetSize(); ++i)
    {
        if(explored == explored_areas[i])
            return true;
    }

    return false;
}

double psCharacter::GetProperty(const char *ptr)
{
    if (!strcasecmp(ptr,"AttackerTargeted"))
    {
        return true;
        // return (attacker_targeted) ? 1 : 0;
    }
    else if (!strcasecmp(ptr,"TotalTargetedBlockValue"))
    {
        return GetTotalTargetedBlockValue();
    }
    else if (!strcasecmp(ptr,"TotalUntargetedBlockValue"))
    {
        return GetTotalUntargetedBlockValue();
    }
    else if (!strcasecmp(ptr,"DodgeValue"))
    {
        return GetDodgeValue();
    }
    else if (!strcasecmp(ptr,"KillExp"))
    {
        return kill_exp;
    }
    else if (!strcasecmp(ptr,"getAttackValueModifier"))
    {
        return attackModifier.Value();
    }
    else if (!strcasecmp(ptr,"getDefenseValueModifier"))
    {
        return defenseModifier.Value();
    }
    else if (!strcasecmp(ptr,"HP"))
    {
        return GetHP();
    }
    else if (!strcasecmp(ptr,"MaxHP"))
    {
        return GetMaxHP().Current();
    }
    else if (!strcasecmp(ptr,"BaseHP"))
    {
        return GetMaxHP().Base();
    }
    else if (!strcasecmp(ptr,"Mana"))
    {
        return GetMana();
    }
    else if (!strcasecmp(ptr,"MaxMana"))
    {
        return GetMaxMana().Current();
    }
    else if (!strcasecmp(ptr,"BaseMana"))
    {
        return GetMaxMana().Base();
    }
    else if (!strcasecmp(ptr,"PStamina"))
    {
        return GetStamina(true);
    }
    else if (!strcasecmp(ptr,"MStamina"))
    {
        return GetStamina(false);
    }
    else if (!strcasecmp(ptr,"MaxPStamina"))
    {
        return GetMaxPStamina().Current();
    }
    else if (!strcasecmp(ptr,"MaxMStamina"))
    {
        return GetMaxMStamina().Current();
    }
    else if (!strcasecmp(ptr,"BasePStamina"))
    {
        return GetMaxPStamina().Base();
    }
    else if (!strcasecmp(ptr,"BaseMStamina"))
    {
        return GetMaxMStamina().Base();
    }
    else if (!strcasecmp(ptr,"Strength"))
    {
        return attributes[PSITEMSTATS_STAT_STRENGTH].Current();
    }
    else if (!strcasecmp(ptr,"Agility"))
    {
        return attributes[PSITEMSTATS_STAT_AGILITY].Current();
    }
    else if (!strcasecmp(ptr,"Endurance"))
    {
        return attributes[PSITEMSTATS_STAT_ENDURANCE].Current();
    }
    else if (!strcasecmp(ptr,"Intelligence"))
    {
        return attributes[PSITEMSTATS_STAT_INTELLIGENCE].Current();
    }
    else if (!strcasecmp(ptr,"Will"))
    {
        return attributes[PSITEMSTATS_STAT_WILL].Current();
    }
    else if (!strcasecmp(ptr,"Charisma"))
    {
        return attributes[PSITEMSTATS_STAT_CHARISMA].Current();
    }
    else if (!strcasecmp(ptr,"BaseStrength"))
    {
        return attributes[PSITEMSTATS_STAT_STRENGTH].Base();
    }
    else if (!strcasecmp(ptr,"BaseAgility"))
    {
        return attributes[PSITEMSTATS_STAT_AGILITY].Base();
    }
    else if (!strcasecmp(ptr,"BaseEndurance"))
    {
        return attributes[PSITEMSTATS_STAT_ENDURANCE].Base();
    }
    else if (!strcasecmp(ptr,"BaseIntelligence"))
    {
        return attributes[PSITEMSTATS_STAT_INTELLIGENCE].Base();
    }
    else if (!strcasecmp(ptr,"BaseWill"))
    {
        return attributes[PSITEMSTATS_STAT_WILL].Base();
    }
    else if (!strcasecmp(ptr,"BaseCharisma"))
    {
        return attributes[PSITEMSTATS_STAT_CHARISMA].Base();
    }
    else if (!strcasecmp(ptr,"AllArmorStrMalus"))
    {
        return modifiers[PSITEMSTATS_STAT_STRENGTH].Current();
    }
    else if (!strcasecmp(ptr,"AllArmorAgiMalus"))
    {
        return modifiers[PSITEMSTATS_STAT_AGILITY].Current();
    }
    else if (!strcasecmp(ptr,"PID"))
    {
        return (double) pid.Unbox();
    }
    else if (!strcasecmp(ptr,"loc_x"))
    {
        return location.loc.x;
    }
    else if (!strcasecmp(ptr,"loc_y"))
    {
        return location.loc.y;
    }
    else if (!strcasecmp(ptr,"loc_z"))
    {
        return location.loc.z;
    }
    else if (!strcasecmp(ptr,"sector"))
    {
        return location.loc_sector->uid;
    }
    else if (!strcasecmp(ptr,"owner"))
    {
        return (double) owner_id.Unbox();
    }

    Error2("Requested psCharacter property not found '%s'", ptr);
    return 0;
}

double psCharacter::CalcFunction(const char * functionName, const double * params)
{
    if (!strcasecmp(functionName, "HasCompletedQuest"))
    {
        const char *questName = MathScriptEngine::GetString(params[0]);
        psQuest *quest = CacheManager::GetSingleton().GetQuestByName(questName);
        return (double) CheckQuestCompleted(quest);
    }
    else if (!strcasecmp(functionName, "GetStatValue"))
    {
        PSITEMSTATS_STAT stat = (PSITEMSTATS_STAT)(int)params[0];

        return (double) attributes[stat].Current();
    }
    else if (!strcasecmp(functionName, "GetAverageSkillValue"))
    {
        PSSKILL skill1 = (PSSKILL)(int)params[0];
        PSSKILL skill2 = (PSSKILL)(int)params[1];
        PSSKILL skill3 = (PSSKILL)(int)params[2];

        double v1 = skills.GetSkillRank(skill1).Current();

        if (skill2!=PSSKILL_NONE) {
            double v2 = skills.GetSkillRank(skill2).Current();
            v1 = (v1+v2)/2;
        }

        if (skill3!=PSSKILL_NONE) {
            double v3 = skills.GetSkillRank(skill3).Current();
            v1 = (v1+v3)/2;
        }

        // always give a small % of combat skill, or players will never be able to get the first exp
        if (v1==0)
            v1 = 0.7;

        return v1;
    }
    else if (!strcasecmp(functionName, "SkillRank"))
    {
        const char *skillName = MathScriptEngine::GetString(params[0]);
        PSSKILL skill = CacheManager::GetSingleton().ConvertSkillString(skillName);
        double value = skills.GetSkillRank(skill).Current();

        // always give a small % of melee (unharmed) skill
        if (skill == PSSKILL_MARTIALARTS && value == 0)
            value = 0.2;

        return value;
    }
    else if (!strcasecmp(functionName, "GetSkillValue"))
    {
        // Deprecated in favor of SkillRank, which takes a string literal.
        Warning2(LOG_CHARACTER, "MathScript: Character:GetSkillValue(%d) is deprecated.", (int) params[0]);
        PSSKILL skill = (PSSKILL)(int)params[0];

        double value = skills.GetSkillRank(skill).Current();

        // always give a small % of melee (unharmed) skill
        if (skill==PSSKILL_MARTIALARTS && value==0)
            value = 0.2;

        return value;
    }
    else if (!strcasecmp(functionName, "HasExploredArea"))
    {
        if(!HasExploredArea(params[0]))
        {
            AddExploredArea(params[0]);
            return 0;
        }

        return 1;
    }
    else if (!strcasecmp(functionName, "IsWithin"))
    {
        if(location.loc_sector->uid != params[4])
            return 0.0;

        csVector3 other(params[1], params[2], params[3]);
        return (csVector3(other - location.loc).Norm() <= params[0]) ? 1.0 : 0.0;
    }
    else if (!strcasecmp(functionName, "IsEnemy"))
    {
        // Check for self.
        if(owner_id == params[0])
            return 0.0;

        Client* owner = EntityManager::GetSingleton().GetClients()->FindPlayer(params[0]);
        if(owner->GetTargetType(GetActor()) == TARGET_FOE)
        {
            return 1.0;
        }

        return 0.0;
    }

    CPrintf(CON_ERROR, "psItem::CalcFunction(%s) failed\n", functionName);
    return 0;
}

/** A skill can only be trained if the player requires points for it.
  */
bool psCharacter::CanTrain( PSSKILL skill )
{
    return skills.CanTrain( skill );
}

void psCharacter::Train( PSSKILL skill, int yIncrease )
{
    // Did we train stats?
    PSITEMSTATS_STAT stat = skillToStat(skill);
    if (stat != PSITEMSTATS_STAT_NONE)
    {
        Skill & cskill = skills.Get(skill);

        skills.Train( skill, yIncrease );
        int know = cskill.y;
        int cost = cskill.yCost;

        // We ranked up
        if(know >= cost)
        {
            cskill.rank.SetBase(cskill.rank.Base()+1);
            cskill.y = 0;
            cskill.CalculateCosts(this);

            // Insert into the stats
            attributes[stat].SetBase(cskill.rank.Base());

            // Save stats
            if(GetActor()->GetClientID() != 0)
            {
                const char *fieldnames[]= {
                   "base_strength",
                   "base_agility",
                   "base_endurance",
                   "base_intelligence",
                   "base_will",
                   "base_charisma"
                    };
                psStringArray fieldvalues;
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_STRENGTH].Base());
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_AGILITY].Base());
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_ENDURANCE].Base());
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_INTELLIGENCE].Base());
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_WILL].Base());
                fieldvalues.FormatPush("%d", attributes[PSITEMSTATS_STAT_CHARISMA].Base());

                csString id((size_t) pid.Unbox());

                if(!db->GenericUpdateWithID("characters","id",id,fieldnames,fieldvalues))
                {
                    Error2("Couldn't save stats for character %u!\n", pid.Unbox());
                }
            }
        }
    }
    else
    {
        skills.Train( skill, yIncrease ); // Normal training
        psServer::CharacterLoader.UpdateCharacterSkill(
                pid,
                skill,
                skills.GetSkillPractice((PSSKILL)skill),
                skills.GetSkillKnowledge((PSSKILL)skill),
                skills.GetSkillRank((PSSKILL)skill).Base()
                );
    }
}

/*-----------------------------------------------------------------*/


void Skill::CalculateCosts(psCharacter* user)
{
    if(!info || !user)
        return;

    // Calc the new Y/Z cost
    csString scriptName;
    if (info->id < PSSKILL_AGI || info->id > PSSKILL_WILL)
        scriptName = "CalculateSkillCosts";
    else
        scriptName = "CalculateStatCosts";

    MathScript *script = psserver->GetMathScriptEngine()->FindScript(scriptName);
    if (!script)
    {
        Error2("Couldn't find script %s!", scriptName.GetData());
        return;
    }

    MathEnvironment env;
    env.Define("BaseCost",       info->baseCost);
    env.Define("SkillRank",      rank.Base());
    env.Define("SkillID",        info->id);
    env.Define("PracticeFactor", info->practice_factor);
    env.Define("MentalFactor",   info->mental_factor);
    env.Define("Actor",          user);

    script->Evaluate(&env);

    MathVar *yCostVar = env.Lookup("YCost");
    MathVar *zCostVar = env.Lookup("ZCost");
    if (!yCostVar || !zCostVar)
    {
        Error2("Failed to evaluate MathScript >%s<.", scriptName.GetData());
        return;
    }

    // Get the output
    yCost = (int)yCostVar->GetValue();
    zCost = (int)zCostVar->GetValue();

    //calculate the next level costs. Used by the CalculateAddExperience.
    env.Define("SkillRank",      rank.Base()+1);
    script->Evaluate(&env);

    yCostNext = (int)yCostVar->GetValue();
    zCostNext = (int)zCostVar->GetValue();

/*
    // Make sure the y values is clamped to the cost.  Otherwise Practice may always
    // fail.
    if  (y > yCost)
    {
        dirtyFlag = true;
        y = yCost;
    }
    if ( z > zCost )
    {
        dirtyFlag = true;
        z = zCost;
    }
*/
}

void Skill::Train( int yIncrease )
{
    if(y < yCost)
    {
        y+=yIncrease;
        dirtyFlag = true;
    }
}


bool Skill::Practice( unsigned int amount, unsigned int& actuallyAdded,psCharacter* user )
{
    bool rankup = false;
    // Practice can take place
    if ( y >= yCost )
    {
        z+=amount;
        if ( z >= zCost )
        {
            rank.SetBase(rank.Base()+1);
            z = 0;
            y = 0;
            actuallyAdded = z - zCost;
            rankup = true;
            // Reset the costs for Y/Z
            CalculateCosts(user);
        }
        else
        {
            actuallyAdded = amount;
        }
    }
    else
    {
        actuallyAdded = 0;
    }

    dirtyFlag = true;
    return rankup;
}

void psCharacter::SetSkillRank(PSSKILL which, unsigned int rank)
{
    if (rank < 0)
        rank = 0;

    skills.SetSkillRank(which, rank);

    if (which == PSSKILL_AGI)
        attributes[PSITEMSTATS_STAT_AGILITY].SetBase(rank);
    else if (which == PSSKILL_CHA)
        attributes[PSITEMSTATS_STAT_CHARISMA].SetBase(rank);
    else if (which == PSSKILL_END)
        attributes[PSITEMSTATS_STAT_ENDURANCE].SetBase(rank);
    else if (which == PSSKILL_INT)
        attributes[PSITEMSTATS_STAT_INTELLIGENCE].SetBase(rank);
    else if (which == PSSKILL_STR)
        attributes[PSITEMSTATS_STAT_STRENGTH].SetBase(rank);
    else if (which == PSSKILL_WILL)
        attributes[PSITEMSTATS_STAT_WILL].SetBase(rank);
}

unsigned int psCharacter::GetCharLevel(bool physical)
{
    if(physical)
    {
        return (attributes[PSITEMSTATS_STAT_STRENGTH].Current()     +
            attributes[PSITEMSTATS_STAT_ENDURANCE].Current()    +
            attributes[PSITEMSTATS_STAT_AGILITY].Current()) / 3;
    }
    else
    {
        return (attributes[PSITEMSTATS_STAT_INTELLIGENCE].Current() +
            attributes[PSITEMSTATS_STAT_WILL].Current()         +
            attributes[PSITEMSTATS_STAT_CHARISMA].Current()) / 3;
    }
}

//This function recalculates Hp, Mana, and Stamina when needed (char creation, combats, training sessions)
void psCharacter::RecalculateStats()
{
    MathEnvironment env; // safe enough to reuse...and faster...
    env.Define("Actor", this);

    // Calculate current Max Mana level:
    static MathScript *maxManaScript;
    if (!maxManaScript)
    {
        maxManaScript = psserver->GetMathScriptEngine()->FindScript("CalculateMaxMana");
        CS_ASSERT(maxManaScript != NULL);
    }


    if (override_max_mana)
    {
        GetMaxMana().SetBase(override_max_mana);
    }
    else if (maxManaScript)
    {
        maxManaScript->Evaluate(&env);
        MathVar* maxMana = env.Lookup("MaxMana");
        if (maxMana)
        {
            GetMaxMana().SetBase(maxMana->GetValue());
        }
        else
        {
            Error1("Failed to evaluate MathScript >CalculateMaxMana<.");
        }
    }

    // Calculate current Max HP level:
    static MathScript *maxHPScript;

    if (!maxHPScript)
    {
        maxHPScript = psserver->GetMathScriptEngine()->FindScript("CalculateMaxHP");
        CS_ASSERT(maxHPScript != NULL);
    }

    if (override_max_hp)
    {
        GetMaxHP().SetBase(override_max_hp);
    }
    else if (maxHPScript)
    {
        maxHPScript->Evaluate(&env);
        MathVar* maxHP = env.Lookup("MaxHP");
        GetMaxHP().SetBase(maxHP->GetValue());
    }

    // The max weight that a player can carry
    inventory.CalculateLimits();

    // Stamina
    CalculateMaxStamina();

    // Speed
//    if (GetActor())
//        GetActor()->UpdateAllSpeedModifiers();
}

size_t psCharacter::GetAssignedGMEvents(psGMEventListMessage& gmeventsMsg, int clientnum)
{
    // GM events consist of events ran by the GM (running & completed) and
    // participated in (running & completed).
    size_t numberOfEvents = 0;
    csString gmeventsStr, event, name, desc;
    gmeventsStr.Append("<gmevents>");
    GMEventStatus eventStatus;

    // XML: <event><name text><role text><status text><id text></event>
    if (assigned_events.runningEventIDAsGM >= 0)
    {
        eventStatus = psserver->GetGMEventManager()->GetGMEventDetailsByID(assigned_events.runningEventIDAsGM,
                                                                           name,
                                                                           desc);
        event.Format("<event><name text=\"%s\" /><role text=\"*\" /><status text=\"R\" /><id text=\"%d\" /></event>",
                     name.GetData(), assigned_events.runningEventIDAsGM);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }
    if (assigned_events.runningEventID >= 0)
    {
        eventStatus = psserver->GetGMEventManager()->GetGMEventDetailsByID(assigned_events.runningEventID,
                                                                           name,
                                       desc);
        event.Format("<event><name text=\"%s\" /><role text=\" \" /><status text=\"R\" /><id text=\"%d\" /></event>",
                     name.GetData(), assigned_events.runningEventID);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    csArray<int>::Iterator iter = assigned_events.completedEventIDsAsGM.GetIterator();
    while(iter.HasNext())
    {
        int gmEventIDAsGM = iter.Next();
        eventStatus = psserver->GetGMEventManager()->GetGMEventDetailsByID(gmEventIDAsGM,
                                                                           name,
                                                                           desc);
        event.Format("<event><name text=\"%s\" /><role text=\"*\" /><status text=\"C\" /><id text=\"%d\" /></event>",
                     name.GetData(), gmEventIDAsGM);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    csArray<int>::Iterator iter2 = assigned_events.completedEventIDs.GetIterator();
    while(iter2.HasNext())
    {
        int gmEventID = iter2.Next();
        eventStatus = psserver->GetGMEventManager()->GetGMEventDetailsByID(gmEventID,
                                                                           name,
                                                                           desc);
        event.Format("<event><name text=\"%s\" /><role text=\" \" /><status text=\"C\" /><id text=\"%d\" /></event>",
                     name.GetData(), gmEventID);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    gmeventsStr.Append("</gmevents>");

    if (numberOfEvents)
    gmeventsMsg.Populate(gmeventsStr, clientnum);

    return numberOfEvents;
}

void psCharacter::AssignGMEvent(int id, bool playerIsGM)
{
    if (playerIsGM)
        assigned_events.runningEventIDAsGM = id;
    else
        assigned_events.runningEventID = id;
}

void psCharacter::CompleteGMEvent(bool playerIsGM)
{
    if (playerIsGM)
    {
        assigned_events.completedEventIDsAsGM.Push(assigned_events.runningEventIDAsGM);
    assigned_events.runningEventIDAsGM = -1;
    }
    else
    {
        assigned_events.completedEventIDs.Push(assigned_events.runningEventID);
        assigned_events.runningEventID = -1;
    }
}

void psCharacter::RemoveGMEvent(int id, bool playerIsGM)
{
    if (playerIsGM)
    {
        if (assigned_events.runningEventIDAsGM == id)
            assigned_events.runningEventIDAsGM = -1;
        else
            assigned_events.completedEventIDsAsGM.Delete(id);
    }
    else
    {
        if (assigned_events.runningEventID == id)
            assigned_events.runningEventID = -1;
        else
            assigned_events.completedEventIDs.Delete(id);
    }
}


bool psCharacter::UpdateFaction(Faction * faction, int delta)
{
    if (!GetActor())
    {
        return false;
    }

    GetActor()->GetFactions()->UpdateFactionStanding(faction->id,delta);
    if (delta > 0)
    {
        psserver->SendSystemInfo(GetActor()->GetClientID(),"Your faction with %s has improved.",faction->name.GetData());
    }
    else
    {
        psserver->SendSystemInfo(GetActor()->GetClientID(),"Your faction with %s has worsened.",faction->name.GetData());
    }


    psFactionMessage factUpdate( GetActor()->GetClientID(), psFactionMessage::MSG_UPDATE);
    int standing;
    float weight;
    GetActor()->GetFactions()->GetFactionStanding(faction->id, standing ,weight);

    factUpdate.AddFaction(faction->name, standing);
    factUpdate.BuildMsg();
    factUpdate.SendMessage();

    return true;
}

bool psCharacter::CheckFaction(Faction * faction, int value)
{
    if (!GetActor()) return false;

    return GetActor()->GetFactions()->CheckFaction(faction,value);
}

const char* psCharacter::GetDescription()
{
    return description.GetDataSafe();
}

void psCharacter::SetDescription(const char* newValue)
{
    description = newValue;
    bool bChanged = false;
    while (description.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        description.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if (bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! Description trimmed.");
}

const char* psCharacter::GetOOCDescription()
{
    return oocdescription.GetDataSafe();
}

void psCharacter::SetOOCDescription(const char* newValue)
{
    oocdescription = newValue;
    bool bChanged = false;
    while (description.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        description.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if (bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! Description trimmed.");
}

//returns the stored char creation info of the player
const char* psCharacter::GetCreationInfo()
{
    return creationinfo.GetDataSafe();
}

void psCharacter::SetCreationInfo(const char* newValue)
{
    creationinfo = newValue;
    bool bChanged = false;
    while (creationinfo.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        creationinfo.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if (bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! creation info trimmed.");
}

//Generates and returns the dynamic life events from factions
bool psCharacter::GetFactionEventsDescription(csString & factionDescription)
{
    //iterates all the various factions in this character
    csHash<FactionStanding*, int>::GlobalIterator iter(GetActor()->GetFactions()->GetStandings().GetIterator());
    while(iter.HasNext())
    {
        FactionStanding* standing = iter.Next();
        int score = 0; //used to store the current score
        if(standing->score >= 0) //positive factions
        {
            csHash<FactionLifeEvent*, int>::GlobalIterator scoreIter = standing->faction->PositiveFactionEvents.GetIterator();
            score = standing->score;
            while (scoreIter.HasNext())
            {
                FactionLifeEvent* lifevt = scoreIter.Next();
                if(score > lifevt->value) //check if the score is enough to attribuite this life event
                {
                    factionDescription += lifevt->event_description; //add the life event to the description
                    factionDescription += "\n";
                    break; //nothing else to do as we found what we needed so bail out
                }
            }
        }
        else //negative factions
        {
            csHash<FactionLifeEvent*, int>::GlobalIterator scoreIter = standing->faction->NegativeFactionEvents.GetIterator();
            score = abs(standing->score); //we store values as positive to make things easier and faster so take the
                                          //absolute value
            while (scoreIter.HasNext())
            {
                FactionLifeEvent* lifevt = scoreIter.Next();
                if(score >= lifevt->value)
                {
                    factionDescription += lifevt->event_description;
                    factionDescription += "\n";
                    break;
                }
            }
        }
    }
    return (factionDescription.Length() > 0); //if the string contains something it means some events were attribuited
}

//returns the stored custom life event info of the player
const char* psCharacter::GetLifeDescription()
{
    return lifedescription.GetDataSafe();
}



void psCharacter::SetLifeDescription(const char* newValue)
{
    lifedescription = newValue;
    bool bChanged = false;
    while (lifedescription.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        lifedescription.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if (bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! custom life events trimmed.");
}

//TODO: Make this not return a temp csString, but fix in place
csString NormalizeCharacterName(const csString & name)
{
    csString normName = name;
    normName.Downcase();
    normName.Trim();
    if (normName.Length() > 0)
        normName.SetAt(0,toupper(normName.GetAt(0)));
    return normName;
}

void SkillStatBuffable::OnChange()
{
    chr->RecalculateStats();
}

void CharStat::SetBase(int x)
{
    SkillStatBuffable::SetBase(x);
    chr->Skills().Calculate();
}

StatSet::StatSet(psCharacter *self) : CharacterAttribute(self)
{
    for (int i = 0; i < PSITEMSTATS_STAT_COUNT; i++)
    {
        stats[i].Initialize(self);
    }
}

CharStat & StatSet::Get(PSITEMSTATS_STAT which)
{
    CS_ASSERT(which >= 0 && which < PSITEMSTATS_STAT_COUNT);
    return stats[which];
}

int SkillSet::AddSkillPractice(PSSKILL skill, unsigned int val)
{
    unsigned int added;
    bool rankUp;
    csString name;

    rankUp = AddToSkillPractice(skill,val, added);

    psSkillInfo* skillInfo = CacheManager::GetSingleton().GetSkillByID(skill);

    if ( skillInfo )
    {
        // Save skill and practice only when the level reached a new rank
        // this is done to avoid saving to db each time a player hits
        // an opponent
        if(rankUp && self->GetActor()->GetClientID() != 0)
        {
            psServer::CharacterLoader.UpdateCharacterSkill(
                self->GetPID(),
                skill,
                GetSkillPractice((PSSKILL)skill),
                GetSkillKnowledge((PSSKILL)skill),
                GetSkillRank((PSSKILL)skill).Base()
                );
        }


        name = skillInfo->name;
        Debug5(LOG_SKILLXP,self->GetActor()->GetClientID(),"Adding %d points to skill %s to character %s (%d)\n",val,skillInfo->name.GetData(),
            self->GetCharFullName(),
            self->GetActor()->GetClientID());
    }
    else
    {
        Debug4(LOG_SKILLXP,self->GetActor()->GetClientID(),"WARNING! Skill practise to unknown skill(%d) for character %s (%d)\n",
            (int)skill,
            self->GetCharFullName(),
            self->GetActor()->GetClientID());
    }

    if ( added > 0 )
    {
        psZPointsGainedEvent evt( self->GetActor(), skillInfo->name, added, rankUp );
        evt.FireEvent();
    }

    return added;
}

unsigned int SkillSet::GetBestSkillValue( bool withBuffer )
{
    unsigned int max=0;
    for (int i=0; i<PSSKILL_COUNT; i++)
    {
        PSSKILL skill = skills[i].info->id;
        if(     skill == PSSKILL_AGI ||
                skill == PSSKILL_CHA ||
                skill == PSSKILL_END ||
                skill == PSSKILL_INT ||
                skill == PSSKILL_WILL ||
                skill == PSSKILL_STR )
            continue; // Jump past the stats, we only want the skills

        unsigned int rank = withBuffer ? skills[i].rank.Current() : skills[i].rank.Base();
        if (rank > max)
            max = rank;
    }
    return max;
}

unsigned int SkillSet::GetBestSkillSlot( bool withBuffer )
{
    unsigned int max = 0;
    unsigned int i = 0;
    for (; i<PSSKILL_COUNT; i++)
    {
        unsigned int rank = withBuffer ? skills[i].rank.Current() : skills[i].rank.Base();
        if (rank > max)
            max = rank;
    }

    if (i == PSSKILL_COUNT)
        return (unsigned int)~0;
    else
        return i;
}

void SkillSet::Calculate()
{
    for ( int z = 0; z < PSSKILL_COUNT; z++ )
    {
        skills[z].CalculateCosts(self);
    }
}

bool SkillSet::CanTrain( PSSKILL skill )
{
    if (skill<0 || skill>=PSSKILL_COUNT)
        return false;
    else
    {
        return skills[skill].CanTrain();
    }
}

void SkillSet::Train( PSSKILL skill, int yIncrease )
{

    if (skill<0 ||skill>=PSSKILL_COUNT)
        return;
    else
    {
        skills[skill].Train( yIncrease );
    }
}


void SkillSet::SetSkillInfo( PSSKILL which, psSkillInfo* info, bool recalculatestats )
{
    if (which<0 || which>=PSSKILL_COUNT)
        return;
    else
    {
        skills[which].info = info;
        skills[which].CalculateCosts(self);
    }

    if (recalculatestats)
      self->RecalculateStats();
}

void SkillSet::SetSkillRank( PSSKILL which, unsigned int rank, bool recalculatestats )
{
    if (which < 0 || which >= PSSKILL_COUNT)
        return;

    bool isStat = (which >= PSSKILL_AGI && which <= PSSKILL_WILL);

    // Clamp rank to stay within sane values, even if given something totally outrageous.
    if (rank < 0)
        rank = 0;
    else if (!isStat && rank > MAX_SKILL)
        rank = MAX_SKILL;
    else if (isStat && rank > MAX_STAT)
        rank = MAX_STAT;

    skills[which].rank.SetBase(rank);
    skills[which].y = skills[which].z = 0; // PS#2906 - setskill should reset training values
    skills[which].CalculateCosts(self);
    skills[which].dirtyFlag = true;

    if (recalculatestats)
      self->RecalculateStats();
}

void SkillSet::SetSkillKnowledge( PSSKILL which, int y_value )
{
    if (which<0 || which>=PSSKILL_COUNT)
        return;
    if (y_value < 0)
        y_value = 0;
    skills[which].y = y_value;
    skills[which].dirtyFlag = true;
}


void SkillSet::SetSkillPractice(PSSKILL which,int z_value)
{
    if (which<0 || which>=PSSKILL_COUNT)
        return;
    if (z_value < 0)
        z_value = 0;

    skills[which].z = z_value;
    skills[which].dirtyFlag = true;
}


bool SkillSet::AddToSkillPractice(PSSKILL skill, unsigned int val, unsigned int& added )
{
    if (skill<0 || skill>=PSSKILL_COUNT)
        return 0;

    bool rankup = false;
    rankup = skills[skill].Practice( val, added, self );
    return rankup;
}


unsigned int SkillSet::GetSkillPractice(PSSKILL skill)
{

    if (skill<0 || skill>=PSSKILL_COUNT)
        return 0;
    return skills[skill].z;
}


unsigned int SkillSet::GetSkillKnowledge(PSSKILL skill)
{

    if (skill<0 || skill>=PSSKILL_COUNT)
        return 0;
    return skills[skill].y;
}

SkillRank & SkillSet::GetSkillRank(PSSKILL skill)
{
    CS_ASSERT(skill >= 0 && skill < PSSKILL_COUNT);
    return skills[skill].rank;
}

Skill& SkillSet::Get(PSSKILL skill)
{
    CS_ASSERT(skill >= 0 && skill < PSSKILL_COUNT);
    return skills[skill];
}


//////////////////////////////////////////////////////////////////////////
// QuestAssignment accessor functions to accommodate removal of csWeakRef better
//////////////////////////////////////////////////////////////////////////

csWeakRef<psQuest>& QuestAssignment::GetQuest()
{
    if (!quest.IsValid())
    {
        psQuest *q = CacheManager::GetSingleton().GetQuestByID(quest_id);
        SetQuest(q);
    }
    return quest;
}

void QuestAssignment::SetQuest(psQuest *q)
{
    if (q)
    {
        quest = q;
        quest_id = q->GetID();
    }
}


