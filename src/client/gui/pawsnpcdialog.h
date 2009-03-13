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

#include "net/subscriber.h"

#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"


class pawsListBox;

/** This window shows the popup menu of available responses
 *  when talking to an NPC.
 */
class pawsNpcDialogWindow: public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    pawsNpcDialogWindow();

    bool PostSetup();
    void HandleMessage( MsgEntry* me );

    void OnListAction( pawsListBox* widget, int status );

	void OnStringEntered(const char *name,int param,const char *value);

private:
	void AdjustForPromptWindow();
	void DisplayTextBubbles(const char *sayWhat);

	pawsListBox* responseList;  
};


CREATE_PAWS_FACTORY( pawsNpcDialogWindow );
#endif    
