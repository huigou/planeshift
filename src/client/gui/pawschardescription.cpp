/*
 * pawschardescription.cpp - Author: Christian Svensson
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

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES
#include "net/msghandler.h"
#include "net/messages.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"

// PAWS INCLUDES
#include "pawschardescription.h"
#include "pawsdetailwindow.h"
#include "paws/pawsmanager.h"

#define BTN_OK     100
#define BTN_CANCEL 101

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsCharDescription::pawsCharDescription()
{
    ooc_editing = false;
}

pawsCharDescription::~pawsCharDescription()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_CHARACTERDETAILS);
}

bool pawsCharDescription::PostSetup()
{
    description = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget( "Description" ));
    if ( !description )
        return false;

    return true;
}

void pawsCharDescription::Show()
{
    RequestDetails();
    pawsWidget::Show();
}

void pawsCharDescription::RequestDetails()
{
    if ( !psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_CHARACTERDETAILS ) )
    {
        printf("Couldn't subscribe to MSGTYPE_CHARACTERDETAILS (pawsCharDescription)");
        return;
    }

    psCharacterDetailsRequestMessage requestMsg(true, true, "pawsCharDescription");
    requestMsg.SendMessage();
}

void pawsCharDescription::HandleMessage( MsgEntry* me )
{

    if (me->GetType() == MSGTYPE_CHARACTERDETAILS)
    {
        psCharacterDetailsMessage msg(me);

        if (msg.requestor == "pawsCharDescription")
        {
            if(ooc_editing)
                description->SetText(msg.desc_ooc);
            else
                description->SetText(msg.desc);
            psengine->GetMsgHandler()->Unsubscribe( this, MSGTYPE_CHARACTERDETAILS );
        }
        return;
    }
}

bool pawsCharDescription::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{       
    switch ( widget->GetID() )
    {
        case BTN_OK:
        {
            csString newTxt(description->GetText());
            psCharacterDescriptionUpdateMessage descUpdate(newTxt,ooc_editing);
            psengine->GetMsgHandler()->SendMessage(descUpdate.msg);
            Hide();

            // Show the details window to let the user see the changes
            pawsDetailWindow *detailWindow = (pawsDetailWindow*) PawsManager::GetSingleton().FindWidget("DetailWindow");
            if(detailWindow)
                detailWindow->RequestDetails();
            return true;
        }
        
        case BTN_CANCEL:
        {
            this->Close();
            return true;
        }

    }
    return false;
}
