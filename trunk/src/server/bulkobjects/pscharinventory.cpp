/*
* pscharinventory.cpp  by Keith Fulton <keith@paqrat.com>
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/xmltiny.h>
#include <ctype.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "util/mathscript.h"

#include "../psserver.h"
#include "../psserverchar.h"
#include "../progressionmanager.h"
#include "../npcmanager.h"
#include "../gem.h"
#include "../client.h"
#include "../cachemanager.h"
#include "../globals.h"
#include "../exchangemanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacterloader.h"
#include "pscharacter.h"
#include "psglyph.h"
#include "psguildinfo.h"
#include "adminmanager.h"
#include "psmerchantinfo.h"

psCharacterInventory::psCharacterInventory(psCharacter *ownr)
{
    int i;
    for (i=0; i<PSCHARACTER_SLOT_BULK1; i++)
    {
        equipment[i].default_if_empty  = NULL;
        equipment[i].itemIndexEquipped = 0;
        equipment[i].eventId           = 0;
        equipment[i].EquipmentFlags    = 0x00000000;
    }

//    totalWeight = 0.0f;
//    totalSize = 0.0f;
//    totalCount = 0;

    maxWeight = 0.0f;
    maxSize = 0.0f;
    
    // Load fists
    psItemStats *fistStats = CacheManager::GetSingleton().GetBasicItemStatsByName("Fist");
    psItem *fist = fistStats->InstantiateBasicItem();
    equipment[PSCHARACTER_SLOT_LEFTHAND].default_if_empty = fist;
    equipment[PSCHARACTER_SLOT_LEFTHAND].EquipmentFlags |= PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK | PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY;
    equipment[PSCHARACTER_SLOT_RIGHTHAND].default_if_empty = fist;
    equipment[PSCHARACTER_SLOT_RIGHTHAND].EquipmentFlags |= PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK | PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY;

    psCharacterInventoryItem newItem(fist); // default item in inv index 0 every time, so we don't have to check for NULL everywhere
    inventory.Push(newItem);

    owner = ownr;

    // Load basecloths. Default item equipped in clothing/armour slots.
    psItemStats *basecloth = CacheManager::GetSingleton().GetBasicItemStatsByName("basecloths");
    psItem *basecloths = basecloth->InstantiateBasicItem();
    basecloths->SetOwningCharacter(owner);

    equipment[PSCHARACTER_SLOT_ARMS].default_if_empty = basecloths;
    equipment[PSCHARACTER_SLOT_BOOTS].default_if_empty = basecloths;
    equipment[PSCHARACTER_SLOT_GLOVES].default_if_empty = basecloths;
    equipment[PSCHARACTER_SLOT_HELM].default_if_empty = basecloths;
    equipment[PSCHARACTER_SLOT_TORSO].default_if_empty = basecloths;
    equipment[PSCHARACTER_SLOT_LEGS].default_if_empty = basecloths;

    doRestrictions = false;
    loaded         = false;
    inExchangeMode = false;
	version = 1; // Client cache version initializes to 0. so, we start with 1 here
}

psCharacterInventory::~psCharacterInventory()
{
    for (size_t t = 0 ; t < inventory.GetSize() ; t++)
    {
        psItem* curritem = inventory.Get(t).GetItem();
        if(curritem)
        {
            delete curritem;
        }
    }
    inventory.DeleteAll();
}

void psCharacterInventory::CalculateLimits()
{
    MathEnvironment env; // safe enough to reuse it for both...faster...
    env.Define("Actor", owner);

    // The max total weight that a player can carry
    static MathScript *maxCarryWeight = NULL;
    if (!maxCarryWeight)
        maxCarryWeight = psserver->GetMathScriptEngine()->FindScript("CalculateMaxCarryWeight");
    if (maxCarryWeight)
    {
        maxCarryWeight->Evaluate(&env);
        MathVar *carry = env.Lookup("MaxCarry");
        if (carry)
        {
            maxWeight = carry->GetValue();
        }
        else
        {
            Error1("Failed to evaluate MathScript >CalculateMaxCarryWeight<.");
        }
    }        

    // The max total size that a player can carry
    static MathScript *maxCarryAmount = NULL;
    if (!maxCarryAmount)
        maxCarryAmount = psserver->GetMathScriptEngine()->FindScript("CalculateMaxCarryAmount");
    if (maxCarryAmount)
    {
        maxCarryAmount->Evaluate(&env);
        MathVar *carry = env.Lookup("MaxAmount");
        if (carry)
        {
            maxSize = carry->GetValue();
        }
        else
        {
            Error1("Failed to evaluate MathScript >CalculateMaxCarryAmount<.");
        }
    }                       
    
    UpdateEncumbrance();
}

void psCharacterInventory::UpdateEncumbrance()
{
    gemActor *actor = owner->GetActor();
    if (!actor)
        return;

    /* TODO: http://www.hydlaa.com/bugtracker/bug.php?op=show&bugid=2618
     *       Currently characters can equip items due to stat buffs, but
     *       later fail to meet those requirements.  We used to unequip
     *       such items; dropping if theere was no space.
     */

    if (doRestrictions && GetCurrentTotalWeight() > MaxWeight())
    {
        actor->SetMode(PSCHARACTER_MODE_OVERWEIGHT);
    }
    else if (actor->GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
    {
        actor->SetMode(PSCHARACTER_MODE_PEACE);
    }
}

bool psCharacterInventory::Load()
{
    return Load(owner->GetPID());
}

psItem *psCharacterInventory::GetItemFactory(psItemStats *stats)
{
    psItem *item;

    if (stats->GetIsGlyph())
    {
        item = new psGlyph();
    }
    else
    {
        item = new psItem();
    }
    return item;
}


bool psCharacterInventory::Load(PID use_id)
{    
    doRestrictions = (owner->GetCharType() == PSCHARACTER_TYPE_PLAYER);

    // Players get restrictions but GMs do not
    if (owner->GetActor() && owner->GetActor()->GetClient() && owner->GetActor()->GetClient()->GetSecurityLevel() > GM_DEVELOPER)
    {
        doRestrictions = false;
    }
        
    Result items(db->Select("SELECT * FROM item_instances WHERE char_id_owner=%u AND location_in_parent!=-1", use_id.Unbox()));
    if ( items.IsValid() )
    {
        // psItemSet* set = new psItemSet;
        for (size_t x = 0; x < items.Count(); x++ )
        {
            unsigned long i = (unsigned long)x;

            // Determine type of item
            unsigned int stats_id = items[i].GetUInt32("item_stats_id_standard");
            psItemStats *stats = CacheManager::GetSingleton().GetBasicItemStatsByID(stats_id);
            if ( !stats )
            {
                Bug3("Error! Item %s could not be created because item_stats id=%d was not found.\n",items[i]["id"], stats_id);
                continue;
            }
            // printf("Inventory loading %d: %s\n", items[i].GetInt("id"),stats->GetName() );

            // Create the instance
            psItem* item = GetItemFactory(stats);  // create detailed type and return superclass generic

            if ( !item->Load(items[i]) )
            {
                Bug2("Error! Item %s could not be loaded. Skipping.\n", items[i]["id"]);
                delete item;
                continue;
            }

            // Now add this instance to our inventory
            if ( !AddLoadedItem(items[i].GetUInt32("parent_item_id"),(INVENTORY_SLOT_NUMBER)items[i].GetInt("location_in_parent"), item) )
            {
                Bug5("Item %s(%u) could not be loaded for %s(%s). Skipping this item.\n",
                     item->GetName(), item->GetUID(), owner->GetCharName(), ShowID(owner->GetPID()));
                delete item;
                continue;
            }
            item->SetOwningCharacter(owner);
            item->SetLoaded();  // nothing will save until this is set
        }

        loaded = true; // Begin dimension checking

        return true;
    }
    else
    {
        Error3("Db Error loading character inventory.\nQuery: %s\nError: %s\n",db->GetLastQuery(), db->GetLastError() );
        return false;    
    }
}    


bool psCharacterInventory::QuickLoad(PID use_id)
{    
    Result items(db->Select("SELECT id, item_stats_id_standard, location_in_parent FROM item_instances WHERE char_id_owner = %u AND location_in_parent != -1", use_id.Unbox()));

    if ( items.IsValid() )
    {
        for ( int i=0; i < (int)items.Count(); i++ )
        {
            unsigned int stats_id = items[i].GetUInt32("item_stats_id_standard");
            psItemStats *stats = CacheManager::GetSingleton().GetBasicItemStatsByID(stats_id);
            
            if ( stats )
            {
                // Quick load; we just need to know what it looks like
                psItem* item = stats->InstantiateBasicItem();
            
                item->UpdateInventoryStatus(owner, 0, (INVENTORY_SLOT_NUMBER) items[i].GetInt("location_in_parent"));

                if (!AddLoadedItem(0, (INVENTORY_SLOT_NUMBER) items[i].GetInt("location_in_parent"), item))
                {
                    Bug5("Item %s(%s) could not be quick loaded for %s(%s). Skipping this item.\n",
                         item->GetName(), items[i]["id"], owner->GetCharName(), ShowID(owner->GetPID()));
                    delete item;
                }
            }
            else
            {
              Bug2("Error! Item %s could not be loaded. Skipping.\n", items[i]["id"] );
            }
        }

        // Items are not flagged as 'loaded', and can not be saved
        return true;
    }
    else
    {
        return false;
    }
}    


bool psCharacterInventory::AddLoadedItem(uint32 parentID, INVENTORY_SLOT_NUMBER slot, psItem *item)
{
    if (slot < PSCHARACTER_SLOT_NONE || slot >= PSCHARACTER_SLOT_BULK_END)
    {
        csString error;
        error.Format("Item %s(%u) Could not be placed in %s(%s) inventory into slot %d because its slot was illegal.", 
                     item->GetName(), 
                     item->GetUID(), 
                     owner->GetCharName(), 
                     ShowID(owner->GetPID()),
                     slot); 
        Bug2("%s", error.GetData());
        return false;
    }

    // Get item into inventory
    psCharacterInventoryItem newItem(item);
    size_t i = inventory.Push(newItem);
    if (slot < PSCHARACTER_SLOT_BULK1)
        equipment[slot].itemIndexEquipped = i;

    return true;
}

size_t psCharacterInventory::GetItemIndex(psItem *item)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i<inventory.GetSize(); i++)
        if (inventory[i].item == item)
            return i;

    return SIZET_NOT_FOUND;
}

void psCharacterInventory::Equip(psItem *item)
{
    if (!item->IsEquipped() || !owner->GetActor() || !owner->GetActor()->GetClient())
        return;

    gemActor *actor = owner->GetActor();
    Client *fromClient = actor->GetClient();

    // We track slots directly to make combat more efficient
    size_t itemIndex = GetItemIndex(item);
    CS_ASSERT(itemIndex != SIZET_NOT_FOUND);
    equipment[item->GetLocInParent()].itemIndexEquipped = itemIndex;
    equipment[item->GetLocInParent()].eventId = 0;

    psserver->GetCharManager()->SendOutPlaySoundMessage(fromClient->GetClientNum(), item->GetSound(), "equip");
    psserver->GetCharManager()->SendOutEquipmentMessages(actor, item->GetLocInParent(false), item, psEquipmentMessage::EQUIP);

    if (!item->IsActive())
    {
        item->RunEquipScript(actor);
    }
    owner->CalculateEquipmentModifiers();
}

void psCharacterInventory::Unequip(psItem *item)
{
    if (!item->IsEquipped() || !owner->GetActor() || !owner->GetActor()->GetClient())
    {       
        return;
    }        

    gemActor *actor = owner->GetActor();
    Client *fromClient = actor->GetClient();

    // 1. Update equipment array...
    equipment[item->GetLocInParent()].itemIndexEquipped = 0;

    // 2. Send out unequip messages
    psserver->GetCharManager()->SendOutPlaySoundMessage(fromClient->GetClientNum(), item->GetSound(), "unequip");
    psserver->GetCharManager()->SendOutEquipmentMessages(actor, item->GetLocInParent(), item, psEquipmentMessage::DEEQUIP);

    // 3. Run unequip script
    if (item->IsActive())
    {
        item->CancelEquipScript();
    }
    owner->CalculateEquipmentModifiers();
}

bool psCharacterInventory::CheckSlotRequirements(psItem *item, INVENTORY_SLOT_NUMBER proposedSlot, unsigned short stackCount)
{
    CS_ASSERT(proposedSlot != PSCHARACTER_SLOT_NONE);
    CS_ASSERT(item != NULL); // attempting either is probably bad

    if(proposedSlot < PSCHARACTER_SLOT_NONE)
        return false;
    
    if (stackCount == 0)
        stackCount = item->GetStackCount();

    // Check container-related conditions...
    if (proposedSlot >= 100)
    {
        // no containers in containers
        if (item->GetIsContainer())
            return false;

        // make sure the destination slot actually exists!
        int parentSlotID = proposedSlot/100;
        psItem *parentItem = GetItem(NULL,(INVENTORY_SLOT_NUMBER) parentSlotID);
        if (!parentItem || !parentItem->GetIsContainer())
        {
            Error2("Client requested transfer to destination slot %d but the parent container doesn't exist.", proposedSlot);
            return false;
        }

        // If moving inside the same container, allow it
        if (item->GetContainerID() == parentItem->GetUID())
            return true;

	 // check if the item fit in the container
        if(doRestrictions && 
		   ((GetContainedSize(parentItem) + (item->GetItemSize())*stackCount) > parentItem->GetContainerMaxSize()))
		{
            return false;
		}

    }

    // Check conditions for equipment slots...
    if (proposedSlot >= 0 && proposedSlot < PSCHARACTER_SLOT_BULK1)
    {
        // It needs to fit in that slot
        if (!item->FitsInSlot(proposedSlot))
            return false;
        // It better not be a stack (unless explicitly allowed)
        if (stackCount > 1 && !item->GetIsEquipStackable())
            return false;
        // The owning character needs to have the stat requirements
        csString response;
        if (!item->CheckRequirements(owner, response))
        {
            if (owner->GetActor())
                psserver->SendSystemError(owner->GetActor()->GetClientID(), response);
            return false;
        }
    }

    return true;
}

bool psCharacterInventory::EquipItem(psItem *item, INVENTORY_SLOT_NUMBER slot)
{
    if (slot < 0 || slot >= PSCHARACTER_SLOT_BULK1)
        return false;

    if (!CheckSlotRequirements(item, slot))
        return false;

    item->UpdateInventoryStatus(owner, 0, slot);

    return true;
}

size_t psCharacterInventory::FindSlotIndex(psItem *container,INVENTORY_SLOT_NUMBER slot)
{
    uint32 contID = container ? container->GetUID() : 0;

    if (container)
    {
        for (size_t i=1; i<inventory.GetSize(); i++)
        {
            if (inventory[i].item->GetContainerID() == contID &&
                inventory[i].item->GetLocInParent(true) == slot)
                return i;
        }
    }
    else
    {
        for (size_t i=1; i<inventory.GetSize(); i++)
        {
            if (inventory[i].item->GetLocInParent(true) == slot)
                return i;
        }
    }
    return SIZET_NOT_FOUND;
}

size_t psCharacterInventory::FindCompatibleStackedItem(psItem *item)
{
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->CheckStackableWith(item, true))
        {
            return i;
        }
    }
    return SIZET_NOT_FOUND;
}

csArray<size_t> psCharacterInventory::FindCompatibleStackedItems(psItem *item)
{
    csArray<size_t> compatibleItems;
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->CheckStackableWith(item, true))
        {
            compatibleItems.Push(i);
        }
    }
    return compatibleItems;
}

INVENTORY_SLOT_NUMBER psCharacterInventory::FindFreeEquipSlot(psItem* itemToPlace)
{
    csArray<INVENTORY_SLOT_NUMBER> fitsIn;

    fitsIn = itemToPlace->GetBaseStats()->GetSlots();

    for (size_t i = 0; i < fitsIn.GetSize(); i++)
    {
        INVENTORY_SLOT_NUMBER proposedSlot = fitsIn[i];
        psItem* existingItem = GetInventoryItem(proposedSlot);
        // If there is an item already here then this is not allowed for equipment.
        if (!existingItem && itemToPlace->FitsInSlots(CacheManager::GetSingleton().slotMap[proposedSlot]))
            return proposedSlot;
    }
    return PSCHARACTER_SLOT_NONE;
}

int psCharacterInventory::FindFirstOpenSlot(psItem *container)
{
    INVENTORY_SLOT_NUMBER max = PSCHARACTER_SLOT_BULK1;
    if (!container)
        max = PSCHARACTER_SLOT_BULK_END;

    for (int i=PSCHARACTER_SLOT_BULK1; i<PSCHARACTER_SLOT_BULK1 + max; i++)
    {
        int containerBase = (container) ? container->GetLocInParent(true)*100 : 0 ;
        size_t index = FindSlotIndex(container,(INVENTORY_SLOT_NUMBER) (containerBase+i) );
        if (index == SIZET_NOT_FOUND) // slot not taken
            return i;
    }
    return PSCHARACTER_SLOT_NONE;
}

bool psCharacterInventory::AddOrDrop(psItem *&item, bool stack)
{
    if (Add(item, false, stack))
    {
        if (owner->GetActor() && owner->GetActor()->GetClientID()) // Actor and no superclient
        {
            psserver->GetCharManager()->UpdateItemViews(owner->GetActor()->GetClientID());
        }
        return true;
    }
    else
    {
        owner->DropItem(item);

        if (owner->GetActor())
        {
            psserver->SendSystemError(owner->GetActor()->GetClientID(),
                        "Item %s dropped because your inventory is full.",
                        item->GetName());
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,0,"You found %s, but dropped it: %s", item->GetName(), lastError.GetDataSafe());
        }
        return false;
    }
}

bool psCharacterInventory::Add(psItem *&item, bool test, bool stack, INVENTORY_SLOT_NUMBER slot, gemContainer* container)
{
    if (slot%100<ANY_EMPTY_BULK_SLOT || slot%100>=PSCHARACTER_SLOT_BULK_END)
    {
        Error2("Inv slot %d is out of bounds! Add failed.", slot );
        return false;
    }

    if (item->GetBaseStats()->IsMoney())
    {
        owner->SetMoney(item);
        return true;
    }

    // Don't stack containers when picking them up...
    if (container)
        stack = false;

    /** If we were passed ANY_BULK_SLOT or ANY_EMPTY_BULK_SLOT we need to scan the possible
     * slots and try to find a valid place to put this item.  (non-negative slot number)
     * Otherwise, we'll just skip over these and attempt to place in the given slot.  If
     * that fails, we'll return false and possibly call this again specifying ANY_BULK_SLOT.
     */
    size_t i;
    size_t itemIndex = SIZET_NOT_FOUND;
    csArray<size_t> itemIndices;


    // Next see if we can stack this with an existing stack in inventory
    if (stack && slot == ANY_BULK_SLOT && item->GetIsStackable())
    {

        itemIndices = FindCompatibleStackedItems(item);
        for (i=0; i<itemIndices.GetSize(); i++)
        {
            int size=0;
            if(inventory[itemIndices[i]].item->GetContainerID()){
                size = GetContainedSize(FindItemID(inventory[itemIndices[i]].item->GetContainerID()));
            }
            if (!inventory[itemIndices[i]].item->GetContainerID() || size + item->GetTotalStackSize() <= FindItemID(inventory[itemIndices[i]].item->GetContainerID())->GetContainerMaxSize()) // adequate space in container
            {
                if (test)
                    return true; // not really doing it here            

                inventory[itemIndices[i]].item->CombineStack(item);

                // Here we update the passed in parameter so it points at the merged stack instead of the item that is gone now
                item = inventory[itemIndices[i]].item;

                // Save new combined stack to db
                inventory[itemIndices[i]].item->Save(true);

                UpdateEncumbrance();

                if (owner->IsNPC() || owner->IsPet())
                    psserver->GetNPCManager()->QueueInventoryPerception(owner->GetActor(), item, true);
                return true;
            }
        }
    }

    itemIndex = SIZET_NOT_FOUND; // We need to reset this in case a stack was found, but the container it was in was full

    // Next check the main bulk slots
    if (itemIndex == SIZET_NOT_FOUND && slot < 0)
    {
        for (i=PSCHARACTER_SLOT_BULK1; i<PSCHARACTER_SLOT_BULK_END; i++)
        {
            itemIndex = FindSlotIndex(NULL,(INVENTORY_SLOT_NUMBER) i );
            if (itemIndex == SIZET_NOT_FOUND)
            {
                if (test)
                    return true; // not really doing it here            

                slot = (INVENTORY_SLOT_NUMBER)i;
                item->UpdateInventoryStatus(owner, 0, slot);
                item->Save(false);
                // Get item into inventory
                psCharacterInventoryItem newItem(item);
                inventory.Push(newItem);

                if ( container )
                {
                    int parent = item->GetUID();

                    gemContainer::psContainerIterator iter(container);
                    while (iter.HasNext())
                    {
                        psItem* child = iter.Next();
                        size_t slot = child->GetLocInParent() + PSCHARACTER_SLOT_BULK1;
                        //iter.RemoveCurrent(); 
                        psCharacterInventoryItem newItem(child);
                        inventory.Push(newItem);
                        child->UpdateInventoryStatus(owner, parent, (INVENTORY_SLOT_NUMBER) slot);
                        child->Save(false);
                    }
                }

                UpdateEncumbrance();

                if (owner->IsNPC() || owner->IsPet())
                    psserver->GetNPCManager()->QueueInventoryPerception(owner->GetActor(), item, true);

                return true;
            }
        }
        itemIndex = SIZET_NOT_FOUND;
    }

    // Now try looking for a container
    if (itemIndex == SIZET_NOT_FOUND && slot < 0 && !item->GetIsContainer())
    {
        for (i=1; i<inventory.GetSize(); i++)
        {
            if (inventory[i].item->GetIsContainer())
            {
                int size = GetContainedSize(inventory[i].item);
                if (size + item->GetTotalStackSize() <= inventory[i].item->GetContainerMaxSize()) // adequate space in container
                {
                    INVENTORY_SLOT_NUMBER containerSlot = (INVENTORY_SLOT_NUMBER)FindFirstOpenSlot(inventory[i].item);
                    if (containerSlot == PSCHARACTER_SLOT_NONE)
                        return false;

                    if (test)
                        return true; // not really doing it here            

                    item->UpdateInventoryStatus(owner, inventory[i].item->GetUID(),containerSlot);
                    item->Save(false);
                    // Get item into inventory
                    psCharacterInventoryItem newItem(item);
                    inventory.Push(newItem);

                    UpdateEncumbrance();

                    if (owner->IsNPC() || owner->IsPet())
                        psserver->GetNPCManager()->QueueInventoryPerception(owner->GetActor(), item, true);
                    return true;
                }
            }
        }
    }

    // Next if it is supposed to be in a particular slot, just stick in there if possible
    if (slot%100 > PSCHARACTER_SLOT_NONE && slot%100 < PSCHARACTER_SLOT_BULK_END) // slot specified
    {
        if (!CheckSlotRequirements(item, slot))
            return false; // -1 means add failed

        uint32 parentID = 0;
        int containerSlot = slot/100;
        if (containerSlot)
        {
            psItem *containerItem = GetItem(NULL,(INVENTORY_SLOT_NUMBER)containerSlot); 
            if (!containerItem->GetIsContainer())
                return false;
            else
                parentID = containerItem->GetUID();
        }

        if (test)
            return true; // not really doing it here            

        psCharacterInventoryItem newItem(item);
        inventory.Push(newItem);

        item->UpdateInventoryStatus(owner, parentID, slot);
        item->Save(false);

        UpdateEncumbrance();

        if (owner->IsNPC() || owner->IsPet())
            psserver->GetNPCManager()->QueueInventoryPerception(owner->GetActor(), item, true);
        return true;
    }
    return false;
}

psCharacterInventory::psEquipInfo& psCharacterInventory::GetEquipmentObject(INVENTORY_SLOT_NUMBER slot)
{
    // We have to return a valid equipInfo in all cases, so make a dummy one here for that
    // ItemIndex 0 is always a dummy item for this purpose
    static psEquipInfo dummy_equipment = {0, 0, 0, 0};

    if (slot<PSCHARACTER_SLOT_NONE || slot>=PSCHARACTER_SLOT_BULK1)
        return dummy_equipment;
    return equipment[slot];
}

psItem *psCharacterInventory::GetInventoryItem(INVENTORY_SLOT_NUMBER slot)
{
    
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK_END)
        return NULL;

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i < inventory.GetSize(); i++)
    {
//        if ( inventory[i].item )
//            printf("Slot %d Inventory[%d]: %s %d\n", slot, i, inventory[i].item->GetName(), inventory[i].item->GetLocInParent(true));
            
        if (inventory[i].item && inventory[i].item->GetLocInParent(true) == slot)
            return inventory[i].item;
    }
    return NULL;
}

psItem *psCharacterInventory::GetItemHeld()
{
    psItem * item = GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if (!item)
    {
        item = GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        
    }

    return item;
}


psCharacterInventory::psCharacterInventoryItem *psCharacterInventory::GetCharInventoryItem(INVENTORY_SLOT_NUMBER slot)
{
    if (slot < 0 || slot >= 10000)
        return NULL;

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i < inventory.GetSize(); i++)
    {
        if (inventory[i].item && inventory[i].item->GetLocInParent(true) == slot)
            return &inventory[i];
    }
    return NULL;
}

INVENTORY_SLOT_NUMBER psCharacterInventory::FindSlotHoldingItem(psItem *item)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i < inventory.GetSize(); i++)
    {
        if (inventory[i].item && inventory[i].item == item)
            return inventory[i].item->GetLocInParent();
    }
    return PSCHARACTER_SLOT_NONE;
}



psItem *psCharacterInventory::GetItem(psItem *container,INVENTORY_SLOT_NUMBER slot)
{
    size_t index = FindSlotIndex(container,slot);

    if (index != SIZET_NOT_FOUND)
        return inventory[index].item;
    else
        return NULL;
}

bool psCharacterInventory::hasItemName(csString & itemname, bool includeBulk)
{
    for (size_t i=1; i < inventory.GetSize(); i++)
    {
        if (inventory[i].item && (csString)inventory[i].item->GetName() == itemname &&
           (includeBulk || inventory[i].item->GetLocInParent(true) < PSCHARACTER_SLOT_BULK1))
            return true;
    }
    return false;
}

bool psCharacterInventory::hasItemCategory(csString & categoryname, bool includeBulk)
{
    for (size_t i=1; i < inventory.GetSize(); i++)
    {
        if (inventory[i].item && inventory[i].item->GetCategory()->name == categoryname &&
           (includeBulk || inventory[i].item->GetLocInParent(true) < PSCHARACTER_SLOT_BULK1))
            return true;
    }
    return false;
}

psItem *psCharacterInventory::RemoveItemIndex(size_t itemIndex, int count)
{
    psItem* currentItem = inventory[itemIndex].item;
    if (currentItem==NULL)
        return NULL;

    // default -1 is to drop all in stack.
    if (count == -1) 
        count = currentItem->GetStackCount();

    if (count<0  ||  count > currentItem->GetStackCount())
        return NULL;

//    inventoryCacheServer.SetSlotModified(bulkslot);   // update cache

    //we have to update clients if we are dropping an item which is being exchanged. We send a removal
    //also if we are only dropping a partial stack as something is happening in that slot and to avoid
    //bugs and confusion on the user it's better removing the item entirely from the exchange window
    //if he/she wants to add it again it's up to him/her to do it after
    if(inExchangeMode && inventory[itemIndex].exchangeOfferSlot  != -1)
    {
        //maybe a bit over careful
        Client * charClient = owner->GetActor() ?  owner->GetActor()->GetClient() : NULL;
        if(charClient) 
        {
            Exchange *exchange = psserver->exchangemanager->GetExchange(charClient->GetExchangeID());
            if(exchange) exchange->RemoveItem(charClient, inventory[itemIndex].exchangeOfferSlot, inventory[itemIndex].exchangeStackCount);
        }
    }

    // Remove ALL items in stack
    if ( count == currentItem->GetStackCount() )
    {
        inventory.DeleteIndex(itemIndex);  // Take out of inventory master list
		UpdateEncumbrance();
        currentItem->UpdateInventoryStatus(owner, 0, PSCHARACTER_SLOT_NONE);

        // Update equipment array to compensate for index shifting
        for (size_t slot = 0; slot < INVENTORY_EQUIP_COUNT; slot++)
        {
            if (equipment[slot].itemIndexEquipped > itemIndex)
                equipment[slot].itemIndexEquipped--;
        }

        return currentItem;
    }
    else
    {
        psItem* newItem = currentItem->SplitStack((unsigned short)count);
        currentItem->Save(false);
        if (newItem)
        {
            UpdateEncumbrance();
            newItem->UpdateInventoryStatus(owner, 0, PSCHARACTER_SLOT_NONE);
        }

        return newItem;
    }
}

psItem *psCharacterInventory::RemoveItemID(uint32 itemID, int count)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t itemIndex=1; itemIndex < inventory.GetSize(); itemIndex++)
    {
        if (inventory[itemIndex].item->GetUID() == itemID)
            return RemoveItemIndex(itemIndex,count);
    }
    return NULL;
}

psItem *psCharacterInventory::RemoveItem(psItem *container, INVENTORY_SLOT_NUMBER slot, int count )
{
    if (slot%100<0 || slot%100>=PSCHARACTER_SLOT_BULK_END)
        return NULL;

    size_t itemIndex = FindSlotIndex(container,slot);
    if (itemIndex == SIZET_NOT_FOUND)
        return NULL;

    return RemoveItemIndex(itemIndex,count);
}

void psCharacterInventory::RunEquipScripts()
{
    for (int equipslot=0;equipslot<INVENTORY_EQUIP_COUNT;equipslot++)
    {
        if (equipment[equipslot].itemIndexEquipped!=0) // 0 is Default fist item
        {
            inventory[equipment[equipslot].itemIndexEquipped].item->RunEquipScript(owner->GetActor());
        }
    }
}
 

unsigned int psCharacterInventory::TotalStackOfItem(psItemStats* item)
{
    unsigned int count = 0;

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->GetBaseStats() == item)
            count += (unsigned int)inventory[i].item->GetStackCount();
    }
    
    return count;
}


psItem *psCharacterInventory::FindItemID(uint32 itemID)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->GetUID() == itemID)
            return inventory[i].item;
    }
    return NULL;
}


bool psCharacterInventory::HaveKeyForLock(uint32 lock)
{
    // Only let clients with a security level of GM_TESTER or greater use skeleton keys
    bool isGM = (owner->GetActor() && owner->GetActor()->GetClient() &&
                 owner->GetActor()->GetClient()->GetSecurityLevel() >= GM_TESTER);

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->CanOpenLock(lock, isGM))
            return true;
    }
    return false;
}

bool psCharacterInventory::CanItemAttack(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return false;


    // The autoattack or singleattack flags must be set
    if ((equipment[slot].EquipmentFlags & PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK) ||
        (equipment[slot].EquipmentFlags & PSCHARACTER_EQUIPMENTFLAG_SINGLEATTACK))
    {
        // Check if the slot is empty and can attack when empty
        if (equipment[slot].itemIndexEquipped==0 && (equipment[slot].EquipmentFlags & PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY))
        {
            return true;
        }

        // Otherwise the slot must have an item in it
        if (equipment[slot].itemIndexEquipped==0)
            return false;

        // If the item is a melee weapon, it's OK
        if (inventory[equipment[slot].itemIndexEquipped].item->GetIsMeleeWeapon())
            return true;

        if (inventory[equipment[slot].itemIndexEquipped].item->GetIsRangeWeapon())
        {
            return true;
        }
    }

    return false;
}


bool psCharacterInventory::IsItemAutoAttack(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return false;

    return (equipment[slot].EquipmentFlags & PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK);
}


psItem *psCharacterInventory::GetEffectiveWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return NULL;

    // if there is a weapon in the slot, return it
    if (equipment[slot].itemIndexEquipped!=0)
    {
        psItem *item = inventory[equipment[slot].itemIndexEquipped].item;
        if (item->GetIsMeleeWeapon() || item->GetIsRangeWeapon())
        {
            return item;
        }
    }
    
    // right hand and left hand can attack also if no weapon is there
    // the default_if_empty is the fist weapon
    if (equipment[slot].EquipmentFlags & PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY)
        return equipment[slot].default_if_empty;

    return NULL;
}


psItem *psCharacterInventory::GetEffectiveArmorInSlot(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if (slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return NULL;

    // if there is an item in the slot, return it
    if (equipment[slot].itemIndexEquipped!=0)
        return inventory[equipment[slot].itemIndexEquipped].item;

    // hit armor locations should use the base armor if none is present
    // the default_if_empty is the basecloth armor
    return equipment[slot].default_if_empty;
}



bool psCharacterInventory::HasEnoughUnusedSpace( float requiredSpace )
{
    if (!doRestrictions )
        return true;

#if !ENABLE_MAX_CAPACITY
    return true;
#endif

    return (GetCurrentTotalSpace() + requiredSpace <= GetCurrentMaxSpace() );
}

int psCharacterInventory::GetCurrentTotalSpace()
{
    int total=0;
    for (size_t i=0; i<inventory.GetSize(); i++)
        total += inventory[i].item->GetItemSize(); // * stackCount here?  KWF

    return total;
}

int psCharacterInventory::GetCurrentMaxSpace()
{
    // Your max available space is your personal space + the sum of the space of your containers
    int total=0;

    total = (int)maxSize; // Calculated by mathscript

    for (size_t i=0; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->GetIsContainer())
            total += inventory[i].item->GetContainerMaxSize(); // * stackCount here?  KWF
    }
    return total;
}

bool psCharacterInventory::HasEnoughUnusedWeight( float requiredWeight ) 
{ 
   if ( !doRestrictions )
        return true;

   return (GetCurrentTotalWeight() + requiredWeight <= MaxWeight() );
}    

float psCharacterInventory::GetCurrentTotalWeight()
{
    float total = 0;
    for (size_t i=0; i<inventory.GetSize(); i++)
        total += inventory[i].item->GetWeight();

    return total;
}

size_t psCharacterInventory::GetContainedItemCount(psItem *container)
{
    size_t count = 0;
    for (size_t i = 0; i < inventory.GetSize(); i++)
    {
        // if this item is contained in the specified container
        if (inventory[i].item->GetContainerID() == container->GetUID())
            count++;
    }
    return count;
}

float psCharacterInventory::GetContainedWeight(psItem *container)
{
    float total=0;
    gemContainer *cont = dynamic_cast<gemContainer*> (container->GetGemObject());
    if(cont)
    {
       gemContainer::psContainerIterator iter(cont);    
        while (iter.HasNext())
        {
            psItem* child = iter.Next();
            total += child->GetWeight();
        }
    }
    else
    {
        for (size_t i=0; i<inventory.GetSize(); i++)
        {
            // if this item is contained in the specified container
            if (inventory[i].item->GetContainerID() == container->GetUID())
			{
                total += inventory[i].item->GetWeight();
			}
        }

    }
    return total;
}

uint32 psCharacterInventory::GetContainedSize(psItem *container)
{
    uint32 total=0;
    gemContainer *cont = dynamic_cast<gemContainer*> (container->GetGemObject());
    if(cont)
	{
        gemContainer::psContainerIterator iter(cont);
        while (iter.HasNext())
        {
            psItem* child = iter.Next();
            total += child->GetTotalStackSize();
        }
    }
    else
    {
        for (size_t i=0; i<inventory.GetSize(); i++)
        {
            // if this item is contained in the specified container
            if (inventory[i].item->GetContainerID() == container->GetUID())
            {
                total += inventory[i].item->GetTotalStackSize();
            }
        }


    }
    return total;
}

size_t psCharacterInventory::HowManyCanFit(psItem* item)
{
    if ( !doRestrictions )
        return 65535; // Fit any amount

    if ( !HasEnoughUnusedWeight(0) || !HasEnoughUnusedSpace(0) )
        return 0; // Can't fit any

    size_t w = 65535;
    size_t s = 65535;

    float individualWeight = item->GetBaseStats()->GetWeight();
    if ( individualWeight > 0.01 )
        w = (size_t)((maxWeight-GetCurrentTotalWeight() )/individualWeight);

#if ENABLE_MAX_CAPACITY
    unsigned short individualSize = item->GetBaseStats()->GetSize();
    if ( individualSize > 0 )
        s = (size_t)((maxSize-GetCurrentMaxSpace() )/individualSize);
#endif

    return (w<s)?w:s; // Capacity depends on the lesser
}


void psCharacterInventory::SetDoRestrictions(bool v)
{
    if (doRestrictions == v)
        return;

    doRestrictions = v;

    // Reassess dimentions and drop items to meet them, if needed
    if (doRestrictions)
        CalculateLimits();
    else
        UpdateEncumbrance(); // this will unlock if already encumbered
}


csArray<psItem*> psCharacterInventory::GetItemsInCategory(psItemCategory * category)
{
    csArray<psItem*> items;

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i = 1; i < inventory.GetSize(); i++)
    {
        if (inventory[i].item->GetCategory() == category)
        {
            items.Push(inventory[i].item);
        }
    }

    return items;
}

psItem* psCharacterInventory::StackNumberItems(const csString & itemname, int count, bool container)
{
    psItem* stackItem = NULL;

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    unsigned int i;
    for (i = 1; i<inventory.GetSize(); i++)
    {
        if (itemname.CompareNoCase(inventory[i].item->GetName()) && !(inventory[i].item->IsInUse()) &&
            (container || (inventory[i].item->GetLocInParent(true) < PSCHARACTER_SLOT_BULK_END) ))
        {
            stackItem = inventory[i].item;
            break;
        }
    }
    
    if(!stackItem)
        return NULL;

    for (unsigned int j = i+1;count > stackItem->GetStackCount() && j < inventory.GetSize(); j++)
    {
        if (itemname.CompareNoCase(inventory[i].item->GetName()) && !(inventory[j].item->IsInUse()) &&
            (container || (inventory[i].item->GetLocInParent(true) < PSCHARACTER_SLOT_BULK_END) ))
        {
            psItem* stackedItem = inventory[j].item;
            if (stackItem->CheckStackableWith(stackedItem, false))
            {
                psItem *stack = RemoveItemID(stackedItem->GetUID());

                stackItem->CombineStack(stack);
                stackItem->Save(false);
            }
        }
    }
    return stackItem;
}

bool psCharacterInventory::HasPurifiedGlyphs(csArray<psItemStats*> glyphsToCheck)
{
    for (size_t i = 1; i < inventory.GetSize(); i++)
    {
        for (size_t j = 0; j < glyphsToCheck.GetSize(); j++)
        {
            if (inventory[i].item->GetBaseStats() == glyphsToCheck[j] && inventory[i].item->GetPurifyStatus() == 2)
            {
                glyphsToCheck.DeleteIndexFast(j);
                break;
            }
        }

        if (glyphsToCheck.GetSize() == 0)
            return true;
    }
    return false;
}

void psCharacterInventory::CreateGlyphList(csArray<glyphSlotInfo>& slots)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i < inventory.GetSize(); i++)
    { 
        psGlyph * glyph = dynamic_cast <psGlyph*> (inventory[i].item);
        if (glyph != NULL)
        {
            // Check if this is a duplicate glyph...
            size_t duplicate = SIZET_NOT_FOUND;
            for (size_t j = 0; j < slots.GetSize() && duplicate == SIZET_NOT_FOUND; j++)
            {
                if (slots[j].glyphType == glyph->GetBaseStats())
                    duplicate = j;
            }
            if (duplicate != SIZET_NOT_FOUND)
            {
                // Yes, it's a duplicate...prefer purified glyphs, then purifying,
                // then unpurified ones.
                if (glyph->GetPurifyStatus() > slots[duplicate].purifyStatus)
                    slots[duplicate].purifyStatus = glyph->GetPurifyStatus();
            }
            else
            {
                // No, it's a new kind of glyph - add it.
                glyphSlotInfo gsi;
                gsi.glyphType = glyph->GetBaseStats();
                gsi.purifyStatus = glyph->GetPurifyStatus();

                slots.Push(gsi);
            }
        }
    }
}

void psCharacterInventory::WriteAllInventory(iDocumentNode *npcRoot)
{
    csRef<iDocumentNode> equipmentNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    equipmentNode->SetValue("equipment");

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=1; i<inventory.GetSize(); i++)
    {
        psItem *item = inventory[i].item;
        int slot = FindSlotHoldingItem(item);
        // 1 = Bulk, 0 = Equipped
        WriteItem(equipmentNode, item, (slot == -1) ? 1 : 0, (slot==-1)?item->GetLocInParent():(INVENTORY_SLOT_NUMBER)slot);
    }
}

void psCharacterInventory::WriteItem(csRef<iDocumentNode> equipmentNode, psItem* item, int bulk, INVENTORY_SLOT_NUMBER slot)
{
    csRef<iDocumentNode> objNode = equipmentNode->CreateNodeBefore(CS_NODE_ELEMENT);

    objNode->SetValue("obj");
    objNode->SetAttribute("name", item->GetName());

    if (item->GetStackCount() != 1)
        objNode->SetAttributeAsInt("stack_count", item->GetStackCount());

    if (item->GetItemQuality())
        objNode->SetAttributeAsFloat("item_quality", item->GetItemQuality());

    if (item->GetIsCrafterIDValid())
    {
        Result result (db->Select("SELECT name FROM characters WHERE id='%u'", item->GetCrafterID().Unbox()));
        if (result.IsValid())
            objNode->SetAttribute("crafter", result[0][0]);
    }

    if (item->GetIsGuildIDValid())
    {
        psGuildInfo* guild = CacheManager::GetSingleton().FindGuild(item->GetGuildID());
        if (guild)
            objNode->SetAttribute("guild", guild->GetName());
    }
    if (bulk==1)
        objNode->SetAttribute("location", "bulk");
    else
        objNode->SetAttribute("location", "equipped");

    objNode->SetAttributeAsInt("slot", slot);
}

size_t psCharacterInventory::FindItemStatIndex(psItemStats *itemstats, size_t startAt)
{
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for (size_t i=startAt; i<inventory.GetSize(); i++)
    {
        if (inventory[i].item->GetBaseStats() == itemstats)
            return i;
    }
    return SIZET_NOT_FOUND;
}

void psCharacterInventory::SetExchangeOfferSlot(psItem *Container,INVENTORY_SLOT_NUMBER slot,int toSlot,int stackCount)
{
    size_t index = FindSlotIndex(Container,slot);
    
    if (index != SIZET_NOT_FOUND)
    {
        inventory[index].exchangeOfferSlot  = toSlot;
        inventory[index].exchangeStackCount = stackCount;
        //printf("Set item %s to offer slot %d, count %d.\n", inventory[index].item->GetName(),toSlot,stackCount);
    }
}

int psCharacterInventory::GetOfferedStackCount(psItem *item)
{
    size_t index = GetItemIndex(item);
    
    return (index == SIZET_NOT_FOUND) ? 0 : inventory[index].exchangeStackCount;
}

psCharacterInventory::psCharacterInventoryItem *psCharacterInventory::FindExchangeSlotOffered(int slotID)
{
    for (size_t i=0; i < inventory.GetSize(); i++)
    {
        //printf("Checking item %lu, %s in offered slot %d.\n",i,inventory[i].item->GetName(),inventory[i].exchangeOfferSlot);
        if (inventory[i].exchangeOfferSlot == slotID)
            return &inventory[i];
    }
    return NULL;
}   

void psCharacterInventory::PurgeOffered()
{
    for (size_t i=0; i < inventory.GetSize(); i++)
    {
        if (inventory[i].exchangeOfferSlot != -1)
        {
            // Careful: If we remove the entire item, the indices shift back 1,
            // so index i will actually be the next item. Hence, we back up.
            // If we don't remove the entire item, we'll visit it again, but
            // setting exchangeOfferSlot to -1 ensures we won't try and remove
            // it twice.
			inventory[i].exchangeOfferSlot = -1; // done with this item.
			int removed = inventory[i].exchangeStackCount;
			inventory[i].exchangeStackCount = 0;
			psItem *pullout = RemoveItemIndex(i, removed);
            if (pullout)
            {
				pullout->Destroy();
                delete pullout;
                i--;
            }
        }
    }
}

bool psCharacterInventory::BeginExchange()
{
    if (inExchangeMode)
        return false;

    inExchangeMode = true;
   
    for (size_t i=0; i<inventory.GetSize(); i++)
    {
        inventory[i].exchangeOfferSlot = -1;
    }

    return true;
}

void psCharacterInventory::CommitExchange()
{
    // inventory array has most current data
    inExchangeMode = false;

    for (size_t i=0; i<inventory.GetSize(); i++)
    {
        inventory[i].exchangeOfferSlot = -1;
    }
}


void psCharacterInventory::RollbackExchange()
{
    if (inExchangeMode)
    {
        inExchangeMode = false;
    }
    for (size_t i=0; i<inventory.GetSize(); i++)
    {
        inventory[i].exchangeOfferSlot = -1;
		inventory[i].exchangeStackCount = 0;
    }
}
