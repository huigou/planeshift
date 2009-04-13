/*
* pawsnpcdialog.cpp - Author: Christian Svensson
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "../globals.h"
#include "paws/pawslistbox.h"
#include "gui/pawscontrolwindow.h"
#include "pscelclient.h"

#include "pawsnpcdialog.h"


pawsNpcDialogWindow::pawsNpcDialogWindow()
{
    responseList = NULL;
}

bool pawsNpcDialogWindow::PostSetup()
{
	psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_DIALOG_MENU );
                
    responseList = (pawsListBox*)FindWidget("ResponseList");
    return true;
}

void pawsNpcDialogWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status == LISTBOX_HIGHLIGHTED)
    {
		pawsTextBox *fld = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
		printf( "Pressed: %s\n",fld->GetText() );
    }
	else if (status == LISTBOX_SELECTED)
	{
		pawsTextBox *fld  = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
		printf("Player chose '%s'.\n", fld->GetText() );
		pawsTextBox *trig = dynamic_cast<pawsTextBox *>(widget->GetSelectedRow()->FindWidgetXMLBinding("trig"));
		printf("Player says '%s'.\n", trig->GetText() );

        csString trigger(trig->GetText());

        // Send the server the original trigger
    	csString cmd;
        if (trigger.GetAt(0) == '=') // prompt window signal
		{
			pawsStringPromptWindow::Create(csString(trigger.GetData()+1),
				                           csString(""),
				                           false, 320, 30, this, trigger.GetData()+1 );
		}
		else
		{
			if (trigger.GetAt(0) != '<')
			{
	    		cmd.Format("/tellnpc %s", trigger.GetData() );
				psengine->GetCmdHandler()->Publish(cmd);
			}
			else
			{
				psSimpleStringMessage gift(0,MSGTYPE_EXCHANGE_AUTOGIVE,trigger);
				gift.SendMessage();
			}
			DisplayTextBubbles(fld->GetText());
		}
		Hide();
	}
}

void pawsNpcDialogWindow::DisplayTextBubbles(const char *sayWhat)
{
	// Now send the chat window and chat bubbles the nice menu text
	csString text(sayWhat);
	size_t dot = text.FindFirst('.'); // Take out the numbering to display
	if (dot != SIZET_NOT_FOUND)
	{
		text.DeleteAt(0,dot+1);
	}
	csString cmd;
	cmd.Format("/tellnpcinternal %s", text.GetData() );
	psengine->GetCmdHandler()->Publish(cmd);
	responseList->Clear();
}

void pawsNpcDialogWindow::HandleMessage( MsgEntry* me )
{
    if ( me->GetType() == MSGTYPE_DIALOG_MENU )
    {
        psDialogMenuMessage mesg(me);

		printf( "Got psDialogMenuMessage: %s\n", mesg.xml.GetDataSafe() );
		responseList->Clear();

		SelfPopulateXML(mesg.xml);

		AdjustForPromptWindow();

		Show();
    }
}

void pawsNpcDialogWindow::AdjustForPromptWindow()
{
	csString str;

	for (size_t i=0; i<responseList->GetRowCount(); i++)
	{
		str = responseList->GetTextCellValue(i,0);
		size_t where = str.Find("?=");
		if (where != SIZET_NOT_FOUND) // we have a prompt choice
		{
			pawsTextBox *hidden = (pawsTextBox *)responseList->GetRow(i)->GetColumn(1);
			if (where != SIZET_NOT_FOUND)
			{
				str.DeleteAt(where,1); // take out the ?
				hidden->SetText(str.GetData() + where); // Save the question prompt, starting with the =, in the hidden column
				str.DeleteAt(where,1); // take out the =

				// now change the visible menu choice to something better
				pawsTextBox *prompt = (pawsTextBox *)responseList->GetRow(i)->GetColumn(0);

				csString menuPrompt(str);
				menuPrompt.Insert(where,"<Answer ");
				menuPrompt.Append('>');
				prompt->SetText(menuPrompt);
			}
		}
	}
}

void pawsNpcDialogWindow::OnStringEntered(const char *name,int param,const char *value)
{
	printf("Got name=%s, value=%s\n", name, value);

	csString cmd;
	cmd.Format("/tellnpc %s", value );
	psengine->GetCmdHandler()->Publish(cmd);
	DisplayTextBubbles(value);
}
