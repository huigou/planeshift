/*
 * economymanager.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Creation Date: 11/Jan 2005
 * Description : This is the implementation for the economymanager
 *               This manager handles all the exchange/trade things and calculates
 *               prices and keeps histories of transactions and so on
 *
 */

#include <psconfig.h>
#include <math.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

#include "util/eventmanager.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/mathscript.h"

#include "bulkobjects/pstrade.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psmerchantinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gem.h"
#include "globals.h"
#include "netmanager.h"
#include "psserver.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "economymanager.h"
#include "events.h"
#include "globals.h"

#if 0
#define ECONOMY_DEBUG
#endif

EconomyManager::EconomyManager()
{
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_BUY_EVENT,NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_SELL_EVENT,NO_VALIDATION);

};


EconomyManager::~EconomyManager()
{
    if(psserver->GetEventManager())
    {
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_BUY_EVENT);
        psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_SELL_EVENT);
    }
}

void EconomyManager::AddTransaction(TransactionEntity* trans,bool moneyIn, const char* type)
{
    if(!trans)
        return;
    
    csString buf;
    
    buf.Format("%d, %d, %s, %d, %u, \"%u\"", trans->from, trans->to, type, trans->item, trans->count, trans->price);
    psserver->GetLogCSV()->Write(CSV_EXCHANGES, buf);
    
    if(!trans->item)
        return;
#ifdef ECONOMY_DEBUG
    CPrintf(
        CON_DEBUG,
        "Adding %s transaction for item %s (%d's, %d qua) (%d => %d) with price %u\n",
        moneyIn?"moneyIn":"moneyOut",
        trans->item.GetData(),
        trans->count,
        trans->quality,
        trans->from,
        trans->to,
        trans->price);
#endif
    trans->moneyIn = moneyIn;
    trans->stamp = time(NULL);

    history.Push(trans);

    if (!supplyDemandInfo.Contains(trans->item))
    {
        csRef<ItemSupplyDemandInfo> newInfo = csPtr<ItemSupplyDemandInfo>(new ItemSupplyDemandInfo);
        supplyDemandInfo.Put(trans->item, newInfo);
    }

    if (moneyIn)
    {
        (*supplyDemandInfo[trans->item])->sold+=trans->count;
    }
    else
    {
        (*supplyDemandInfo[trans->item])->bought+=trans->count;
    }

}

void EconomyManager::HandleMessage(MsgEntry* me,Client* client)
{
    switch(me->GetType())
    {
        case MSGTYPE_BUY_EVENT:
        {
            psBuyEvent event(me);
            AddTransaction(event.trans,false, "Buy");
            economy.buyingValue += event.trans->price;
            break;
        }
        case MSGTYPE_SELL_EVENT:
        {
            psSellEvent event(me);
            AddTransaction(event.trans,true, "Sell");
            economy.sellingValue += event.trans->price;
            break;
        }
        case MSGTYPE_PICKUP_EVENT:
        {
            psPickupEvent event(me);
            AddTransaction(event.trans,true, "Pickup");
            economy.pickupsValue += event.trans->price;
            break;
        }
        case MSGTYPE_DROP_EVENT:
        {
            psDropEvent event(me);
            AddTransaction(event.trans,false, "Drop");
            economy.droppedValue += event.trans->price;
            break;
        }
        case MSGTYPE_LOOT_EVENT:
        {
            psLootEvent event(me);
            AddTransaction(event.trans,true, "Loot");
            economy.lootValue += event.trans->price;
            break;
        }
    }
}

TransactionEntity* EconomyManager::GetTransaction(int id)
{
    if((size_t)id > history.GetSize() || id < 0)
        return NULL;

    return history[id];
}

void EconomyManager::ScheduleDrop(csTicks ticks,bool loop)
{
    // Construct a new psEconomyDrop event
    psEconomyDrop* drop = new psEconomyDrop(this,ticks,loop);

    // Add the event to the que
    psserver->GetEventManager()->Push(drop);
}

unsigned int EconomyManager::GetTotalTransactions()
{
    return (unsigned int)history.GetSize();
}

void EconomyManager::ClearTransactions()
{
    history.DeleteAll();
    supplyDemandInfo.DeleteAll();
}

csRef<ItemSupplyDemandInfo> EconomyManager::GetItemSupplyDemandInfo(unsigned int itemId)
{
    if (!supplyDemandInfo.Contains(itemId))
    {
        csRef<ItemSupplyDemandInfo> newInfo = csPtr<ItemSupplyDemandInfo> (new ItemSupplyDemandInfo);
        supplyDemandInfo.Put(itemId, newInfo);
    }
    return *supplyDemandInfo[itemId];
}

psEconomyDrop::psEconomyDrop(EconomyManager* manager,csTicks ticks, bool loop)
            :psGameEvent(0,ticks,"psEconomyDrop")
{
    this->loop = loop;
    economy = manager;
    eachTimeTicks = ticks;
}

struct ItemCount
{
    unsigned int item;
    bool sold;
    int count;
    int price;
};

void psEconomyDrop::Trigger()
{
    // Calculate the stat
    csString seperator;
    seperator.Format("Time: %d,Transactions recorded: %u",
        (int)time(NULL),
        economy->GetTotalTransactions()
        );
    psserver->GetLogCSV()->Write(CSV_ECONOMY, seperator );
#ifdef ECONOMY_DEBUG
    CPrintf(CON_DEBUG,seperator);
#endif

    if(economy->GetTotalTransactions() > 0)
    {
        csArray<ItemCount> items;
        for(unsigned int i = 0; i< economy->GetTotalTransactions();i++)
        {
            TransactionEntity* trans = economy->GetTransaction(i);
            if(trans)
            {
                // Look if we already recorded the item once
                bool found = false;
                for(unsigned int z = 0; z < items.GetSize();z++)
                {
                    if(items[z].item == trans->item && items[z].sold == trans->moneyIn)
                    {
                        items[z].count++; // Increase the count
                        if(items[z].price < (int)trans->price)
                            items[z].price = (int)trans->price;

                        found = true;
                        break;
                    }
                }

                // Add the item
                if(!found)
                {
                    ItemCount item;
                    item.item = trans->item;
                    item.count = 1;
                    item.sold = trans->moneyIn;
                    item.price = trans->price;
                    items.Push(item);
                }

                // Dump it
                csString str;
                str.Format("%s,%d,%d,%d,%d,%d,%u,%d",
                    trans->moneyIn?"moneyIn":"moneyOut",
                    trans->count,
                    trans->item,
                    trans->quality,
                    trans->from,
                    trans->to,
                    trans->price,
                    (int)time(NULL) - trans->stamp);

                // Write
                psserver->GetLogCSV()->Write(CSV_ECONOMY, str);

    #ifdef ECONOMY_DEBUG
                CPrintf(CON_DEBUG,str);
    #endif
            }
        }

        unsigned int mosts = 0; // Sold
        unsigned int mostb = 0;  // Bought
        unsigned int mostv = 0; // Valuable
        for(unsigned int z = 0; z < items.GetSize();z++)
        {
            if (items[z].count > items[mosts].count && items[z].sold)
                mosts = z;

            if (items[z].count > items[mostb].count && !items[z].sold)
                mostb = z;

            if (items[z].price > items[mostv].price)
                mostv = z;
        }

        // Write the ending stuff
        seperator.Format("Most valueable item: %d (%d), Most sold item: %d, Most bought item: %d",
            items[mostv].item,
            items[mostv].price,
            items[mosts].item,
            items[mostb].item
            );
        psserver->GetLogCSV()->Write(CSV_ECONOMY, seperator );
    #ifdef ECONOMY_DEBUG
        CPrintf(CON_DEBUG,seperator);
    #endif
        economy->ClearTransactions();
    }
    
    if(loop)
        economy->ScheduleDrop(eachTimeTicks,true);
}

