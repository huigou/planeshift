/*
 * pawssmallinventory.cpp 
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pawssmallinventory.h"
#include "paws/pawsborder.h"
#include "paws/pawslistbox.h"
#include "util/log.h"
#include "util/psconst.h"
#include "net/messages.h"
#include "net/msghandler.h"

#include "gui/pawsslot.h"
#include "gui/pawsmoney.h"

#include "globals.h"


pawsSmallInventoryWindow::pawsSmallInventoryWindow()
{
    bulkSlots.SetSize( INVENTORY_BULK_COUNT );
   
}                         

void pawsSmallInventoryWindow::Show()
{
    pawsWidget::Show();
    // Ask the server to send us the inventory
    psGUIInventoryMessage outGoingMessage;
    msgHandler->SendMessage( outGoingMessage.msg );
}



bool pawsSmallInventoryWindow::PostSetup()
{

    if ( !border )
    {
        Error1( "Small Inventory Window expects border=\"yes\" in xml" );
        return false;
    }
    else
    {       
        border->JustTitle();
        
        // Setup our inventory slots in the list box.
        pawsListBox * bulkList = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));        
        for (int i = 0; i < INVENTORY_BULK_COUNT/2; i++)
        {
            pawsListBoxRow * listRow = bulkList->NewRow(i);
            for (int j = 0; j < 2; j++)
            {
                pawsSlot * slot;
                slot = dynamic_cast <pawsSlot*> (listRow->GetColumn(j));
                slot->SetContainer( CONTAINER_INVENTORY_BULK );
                slot->SetSlotID( i*2+j );
                csString name;
                name.Format("invslot_%d",16 + i*2+j);
                slot->SetSlotName(name);
                PawsManager::GetSingleton().Subscribe( name, slot );
                PawsManager::GetSingleton().Subscribe("sigClearInventorySlots", slot);
                slot->SetName( name );     
                bulkSlots[i*2+j] = slot;            
            }                
        }  
        money = dynamic_cast <pawsMoney*> (FindWidget("Money")); 
        if ( !money )
            return false;
    
        money->SetContainer( CONTAINER_INVENTORY_MONEY );        
        
        
        // We want to know about inventory messages. 
        msgHandler = psengine->GetMsgHandler();
        if ( !msgHandler ) 
            return false;
            
        return true;            
    }
    
    return true;    
}

