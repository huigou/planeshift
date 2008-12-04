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
#include "net/msghandler.h"
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
        // Now send the chat window and chat bubbles the nice menu text
        csString text(fld->GetText());
        size_t dot = text.FindFirst('.'); // Take out the numbering to display
        if (dot != SIZET_NOT_FOUND)
        {
            text.DeleteAt(0,dot+1);
        }
		cmd.Format("/tellnpcinternal %s", text.GetData() );
		psengine->GetCmdHandler()->Publish(cmd);
		responseList->Clear();
		Hide();
	}
}

void pawsNpcDialogWindow::HandleMessage( MsgEntry* me )
{
    if ( me->GetType() == MSGTYPE_DIALOG_MENU )
    {
        psDialogMenuMessage mesg(me);

		printf( "Got psDialogMenuMessage: %s\n", mesg.xml.GetDataSafe() );
		responseList->Clear();

		SelfPopulateXML(mesg.xml);

		Show();
    }
}
