/*
 * psitem.cpp
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
#include <csutil/stringarray.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psstring.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"

#include "../playergroup.h"
#include "../psserver.h"
#include "../entitymanager.h"
#include "../cachemanager.h"
#include "../exchangemanager.h"
#include "../gem.h"
#include "../events.h"
#include "../spawnmanager.h"
#include "../psserverchar.h"
#include "../progressionmanager.h"
#include "../scripting.h"
#include "../globals.h"
#include "../adminmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "client.h"
#include "psguildinfo.h"
#include "psitem.h"
#include "pscharacter.h"
#include "pscharacterloader.h"
#include "pssectorinfo.h"
#include "pstrade.h"
#include "psmerchantinfo.h"
#include "psraceinfo.h"

#if SAVE_DEBUG
#include <typeinfo>
#endif

#if SAVE_TRACER
#include <csutil/callstack.h>
#endif

/**
 * This class handles auto-removal of transient objects
 * (objects placed in the world by players, basically).
 */
class psItemRemovalEvent : public psGameEvent
{
protected:
    EID item_to_remove;

public:
    psItemRemovalEvent(int delayticks, EID gemID)
        : psGameEvent(0,delayticks*1000,"psItemRemovalEvent")
    {
        item_to_remove = gemID;
    }


    virtual void Trigger()
    {
        Debug2(LOG_USER, 0, "Removing object %s now.\n", ShowID(item_to_remove));
        // cannot store pointer because object may have already been removed and ptr not valid
        gemObject *obj = GEMSupervisor::GetSingleton().FindObject(item_to_remove);
        if (obj)
        {
            psItem *item = obj->GetItem();
            if (item)
            {
                // Is the item being guarded?
                InstanceID instance = obj->GetInstance();

                // Do checks
                PID guardCharacterID = item->GetGuardingCharacterID();
                gemActor* guardActor = GEMSupervisor::GetSingleton().FindPlayerEntity(guardCharacterID);
                if (guardCharacterID.IsValid() &&
                    guardActor &&
                    guardActor->RangeTo(item->GetGemObject()) < RANGE_TO_SELECT &&
                    (guardActor->GetInstance() == instance))
                {
                    // Item is guarded, reschedule
                    item->ScheduleRemoval();
                } else {
                    // Item isn't guarded, remove
                    EntityManager::GetSingleton().RemoveActor(obj);
                    item->Destroy();  // obj is deleted in RemoveActor
                }
            }
        }
    }
};





// Definition of the itempool for psItems
PoolAllocator<psItem> psItem::itempool;

void *psItem::operator new(size_t allocSize)
{
    CS_ASSERT(allocSize<=sizeof(psItem));
    return (void *)itempool.CallFromNew();
}

void psItem::operator delete(void *releasePtr)
{
    itempool.CallFromDelete((psItem *)releasePtr);
}

psItem::psItem() : transformationEvent(NULL), gItem(NULL), pendingsave(false), loaded(false)
{
    int i;

    uid=0;

    loc_in_parent = PSCHARACTER_SLOT_NONE;
    item_in_use   = false;

////////    for (i=0;i<GetContainerMaxSlots();i++)
////////        container_data.contained_item_ptr[i]=NULL;

    decay_resistance = 0;
    item_quality = 0;
    crafted_quality = -1;

    stack_count=1; // There will allways be one of a item.
                   // The GetIsStackable is to be used to check if this item
                   // is stackable.
    flags      = 0;
    crafter_id = 0;
    guild_id   = 0;
    parent_item_InstanceID = 0;

//    container_in_location=(unsigned int)-1;
    base_stats=NULL;
    current_stats=NULL;
    owning_character=NULL;
    owningCharacterID = 0;
    guardingCharacterID = 0;

    location.loc_sectorinfo=NULL;
    location.worldInstance= DEFAULT_INSTANCE;
    location.loc_x=0.0f;
    location.loc_y=0.0f;
    location.loc_z=0.0f;
    location.loc_xrot=0.0f;
    location.loc_yrot=0.0f;
    location.loc_zrot=0.0f;

    for (i=0;i<PSITEM_MAX_MODIFIERS;i++)
        modifiers[i]=NULL;

    lockStrength = 0;
    lockpickSkill = PSSKILL_NONE;
    schedule = NULL;
}

psItem::~psItem()
{
    // printf("In item %s:%u dtor...\n", GetName(), uid);

    if (!current_stats)
        return;

    if (item_in_use)
    {
        Error2("Item %s is being deleted while in use!\n", GetName() );
    }

    if (schedule)
    {
        delete schedule; // Finally delete the pattern used for spawning this item
        schedule = NULL;
    }

    // If this is a uniq item delete the item state
    if (base_stats!=NULL && flags & PSITEM_FLAG_UNIQUE_ITEM )
    {
        delete base_stats;
        base_stats = NULL;
        flags |= PSITEM_FLAG_USES_BASIC_ITEM;
        flags &= ~PSITEM_FLAG_UNIQUE_ITEM;
    }


    if (item_quality != item_quality_original )
    {
        UpdateItemQuality(uid, item_quality);
    }

    if (gItem)
        gItem->UnregisterCallback(this);
    gItem = NULL;
}

void psItem::UpdateItemQuality(uint32 id, float qual)
{
    if (!id || id==ID_DONT_SAVE_ITEM)
        return;

    Debug3(LOG_USER,id,"UpdateItemQuality(%u,%1.2f)\n",id, qual);
    int ret = db->CommandPump("update item_instances set item_quality=%1.2f where id=%u",qual, id);
    if (ret == 0 && strlen(db->GetLastError())) // 0 updates could mean the value was the same, not an error
    {
        Error3("Could not update item quality.  SQL was <%s> and error was <%s>",db->GetLastQuery(),db->GetLastError());
    }
}

const char* psItem::GetQualityString()
{
    // Create a string basedi on items crafted quality
    if ( crafted_quality >= 250 )
        return "Finest";
    else if ( crafted_quality >= 200 )
        return "Extraordinary";
    else if ( crafted_quality >= 150 )
        return "Superior";
    else if ( crafted_quality >= 100 )
        return "Standard";
    else if ( crafted_quality >= 50 )
        return "Common";
    return "Inferior";
}

// Functions that manipulate psItem Data

bool psItem::Load(iResultRow& row)
{
    // Begin filling in the item properties
    // Item Unique ID #
    SetUID(row.GetUInt32("id"));
    CS_ASSERT(uid != 0);
    parent_item_InstanceID = row.GetUInt32("parent_item_id");
    loc_in_parent = (INVENTORY_SLOT_NUMBER) row.GetInt("location_in_parent");

    // Stack count (will be 0 if NULL - either means this is a non stackable item)
    stack_count=(unsigned short)row.GetInt("stack_count");

    // Clamp stacks so bugs resulting in huge item counts don't persist
    if (stack_count > MAX_STACK_COUNT)
        stack_count = MAX_STACK_COUNT;

    if (row.GetInt("creator_mark_id"))
    {
        SetCrafterID(PID(row.GetInt("creator_mark_id")));
    }
    if (row.GetInt("guild_mark_id"))
    {
        SetGuildID(row.GetInt("guild_mark_id"));
    }
    if (row.GetFloat("decay_resistance"))
    {
        SetDecayResistance(row.GetFloat("decay_resistance"));
    }
    else
    {
        SetDecayResistance(0);
    }

    // Flags
    psString flagstr(row["flags"]);
    if (flagstr.FindSubString("LOCKED",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_LOCKED;
    }
    if (flagstr.FindSubString("LOCKABLE",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_LOCKABLE;
    }
    if (flagstr.FindSubString("SECURITYLOCK",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_SECURITYLOCK;
    }
    if (flagstr.FindSubString("UNPICKABLE",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_UNPICKABLE;
    }
    if (flagstr.FindSubString("KEY",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_KEY;
    }
    if (flagstr.FindSubString("MASTERKEY",0,true)!=-1)
    {
        flags |= PSITEM_FLAG_MASTERKEY;
    }
    if (flagstr.FindSubString("PURIFIED",0,true)!=-1)
    {
       flags |= PSITEM_FLAG_PURIFIED;
    }
    if (flagstr.FindSubString("PURIFYING",0,true)!=-1)
    {
       flags |= PSITEM_FLAG_PURIFIED;
    }
    if (flagstr.FindSubString("NOPICKUP",0,true)!=-1)
    {
       flags |= PSITEM_FLAG_NOPICKUP;
    }
    if (flagstr.FindSubString("TRANSIENT",0,true)!=-1)
    {
       flags |= PSITEM_FLAG_TRANSIENT;
    }
    if (flagstr.FindSubString("NPCOWNED", 0, true) != -1)
    {
        flags |= PSITEM_FLAG_NPCOWNED;
    }
    if (flagstr.FindSubString("USECD", 0, true) != -1)
    {
        flags |= PSITEM_FLAG_USE_CD;
    }
    if (flagstr.FindSubString("UNSTACKABLE", 0, true) != -1)
    {
        flags |= PSITEM_FLAG_UNSTACKABLE;
        flags &= ~PSITEM_FLAG_STACKABLE;
    }
    if (flagstr.FindSubString("STACKABLE", 0, true) != -1)
    {
        flags |= PSITEM_FLAG_STACKABLE;
        flags &= ~PSITEM_FLAG_UNSTACKABLE;
    }
    if (flagstr.FindSubString("SETTINGITEM", 0, true) != -1)
    {
        flags |= PSITEM_FLAG_SETTINGITEM;
    }

    // Lockpick stuff
    SetLockStrength(row.GetInt("lock_str"));
    SetLockpickSkill((PSSKILL)row.GetInt("lock_skill"));

    // load openableLocks
    psString olstr(row["openable_locks"]);
    psString w;
    olstr.GetWordNumber(1, w);
    for (int n = 2; w.Length(); olstr.GetWordNumber(n++, w))
    {
        if (w == "SKEL")
            openableLocks.Push(KEY_SKELETON);
        else
        {
            unsigned int u;
            sscanf(w.GetData(), "%u", &u);
            openableLocks.Push(u);
        }
    }

    unsigned int stats_id=row.GetUInt32("item_stats_id_standard");
    psItemStats *stats=CacheManager::GetSingleton().GetBasicItemStatsByID(stats_id);
    if (!stats)
    {
        Error3("Item with id %s has unresolvable basic item stats id %u",row["id"],stats_id);
        return false;
    }
    SetBaseStats(stats);

    if (row.GetFloat("item_quality"))
    {
        SetItemQuality(row.GetFloat("item_quality"));
    }
    else
    {
        SetItemQuality(GetMaxItemQuality());
    }

    SetCharges(row.GetInt("charges"));

    // Set the crafted quality for this item.
    crafted_quality = row.GetFloat("crafted_quality");

    owningCharacterID = row.GetUInt32("char_id_owner");
    guardingCharacterID = row.GetUInt32("char_id_guardian");

    if(row.GetInt("location_in_parent") == -1 ||  // SLOT_NONE
      (row.GetInt("location_in_parent") == 0 &&
       row.GetInt("char_id_owner") == 0 &&
       row.GetInt("parent_item_id") == 0)) // No owner and no slot
    {
        float x,y,z,xrot,yrot,zrot;
        InstanceID instance;

        instance = row.GetUInt32("loc_instance");
        //        printf("KWF: Item instance=%d\n", instance);

        psSectorInfo *itemsector=CacheManager::GetSingleton().GetSectorInfoByID(row.GetInt("loc_sector_id"));
        if (!itemsector)
        {
            Error4("Item %s(%s) Could not be loaded\nIt is in sector id %s which does not resolve\n",
                   GetName(), row["id"], row["loc_sector_id"]);
            return false;
        }

        x = row.GetFloat("loc_x");
        y = row.GetFloat("loc_y");
        z = row.GetFloat("loc_z");
        xrot = row.GetFloat("loc_xrot");
        yrot = row.GetFloat("loc_yrot");
        zrot = row.GetFloat("loc_zrot");
        SetLocationInWorld(instance,itemsector,x,y,z,yrot);
        SetXZRotationInWorld(xrot,zrot);
    }
    else // in inventory, they have no location
    {
        SetLocationInWorld(0,NULL,0,0,0,0);
    }


    // TODO:Modifiers loaded and resolved later


    // Unique item handling, stats specified are deltas to standard stats

    stats_id = row.GetUInt32("item_stats_id_unique");
    if (stats_id)
    {

        Result result(db->Select("SELECT * from item_stats where id=%u",stats_id));
        if (!result.IsValid())
        {
            Error3("Item with id %s has unresolvable unique item stats id %u",row["id"],stats_id);
            return false;
        }

        psItemStats *stats=new psItemStats;
        if (!stats->ReadItemStats(result[0]))
        {
            Error3("Item with id %s has unique item stats that cannot be parsed (unique item stats id %u)",row["id"],stats_id);
            delete stats;
            return false;
        }
        SetUniqueStats(stats);

    }

    item_name = row["item_name"];
    item_description = row["item_description"];

    return true;
}

void psItem::Save(bool children)
{
    CS_ASSERT(!(loc_in_parent == -1 && owning_character && parent_item_InstanceID==0));

    if (loaded && !pendingsave)
    {
#if SAVE_TRACER
        csCallStack* stack = csCallStackHelper::CreateCallStack(0,true);
        if (stack)
        {
            /// Store the function that queued this save (check this with a debugger if Commit() fails)
            last_save_queued_from = stack->GetEntryAll(1,true);

#if SAVE_DEBUG
            printf("\n%s::Save() for '%s', queued from stack:\n", typeid(*(item)).name(), GetName() );
            stack->Print();
            printf("\n");
#endif

            stack->Free();
        }
        else
        {
            Bug2("Could not get call stack for %p!",this);
            last_save_queued_from = "ERROR:  csCallStackHelper::CreateCallStack(0,true) returned NULL!";
        }
#elif SAVE_DEBUG
        printf("%s::Save() for '%s' queued\n", typeid(*(item)).name(), GetName() );
#endif

        pendingsave = true;
        Commit(children);
    }

#if SAVE_DEBUG
    else if (loaded) printf("%s::Save() for '%s' skipped\n", typeid(*(GetSafeReference()->item)).name(), GetName() );
#endif
}

void psItem::Commit(bool children)
{
    if (!pendingsave || uid == ID_DONT_SAVE_ITEM)
        return;

    pendingsave = false;

    if (!loaded)
        return;

    static iRecord* updateQuery;
    static iRecord* insertQuery;

    iRecord* targetQuery;

    if (GetUID()==0)
    {
        if(insertQuery == NULL)
            insertQuery = db->NewInsertPreparedStatement("item_instances", 26, __FILE__, __LINE__); // 26 fields
        targetQuery = insertQuery;
    }
    else
    {
        if(updateQuery == NULL)
            updateQuery = db->NewUpdatePreparedStatement("item_instances", "id", 27, __FILE__, __LINE__); // 26 fields + 1 id field
        targetQuery = updateQuery;
    }

    targetQuery->Reset();

    targetQuery->AddField("char_id_owner", owning_character ? owning_character->GetPID().Unbox() : 0);
    targetQuery->AddField("char_id_guardian", guardingCharacterID.Unbox());
    targetQuery->AddField("stack_count",GetStackCount());
    targetQuery->AddField("item_quality",GetItemQuality());
    targetQuery->AddField("crafted_quality",GetMaxItemQuality());
    targetQuery->AddField("decay_resistance",GetDecayResistance());

    // Crafter ID
    if (GetIsCrafterIDValid())
        targetQuery->AddField("creator_mark_id", GetCrafterID().Unbox());
    else
        targetQuery->AddFieldNull("creator_mark_id");

    // Guild ID
    if (GetIsGuildIDValid())
        targetQuery->AddField("guild_mark_id",GetGuildID());
    else
        targetQuery->AddFieldNull("guild_mark_id");

    // Flags
    csString flagString;

    // Thise two are actualy glyhs things and should be moved to psGlyph
    // if a generic way of updating flags are implemented.
    if (flags & PSITEM_FLAG_PURIFIED)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("PURIFIED");
    }
    if (flags & PSITEM_FLAG_PURIFYING)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("PURIFYING");
    }
    if (flags & PSITEM_FLAG_LOCKED)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("LOCKED");
    }
    if (flags & PSITEM_FLAG_LOCKABLE)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("LOCKABLE");
    }
    if (flags & PSITEM_FLAG_SECURITYLOCK)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("SECURITYLOCK");
    }
    if (flags & PSITEM_FLAG_UNPICKABLE)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("UNPICKABLE");
    }
    if (flags & PSITEM_FLAG_KEY)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("KEY");
    }
    if (flags & PSITEM_FLAG_MASTERKEY)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("MASTERKEY");
    }
    if (flags & PSITEM_FLAG_NOPICKUP)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("NOPICKUP");
    }
    if (flags & PSITEM_FLAG_TRANSIENT)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("TRANSIENT");
    }
    if (flags & PSITEM_FLAG_NPCOWNED)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("NPCOWNED");
    }
    if (flags & PSITEM_FLAG_USE_CD)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("USECD");
    }
    if (flags & PSITEM_FLAG_UNSTACKABLE)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("UNSTACKABLE");
    }
    if (flags & PSITEM_FLAG_STACKABLE)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("STACKABLE");
    }
    if (flags & PSITEM_FLAG_SETTINGITEM)
    {
        if (!flagString.IsEmpty()) flagString.Append(",");
        flagString.Append("SETTINGITEM");
    }

    targetQuery->AddField("flags",flagString);

    // item_stats_id_standard - base stats if non unique, unique stats if unique
    targetQuery->AddField("item_stats_id_standard",GetBaseStats()->GetUID());

    // Container stuff
    if (!parent_item_InstanceID)  // if not in container
    {
        targetQuery->AddFieldNull("parent_item_id");  // id of object containing this one
        targetQuery->AddField("location_in_parent",loc_in_parent);  // slot number, or -1 if out in the world
    }
    else // in container
    {
        targetQuery->AddField("parent_item_id",parent_item_InstanceID);
        targetQuery->AddField("location_in_parent",loc_in_parent);
    }


    float locx,locy,locz,locxrot,locyrot,loczrot;
    psSectorInfo *sectorinfo;
    InstanceID instance;

    GetLocationInWorld(instance,&sectorinfo,locx,locy,locz,locyrot);
    GetXZRotationInWorld(locxrot,loczrot);

    if (!sectorinfo || parent_item_InstanceID)
    {
        targetQuery->AddFieldNull("loc_x");
        targetQuery->AddFieldNull("loc_y");
        targetQuery->AddFieldNull("loc_z");
        targetQuery->AddFieldNull("loc_xrot");
        targetQuery->AddFieldNull("loc_yrot");
        targetQuery->AddFieldNull("loc_zrot");
        targetQuery->AddFieldNull("loc_sector_id");
    }
    else  // Item is not held or in something; must be in the world
    {
        if ( sectorinfo )
        {
            targetQuery->AddField("loc_x",locx);
            targetQuery->AddField("loc_y",locy);
            targetQuery->AddField("loc_z",locz);
            targetQuery->AddField("loc_xrot",locxrot);
            targetQuery->AddField("loc_yrot",locyrot);
            targetQuery->AddField("loc_zrot",loczrot);
            targetQuery->AddField("loc_sector_id",sectorinfo->uid);
        }
        else  //  Item is nowhere; cannot be saved
        {
            Error3("Item %s(%u) could not be saved because it is in a null location and has no parent or character owner", GetName(), GetUID() );
            CS_ASSERT(!"Attempt to save item without a parent in a NULL position");
        }
    }

    targetQuery->AddField("lock_str",GetLockStrength());
    targetQuery->AddField("lock_skill",GetLockpickSkill());

    // push openableLocks
    csString openableLocksString;
    csArray<unsigned int>::Iterator iter = openableLocks.GetIterator();
    while(iter.HasNext())
    {
        csString tmp;
        unsigned int n = iter.Next();
        if (!openableLocksString.IsEmpty()) openableLocksString.Append(" "); // Space to sparate since GetWordNumber is used to decode
        if (n == KEY_SKELETON)
            openableLocksString.Append("SKEL");
        else
        {
            tmp.Format( "%u", n);
            openableLocksString.Append(tmp);
        }
    }
    targetQuery->AddField("openable_locks", openableLocksString);
    targetQuery->AddField("loc_instance", instance);

    targetQuery->AddField("item_name", item_name);
    targetQuery->AddField("item_description", item_description);

    targetQuery->AddField("charges",GetCharges());

    if (GetUID()==0)
    {
        //printf("Saving item %s through SQL Insert\n",GetName() );

        // Insert to item_instances and set the ID to the new ID that the db gives us
        if(targetQuery->Execute(0))
        {
            item_quality_original = item_quality;
            SetUID(db->GetLastInsertID());
        }
        else
            Error2("Failed to insert item instance!\nError: %s", db->GetLastError());


    }
    else
    {
        // Existing Item, update
        //printf("Saving item %d (%s owned by %s) through SQL Update\n", GetUID(), GetName(), owning_character? owning_character->GetCharName():"no one" );

        // Save this entry

        if ( !targetQuery->Execute(GetUID()) )
        {
            Error3("Failed to save item instance %u!\nError: %s", GetUID(), db->GetLastError());
        }
        else
            item_quality_original = item_quality;
    }
}

void psItem::ForceSaveIfNew()
{
    if (GetUID()==0)
    {
        // No UID. Force a save now to generate one.
        pendingsave = true;
        Commit();
    }
    else
    {
        // Already saved. Queue normal save.
        Save(false);
    }
}

INVENTORY_SLOT_NUMBER psItem::GetLocInParent(bool adjustSlot)
{
    if (adjustSlot && parent_item_InstanceID)
    {
        psItem *container = owning_character->Inventory().FindItemID(parent_item_InstanceID);
        if (!container)
        {
//            Error3("Bad container id %d in item %d.",parent_item_InstanceID,uid);
            return loc_in_parent;
        }
        int slot = container->GetLocInParent(false);
        return (INVENTORY_SLOT_NUMBER)(slot*100 + loc_in_parent);
    }
    else
    {
        return loc_in_parent;
    }
}


bool psItem::GetIsCrafterIDValid()
{
    return (flags & PSITEM_FLAG_CRAFTER_ID_IS_VALID);
}

void psItem::SetIsCrafterIDValid(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_CRAFTER_ID_IS_VALID;
    else
        flags=flags & ~PSITEM_FLAG_CRAFTER_ID_IS_VALID;
}

bool psItem::GetIsGuildIDValid()
{
    return (flags & PSITEM_FLAG_GUILD_ID_IS_VALID) ? true : false;
}

void psItem::SetIsGuildIDValid(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_GUILD_ID_IS_VALID;
    else
        flags=flags & ~PSITEM_FLAG_GUILD_ID_IS_VALID;
}

bool psItem::GetIsUnique() const
{
    return (flags & PSITEM_FLAG_UNIQUE_ITEM) ? true : false;
}

void psItem::SetUID(uint32 v)
{
    uid=v;
}

void psItem::SetStackCount(unsigned short v)
{
    CS_ASSERT(v <= MAX_STACK_COUNT && v > 0);
    stack_count=v;
}

void psItem::SetCrafterID(PID v)
{
    SetIsCrafterIDValid(true);
    crafter_id=v;
}

void psItem::SetGuildID(unsigned int v)
{
    SetIsGuildIDValid(true);
    guild_id=v;
}

float psItem::GetMaxItemQuality() const
{
    if ( crafted_quality == -1 )
        return current_stats->GetQuality();
    else
        return crafted_quality;
}

void psItem::SetMaxItemQuality(float v)
{
    if (v > 300)
        v = 300; // Clamp item quality to no more than 300

    crafted_quality = v;
}

float psItem::GetItemQuality() const
{
    return item_quality;
}

void psItem::SetItemQuality(float v)
{
    if (v > 300)
        v = 300; // Clamp item quality to no more than 300

    item_quality = v;
    item_quality_original = v;
}

void psItem::SetDecayResistance(float v)
{
    decay_resistance=v;
}

float psItem::AddDecay(float severityFactor)
{
    if (!this)
        return 0;

    item_quality -= base_stats->GetDecayRate() * severityFactor * (1.0F-decay_resistance);
    if (item_quality < 1)
    {
        item_quality = 1;
    }
    Debug4(LOG_USER,0,"Item %s quality decayed by %1.2f to %1.2f.\n", GetName(), base_stats->GetDecayRate()*severityFactor*(1.0F-decay_resistance), item_quality);
    return item_quality;
}

int psItem::GetRequiredRepairTool()
{
    if (GetCategory())
        return GetCategory()->repairToolStatId;
    else
        return 0;
}

bool psItem::GetRequiredRepairToolConsumed()
{
    if (GetCategory())
        return GetCategory()->repairToolConsumed;
    else
        return false;
}

int psItem::GetIdentifySkill()
{
    if (GetCategory())
        return GetCategory()->identifySkillId;
    else
        return 0;
}

int psItem::GetIdentifyMinSkill()
{
    if (GetCategory())
        return GetCategory()->identifyMinSkill;
    else
        return 0;
}

void psItem::GetLocationInWorld(InstanceID &instance,psSectorInfo **sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot) const
{
    instance    = location.worldInstance;
    *sectorinfo = location.loc_sectorinfo;
    loc_x       = location.loc_x;
    loc_y       = location.loc_y;
    loc_z       = location.loc_z;
    loc_yrot    = location.loc_yrot;
}

void psItem::SetLocationInWorld(InstanceID instance,psSectorInfo *sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot)
{
    location.worldInstance  = instance;
    location.loc_sectorinfo = sectorinfo;
    location.loc_x          = loc_x;
    location.loc_y          = loc_y;
    location.loc_z          = loc_z;
    location.loc_yrot       = loc_yrot;
}

void psItem::GetXZRotationInWorld(float &loc_xrot, float &loc_zrot)
{
    loc_xrot    = location.loc_xrot;
    loc_zrot    = location.loc_zrot;
}

void psItem::SetXZRotationInWorld(float loc_xrot, float loc_zrot)
{
    location.loc_xrot = loc_xrot;
    location.loc_zrot = loc_zrot;
}

void psItem::GetRotationInWorld(float &loc_xrot, float &loc_yrot, float &loc_zrot)
{
    loc_xrot = location.loc_xrot;
    loc_yrot = location.loc_yrot;
    loc_zrot = location.loc_zrot;
}

void psItem::SetRotationInWorld(float loc_xrot, float loc_yrot, float loc_zrot)
{
    location.loc_xrot = loc_xrot;
    location.loc_yrot = loc_yrot;
    location.loc_zrot = loc_zrot;
}

csString psItem::GetQuantityName()
{
    return GetQuantityName(GetName(),stack_count, GetCreative());
}

csString psItem::GetQuantityName(const char *namePtr, int stack_count, PSITEMSTATS_CREATIVETYPE creativeType, bool giveDetail)
{
    psString name(namePtr);
    if (name.IsEmpty())
        return "???";

    csString list;
    // sort out name for creative items like books & maps
    if (creativeType != PSITEMSTATS_CREATIVETYPE_NONE)
    {
        psString creativeDesc;

        if (creativeType == PSITEMSTATS_CREATIVETYPE_LITERATURE)
            creativeDesc = "book";
        else if (creativeType == PSITEMSTATS_CREATIVETYPE_SKETCH)
            creativeDesc = "map";
        else
            creativeDesc = "???";

        if (giveDetail)
        {
            if (stack_count == 1)
            {
                list.Format("%s ", (creativeDesc.IsVowel(0))?"an":"a");
            }
            else
            {
                creativeDesc.Plural();
                list.Format("%d ", stack_count);
            }

            list.AppendFmt("%s called \"%s\"", creativeDesc.GetData(), name.GetData());
            return list;
        }
        else
            name = creativeDesc;
    }

    // normal items like swords, etc etc
    if (stack_count == 1)
    {
        list.Format("%s %s", (name.IsVowel(0))?"an":"a", name.GetData() );
    }
    else
    {
        name.Plural(); // Get plural form of the name
        list.Format("%d %s", stack_count, name.GetData() );
    }

    return list;
}


void psItem::SetOwningCharacter(psCharacter *owner)
{
    owning_character = owner;
    owningCharacterID = owner ? owner->GetPID() : 0;

    // Owned items are always in inventory, never in the world.
    // Also, an item is owned, not guarded, when in the inventory.
    if (owner)
    {
        guardingCharacterID = 0;
        SetLocationInWorld(0, NULL, 0, 0, 0, 0);
    }
}

void psItem::SetUniqueStats(psItemStats *statptr)
{
    // Consider weight changes
    float weight_delta=0.0f;

    // If base_stats is NULL then this is a new item. No chickens here, just us egg.
    if (base_stats!=NULL)
        weight_delta-=GetWeight();

    if (current_stats==base_stats)
        current_stats=statptr;

    base_stats=statptr;

    // Clear the "uses a stock basic item" flag and set the "uses a unique item stats entry" flag
    flags &= ~PSITEM_FLAG_USES_BASIC_ITEM;
    flags |= PSITEM_FLAG_UNIQUE_ITEM;


    if (current_stats!=base_stats)
        RecalcCurrentStats();

    weight_delta+=GetWeight();

//    if (weight_delta!=0.0f)
//        AdjustSumWeight(weight_delta);
}



void psItem::SetBaseStats(psItemStats *statptr)
{
    // Consider weight changes
    float weight_delta=0.0f;

    // If base_stats is NULL then this is a new item. No chickens here, just us egg.
    if (base_stats!=NULL)
    {
        weight_delta-=GetWeight();
        // Consider the possibility of this previously being a unique item
        if (flags & PSITEM_FLAG_UNIQUE_ITEM)
        {
            // Note we delete here and then quickly reassign base_stats below after a pointer comparison
            delete base_stats;
            flags |= PSITEM_FLAG_USES_BASIC_ITEM;
            flags &= ~PSITEM_FLAG_UNIQUE_ITEM;
        }
    }

    // Consider quality
    SetItemQuality(statptr->GetQuality());

    // Set pickupability based on movability, but only if the item is currently pickupable
    if ((flags & PSITEM_FLAG_NOPICKUP) == 0)
        SetIsPickupable(!statptr->GetUnmovable());

    if (current_stats==base_stats)
        current_stats=statptr;

    base_stats=statptr;

    if (current_stats!=base_stats)
        RecalcCurrentStats();

    weight_delta+=GetWeight();

    SetCharges(statptr->GetMaxCharges());

//    if (weight_delta!=0.0f)
//        AdjustSumWeight(weight_delta);
}

void psItem::UpdateInventoryStatus(psCharacter *owner,uint32 parent_id, INVENTORY_SLOT_NUMBER slot)
{
    if (IsEquipped() && owning_character)
        owning_character->Inventory().Unequip(this);

    SetOwningCharacter(owner);
    parent_item_InstanceID = parent_id;
    loc_in_parent           = (INVENTORY_SLOT_NUMBER)(slot%100);

    if (IsEquipped() && owning_character)
        owning_character->Inventory().Equip(this);
}

void psItem::SetCurrentStats(psItemStats *statptr)
{
    current_stats=statptr;
}

void psItem::RecalcCurrentStats()
{
    int i;
    bool has_modifiers=false;
    /* If there are no modifiers and no effects, then current_stats should equal base_stats.
     *  If the current stats pointer is not the same as the base stats or any of the modifiers
     *  then it may need adjustment.
     *
     */
    if (current_stats==base_stats)
        return;
    for (i=0;i<PSITEM_MAX_MODIFIERS;i++)
    {
        if (current_stats==modifiers[i])
            return;
        if (modifiers[i]!=NULL)
            has_modifiers=true;
    }



    // TODO:  When effects are added, take them into consideration here

    // TODO:  When modifiers are defined and the operations of how they apply to base stats is defined, implement that logic here.

    // For now, if there are no modifiers we presume that current_stats should = base_stats and that current_stats can be freed.
    if (!has_modifiers && current_stats!=NULL)
        delete current_stats;
    current_stats=base_stats;
}


bool psItem::HasModifier(psItemStats *modifier)
{
    int i;
    for (i=0;i<PSITEM_MAX_MODIFIERS;i++)
    {
        if (modifier==modifiers[i])
            return true;
    }
    return false;
}


bool psItem::AddModifier(psItemStats *modifier)
{
    int mod_number;

    for (mod_number=0;mod_number<PSITEM_MAX_MODIFIERS;mod_number++)
    {
        if (modifiers[mod_number]==NULL)
            break;
    }
    // No free modifier slots available
    if (mod_number==PSITEM_MAX_MODIFIERS)
        return false;

    // TODO:  We need to apply the modifier to the current stats of the item as well
    modifiers[mod_number]=modifier;

    /*  Modifiers can be applied to unique items but the effects of the modifier do not get rolled in.
     *  Instead, this allows modifiers to be used as a "flag" in some cases.
     *
     *
     */
    if (GetIsUnique())
        return true;

    /* if (current_stats==base_stats)
     * {
     *    // Create a new temporary stat entry that's a copy of the base stats
     *    current_stats=base_stats->CloneTemporary();
     * }
     * // Add this modifier's stats to the current_stats
     */

    return true;
}

psItemStats *psItem::GetModifier(int index)
{
    if (index<0 || index>=PSITEM_MAX_MODIFIERS)
        return NULL;
    return modifiers[index];
}

bool psItem::IsEquipped() const
{
    return (loc_in_parent < PSCHARACTER_SLOT_BULK1
        && loc_in_parent >= 0);
}

bool psItem::IsActive() const
{
    return (flags & PSITEM_FLAG_ACTIVE) != 0;
}

void psItem::SetActive(bool state)
{
    if(state)
    {
        flags |= PSITEM_FLAG_ACTIVE;
    }
    else
    {
        flags &= ~PSITEM_FLAG_ACTIVE;
    }
}

void psItem::RunEquipScript(gemActor *actor)
{
    if (IsActive())
    {
        Warning2(LOG_SCRIPT, "Didn't run equip script for item \"%s\" because it's already active!", GetName());
        return;
    }

    ApplicativeScript *script = base_stats->GetEquipScript();
    if (!script)
        return;

    MathEnvironment env;
    env.Define("Actor", actor);
    env.Define("Item", this);
    equipActiveSpell = script->Apply(&env);
    equipActiveSpell->SetCancelOnDeath(false);

    SetActive(true);
}

void psItem::CancelEquipScript()
{
    if (!IsActive())
        return;

    SetActive(false);

    if (equipActiveSpell && equipActiveSpell->Cancel())
    {
        delete equipActiveSpell;
        equipActiveSpell = NULL;
    }
}

bool psItem::CheckStackableWith(const psItem *otheritem, bool precise) const
{
    int i;

    // If we want precise quality check and qualities are different exit
    if (GetItemQuality()!=otheritem->GetItemQuality() && precise)
        return false;

    // If we want precise quality check and max qualities are different exit
    if (GetMaxItemQuality()!=otheritem->GetMaxItemQuality() && precise)
        return false;

    // If we want precise quality check and they were crafted by different folk then exit
    if (GetCrafterID()!=otheritem->GetCrafterID() && precise)
        return false;

    // TODO:  Should unique items ever be stackable?
    if (GetIsUnique())
        return false;

    if (!GetIsEquipStackable() && IsEquipped())
        return false;

    int purifyStatus = GetPurifyStatus();
    if (purifyStatus == 1)                      // purifying glyphs cannot be stacked
        return false;
    int otherPurifyStatus = otheritem->GetPurifyStatus();
    if (purifyStatus != otherPurifyStatus)      // glyphs with different purification status cannot be stacked
        return false;

    if (!GetIsStackable() || !otheritem->GetIsStackable())
        return false;

    if (otheritem->GetStackCount() > MAX_STACK_COUNT - stack_count)
        return false;

    if (strcmp(GetName(), otheritem->GetName()) || strcmp(GetDescription(), otheritem->GetDescription()))
        return false;

    /* Conditions that must be met for stacking:
     * 1) Base item stats point to the same entry
     * 2) If there are any modifiers they must point to the same entry in the same order
     * 3) No effects can be applied.
     * 4) Crafting attributes match
     */

    if (GetBaseStats()!=otheritem->GetBaseStats())
        // If these have different base stats it doesn't matter about modifiers - they cant be combined
        return false;

    // TODO: Instead of checking each modifier, we can compare the resulting stats and see if they are equivalent
    // This requires implementing a comparison function in psItemStats
    for (i=0;i<PSITEM_MAX_MODIFIERS;i++)
    {
        if (modifiers[i]!=otheritem->modifiers[i])
            return false;
    }

    // Check same instance of world
    if (location.worldInstance != otheritem->location.worldInstance)
        return false;

    // Check for keys
    if (GetIsKey() != otheritem->GetIsKey() || GetIsMasterKey() != otheritem->GetIsMasterKey())
        return false;

    if (GetIsKey())
    {
        // Both are either keys or master keys
        if (!CompareOpenableLocks(otheritem) ||
            !otheritem->CompareOpenableLocks(this))
            return false;
    }

    // if both are non null and different
    if (GetGuardingCharacterID() != otheritem->GetGuardingCharacterID() &&
        GetGuardingCharacterID() != 0 && otheritem->GetGuardingCharacterID() != 0)
        return false;

    // TODO: Check effects

    if (GetCurrentStats()==otheritem->GetCurrentStats())
    {
        return true;
    }

    // This checks to make sure that if the quality is different that these
    // items can still be stacked and use an average qualiy system.
    if (item_quality != otheritem->item_quality || GetMaxItemQuality() != otheritem->GetMaxItemQuality())
    {
        if ( GetCurrentStats()->GetFlags() & PSITEMSTATS_FLAG_AVERAGEQUALITY )
            return true;
        else
            return false;
    }

    return false;
}

bool psItem::CompareOpenableLocks(const psItem* key) const
{
    size_t locksCount = openableLocks.GetSize();
    for (size_t i = 0 ; i < locksCount ; i++)
        if (!key->CanOpenLock(openableLocks[i], false))
            return false;
    return true;
}

psItem *psItem::Copy(unsigned short newstackcount)
{
    psItem *newitem;

    // Cannot split into a stack of 0 or a stack of the same amount or more than there already are
    if (newstackcount < 1)
        return NULL;

    // Cannot copy unique items
    if (GetIsUnique())
        return NULL;

    // Allocate a new item
    newitem = CreateNew();
    Copy(newitem);

    newitem->SetStackCount(newstackcount);

    newitem->SetLoaded();  // Item is fully created

    return newitem;
}

void psItem::Copy(psItem * target)
{
    Notify3(LOG_USER,"Copying item '%s' clone it.  Owner is %s.\n", GetName(),
           owning_character ? owning_character->GetCharName() : "None" );

    // The location in world is the same
    target->SetLocationInWorld(location.worldInstance,location.loc_sectorinfo,location.loc_x,location.loc_y,location.loc_z,location.loc_yrot);

    // Base stats are the same
    target->SetBaseStats(GetBaseStats());

    // The decay is the same.
    target->SetDecayResistance(decay_resistance);

    // The qualities are the same.
    target->SetItemQuality(item_quality);
    target->SetMaxItemQuality(crafted_quality);

    // The charges is the same
    target->SetCharges(GetCharges());

    // The flags are the same
    target->flags = flags;

    // The crafter is the same.
    target->crafter_id = crafter_id;

    // The guild_id is the same.
    target->guild_id = guild_id;

    // The load status is the same
    target->loaded = loaded;

    // Make sure a cloned key can open same things
    target->openableLocks = openableLocks;

    //unique name & description are the same
    target->item_name = item_name;
    target->item_description = item_description;

    target->SetGuardingCharacterID(GetGuardingCharacterID());

    // Current stats are rebuilt;
    int i;
    for (i=0;i<PSITEM_MAX_MODIFIERS;i++)
    {
        if (modifiers[i]!=NULL)
            target->AddModifier(modifiers[i]);
    }
    target->SetOwningCharacter( owning_character);

}

psItem *psItem::SplitStack(unsigned short newstackcount)
{
    // Cannot split into a stack of 0 or a stack of the same amount or more than there already are
    if (newstackcount<1 || newstackcount>=GetStackCount())
        return NULL;

    psItem *newitem = Copy(newstackcount);
    if (newitem == NULL)
        return NULL;

    // Adjust stack count of source
    SetStackCount(stack_count-newstackcount);

    Notify2(LOG_USER,"Split Stack to make a new stack of %d items.\n", newstackcount);

    return newitem;
}

void psItem::CombineStack(psItem *& stackme)
{
    // Make sure you call CheckStackableWith first!
    CS_ASSERT(CheckStackableWith(stackme,false));

    // If stacked item is from a spawn, we want to keep its spawning rules
    if (stackme->schedule)
    {
        if (!schedule)  // Absorb schedule
            SetScheduledItem( stackme->schedule );

        stackme->schedule = NULL; // Prevent deleting of shedule later in delete in RemoveInstance
    }

    // Average the qualities and set stack count
    unsigned short newStackCount = stack_count + stackme->GetStackCount();
    float newQuality = ((GetItemQuality()*GetStackCount())+(stackme->GetItemQuality()*stackme->GetStackCount()))/newStackCount;
    SetItemQuality(newQuality);
    float newMaxQuality = ((GetMaxItemQuality()*GetStackCount())+(stackme->GetMaxItemQuality()*stackme->GetStackCount()))/newStackCount;
    SetMaxItemQuality(newMaxQuality);
    SetStackCount(newStackCount);

    if (owning_character)
        owning_character->Inventory().UpdateEncumbrance();

    // if this item has no guarding character, and the other has one, keep it
    if(GetGuardingCharacterID() == 0)
        SetGuardingCharacterID(stackme->GetGuardingCharacterID());

    // Average charges
    int newCharges = (GetCharges()*GetStackCount() + stackme->GetCharges()*stackme->GetStackCount())/newStackCount;
    SetCharges(newCharges);

    CacheManager::GetSingleton().RemoveInstance(stackme);

    // Point to the final stack
    stackme = this;
}

// Functions that call into the appropriate psItemStats object

int psItem::GetAttackAnimID(psCharacter *pschar)
{
    PSSKILL skill = current_stats->Weapon().Skill(PSITEMSTATS_WEAPONSKILL_INDEX_0);
    int curr_level = pschar->Skills().GetSkillRank(skill).Current();

    return current_stats->GetAttackAnimID(curr_level);
}


bool psItem::GetIsMeleeWeapon()
{
    return current_stats->GetIsMeleeWeapon();
}

bool psItem::GetIsRangeWeapon()
{
    return current_stats->GetIsRangeWeapon();
}

bool psItem::GetIsAmmo()
{
    return current_stats->GetIsAmmo();
}

bool psItem::GetIsArmor()
{
    return current_stats->GetIsArmor();
}

bool psItem::GetIsShield()
{
    return current_stats->GetIsShield();
}

bool psItem::GetIsContainer()
{
    return current_stats->GetIsContainer();
}

bool psItem::GetIsConstructible()
{
    return current_stats->GetIsConstructible();
}

bool psItem::GetIsTrap()
{
    return current_stats->GetIsTrap();
}

bool psItem::GetCanTransform()
{
    return current_stats->GetCanTransform();
}

bool psItem::GetUsesAmmo()
{
    return current_stats->GetUsesAmmo();
}

bool psItem::GetIsStackable() const
{
    if(flags & PSITEM_FLAG_UNSTACKABLE)
    {
        return false;
    }
    if(flags & PSITEM_FLAG_STACKABLE)
    {
        return true;
    }
    return current_stats->GetIsStackable();
}

bool psItem::GetIsEquipStackable() const
{
    return current_stats->GetIsEquipStackable();
}

PSITEMSTATS_CREATIVETYPE psItem::GetCreative()
{
    return current_stats->GetCreative();
}

bool psItem::GetBuyPersonalise()
{
    return current_stats->GetBuyPersonalise();
}

const char *psItem::GetName() const
{
    if (!item_name.IsEmpty())
        return item_name;
    return current_stats->GetName();
}

const char *psItem::GetDescription() const
{
    if (!item_description.IsEmpty())
        return item_description;
    return current_stats->GetDescription();
}

void psItem::SetName(const char* newName)
{
    item_name = newName;
}

void psItem::SetDescription(const char* newDescription)
{
    item_description = newDescription;
}

const char *psItem::GetStandardName()
{
    return current_stats->GetName();
}

const char *psItem::GetStandardDescription()
{
    return current_stats->GetDescription();
}

PSITEMSTATS_WEAPONTYPE psItem::GetWeaponType()
{
    return current_stats->Weapon().Type();
}

PSSKILL psItem::GetWeaponSkill(PSITEMSTATS_WEAPONSKILL_INDEX index)
{
    return current_stats->Weapon().Skill(index);
}

float psItem::GetLatency()
{
    return current_stats->Weapon().Latency();
}


float psItem::GetDamage(PSITEMSTATS_DAMAGETYPE dmgtype)
{
    return current_stats->Weapon().Damage(dmgtype);
}

PSITEMSTATS_AMMOTYPE psItem::GetAmmoType()
{
   return current_stats->Ammunition().AmmoType();
}

float psItem::GetPenetration()
{
    return current_stats->Weapon().Penetration();
}

float psItem::GetUntargetedBlockValue()
{
    return current_stats->Weapon().UntargetedBlockValue();
}

float psItem::GetTargetedBlockValue()
{
    return current_stats->Weapon().TargetedBlockValue();
}

float psItem::GetCounterBlockValue()
{
    return current_stats->Weapon().CounterBlockValue();
}

PSITEMSTATS_ARMORTYPE psItem::GetArmorType()
{
    return current_stats->Armor().Type();
}

float psItem::GetDamageProtection(PSITEMSTATS_DAMAGETYPE dmgtype)
{
    // Add the characters natural armour bonus onto his normal armour.
    psItemStats* natArm;
    natArm = CacheManager::GetSingleton().GetBasicItemStatsByID(owning_character->raceinfo->natural_armor_id);
    if (!natArm || current_stats->Armor().Protection(dmgtype) > natArm->Armor().Protection(dmgtype))
    {
        useNat = false;
        return current_stats->Armor().Protection(dmgtype);
    }
    useNat = true;
    return natArm->Armor().Protection(dmgtype);
}

float psItem::GetHardness()
{
    return current_stats->Armor().Hardness();
}

PSITEMSTATS_STAT psItem::GetWeaponAttributeBonusType(int index)
{
    return current_stats->Weapon().AttributeBonusType(index);
}

float psItem::GetWeaponAttributeBonus(PSITEMSTATS_STAT stat)
{
    for (int i = 0; i < PSITEMSTATS_STAT_BONUS_COUNT; i++)
    {
        if (GetWeaponAttributeBonusType(i) == stat)
            return GetWeaponAttributeBonusMax(i);
    }
    return 0.0f;
}

float psItem::GetWeaponAttributeBonusMax(int index)
{
    return current_stats->Weapon().AttributeBonusMax(index);
}

float psItem::GetWeight()
{
    return (current_stats->GetWeight() * GetStackCount());
}

unsigned short psItem::GetItemSize()
{
    return current_stats->GetSize();
}

unsigned short psItem::GetContainerMaxSize()
{
    return current_stats->GetContainerMaxSize();
}

int psItem::GetContainerMaxSlots()
{
    return current_stats->GetContainerMaxSlots();
}

PSITEMSTATS_SLOTLIST psItem::GetValidSlots()
{
    return current_stats->GetValidSlots();
}

bool psItem::FitsInSlots(PSITEMSTATS_SLOTLIST slotmask)
{
    return current_stats->FitsInSlots(slotmask);
}

bool psItem::FitsInSlot(INVENTORY_SLOT_NUMBER slot)
{
    return (GetValidSlots() & (1<<(slot+1)))!=0;
}

float psItem::GetDecayResistance()
{
    return decay_resistance;
}

psMoney psItem::GetPrice()
{
    static MathScript *script;
    if (!script)
        script = psserver->GetMathScriptEngine()->FindScript("Calc Item Price");
    if (!script)
    {
        Error1("Cannot find mathscript: Calc Item Price");
        return current_stats->GetPrice();
    }

    MathEnvironment env;
    env.Define("Price", current_stats->GetPrice().GetTotal());
    env.Define("Quality", GetItemQuality());
    env.Define("MaxQuality", GetMaxItemQuality());
    script->Evaluate(&env);

    MathVar *finalPrice = env.Lookup("FinalPrice");
    if (!finalPrice)
    {
        Error1("Failed to evaluate MathScript >Calc Item Price<.");
        return current_stats->GetPrice();
    }
    return psMoney(int(finalPrice->GetValue()));
}

psMoney psItem::GetSellPrice()
{
    static MathScript *script;
    if (!script)
        script = psserver->GetMathScriptEngine()->FindScript("Calc Item Sell Price");
    if (!script)
    {
        Error1("Cannot find mathscript: Calc Item Sell Price");
        int sellPrice = (int)(current_stats->GetPrice().GetTotal() * 0.8);
        if (sellPrice == 0)
        {
            sellPrice = 1;
        }
        return sellPrice;
    }

    MathEnvironment env;
    env.Define("Price", current_stats->GetPrice().GetTotal());
    env.Define("Quality", GetItemQuality());
    env.Define("MaxQuality", GetMaxItemQuality());
    script->Evaluate(&env);

    MathVar *finalPrice = env.Lookup("FinalPrice");
    if (!finalPrice)
    {
        Error1("Failed to evaluate MathScript >Calc Item Price<.");
        return current_stats->GetPrice();
    }
    return psMoney(int(finalPrice->GetValue()));
}

psItemCategory * psItem::GetCategory()
{
    return current_stats->GetCategory();
}

float psItem::GetVisibleDistance()
{
    return current_stats->GetVisibleDistance();
}



// Removed until future implementation to avoid confusion
/*
unsigned int psItem::GetMeshIndex()
{
    return current_stats->GetMeshIndex();
}

unsigned int psItem::GetTextureIndex()
{
    return current_stats->GetTextureIndex();
}

unsigned int psItem::GetTexturePartIndex()
{
    return current_stats->GetTexturePartIndex();
}

*/

const char *psItem::GetMeshName()
{
    return current_stats->GetMeshName();
}

const char *psItem::GetTextureName()
{
    return current_stats->GetTextureName();
}

const char *psItem::GetPartName()
{
    return current_stats->GetPartName();
}

const char *psItem::GetPartMeshName()
{
    return current_stats->GetPartMeshName();
}

const char *psItem::GetImageName()
{
    return current_stats->GetImageName();
}

const char *psItem::GetSound()
{
    return current_stats->GetSound();
}

float psItem::GetArmorVSWeaponResistance(psItemStats* armor)
{
    return CacheManager::GetSingleton().GetArmorVSWeaponResistance(armor, current_stats);
}

bool psItem::HasCharges() const
{
    return base_stats->HasCharges();
}

bool psItem::IsRechargeable() const
{
    return base_stats->IsRechargeable();
}

void psItem::SetCharges(int charges)
{
    this->charges = charges;
}

int psItem::GetCharges() const
{
    return charges;
}

int psItem::GetMaxCharges() const
{
    return base_stats->GetMaxCharges();
}

double psItem::GetProperty(const char *ptr)
{
    if (!strcasecmp(ptr,"Skill1"))
    {
        return GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)0);
    }
    else if (!strcasecmp(ptr,"Skill2"))
    {
        return GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)1);
    }
    else if (!strcasecmp(ptr,"Skill3"))
    {
        return GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)2);
    }
    else if (!strcasecmp(ptr,"Quality"))
    {
        return GetItemQuality();
    }
    else if (!strcasecmp(ptr,"ArmQuality"))
    {
        // For natural armour quality
        if(useNat)
            return CacheManager::GetSingleton().GetBasicItemStatsByID(owning_character->raceinfo->natural_armor_id)->GetQuality();
        return GetItemQuality();
    }
    else if (!strcasecmp(ptr,"MaxQuality"))
    {
        return GetMaxItemQuality();
    }
    else if (!strcasecmp(ptr,"WeaponCBV"))
    {
        return GetCounterBlockValue();
    }
    else if (!strcasecmp(ptr,"Hardness"))
    {
        return GetHardness();
    }
    else if (!strcasecmp(ptr,"Penetration"))
    {
        return GetPenetration();
    }
    else if (!strcasecmp(ptr,"DamageSlash"))
    {
        return GetDamage(PSITEMSTATS_DAMAGETYPE_SLASH);
    }
    else if (!strcasecmp(ptr,"ProtectSlash"))
    {
        return GetDamageProtection(PSITEMSTATS_DAMAGETYPE_SLASH);
    }
    else if (!strcasecmp(ptr,"ExtraDamagePctSlash"))
    {
        return 0; // in the future, this should be read from weapon/armor XML
    }
    else if (!strcasecmp(ptr,"DamageBlunt"))
    {
        return GetDamage(PSITEMSTATS_DAMAGETYPE_BLUNT);
    }
    else if (!strcasecmp(ptr,"ProtectBlunt"))
    {
        return GetDamageProtection(PSITEMSTATS_DAMAGETYPE_BLUNT);
    }
    else if (!strcasecmp(ptr,"ExtraDamagePctBlunt"))
    {
        return 0; // in the future, this should be read from weapon/armor XML
    }
    else if (!strcasecmp(ptr,"DamagePierce"))
    {
        return GetDamage(PSITEMSTATS_DAMAGETYPE_PIERCE);
    }
    else if (!strcasecmp(ptr,"ProtectPierce"))
    {
        return GetDamageProtection(PSITEMSTATS_DAMAGETYPE_PIERCE);
    }
    else if (!strcasecmp(ptr,"ExtraDamagePctPierce"))
    {
        return 0; // in the future, this should be read from weapon/armor XML
    }
    else if (!strcasecmp(ptr,"StrMalus"))
    {
        return GetWeaponAttributeBonus(PSITEMSTATS_STAT_STRENGTH);
    }
    else if (!strcasecmp(ptr,"AgiMalus"))
    {
        return GetWeaponAttributeBonus(PSITEMSTATS_STAT_AGILITY);
    }
    else if (!strcasecmp(ptr,"Weight"))
    {
        return GetWeight();
    }
    else if (!strcasecmp(ptr,"MentalFactor"))
    {
        int temp = GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)0);
        return ( (double)CacheManager::GetSingleton().GetSkillByID((temp<0)?0:temp)->mental_factor / 100.0 );
    }
    else if (!strcasecmp(ptr,"RequiredRepairSkill"))
    {
        return base_stats->GetCategory()->repairSkillId;
    }
    else if (!strcasecmp(ptr,"SalePrice"))
    {
        return base_stats->GetPrice().GetTotal();
    }
    else if (!strcasecmp(ptr,"Charges"))
    {
        return (double)GetCharges();
    }
    else if (!strcasecmp(ptr,"MaxCharges"))
    {
        return (double)GetMaxCharges();
    }
    else if (!strcasecmp(ptr,"Range"))
    {
        return (double)GetRange();
    }
    else
    {
        CPrintf(CON_ERROR, "psItem::GetProperty(%s) failed\n",ptr);
        return 0;
    }
}

double psItem::CalcFunction(const char * functionName, const double * params)
{
    if (!strcasecmp(functionName, "GetArmorVSWeaponResistance"))
    {
        psItem *weapon = this;
        psItem *armor  = (psItem *)(intptr_t)params[0];

        // if no armor return 1
        if (!armor)
            return 1.0F;

        return weapon->GetArmorVSWeaponResistance(armor->GetCurrentStats());
    }

    CPrintf(CON_ERROR, "psItem::CalcFunction(%s) failed\n", functionName);
    return 0;
}

void psItem::SetIsLocked(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_LOCKED;
    else
        flags=flags & ~PSITEM_FLAG_LOCKED;
}

void psItem::SetIsLockable(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_LOCKABLE;
    else
        flags=flags & ~PSITEM_FLAG_LOCKABLE;
}

void psItem::SetIsSecurityLocked(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_SECURITYLOCK;
    else
        flags=flags & ~PSITEM_FLAG_SECURITYLOCK;
}

void psItem::SetIsUnpickable(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_UNPICKABLE;
    else
        flags=flags & ~PSITEM_FLAG_UNPICKABLE;
}

void psItem::SetIsNpcOwned(bool v)
{
    if (v)
        flags |= PSITEM_FLAG_NPCOWNED;
    else
        flags &= ~PSITEM_FLAG_NPCOWNED;
}

void psItem::SetIsCD(bool v)
{
    if (v)
        flags |= PSITEM_FLAG_USE_CD;
    else
        flags &= ~PSITEM_FLAG_USE_CD;
}

/// this only change the stackability for this instance, not all items (itemStats)
void psItem::SetIsItemStackable(bool v)
{
    if (v)
    {
        flags |= PSITEM_FLAG_STACKABLE;
        flags &= ~PSITEM_FLAG_UNSTACKABLE;
    }
    else
    {
        flags |= PSITEM_FLAG_UNSTACKABLE;
        flags &= ~PSITEM_FLAG_STACKABLE;
    }
}

///remove the STACKABLE and UNSTACKABLE flag, the items stackability is depending on his type
void psItem::ResetItemStackable()
{
    flags &= ~PSITEM_FLAG_STACKABLE;
    flags &= ~PSITEM_FLAG_UNSTACKABLE;
}

void psItem::SetIsKey(bool v)
{
    if (v)
        flags = flags | PSITEM_FLAG_KEY;
    else
        flags = flags & ~PSITEM_FLAG_KEY;
}

void psItem::SetIsMasterKey(bool v)
{
    if (v)
        flags = flags | PSITEM_FLAG_MASTERKEY;
    else
        flags = flags & ~PSITEM_FLAG_MASTERKEY;
}

void psItem::SetLockpickSkill(PSSKILL v)
{
    lockpickSkill = v;
}

void psItem::SetLockStrength(unsigned int v)
{
    lockStrength = v;
}

bool psItem::CanOpenLock(uint32 id, bool includeSkel) const
{
    if (includeSkel)
        return openableLocks.Find(id) != csArrayItemNotFound || openableLocks.Find(KEY_SKELETON) != csArrayItemNotFound;
    else
        return openableLocks.Find(id) != csArrayItemNotFound;
}

void psItem::AddOpenableLock(uint32 v)
{
    if (openableLocks.Find(v) == csArrayItemNotFound)
        openableLocks.Push(v);
}

void psItem::CopyOpenableLock(psItem* origKey)
{
    openableLocks = origKey->openableLocks;
}

void psItem::MakeSkeleton(bool b)
{
    if (b)
        AddOpenableLock(KEY_SKELETON);
    else
        RemoveOpenableLock(KEY_SKELETON);
}

bool psItem::GetIsSkeleton()
{
    return openableLocks.Find(KEY_SKELETON) != csArrayItemNotFound;
}

void psItem::SetIsSettingItem(bool v)
{
    if (v)
        flags = flags | PSITEM_FLAG_SETTINGITEM;
    else
        flags = flags & ~PSITEM_FLAG_SETTINGITEM;
}

void psItem::RemoveOpenableLock(uint32 v)
{
    size_t n = openableLocks.Find(v);
    if (n != csArrayItemNotFound)
        openableLocks.DeleteIndexFast(n);
}

void psItem::ClearOpenableLocks()
{
    openableLocks.DeleteAll();
}

csString psItem::GetOpenableLockNames()
{
    csString openableLocksString;
    csArray<unsigned int>::Iterator iter = openableLocks.GetIterator();
    while(iter.HasNext())
    {
        uint32 idNum = iter.Next();
        if (!openableLocksString.IsEmpty()) openableLocksString.Append(", ");
        if (idNum == KEY_SKELETON)
            openableLocksString.Append("All locks");
        else
        {
            // find lock gem
            gemItem* lockItem = GEMSupervisor::GetSingleton().FindItemEntity( idNum );
            if (!lockItem)
            {
                Error2("Can not find genItem for lock instance ID %u.\n", idNum);
                return openableLocksString;
            }

            // get real item
            psItem* item = lockItem->GetItem();
            if ( !item )
            {
                Error2("Invalid ItemID from gemItem for instance ID %u.\n", idNum);
                return openableLocksString;
            }
            openableLocksString.Append(item->GetName());
        }
    }
    return openableLocksString;
}

void  psItem::SetLocInParent(INVENTORY_SLOT_NUMBER location)
{
    loc_in_parent = (INVENTORY_SLOT_NUMBER)(location % 100); // only last 2 digits are actual slot location
}

void psItem::SetIsPickupable(bool v)
{
    if (!v)
        flags=flags | PSITEM_FLAG_NOPICKUP;
    else
        flags=flags & ~PSITEM_FLAG_NOPICKUP;
}

void psItem::SetIsTransient(bool v)
{
    if (v)
        flags=flags | PSITEM_FLAG_TRANSIENT;
    else
        flags=flags & ~PSITEM_FLAG_TRANSIENT;
}

bool psItem::CheckRequirements( psCharacter* charData, csString& resp )
{
    return base_stats->CheckRequirements( charData, resp );
}

void psItem::ScheduleRespawn()
{
    if(!schedule)
        return;

    // Transfer the spawn rules to the new item
    if(!schedule->WantToDie()) // removed?
    {
        psItemSpawnEvent* event = new psItemSpawnEvent(schedule);
        psserver->GetEventManager()->Push(event);
    }
    else
    {
        // Want to die, delete it
        delete schedule;
    }

    // Remove this shedule for this item, since we don't want an item in for example
    // the inventory to call respawn when it's dropped and picked up again
    schedule = NULL;
}

psScheduledItem::psScheduledItem(int id,uint32 itemID,csVector3& position, psSectorInfo* sector,InstanceID instance, int interval,int maxrnd,
            float range)
{
    spawnID = id;
    this->itemID = itemID;
    this->pos = position;
    this->sector = sector;
    this->interval = interval;
    this->maxrnd = maxrnd;
    this->worldInstance = instance;
    this->range = range;
    wantToDie= false;
}

psItem* psScheduledItem::CreateItem() // Spawns the item
{
    if(wantToDie)
        return NULL;

    Notify3(LOG_SPAWN,"Spawning item (%u) in instance %u.\n",itemID,worldInstance);

    psItemStats *stats = CacheManager::GetSingleton().GetBasicItemStatsByID(itemID);
    if (stats==NULL)
    {
        Error2("Could not find basic stats with ID %u for item spawn.\n",itemID);
    }
    else
    {
        psItem *item = stats->InstantiateBasicItem();
        if (item)
        {
            // Create the item
            float xpos = GetPosition().x - range + psserver->GetRandom() * range * 2; // Random position within range
            float zpos = GetPosition().z - range + psserver->GetRandom() * range * 2;
            item->SetLocationInWorld(worldInstance,GetSector(),xpos, GetPosition().y, zpos, 0);
            if ( !EntityManager::GetSingleton().CreateItem(item,false) )
            {
                delete item;
                return NULL;
            }

            // Transfer the spawning rules for it to pass it forward
            psScheduledItem* newsch = new psScheduledItem(*this);
            item->SetScheduledItem(newsch);

            lastSpawn = csGetTicks();

            item->SetLoaded();  // Item is fully created
            item->Save(false);    // First save
            return item;
        }
    }
    return NULL;
}

void psScheduledItem::UpdatePosition(csVector3& position, const char *sector)
{
    if(wantToDie)
        return;

    pos = position;
    db->Command("UPDATE hunt_locations SET x='%f', y='%f', z='%f', sector='%s' WHERE id='%d'",
                pos.x,pos.y,pos.z,sector,spawnID);
}

void psScheduledItem::ChangeIntervals(int newint, int newrand)
{
    if(wantToDie)
        return;

    interval = newint;
    maxrnd = newrand;
    db->Command("UPDATE hunt_locations SET interval='%d', max_random='%d' WHERE id='%d'",
                interval,maxrnd,spawnID);
}

void psScheduledItem::ChangeRange(float newRange)
{
    if(wantToDie)
        return;

    range = newRange;
    db->Command("UPDATE hunt_locations SET range='%f' WHERE id='%d'",
                range, spawnID);
}

void psScheduledItem::ChangeAmount(int newAmount)
{
    if(wantToDie)
        return;

    db->Command("UPDATE hunt_locations SET amount='%d' WHERE id='%d'",
                newAmount, spawnID);
}

void psScheduledItem::Remove()
{
    db->Command("DELETE FROM hunt_locations WHERE id='%d'",spawnID);
    wantToDie = true;
}

int psScheduledItem::MakeInterval()
{
    int rnd = (int)psserver->GetRandom(maxrnd);
    return interval + rnd;
}

bool psItem::Destroy()
{
    // TODO:  Delete unique item entry
    //if (GetIsUnique())
    //{
    //}

    // Check for already removed
    if ((this->GetUID() != 0) && !DeleteFromDatabase())
    {
        Error3("Failed to delete item ID %u.  Error '%s'.",this->GetUID(),db->GetLastError());
        return false;
    }

    return true;
}

bool psItem::DeleteFromDatabase()
{
    if ( db->CommandPump("DELETE FROM item_instances where id='%u'",this->uid)!=1)
        return false;

    uid = ID_DONT_SAVE_ITEM;  // prevent update attempts when key is -1 unsigned
    return true;
}

void psItem::ScheduleRemoval()
{
    this->SetFlags(this->GetFlags() | PSITEM_FLAG_TRANSIENT);

    int randomized_interval = psserver->rng->Get(REMOVAL_INTERVAL_RANGE);

    psItemRemovalEvent *event = new psItemRemovalEvent(REMOVAL_INTERVAL_MINIMUM + randomized_interval, this->uid );
    psserver->GetEventManager()->Push(event);

    Notify2(LOG_USER,"Scheduling removal of object for %d ticks from now.\n",
     REMOVAL_INTERVAL_MINIMUM + randomized_interval);
}

void psItem::DeleteObjectCallback(iDeleteNotificationObject * object)
{
    if (gItem)
    {
        gItem->UnregisterCallback(this);
        gItem = NULL;
    }
}

void psItem::UpdateView(Client *fromClient, EID eid, bool clear)
{
    if (!fromClient)
        return;

    gemActor *guardian = clear ? 0 : GEMSupervisor::GetSingleton().FindPlayerEntity(GetGuardingCharacterID());
    psViewItemUpdate mesg(fromClient->GetClientNum(),
                          eid,
                          GetLocInParent(),
                          clear,
                          GetName(),
                          GetImageName(),
                          GetStackCount(),
                          guardian ? guardian->GetEID() : 0);

    mesg.Multicast(fromClient->GetActor()->GetMulticastClients(),0,5);
}

void psItem::SetGemObject(gemItem *object)
{
    // Unregister previous callbacks if the current gItem is not NULL
    if (gItem)
        gItem->UnregisterCallback(this);

    // Set the new gItem and register callback
    gItem = object;
    if (gItem)
        gItem->RegisterCallback(this);
}

bool psItem::SetBookText(const csString& newText)
{
    return GetBaseStats()->SetCreation(PSITEMSTATS_CREATIVETYPE_LITERATURE,
                                       newText,
                                       owning_character ? owning_character->GetCharFullName():"Unknown");
}

bool psItem::SetSketch(const csString& newSketchData)
{
    return GetBaseStats()->SetCreation(PSITEMSTATS_CREATIVETYPE_SKETCH,
                                       newSketchData,
                                       owning_character ? owning_character->GetCharFullName():"Unknown");
}

void psItem::FillContainerMsg(Client* client, psViewItemDescription& outgoing)
{
    gemContainer *container = dynamic_cast<gemContainer*> (gItem);

    if (!container)
    {
        // This is not a container in the world so it must be a container inside the person's
        // inventory.  So check to see which items the player has that are in the container.
        int slot = 0;
        for (size_t i = 0; i < client->GetCharacterData()->Inventory().GetInventoryIndexCount(); i++)
        {
            psItem *child = client->GetCharacterData()->Inventory().GetInventoryIndexItem(i);
            if (parent_item_InstanceID == uid)
            {
                outgoing.AddContents(child->GetName(), child->GetImageName(),
                        child->GetPurifyStatus(), slot++,
                        child->GetStackCount());
            }
        }
        return;
    }

    gemContainer::psContainerIterator it(container);
    while (it.HasNext())
    {
        psItem* child = it.Next();
        if (!child)
        {
            Debug2(LOG_NET,client->GetClientNum(),"Container iterator has next but returns null psItem pointer for %u.\n",client->GetClientNum());
            return;
        }

        int stackCount = container->CanTake(client,child) ? child->GetStackCount() : -1;
        outgoing.AddContents(child->GetName(), child->GetImageName(),
                child->GetPurifyStatus(), child->GetLocInParent(), stackCount);
    }
}

bool psItem::SendItemDescription( Client *client)
{
    csString itemInfo, weight, size, itemQuality, itemCategory, itemName, itemCharges;

    if(GetIsLocked())
    {
        itemInfo = "This item is locked\n\n";
    }

    itemCategory.Format( "Category: %s", current_stats->GetCategory()->name.GetData() );
    float fweight = GetWeight();
    unsigned short ssize = GetItemSize();
    if(stack_count > 1)
    {
        weight.Format("\nWeight: %.2f (%hu x %.2f)", fweight, stack_count, fweight/stack_count);
        size.Format("\nSize: %hu (%hu x %hu)", ssize*stack_count, stack_count, ssize);
    }
    else
    {
        weight.Format("\nWeight: %.2f", fweight);
        size.Format("\nSize: %hu", ssize);
    }
    itemInfo += itemCategory+weight+size;

    // Check identify skill before sending quality detail
    itemQuality = "";
    int idSkill = GetIdentifySkill();
    int idMin = GetIdentifyMinSkill();
    if (!idSkill || idMin < client->GetCharacterData()->Skills().GetSkillRank((PSSKILL) idSkill).Current())
    {
        // If the item is an average stackable type object it has no max quality so don't
        // send that information to the client since it is not applicable.
        if ( current_stats->GetFlags() & PSITEMSTATS_FLAG_AVERAGEQUALITY )
        {
            itemQuality.Format("\nAverage Quality: %.0f", GetItemQuality() );
        }
        else
        {
            itemQuality.Format("\nQuality: %.0f/%.0f", GetItemQuality(),GetMaxItemQuality() );
        }
    }
    itemInfo += itemQuality;

    if (HasCharges())
    {
        itemCharges.Format("\nCharges: %d",GetCharges());
        itemInfo += itemCharges;
    }

    // Generate item name
    if ( crafter_id != 0 )
    {
        // Add craft adjective
        itemName.Format("%s %s",GetQualityString(),GetName());
    }
    else
    {
        itemName = GetName();
    }

    // Item was crafted
    if ( crafter_id != 0 )
    {
        // Crafter and guild names
        csString crafterInfo;
        psCharacter* charData = psServer::CharacterLoader.QuickLoadCharacterData( crafter_id, true );
        if ( charData )
        {
            crafterInfo.Format( "\n\nCrafter: %s", charData->GetCharFullName());
            itemInfo += crafterInfo;
        }

        // Item was crafted by a guild member
        if ( GetGuildID() != 0 )
        {
            csString guildInfo;
            psGuildInfo* guild = CacheManager::GetSingleton().FindGuild( GetGuildID() );
            if ( guild && !guild->IsSecret())
            {
                guildInfo.Format( "\nGuild: %s", guild->GetName().GetData());
                itemInfo += guildInfo;
            }
        }
    }

    if (GetGuardingCharacterID().IsValid())
    {
        csString guardingChar;
        gemActor *guardian = GEMSupervisor::GetSingleton().FindPlayerEntity(GetGuardingCharacterID());
        if (guardian && guardian->GetCharacterData())
        {
            guardingChar.Format("\nGuarded by: %s", guardian->GetCharacterData()->GetCharFullName());
            itemInfo += guardingChar;
        }
    }

    // Item is a key
    if ( GetIsKey() )
    {
        csString lockInfo;
        if (GetIsMasterKey())
        {
            lockInfo.Format( "\nThis is a master key!");
            itemInfo += lockInfo;
        }
        lockInfo.Format( "\nOpens: %s", GetOpenableLockNames().GetData());
        itemInfo += lockInfo;
    }

    // Item is a weapon
    if ( GetIsMeleeWeapon() || GetIsRangeWeapon() )
    {
        csString speed, damage;
        // Weapon Speed
        speed.Format( "\n\nAttack delay: %.2f", current_stats->Weapon().Latency() );

        // Weapon Damage Type
        damage = "\n\nDamage:";
        float dmgSlash, dmgBlunt, dmgPierce;
        dmgSlash = GetDamage(PSITEMSTATS_DAMAGETYPE_SLASH);
        dmgBlunt = GetDamage(PSITEMSTATS_DAMAGETYPE_BLUNT);
        dmgPierce = GetDamage(PSITEMSTATS_DAMAGETYPE_PIERCE);

        // Only worth printing if their value is not zero
        if ( dmgSlash )
            damage += csString().Format( "\n Slash: %.2f", dmgSlash );
        if ( dmgBlunt )
            damage += csString().Format( "\n Blunt: %.2f", dmgBlunt );
        if ( dmgPierce )
            damage += csString().Format( "\n Pierce: %.2f", dmgPierce );

        itemInfo+= speed + damage;
    }

    // Item is armor
    if ( GetIsArmor() )
    {
        csString armor_type = "\n\n";
        switch (GetBaseStats()->Armor().Type())
        {
            case PSITEMSTATS_ARMORTYPE_LIGHT:
                armor_type.Append("Light Armor");
                break;
            case PSITEMSTATS_ARMORTYPE_MEDIUM:
                armor_type.Append("Medium Armor");
                break;
            case PSITEMSTATS_ARMORTYPE_HEAVY:
                armor_type.Append("Heavy Armor");
                break;
        default:
            break;
        }
        itemInfo += armor_type;
    }

    // Item is a container
    if(GetIsContainer())
    {
        itemInfo += csString().Format("\nCapacity: %d", GetContainerMaxSize());
    }

    if (strcmp(GetDescription(),"0") != 0)
    {
        itemInfo += "\n\nDescription: ";
        itemInfo += GetDescription();
    }

    psViewItemDescription outgoing( client->GetClientNum(), itemName.GetData(), itemInfo.GetData(), GetImageName(), stack_count );

    if ( outgoing.valid )
        psserver->GetEventManager()->SendMessage(outgoing.msg);
    else
    {
        Bug2("Could not create valid psViewItemDescription for client %u.\n",client->GetClientNum());
        return false;
    }

    return true;
}

bool psItem::SendContainerContents(Client *client, int containerID)
{
    if (GetIsLocked() && client->GetSecurityLevel() < GM_LEVEL_2)
        return SendItemDescription(client);

    csString desc( GetDescription() );

    // FIXME: This function is called for world containers too...
    desc.AppendFmt("\n\nWeight: %.2f\nCapacity: %u/%u",
                    client->GetCharacterData()->Inventory().GetContainedWeight(this),
                    client->GetCharacterData()->Inventory().GetContainedSize(this),
                    GetContainerMaxSize() );

    psViewItemDescription outgoing( client->GetClientNum(),
                                    GetName(),
                                    desc,
                                    GetImageName(),
                                    0,
                                    IS_CONTAINER );

    if (gItem != NULL )
        outgoing.containerID = gItem->GetEID().Unbox();
    else
        outgoing.containerID = containerID;

    outgoing.ContainerSlots = GetContainerMaxSlots();

    FillContainerMsg( client, outgoing);

    outgoing.ConstructMsg();
    outgoing.SendMessage();

    return true;
}

void psItem::SendCraftTransInfo(Client *client)
{
    csString mess;
    psItemStats* mindStats = GetCurrentStats();
    if ( !mindStats )
    {
        Error1("No stats for mind item.");
        return;
    }

    // Get character pointer
    if ( client == NULL )
    {
        Error1("Bad client pointer.");
        return;
    }

    psCharacter* character = client->GetCharacterData();
    if ( character == NULL )
    {
        Error1("Bad client psCharacter pointer.");
        return;
    }

    // Get craft combo info string based on skill levels
    csString comboString;
    GetComboInfoString(character, mindStats->GetUID(), comboString);
    mess.Append(comboString);
    mess.Append("\n");

    // Get transformation info string based on skill levels
    csString transString;
    GetTransInfoString(character,mindStats->GetUID(), transString);
    mess.Append(transString);

    // Send info to client
    psMsgCraftingInfo msg(client->GetClientNum(),mess);
    if (msg.valid)
        psserver->GetEventManager()->SendMessage(msg.msg);
    else
        Bug2("Could not create valid psMsgCraftingInfo for client %u.\n",client->GetClientNum());
}

void psItem::GetComboInfoString(psCharacter* character, uint32 designID, csString & comboString)
{
    csArray<CraftComboInfo*>* combInfoArray = CacheManager::GetSingleton().GetTradeComboInfoByItemID(designID);
    if (!combInfoArray)
        return;

    csArray<CraftComboInfo*>::Iterator combInfoIter(combInfoArray->GetIterator());
    while(combInfoIter.HasNext())
    {
        CraftComboInfo* combInfo = combInfoIter.Next();
        // If any skill check succeed then display this combinations string.
        // that means we have a way to do something with the result of the combine
        csArray<CraftSkills*>* skillArray = combInfo->skillArray;
        for ( int count = 0; count<(int)skillArray->GetSize(); count++ )
        {
            // Check if craft step minimum primary skill level is meet by client
            int priSkill = skillArray->Get(count)->priSkillId;
            if(priSkill >= 0 && skillArray->Get(count)->minPriSkill > character->Skills().GetSkillRank((PSSKILL) priSkill).Current())
            {
                continue;
            }

            // Check if craft step minimum secondary skill level is meet by client
            int secSkill = skillArray->Get(count)->secSkillId;
            if(secSkill >= 0 && skillArray->Get(count)->minSecSkill > (int)character->Skills().GetSkillRank((PSSKILL) secSkill).Current())
            {
                    continue;
            }
            // Otherwise send combination string, and go on to next combine
            comboString.Append(combInfo->craftCombDescription);
            break;
        }
    }
}

void psItem::GetTransInfoString(psCharacter* character, uint32 designID, csString & transString)
{
    csArray<CraftTransInfo*>* craftArray = CacheManager::GetSingleton().GetTradeTransInfoByItemID(designID);
    if (!craftArray)
        return;

    for ( int count = 0; count<(int)craftArray->GetSize(); count++ )
    {
        // Check if craft step minimum primary skill level is meet by client
        int priSkill = craftArray->Get(count)->priSkillId;
        if(priSkill >= 0 && craftArray->Get(count)->minPriSkill > character->Skills().GetSkillRank((PSSKILL) priSkill).Current())
        {
                continue;
        }

        // Check if craft step minimum seconday skill level is meet by client
        int secSkill = craftArray->Get(count)->secSkillId;
        if(secSkill >= 0 && craftArray->Get(count)->minSecSkill > (int) character->Skills().GetSkillRank((PSSKILL) secSkill).Current())
        {
                continue;
        }

        // Otherwise tack on trasnformation step description to message
        transString.Append(craftArray->Get(count)->craftStepDescription);
    }
}

bool psItem::SendBookText(Client *client, int containerID, int slotID)
{
    csString name = GetStandardName();
    csString text;
    csString lockedText("This item is locked\n");
    if(GetIsLocked())
    {
        text = lockedText;
    } else text = GetBookText();
    //determine whether to display the 'write' button
    //and send the appropriate information if so

    //is it a writable book?  In our inventory? Are we the author?
    bool shouldWrite = (GetBaseStats()->GetIsWriteable() &&
        GetOwningCharacter() == client->GetCharacterData() &&
        GetBaseStats()->IsThisTheCreator(client->GetCharacterData()->GetPID()));

  //  CPrintf(CON_DEBUG,"Sent text for book %u %u\n",slotID, containerID);
    psReadBookTextMessage outgoing(client->GetClientNum(), name, text, shouldWrite, slotID, containerID);

    if (outgoing.valid)
    {
        psserver->GetEventManager()->SendMessage(outgoing.msg);
    }
    else
    {
        Bug2("Could not create valid psReadBookText for client %u.\n",client->GetClientNum());
        return false;
    }

    return true;
}

void psItem::SendSketchDefinition(Client *client)
{
    // Get character capabilities
    static MathScript *script;
    if (!script)
        script = psserver->GetMathScriptEngine()->FindScript("Calc Player Sketch Limits");
    if (!script)
    {
        Error1("Cannot find mathscript: Calc Player Sketch Limits");
        return;
    }

    MathEnvironment env;
    env.Define("Actor", client->GetCharacterData());
    script->Evaluate(&env);

    MathVar *score = env.Lookup("IconScore");
    MathVar *count = env.Lookup("PrimCount");
    CS_ASSERT(score && count);
    int playerScore = (int)score->GetValue();
    int primCount   = (int)count->GetValue();

    // Build the limit string for the client
    csString xml("<limits>");
    xml.AppendFmt("<count>%d</count>",primCount);  // This limits how many things you can add on the client.

    // writeable sketch? in inventory? author?
    bool sketchReadOnly = !(GetBaseStats()->GetIsWriteable() &&
          GetOwningCharacter() == client->GetCharacterData() &&
          GetBaseStats()->IsThisTheCreator(client->GetCharacterData()->GetPID()));
    if (sketchReadOnly)
        xml.Append("<rdonly/>");

    size_t i=0;
    while (CacheManager::GetSingleton().GetLimitation(i))
    {
        const psCharacterLimitation *charlimit = CacheManager::GetSingleton().GetLimitation(i);

        // This limits which icons each player can use to only the ones below his level.
        if (playerScore > charlimit->min_score)
            xml.AppendFmt("<ic>%s</ic>",charlimit->value.GetDataSafe() );
        i++;
    }
    xml += "</limits>";

    // Now send all this
    psSketchMessage msg( client->GetClientNum(), GetUID(), 0, xml, GetBaseStats()->GetSketch(), GetBaseStats()->IsThisTheCreator(client->GetCharacterData()->GetPID()), GetStandardName() );
    msg.SendMessage();
}

void psItem::ViewItem(Client* client, int containerID,
        INVENTORY_SLOT_NUMBER slotID)
{
    psCharacter *owningCharacter = GetOwningCharacter();

    if (current_stats->GetIsContainer())
    {
        SendContainerContents(client, slotID);
    }

    // for now, just examining mind item gives craft information rather then its normal description
    //  eventually /study will give same results after some delay
    // check if we are examining an item that is meant for mind slot
    else if (FitsInSlots(PSITEMSTATS_SLOT_MIND))
    {
        //We pass through container, slot & parent IDs so that we can pass them back and forth when writing books.
        SendCraftTransInfo(client);
    }

    //for now, we pretend that /examine reads.  When we implement /read, this will change to only send the description of the book
    //e.g. "This book is bound in a leathery Pterosaur hide and branded with the insignia of the Sunshine Squadron"
    //NOTE that this logic is sensitive to ordering; Books currently have a nonzero "sketch" length since that's where they
    //store their content.
    else if (GetBaseStats()->GetIsReadable() && (client->GetCharacterData()
            == owningCharacter || !owningCharacter))
    {
        //We pass through container, slot & parent IDs so that we can pass them back and forth when writing books.
        SendBookText(client, containerID, slotID);
    }
    else if (GetBaseStats()->GetCreative() == PSITEMSTATS_CREATIVETYPE_SKETCH
            && GetBaseStats()->GetSketch().Length() > 0
            && (client->GetCharacterData() == owningCharacter
                    || !owningCharacter)) // Sketch
    {
        SendSketchDefinition(client);
    }
    else // Not a container, show description
    {
        SendItemDescription(client);
    }
}

bool psItem::SendActionContents(Client *client, psActionLocation *action)
{
    if ( action == NULL )
        return false;

    //if(GetIsLocked())
    //    return SendItemDescription(client);

    csString name( action->name );
    csString desc( GetDescription() );
    csString icon( GetImageName() );

    csString description = action->GetDescription();
    if ( description )
    {
        desc = description.GetData();
    }

    bool isContainer = GetIsContainer();

    psViewItemDescription outgoing( client->GetClientNum(),
                                    name,
                                    desc,
                                    icon,
                                    0,
                                    isContainer );

    /* REMOVED: was probably there to avoid a crash, remove after some testing.
    if (action->gItem != NULL )
        outgoing.containerID = action->gItem->GetEntity()->GetID();
    else
        outgoing.containerID = containerID;
    */

    outgoing.containerID = gItem->GetEID().Unbox();

    outgoing.ContainerSlots = GetContainerMaxSlots();

    if ( isContainer )
    {
        FillContainerMsg( client, outgoing );
        outgoing.ConstructMsg();
    }

    outgoing.SendMessage();
    return true;
}
