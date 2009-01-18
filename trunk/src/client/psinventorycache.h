/** psinventorycache.h
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
 * Inventory cache for the client.
 */

#ifndef PS_INVENTORY_CACHE
#define PS_INVENTORY_CACHE

#include "util/pscache.h"
#include "net/msghandler.h"

/**
 * @class psInventoryCache
 *
 * The psInventoryCache class implements the inventory cache on the client.
 */
class psInventoryCache : public psCache
{
    public:
        psInventoryCache();
        ~psInventoryCache();

        struct CachedItemDescription
        {
            csString name;
            float weight;
            float size;
            int stackCount;
            csString iconImage;
            int purifyStatus;
        };
	
        /** Requests inventory from server.
         *  Decides whether to request full inventory
         *  list or just updates the local cache.
         *
         *  @return bool success of inventory request
         */
        bool GetInventory (void);

        /** Store an item in the inventory.
         *
         * @param slot: Slot number
         * @param container: Container id
         * @param name: Item name
         * @param weight: Weight of item
         * @param size: Size of item
         * @param stackCount: Number of items in stack
         * @param purifyStatus: Purify status
         * @return bool Success flag
         */
        bool SetInventoryItem(int slot,
                              int container,
                              csString name,
                              float weight,
                              float size,
                              int stackCount,
                              csString iconImage,
                              int purifyStatus);

        /** Set empty slot.
         *
         * @param slot Slot number
         * @param container: Container id
         * @return bool Success flag
         */
        bool EmptyInventoryItem(int slot, int container);

        /** Empty entire inventory
         *
         */
        void EmptyInventory(void);

        /** Get item from container slot.
         *
         * @param slot: Slot number
         * @param container: Container id
         * @param item: The item pointer
         * @return bool success flag
         */
         CachedItemDescription*GetInventoryItem(int slot);
	
		 /// inline uint32 GetInventoryVersion() const
		 /// Info: Returns the cache version (PS#2691)
		 inline uint32 GetInventoryVersion() const { return version; }
	
		 /// inline void SetInventoryVersion(uint32 ver)
		 /// Info: Sets the cache version (PS#2691)
		 inline void SetInventoryVersion(uint32 ver) { version = ver; }

    private:

        csHash<CachedItemDescription*> itemhash;  // all cached items in equip, bulk or containers
        // CachedItemDescription bulkItems[INVENTORY_BULK_COUNT];    /// cached bulk items
        // CachedItemDescription equipItems[INVENTORY_EQUIP_COUNT];  /// cached equip items

        csRef<MsgHandler> msgHandler;

		uint32 version; // Current cache version (PS#2691)
};

#endif

