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
#define NEXT 1003
#define PREV 1004

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool pawsBookReadingWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_READ_BOOK);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_INFO);

    // Store some of our children for easy access later on.
    name = dynamic_cast<pawsTextBox*>(FindWidget("ItemName"));
    if(!name) return false;

    description = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescription"));
    if(!description) return false;

    // get the right hand window, this is needed for the open book view
    descriptionRight = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionRight"));
    if(!descriptionRight) return false;

    descriptionCraft = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionCraft"));
    //if ( !descriptionCraft ) return false;

    // the same for the craft window, otherwise everything could bug
    descriptionCraftRight = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionCraftRight"));
    //if ( !descriptionCraftRight ) return false;

    writeButton = FindWidget("WriteButton");
    //if ( !writeButton ) return false;

    saveButton = FindWidget("SaveButton");
    //if ( !saveButton ) return false;


    nextButton = FindWidget("NextButton");
    prevButton = FindWidget("PrevButton");

    usingCraft = false; // until the message turns up
    return true;
}

void pawsBookReadingWindow::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_READ_BOOK:
        {
            Show();
            psReadBookTextMessage mesg(me);
            csRef<iDocumentNode> docnode = ParseStringGetNode(mesg.text, "Contents", false);
            if(docnode)
            {
                // these are casted into the docnode object
                dynamic_cast<pawsMultiPageDocumentView*>(description)->SetText(mesg.text.GetData());
                dynamic_cast<pawsMultiPageDocumentView*>(descriptionRight)->SetText(mesg.text.GetData());
            }
            else
            {
                description->SetText(mesg.text);
                descriptionRight->SetText(mesg.text);
            }
            name->SetText(mesg.name);
            slotID = mesg.slotID;
            containerID = mesg.containerID;
            if(writeButton)
            {
                if(mesg.canWrite)
                {
                    writeButton->Show();
                }
                else
                {
                    writeButton->Hide();
                }
            }
            if(saveButton)
            {
                if(mesg.canWrite)
                {
                    saveButton->Show();
                }
                else
                {
                    saveButton->Hide();
                }
            }
            if(descriptionCraft)
            {
                descriptionCraft->Hide();
            }
            if(descriptionCraftRight)
            {
                descriptionCraftRight->Hide();
            }
            // setup the windows for multi page view

            numPages = description->GetNumPages();

            // set the descriptionRight to be 1 page ahead of description
            descriptionRight->SetCurrentPageNum(description->GetCurrentPageNum()+1) ;

            description->Show();
            descriptionRight->Show();
            //set the background image for the book
            bookBgImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(mesg.backgroundImg);
            usingCraft = false;
            break;
        }
        case MSGTYPE_CRAFT_INFO:
        {
            Show();
            if(writeButton)
            {
                writeButton->Hide();
            }
            if(saveButton)
            {
                saveButton->Hide();
            }
            if(description)
            {
                description->Hide();
            }
            if(descriptionRight)
            {
                descriptionRight->Hide();
            }

            psMsgCraftingInfo mesg(me);
            csString text(mesg.craftInfo);
            text.ReplaceAll( "[[", "   With Higher " );
            text.ReplaceAll( "]]   ", " skill you could: " );

            if(text && descriptionCraft && descriptionCraftRight)
            {
                // setup the craft windows for multi page view

                descriptionCraft->SetText(text.GetData());
                descriptionCraftRight->SetText(text.GetData());

                numPages = descriptionCraft->GetNumPages();

                // set the descriptionCraftRight to be one page ahead
                descriptionCraftRight->SetCurrentPageNum(descriptionCraft->GetCurrentPageNum()+1) ;

                // show both pages
                descriptionCraft->Show();
                descriptionCraftRight->Show();

                usingCraft = true;
            }
            //name->SetText("You discover you can do the following:");
            name->SetText("");
            break;
        }
    }}

bool pawsBookReadingWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(widget->GetID() == EDIT)
    {
        // attempt to write on this book
        psWriteBookMessage msg(slotID, containerID);
        msg.SendMessage();
    }

    if(widget->GetID() == SAVE)
    {
        csRef<iVFS> vfs = psengine->GetVFS();
        csString tempFileNameTemplate = "/planeshift/userdata/books/%s.txt", tempFileName;
        csString bookFormat;
        if(filenameSafe(name->GetText()).Length())
        {
            tempFileName.Format(tempFileNameTemplate, filenameSafe(name->GetText()).GetData());
        }
        else   //The title is made up of Only special chars :-(
        {
            tempFileNameTemplate = "/planeshift/userdata/books/book%d.txt";
            unsigned int tempNumber = 0;
            do
            {
                tempFileName.Format(tempFileNameTemplate, tempNumber);
                tempNumber++;
            }
            while(vfs->Exists(tempFileName));
        }

        bookFormat = description->GetText();
#ifdef _WIN32
        bookFormat.ReplaceAll("\n", "\r\n");
#endif
        vfs->WriteFile(tempFileName, bookFormat, bookFormat.Length());

        psSystemMessage msg(0, MSG_ACK, "Book saved to %s", tempFileName.GetData()+27);
        msg.FireEvent();
        return true;
    }

    if(widget->GetID() == NEXT)
    {
        // traverse forwards if there is a page to go forward to
        if(!usingCraft)
        {
            if(description->GetCurrentPageNum() <= numPages-2)
            {
                // pages were set explicitly rather than using the NextPage() functions
                // this is because the pages are connected in this object
                description->SetCurrentPageNum(description->GetCurrentPageNum() + 2);
                descriptionRight->SetCurrentPageNum(descriptionRight->GetCurrentPageNum() + 2);
            }
        }
        else
        {
            if(descriptionCraft && descriptionCraftRight)
            {
                if(descriptionCraft->GetCurrentPageNum() <= numPages-2)
                {
                    descriptionCraft->SetCurrentPageNum(descriptionCraft->GetCurrentPageNum() + 2);
                    descriptionCraftRight->SetCurrentPageNum(descriptionCraftRight->GetCurrentPageNum() + 2);
                }
            }
        }
        return true;

    }
    if(widget->GetID() == PREV)
    {
        // traverse backwards if there is a page to go back to
        if(!usingCraft)
        {
            if(description->GetCurrentPageNum() >= 2)
            {
                description->SetCurrentPageNum(description->GetCurrentPageNum()- 2);
                descriptionRight->SetCurrentPageNum(descriptionRight->GetCurrentPageNum()-2);
            }
        }
        else
        {
            if(descriptionCraft && descriptionCraftRight)
            {
                if(descriptionCraft->GetCurrentPageNum() >= 2)
                {
                    descriptionCraft->SetCurrentPageNum(descriptionCraft->GetCurrentPageNum()- 2);
                    descriptionCraftRight->SetCurrentPageNum(descriptionCraftRight->GetCurrentPageNum()-2);
                }
            }
        }
        return true;
    }

    // close the Read window
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);
    return true;
}

bool pawsBookReadingWindow::isBadChar(char c)
{
    csString badChars = "/\\?%*:|\"<>";
    if(badChars.FindFirst(c) == (size_t) -1)
        return false;
    else
        return true;
}

void pawsBookReadingWindow::Draw()
{
    pawsWidget::DrawWindow();

    //draw background
    if(bookBgImage)
        bookBgImage->Draw(screenFrame, 0);


    pawsWidget::DrawForeground();
}

csString pawsBookReadingWindow::filenameSafe(const csString &original)
{
    csString safe;
    size_t len = original.Length();
    for(size_t c = 0; c < len; ++c)
    {
        if(!isBadChar(original[c]))
            safe += original[c];
    }
    return safe;
}

