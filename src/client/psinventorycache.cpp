/** psinventorycache.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Client's inventory cache.
 */

#include <psconfig.h>
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "psinventorycache.h"
#include "globals.h"

psInventoryCache::psInventoryCache () : version(0)
{
    EmptyInventory();
}

psInventoryCache::~psInventoryCache ()
{
    csHash<CachedItemDescription*>::GlobalIterator loop = itemhash.GetIterator();

    while (loop.HasNext())
    {
        CachedItemDescription *next = loop.Next();
        delete next;
    }

    itemhash.Empty();
}

bool psInventoryCache::GetInventory()
{
    // testing if need to request full inventory or
    // just to refresh local cache.
    if (cacheStatus == INVALID)
    {
        // full list request
        psGUIInventoryMessage request;
        request.SendMessage();
    }
    else
    {
        // updates request
        psGUIInventoryMessage request(psGUIInventoryMessage::UPDATE_REQUEST);
        request.SendMessage();
    }
    
    return true;
}

void psInventoryCache::EmptyInventory(void)
{
    csHash<CachedItemDescription*>::GlobalIterator loop = itemhash.GetIterator();

    while (loop.HasNext())
    {
        CachedItemDescription *next = loop.Next();
        delete next;
    }

    itemhash.Empty();

    PawsManager::GetSingleton().Publish("sigClearInventorySlots");
}

bool psInventoryCache::EmptyInventoryItem(int slot, int container)
{
    CachedItemDescription *id = itemhash.Get(slot,NULL);
    delete id;
    itemhash.DeleteAll(slot);

    return true;
}

bool psInventoryCache::SetInventoryItem(int slot,
                                        int container,
                                        csString name,
                                        csString meshName,
                                        float weight,
                                        float size,
                                        int stackCount,
                                        csString iconImage,
                                        int purifyStatus)
{
    if (itemhash.Get(slot,NULL))
    {
        delete itemhash.Get(slot,NULL);
    }

    //printf("Setting item %s in slot %d\n", name.GetDataSafe(), slot);

    CachedItemDescription *newItem = new CachedItemDescription;
    newItem->name = name;
    newItem->meshName = meshName;
    newItem->weight = weight;
    newItem->size = size;
    newItem->stackCount = stackCount;
    newItem->iconImage = iconImage;
    newItem->purifyStatus = purifyStatus;

    itemhash.PutUnique(slot,newItem);

    if (newItem && newItem->stackCount>0 && newItem->iconImage.Length() != 0)
    {
        csString sigData, data;
        sigData.Format("invslot_%d", slot);

        data.Format( "%s %d %d %s %s", newItem->iconImage.GetData(),
            newItem->stackCount,
            newItem->purifyStatus,
            newItem->meshName.GetData(),
            newItem->name.GetData());

        PawsManager::GetSingleton().Publish(sigData, data);
    }

    return true;
}

psInventoryCache::CachedItemDescription *psInventoryCache::GetInventoryItem(int slot)
{
    return itemhash.Get(slot,NULL);
}

