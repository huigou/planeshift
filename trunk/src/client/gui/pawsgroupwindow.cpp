/*
 * pawsgroupwidow.cpp - Author: Andrew Craig
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
#include "net/messages.h"
#include "net/cmdhandler.h"
#include "net/msghandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "clientvitals.h"

// PAWS INCLUDES
#include "pawsgroupwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawsstringpromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

#include "net/messages.h"
#include "util/log.h"

#define INVITE_BUTTON       100
#define LEAVE_BUTTON        101
#define DISBAND_BUTTON      102



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsGroupWindow::pawsGroupWindow()
{
    memberList = 0;
    player = NULL;
}

pawsGroupWindow::~pawsGroupWindow()
{
    msgHandler->Unsubscribe( this, MSGTYPE_GUIGROUP );
}

bool pawsGroupWindow::PostSetup()
{
    msgHandler = psengine->GetMsgHandler();

    if ( !msgHandler ) return false;
    if ( !msgHandler->Subscribe( this, MSGTYPE_GUIGROUP ) ) return false;

    memberList = (pawsListBox*)FindWidget("List");
    if ( !memberList ) return false;
    
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if (!chatWindow)
        return false;
    
    return true;
}


void pawsGroupWindow::HandleMessage( MsgEntry* me )
{   
    psGUIGroupMessage incomming(me);

    switch ( incomming.command )
    {
        case psGUIGroupMessage::GROUP:
        {
            Show();            
            HandleGroup( incomming.commandData );
            break;
        }

        case psGUIGroupMessage::MEMBERS:
        {
            Show();            
            HandleMembers( incomming.commandData );
            break;
        }

        case psGUIGroupMessage::LEAVE:
        {
            Hide();
            memberList->Clear();
            pawsTextBox * count = dynamic_cast <pawsTextBox*> (FindWidget("MemberCount"));
            if (count != NULL)
                count->SetText("Members: 0");
                
            if ( incomming.commandData.Length() > 0 )
            {
                psSystemMessage note(0,MSG_INFO, incomming.commandData );
                psengine->GetMsgHandler()->Publish(note.msg);
            }
            
                            
            break; 
        }
    }
}

void pawsGroupWindow::HandleGroup( csString& group )
{
    Show();
}

void pawsGroupWindow::HandleMembers( csString& members )
{
    memberList->Clear();

    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( members );

    if ( error )
    {
        Error2("Error in XML: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", members.GetData());
        return;
    }
    csRef<iDocumentNode> xmlmembers = root->GetNode("L");
    if(!xmlmembers)
    {
        Error2("No <L> tag in %s", members.GetData());
        return;

    }

    csRef<iDocumentNodeIterator> iter = xmlmembers->GetNodes("M");

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> member = iter->Next();

        pawsListBoxRow* box = memberList->NewRow();
        pawsWidget* stats = box->GetColumn(0)->FindWidget("stats");
        pawsProgressBar *progress;
        pawsTextBox *text;
  
        box->SetName ( member->GetAttributeValue("N") );
        
        chatWindow->AddAutoCompleteName( member->GetAttributeValue("N") );
        
        //Set name
        text = (pawsTextBox*)stats->FindWidget("NAME");
        text->SetText( member->GetAttributeValue("N") );

        //Set HP
        progress = (pawsProgressBar*)stats->FindWidget("HP");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue(member->GetAttributeValueAsFloat("H")/100);

        //Set mana
        progress = (pawsProgressBar*)stats->FindWidget("MANA");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue(member->GetAttributeValueAsFloat("M")/100);

    }

    pawsTextBox * count = dynamic_cast <pawsTextBox*> (FindWidget("MemberCount"));
    if (count != NULL)
    {
        csString countStr;
        countStr.Format("Members: %i", memberList->GetRowCount());
        count->SetText(countStr);
    }
}

void pawsGroupWindow::SetStats( GEMClientActor* actor )
{   
    
    csString firstName(actor->GetEntity()->GetName());
    size_t pos=firstName.FindFirst(' ');
    firstName = firstName.Slice(0,pos);

    pawsListBoxRow* row = (pawsListBoxRow*)FindWidget( firstName.GetData() ); 
    
    if ( row )
    {
        float hp;
        float mana;

        // Adjust the hitpoints
        hp = actor->GetVitalMgr()->GetHP();   

        // Adjust the mana 
        mana = actor->GetVitalMgr()->GetMana();   

        pawsProgressBar *progress;

        pawsWidget* stats = row->GetColumn(0)->FindWidget("stats");
        
        //Set HP
        progress = (pawsProgressBar*)stats->FindWidget("HP");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue( hp );

        //Set mana
        progress = (pawsProgressBar*)stats->FindWidget("MANA");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue( mana );
    }
}

void pawsGroupWindow::Draw()
{    
    if (player == NULL)
    {
        SetMainActor(psengine->GetCelClient()->GetMainPlayer());
    }

    player->GetVitalMgr()->Predict( csGetTicks(),"Self" );
    
    if (memberList->GetRowCount()>0)
        SetStats( player );
    pawsWidget::Draw();
}


bool pawsGroupWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    switch ( widget->GetID() )
    {
        case INVITE_BUTTON:
            pawsStringPromptWindow::Create("Who do you want to invite ?", "",
                                           false, 250, 20, this, "NewInvitee");
            return true;
        case LEAVE_BUTTON:
            psengine->GetCmdHandler()->Execute("/leavegroup");
            return true;
        case DISBAND_BUTTON:
            psengine->GetCmdHandler()->Execute("/disband");
            return true;
    }

    return false;
}

void pawsGroupWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_SELECTED)
    {
        pawsListBoxRow* row =  widget->GetSelectedRow();
        if ( row )
        {
            pawsWidget* container = row->GetColumn(0)->FindWidget("stats");
            pawsTextBox* nameTxt = (pawsTextBox*)container->FindWidget("NAME");
            csString name(nameTxt->GetText());

            csString title("Tell ");
            title.Append( name );
            pawsStringPromptWindow::Create(title, csString(""),
                                           false, 220, 20, this, name );
        }
    }
}

void pawsGroupWindow::OnStringEntered(const char *name,int param,const char *value)
{
    if (value && strlen(value))
    {
        if (!strcmp(name,"NewInvitee"))
        {
            psString cmd;
            cmd.AppendFmt("/invite %s", value);
            psengine->GetCmdHandler()->Execute(cmd);
        }
        else
        {
            csString command;
            command.Format("/tell %s %s", name, value);
            psengine->GetCmdHandler()->Execute(command);
        }
    }                
}

