/*
 * CacheManager.cpp
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

//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/log.h"
#include "util/stringarray.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"

#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pstrade.h"
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psquest.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psspell.h"
#include "bulkobjects/psglyph.h"
#include "bulkobjects/pstrait.h"


#include "rpgrules/factions.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "cachemanager.h"
#include "commandmanager.h"
#include "questmanager.h"
#include "client.h"
#include "globals.h"

CacheManager::CacheManager()
{
    slotMap[PSCHARACTER_SLOT_RIGHTHAND]   = PSITEMSTATS_SLOT_RIGHTHAND;
    slotMap[PSCHARACTER_SLOT_LEFTHAND]    = PSITEMSTATS_SLOT_LEFTHAND;
    slotMap[PSCHARACTER_SLOT_BOTHHANDS]   = PSITEMSTATS_SLOT_BOTHHANDS;
    slotMap[PSCHARACTER_SLOT_HELM]        = PSITEMSTATS_SLOT_HELM;
    slotMap[PSCHARACTER_SLOT_RIGHTFINGER] = PSITEMSTATS_SLOT_RIGHTFINGER;
    slotMap[PSCHARACTER_SLOT_LEFTFINGER]  = PSITEMSTATS_SLOT_LEFTFINGER;
    slotMap[PSCHARACTER_SLOT_NECK]        = PSITEMSTATS_SLOT_NECK;
    slotMap[PSCHARACTER_SLOT_BOOTS]       = PSITEMSTATS_SLOT_BOOTS;
    slotMap[PSCHARACTER_SLOT_BACK]        = PSITEMSTATS_SLOT_BACK;
    slotMap[PSCHARACTER_SLOT_ARMS]        = PSITEMSTATS_SLOT_ARMS;
    slotMap[PSCHARACTER_SLOT_GLOVES]      = PSITEMSTATS_SLOT_GLOVES;
    slotMap[PSCHARACTER_SLOT_LEGS]        = PSITEMSTATS_SLOT_LEGS;
    slotMap[PSCHARACTER_SLOT_BELT]        = PSITEMSTATS_SLOT_BELT;
    slotMap[PSCHARACTER_SLOT_BRACERS]     = PSITEMSTATS_SLOT_BRACERS;
    slotMap[PSCHARACTER_SLOT_TORSO]       = PSITEMSTATS_SLOT_TORSO;
    slotMap[PSCHARACTER_SLOT_MIND]        = PSITEMSTATS_SLOT_MIND;

    psItemStatFlags statflag("MELEEWEAPON",  PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON);
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "ARMOR",        PSITEMSTATS_FLAG_IS_ARMOR);
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "RANGEWEAPON",  PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON);
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "SHIELD",       PSITEMSTATS_FLAG_IS_A_SHIELD );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "AMMO",         PSITEMSTATS_FLAG_IS_AMMO );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "CONTAINER",    PSITEMSTATS_FLAG_IS_A_CONTAINER );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "CANTRANSFORM", PSITEMSTATS_FLAG_CAN_TRANSFORM );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "NOPICKUP", PSITEMSTATS_FLAG_NOPICKUP );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "USESAMMO",     PSITEMSTATS_FLAG_USES_AMMO );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "STACKABLE",    PSITEMSTATS_FLAG_IS_STACKABLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "EQUIP_STACKABLE",    PSITEMSTATS_FLAG_IS_EQUIP_STACKABLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "GLYPH",        PSITEMSTATS_FLAG_IS_GLYPH );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "TRIA",         PSITEMSTATS_FLAG_TRIA );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "HEXA",         PSITEMSTATS_FLAG_HEXA );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "OCTA",         PSITEMSTATS_FLAG_OCTA );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "CIRCLE",       PSITEMSTATS_FLAG_CIRCLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "CONSUMABLE",   PSITEMSTATS_FLAG_CONSUMABLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "READABLE",     PSITEMSTATS_FLAG_IS_READABLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "WRITEABLE",    PSITEMSTATS_FLAG_IS_WRITEABLE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "CREATIVE",     PSITEMSTATS_FLAG_CREATIVE );
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "BUY_PERSONALISE",PSITEMSTATS_FLAG_BUY_PERSONALISE );
    ItemStatFlagArray.Push(statflag);

    // Flag that allows an item to be stacked if qualities are different
    // and the resulting stack will have the new average.
    statflag.Set("AVERAGEQUALITY", PSITEMSTATS_FLAG_AVERAGEQUALITY);
    ItemStatFlagArray.Push(statflag);
    statflag.Set( "RECHARGEABLE", PSITEMSTATS_FLAG_IS_RECHARGEABLE );
    ItemStatFlagArray.Push(statflag);

    statflag.Set( "END",0 );
    ItemStatFlagArray.Push(statflag);

    effectID = 0;

    commandManager = NULL;
}

CacheManager::~CacheManager()
{
    UnloadAll();
}

bool CacheManager::PreloadAll()
{
    maxCommonStrID = db->SelectSingleNumber("SELECT MAX(id) FROM common_strings");

    if (!PreloadCommonStrings())
        return false;
    if (!PreloadSectors())
        return false;
    if (!PreloadSkills())
        return false;
    if (!PreloadLimitations())
        return false;
    if (!PreloadRaceInfo())
        return false;
    if (!PreloadTraits())
        return false; // Need RaceInfo
    if (!PreloadItemCategories())
        return false;
    if (!PreloadItemAnimList())
        return false;
    if (!PreloadItemStatsDatabase())
        return false;
    if (!PreloadWays())
        return false;
    if (!PreloadFactions())
        return false;
    if (!PreloadSpells())
        return false;
    if (!PreloadQuests())
        return false;
    if (!PreloadTradeCombinations())
        return false;
    if (!PreloadTradeTransformations())
        return false;
    if (!PreloadUniqueTradeTransformations())
        return false;
    if (!PreloadTradeProcesses())
        return false;
    if (!PreloadTradePatterns())
        return false;
    if (!PreloadCraftMessages())
        return false;
    if (!PreloadTips())
        return false;
    if (!PreloadBadNames())
        return false;
    if (!PreloadArmorVsWeapon())
        return false;
    if (!PreloadMovement())
        return false;
    if (!PreloadStances())
        return false;

//    PreCreateCraftMessages();
    PreloadCommandGroups();

    return true;
}

void CacheManager::PreloadCommandGroups()
{
    commandManager = new psCommandManager;
    commandManager->LoadFromDatabase();
}

void CacheManager::UnloadAll()
{

    delete commandManager;

    quests_by_id.DeleteAll();


    {
        csHash<csPDelArray<CombinationConstruction>*,uint32>::GlobalIterator it(tradeCombinations_IDHash.GetIterator ());

        while (it.HasNext ())
        {
            csPDelArray<CombinationConstruction>* newArray = it.Next ();
            delete newArray;
        }
        tradeCombinations_IDHash.Empty();
    }

    {
        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *, uint32>::GlobalIterator it(tradeTransformations_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash = it.Next ();
            csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator it2(transHash->GetIterator ());
            while (it2.HasNext ())
            {
                csPDelArray<psTradeTransformations>* newArray = it2.Next ();
                delete newArray;
            }
            transHash->Empty();
            delete transHash;
        }
        tradeTransformations_IDHash.Empty();
    }

    {
        csHash<csArray<uint32>*,uint32>::GlobalIterator it(tradeTransUnique_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            csArray<uint32>* newArray = it.Next ();
            delete newArray;
        }
        tradeTransUnique_IDHash.Empty();
    }

    {
        csHash<csArray<psTradeProcesses*>*,uint32>::GlobalIterator it(tradeProcesses_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            csArray<psTradeProcesses*>* newArray = it.Next ();
            delete newArray;
        }
        tradeProcesses_IDHash.Empty();
    }

    {
        csHash<psTradePatterns *,uint32>::GlobalIterator it(tradePatterns_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            psTradePatterns* pattern  = it.Next ();
            delete pattern;
        }
        tradePatterns_IDHash.Empty();
    }

    {
        csHash<csArray<CraftTransInfo*>*,uint32>::GlobalIterator it(tradeCraftTransInfo_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            csArray<CraftTransInfo*>* newArray = it.Next ();
            delete newArray;
        }
        tradeCraftTransInfo_IDHash.Empty();
    }

    {
        csHash<CraftComboInfo*,uint32>::GlobalIterator it(tradeCraftComboInfo_IDHash.GetIterator ());
        while (it.HasNext ())
        {
            CraftComboInfo* newCombo = it.Next ();
            delete newCombo;
        }
        tradeCraftComboInfo_IDHash.Empty();
    }

    {
        csHash<psGuildAlliance *>::GlobalIterator it (alliance_by_id.GetIterator ());
        while (it.HasNext ())
        {
            psGuildAlliance* alliance = it.Next ();
            delete alliance;
        }
        alliance_by_id.Empty();
    }

    {
        csHash<psGuildInfo *>::GlobalIterator it (guildinfo_by_id.GetIterator ());
        while (it.HasNext ())
        {
            psGuildInfo* guildinfo = it.Next ();
            delete guildinfo;
        }
        guildinfo_by_id.Empty();
    }

    {
        csHash<psItemStats *, csString>::GlobalIterator it (itemStats_NameHash.GetIterator ());
        while (it.HasNext ())
        {
            psItemStats* itemstats = it.Next ();
            delete itemstats;
        }
        itemStats_NameHash.Empty();
    }

    {
        csHash<psSectorInfo *>::GlobalIterator it (sectorinfo_by_id.GetIterator ());
        while (it.HasNext ())
        {
            psSectorInfo* sector = it.Next ();
            delete sector;
        }
        sectorinfo_by_id.Empty();
    }
    // ToDo: unload everything else
}

void CacheManager::RemoveInstance( psItem * & item )
{
    Notify2(LOG_CACHE, "Removing Instance of item: %u", item->GetUID());
    if (item->GetUID() != 0)
    {
        db->Command("DELETE from item_instances where id='%u'", item->GetUID());
    }
    delete item;
    item = NULL;
}

void CacheManager::RemoveItemStats (psItemStats *&itemStats)
{
    if (itemStats && itemStats->GetUID() != 0)
    {
        itemStats_NameHash.Delete(itemStats->GetDownCaseName(), itemStats);
        itemStats_IDHash.Delete(itemStats->GetUID(), itemStats);

        db->Command("DELETE from item_stats where id='%u'", itemStats->GetUID());

        delete itemStats;
        itemStats = NULL;
    }
}

bool CacheManager::PreloadSkills()
{
    unsigned int currentrow;
    psSkillInfo *newskill;
    Result result(db->Select("SELECT * from skills") );

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        if (result[currentrow]["skill_id"]!=NULL)
        {
            newskill=new psSkillInfo;

            newskill->id = (PSSKILL) result[currentrow].GetInt("skill_id");
            newskill->name = result[currentrow]["name"];
            newskill->description = result[currentrow]["description"];
            newskill->practice_factor = result[currentrow].GetInt("practice_factor");
            newskill->mental_factor = result[currentrow].GetInt("mental_factor");
            newskill->price = psMoney(result[currentrow].GetInt("price"));
            newskill->baseCost = result[currentrow].GetInt("base_rank_cost");

            csString type( result[currentrow]["category"] );
            if ( type == "STATS" )
                newskill->category = PSSKILLS_CATEGORY_STATS;
            else if ( type == "COMBAT" )
                newskill->category = PSSKILLS_CATEGORY_COMBAT;
            else if ( type == "MAGIC" )
                newskill->category = PSSKILLS_CATEGORY_MAGIC;
            else if ( type == "JOBS" )
                newskill->category = PSSKILLS_CATEGORY_JOBS;
            else if ( type == "VARIOUS" )
                newskill->category = PSSKILLS_CATEGORY_VARIOUS;

            skillinfolist.Push(newskill);
            maxCommonStrID++;
            msg_strings.Register(newskill->name,(csStringID)maxCommonStrID);
        }
    }
    Notify2( LOG_STARTUP, "%lu Skills Loaded", result.Count() );
    return true;
}

bool CacheManager::PreloadLimitations()
{
    psCharacterLimitation *limit;
    unsigned int currentrow;
    Result result(db->Select("SELECT * from character_limitations") );

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow< result.Count(); currentrow++)
    {
        limit = new psCharacterLimitation;
        limit->id = result[currentrow].GetInt("id");
        limit->limit_type = result[currentrow]["limit_type"];
        limit->min_score  = result[currentrow].GetInt("player_score");
        limit->value      = result[currentrow]["value"];

        limits.Push(limit);
    }
    Notify2( LOG_STARTUP, "%lu Limitations Loaded", result.Count() );
    return true;
}

const psCharacterLimitation *CacheManager::GetLimitation(size_t index)
{
    if (index >= limits.GetSize() )
    {
        return NULL;
    }
    else
    {
        return limits[index];
    }
}

bool CacheManager::PreloadSectors()
{
    unsigned int currentrow;
    psSectorInfo *newsector;
    Result result(db->Select("SELECT * from sectors") );

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow< result.Count(); currentrow++)
    {
        newsector=new psSectorInfo;
        CS_ASSERT(newsector!=NULL);

        newsector->uid  = result[currentrow].GetInt("id");
        newsector->name = result[currentrow]["name"];
        newsector->rain_enabled = strcmp(result[currentrow]["rain_enabled"],"Y")==0;

        newsector->rain_min_gap = result[currentrow].GetInt("rain_min_gap");
        newsector->rain_max_gap = result[currentrow].GetInt("rain_max_gap");
        CS_ASSERT(newsector->rain_min_gap <= newsector->rain_max_gap);

        newsector->rain_min_duration = result[currentrow].GetInt("rain_min_duration");
        newsector->rain_max_duration = result[currentrow].GetInt("rain_max_duration");
        CS_ASSERT(newsector->rain_min_duration <= newsector->rain_max_duration);

        newsector->rain_min_drops = result[currentrow].GetInt("rain_min_drops");
        newsector->rain_max_drops = result[currentrow].GetInt("rain_max_drops");
        CS_ASSERT(newsector->rain_min_drops <= newsector->rain_max_drops);

        newsector->lightning_min_gap = result[currentrow].GetInt("lightning_min_gap");
        newsector->lightning_max_gap = result[currentrow].GetInt("lightning_max_gap");
        CS_ASSERT(newsector->lightning_min_gap <= newsector->lightning_max_gap);

        newsector->rain_min_fade_in = result[currentrow].GetInt("rain_min_fade_in");
        newsector->rain_max_fade_in = result[currentrow].GetInt("rain_max_fade_in");
        CS_ASSERT(newsector->rain_min_fade_in <= newsector->rain_max_fade_in);

        newsector->rain_min_fade_out = result[currentrow].GetInt("rain_min_fade_out");
        newsector->rain_max_fade_out = result[currentrow].GetInt("rain_max_fade_out");
        CS_ASSERT(newsector->rain_min_fade_out <= newsector->rain_max_fade_out);

        newsector->is_colliding = (result[currentrow].GetInt("collide_objects") != 0);
        newsector->is_non_transient = (result[currentrow].GetInt("non_transient_objects") != 0);

        newsector->god_name = result[currentrow]["god_name"];

        sectorinfo_by_id.Put(newsector->uid,newsector);
        sectorinfo_by_name.Put(csHashCompute(newsector->name),newsector);

        maxCommonStrID++;
        msg_strings.Register(newsector->name,(csStringID)maxCommonStrID);
    }

    Notify2( LOG_STARTUP, "%lu Sectors Loaded", result.Count() );
    return true;
}

bool CacheManager::PreloadMovement()
{
    Result modes(db->Select("SELECT * FROM movement_modes"));
    if ( !modes.IsValid() )
    {
        return false;
    }
    if ( !modes.Count() )
    {
        Error1("No movement modes to load.");
        return false;
    }

    for (unsigned int i=0; i<modes.Count(); i++)
    {
        psCharMode* newmode = new psCharMode;

        newmode->id = modes[i].GetUInt32("id");
        newmode->name = modes[i]["name"];
        newmode->move_mod.x = modes[i].GetFloat("move_mod_x");
        newmode->move_mod.y = modes[i].GetFloat("move_mod_y");
        newmode->move_mod.z = modes[i].GetFloat("move_mod_z");
        newmode->rotate_mod.x = modes[i].GetFloat("rotate_mod_x");
        newmode->rotate_mod.y = modes[i].GetFloat("rotate_mod_y");
        newmode->rotate_mod.z = modes[i].GetFloat("rotate_mod_z");
        newmode->idle_animation = modes[i]["idle_animation"];

        if (newmode->id > 127)  // Based on DR message variable size
        {
            Error3("ID %u for movement '%s' is to large.\n"
                   "Clients only support up to 128 different movement types, with IDs from 0-127",
                   newmode->id, newmode->name.GetData() );
            return false;
        }

        char_modes.Put(newmode->id,newmode);
    }
    Notify2( LOG_STARTUP, "%lu Movement Modes Loaded", modes.Count() );

    Result types(db->Select("SELECT * FROM movement_types"));
    if ( !types.IsValid() )
    {
        return false;
    }

    if ( !types.Count() )
    {
        Error1("No movement types to load.");
        return false;
    }

    for (unsigned int i=0; i<types.Count(); i++)
    {
        psMovement* newmove = new psMovement;

        newmove->id = types[i].GetUInt32("id");
        newmove->name = types[i]["name"];
        newmove->base_move.x = types[i].GetFloat("base_move_x");
        newmove->base_move.y = types[i].GetFloat("base_move_y");
        newmove->base_move.z = types[i].GetFloat("base_move_z");
        newmove->base_rotate.x = types[i].GetFloat("base_rotate_x");
        newmove->base_rotate.y = types[i].GetFloat("base_rotate_y");
        newmove->base_rotate.z = types[i].GetFloat("base_rotate_z");

        if (newmove->id > 31)  // Based on active move bits in client movement manager
        {
            Error3("ID %u for movement '%s' is to large.\n"
                   "Clients only support up to 32 different movement types, with IDs from 0-31",
                   newmove->id, newmove->name.GetData() );
            return false;
        }

        movements.Put(newmove->id,newmove);
    }

    Notify2( LOG_STARTUP, "%lu Movement Types Loaded", types.Count() );
    return true;
}

uint8_t CacheManager::GetCharModeID(const char* name)
{
    for (size_t i=0; i<char_modes.GetSize(); i++)
    {
        if (char_modes[i]->name == name)
        {
            return (uint8_t)i;
        }
    }
    return (uint8_t)-1;
}

uint8_t CacheManager::GetMovementID(const char* name)
{
    for (size_t i=0; i<movements.GetSize(); i++)
    {
        if (movements[i]->name == name)
        {
            return (uint8_t)i;
        }
    }
    return (uint8_t)-1;
}

bool CacheManager::PreloadArmorVsWeapon()
{
    unsigned int currentrow;
    Result result(db->Select("select * from armor_vs_weapon"));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        ArmorVsWeapon* newvs=new ArmorVsWeapon;

        newvs->id          = result[currentrow].GetUInt32("id");
        newvs->c[0][0]     = result[currentrow].GetFloat("1a");
        newvs->c[0][1]     = result[currentrow].GetFloat("1b");
        newvs->c[0][2]     = result[currentrow].GetFloat("1c");
        newvs->c[0][3]     = result[currentrow].GetFloat("1d");

        newvs->c[1][0]     = result[currentrow].GetFloat("2a");
        newvs->c[1][1]     = result[currentrow].GetFloat("2b");
        newvs->c[1][2]     = result[currentrow].GetFloat("2c");

        newvs->c[2][0]     = result[currentrow].GetFloat("3a");
        newvs->c[2][1]     = result[currentrow].GetFloat("3b");
        newvs->c[2][2]     = result[currentrow].GetFloat("3c");
        newvs->weapontype  = result[currentrow]["weapon_type"];

        armor_vs_weapon.Push(newvs);
    }


    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('3c','Dagger'): %f\n",GetArmorVSWeaponResistance("3c","Dagger"));
    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('1d','Claymore'): %f\n",GetArmorVSWeaponResistance("1d","Claymore"));
    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('2b','Sabre'): %f\n",GetArmorVSWeaponResistance("2b","Sabre"));


    Notify2( LOG_STARTUP, "%lu Armor vs Weapon Loaded", result.Count() );
    return true;
}

bool CacheManager::PreloadCommonStrings()
{
    unsigned int currentrow;
    Result result(db->Select("select id,string from common_strings"));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        msg_strings.Register(result[currentrow]["string"],(csStringID)result[currentrow].GetUInt32("id"));
    }

    Notify2( LOG_STARTUP, "%zu Common Strings Loaded", msg_strings.GetSize() );
    return true;
}

bool CacheManager::PreloadStances()
{
    Result result(db->Select("select * from stances order by id"));

    if(!result.IsValid())
    {
        return false;
    }

    for(unsigned int currentRow = 0; currentRow < result.Count(); currentRow++)
    {
        iResultRow& row = result[currentRow];
        Stance temp;
        temp.stance_id = row.GetInt("id")-1;
        temp.stance_name = row["name"];
        temp.stamina_drain_P = row.GetFloat("stamina_drain_P");
        temp.stamina_drain_M = row.GetFloat("stamina_drain_M");
        temp.attack_speed_mod = row.GetFloat("attack_speed_mod");
        temp.attack_damage_mod = row.GetFloat("attack_damage_mod");
        temp.defense_avoid_mod = row.GetFloat("defense_avoid_mod");
        temp.defense_absorb_mod = row.GetFloat("defense_absorb_mod");
        stances.Push(temp);
        stanceID.Push(temp.stance_name);
    }
    Notify2( LOG_STARTUP, "%lu Stances Loaded", result.Count() );
    return true;
}

bool CacheManager::PreloadQuests()
{
    unsigned int currentrow;
    psQuest *quest;
    csArray<psQuest *> failed;

    Result result(db->Select("select * from quests order by id"));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        quest = new psQuest;

        if (quest->Load(result[currentrow]))
        {
            quests_by_id.Put(quest->GetID(),quest);
        }
        else
        {
            delete quest;
        }
    }

    Notify2( LOG_STARTUP, "%lu Quests Loaded", result.Count() );
    return true;
}

csHash<psQuest *>::GlobalIterator CacheManager::GetQuestIterator()
{
    return quests_by_id.GetIterator();
}

bool CacheManager::UnloadQuest(int id)
{
    bool ret = false;

    psQuest* quest = quests_by_id.Get(id, NULL);
    if(quest)
    {
        delete quest;
        quests_by_id.DeleteAll(id);
        ret = true;
    }
    else
    {
        CPrintf(CON_ERROR, "Cannot find quest %d to remove.\n", id);
    }

    return ret;
}

bool CacheManager::LoadQuest(int id)
{
    if(quests_by_id.Get(id, NULL))
    {
        CPrintf(CON_ERROR, "Quest already exists.\n", id);
        return false;
    }

    psQuest *quest;

    Result result(db->Select("select * from quests where id=%d", id));

    if (!result.IsValid() || result.Count() == 0)
    {
        CPrintf(CON_ERROR, "Cannot find quest %d in database.\n", id);
        return false;
    }

    quest = new psQuest;

    bool success = false;

    success = quest->Load(result[0]);
    if(success)
    {
        // The quest id must first be put into the hash because PostLoad may references it.
        quests_by_id.Put(quest->GetID(),quest);

        success = quest->PostLoad();
        if(!success)
        {
            quests_by_id.DeleteAll(quest->GetID());
            CPrintf(CON_ERROR, "Could not load quest prerequisites for quest id %d", quest->GetID());
        }
    }


    if(success)
    {
        // Load scripts
        success = psserver->questmanager->LoadQuestScript(id);
        if(!success)
        {
            quests_by_id.DeleteAll(quest->GetID());
            CPrintf(CON_ERROR, "Could not load quest script for quest %d\n", id);
        }
    }

    if(!success)
    {
        delete quest;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////
// Trade Crafts

// Trade Combinations
bool CacheManager::PreloadTradeCombinations()
{
    unsigned int currentrow;
    uint32 lastarrayid = (uint32)-1;
    uint32 lastitemid = (uint32)-1;
    int lastitemqty = -1;
    CombinationConstruction *ctr;
    csPDelArray<CombinationConstruction> *newArray = NULL;

    Result result(db->Select("select * from trade_combinations order by pattern_id, result_id, item_id"));
    if (!result.IsValid())
    {
        Error1("No data in trade_combinations could be found");
    }
    else
    {
        // Loop through all the combinations
        for (currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // If it is a new pattern start a new combination set for them
            uint32 id = result[currentrow].GetUInt32("pattern_id");
            if ( id != lastarrayid )
            {
                newArray = new csPDelArray<CombinationConstruction>;
                tradeCombinations_IDHash.Put(id,newArray);
                lastarrayid = id;
            }

            // If it is a new item or quantity start a new combination construction for it.
            uint32 resultItem = result[currentrow].GetUInt32("result_id");
            int resultQty = result[currentrow].GetInt("result_qty");
            if (( resultItem != lastitemid ) || ( resultQty != lastitemqty ))
            {
                ctr = new CombinationConstruction;
                ctr->resultItem = resultItem;
                ctr->resultQuantity = resultQty;
                newArray->Push(ctr);
                lastitemid = resultItem;
                lastitemqty = resultQty;
            }

            // Load the combination and push it into it's construction set.
            psTradeCombinations* comb = new psTradeCombinations;
            if (comb->Load(result[currentrow]))
                ctr->combinations.Push(comb);
            else
                delete comb;
        }

        Notify2( LOG_STARTUP, "%lu Trade Combinations Loaded", result.Count() );
    }
    return true;
}

// Get set of transformations for that pattern
csPDelArray<CombinationConstruction>* CacheManager::FindCombinationsList(uint32 patternid)
{
    return tradeCombinations_IDHash.Get(patternid,NULL);
}

// Trade Transformations
bool CacheManager::PreloadTradeTransformations()
{
    unsigned int currentrow;
    uint32 lastpid = (uint32)-1;
    uint32 lastiid = (uint32)-1;
    csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash = NULL;
    csPDelArray<psTradeTransformations>* newArray = NULL;

    Result result(db->Select("select * from trade_transformations order by pattern_id, item_id"));
    if (!result.IsValid())
    {
        Error1("No data in trade_transformations could be found");
    }
    else
    {
        for (currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // For new patterns create a new transformation hash and push it onto IDHash table
            uint32 pid = result[currentrow].GetUInt32("pattern_id");
            if ( pid != lastpid )
            {
                transHash = new csHash<csPDelArray<psTradeTransformations> *,uint32>;
                tradeTransformations_IDHash.Put(pid,transHash);
                lastpid = pid;
            }

            // For new items create new transformation array
            uint32 iid = result[currentrow].GetUInt32("item_id");
            if ( iid != lastiid )
            {
                newArray = new csPDelArray<psTradeTransformations>;
                transHash->Put(iid,newArray);
                lastiid = iid;
            }

            // Finnaly just load the transformation into existing array
            psTradeTransformations* tran = new psTradeTransformations;
            if (tran->Load(result[currentrow]))
                newArray->Push(tran);
            else
                delete tran;
        }

        Notify2( LOG_STARTUP, "%lu Trade Transformations Loaded", result.Count() );
    }
    return true;
}

// Get transformation array for pattern and target item
csPDelArray<psTradeTransformations> *CacheManager::FindTransformationsList(uint32 patternid, uint32 targetid)
{
    // First get transformation hash table using the patternid
    csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash;
    transHash = tradeTransformations_IDHash.Get(patternid,NULL);
    if (!transHash)
    {
        // Error2("No data in trade_transformations for pattern %d", patternid);
        return NULL;
    }

    // then return transformation array pointer
    return transHash->Get(targetid,NULL);
}

// Trade Transformations
bool CacheManager::PreloadUniqueTradeTransformations()
{
    uint32 currentID;
    csString query;
    csArray<uint32>* newArray;

    // Get a list of the trade patterns ids
    Result result(db->Select("select id from trade_patterns order by id"));
    if (!result.IsValid())
    {
        Error1("No data in trade_patterns could be found");
    }
    else
    {
        for (int currentrow=0; currentrow<(int)result.Count(); currentrow++)
        {
            newArray = new csArray<uint32>;
            currentID = result[currentrow].GetUInt32("id");

            // Now get a list of unique transformation items for each pattern
            query.Format( "select distinct item_id from trade_transformations where pattern_id =%d order by item_id", currentID );
            Result result (db->Select( query ) );
            if (!result.IsValid())
            {
                Error1("No distinct item ids found in trade_transformations");
            }
            else
            {
                // Push each result item ID into array
                for (int transrow=0; transrow<(int)result.Count(); transrow++)
                {
                    newArray->Push(result[transrow].GetUInt32("item_id"));
                }

                // Now get a list of unique combination items for each pattern
                query.Format( "select distinct item_id from trade_combinations where pattern_id =%d order by item_id", currentID );
                Result result2 (db->Select( query ) );
                if (!result2.IsValid())
                {
                    Error1("No distinct item ids found in trade_combinations");
                }
                else
                {
                    // Push each result item ID into array
                    for (int combsrow=0; combsrow<(int)result2.Count(); combsrow++)
                    {
                        newArray->Push(result2[combsrow].GetUInt32("item_id"));
                    }

                    // Add hash
                    tradeTransUnique_IDHash.Put(currentID,newArray);
                }
            }
        }

        Notify2( LOG_STARTUP, "%lu Unique Trade Transformations Loaded", result.Count() );
    }
    return true;
}

csArray<uint32>* CacheManager::GetTradeTransUniqueByID(uint32 id)
{
    return tradeTransUnique_IDHash.Get(id,NULL);
}

// Trade Processes
bool CacheManager::PreloadTradeProcesses()
{
    uint32 lastpid = (uint32)-1;
    unsigned int currentrow;
    psTradeProcesses* newProcess;
    csArray<psTradeProcesses*>* newArray = NULL;

    // Get a list of the trade processes
    Result result(db->Select("select * from trade_processes order by process_id, subprocess_number"));
    if (!result.IsValid())
    {
        Error1("No data in trade_processes could be found");
    }
    else
    {
        for (currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // For a new process_id create a new process array and hash it
            uint32 pid = result[currentrow].GetUInt32("process_id");
            if ( pid != lastpid )
            {
                newArray = new csArray<psTradeProcesses*>;
                tradeProcesses_IDHash.Put(pid,newArray);
                lastpid = pid;
            }

            // For each row create a new processes class to store it
            newProcess = new psTradeProcesses;
            if (!newProcess->Load(result[currentrow]))
            {
                Error1("Failure to load processes");
                delete newProcess;
                continue;
            }

            // Put process into array
            newArray->Push(newProcess);
        }
        Notify2( LOG_STARTUP, "%lu Trade Processes Loaded", result.Count() );
    }
    return true;
}

csArray<psTradeProcesses*>* CacheManager::GetTradeProcessesByID(uint32 id)
{
    return tradeProcesses_IDHash.Get(id,NULL);
}

// Trade Patterns
bool CacheManager::PreloadTradePatterns()
{
    unsigned int currentrow;
    psTradePatterns* newPattern;

    // Get a list of the trade patterns ignoring the dummy ones
    Result result(db->Select("select * from trade_patterns order by designitem_id"));
    if (!result.IsValid())
    {
        Error1("Invalid select from trade patterns.");
    }
    else
    {
        for (currentrow=0; currentrow<result.Count(); currentrow++)
        {
            newPattern = new psTradePatterns;
            if (!newPattern->Load(result[currentrow]))
            {
                delete newPattern;
                Error1("Unable to load new pattern.");
                continue;
            }
            csString newName = newPattern->GetPatternString();
            tradePatterns_IDHash.Put(newPattern->GetDesignItemId(),newPattern);
            tradePatterns_NameHash.Put(newName, newPattern);
        }

        Notify2( LOG_STARTUP, "%lu Trade Patterns Loaded", result.Count() );
    }
    return true;
}

psTradePatterns *CacheManager::GetTradePatternByItemID(uint32 id)
{
    return tradePatterns_IDHash.Get(id,NULL);
}

psTradePatterns *CacheManager::GetTradePatternByName(csString name)
{
    return tradePatterns_NameHash.Get(name,NULL);
}

// Trade Info Message
bool CacheManager::PreloadCraftMessages()
{
    // Create transform info
    csString query;
    csString stepDescription;
    uint32 lastdid = (uint32)-1;
    CraftTransInfo *craftInfo;
    csArray<CraftTransInfo*>* newArray = NULL;
    psTradeTransformations* tran = new psTradeTransformations;

    // Get a list of all the trade patterns in the database ordered by design item ID
    Result result(db->Select("SELECT * from trade_patterns order by designitem_id") );
    for (int currentPattern=0; currentPattern<(int)result.Count(); currentPattern++)
    {
        // Get the design item that goes with the pattern
        int currentID = result[currentPattern].GetInt("id");
        uint32 designItemID = result[currentPattern].GetInt("designitem_id");
        int currentGroupID = result[currentPattern].GetInt("group_id");

        // Skip the patterns with no design item
        if (designItemID == 0)
        {
            continue;
        }

        // Preload the combination craft string for current pattern
        csPDelArray<CombinationConstruction>* combArray = FindCombinationsList(currentID);
        if (combArray)
        {
            // Get the combination craft info string
            CraftComboInfo* combInfo = new CraftComboInfo;
            csArray<CraftSkills*>* craftArray = new csArray<CraftSkills*>;
            combInfo->skillArray = craftArray;
            combInfo->craftCombDescription = CreateComboCraftDescription(combArray);

            // Get the skills array from the transformations
            for (size_t i=0; i<combArray->GetSize(); i++)
            {
                uint32 resultID = combArray->Get(i)->resultItem;
                csPDelArray<psTradeTransformations>* transArray = FindTransformationsList(currentID, resultID);
                if (!transArray) {
                    Error3("Can not find any transformation data for pattern %d and result %u ",currentID, resultID);
                    continue;
                }
                for (size_t j=0; j<transArray->GetSize(); j++)
                {
                    psTradeTransformations* trans = transArray->Get(j);
                    csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(trans->GetProcessId());

                    // If no process array just continue on
                    if (!procArray)
                    {
                        continue;
                    }

                    // Get the process array
                    for (size_t i=0; i<procArray->GetSize(); i++)
                    {
                        psTradeProcesses* proc = procArray->Get(i);
                        CraftSkills* craftSkill = new CraftSkills;

                        craftSkill->priSkillId = proc->GetPrimarySkillId();
                        craftSkill->minPriSkill = proc->GetMinPrimarySkill();
                        craftSkill->secSkillId = proc->GetSecondarySkillId();
                        craftSkill->minSecSkill = proc->GetMinSecondarySkill();
                        craftArray->Push(craftSkill);
                    }
                }
            }
            tradeCraftComboInfo_IDHash.Put(designItemID,combInfo);
        }

        // Preload the combination craft string for current group pattern
        csPDelArray<CombinationConstruction>* combGroupArray = FindCombinationsList(currentGroupID);
        if (combGroupArray)
        {
            // Get the combination craft info string
            CraftComboInfo* combInfo = new CraftComboInfo;
            csArray<CraftSkills*>* craftArray = new csArray<CraftSkills*>;
            combInfo->skillArray = craftArray;
            combInfo->craftCombDescription = CreateComboCraftDescription(combGroupArray);

            // Get the skills array from the transformations
            for (size_t i=0; i<combGroupArray->GetSize(); i++)
            {
                uint32 resultID = combGroupArray->Get(i)->resultItem;
                csPDelArray<psTradeTransformations>* transArray = FindTransformationsList(currentGroupID, resultID);
                if (!transArray) {
                    Error3("Can not find any transformation data for group pattern %d and result %u ",currentGroupID, resultID);
                    continue;
                }
                for (size_t j=0; j<transArray->GetSize(); j++)
                {
                    psTradeTransformations* trans = transArray->Get(j);
                    csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(trans->GetProcessId());

                    // If no process array just continue on
                    if (!procArray)
                    {
                        continue;
                    }

                    // Get the process array
                    for (size_t i=0; i<procArray->GetSize(); i++)
                    {
                        psTradeProcesses* proc = procArray->Get(i);
                        CraftSkills* craftSkill = new CraftSkills;

                        craftSkill->priSkillId = proc->GetPrimarySkillId();
                        craftSkill->minPriSkill = proc->GetMinPrimarySkill();
                        craftSkill->secSkillId = proc->GetSecondarySkillId();
                        craftSkill->minSecSkill = proc->GetMinSecondarySkill();
                        craftArray->Push(craftSkill);
                    }
                }
            }
            tradeCraftComboInfo_IDHash.Put(designItemID,combInfo);
        }

        // If it is a new design item then create new cache
        if ( designItemID != lastdid )
        {
            newArray = new csArray<CraftTransInfo*>;
            tradeCraftTransInfo_IDHash.Put(designItemID,newArray);
            lastdid = designItemID;
        }

        // Get all the transforms for that pattern
        query.Format( "select * from trade_transformations where pattern_id=%d order by process_id", currentID );
        Result result (db->Select( query ) );
        if (!result.IsValid())
        {
            Error2("No data in trade_transformations for pattern %d", currentID);
        }
        else
        {
            // Create craft info for each transform
            for (int transrow=0; transrow<(int)result.Count(); transrow++)
            {
                // Load the transformation
                if (!tran->Load(result[transrow]))
                {
                    Error2("Error loading trade_transformations for pattern %d", currentID);
                    continue;
                }

                // Load processes that goes with transformation
                uint32 pid = result[transrow].GetUInt32("process_id");
                csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(pid);

                // If no process array just continue on
                if (!procArray)
                {
                    continue;
                }

                // Get the process array
                for (size_t i=0; i<procArray->GetSize(); i++)
                {
                    psTradeProcesses* proc = procArray->Get(i);
                    craftInfo = new CraftTransInfo;

                    // Load up process information
                    craftInfo->priSkillId = proc->GetPrimarySkillId();
                    craftInfo->minPriSkill = proc->GetMinPrimarySkill();
                    craftInfo->secSkillId = proc->GetSecondarySkillId();
                    craftInfo->minSecSkill = proc->GetMinSecondarySkill();
                    craftInfo->craftStepDescription = CreateTransCraftDescription(tran,proc);
                    newArray->Push(craftInfo);
                }
            }
        }

        // Get all the transforms for that group pattern
        query.Format( "select * from trade_transformations where pattern_id=%d order by process_id", currentGroupID );
        Result groupResult (db->Select( query ) );
        if (!groupResult.IsValid())
        {
            Error2("No data in trade_transformations for group pattern %d", currentGroupID);
        }
        else
        {
            // Create craft info for each transform
            for (int transrow=0; transrow<(int)groupResult.Count(); transrow++)
            {
                // Load the transformation
                if (!tran->Load(groupResult[transrow]))
                {
                    Error2("Error loading trade_transformations for group pattern %d", currentGroupID);
                    continue;
                }

                // Load processes that goes with transformation
                int32 pid = groupResult[transrow].GetUInt32("process_id");
                 csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(pid);

                // If no process array just continue on
                if (!procArray)
                {
                    continue;
                }

                // Get the process array
                for (size_t i=0; i<procArray->GetSize(); i++)
                {
                    psTradeProcesses* proc = procArray->Get(i);
                    craftInfo = new CraftTransInfo;

                    // Load up process information
                    craftInfo->priSkillId = proc->GetPrimarySkillId();
                    craftInfo->minPriSkill = proc->GetMinPrimarySkill();
                    craftInfo->secSkillId = proc->GetSecondarySkillId();
                    craftInfo->minSecSkill = proc->GetMinSecondarySkill();
                    craftInfo->craftStepDescription = CreateTransCraftDescription(tran,proc);
                    newArray->Push(craftInfo);
                }
            }
        }
    }
    delete tran;
    Notify2( LOG_STARTUP, "%lu Trade Patterns Loaded", result.Count() );
    return true;
}

csString CacheManager::CreateTransCraftDescription(psTradeTransformations* tran, psTradeProcesses* proc)
{
    csString desc("");

    // Get item names or skip for 0 id
    psItemStats* itemStats = CacheManager::GetSingleton().GetBasicItemStatsByID( tran->GetItemId() );
    if (!itemStats)
    {
        return desc;
    }

    // Get result name or skip for 0 id
    psItemStats* resultStats = CacheManager::GetSingleton().GetBasicItemStatsByID( tran->GetResultId() );
    if (!resultStats)
    {
        return desc;
    }

    // Get work name or skip for 0 id
    psItemStats* workStats = CacheManager::GetSingleton().GetBasicItemStatsByID( proc->GetWorkItemId() );
    if (!workStats)
    {
        return desc;
    }

    // Create craft message
    //  Example: "Bake with skill 2 waybread dough into a waybread using oven"
    if (tran->GetItemQty() == 0)
    {
        desc.Format("%s %s into ", proc->GetName().GetData(), itemStats->GetName());
    }
    else if (tran->GetItemQty() == 1)
    {
        desc.Format("%s %s into ", proc->GetName().GetData(), itemStats->GetName());
    }
    else
    {
        desc.Format("%s %d %ss into ", proc->GetName().GetData(), tran->GetItemQty(), itemStats->GetName());
    }
    csString secondHalf;
    if (tran->GetResultQty() == 0)
    {
        secondHalf.Format("%s using %s", resultStats->GetName(), workStats->GetName());
    }
    else if (tran->GetResultQty() == 1)
    {
        secondHalf.Format("%s using %s", resultStats->GetName(), workStats->GetName());
    }
    else
    {
        secondHalf.Format("%d %ss using %s", tran->GetResultQty(), resultStats->GetName(), workStats->GetName());
    }
    desc.Append(secondHalf);

    // Get tool name if one exists
    if (proc->GetEquipementId() != 0)
    {
        psItemStats* toolStats = CacheManager::GetSingleton().GetBasicItemStatsByID( proc->GetEquipementId() );
        if (!toolStats)
        {
            Error2("No tool id %u", proc->GetEquipementId());
            return desc;
        }
        csString temp;
        temp.Format(" with a %s.\n", toolStats->GetName());
        desc.Append(temp);
    }
    else
    {
        desc.Append(".\n");
    }
    return desc;
}

csString CacheManager::CreateComboCraftDescription(csPDelArray<CombinationConstruction>* combArray)
{
    csString temp;
    csString desc("");

    // Go through all of the combinations creating the info description
    for (size_t i=0; i<combArray->GetSize(); i++)
    {
        // Check for matching lists and create combination info string
        //  Example: "Combine between 2 and 4 Eggs, Nuts, 5 Milk into Waybread Batter."
        CombinationConstruction* current = combArray->Get(i);
        if (!current)
        {
            Error2("No combination construction in combination array location %zu", i);
            return desc;
        }
        desc.Append("Combine ");

        // Get each of the items
        for (size_t j=0; j<current->combinations.GetSize(); j++)
        {
            uint32 combId  = current->combinations[j]->GetItemId();
            int combMinQty = current->combinations[j]->GetMinQty();
            int combMaxQty = current->combinations[j]->GetMaxQty();
            psItemStats* itemStats = CacheManager::GetSingleton().GetBasicItemStatsByID( combId );
            if (!itemStats)
            {
                Error2("No item stats for id %u", combId);
                return desc;
            }

            // Check if min and max is same
            if(combMinQty == combMaxQty)
            {
                if (combMinQty == 1)
                {
                    temp.Format("%s, ", itemStats->GetName());
                    desc.Append(temp);
                }
                else
                {
                    temp.Format("%d %ss, ", combMinQty, itemStats->GetName());
                    desc.Append(temp);
                }
            }
            else
            {
                temp.Format("between %d and %d %ss, ", combMinQty, combMaxQty, itemStats->GetName());
                desc.Append(temp);
            }
        }

        // Get result item names
        psItemStats* resultItemStats = CacheManager::GetSingleton().GetBasicItemStatsByID( current->resultItem );
        if (!resultItemStats)
        {
            Error2("No item stats for id %u", current->resultItem);
            return desc;
        }

        // Add result part of description
        if (current->resultQuantity == 1)
        {
            temp.Format("into %s.\n", resultItemStats->GetName());
            desc.Append(temp);
        }
        else
        {
            temp.Format("into %d %ss.\n", current->resultQuantity, resultItemStats->GetName());
            desc.Append(temp);
        }
    }
    return desc;
}

csArray<CraftTransInfo*>* CacheManager::GetTradeTransInfoByItemID(uint32 id)
{
    return tradeCraftTransInfo_IDHash.Get(id,NULL);
}

CraftComboInfo* CacheManager::GetTradeComboInfoByItemID(uint32 id)
{
    return tradeCraftComboInfo_IDHash.Get(id,NULL);
}

////////////////////////////////////////////////////////////////////
bool CacheManager::PreloadTips()
{
    unsigned int currentrow;

    // Id<1000 means we are excluding Tutorial tips
    Result result(db->Select("select tip from tips where id<1000"));
    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        csString dummy(result[currentrow][0]);
        tips_list.Push(dummy);
    }

    Notify2( LOG_STARTUP, "%lu Tips Loaded", result.Count() );
    return true;
}

/////////////////////////////////////////////////////////////////////
const char *CacheManager::FindCommonString(unsigned int id)
{
    if (id==0)
    {
        return "";
    }
    return msg_strings.Request(id);
}
unsigned int CacheManager::FindCommonStringID(const char *name)
{
    if ( name == NULL )
    {
        return 0;
    }

    csStringID id = (csStringID) msg_strings.Request(name);
    return (id == csInvalidStringID) ? 0 : id.GetHash();
}

psQuest *CacheManager::GetQuestByID(unsigned int id)
{
    psQuest *quest = quests_by_id.Get(id, NULL);
    return quest;
}

psQuest *CacheManager::GetQuestByName(const char *name)
{
    if (name==NULL)
        return NULL;

    csHash<psQuest *>::GlobalIterator it (quests_by_id.GetIterator ());
    while (it.HasNext ())
    {
        psQuest* quest = it.Next();
        if (!strcasecmp(quest->GetName(), name))
            return quest;
    }
    return NULL;
}

psQuest *CacheManager::AddDynamicQuest(const char *name, psQuest *parentQuest, int step)
{
    if (!name)
    {
        return NULL;
    }

    psQuest *ptr;
    //  subquests need a fixed id to be loaded at next restart
    // quest_id*10000+sub_id
    int id = 10000+(parentQuest->GetID()*100)+step;
    ptr = new psQuest;
    ptr->Init(id,name);
    ptr->SetTask("NULL");
    ptr->SetParentQuest(parentQuest);
    parentQuest->AddSubQuest(id);
    quests_by_id.Put(id, ptr);

    return ptr;
}




// TODO: This should be done faster, probably not with an array
psSkillInfo *CacheManager::GetSkillByID(unsigned int id)
{
    size_t i;
    psSkillInfo *currentskill;

    for (i=0;i<skillinfolist.GetSize();i++)
    {
        currentskill=skillinfolist.Get(i);
        if (currentskill && (unsigned int)currentskill->id==id)
        {
            return currentskill;
        }
    }
    return NULL;

}

// TODO: This should be done faster, probably not with an array
psSkillInfo *CacheManager::GetSkillByName(const char *name)
{
    size_t i;
    psSkillInfo *currentskill;

    for (i=0;i<skillinfolist.GetSize();i++)
    {
        currentskill=skillinfolist.Get(i);
        if (currentskill && currentskill->name.CompareNoCase(name) )
        {
            return currentskill;
        }
    }
    return NULL;
}

void CacheManager::GetSkillsListbyCategory(csArray <psSkillInfo>& listskill,int category )
{
    psSkillInfo *currentskill;
    for(size_t i=0;i<skillinfolist.GetSize();i++)
    {
        currentskill=skillinfolist.Get(i);
        if(currentskill && currentskill->category == category)
        {
            listskill.Push(*currentskill);
        }
    }
}


csHash<psSectorInfo *>::GlobalIterator CacheManager::GetSectorIterator()
{
    return sectorinfo_by_name.GetIterator();
}

psSectorInfo *CacheManager::GetSectorInfoByID(unsigned int id)
{
    return sectorinfo_by_id.Get(id, NULL);
}

psSectorInfo *CacheManager::GetSectorInfoByName(const char *name)
{
    if (name == NULL || strlen(name) == 0)
    {
        return NULL;
    }

    return sectorinfo_by_name.Get(csHashCompute(name), NULL);
}


PSTRAIT_LOCATION CacheManager::ConvertTraitLocationString(const char *locationstring)
{

    if (locationstring==NULL)
    {
        return PSTRAIT_LOCATION_NONE;
    }

    for (int i = 0; i < PSTRAIT_LOCATION_COUNT; i++)
    {
        if (!strcasecmp(locationstring,psTrait::locationString[i]))
            return (PSTRAIT_LOCATION)i;
    }

    return PSTRAIT_LOCATION_NONE;
}

bool CacheManager::PreloadTraits()
{
    unsigned int currentrow;
    psTrait *newtrait;
    Result result(db->Select("SELECT * from traits order by id"));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        newtrait=new psTrait;

        PSTRAIT_LOCATION loc;

        newtrait->uid               = result[currentrow].GetUInt32("id");
        newtrait->next_trait_uid    = result[currentrow].GetUInt32("next_trait");
        newtrait->raceID            = result[currentrow].GetUInt32("race_id");
        newtrait->name              = result[currentrow]["name"];
        newtrait->cstr_id_mesh      = result[currentrow].GetUInt32("cstr_id_mesh");
        newtrait->cstr_id_material  = result[currentrow].GetUInt32("cstr_id_material");
        newtrait->cstr_id_texture   = result[currentrow].GetUInt32("cstr_id_texture");
        newtrait->onlyNPC           = result[currentrow].GetInt("only_npc") != 0;
        newtrait->shaderVar         = result[currentrow]["shader"];

        psRaceInfo * raceInfo = GetRaceInfoByID(newtrait->raceID);
        if (raceInfo == NULL)
        {
            Error3("Trait (%u) references unresolvable race  %s.",
                newtrait->uid,result[currentrow]["race_id"]);
            delete newtrait;
            continue;
        }

        newtrait->race = raceInfo->race;
        newtrait->gender = raceInfo->gender;

        loc=ConvertTraitLocationString(result[currentrow]["location"]);
        if (loc==PSTRAIT_LOCATION_NONE)
        {
            Error3("Trait (%u) references unresolvable location  %s.",
                newtrait->uid,result[currentrow]["location"]);
            delete newtrait;
            continue;
        }
        newtrait->location = loc;

        traitlist.Push(newtrait);
    }
    // Update cross ref to next_trait
    for (size_t i = 0; i < traitlist.GetSize(); i++)
    {
         unsigned int next_uid = traitlist[i]->next_trait_uid;
         traitlist[i]->next_trait = GetTraitByID(next_uid);
    }


    Notify2( LOG_STARTUP, "%zu Traits Loaded", traitlist.GetSize() );
    return true;
}

// TODO: This should be done faster, probably not with an array
psTrait *CacheManager::GetTraitByID(unsigned int id)
{
    size_t i;
    psTrait *currenttrait;

    for (i=0;i<traitlist.GetSize();i++)
    {
        currenttrait=traitlist.Get(i);
        if (currenttrait!=NULL && currenttrait->uid==id)
        {
            return currenttrait;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psTrait *CacheManager::GetTraitByName(const char *name)
{
/*
    int i;
    psTrait *currenttrait;

    for (i=0;i<traitlist.GetSize();i++)
    {
        currenttrait=traitlist.Get(i);
        if (currenttrait!=NULL && currenttrait->name == name)
            return currenttrait;
    }
    */
    return NULL;
}

CacheManager::TraitIterator CacheManager::GetTraitIterator()
{
    return traitlist.GetIterator();
}


PSCHARACTER_GENDER CacheManager::ConvertGenderString(const char *genderstring)
{
    if (genderstring==NULL)
        return PSCHARACTER_GENDER_NONE;

    switch (genderstring[0])
    {
    case 'M':
    case 'm':
        return PSCHARACTER_GENDER_MALE;
    case 'F':
    case 'f':
        return PSCHARACTER_GENDER_FEMALE;
    case 'N':
    case 'n':
        return PSCHARACTER_GENDER_NONE;
    };

    return PSCHARACTER_GENDER_NONE;
}


bool CacheManager::PreloadRaceInfo()
{
    unsigned int currentrow;
    psRaceInfo *newraceinfo;
    Result result(db->Select("SELECT * from race_info"));

    if (!result.IsValid())
    {
        return false;
    }


    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        newraceinfo=new psRaceInfo;

    if (newraceinfo->Load(result[currentrow]))
    {
        newraceinfo->LoadBaseSpeeds(psserver->GetObjectReg());
        raceinfolist.Push(newraceinfo);
    }
    else
    {
        delete newraceinfo;
    }
    }

    Notify2( LOG_STARTUP, "%lu Races Loaded", result.Count() );
    return true;
}

size_t CacheManager::GetRaceInfoCount()
{
    return raceinfolist.GetSize();
}

psRaceInfo *CacheManager::GetRaceInfoByIndex(int idx)
{
    if (idx<0 || (size_t)idx>=raceinfolist.GetSize())
    {
        return NULL;
    }

    return raceinfolist.Get(idx);
}


// TODO: This should be done faster, probably not with an array
psRaceInfo *CacheManager::GetRaceInfoByID(unsigned int id)
{
    size_t i;
    psRaceInfo *currentri;

    for (i=0;i<raceinfolist.GetSize();i++)
    {
        currentri=raceinfolist.Get(i);
        if (currentri && currentri->uid==id)
        {
            return currentri;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psRaceInfo *CacheManager::GetRaceInfoByNameGender(const char *name,PSCHARACTER_GENDER gender)
{
    size_t i;
    psRaceInfo *currentri;
    csString raceName(name);

    for (i=0;i<raceinfolist.GetSize();i++)
    {
        currentri=raceinfolist.Get(i);
        if (currentri!=NULL && currentri->gender==gender && raceName.CompareNoCase(currentri->name))
        {
            return currentri;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psRaceInfo *CacheManager::GetRaceInfoByNameGender( unsigned int id, PSCHARACTER_GENDER gender)
{
    size_t i;
    psRaceInfo *currentri;

    for (i=0;i<raceinfolist.GetSize();i++)
    {
        currentri=raceinfolist.Get(i);
        // If the current race matches the race we're looking for, and only has the 'none' gender,
        // it's the race we need.
        if (currentri!=NULL &&
            currentri->race==id &&
            (currentri->gender == PSCHARACTER_GENDER_NONE || currentri->gender==gender))
        {
            return currentri;
        }
    }
    return NULL;
}


psRaceInfo *CacheManager::GetRaceInfoByMeshName(const csString & meshname)
{
    size_t i;
    psRaceInfo *currentri;

    for (i=0;i<raceinfolist.GetSize();i++)
    {
        currentri=raceinfolist.Get(i);
        csString meshName(currentri->mesh_name);
        if (currentri!=NULL && meshName==meshname)
        {
            return currentri;
        }
    }
    return NULL;
}


psItemCategory *CacheManager::GetItemCategoryByID(unsigned int id)
{
    size_t i;
    for (i=0;i<itemCategoryList.GetSize();i++)
    {
        psItemCategory *currentCategory;
        currentCategory=itemCategoryList.Get(i);
        if (currentCategory && id==currentCategory->id)
        {
             return currentCategory;
        }
    }
    return NULL;

}

psItemCategory *CacheManager::GetItemCategoryByName(const csString & name)
{
    size_t i;
    for (i=0;i<itemCategoryList.GetSize();i++)
    {
        psItemCategory *currentCategory;
        currentCategory=itemCategoryList.Get(i);
        if (currentCategory && name==currentCategory->name)
        {
            return currentCategory;
        }
    }
    return NULL;

}

// TODO:  This function needs to be implemented in a fast fashion
psWay *CacheManager::GetWayByID(unsigned int id)
{
    size_t i;
    for (i=0;i<wayList.GetSize();i++)
    {
        psWay *currentWay;
        currentWay=wayList.Get(i);
        if (currentWay && id==currentWay->id)
        {
            return currentWay;
        }
    }
    return NULL;
}

// TODO:  This function needs to be implemented in a fast fashion
psWay *CacheManager::GetWayByName(const csString & name)
{
    size_t i;
    for (i=0;i<wayList.GetSize();i++)
    {
        psWay *currentWay;
        currentWay=wayList.Get(i);
        if (currentWay && name==currentWay->name)
        {
            return currentWay;
        }
    }
    return NULL;
}

Faction *CacheManager::GetFaction(const char *name)
{
    Faction fac;
    fac.name = name;
    return factions.Find(&fac);
}

Faction *CacheManager::GetFactionByName(const char *name)
{
    if (name==NULL)
        return NULL;

    csHash<Faction*, int>::GlobalIterator it (factions_by_id.GetIterator ());
    while (it.HasNext ())
    {
        Faction* faction = it.Next();
        if (!strcasecmp(faction->name, name))
            return faction;
    }
    return NULL;
}

Faction *CacheManager::GetFaction(int id)
{
    return factions_by_id.Get(id,0);
}


// TODO:  This function needs to be implemented in a fast fashion
psSpell *CacheManager::GetSpellByID(unsigned int id)
{
    size_t i;
    for (i=0;i<spellList.GetSize();i++)
    {
        psSpell *currentSpell;
        currentSpell=spellList.Get(i);
        if (currentSpell && id==(unsigned int)currentSpell->GetID())
            return currentSpell;
    }
    return NULL;
}

// TODO:  This function needs to be implemented in a fast fashion
psSpell *CacheManager::GetSpellByName(const csString & name)
{
    size_t i;
    for (i=0;i<spellList.GetSize();i++)
    {
        psSpell *currentSpell;
        currentSpell=spellList.Get(i);
        if (currentSpell && name.CompareNoCase(currentSpell->GetName()))
            return currentSpell;
    }
    return NULL;
}

CacheManager::SpellIterator CacheManager::GetSpellIterator()
{
    return spellList.GetIterator();
}


// Get item basic stats by hashed table
psItemStats *CacheManager::GetBasicItemStatsByName(csString name)
{
    psItemStats *itemstats = itemStats_NameHash.Get(name.Downcase(),NULL);
    if(itemstats)
    {
        return itemstats;
    }

    // Not found in cache so look it up in DB.
    bool loaded;
    csString escape;
    db->Escape( escape, name );

    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R') and name='%s'", (const char *) name));

    if (!result.IsValid() || result.Count() == 0)
    {
        return NULL;
    }

    itemstats = new psItemStats;
    loaded = itemstats->ReadItemStats(result[0]);

    // Prevent id conflicts
    if (!loaded|| itemStats_IDHash.Get(itemstats->GetUID(),NULL))
    {
        if(loaded)
        {
            CPrintf(CON_ERROR, "Duplicate item_stats ID where id='%u' found.\n", itemstats->GetUID());
        }

        delete itemstats;
        return NULL;
    } else
    {
        CS_ASSERT( itemstats->GetUID() != 0 );

        itemStats_IDHash.Put(itemstats->GetUID(),itemstats);
        itemStats_NameHash.Put(itemstats->GetDownCaseName(),itemstats);
    }
    return itemstats;
}

// return id of item if 'name' exists already
uint32 CacheManager::BasicItemStatsByNameExist(csString name)
{
    psItemStats *itemstats = itemStats_NameHash.Get(name.Downcase(), NULL);
    if (itemstats)
    {
        return itemstats->GetUID();
    }

    return 0;
}

// Get item basic stats by hashed table
psItemStats *CacheManager::GetBasicItemStatsByID(uint32 id)
{
    if ( id == 0 )
    {
        return NULL;
    }

    psItemStats *itemstats = itemStats_IDHash.Get(id,NULL);
    if(itemstats)
    {
        return itemstats;
    }

    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R')  and id='%u'", id));

    if (!result.IsValid() || result.Count() == 0)
    {
        return NULL;
    }

    itemstats = new psItemStats;
    bool loaded = itemstats->ReadItemStats(result[0]);
    // Prevent name conflicts
    if (!loaded || itemStats_NameHash.Get(itemstats->GetDownCaseName(),NULL))
    {
        if(loaded)
        {
            CPrintf(CON_ERROR, "Duplicate item_stats name where name='%s' found.\n", itemstats->GetName());
        }
        delete itemstats;
        return NULL;
    } else
    {
        CS_ASSERT( itemstats->GetUID() != 0 );

        itemStats_IDHash.Put(itemstats->GetUID(),itemstats);
        itemStats_NameHash.Put(itemstats->GetDownCaseName(),itemstats);
    }
    return itemstats;
}

/// If an item changes name (eg book title) keep cache up to date
void CacheManager::CacheNameChange(csString oldName, csString newName)
{
    if (oldName.Length() > 0 && newName.Length() > 0)
    {
        /// first remove old name
        psItemStats *itemstats = itemStats_NameHash.Get(oldName.Downcase(), NULL);
        if (itemstats)
        {
            itemStats_NameHash.Delete(oldName.Downcase(), itemstats);

            /// add new one
            itemStats_NameHash.Put(newName.Downcase(), itemstats);
        }
        else
        {
            Error2("CacheNameChange error: no item stats for \"%s\".", oldName.GetDataSafe());
        }
    }
    else
    {
        Error1("CacheNameChange error: old or new name zero length.");
    }
}

void CacheManager::GetTipByID(int id, csString& tip)
{
    if ((size_t)id>=tips_list.GetSize())
    {
        tip.Clear();
        return;
    }

    tip = tips_list.Get(id);
}

unsigned int CacheManager::GetTipLength()
{
    return (unsigned int)tips_list.GetSize();
}

const char* CacheManager::Attribute2String( PSITEMSTATS_STAT s )
{
    switch ( s )
    {
        case PSITEMSTATS_STAT_STRENGTH:
            return "STRENGTH";
        case PSITEMSTATS_STAT_AGILITY:
            return "AGILITY";
        case PSITEMSTATS_STAT_ENDURANCE:
            return "ENDURANCE";
        case PSITEMSTATS_STAT_INTELLIGENCE:
            return "INTELLIGENCE";
        case PSITEMSTATS_STAT_WILL:
            return "WILL";
        case PSITEMSTATS_STAT_CHARISMA:
            return "CHARISMA";
        default:
            return "None";
    }
}

PSITEMSTATS_STAT CacheManager::ConvertAttributeString(const char *attributestring)
{
    if (attributestring==NULL)
        return PSITEMSTATS_STAT_NONE;

    if (!strcasecmp(attributestring,"STRENGTH") || !strcasecmp(attributestring,"STR"))
        return PSITEMSTATS_STAT_STRENGTH;
    if (!strcasecmp(attributestring,"AGILITY") || !strcasecmp(attributestring,"AGI"))
        return PSITEMSTATS_STAT_AGILITY;
    if (!strcasecmp(attributestring,"ENDURANCE") || !strcasecmp(attributestring,"END") )
        return PSITEMSTATS_STAT_ENDURANCE;
    if (!strcasecmp(attributestring,"INTELLIGENCE") || !strcasecmp(attributestring,"INT"))
        return PSITEMSTATS_STAT_INTELLIGENCE;
    if (!strcasecmp(attributestring,"WILL") || !strcasecmp(attributestring,"WIL"))
        return PSITEMSTATS_STAT_WILL;
    if (!strcasecmp(attributestring,"CHARISMA") || !strcasecmp(attributestring,"CHA"))
        return PSITEMSTATS_STAT_CHARISMA;
    return PSITEMSTATS_STAT_NONE;
}


PSSKILL CacheManager::ConvertSkillString(const char *skillstring)
{
    if (skillstring==NULL)
        return PSSKILL_NONE;

    psSkillInfo *skillinfo=GetSkillByName(skillstring);

    if (skillinfo==NULL)
        return PSSKILL_NONE;
    return skillinfo->id;
}

PSSKILL CacheManager::ConvertSkill(int skill_id)
{
    if (skill_id >= (int)PSSKILL_NONE && skill_id < (int)PSSKILL_COUNT)
    {
        return (PSSKILL)skill_id;
    }

    return PSSKILL_NONE;
}

bool CacheManager::PreloadItemCategories()
{
    Result categories(db->Select("SELECT * from item_categories"));

    if (categories.IsValid())
    {
        int i,count=categories.Count();

        for (i=0;i<count;i++)
        {
            psItemCategory *category = new psItemCategory;

            category->id                   = categories[i].GetInt("category_id");
            category->name                 = categories[i]["name"];
            category->repairToolStatId  = categories[i].GetInt("item_stat_id_repair_tool");
            const char *flag = categories[i]["is_repair_tool_consumed"];
            category->repairToolConsumed = (flag && flag[0]=='Y');
            category->repairSkillId      = categories[i].GetInt("skill_id_repair");
            category->identifySkillId    = categories[i].GetInt("identify_skill_id");
            category->identifyMinSkill   = categories[i].GetInt("identify_min_skill");

            itemCategoryList.Push(category);
         }
    }
    Notify2( LOG_STARTUP, "%lu Item Categories Loaded", categories.Count() );
    return true;
}

bool CacheManager::PreloadWays()
{
    Result ways(db->Select("SELECT * from ways"));
    if (ways.IsValid())
    {
        int i,count=ways.Count();

        for (i=0;i<count;i++)
        {
            psWay *way = new psWay;
            way->id =   atoi(ways[i]["id"]);
            way->name = ways[i]["name"];
            if (way->name == "Crystal")
            {
                way->skill = PSSKILL_CRYSTALWAY;
                way->related_stat = PSSKILL_CHA;
            }
            else
            if (way->name == "Azure")
            {
                way->skill = PSSKILL_AZUREWAY;
                way->related_stat = PSSKILL_WILL;
            }
            else
            if (way->name == "Red")
            {
                way->skill = PSSKILL_REDWAY;
                way->related_stat = PSSKILL_WILL;
            }
            else
            if (way->name == "Dark")
            {
                way->skill = PSSKILL_DARKWAY;
                way->related_stat = PSSKILL_CHA;
            }
            else
            if (way->name == "Brown")
            {
                way->skill = PSSKILL_BROWNWAY;
                way->related_stat = PSSKILL_INT;
            }
            else
            if (way->name == "Blue")
            {
                way->skill = PSSKILL_BLUEWAY;
                way->related_stat = PSSKILL_INT;
            }
            else
            {
                Error2("Unknown WAY: %s",way->name.GetData());
            }

            wayList.Push(way);
        }
    }

    Notify2( LOG_STARTUP, "%lu Ways Loaded", ways.Count() );
    return true;
}

bool CacheManager::PreloadFactions()
{
    Result result_factions(db->Select("SELECT * from factions"));

    unsigned int x = 0;

    if ( result_factions.IsValid() )
    {
        for ( x = 0; x < result_factions.Count(); x++ )
        {
            Faction *f = new Faction;
            f->id = atoi( result_factions[x]["id"] );
            f->name = result_factions[x]["faction_name"];
            f->description = result_factions[x]["faction_description"];
            f->weight = atof( result_factions[x]["faction_weight"] );
            // Parses the script for factions dynamic life events and populates the arrays for it
            // used to store the script to be parsed to generate dynamic life events
            csString factionCharacterEvents = result_factions[x]["faction_character"];
            // Parsing of the script
            while(factionCharacterEvents.Length())
            {
                //get the value from the string
                size_t cutpos = factionCharacterEvents.FindFirst(' ', 0);
                if(cutpos == (size_t) -1) //script error or finished parsing bail out
                    break;
                size_t cutpos2 = factionCharacterEvents.FindFirst('\n', 0);
                if(cutpos2 == (size_t) -1) //script error or finished parsing bail out
                    break;

                //get the value of the faction which will trigger the text
                int value = atoi(factionCharacterEvents.Slice(0,cutpos).GetDataSafe());
                if(value == 0) //script error or finished parsing bail out
                    break;

                //take out the string we need
                csString entry_text = factionCharacterEvents.Slice(cutpos+1, cutpos2-cutpos-1);

                //prepare the struct containing the parsed data
                FactionLifeEvent * factionevt = new FactionLifeEvent;
                factionevt->event_description = entry_text;
                factionevt->value = abs(value); //we want a positive value for checking later
                if(value > 0) //check what cshash should get the parsed data
                    f->PositiveFactionEvents.Put(0, factionevt); //this was a positive value
                else
                    f->NegativeFactionEvents.Put(0, factionevt); //this was a negative value

                //prepare for the next line
                factionCharacterEvents = factionCharacterEvents.Slice(cutpos2+1);
            }

            // Stored two different ways
            factions.Insert(f,TREE_OWNS_DATA);
            factions_by_id.Put(f->id,f);
        }
    }

    Notify2( LOG_STARTUP, "%lu Factions Loaded", result_factions.Count() );
    return true;
}


bool CacheManager::PreloadSpells()
{
    Result spells(db->Select("SELECT * from spells"));
    if (spells.IsValid())
    {
        int i,count=spells.Count();

        for (i=0;i<count;i++)
        {
            psSpell *spell = new psSpell;
            if (spell->Load(spells[i]))
            {
                spellList.Push(spell);
            }
            else
            {
                delete spell;
            }
        }
    }
    Notify2( LOG_STARTUP, "%lu Spells Loaded", spells.Count() );
    return true;
}

csPDelArray<psItemAnimation> *CacheManager::FindAnimationList(int id)
{
    for (size_t x=0; x<item_anim_list.GetSize(); x++)
    {
        if (item_anim_list[x]->Get(0)->id == id)
        {
            return item_anim_list[x];
        }
    }
    return NULL;
}

bool CacheManager::PreloadItemAnimList()
{
    unsigned int currentrow,lastarrayid=0;
    psItemAnimation *newitem;
    csPDelArray<psItemAnimation> *newarray;

    Result result(db->Select("SELECT * from item_animations order by id, min_use_level"));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        unsigned int id = result[currentrow].GetInt("id");
        if (id != lastarrayid) // new set
        {
            newarray = new csPDelArray<psItemAnimation>;
            item_anim_list.Push(newarray);
            lastarrayid = id;
        }
        newitem            = new psItemAnimation;
        newitem->id        = id;
        newitem->anim_name = FindCommonString( result[currentrow].GetInt("cstr_id_animation") );
        newitem->anim_id   = csInvalidStringID;
        newitem->flags     = result[currentrow].GetInt("type_flags");
        newitem->min_level_required = result[currentrow].GetInt("min_use_level");

        newarray->Push(newitem);
    }

    Notify2( LOG_STARTUP, "%lu Item Animations Loaded", result.Count() );
    return true;
}

psItemSet *CacheManager::LoadWorldItems(psSectorInfo *sector,int &loadeditems)
{
    loadeditems=0;


    /* We have 6 sets of items in game:
     * - items on the ground (loc_sector_id!=0)
     * - containers on the ground (loc_sector_id!=0)
     * - items inside containers on the ground (parent_item_id object has loc_sector_id!=0)
     *
     * - items inside player inventory (location='I' or location='E')
     * - containers in player inventory (location='I' or location='E')
     * - items inside containers in player inventory (parent_item_id object has location='I' or location='E')
     *
     * Here we want to load only the first 3 sets
     */

    Result result(sector ?
        db->Select("SELECT * from item_instances where loc_sector_id='%u'",sector->uid) :
        // it's an object on the ground, not held by players
        // or the parent container is an object on the ground, not held by players
        db->Select("SELECT * from item_instances"
                   " where ifnull(loc_sector_id,0)!=0"
                   " or parent_item_id IN (select id "
                                           " from item_instances "
                                           "where ifnull(loc_sector_id,0)!=0) order by parent_item_id"));

    psItemSet *itemset = new psItemSet;

    // Load items
    if ( result.IsValid() )
    {
        int i, count=result.Count();

        for (i=0;i<count;i++)
        {
            psItem *item;
            unsigned int stats_id=result[i].GetUInt32("item_stats_id_standard");
            psItemStats *stats= GetBasicItemStatsByID(stats_id);

            if (!stats)
            {
                Error3("Error in LoadWorldItems! Item instance %lu could not load base stats %lu. Check the item_stats_id_standard value for this item. Skipping.\n",
                    result[i].GetUInt32("id"), result[i].GetUInt32("item_stats_id_standard"));
                continue;
            }

            if (stats->GetIsGlyph())
            {
                item = new psGlyph();
            }
            else
            {
                item = new psItem();
            }

            if (!item->Load(result[i]))
            {
                Error2("Error in LoadWorldItems! Item instance %lu could not be loaded. Skipping.\n",result[i].GetUInt32("id"));
                delete item;
                continue;
            }

            // This item is in another item.  Update the parent item id list and then move on.
            /*
            if ( item->GetContainerID() && !item->GetLocInParent() )
            {
                // Field missing.  Needed for items in other items.
                Error2("Item with id %s has parent item without location in parent.\n",result[i]["id"]);
                delete item;
                continue;
            }
            */

            // printf("Loaded world item %d: %s\n", item->GetUID(), item->GetName() );

            loadeditems++;
            itemset->Add(item,0); // parentid not used anymore KWF
        }
    }
    else
    {
        Error3("Error loading world items.\nQuery: %s\nError: %s",db->GetLastQuery(), db->GetLastError() );
        delete itemset;
        return NULL;
    }

    for (size_t i=0; i < itemset->GetSize(); i++)
    {
        if (itemset->Get(i))
        {
            itemset->Get(i)->SetLoaded();
        }
    }

    return itemset;
}

void CacheManager::AddItemStatsToHashTable(psItemStats* newitem)
{
    itemStats_IDHash.Put(newitem->GetUID(),newitem);
    itemStats_NameHash.Put(newitem->GetDownCaseName(),newitem);
}

bool CacheManager::PreloadItemStatsDatabase()
{
    uint32 currentrow;
    psItemStats *newitem;
    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R') "));

    if (!result.IsValid())
    {
        return false;
    }

    for (currentrow=0; currentrow<result.Count(); currentrow++)
    {
        newitem = new psItemStats;
        bool loaded = newitem->ReadItemStats(result[currentrow]);
        // Prevent name conflicts
        if (!loaded || itemStats_NameHash.Get(newitem->GetDownCaseName(), NULL))
        {
            if(loaded)
            {
                CPrintf(CON_ERROR, "Duplicate item_stats name where name='%s' found.\n", newitem->GetName());
            }
            else
            {
                CPrintf(CON_ERROR, "Failed to load item_stats with id='%s' and name='%s'\n",
                        result[currentrow]["id"],result[currentrow]["name"]);
            }

            delete newitem;
            return false;
        } else
        {
            CS_ASSERT( newitem->GetUID() != 0 );

            AddItemStatsToHashTable(newitem);
        }
    }
    Notify2( LOG_STARTUP, "%lu Item Stats Loaded", result.Count() );
    return true;
}

psItemStats* CacheManager::CopyItemStats(uint32 id, csString newName)
{
    // NOTE: This -must- match the schema of item_stats, except for UNIQUE KEY 'name' which is handled below.
    const char *fields = "stat_type, weight, visible_distance, size, container_max_size, valid_slots, flags, decay_rate, item_skill_id_1, item_skill_id_2, item_skill_id_3, item_bonus_1_attr, item_bonus_2_attr, item_bonus_3_attr, item_bonus_1_max, item_bonus_2_max, item_bonus_3_max, dmg_slash, dmg_blunt, dmg_pierce, weapon_speed, weapon_penetration, weapon_block_targeted, weapon_block_untargeted, weapon_counterblock, armor_hardness, cstr_id_gfx_mesh, cstr_id_gfx_icon, cstr_id_gfx_texture, cstr_id_part, cstr_id_part_mesh, armorvsweapon_type, category_id, base_sale_price, item_type, requirement_1_name, requirement_1_value, requirement_2_name, requirement_2_value, requirement_3_name, requirement_3_value, item_type_id_ammo, spell_id_on_hit, spell_on_hit_prob, spell_id_feature, spell_feature_charges, spell_feature_timing, item_anim_id, description, sound, item_max_quality, prg_evt_equip, prg_evt_unequip, creative_definition";

    csString sql;

    sql.Format("INSERT INTO item_stats (%s) SELECT %s FROM item_stats WHERE id = '%%u'",
               fields, fields);

    psItemStats *newItem;

    if ( db->Command( sql.GetData(), id ) == QUERY_FAILED)
    {
        Error4("Error while copying item stats for id %d.\nSQL:%s\nError:%s\n",id,db->GetLastQuery(),db->GetLastError());
        return NULL;
    }

    id = db->GetLastInsertID();
    CS_ASSERT(id != 0);
    csString escape;
    db->Escape( escape, newName );

    // change to the new name for the new item
    if (db->Command("UPDATE item_stats SET name = \'%s\' WHERE id = \'%u\'", escape.GetDataSafe(), id) == QUERY_FAILED)
    {
        Error4("Error while renaming copied item_stats for id %d.\nSQL:%s\nError:%s\n", id, db->GetLastQuery(), db->GetLastError());
        db->Command("DELETE FROM item_stats WHERE id = \'%u\'", id);
        return NULL;
    }

    Result result( db->Select( "SELECT * from item_stats where id='%u'", id ) );

    if ( !result.IsValid() )
    {
        return NULL;
    }

    if ( result.Count() != 1 )
    {
        return NULL;
    }

    newItem = new psItemStats;
    if ( !newItem->ReadItemStats( result[0] ) )
    {
        delete newItem;
        return NULL;
    }
//    basicitemstatslist.Push( newItem );
    itemStats_IDHash.Put(newItem->GetUID(),newItem);
    itemStats_NameHash.Put(newName.Downcase(), newItem);
    return newItem;
}

bool CacheManager::PreloadBadNames()
{
    Result result(db->Select("SELECT * from bad_names"));

    if (!result.IsValid())
    {
        Error1("Couldn't load bad names!");
        return false;
    }

    for (unsigned int i = 0; i < result.Count(); i++)
    {
        bad_names.Push(NormalizeCharacterName(result[i]["name"]));
    }

    Notify2( LOG_STARTUP, "%lu Bad Names Loaded", result.Count() );
    return true;
}

void CacheManager::AddBadName(const char* name)
{
    if (!name)
        return;

    csString newname = NormalizeCharacterName(name);

    db->Command("INSERT INTO bad_names (`name`) VALUES ('%s')", newname.GetDataSafe());

    bad_names.Push(newname);
}

void CacheManager::DelBadName(const char* name)
{
    csString cname = NormalizeCharacterName(name);

    for(size_t i = 0; i < bad_names.GetSize();i++)
    {
        if(cname.CompareNoCase(bad_names[i]))
        {
            bad_names.DeleteIndex(i);
            db->Command("DELETE FROM bad_names WHERE name='%s'",cname.GetData());
            return;
        }
    }
}

size_t CacheManager::GetBadNamesCount()
{
    return bad_names.GetSize();
}

const char* CacheManager::GetBadName(int pos)
{
     return bad_names[pos];
}

psAccountInfo *CacheManager::GetAccountInfoByID(AccountID accountid)
{
    Result result(db->Select("SELECT * from accounts where id=%u", accountid.Unbox()));
    if (!result.IsValid() || result.Count()<1)
    {
        Warning3(LOG_CONNECTIONS, "Could not find %s: %s", ShowID(accountid), db->GetLastError());
        return NULL;
    }
    psAccountInfo *accountinfo=new psAccountInfo;
    if (accountinfo->Load(result[0]))
    {
        return accountinfo;
    }
    else
    {
        delete accountinfo;
        return NULL;
    }
}


psAccountInfo *CacheManager::GetAccountInfoByCharID(PID charid)
{
    unsigned int accountid = db->SelectSingleNumber("SELECT account_id from characters where id=%u", charid.Unbox());
    return GetAccountInfoByID( accountid );
}

psAccountInfo *CacheManager::GetAccountInfoByUsername(const char *username)
{
    iCachedObject *obj = RemoveFromCache(username);
    if (obj)
    {
        Notify2(LOG_CACHE, "Found account for %s in cache!", username);
        return (psAccountInfo *)obj->RecoverObject();
    }

    csString escape;
    db->Escape( escape, username );
    Result result(db->Select("SELECT * from accounts where username='%s'",escape.GetData()));
    if (!result.IsValid() || result.Count()<1)
    {
        Warning3(LOG_CONNECTIONS,"Could not find account for login %s.  Error: %s",username,db->GetLastError());
        return NULL;
    }
    psAccountInfo *accountinfo=new psAccountInfo;
    if (accountinfo->Load(result[0]))
    {
        return accountinfo;
    }
    else
    {
        delete accountinfo;
        return NULL;
    }
}


bool CacheManager::UpdateAccountInfo(psAccountInfo *ainfo)
{
    const char *fieldnames[]= {
        "username",
        "password",
        "last_login_ip",
        "security_level",
        "last_login"
    };
    char accountidstring[11];
    psStringArray fields;

    fields.Push(ainfo->username);
    fields.Push(ainfo->password);
    fields.Push(ainfo->lastloginip);
    fields.FormatPush("%d",ainfo->securitylevel);
    fields.Push(ainfo->lastlogintime);
    snprintf(accountidstring,11,"%u",ainfo->accountid);
    accountidstring[10]=0x00;

    if (!db->GenericUpdateWithID("accounts","id",accountidstring,fieldnames,fields))
    {
        Error3("Failed to update account %u. Error %s",ainfo->accountid,db->GetLastError());
        return false;
    }
    return true;
}

unsigned int CacheManager::NewAccountInfo(psAccountInfo *ainfo)
{
    const char *fieldnames[]= {
        "username",
        "password",
        "last_login_ip",
        "security_level"
    };
    psStringArray fields;

    fields.Push(ainfo->username);
    fields.Push(ainfo->password);
    fields.Push(ainfo->lastloginip);
    fields.FormatPush("%d",ainfo->securitylevel);

    unsigned int id=db->GenericInsertWithID("accounts",fieldnames,fields);

    if (id == 0)
    {
        Error3("Failed to create new account for user %s. Error %s",ainfo->username.GetData(),db->GetLastError());
        return false;
    }

    ainfo->accountid = id;
    return id;
}

psGuildInfo *CacheManager::FindGuild(unsigned int id)
{
    psGuildInfo *g = guildinfo_by_id.Get(id, 0);
    if (g)
    {
        return g;
    }

    // Load on demand if not found
    g = new psGuildInfo;
    if (g->Load(id))
    {
        guildinfo_by_id.Put(g->id,g);
        return g;
    }
    delete g;
    return NULL;
}

psGuildInfo * CacheManager::FindGuild(const csString & name)
{
    unsigned int id;
    csString escape;
    db->Escape( escape, name );
    Result result(db->Select("select id from guilds where name=\"%s\"", escape.GetData()) );
    if (!result.IsValid())
    {
        return NULL;
    }

    if (result.Count() == 0)
    {
        return NULL;
    }

    id = result[0].GetInt("id");

    return FindGuild(id);
}

bool CacheManager::CreateGuild(const char *guildname, Client *client)
{
    PID leaderID = client->GetPID();

    psGuildInfo *gi = new psGuildInfo(guildname,leaderID);

    if (!gi->InsertNew(leaderID))
    {
        delete gi;
        return false;
    }

    gi->AddNewMember(client->GetCharacterData(), MAX_GUILD_LEVEL);

    guildinfo_by_id.Put(gi->id,gi);
    return true;
}

void CacheManager::RemoveGuild(psGuildInfo *which)
{
    guildinfo_by_id.Delete(which->id,which);
    delete which;
}




psGuildAlliance * CacheManager::FindAlliance(unsigned int id)
{
    psGuildAlliance *a = alliance_by_id.Get(id, 0);
    if (a)
    {
        return a;
    }

    // Load on demand if not found
    a = new psGuildAlliance();
    if (a->Load(id))
    {
        alliance_by_id.Put(id,a);
        return a;
    }
    delete a;
    return NULL;
}

bool CacheManager::CreateAlliance(const csString & name, psGuildInfo * founder, Client *client)
{
    psGuildAlliance *a = new psGuildAlliance(name);

    if (!a->InsertNew())
    {
        delete a;
        return false;
    }

    a->AddNewMember(founder);
    a->SetLeader(founder);

    alliance_by_id.Put(a->GetID(),a);
    return true;
}

bool CacheManager::RemoveAlliance(psGuildAlliance *which)
{
    alliance_by_id.Delete(which->GetID(),which);
    which->RemoveAlliance();
    delete which;
    return true;
}


float CacheManager::GetArmorVSWeaponResistance(const char* armor_type, const char* weapon_type)
{
    if(!weapon_type || !armor_type || strlen(armor_type) < 2)
        return 1.0f;

    // Loopide loop
    for(size_t i = 0;i < armor_vs_weapon.GetSize();i++)
    {
        ArmorVsWeapon* atbl = armor_vs_weapon.Get(i);
        if(!atbl)
            continue;

        if(atbl->weapontype == weapon_type)
        {
            // Get the float in our 2d array
            csString armor = armor_type;
            char lvl = armor.GetAt(1);
            int arrayLvl = 0;

            int cat = atoi(csString(armor.GetAt(0)));
            if(lvl == 'a')
                arrayLvl = 0;
            else if(lvl =='b')
                arrayLvl = 1;
            else if(lvl == 'c')
                arrayLvl = 2;
            else
                arrayLvl = 3;

            return atbl->c[cat-1][arrayLvl];
        }
    }

    // Not found
    Debug2(LOG_COMBAT,0,"Didn't find weapon %s in the armor vs weapon table, assuming value is 1.0\n",weapon_type);
    return 1.0f;
}

float CacheManager::GetArmorVSWeaponResistance(psItemStats* armor, psItemStats* weapon)
{
    if(!armor || !weapon)
        return 1.0f;

    // Get the values to use
    csString armorstr,weaponstr;
    armor->GetArmorVsWeaponType(armorstr);
    weapon->GetArmorVsWeaponType(weaponstr);

    return GetArmorVSWeaponResistance(armorstr,weaponstr);
}

const char *CacheManager::MakeCacheName(const char *prefix, uint32 id)
{
    sprintf(CacheNameBuffer,"%.5s%u",prefix,id);
    return CacheNameBuffer;
}

void CacheManager::AddToCache(iCachedObject *obj, const char *name, int max_cache_time_seconds)
{
    Debug4(LOG_CACHE,0,"Now adding object <%s:%p> to cache for %d seconds.",name,obj,max_cache_time_seconds);

    // Set up the object which the event trigger will use to recover pointers or cancel itself.
    CachedObject *newRecord = new CachedObject;

    newRecord->name = name;
    newRecord->object = obj;

    // Now create the timed event which will make the cache clean it up later
    psCacheExpireEvent *evt = new psCacheExpireEvent(max_cache_time_seconds*1000, newRecord);

    // Setting this enables removal from cache to clear this event
    newRecord->event = evt;

    // Now queue it
    psserver->GetEventManager()->Push(evt);

    // Ensure no duplicate keys in generic cache, and clean up after self if found
    iCachedObject *oldObject = RemoveFromCache(newRecord->name);

    if (oldObject)
    {
        oldObject->ProcessCacheTimeout(); // Make sure object knows it is gone
        oldObject->DeleteSelf();
    }

    // Now add the new one back in
    generic_object_cache.Put(newRecord->name, newRecord);
}

iCachedObject *CacheManager::RemoveFromCache(const char *name)
{
    CachedObject *oldRecord = generic_object_cache.Get(name, NULL);
    if (oldRecord)
    {
        oldRecord->event->CancelEvent();
        generic_object_cache.DeleteAll(oldRecord->name);
        iCachedObject *save = oldRecord->object;
        delete oldRecord;
        Notify2(LOG_CACHE,"Found object in cache and returning ptr %p.",save);
        return save;
    }
    Debug2(LOG_CACHE,0,"Object <%s> not found in cache.",name);
    return NULL;
}

CacheManager::psCacheExpireEvent::psCacheExpireEvent(int delayticks,CachedObject *object)
: psGameEvent(0,delayticks,"psCacheExpireEvent")
{
    valid = true;
    myObject = object;
}

void CacheManager::psCacheExpireEvent::Trigger()
{
    if (valid)
    {
        Debug2(LOG_CACHE,0,"Deleting object <%s> from cache due to timeout.",myObject->name.GetDataSafe());
        // Notify object it is going away
        myObject->object->ProcessCacheTimeout();
        // Delete the underlying object
        myObject->object->DeleteSelf();
        // Now remove the record from the cache last
        CacheManager::GetSingleton().RemoveFromCache(myObject->name);
    }
}
