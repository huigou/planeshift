/*
 * pawsdetailwindow.cpp - Author: Christian Svensson
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
// pawsdetailwindow.cpp: implementation of the pawsDetailWindow class.
//
//////////////////////////////////////////////////////////////////////

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
#include "pawsdetailwindow.h"
#include "pawschardescription.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"

#define BTN_EDIT      100  //Edit button, if viewing your own description
#define BTN_DESCR     999  //Description button for the tab panel
#define BTN_DESCROOC 1006  //Out of character Description button for the tab panel
#define BTN_DESCRCC  1007  //Char creation description button for the tab panel
#define BTN_STATS    1000 //Stats button for the tab panel
#define BTN_COMBAT   1001 //Combat button for the tab panel
#define BTN_MAGIC    1002 //Magic button for the tab panel
#define BTN_JOBS     1003 //Jobs button for the tab panel
#define BTN_VARIOUS  1004 //Various button for the tab panel
#define BTN_FACTION  1005

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsDetailWindow::pawsDetailWindow()
{
    target = NULL;     
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_CHARACTERDETAILS );
}

pawsDetailWindow::~pawsDetailWindow()
{
    psengine->GetMsgHandler()->Unsubscribe( this, MSGTYPE_CHARACTERDETAILS );
}

bool pawsDetailWindow::PostSetup()
{
    intro = (pawsMultiLineTextBox*)FindWidget( "intro" );
    if ( !intro )
        return false;

    description = (pawsMultiLineTextBox*)FindWidget("Description");
    if ( !description )
        return false;

    editButton = (pawsButton*) FindWidget("EditDesc");
    if (!editButton)
        return false;

    pawsButton* button = (pawsButton*)FindWidget( "ShowDescr" );
    if(button)
    {
        lastTab = button;
        button->SetState(true);
        button->Show();
    }
    button = (pawsButton*)FindWidget( "ShowDescrOOC" );
    if(button) button->Show();
    button = (pawsButton*)FindWidget( "ShowCC" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowStats" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowCombat" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowMagic" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowCraft" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowMisc" );
    if(button) button->Hide();
    button = (pawsButton*)FindWidget( "ShowFaction" );
    if(button) button->Hide();
    skills.SetSize(6);
    for(int i=0; i<6; i++)
    {
        skills[i].Clear();
    }
    return true;
}

void pawsDetailWindow::RequestDetails()
{
    psCharacterDetailsRequestMessage requestMsg(false, false, "pawsDetailWindow");
    psengine->GetMsgHandler()->SendMessage(requestMsg.msg);
}

void pawsDetailWindow::HandleMessage( MsgEntry* me )
{

    if (me->GetType() == MSGTYPE_CHARACTERDETAILS)
    {
        psCharacterDetailsMessage msg(me);

        if (   msg.requestor!="pawsDetailWindow" 
            && msg.requestor!="behaviorMsg" 
            && msg.requestor!="ShowDetailsOp")
            return;

        //Begin sentence
        csString str("You see a ");

        //Add gender
        switch(msg.gender)
        {
        case PSCHARACTER_GENDER_FEMALE:
            {
                str.Append("female ");
                break;
            }
        case PSCHARACTER_GENDER_MALE:
            {
                str.Append("male ");
                break;
            }
        case PSCHARACTER_GENDER_NONE:
            {    //Don't append anything if it is neutral
                break;
            }
        }

        //Add race
        str.Append(msg.race);

        //Add seperator
        str.Append(" in front\nof you named ");

        //Add name
        str.Append(msg.name);

        intro->SetText( str.GetData() );
        storedescription = msg.desc.GetData();
        storedoocdescription = msg.desc_ooc;
        storedcreationinfo = msg.creationinfo;

        for(int i=0; i<6; i++)
        {
            skills[i].Clear();
        }
        if(storedcreationinfo.Length())
        {
            pawsButton* button = (pawsButton*)FindWidget( "ShowCC" );
            if(button) button->Show();
        }
        if(msg.skills.GetSize() != 0 ) {
            pawsButton* button = (pawsButton*)FindWidget( "ShowStats" );
            if(button) button->Show();
            button = (pawsButton*)FindWidget( "ShowCombat" );
            if(button) button->Show();
            button = (pawsButton*)FindWidget( "ShowMagic" );
            if(button) button->Show();
            button = (pawsButton*)FindWidget( "ShowCraft" );
            if(button) button->Show();
            button = (pawsButton*)FindWidget( "ShowMisc" );
            if(button) button->Show();
            button = (pawsButton*)FindWidget( "ShowFaction" );
            if(button) button->Show();
            for( size_t s = 0; s < msg.skills.GetSize(); s++ )
            {
                int cat = msg.skills[s].category;
                if(cat >= 0 && cat < 6)
                {
                    skills[cat].Append(msg.skills[s].text);
                }
		        SelectTab((pawsWidget*)lastTab);
            }
        }

        //check if the player is looking at his/her/its own description
        if (msg.name == psengine->GetCelClient()->GetMainPlayer()->GetName())
        {
            details_editable = true;
            if(!storedescription.Length() ) //if the player didn't input anything inform him
                storedescription = PawsManager::GetSingleton().Translate("This window is what people will read when they click on you; this box is not to be filled with anything other than things that a person could reasonably perceive with their senses. There is no reason to include anything beyond what one can sense when looking at your character. While sensual detail ONLY is to be included, this does not mean that you should be sparing, please include as many appeals to the senses as your character clearly eminates that can be sensed with the five accepted senses (sight, touch, hearing and smell (being primary)(only list taste if you anticipate people tasting your character)).");
        }
        else
        {
            details_editable = false;
        }

        SelectTab(lastTab);

        this->Show();
        return;
    }
}

bool pawsDetailWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    return SelectTab(widget);
}

bool pawsDetailWindow::SelectTab( pawsWidget* widget )
{       
    switch ( widget->GetID() )
    {
    case BTN_EDIT:
        {
            pawsCharDescription *editWindow = (pawsCharDescription*) PawsManager::GetSingleton().FindWidget("DescriptionEdit");
            editWindow->PostSetup();
            if(lastTab == (pawsButton*)FindWidget( "ShowDescrOOC" ))
                editWindow->SetOOCDescription(true);
            editWindow->Show();
            return true;
        }
    case BTN_STATS:
    case BTN_FACTION:
    case BTN_COMBAT:
    case BTN_MAGIC:
    case BTN_JOBS:
    case BTN_VARIOUS:
        {   
            int id = widget->GetID() - 1000;
            if(id >= 0 && id < 6)
            {
                if(lastTab != (pawsButton*)widget)
                {
                    lastTab->SetState(false);
                    lastTab = (pawsButton*)widget;
                }
                else
                {
                    lastTab->SetState(true);
                }
                description->SetText(skills[id]);
            }
            editButton->Hide();  //hide the edit button it's useless here
            break;
        }
    case BTN_DESCR:
    case BTN_DESCROOC:
    case BTN_DESCRCC:
        {
            if(lastTab != (pawsButton*)widget)
            {
                lastTab->SetState(false);
                lastTab = (pawsButton*)widget;
            }
            else
            {
                lastTab->SetState(true);
            }
            if(widget->GetID() == BTN_DESCR)
            {
                description->SetText(storedescription);
                if(details_editable) editButton->Show(); //if it's editable show the button
            }
            else if(widget->GetID() == BTN_DESCRCC)
            {
                description->SetText(storedcreationinfo);
                editButton->Hide();  //hide the edit button it's useless here
            }
            else
            {
                description->SetText(storedoocdescription);
                if(details_editable) editButton->Show(); //if it's editable show the button
            }
            break;
        }
    }
    return false;
}
