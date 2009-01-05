/*
 * psserverchar.h
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PS_SERVER_CHAR_MANAGER_H
#define PS_SERVER_CHAR_MANAGER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <csutil/ref.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

#include "util/slots.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

class MsgEntry;
class ClientConnectionSet;
class psServer;
class Client;
class psMerchantInfo;
class psItem;
class psActionLocation;
class SlotManager;
class MathScriptVar;
class gemActor;
class gemObject;
class psCharacter;

struct CraftTransInfo;
struct psItemCategory;


/// Manages character details over the net.
/**
 * This is used instead of cel persistance because the persistance
 * is an all or nothing sort of thing. By using this we can make
 * more efficient use of the net traffic
 */
class psServerCharManager : public MessageManager
{
public:

    psServerCharManager();
    virtual ~psServerCharManager();

    bool Initialize( ClientConnectionSet* ccs);

    virtual void HandleMessage( MsgEntry *me, Client *client );

    // Handles any incomming messages about the character gui.
    virtual bool HandleInventoryMessage( MsgEntry* me );

	void HandleFaction(MsgEntry* me);

    /// Sends the client an inventory
    virtual bool SendInventory( int clientNum, bool sendUpdatesOnly=true );

    void SendPlayerMoney( Client *client);

    /// Update all views with items
    virtual bool UpdateItemViews( int clientNum );

    void SendOutPlaySoundMessage( int clientnum, const char* itemsound, const char* action );

    /** Sends out equipment messages to all the people around client.
     *
     * This is used when a player changes their visible equipment and needs
     * to be reflected on other nearby clients. Visible changes can be either
     * new weapons/shields ( new mesh ) or texture changes for clothes ( new material ).
     *
     * @param actor The actor that has changed equipment.
     * @param slotName To what slot has changed.
     * @param item The item that is the piece of visible equipment.
     * @param type  The equiping type. One of:
     *                psEquipmentMessage::DEEQUIP
     *                psEquipmentMessage::EQUIP
     *
     */
    void SendOutEquipmentMessages( gemActor* actor,
                                   INVENTORY_SLOT_NUMBER slotID,
                                   psItem* item,
                                   int equipped );

    void ViewItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID);

    void BeginTrading(Client * client, gemObject * target, const csString & type);

    bool IsBanned(const char* name);

    ///Checked if the character exists still or if it hasn't connected in two months.
    bool HasConnected( csString name );

protected:

    // -------------------Merchant Handling -------------------------------
    int CalculateMerchantPrice(psItem *item, Client *client, bool sellPrice);
    bool SendMerchantItems( Client *client, psCharacter * merchant, psItemCategory * category);
    void HandleMerchantMessage( MsgEntry* me, Client *client );
    void HandleMerchantRequest(psGUIMerchantMessage& msg, Client *client);
    void HandleMerchantCategory(psGUIMerchantMessage& msg, Client *client);
    void HandleMerchantBuy(psGUIMerchantMessage& msg, Client *client);
    void HandleMerchantSell(psGUIMerchantMessage& msg, Client *client);
    void HandleMerchantView(psGUIMerchantMessage& msg, Client *client);


    bool SendPlayerItems( Client *client, psItemCategory * category);
   
    void HandleBookWrite(MsgEntry* me, Client* client);
    void HandleCraftTransInfo( MsgEntry * me, Client *client );

    /// Return true if all trade params are ok
    bool VerifyTrade( Client * client, psCharacter * character, psCharacter ** merchant, psMerchantInfo ** info,
                      const char * trade, const char * itemName, PID merchantID);

    // verifies that item dropped in mind slot is a valid goal
    bool VerifyGoal(Client* client, psCharacter* character, psItem* goal);

    ClientConnectionSet*    clients;

    SlotManager *slotManager;

    void ViewItem( MsgEntry* me );
    void UpdateSketch( MsgEntry* me );

    MathScript* calc_item_merchant_price_buy;
    MathScriptVar* calc_item_merchant_price_item_price_buy;
    MathScriptVar* calc_item_merchant_price_char_data_buy;
    MathScriptVar* calc_item_merchant_price_char_result_buy;
    MathScriptVar* calc_item_merchant_price_supply_buy;
    MathScriptVar* calc_item_merchant_price_demand_buy;
    MathScriptVar* calc_item_merchant_price_time_buy;
    
    MathScript* calc_item_merchant_price_sell;
    MathScriptVar* calc_item_merchant_price_item_price_sell;
    MathScriptVar* calc_item_merchant_price_char_data_sell;
    MathScriptVar* calc_item_merchant_price_char_result_sell;
    MathScriptVar* calc_item_merchant_price_supply_sell;
    MathScriptVar* calc_item_merchant_price_demand_sell;
    MathScriptVar* calc_item_merchant_price_time_sell;
};

#endif


