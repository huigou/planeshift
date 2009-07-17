/*
 * psitemstats.cpp
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
#include <csutil/randomgen.h>
#include <imesh/object.h>
#include <iengine/mesh.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psstring.h"
#include "util/serverconsole.h"

#include "../psserver.h"
#include "../cachemanager.h"
#include "../scripting.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psitemstats.h"
#include "psglyph.h"
#include "psitem.h"
#include "psmerchantinfo.h"

PSITEMSTATS_STAT skillToStat(PSSKILL skill)
{
    switch (skill)
    {
        case PSSKILL_AGI:  return PSITEMSTATS_STAT_AGILITY;
        case PSSKILL_END:  return PSITEMSTATS_STAT_ENDURANCE;
        case PSSKILL_STR:  return PSITEMSTATS_STAT_STRENGTH;
        case PSSKILL_CHA:  return PSITEMSTATS_STAT_CHARISMA;
        case PSSKILL_INT:  return PSITEMSTATS_STAT_INTELLIGENCE;
        case PSSKILL_WILL: return PSITEMSTATS_STAT_WILL;
        default:           return PSITEMSTATS_STAT_NONE;
    }
}

//-----------------------------------------------------------------------------

psItemArmorStats::psItemArmorStats()
{
    armor_type = PSITEMSTATS_ARMORTYPE_NONE;
    armor_class = 0;
    memset(damage_protection,0,sizeof(damage_protection));
    hardness = 0.0;
}

float psItemArmorStats::Protection(PSITEMSTATS_DAMAGETYPE dmgtype)
{
    if (dmgtype <0 || dmgtype >= PSITEMSTATS_DAMAGETYPE_COUNT)
        return 0.0f;
    return damage_protection[dmgtype];
}

void psItemArmorStats::ReadStats( iResultRow& row )
{
    hardness = row.GetFloat("armor_hardness");
    damage_protection[PSITEMSTATS_DAMAGETYPE_SLASH]     = row.GetFloat("dmg_slash");
    damage_protection[PSITEMSTATS_DAMAGETYPE_BLUNT]     = row.GetFloat("dmg_blunt");
    damage_protection[PSITEMSTATS_DAMAGETYPE_PIERCE]    = row.GetFloat("dmg_pierce");

    csString temp(row["armorvsweapon_type"]);
    if ( temp.Length() == 2 )
    {
        if (temp.GetAt(0) == '1')
            armor_type = PSITEMSTATS_ARMORTYPE_LIGHT;
        else if (temp.GetAt(0) == '2')
            armor_type = PSITEMSTATS_ARMORTYPE_MEDIUM;
        else if (temp.GetAt(0) == '3')
            armor_type = PSITEMSTATS_ARMORTYPE_HEAVY;

        armor_class = temp.GetAt(1);
    }

}

//-----------------------------------------------------------------------------

psItemWeaponStats::psItemWeaponStats()
{
    weapon_type=PSITEMSTATS_WEAPONTYPE_NONE;

    memset(weapon_skill,0,sizeof(weapon_skill));
    memset(attribute_bonuses,0,sizeof(attribute_bonuses));
    memset(damages,0,sizeof(damages));

    latency=0.0f;
    penetration=0.0f;

    untargeted_block_value=0.0f;
    targeted_block_value=0.0f;
    counter_block_value=0.0f;
}

void psItemWeaponStats::ReadStats( iResultRow& row )
{
    csString type( row["item_type"] );
    if ( type == "SWORD" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_SWORD;
    else if ( type == "AXE" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_AXE;
    else if ( type == "DAGGER" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_DAGGER;
    else if ( type == "HAMMER" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_HAMMER;
    else if ( type == "BOW" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_BOW;
    else if ( type == "CROSSBOW" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_CROSSBOW;
    else if ( type == "SLING" )
        weapon_type = PSITEMSTATS_WEAPONTYPE_SLING;

    weapon_skill[PSITEMSTATS_WEAPONSKILL_INDEX_0] = CacheManager::GetSingleton().ConvertSkill(row.GetInt("item_skill_id_1"));
    weapon_skill[PSITEMSTATS_WEAPONSKILL_INDEX_1] = CacheManager::GetSingleton().ConvertSkill(row.GetInt("item_skill_id_2"));
    weapon_skill[PSITEMSTATS_WEAPONSKILL_INDEX_2] = CacheManager::GetSingleton().ConvertSkill(row.GetInt("item_skill_id_3"));

    latency = row.GetFloat("weapon_speed");
    if (latency < 1.5F)
    {
        // Clamp weapon speed to maximum of 1 hit per 1.5 seconds to keep a fight from monopolizing the server
        latency = 1.5F;
    }

    damages[PSITEMSTATS_DAMAGETYPE_SLASH] = row.GetFloat("dmg_slash");
    damages[PSITEMSTATS_DAMAGETYPE_BLUNT] = row.GetFloat("dmg_blunt");
    damages[PSITEMSTATS_DAMAGETYPE_PIERCE] = row.GetFloat("dmg_pierce");

    penetration = row.GetFloat("weapon_penetration");
    targeted_block_value = row.GetFloat("weapon_block_targeted");
    untargeted_block_value = row.GetFloat("weapon_block_untargeted");
    counter_block_value = row.GetFloat("weapon_counterblock");
}

PSSKILL psItemWeaponStats::Skill(PSITEMSTATS_WEAPONSKILL_INDEX index)
{
    if (index < 0 || index >= PSITEMSTATS_WEAPONSKILL_INDEX_COUNT)
        return PSSKILL_NONE;
    return weapon_skill[index];
}

PSITEMSTATS_STAT psItemWeaponStats::AttributeBonusType(int index)
{
    if (index < 0 || index >= PSITEMSTATS_STAT_BONUS_COUNT)
        return PSITEMSTATS_STAT_NONE;
    return attribute_bonuses[index].attribute_id;
}

float psItemWeaponStats::AttributeBonusMax(int index)
{
    if (index < 0 || index >= PSITEMSTATS_STAT_BONUS_COUNT)
        return 0.0f;
    return attribute_bonuses[index].bonus_max;
}

float psItemWeaponStats::Damage(PSITEMSTATS_DAMAGETYPE dmgtype)
{
    if (dmgtype<0 || dmgtype >= PSITEMSTATS_DAMAGETYPE_COUNT)
        return 0.0f;
    return damages[dmgtype];
}

//-----------------------------------------------------------------------------
psItemAmmoStats::psItemAmmoStats()
{
    ammunition_type = PSITEMSTATS_AMMOTYPE_NONE;
}

psItemAmmoStats::~psItemAmmoStats()
{
}

void psItemAmmoStats::ReadStats( iResultRow& row )
{
    csString type( row["item_type"] );
    if ( type == "ARROW" )
        ammunition_type = PSITEMSTATS_AMMOTYPE_ARROWS;
    else if ( type == "BOLT" )
        ammunition_type = PSITEMSTATS_AMMOTYPE_BOLTS;
    else if ( type == "ROCK" )
        ammunition_type = PSITEMSTATS_AMMOTYPE_ROCKS;
}

//-----------------------------------------------------------------------------

psItemCreativeStats::psItemCreativeStats()
{
    creativeType = PSITEMSTATS_CREATIVETYPE_NONE;

    creatorIDStatus = PSITEMSTATS_CREATOR_UNASSIGNED;     // creator not yet been assigned
    creatorID = 0;

    content.Empty();
}

psItemCreativeStats::~psItemCreativeStats()
{
}

void psItemCreativeStats::ReadStats(iResultRow& row)
{
    psXMLString contentXML;
    psXMLTag creativeTag;
    csString creativeTypeStr, creatorStr;

    // find type of creative thing from <creative type="...">
    creativeDefinitionXML = row["creative_definition"];
    int tagPos = creativeDefinitionXML.FindTag("creative");
    size_t len = creativeDefinitionXML.GetTag(0, creativeTag);
    if (tagPos >=0 && len > 0)
    {
        creativeTag.GetTagParm("type", creativeTypeStr);
        // atm, only literature (books, scrolls, etc) and sketches (maps, etc) supported
        if (creativeTypeStr.Downcase() == "literature")
            creativeType = PSITEMSTATS_CREATIVETYPE_LITERATURE;
        else if (creativeTypeStr.Downcase() == "sketch")
            creativeType = PSITEMSTATS_CREATIVETYPE_SKETCH;

        // get the creator (author, artist, etc)
        creativeTag.GetTagParm("creator", creatorStr);
        if (creatorStr.Length() == 0)
        {
            creatorIDStatus = PSITEMSTATS_CREATOR_UNASSIGNED;     // creator not yet been assigned
            creatorID = 0;
        }
        else
        {
            if (creatorStr == "public")
                creatorIDStatus = PSITEMSTATS_CREATOR_PUBLIC;
            else
            {
                creatorID = atoi(creatorStr.GetData());
                if (creatorID == 0)
                    creatorIDStatus = PSITEMSTATS_CREATOR_UNKNOWN; // creator no longer around
                else
                    creatorIDStatus = PSITEMSTATS_CREATOR_VALID;   // valid
            }
        }

        if (creativeType != PSITEMSTATS_CREATIVETYPE_NONE)
        {
            len = creativeDefinitionXML.GetWithinTagSection(0, "creative", contentXML);
            if (len <= 0)
                creativeType = PSITEMSTATS_CREATIVETYPE_NONE;
            else
            {
                // now extract the content: <content>[stuff...]</content>.
                tagPos = contentXML.FindTag("content", 0);
                if (tagPos >= 0)
                {
                    // everythings good: go back happy
                    len = contentXML.GetWithinTagSection(0, "content", content);
                    return;
                }
            }
        }
    }

    content.Empty();
    Error2("XML error in CREATIVE item %lu", row.GetUInt32("id"));
}

bool psItemCreativeStats::SetCreativeContent(PSITEMSTATS_CREATIVETYPE updatedCreativeType, const csString& updatedCreative, uint32 uid)
{
    if (creativeType == updatedCreativeType)
    {
        content = updatedCreative;
        if (FormatCreativeContent())
        {
            SaveCreation(uid);
            return true;
        }
    }

    return false;
}

bool psItemCreativeStats::FormatCreativeContent(void)
{
    csString contentForXML(content);

    creativeDefinitionXML = "<creative type=\"";

    // start of creative data xml
    if (creativeType == PSITEMSTATS_CREATIVETYPE_LITERATURE)
    {
        contentForXML.ReplaceAll("%", "%%");
        creativeDefinitionXML.Append("literature");
    }
    else if (creativeType == PSITEMSTATS_CREATIVETYPE_SKETCH)
        creativeDefinitionXML.Append("sketch");
    creativeDefinitionXML.AppendFmt("\"");

    // add author, artist... if necessary
    if (creatorIDStatus == PSITEMSTATS_CREATOR_PUBLIC)
    {
        creativeDefinitionXML.AppendFmt(" creator=\"public\"");
    }
    else if (creatorIDStatus != PSITEMSTATS_CREATOR_UNASSIGNED)
    {
        creativeDefinitionXML.AppendFmt(" creator=\"%u\"", creatorID.Unbox());
    }

    creativeDefinitionXML.AppendFmt("><content>%s</content></creative>", contentForXML.GetDataSafe());

    if (creativeDefinitionXML.Length() <= CREATIVEDEF_MAX)
        return true;

    return false;
}

void psItemCreativeStats::SaveCreation(uint32 uid)
{
    // save to database
    psStringArray values;
    const char *fieldnames[] = {"creative_definition"};
    csString litID;

    litID.Format("%u", uid);
    values.Push(this->creativeDefinitionXML);

    if ( !db->GenericUpdateWithID("item_stats", "id", litID, fieldnames, values) )
    {
        Error2("Failed to save text for item %u!", uid );
    }

}

/// Update the description of the book, etc.
csString psItemCreativeStats::UpdateDescription(PSITEMSTATS_CREATIVETYPE creativeType,
                                                csString title, csString author)
{
    csString newDescription = "No description yet";

    if (creativeType == PSITEMSTATS_CREATIVETYPE_LITERATURE)
    {
        newDescription = "A book, titled \"" + title + "\"";

        if (creatorIDStatus == PSITEMSTATS_CREATOR_VALID)
            newDescription.AppendFmt(" written by %s.", author.GetDataSafe());
        else if (creatorIDStatus == PSITEMSTATS_CREATOR_PUBLIC)
            newDescription.Append(". Anyone can write in this book.");
        else
            newDescription.Append(". Unknown author.");

        newDescription.AppendFmt(" The book is %d characters long.", (int)content.Length());
    }
    else if (creativeType == PSITEMSTATS_CREATIVETYPE_SKETCH)
    {
        newDescription = "A map, titled \"" + title + "\"";

        if (creatorIDStatus == PSITEMSTATS_CREATOR_VALID)
            newDescription.AppendFmt(" drawn by %s.", author.GetDataSafe());
        else if (creatorIDStatus == PSITEMSTATS_CREATOR_PUBLIC)
            newDescription.Append(". Anyone can draw this map.");
        else
            newDescription.Append(". Unknown artist.");
    }

    return newDescription;
}

//-----------------------------------------------------------------------------

// Definition of the itempool for psItemStats
PoolAllocator<psItemStats> psItemStats::itempool;

void *psItemStats::operator new(size_t allocSize)
{
    CS_ASSERT(allocSize<=sizeof(psItemStats));
    return (void *)itempool.CallFromNew();
}

void psItemStats::operator delete(void *releasePtr)
{
    itempool.CallFromDelete((psItemStats *)releasePtr);
}


psItemStats::psItemStats()
{
    uid=0;
    name.Clear();
    description="";
    weight=0.0f;
    size=0;
    container_max_size=0;

    valid_slots=0x00000000;
    decay_rate=0;
    flags=0x00000000;

    mesh_name=NULL;
    texture_name.Clear();
    texturepart_name.Clear();
    image_name=NULL;

    equipScript = NULL;

    visible_distance = DEF_PROX_DIST;
}

psItemStats::~psItemStats()
{
    if (equipScript)
    {
        delete equipScript;
        equipScript = NULL;
    }
}

bool psItemStats::ReadItemStats(iResultRow& row)
{
    uid = row.GetUInt32("id");
    name = row["name"];
    stat_type = row["stat_type"];
    SetDescription(row["description"]);

    consumeScriptName = row["consume_script"];
    equipScript = NULL;

    csString equipXML(row["equip_script"]);
    if (!equipXML.IsEmpty())
    {
        equipScript = ApplicativeScript::Create(equipXML);
        if (!equipScript)
        {
            Error4("Could not create ApplicativeScript for ItemStats %d (%s)'s equip script: %s.", uid, name.GetData(), equipXML.GetData());
            return false;
        }
    }

    item_quality       = row.GetFloat("item_max_quality");
    if (item_quality > 300)
    {
        // Clamp item quality to 300 or less so cheats and bugs don't result in ridiculous items
        item_quality = 300;
    }
    weight             = row.GetFloat("weight");
    size               = (unsigned short)row.GetInt("size");
    container_max_size = (unsigned short)row.GetInt("container_max_size");
    visible_distance   = row.GetFloat("visible_distance");
    decay_rate         = row.GetFloat("decay_rate");

    // Load in the valid slots for the item.
    LoadSlots( row );

    ParseFlags( row );

    if (flags==0) // no type specified is illegal
    {
        Error2("Item %s has no valid flags specified.",row["id"] );
        return false;
    }

    SetArmorVsWeaponType(row["armorvsweapon_type"]);

    weaponStats.ReadStats( row );
    armorStats.ReadStats( row );
    ammoStats.ReadStats( row );
    if (flags & PSITEMSTATS_FLAG_CREATIVE)
    {
        creativeStats.ReadStats( row );
    }

    // Mesh, Texture, Part and Image strings
    const char *meshname = CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_gfx_mesh"));
    if (row.GetUInt32("cstr_id_gfx_mesh")!=0 && !meshname)
    {
        Error3("Error, no commonstring exists with number %lu (cstr_id_gfx_mesh) while loading item_stats.id %u.\n",row.GetUInt32("cstr_id_gfx_mesh"),uid);
        return false;
    }
    else
    {
        SetMeshName(meshname);
    }

    const char *texturename = CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_gfx_texture"));
    if (row.GetUInt32("cstr_id_gfx_texture")!=0 && !texturename)
    {
        Error3("Error, no commonstring exists with number %lu (cstr_id_gfx_texture) while loading item_stats.id %u.\n",row.GetUInt32("cstr_id_gfx_texture"),uid);
        return false;
    }
    else
    {
        SetTextureName(texturename);
    }
    
    const char *partname = CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_part"));
    if (row.GetUInt32("cstr_id_part")!=0 && !texturename)
    {
        Error3("Error, no commonstring exists with number %lu (cstr_id_part) while loading item_stats.id %u.\n",row.GetUInt32("cstr_id_part"),uid);
        return false;
    }
    else
    {
        SetPartName(partname);
    }
    
    const char *imagename = CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_gfx_icon"));
    if (row.GetUInt32("cstr_id_gfx_icon")!=0 && !texturename)
    {
        Error3("Error, no commonstring exists with number %lu (cstr_id_gfx_icon) while loading item_stats.id %u.\n",row.GetUInt32("cstr_id_gfx_icon"),uid);
        return false;
    }
    else
    {
        SetImageName(imagename);
    }
    
    const char *partmeshname = CacheManager::GetSingleton().FindCommonString(row.GetUInt32("cstr_id_part_mesh"));
    if (row.GetUInt32("cstr_id_gfx_icon")!=0 && !texturename)
    {
        Error3("Error, no commonstring exists with number %lu (cstr_id_part_mesh) while loading item_stats.id %u.\n",row.GetUInt32("cstr_id_part_mesh"),uid);
        return false;
    }
    else
    {
        SetPartMeshName(imagename);
    }

    SetPrice(row.GetInt("base_sale_price"));
    int categoryId = row.GetInt("category_id");
    psItemCategory * category = CacheManager::GetSingleton().GetItemCategoryByID(categoryId);
    if (!category)
    {
        // Should not load without a category.
        Error3("None exsisting item category '%d' for itemstat '%d'",categoryId,uid);
        return false;
    }
    SetCategory(category);

    reqs[0].name = row["requirement_1_name"];
    reqs[0].min_value = row.GetFloat("requirement_1_value");
    reqs[1].name = row["requirement_2_name"];
    reqs[1].min_value = row.GetFloat("requirement_2_value");
    reqs[2].name = row["requirement_3_name"];
    reqs[2].min_value = row.GetFloat("requirement_3_value");

    psString strTmpAmmoList = row["item_type_id_ammo"];
    csStringArray strTmpAmmoListArray;
    strTmpAmmoList.Split(strTmpAmmoListArray, ',');
    while (!strTmpAmmoListArray.IsEmpty())
    {
        char* currAmmo = strTmpAmmoListArray.Pop();
        unsigned int ammoID = atoi(currAmmo);
        if (ammoID != 0)
        {
            ammo_types.Add(ammoID);
        }
        delete[] currAmmo;
    }


    spell_id_on_hit          = row.GetInt("spell_id_on_hit");
    spell_on_hit_probability = row.GetFloat("spell_on_hit_prob");
    spell_id_feature         = row.GetInt("spell_id_feature");
    spell_feature_charges    = row.GetInt("spell_feature_charges");
    spell_feature_timing     = row.GetInt("spell_feature_timing");

    SetMaxCharges(row.GetInt("max_charges"));

    SetSound(row["sound"]);

    int anim_list_id = row.GetInt("item_anim_id");
    if (anim_list_id)
    {
        anim_list = CacheManager::GetSingleton().FindAnimationList(anim_list_id);
        if (!anim_list)
        {
            Error3("Failed to find Item Animation '%d' for itemstat '%d'",
                   anim_list_id,uid);
            return false;
        }
    }
    else
    {
        anim_list = NULL;
    }

    weaponRange = row.GetFloat("weapon_range");

    return true;
}


void psItemStats::ParseFlags( iResultRow& row )
{
    psString flagstr(row["flags"]);

    int z = 0;
    // Iterates over all the flags in the list and adds that flag to the value if it
    // has been found in the flag string from the database.
    while ( CacheManager::GetSingleton().ItemStatFlagArray[z].string != "END" )
    {
        if (flagstr.FindSubString(CacheManager::GetSingleton().ItemStatFlagArray[z].string,0,true)!=-1)
        {
            flags |= CacheManager::GetSingleton().ItemStatFlagArray[z].flag;
        }
        z++;
    }

    // Enforce extra rules: Container => !Stackable
    if (GetIsContainer() && GetIsStackable())
    {
        flags &= ~PSITEMSTATS_FLAG_IS_STACKABLE;
        Error3("Invalid data: item_stats entry %u (%s) is a container and thus can't be stackable.", uid, name.GetData());
    }
}


void psItemStats::LoadSlots( iResultRow& row )
{
    // Valid slots
    csStringArray slots;
    if(row["valid_slots"]) //check if it's valid (aka the row is not NULL)
        slots.SplitString(row["valid_slots"],  " ");

    for(size_t i = 0; i < slots.GetSize(); i++)
    {
        csString currentSlot = slots.Get(i); //get the current slot

        //checks what slot it is and push in an ordered fashion in order
        //to allow to define what slot has a precedence in /equip
        if (currentSlot == "BULK")
        {
            valid_slots|=PSITEMSTATS_SLOT_BULK;
        }
        else if (currentSlot == "RIGHTHAND")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_RIGHTHAND);
            valid_slots|=PSITEMSTATS_SLOT_RIGHTHAND;
        }
        else if (currentSlot == "LEFTHAND")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_LEFTHAND);
            valid_slots|=PSITEMSTATS_SLOT_LEFTHAND;
        }
        else if (currentSlot == "BOTHHANDS")
        {
            valid_slots|=PSITEMSTATS_SLOT_BOTHHANDS;
            valid_slots_array.Push(PSCHARACTER_SLOT_BOTHHANDS);
        }
        else if (currentSlot == "HELM")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_HELM);
            valid_slots|=PSITEMSTATS_SLOT_HELM;
        }
        else if (currentSlot == "LEFTFINGER")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_LEFTFINGER);
            valid_slots|=PSITEMSTATS_SLOT_LEFTFINGER;
        }
        else if (currentSlot == "RIGHTFINGER")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_RIGHTFINGER);
            valid_slots|=PSITEMSTATS_SLOT_RIGHTFINGER;
        }
        else if (currentSlot == "NECK")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_NECK);
            valid_slots|=PSITEMSTATS_SLOT_NECK;
        }
        else if (currentSlot == "BACK")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_BACK);
            valid_slots|=PSITEMSTATS_SLOT_BACK;
        }
        else if (currentSlot == "ARMS")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_ARMS);
            valid_slots|=PSITEMSTATS_SLOT_ARMS;
        }
        else if (currentSlot == "GLOVES")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_GLOVES);
            valid_slots|=PSITEMSTATS_SLOT_GLOVES;
        }
        else if (currentSlot == "BOOTS")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_BOOTS);
            valid_slots|=PSITEMSTATS_SLOT_BOOTS;
        }
        else if (currentSlot == "LEGS")
        {
            valid_slots|=PSITEMSTATS_SLOT_LEGS;
            valid_slots_array.Push(PSCHARACTER_SLOT_LEGS);
        }
        else if (currentSlot == "BELT")
        {
            valid_slots|=PSITEMSTATS_SLOT_BELT;
            valid_slots_array.Push(PSCHARACTER_SLOT_BELT);
        }
        else if (currentSlot == "BRACERS")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_BRACERS);
            valid_slots|=PSITEMSTATS_SLOT_BRACERS;
        }
        else if (currentSlot == "TORSO")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_TORSO);
            valid_slots|=PSITEMSTATS_SLOT_TORSO;
        }
        else if (currentSlot == "MIND")
        {
            valid_slots_array.Push(PSCHARACTER_SLOT_MIND);
            valid_slots|=PSITEMSTATS_SLOT_MIND;
        }
        else if (currentSlot == "BANK")
        {
            valid_slots|=PSITEMSTATS_SLOT_BANK;
        }
        else if (currentSlot == "CRYSTAL")
        {
            valid_slots|=PSITEMSTATS_SLOT_CRYSTAL;
        }
        else if (currentSlot == "AZURE")
        {
            valid_slots|=PSITEMSTATS_SLOT_AZURE;
        }
        else if (currentSlot == "RED")
        {
            valid_slots|=PSITEMSTATS_SLOT_RED;
        }
        else if (currentSlot == "DARK")
        {
            valid_slots|=PSITEMSTATS_SLOT_DARK;
        }
        else if (currentSlot == "BROWN")
        {
            valid_slots|=PSITEMSTATS_SLOT_BROWN;
        }
        else if (currentSlot == "BLUE")
        {
            valid_slots|=PSITEMSTATS_SLOT_BLUE;
        }
    }
}

// Saves current Item Stat values back to the DB.
bool psItemStats::Save()
{
    static iRecord* update;

    if(update == NULL)
        update = db->NewUpdatePreparedStatement("item_stats", "id", 28, __FILE__, __LINE__); // 27 parameters plus 1 id

    update->Reset();

    bool update_ok=true;

    // Existing Item, update

    // Owning character ID
    // INT        fields.FormatPush("%u",
    // FLOAT    fields.FormatPush("%1.2f",
    // STRING    fields.Push(

    update->AddField("name", name);

    update->AddField("stat_type", stat_type);

    update->AddField("weight", weight);
    update->AddField("visible_distance", visible_distance);
    update->AddField("size", size);
    update->AddField("container_max_size", container_max_size);
    update->AddField("decay_rate", decay_rate);
    update->AddField("category_id", category->id);
    update->AddField("base_sale_price", price.GetTotal());
    // update->AddField("item_type_id_ammo", ammo_item_type_id);
    if (this->anim_list)
        update->AddField("item_anim_id", anim_list->Get(0)->id ); // item_anim_id
    else
        update->AddField("item_anim_id", 0); // No anims

    update->AddField("flags", FlagsToText().GetDataSafe()); // save flags

    update->AddField("description", description );                       // description
    update->AddField("weapon_speed", weaponStats.Latency()); // weapon_speed
    update->AddField("weapon_block_targeted", weaponStats.TargetedBlockValue()); //weapon_block_targeted
    update->AddField("weapon_block_untargeted", weaponStats.UntargetedBlockValue()); //weapon_block_untargeted
    update->AddField("weapon_counterblock", weaponStats.CounterBlockValue()); //weapon_counterblock

    // Note that these are the same as armorStats.damage_protection - weaponStats is populated
    // for all kinds of items, and both are filled from the same DB columns.  So it doesn't matter.
    update->AddField("dmg_slash", weaponStats.damages[PSITEMSTATS_DAMAGETYPE_SLASH] );
    update->AddField("dmg_blunt", weaponStats.damages[PSITEMSTATS_DAMAGETYPE_BLUNT] );
    update->AddField("dmg_pierce", weaponStats.damages[PSITEMSTATS_DAMAGETYPE_PIERCE] );

    update->AddField("cstr_id_gfx_mesh", CacheManager::GetSingleton().FindCommonStringID( this->mesh_name ) );    //Mesh

    // stat requirements
    update->AddField("requirement_1_name", reqs[0].name);
    update->AddField("requirement_1_value", reqs[0].min_value);
    update->AddField("requirement_2_name", reqs[1].name);
    update->AddField("requirement_2_value", reqs[1].min_value);
    update->AddField("requirement_3_name", reqs[2].name);
    update->AddField("requirement_3_value", reqs[2].min_value);

    // equip/unequip events
    //update->AddField("equip_script", we haven't saved the script XML);
    //update->AddField("consume_script", consumeScriptName);
    update->AddField("creative_definition", this->creativeStats.creativeDefinitionXML );

    // Save this entry
    update_ok &= update->Execute(GetUID());

    return update_ok;
}

// If the item is a weapon, use a whole string
// If the item is an armor, first char is 1,2,3 and second is a,b,c
void psItemStats::SetArmorVsWeaponType(const char* v)
{
    if(!v)
        return;

    if (GetIsMeleeWeapon() || GetIsRangeWeapon())
    {
        weapon_type = v;
    }
}

void psItemStats::GetArmorVsWeaponType(csString& buff)
{
    if (GetIsMeleeWeapon() || GetIsRangeWeapon())
    {
        buff = weapon_type;
        return;
    }
    else
    {
        // Need to do some formating here since the values are splitten
        csString formated;
        switch(armorStats.Type())
        {
            case PSITEMSTATS_ARMORTYPE_LIGHT:
            {
                formated = "1";
                break;
            }
            case PSITEMSTATS_ARMORTYPE_MEDIUM:
            {
                formated = "2";
                break;
            }
            case PSITEMSTATS_ARMORTYPE_HEAVY:
            {
                formated = "3";
                break;
            }
            default:
                break;
        }

        formated += armorStats.Class();
        buff = formated;
        return;
    }

    //buff = NULL;
    // @@@ Jorrit: I think the below is the intention rather then the above?
    // Fixes a warning.
    buff.Clear ();
}

int psItemStats::GetAttackAnimID(unsigned int skill_level)
{
    if (anim_list)
    {
        int max;
        for (max = 0; max < (int)anim_list->GetSize(); max++)
        {
            if (anim_list->Get(max)->min_level_required >(int) skill_level)
                break;
        }
        int which = psserver->rng->Get(max);
        if (anim_list->Get(which)->anim_id == csInvalidStringID)
        {
            anim_list->Get(which)->anim_id = CacheManager::GetSingleton().GetMsgStrings()->Request(anim_list->Get(which)->anim_name);
            return anim_list->Get(which)->anim_id;
        }
        else
        {
            Debug2(LOG_COMBAT, uid, "Sending anim %s.\n",anim_list->Get(which)->anim_name.GetData() );
            return anim_list->Get(which)->anim_id;
        }
    }
    else
        return 0;
}

bool psItemStats::GetIsConsumable()
{
    return (flags & PSITEMSTATS_FLAG_CONSUMABLE) ? true : false;
}

bool psItemStats::GetIsMeleeWeapon()
{
    return (flags & PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON);
}

bool psItemStats::GetIsArmor()
{
    return (flags & PSITEMSTATS_FLAG_IS_ARMOR) ? true : false;
}

bool psItemStats::IsMoney()
{
    return ( (flags & PSITEMSTATS_FLAG_TRIA) ||
             (flags & PSITEMSTATS_FLAG_HEXA) ||
             (flags & PSITEMSTATS_FLAG_OCTA) ||
             (flags & PSITEMSTATS_FLAG_CIRCLE) );
}

bool psItemStats::GetIsRangeWeapon()
{
    return (flags & PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON) ? true : false;
}


bool psItemStats::GetIsAmmo()
{
    return (flags & PSITEMSTATS_FLAG_IS_AMMO) ? true : false;
}


bool psItemStats::GetIsShield()
{
    return (flags & PSITEMSTATS_FLAG_IS_A_SHIELD)? true : false ;
}


bool psItemStats::GetIsContainer()
{
    return (flags & PSITEMSTATS_FLAG_IS_A_CONTAINER) ? true : false;
}

bool psItemStats::GetIsTrap()
{
    return (flags & PSITEMSTATS_FLAG_IS_A_TRAP) ? true : false;
}

bool psItemStats::GetIsConstructible()
{
    return (flags & PSITEMSTATS_FLAG_IS_CONSTRUCTIBLE) ? true : false;
}

bool psItemStats::GetCanTransform()
{
    return (flags & PSITEMSTATS_FLAG_CAN_TRANSFORM) ? true : false;
}


bool psItemStats::GetUnmovable()
{
    return (flags & PSITEMSTATS_FLAG_NOPICKUP) ? true : false;
}


bool psItemStats::GetUsesAmmo()
{
    return (flags & PSITEMSTATS_FLAG_USES_AMMO) ? true : false;
}


bool psItemStats::GetIsStackable()
{
    return (flags & PSITEMSTATS_FLAG_IS_STACKABLE) ? true : false;
}

bool psItemStats::GetIsEquipStackable()
{
    return (flags & PSITEMSTATS_FLAG_IS_EQUIP_STACKABLE) ? true : false;
}

bool psItemStats::GetIsReadable()
{
    return (flags & PSITEMSTATS_FLAG_IS_READABLE) ? true : false;
}

bool psItemStats::GetIsWriteable()
{
    return (flags & PSITEMSTATS_FLAG_IS_WRITEABLE) ? true : false;
}

PSITEMSTATS_CREATIVETYPE psItemStats::GetCreative()
{
    return (flags & PSITEMSTATS_FLAG_CREATIVE) ? creativeStats.creativeType : PSITEMSTATS_CREATIVETYPE_NONE;
}

bool psItemStats::GetBuyPersonalise()
{
    return (flags & PSITEMSTATS_FLAG_BUY_PERSONALISE) ? true : false;
}

float psItemStats::GetRange() const
{
    return weaponRange;
}

PID psItemStats::GetCreator (PSITEMSTATS_CREATORSTATUS& creatorStatus)
{
    creatorStatus = creativeStats.creatorIDStatus;
    if (creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_VALID)
        return creativeStats.creatorID;
    return PID(0);
}

bool psItemStats::GetIsGlyph()
{
    return (flags & PSITEMSTATS_FLAG_IS_GLYPH) ? true : false;
}



uint32 psItemStats::GetUID()
{
    return uid;
}

const char *psItemStats::GetUIDStr()
{
    static char str[20];
    sprintf(str,"%u",uid);
    return str;
}

const char *psItemStats::GetName() const
{
    return name.GetData();
}

csString psItemStats::FlagsToText()
{
    csString TempString = csString();
    if (flags & PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON)
        TempString.Append("MELEEWEAPON ");
    if (flags & PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON)
        TempString.Append("RANGEDWEAPON ");
    if (flags & PSITEMSTATS_FLAG_IS_A_SHIELD)
        TempString.Append("SHIELD ");
    if (flags & PSITEMSTATS_FLAG_IS_AMMO)
        TempString.Append("AMMO ");
    if (flags & PSITEMSTATS_FLAG_IS_A_CONTAINER)
        TempString.Append("CONTAINER ");
    if (flags & PSITEMSTATS_FLAG_IS_A_TRAP)
        TempString.Append("TRAP ");
    if (flags & PSITEMSTATS_FLAG_IS_CONSTRUCTIBLE)
        TempString.Append("CONSTRUCTIBLE ");
    if (flags & PSITEMSTATS_FLAG_USES_AMMO)
        TempString.Append("USESAMMO ");
    if (flags & PSITEMSTATS_FLAG_IS_STACKABLE)
        TempString.Append("STACKABLE ");
    if (flags & PSITEMSTATS_FLAG_IS_GLYPH)
        TempString.Append("GLYPH ");
    if (flags & PSITEMSTATS_FLAG_CAN_TRANSFORM)
        TempString.Append("CANTRANSFORM ");
    if (flags & PSITEMSTATS_FLAG_TRIA)
        TempString.Append("TRIA ");
    if (flags & PSITEMSTATS_FLAG_HEXA)
        TempString.Append("HEXA ");
    if (flags & PSITEMSTATS_FLAG_OCTA)
        TempString.Append("OCTA ");
    if (flags & PSITEMSTATS_FLAG_CIRCLE)
        TempString.Append("CIRCLE ");
    if (flags & PSITEMSTATS_FLAG_CONSUMABLE)
        TempString.Append("CONSUMABLE ");
    if (flags & PSITEMSTATS_FLAG_IS_READABLE)
        TempString.Append("READABLE ");
    if (flags & PSITEMSTATS_FLAG_IS_ARMOR)
        TempString.Append("ARMOR ");
    if (flags & PSITEMSTATS_FLAG_IS_EQUIP_STACKABLE)
        TempString.Append("EQUIP_STACKABLE ");
    if (flags & PSITEMSTATS_FLAG_IS_WRITEABLE)
        TempString.Append("WRITEABLE ");
    if (flags & PSITEMSTATS_FLAG_NOPICKUP)
        TempString.Append("NOPICKUP ");
    if (flags & PSITEMSTATS_FLAG_AVERAGEQUALITY)
        TempString.Append("AVERAGEQUALITY ");
    if (flags & PSITEMSTATS_FLAG_CREATIVE)
        TempString.Append("CREATIVE ");
    if (flags & PSITEMSTATS_FLAG_BUY_PERSONALISE)
        TempString.Append("BUY_PERSONALISE ");
    if (flags & PSITEMSTATS_FLAG_IS_RECHARGEABLE)
        TempString.Append("RECHARGEABLE ");
    return(TempString);
}

const csString psItemStats::GetDownCaseName()
{
    csString downcaseName(name);
    return downcaseName.Downcase();
}

void psItemStats::SetName(const char *v)
{
    if (!v)
        return;

    CacheManager::GetSingleton().CacheNameChange(csString(name), csString(v));

    name = v;
}

void psItemStats::SaveName(void)
{
    // save to database
    psStringArray values;
    const char *fieldnames[] = {"name"};
    csString itemStatsID;

    itemStatsID.Format("%u", uid);
    values.Push(this->name);

    if ( !db->GenericUpdateWithID("item_stats", "id", itemStatsID, fieldnames, values) )
    {
        Error2("Failed to save name for item %u!", uid );
    }

}

const char *psItemStats::GetDescription() const
{
    return description;
}

void psItemStats::SetDescription(const char *v)
{
    if (!v)
        return;

    description = v;
}

void psItemStats::SaveDescription(void)
{
    // save to database
    psStringArray values;
    const char *fieldnames[] = {"description"};
    csString itemStatsID;

    itemStatsID.Format("%u", uid);
    values.Push(this->description);

    if ( !db->GenericUpdateWithID("item_stats", "id", itemStatsID, fieldnames, values) )
    {
        Error2("Failed to save description for item %u!", uid );
    }

}

float psItemStats::GetWeight()
{
    return weight;
}


unsigned short psItemStats::GetSize()
{
    return size;
}


unsigned short psItemStats::GetContainerMaxSize()
{
    return container_max_size;
}

float psItemStats::GetVisibleDistance()
{
    return visible_distance;
}

PSITEMSTATS_SLOTLIST psItemStats::GetValidSlots()
{
    return valid_slots;
}

bool psItemStats::FitsInSlots(PSITEMSTATS_SLOTLIST slotmask)
{
    if ((valid_slots & slotmask)==slotmask)
        return true;
    return false;
}

float psItemStats::GetDecayRate()
{
    return decay_rate;
}


const char *psItemStats::GetMeshName()
{
    return mesh_name;
}
void psItemStats::SetMeshName(const char *v)
{
    mesh_name=v;
}
const char *psItemStats::GetTextureName()
{
    return texture_name;
}
void psItemStats::SetTextureName(const char *v)
{
    texture_name=v;
}
const char *psItemStats::GetPartName()
{
    return texturepart_name;
}
void psItemStats::SetPartName(const char *v)
{
    texturepart_name=v;
}
const char *psItemStats::GetPartMeshName()
{
    return partmesh_name;
}
void psItemStats::SetPartMeshName(const char *v)
{
    partmesh_name=v;
}

const char *psItemStats::GetImageName()
{
    return image_name;
}
void psItemStats::SetImageName(const char *v)
{
    image_name=v;
}

void psItemStats::SetUnique()
{
    stat_type="U";
    flags &= ~PSITEMSTATS_FLAG_BUY_PERSONALISE;
}

bool  psItemStats::GetUnique()
{
    return ((stat_type=="U")?true:false);
}

void psItemStats::SetPrice(int trias)
{
    price = psMoney(trias);
}

psMoney& psItemStats::GetPrice()
{
    return price;
}

void psItemStats::SetCategory(psItemCategory * item_category)
{
    category = item_category;
}


psItemCategory * psItemStats::GetCategory()
{
    return category;
}

const char *psItemStats::GetSound()
{
    return sound;
}

void psItemStats::SetSound(const char *v)
{
    sound = csString("item.") + v;
}

bool psItemStats::SetEquipScript(const csString & equipXML)
{
    ApplicativeScript *aps = NULL;

    if (!equipXML.IsEmpty())
    {
        aps = ApplicativeScript::Create(equipXML);
        if (!aps)
        {
            Error4("Could not create ApplicativeScript for ItemStats %u (%s)'s equip script: %s.", uid, name.GetData(), equipXML.GetData());
            return false;
        }
    }

    csString escXML;
    db->Escape(escXML, equipXML);
    unsigned long res = db->Command("UPDATE item_stats SET equip_script='%s' WHERE id=%u", escXML.GetData(), uid);
    if (res == QUERY_FAILED)
    {
        Error2("sql error: %s", db->GetLastError());
        return false;
    }

    equipScript = aps;

    return true;
}

bool psItemStats::CheckRequirements( psCharacter* charData, csString& resp )
{
    float val = 0;
    csString needed = "You need to have ";
    bool first= true;

    for ( int z = 0; z < 3; z++ )
    {
        PSITEMSTATS_STAT stat = CacheManager::GetSingleton().ConvertAttributeString(reqs[z].name);
        if ( stat != PSITEMSTATS_STAT_NONE )
        {
            // Stat buffs may be negative; don't use those here
            CharStat & cs = charData->Stats()[stat];
            val = MAX(cs.Base(), cs.Current());

            // TODO: This should just use the buff always when a move from equipment to bulk can't fail
        }
        else
        {
            PSSKILL skill = CacheManager::GetSingleton().ConvertSkillString(reqs[z].name);
            if ( skill != PSSKILL_NONE )
            {
                val = charData->Skills().GetSkillRank(skill).Current();
            }
        }

        if ( val < reqs[z].min_value )
        {
            if(!first)
                needed +=" and ";
            else
                first = false;

            if(reqs[z].min_value - val > 40)
                needed += "a lot ";

            needed += "more in ";
            needed += reqs[z].name;
        }
    }

    if(first) // No needed things
    {
        resp = "None";
        return true;
    }

    needed.Append(" to equip this item");
    resp = needed;
    return false;
}

bool psItemStats::SetRequirement(const csString & statName, float statValue)
{
    CS_ASSERT(!statName.IsEmpty());

    // If it's already required, use the higher of the two requirements.
    for (int i = 0; i < 3; i++)
    {
        if (statName.CompareNoCase(reqs[i].name))
        {
            reqs[i].min_value = MAX(reqs[i].min_value, statValue);
            return true;
        }
    }

    // Otherwise try and add it.
    for (int i = 0; i < 3; i++)
    {
        if (reqs[i].name.IsEmpty())
        {
            reqs[i].name = statName;
            reqs[i].min_value = statValue;
            return true;
        }
    }

    // No space available (we're limited to three requirements by the DB)
    return false;
}

psItem *psItemStats::InstantiateBasicItem(bool transient)
{
    psItem *newitem;

    if (this->GetIsGlyph())
    {
        newitem = new psGlyph();
    }
    else
    {
        newitem = new psItem();
    }

    newitem->SetBaseStats(this);
    newitem->SetCurrentStats(this);

    if (transient)
    {
        newitem->ScheduleRemoval();  // Remove from world in a few hrs if no one picks it up
    }

    return newitem;  // MUST do newitem->Loaded() when it is safe to save!
}

bool psItemStats::SetAttribute(const csString & op, const csString & attrName, float modifier)
{
    float* value[3] = { NULL, NULL, NULL };
    // Attribute Names:
    // item
    //        weight
    //        damage
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        protection
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        speed

    csString AttributeName(attrName);
    AttributeName.Downcase();
    if ( AttributeName.Compare( "item.weight" ) )
    {
        value[0] = &this->weight;
    }
    else if ( AttributeName.Compare( "item.speed" ) )
    {
        value[0] = &this->weaponStats.latency;
    }
    else if ( AttributeName.Compare( "item.damage" ) )
    {
        value[0] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.damage.slash" ) )
    {
        value[0] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if ( AttributeName.Compare( "item.damage.pierce" ) )
    {
        value[0] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.damage.blunt" ) )
    {
        value[0] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }
    else if ( AttributeName.Compare( "item.protection" ) )
    {
        value[0] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &this->weaponStats.damages[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.protection.slash" ) )
    {
        value[0] = &this->armorStats.damage_protection[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if ( AttributeName.Compare( "item.protection.pierce" ) )
    {
        value[0] = &this->armorStats.damage_protection[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.protection.blunt" ) )
    {
        value[0] = &this->armorStats.damage_protection[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }

    // Operations  = ADD, MUL, SET
    for (int i = 0; i < 3; i++)
    {
        if (op.CompareNoCase("ADD"))
        {
            if (value[i]) *value[i] += modifier;
        }

        if (op.CompareNoCase("MUL"))
        {
            if (value[i]) *value[i] *= modifier;
        }

        if (op.CompareNoCase("VAL"))
        {
            if (value[i]) *value[i] = modifier;
        }
    }

    return true;
}

bool psItemStats::SetCreation (PSITEMSTATS_CREATIVETYPE creativeType, const csString& newCreation, csString creatorName)
{
    csString newDescription;

    if (creativeStats.SetCreativeContent(creativeType, newCreation, uid))
    {
        newDescription = creativeStats.UpdateDescription(creativeType, name, creatorName);
        SetDescription(newDescription.GetDataSafe());
        SaveDescription();
        return true;
    }

    return false;
}

void psItemStats::SetCreator (PID characterID, PSITEMSTATS_CREATORSTATUS creatorStatus)
{
    // if creator already set (i.e. valid or public) it cant be changed so return straightaway.
    if (creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_PUBLIC ||
        creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_VALID)
    {
        return;
    }

    creativeStats.creatorIDStatus = creatorStatus;
    if (creatorStatus == PSITEMSTATS_CREATOR_VALID)
    {
        creativeStats.creatorID = characterID;
    }
    else
    {
        creativeStats.creatorID = PID(0);
    }

    creativeStats.FormatCreativeContent();   // result is n/a to setting creator.
}

bool psItemStats::IsThisTheCreator(PID characterID)
{
    // if characterID is creator of this item, or no creator assigned (then anyone can edit)
    // of if the item is 'public' then any can edit it.
    if ((creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_VALID &&
         creativeStats.creatorID == characterID) ||
        creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_PUBLIC ||
        creativeStats.creatorIDStatus == PSITEMSTATS_CREATOR_UNASSIGNED)
        return true;

    return false;
}

bool psItemStats::HasCharges() const
{
    return maxCharges != -1;
}

bool psItemStats::IsRechargeable() const
{
    return (flags & PSITEMSTATS_FLAG_IS_RECHARGEABLE) ? true : false;
}


void psItemStats::SetMaxCharges(int charges)
{
    maxCharges = charges;
}

int psItemStats::GetMaxCharges() const
{
    return maxCharges;
}

