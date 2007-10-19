/*
* pscharacterloader.cpp
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
#include <iutil/object.h>
#include <csutil/thread.h>
#include <csutil/stringarray.h>
#include <physicallayer/entity.h>
#include <propclass/mesh.h>

#include "util/log.h"
#include "util/gameevent.h"
#include "pscharacterloader.h"
#include "../psserver.h"
#include "../globals.h"
#include "../playergroup.h"
#include "../entitymanager.h"
#include "util/eventmanager.h"
#include "psglyph.h"
#include "rpgrules/factions.h"

#include "../gem.h"
#include "util/psdatabase.h"
#include "psraceinfo.h"
#include "psitem.h"
#include "../clients.h"
#include "pssectorinfo.h"
#include "pscharacter.h"
#include "../cachemanager.h"
#include "pscharacterlist.h"
#include <iengine/sector.h>
#include "gmeventmanager.h"

psCharacterLoader::psCharacterLoader()
{
}


psCharacterLoader::~psCharacterLoader()
{
}

bool psCharacterLoader::Initialize()
{

    // Per-instance initialization should go here

    return true;
}

bool psCharacterLoader::AccountOwner( const char* characterName, unsigned int accountID )
{
    csString escape;
    db->Escape(escape,characterName);
    unsigned int account = db->SelectSingleNumber("SELECT count(*) from characters where name='%s' and account_id=%d",escape.GetData(), accountID);
    //unsigned int account = db->SelectSingleNumber("SELECT account_id from characters where name='%s'",escape.GetData());
    return (account > 0);
}

psCharacterList *psCharacterLoader::LoadCharacterList(unsigned int accountid)
{
    // Check the generic cache first
    iCachedObject *obj = CacheManager::GetSingleton().RemoveFromCache(CacheManager::GetSingleton().MakeCacheName("list",accountid));
    if (obj)
    {
        Notify2(LOG_CACHE,"Returning char object %p from cache.",obj->RecoverObject());
        return (psCharacterList *)obj->RecoverObject();
    }

    Notify1(LOG_CACHE,"******LOADING CHARACTER LIST*******");
    // Load if not in cache
    Result result(db->Select("SELECT id,name,lastname from characters where account_id=%u order by id",accountid));

    if (!result.IsValid())
        return NULL;

    psCharacterList *charlist = new psCharacterList;

    charlist->SetValidCount( result.Count() );        
    for (unsigned int i=0;i<result.Count();i++)
    {
        charlist->SetEntryValid(i,true);
        charlist->SetCharacterID(i,result[i].GetInt("id"));
        charlist->SetCharacterFullName(i,result[i]["name"],result[i]["lastname"]);
    }
    return charlist;
}



/*  This uses a relatively slow method of looking up the list of ids of all npcs (optionally in a given sector)
*  and then loading each character data element with a seperate query. 
*/

psCharacter **psCharacterLoader::LoadAllNPCCharacterData(psSectorInfo *sector,int &count)
{
    psCharacter **charlist;
    unsigned int i;
    count=0;

    Result npcs( (sector)?
        db->Select("SELECT * from characters where characters.loc_sector_id='%u' and npc_spawn_rule>0",sector->uid)
        : db->Select("SELECT * from characters where npc_spawn_rule>0"));


    if ( !npcs.IsValid() )
    {
        Error3("Failed to load NPCs in Sector %s.   Error: %s",
            sector==NULL?"ENTIRE WORLD":sector->name.GetData(),db->GetLastError());
        return NULL;
    }

    if (npcs.Count()==0)
    {
        Error2("No NPCs found to load in Sector %s.",
            sector==NULL?"ENTIRE WORLD":sector->name.GetData());
        return NULL;
    }

    charlist=new psCharacter *[npcs.Count()];

    for (i=0; i<npcs.Count(); i++)
    {
        charlist[count]=new psCharacter();
        if (!charlist[count]->Load(npcs[i]))
        {
            delete charlist[count];
            charlist[count]=NULL;
            continue;
        }
        count++;
    }

    return charlist;
}

psCharacter *psCharacterLoader::LoadCharacterData(unsigned int uid, bool forceReload)
{
    if (!forceReload)
    {
        // Check the generic cache first
        iCachedObject *obj = CacheManager::GetSingleton().RemoveFromCache(CacheManager::GetSingleton().MakeCacheName("char",uid));
        if (obj)
        {
            if (!forceReload)
            {
                Debug2(LOG_CACHE,uid,"Returning char object %p from cache.\n",obj->RecoverObject());

                psCharacter *charData = (psCharacter *)obj->RecoverObject();

                // Clear loot items
                if (charData)
                    charData->ClearLoot();

                return charData;
            }
        }
    }

    // Now load from the database if not found in cache
    csTicks start = csGetTicks();

    Result result(db->Select("SELECT * from characters where id=%u",uid));

    if (!result.IsValid())
    {
        Error2("Character data invalid for ID %u.",uid);
        return NULL;
    }

    if (result.Count()>1)
    {
        Error2("Character ID %u has multiple entries.  Check table constraints.",uid);
        return NULL;
    }
    if (result.Count()<1)
    {
        Error2("Character ID %u has no character entry!",uid);
        return NULL;
    }

    psCharacter *chardata = new psCharacter();

    Debug2(LOG_CACHE,uid,"New character data ptr is %p.",chardata);

    // Read basic stats
    if (!chardata->Load(result[0]))
    {
        if(csGetTicks() - start > 500)
        {
            csString status;
            status.Format("Warning: Spent %u time loading FAILED character ID %u", 
                csGetTicks() - start, uid);
            psserver->GetLogCSV()->Write(CSV_STATUS, status);
        }
        Error2("Load failed for character ID %u.",uid);
        delete chardata;
        return NULL;
    }
    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character ID %u", 
            csGetTicks() - start, uid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    return chardata;
}


psCharacter *psCharacterLoader::QuickLoadCharacterData(unsigned int uid, bool noInventory)
{
    Result result(db->Select("SELECT id, name, lastname, racegender_id from characters where id=%u LIMIT 1",uid));

    if (!result.IsValid() || result.Count() < 1)
    {
        Error2("Character data invalid for ID %u.",uid);
        return NULL;
    }

    psCharacter *chardata = new psCharacter();

    if (!chardata->QuickLoad(result[0], noInventory))
    {
        Error2("Quick load failed for character ID %u.",uid);
        delete chardata;
        return NULL;
    }

    return chardata;
}


bool psCharacterLoader::NewNPCCharacterData(unsigned int accountid,psCharacter *chardata)
{
    const char *fieldnames[]= {
            "account_id",
            "name",
            "lastname",
            "racegender_id",
            "character_type",
            "description", //
            "base_strength",
            "base_agility",
            "base_endurance",
            "base_intelligence",
            "base_will",
            "base_charisma",
            "mod_hitpoints",
            "base_hitpoints_max",
            "mod_mana",
            "base_mana_max",
            "stamina_physical",
            "stamina_mental",
            "money_circles",
            "money_trias",
            "money_hexas",
            "money_octas",
            "bank_money_circles",
            "bank_money_trias",
            "bank_money_hexas",
            "bank_money_octas",
            "loc_x",
            "loc_y",
            "loc_z",
            "loc_yrot",
            "loc_sector_id",
            "loc_instance",
            "faction_standings",
            "npc_spawn_rule",
            "npc_master_id",
            "npc_addl_loot_category_id",
            "npc_impervious_ind",
            "kill_exp",
            "animal_affinity",
    };
    psStringArray values;

    values.FormatPush("%d",accountid?accountid:chardata->GetAccount());
    values.FormatPush("%s",chardata->GetCharName());
    values.FormatPush("%s",chardata->GetCharLastName());
    values.FormatPush("%u",chardata->GetRaceInfo()->uid);
    values.FormatPush("%u",chardata->GetCharType() );
    values.FormatPush("%s",chardata->GetDescription());
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_STRENGTH, false));
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_AGILITY, false));
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_ENDURANCE, false));
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_INTELLIGENCE, false));
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_WILL, false));
    values.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_CHARISMA, false));
    values.FormatPush("%10.2f",chardata->GetHP());
    values.FormatPush("%10.2f",chardata->GetHitPointsMax());
    values.FormatPush("%10.2f",chardata->GetMana());
    values.FormatPush("%10.2f",chardata->GetManaMax());
    values.FormatPush("%10.2f",chardata->GetStamina(true));
    values.FormatPush("%10.2f",chardata->GetStamina(false));
    values.FormatPush("%u",chardata->Money().GetCircles());
    values.FormatPush("%u",chardata->Money().GetTrias());
    values.FormatPush("%u",chardata->Money().GetHexas());
    values.FormatPush("%u",chardata->Money().GetOctas());
    values.FormatPush("%u",chardata->BankMoney().GetCircles());
    values.FormatPush("%u",chardata->BankMoney().GetTrias());
    values.FormatPush("%u",chardata->BankMoney().GetHexas());
    values.FormatPush("%u",chardata->BankMoney().GetOctas());
    float x,y,z,yrot;
    psSectorInfo *sectorinfo;
    int instance;
    chardata->GetLocationInWorld(instance,sectorinfo,x,y,z,yrot);
    values.FormatPush("%10.2f",x);
    values.FormatPush("%10.2f",y);
    values.FormatPush("%10.2f",z);
    values.FormatPush("%10.2f",yrot);
    values.FormatPush("%u",sectorinfo->uid);
    values.FormatPush("%d",instance);
    csString csv;
    if (chardata->GetActor())
        chardata->GetActor()->GetFactions()->GetFactionListCSV(csv);
    values.FormatPush("%s",csv.GetData());     

    values.FormatPush("%u",chardata->npc_spawnruleid);
    values.FormatPush("%u",chardata->npc_masterid);
    values.FormatPush("%u",chardata->loot_category_id);
    values.FormatPush("%c",chardata->GetImperviousToAttack() & ALWAYS_IMPERVIOUS ? 'Y' : 'N');
    values.FormatPush("%u",chardata->GetKillExperience() );
    values.FormatPush("%s",chardata->GetAnimalAffinity() );

    
    unsigned int id=db->GenericInsertWithID("characters",fieldnames,values);

    if (id==0)
    {
        Error3("Failed to create new character for account id %u. Error %s",accountid,db->GetLastError());
        return false;
    }

    chardata->SetCharacterID(id);

    return true;
}

bool psCharacterLoader::NewCharacterData(unsigned int accountid,psCharacter *chardata)
{
    chardata->npc_spawnruleid = 0;
    chardata->npc_masterid    = 0;

    if (!NewNPCCharacterData(accountid,chardata))
        return false;

    int i;

    // traits
    if (!ClearCharacterTraits(chardata->GetCharacterID()))
        Error3("Failed to clear traits for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    for (i=0;i<PSTRAIT_LOCATION_COUNT;i++)
    {
        psTrait *trait=chardata->GetTraitForLocation((PSTRAIT_LOCATION)i);
        if (trait!=NULL)
            SaveCharacterTrait(chardata->GetCharacterID(),trait->uid);
    }


    // skills
    if (!ClearCharacterSkills(chardata->GetCharacterID()))
        Error3("Failed to clear skills for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    for (i=0;i<PSSKILL_COUNT;i++)
    {
        unsigned int skillRank=chardata->GetSkills()->GetSkillRank((PSSKILL)i, false);
        unsigned int skillY=chardata->GetSkills()->GetSkillKnowledge((PSSKILL)i);
        unsigned int skillZ=chardata->GetSkills()->GetSkillPractice((PSSKILL)i);
        SaveCharacterSkill(chardata->GetCharacterID(),i,skillZ,skillY,skillRank);
    }


    // advantages
    if (!ClearCharacterAdvantages(chardata->GetCharacterID()))
        Error3("Failed to clear advantages for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());

    for (i=0;i<PSCHARACTER_ADVANTAGE_COUNT;i++)
    {
        if (chardata->HasAdvantage((PSCHARACTER_ADVANTAGE)i))
            SaveCharacterAdvantage(chardata->GetCharacterID(),i);
    }

    return true;
}

bool psCharacterLoader::UpdateQuestAssignments(psCharacter *chr)
{
    return chr->UpdateQuestAssignments();
}

bool psCharacterLoader::ClearCharacterSpell( psCharacter * character )
{
    unsigned int character_id = character->GetCharacterID();
    unsigned long result=db->Command("DELETE FROM player_spells WHERE player_id='%u'",character_id);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::SaveCharacterSpell( psCharacter * character )
{
    int index = 0;
    while (psSpell * spell = character->GetSpellByIdx(index))
    {
        unsigned int character_id = character->GetCharacterID();
        unsigned long result=db->Command("INSERT INTO player_spells (player_id,spell_id,spell_slot) VALUES('%u','%u','%u')",
            character_id,spell->GetID(),index);
        if (result==QUERY_FAILED)
            return false;
        index++;
    }
    return true;
}



bool psCharacterLoader::ClearCharacterTraits(unsigned int character_id)
{
    unsigned long result=db->Command("DELETE FROM character_traits WHERE character_id='%u'",character_id);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::SaveCharacterTrait(unsigned int character_id,unsigned int trait_id)
{
    unsigned long result=db->Command("INSERT INTO character_traits (character_id,trait_id) VALUES('%u','%u')",character_id,trait_id);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::ClearCharacterSkills(unsigned int character_id)
{
    unsigned long result=db->Command("DELETE FROM character_skills WHERE character_id='%u'",character_id);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::UpdateCharacterSkill(unsigned int character_id,unsigned int skill_id,
                                           unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank)
{
    csString sql;
    sql.Format("DELETE FROM character_skills WHERE character_id='%u' AND skill_id='%u'",
        character_id,
        (unsigned int)skill_id);

    unsigned long result=db->Command(sql);
    if (result==QUERY_FAILED)
        return false;
    
    return SaveCharacterSkill(
        character_id,
        skill_id,
        skill_z,
        skill_y,
        skill_rank
        );
}

bool psCharacterLoader::SaveCharacterSkill(unsigned int character_id,unsigned int skill_id,
                                           unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank)
{
    // If a player has no knowledge of the skill then no need to add to database yet. 
    if ( skill_z == 0 && skill_y == 0 && skill_rank == 0 )
        return true;
        
    unsigned long result=db->Command("INSERT INTO character_skills (character_id,skill_id,skill_y,skill_z,skill_rank) VALUES('%u','%u','%u','%u','%u')",
        character_id,skill_id,skill_y,skill_z,skill_rank);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::ClearCharacterAdvantages(unsigned int character_id)
{
    unsigned long result=db->Command("DELETE FROM character_advantages WHERE character_id='%u'",character_id);
    if (result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::SaveCharacterAdvantage(unsigned int character_id,unsigned int advantage_id)
{
    unsigned long result=db->Command("INSERT INTO character_advantages (character_id,advantages_id) VALUES('%u','%u')",
        character_id,advantage_id);
    if (result==QUERY_FAILED)
        return false;

    return true;

}


bool psCharacterLoader::DeleteCharacterData( unsigned int charID, csString& error )
{
    csString query;
    query.Format( "SELECT name, lastname, guild_member_of, guild_level FROM characters where id='%u'\n", charID );
    Result result( db->Select(query) );
    if ( !result.IsValid() || !result.Count() )
    {
        error = "Invalid DB entry!";
        return false;
    }

    iResultRow& row = result[0];

    const char* charName = row["name"];
    const char* charLastName = row["lastname"];
    csString charFullName;
    charFullName.Format("%s %s", charName, charLastName);

    unsigned int guild = row.GetUInt32("guild_member_of");
    int guildLevel = row.GetInt("guild_level");
    
    // If Guild leader? then can't delete
    if ( guildLevel == 9 )
    {
        error = "Character is a guild leader";
        return false;
    }

    // Online? Kick
    Client* zombieClient = psserver->GetConnections()->Find( charName );
    if ( zombieClient )
        psserver->RemovePlayer(zombieClient->GetClientNum(),"This character is being deleted");
    
    // Remove the character from guild if he has joined any
    psGuildInfo* guildinfo = CacheManager::GetSingleton().FindGuild( guild );
    if ( guildinfo )
        guildinfo->RemoveMember( guildinfo->FindMember( charID ) );

    // Now delete this character and all refrences to him from DB

    // Note: Need to delete the pets using this function as well.

    query.Format( "delete from character_relationships where character_id=%d or related_id=%d", charID, charID );
    db->Command( query ); 

    query.Format("delete from character_quests where player_id=%d", charID );
    db->Command( query );
        
    query.Format("delete from character_skills where character_id=%d", charID );
    db->Command( query );
    
    query.Format("delete from character_traits where character_id=%d", charID );
    db->Command( query );
            
    query.Format("delete from player_spells where player_id=%d", charID);
    db->Command( query );

    query.Format("delete from characters where id=%d", charID );
    db->Command( query );

    /// Let GMEventManager sort the DB out, as it is a bit complex, and its cached too
    if (!psserver->GetGMEventManager()->RemovePlayerFromGMEvents(charID))
        Error2("Failed to remove %d character from GM events database/cache", charID);

    csArray<gemObject*> list;
    GEMSupervisor::GetSingleton().GetPlayerObjects( charID, list );
    for ( size_t x = 0; x < list.GetSize(); x++ )
    {
        GEMSupervisor::GetSingleton().RemoveEntity(list[x],list[x]->GetGemID());
    }                            
    
    query.Format("delete from item_instances where char_id_owner=%d", charID );
    db->Command( query );

    CPrintf( CON_DEBUG, "\nSuccessfully delete character id: %d\n", charID );

    return true;
    
}

// 08-02-2005 Borrillis:
// NPC's should not save their locations back into the characters table
// This causes mucho problems so don't "fix" it.
bool psCharacterLoader::SaveCharacterData(psCharacter *chardata,gemActor *actor,bool charRecordOnly)
{
    bool playerORpet = chardata->GetCharType() == PSCHARACTER_TYPE_PLAYER ||
                       chardata->GetCharType() == PSCHARACTER_TYPE_PET;

    int i;
    const char *player_fieldnames[]= {
        "name",
        "lastname",
        "old_lastname",
        "racegender_id",
        "character_type",
        "base_strength",
        "base_agility",
        "base_endurance",
        "base_intelligence",
        "base_will",
        "base_charisma",
        "mod_hitpoints",
//        "base_hitpoints_max",
        "mod_mana",
//        "base_mana_max",
        "stamina_physical",
        "stamina_mental",
        "money_circles",
        "money_trias",
        "money_hexas",
        "money_octas",        
        "bank_money_circles",
        "bank_money_trias",
        "bank_money_hexas",
        "bank_money_octas",
        "loc_x",
        "loc_y",
        "loc_z",
        "loc_yrot",
        "loc_sector_id",
        "last_login",
        "faction_standings",
        "progression_script",
        "time_connected_sec",
        "experience_points",
        "animal_affinity",
        "help_event_flags",
        "description"
    };

    const char *npc_fieldnames[]= {
        "name",
        "lastname",
        "old_lastname",
        "racegender_id",
        "character_type",
        "base_strength",
        "base_agility",
        "base_endurance",
        "base_intelligence",
        "base_will",
        "base_charisma",
        "mod_hitpoints",
//        "base_hitpoints_max",
        "mod_mana",
//        "base_mana_max",
        "stamina_physical",
        "stamina_mental",
        "money_circles",
        "money_trias",
        "money_hexas",
        "money_octas",
        "bank_money_circles",
        "bank_money_trias",
        "bank_money_hexas",
        "bank_money_octas",
        "last_login",
        "faction_standings",
        "progression_script",
        "time_connected_sec",
        "experience_points",
        "animal_affinity",
        "help_event_flags",
        "description"
    };
    psStringArray fields;

    // Give 100% hp if the char is dead
    if(!actor->IsAlive())
        chardata->SetHitPoints( chardata->GetHitPointsMax() );

    fields.FormatPush("%s",chardata->GetCharName());
    fields.FormatPush("%s",chardata->GetCharLastName());
    fields.FormatPush("%s",chardata->GetOldLastName()); // old_lastname
    fields.FormatPush("%u",chardata->GetRaceInfo()->uid);
    fields.FormatPush("%u",chardata->GetCharType() );
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_STRENGTH, false));
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_AGILITY, false));
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_ENDURANCE, false));
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_INTELLIGENCE, false));
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_WILL, false));
    fields.FormatPush("%d",chardata->GetAttributes()->GetStat(PSITEMSTATS_STAT_CHARISMA, false));
    fields.FormatPush("%1.2f", (playerORpet)?chardata->GetHP():chardata->GetHitPointsMax() );
//    fields.FormatPush("%1.2f",chardata->GetHitPointsMax());
    fields.FormatPush("%1.2f", (playerORpet)?chardata->GetMana():chardata->GetManaMax() );
//    fields.FormatPush("%1.2f",chardata->GetManaMax());
    fields.FormatPush("%1.2f",chardata->GetStamina(true) );
    fields.FormatPush("%1.2f",chardata->GetStamina(false));
    fields.FormatPush("%u",chardata->Money().GetCircles());
    fields.FormatPush("%u",chardata->Money().GetTrias());
    fields.FormatPush("%u",chardata->Money().GetHexas());
    fields.FormatPush("%u",chardata->Money().GetOctas());
    fields.FormatPush("%u",chardata->BankMoney().GetCircles());
    fields.FormatPush("%u",chardata->BankMoney().GetTrias());
    fields.FormatPush("%u",chardata->BankMoney().GetHexas());
    fields.FormatPush("%u",chardata->BankMoney().GetOctas());

    float yrot;
    csVector3 pos(0,0,0);
    psSectorInfo *sectorinfo;
    csString sector;
    
    if ( playerORpet ) // Only Pets and Players save location info
    {
        if ( actor->IsAlive() )       
        {
            float vel_y;
            iSector* sec;
            // We want to save the last reported location
            actor->GetLastLocation(pos, vel_y, yrot, sec);
            sector = sec->QueryObject()->GetName();

            sectorinfo = CacheManager::GetSingleton().GetSectorInfoByName(sector);
        }
        else
        {
            // Todo: Get these from database.
            iSector* sec;
            actor->GetPosition(pos,yrot,sec);
            if (sec && !strcmp ("NPCroom", sec->QueryObject()->GetName()) )
            {
                pos.x = -20.0f;
                pos.y = 1.0f;
                pos.z =  -180.0f; 
                yrot = 0.0f;
                sector = "NPCroom";
            }
            else
            {
                pos.x = -23.5f;
                pos.y = -116.0f;
                pos.z =  23.7f; 
                yrot = 0.0f;
                sector = "DR01";
            }

            // Update actor's position for cached objects
            sec = EntityManager::GetSingleton().GetEngine()->FindSector(sector);
            if (sec)
            {
                actor->SetPosition(pos, yrot, sec);
            }

            sectorinfo =  CacheManager::GetSingleton().GetSectorInfoByName(sector);
        }      
            
        if(!sectorinfo)
        {
            Error3("ERROR: Sector %s could not be found in the database! Character %s could not be saved!",sector.GetData(),chardata->GetCharName());
            return false;
        }

          
        fields.FormatPush("%1.2f",pos.x);
        fields.FormatPush("%1.2f",pos.y);
        fields.FormatPush("%1.2f",pos.z);
        fields.FormatPush("%1.2f",yrot);
        fields.FormatPush("%u",sectorinfo->uid);
    }

    if(!chardata->GetLastLoginTime().GetData())
    {
        time_t curr=time(0);
        tm* gmtm = gmtime(&curr);
        csString timeStr;

        timeStr.Format("%d-%02d-%02d %02d:%02d:%02d",
                        gmtm->tm_year+1900,
                        gmtm->tm_mon+1,
                        gmtm->tm_mday,
                        gmtm->tm_hour,
                        gmtm->tm_min,
                        gmtm->tm_sec);
        fields.FormatPush("%s", timeStr.GetData() );
    }
    else
        fields.FormatPush("%s", chardata->GetLastLoginTime().GetData() );

    csString csv;
    actor->GetFactions()->GetFactionListCSV(csv);   
    fields.FormatPush("%s",csv.GetData());

    csString progressionEvents;
    while (chardata->progressionEvents.GetSize() > 0)
    {
        SavedProgressionEvent evt = chardata->progressionEvents.Pop();
        evt.ticksElapsed += csGetTicks() - evt.registrationTime;
        progressionEvents.AppendFmt("<evt elapsed=\"%u\">%s</evt>", evt.ticksElapsed, evt.script.GetData());
    }
    fields.FormatPush("<evts>%s</evts>", progressionEvents.GetData());
  
    fields.FormatPush("%u", chardata->GetTotalOnlineTime());
    fields.FormatPush("%d", chardata->GetExperiencePoints()); // Save W
    // X is saved when changed
    fields.FormatPush("%s", chardata->animal_affinity.GetData() );
    //fields.FormatPush("%u", chardata->owner_id );
    fields.FormatPush("%u", chardata->help_event_flags );
    fields.FormatPush("%s",chardata->GetDescription());

    // Done building the fields struct, now
    // SAVE it to the DB.
    char characteridstring[25];
    sprintf(characteridstring,"%u",chardata->GetCharacterID());

    if ( playerORpet )  // Player or Pet
    {
        if (!UpdateCharacterData(characteridstring,player_fieldnames,fields))
        {
            Error3("Failed to save character %s. Error %s",characteridstring,db->GetLastError());
            return false;
        }    
    }
    else // an NPC
    {
        if (!UpdateCharacterData(characteridstring,npc_fieldnames,fields))
        {
            Error3("Failed to save character %s. Error %s",characteridstring,db->GetLastError());
            return false;
        }    
    }

    if (charRecordOnly)
        return true;   // some updates don't need to save off every table.

    // traits
    if (!ClearCharacterTraits(chardata->GetCharacterID()))
        Error3("Failed to clear traits for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    for (i=0;i<PSTRAIT_LOCATION_COUNT;i++)
    {
        psTrait *trait=chardata->GetTraitForLocation((PSTRAIT_LOCATION)i);
        if (trait!=NULL)
            SaveCharacterTrait(chardata->GetCharacterID(),trait->uid);
    }


    // skills
    if (!ClearCharacterSkills(chardata->GetCharacterID()))
        Error3("Failed to clear skills for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    for (i=0;i<PSSKILL_COUNT;i++)
    {
        unsigned int skillY=chardata->GetSkills()->GetSkillKnowledge((PSSKILL)i);
        unsigned int skillZ=chardata->GetSkills()->GetSkillPractice((PSSKILL)i);
        unsigned int skillRank=chardata->GetSkills()->GetSkillRank((PSSKILL)i, false);
        SaveCharacterSkill(chardata->GetCharacterID(),i,skillZ,skillY,skillRank);
    }


    // advantages
    if (!ClearCharacterAdvantages(chardata->GetCharacterID()))
        Error3("Failed to clear advantages for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    for (i=0;i<PSCHARACTER_ADVANTAGE_COUNT;i++)
    {
        if (chardata->HasAdvantage((PSCHARACTER_ADVANTAGE)i))
            SaveCharacterAdvantage(chardata->GetCharacterID(),i);
    }

    if (!ClearCharacterSpell(chardata))
        Error3("Failed to clear spells for character id %u.  Error %s.",chardata->GetCharacterID(),db->GetLastError());
    SaveCharacterSpell( chardata ); 

    UpdateQuestAssignments( chardata );
       
    return true;
}

unsigned int psCharacterLoader::InsertNewCharacterData(const char **fieldnames, psStringArray& fieldvalues)
{
    return db->GenericInsertWithID("characters",fieldnames,fieldvalues);
}

bool psCharacterLoader::UpdateCharacterData(const char *id,const char **fieldnames, psStringArray& fieldvalues)
{
    return db->GenericUpdateWithID("characters","id",id,fieldnames,fieldvalues);
}



unsigned int psCharacterLoader::FindCharacterID(const char *character_name, bool excludeNPCs )
{
    csString escape;

    // Don't crash
    if (character_name==NULL)
        return 0;
    // Insufficient Escape Buffer space, and this is too long anyway
    if (strlen(character_name)>32)
        return 0;
    db->Escape(escape,character_name);

    unsigned long result;

    if ( !excludeNPCs )
        result = db->SelectSingleNumber("SELECT id from characters where name='%s'",escape.GetData());
    else
        result = db->SelectSingleNumber("SELECT id from characters where name='%s' AND npc_master_id=0",escape.GetData());        

    if(result == QUERY_FAILED)
        return 0;
    else
        return result;
}

unsigned int psCharacterLoader::FindCharacterID(unsigned int accountID, const char *character_name )
{
    csString escape;
 
    // Don't crash
    if (character_name==NULL)
        return 0;
    // Insufficient Escape Buffer space, and this is too long anyway
    if (strlen(character_name)>32)
        return 0;
    db->Escape(escape,character_name);
 
    unsigned long result;
 
    result = db->SelectSingleNumber("SELECT id FROM characters WHERE name='%s' AND account_id=%d", escape.GetData(), accountID);
 
    if(result == QUERY_FAILED)
        return 0;
    else
        return result;
}


//-----------------------------------------------------------------------------



psSaveCharEvent::psSaveCharEvent(gemActor* object)
    : psGameEvent(0,psSaveCharEvent::SAVE_PERIOD,"psCharSaveEvent")
{
    this->actor = NULL;
    
    object->RegisterCallback( this );    
    this->actor = object;
}

psSaveCharEvent::~psSaveCharEvent()
{
    if ( this->actor )
    {
        this->actor->UnregisterCallback(this);    
    }
}

void psSaveCharEvent::DeleteObjectCallback(iDeleteNotificationObject * object)
{
    if ( this->actor )        
        this->actor->UnregisterCallback(this);

    this->actor = NULL;
    SetValid(false);
}


void psSaveCharEvent::Trigger()
{
    psServer::CharacterLoader.SaveCharacterData( actor->GetCharacterData(), actor );
    psSaveCharEvent *saver = new psSaveCharEvent(actor);
    saver->QueueEvent();       
}
