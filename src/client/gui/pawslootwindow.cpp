/*
* pawslootwindow.cpp - Author: Keith Fulton
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//////////////////////////////////////////////////////////////////////

// STANDARD INCLUDE
#include <psconfig.h>
#include "globals.h"

// CLIENT INCLUDES
#include "pscelclient.h"


// COMMON/NET INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"
#include "util/strutil.h"
#include <ivideo/fontserv.h>

// PAWS INCLUDES
#include "pawslootwindow.h"
#include "paws/pawsprefmanager.h"
#include "inventorywindow.h"

#define ROLL_BUTTON  100
#define TAKE_BUTTON  200
#define ROLLALL_BUTTON  300
#define TAKEALL_BUTTON  400


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsLootWindow::pawsLootWindow()
    : psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
    lootList = NULL;
}

pawsLootWindow::~pawsLootWindow()
{
    if (msgqueue)
    {
        msgqueue->Unsubscribe(this, MSGTYPE_LOOT);
        msgqueue->Unsubscribe(this, MSGTYPE_LOOTREMOVE);
    }
}

bool pawsLootWindow::PostSetup()
{
    // Setup this widget to receive messages and commands
    if ( !psCmdBase::Setup( psengine->GetMsgHandler(),
        psengine->GetCmdHandler()) )
        return false;

    // Subscribe to certain types of messages (those we want to handle)
    msgqueue->Subscribe(this, MSGTYPE_LOOT);
    msgqueue->Subscribe(this, MSGTYPE_LOOTREMOVE);

    // Grab the pointer to the loot listbox:
    lootList  = (pawsListBox*)FindWidget("LootList");

    // grab the pointer to the roll buttons
    Roll_btn = (pawsButton*)FindWidget(ROLL_BUTTON);
    RollAll_btn = (pawsButton*)FindWidget(ROLLALL_BUTTON);

    return true;
}

//////////////////////////////////////////////////////////////////////
// Command and Message Handling
//////////////////////////////////////////////////////////////////////

const char* pawsLootWindow::HandleCommand(const char* /*cmd*/)
{
    /*
    char* buff = csStrNew(cmd);

    // Check which command was invoked:
    if ( !strncmp(buff+1, "loot_list", 13))
    {
    // First, display this window to the user
    Show();

    // Query the server for messages:
    QueryServer();
    }
    */
    return NULL;
}

void pawsLootWindow::HandleMessage ( MsgEntry* me )
{
    switch(me->GetType())
    {
        case MSGTYPE_LOOT:
        {
            // Get the loot message data from the server
            psLootMessage message(me);

            lootEntity = message.entity_id;

            // Clear the list box and add the user's loots
            lootList->Clear();

            lootList->SelfPopulateXML(message.lootxml);

            // Set up the row ids.
            for (size_t i = 0;i < lootList->GetRowCount();i++)
            {
                pawsListBoxRow* row = lootList->GetRow(i);
                if(!row)
                    continue;

                pawsTextBox* field = (pawsTextBox*)row->GetColumn(LCOL_ID);
                int id = atoi(field->GetText());
                row->SetID( id );
            }

            if (lootList->GetRowCount() > 0)
            {
                lootList->Select(lootList->GetRow(0),false); // Select first as default
                GEMClientActor* player = psengine->GetCelClient()->GetMainPlayer();  // get the player
                if (player->GetGroupID() == 0)  // if player is not grouped, hide the roll buttons
                {
                    grouped = false;
                    Roll_btn->Hide();
                    RollAll_btn->Hide();
                }
                else
                {
                    grouped = true;
                    Roll_btn->Show();
                    RollAll_btn->Show();
                }

                Show();
            } 
            break;
        }
        case MSGTYPE_LOOTREMOVE:
        {
            if (!IsVisible())
                return;

            psLootRemoveMessage msg(me);

            for (size_t i=0; i < lootList->GetRowCount(); i++)
            {
                pawsListBoxRow* row = lootList->GetRow(i);
                if (!row)
                    continue;

                pawsTextBox* field = (pawsTextBox*)row->GetColumn(LCOL_ID);
                int id = atoi(field->GetText());
                if (id == msg.id)
                {
                    lootList->Remove(row->GetID());

                    size_t count = lootList->GetRowCount();
                    if (count == 0) // Clear and close when empty
                    {
                        lootList->Clear();
                        Hide();
                    }
                    else // Select row at same position, or new last row
                        lootList->Select(lootList->GetRow( (i<count)?i:count-1 ) ,false);

                    return;
                }
            }
            break;
        }
    }
}

bool pawsLootWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    int button = widget->GetID();

    switch( button )
    {
        // Look a selected item.  Sends a message to the server about the ID of
        // item that we want to loot.
        case TAKE_BUTTON:
        {
            pawsListBoxRow *row = lootList->GetSelectedRow();
            if (row)
            {
                pawsTextBox *field = (pawsTextBox*)row->GetColumn(LCOL_ID);
                int id = atoi(field->GetText());
                psLootItemMessage take(0,lootEntity,id,psLootItemMessage::LOOT_SELF);
                msgqueue->SendMessage(take.msg);
            }
            break;
        }

        case ROLL_BUTTON:
        {
            pawsListBoxRow *row = lootList->GetSelectedRow();
            if (row)
            {
                pawsTextBox *field = (pawsTextBox*)row->GetColumn(LCOL_ID);
                int id = atoi(field->GetText());
                psLootItemMessage take(0,lootEntity,id,psLootItemMessage::LOOT_ROLL);
                msgqueue->SendMessage(take.msg);
            }
            break;
        }

        case TAKEALL_BUTTON:
        {
                       psengine->GetCmdHandler()->Execute("/loot all");
            break;
        }

        case ROLLALL_BUTTON:
        {
                       psengine->GetCmdHandler()->Execute("/loot roll all");
            break;
        }
    }

    return true;
}
