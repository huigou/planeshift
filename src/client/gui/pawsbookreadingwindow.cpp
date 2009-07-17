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

// CLIENT INCLUDES
#include "pscelclient.h"

// PAWS INCLUDES

#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/log.h"
#include "pawsbookreadingwindow.h"

#define EDIT 1001
#define SAVE 1002

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool pawsBookReadingWindow::PostSetup()
{
    if (!psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_READ_BOOK))  return false;
    if (!psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_INFO)) return false;

    // Store some of our children for easy access later on.
    name = dynamic_cast<pawsTextBox*> (FindWidget("ItemName"));
    if ( !name ) return false;

    description = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("ItemDescription"));
    if ( !description ) return false;

    descriptionCraft = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("ItemDescriptionCraft"));
    //if ( !descriptionCraft ) return false;
    
    writeButton = dynamic_cast<pawsWidget*> (FindWidget("WriteButton"));
    //if ( !writeButton ) return false;
	
	saveButton= dynamic_cast<pawsWidget*> (FindWidget("SaveButton"));
    //if ( !saveButton ) return false;
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
            if( writeButton )
            {
                if( mesg.canWrite ) {
                    writeButton->Show();
                } else {
                    writeButton->Hide();
                }
            }
            if( saveButton )
            {
                if( mesg.canWrite ) {
                    saveButton->Show();
                } else {
                    saveButton->Hide();
                }
            }
            if( descriptionCraft )
            {
                descriptionCraft->Hide();
            }
            description->Show();
            break;
        }
        case MSGTYPE_CRAFT_INFO:
        {
            Show();
            if( writeButton ) {
                writeButton->Hide();
            }
            if( saveButton ) {
                saveButton->Hide();
            }
            if( description ) {
                description->Hide();
            }
            psMsgCraftingInfo mesg(me);
            csString text(mesg.craftInfo);
            if (text && descriptionCraft)
            {
                descriptionCraft->SetText(text.GetData());
                descriptionCraft->Show();
            }
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

    if (widget->GetID() == SAVE)
    {
        csRef<iVFS> vfs = psengine->GetVFS();
        csString tempFileNameTemplate = "/planeshift/userdata/books/%s.txt", tempFileName;
        csString bookFormat;
		if (filenameSafe(name->GetText()).Length()) {
			tempFileName.Format(tempFileNameTemplate, filenameSafe(name->GetText()).GetData());
		}
		else { //The title is made up of Only special chars :-(
			tempFileNameTemplate = "/planeshift/userdata/books/book%d.txt";
			unsigned int tempNumber = 0;
			do
	        {
	           tempFileName.Format(tempFileNameTemplate, tempNumber);
	           tempNumber++;
	        } while (vfs->Exists(tempFileName));
		}
        
        bookFormat = description->GetText();
#ifdef _WIN32
		bookFormat.ReplaceAll("\n", "\r\n");
#endif
        vfs->WriteFile(tempFileName, bookFormat, bookFormat.Length());
		
        psSystemMessage msg(0, MSG_ACK, "Book saved to %s", tempFileName.GetData()+27 );
        msg.FireEvent();
        return true;
    }

    // close the Read window
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
    return true;
}

bool pawsBookReadingWindow::isBadChar(char c)
{
	csString badChars = "/\\?%*:|\"<>";
	if (badChars.FindFirst(c) == (size_t) -1)
		return false;
	else
		return true;
}

csString pawsBookReadingWindow::filenameSafe(const csString &original)
{
	csString safe;
	size_t len = original.Length();
	for (size_t c = 0; c < len; ++c) {
		if (!isBadChar(original[c]))
			safe += original[c];
	}
	return safe;
}

