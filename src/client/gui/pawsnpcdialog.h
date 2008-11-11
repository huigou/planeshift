/*
 * Author: Christian Svensson
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
#ifndef PAWS_NPC_DIALOG
#define PAWS_NPC_DIALOG

#include "paws/pawswidget.h"
#include "net/subscriber.h"

#include "gui/pawscontrolwindow.h"

class pawsListBox;

/** The buddy window that shows your current list of in game 'friends'.  
 *  This allows you to send them a tell or add/remove buddies.
 */
class pawsNpcDialogWindow: public pawsWidget, public psClientNetSubscriber
{
public:
    pawsNpcDialogWindow();

    bool PostSetup();
    void HandleMessage( MsgEntry* me );

    void OnListAction( pawsListBox* widget, int status );

private:
    pawsListBox* responseList;  

};


CREATE_PAWS_FACTORY( pawsNpcDialogWindow );
#endif    
