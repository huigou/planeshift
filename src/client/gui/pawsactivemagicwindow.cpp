/*
 * pawsactivemagicwindow.cpp
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

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/strutil.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsactivemagicwindow.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"

#define BUFF_CATEGORY_PREFIX    "+"
#define DEBUFF_CATEGORY_PREFIX  "-"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool pawsActiveMagicWindow::PostSetup()
{
    pawsWidget::PostSetup();

    buffCategories        = (pawsListBox*)FindWidget("BuffCategories");
    if (!buffCategories)
        return false;
    debuffCategories      = (pawsListBox*)FindWidget("DebuffCategories");
    if (!debuffCategories)
        return false;

    if (!psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC))
        return false;

    // do something here....
    return true;
}

void pawsActiveMagicWindow::HandleMessage( MsgEntry* me )
{
    psGUIActiveMagicMessage incoming(me);
    csList<csString> rowEntry;

    if (!IsVisible() && psengine->loadstate == psEngine::LS_DONE)
        Show();

    pawsListBox *list = incoming.type == BUFF ? buffCategories : debuffCategories;
    switch ( incoming.command )
    {
        case psGUIActiveMagicMessage::Add:
        {
            rowEntry.PushBack(incoming.name);
            pawsListBoxRow *row = list->NewTextBoxRow(rowEntry);

            break;
        }
        case psGUIActiveMagicMessage::Remove:
        {
            for (size_t i = 0; i < list->GetRowCount(); i++)
            {
                pawsListBoxRow *row = list->GetRow(i);
                pawsTextBox *name = dynamic_cast<pawsTextBox*>(row->GetColumn(0));
                if (incoming.name == name->GetText())
                    list->Remove(row);
            }

            // If no active magic, hide the window.
            if (debuffCategories->GetRowCount() + buffCategories->GetRowCount() == 0)
                Hide();

            break;
        }
    }
}

void pawsActiveMagicWindow::Close()
{
    Hide();
}


