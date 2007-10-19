/*
 * pawsbookreadingwindow.cpp - Author: Daniel Fryer, based on code by Andrew Craig
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

#include <psconfig.h>
#include "globals.h"

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES
#include <propclass/linmove.h>

// CLIENT INCLUDES
#include "pscelclient.h"

// PAWS INCLUDES

#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/msghandler.h"
#include "util/log.h"
#include "pawsbookreadingwindow.h"

#define EDIT 1001

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool pawsBookReadingWindow::PostSetup()
{
    msgHandler = psengine->GetMsgHandler();

    if ( !msgHandler ) return false;
    if ( !msgHandler->Subscribe( this,  MSGTYPE_READ_BOOK ) ) return false;
    if ( !msgHandler->Subscribe( this, MSGTYPE_CRAFT_INFO ) ) return false;

    // Store some of our children for easy access later on.
    name = dynamic_cast<pawsTextBox*> (FindWidget("ItemName"));
    if ( !name ) return false;

    description = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("ItemDescription"));
    if ( !description ) return false;
    
    writeButton = dynamic_cast<pawsWidget*> (FindWidget("WriteButton"));
    //if ( !writeButton ) return false;

    return true;
}

void pawsBookReadingWindow::HandleMessage( MsgEntry* me )
{   
    switch ( me->GetType() )
    {
        case MSGTYPE_READ_BOOK:
        {
            Show();
            psReadBookTextMessage mesg( me );
            description->SetText( mesg.text );     
            name->SetText( mesg.name );       
            slotID = mesg.slotID;
            containerID = mesg.containerID;
            if( writeButton ){
               if( mesg.canWrite ) writeButton->Show();
               else writeButton->Hide();
            }
            break;
        }
        case MSGTYPE_CRAFT_INFO:
        {
            Show();
            writeButton->Hide();
            psMsgCraftingInfo mesg(me);
            csString text(mesg.craftInfo);
            if (text)
                description->SetText(text.GetData());
            name->SetText( "You discover you can do the following:" );
            break;
        }
    }
}

bool pawsBookReadingWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if(widget->GetID() == EDIT){
        // attempt to write on this book
        psWriteBookMessage msg(slotID, containerID);
        msg.SendMessage();
    }

    // close the Read window
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
    return true;
}

