/*
 * Author: Andrew Craig
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_BUDDY_HEADER
#define PAWS_BUDDY_HEADER

#include "paws/pawswidget.h"
#include "net/subscriber.h"

#include "paws/pawsstringpromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

class pawsListBox;

#define ALIASES_FILE_PREFIX       "/planeshift/userdata/aliases_"


/** The buddy window that shows your current list of in game 'friends'.  
 *  This allows you to send them a tell or add/remove buddies.
 */
class pawsBuddyWindow : public pawsControlledWindow, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    pawsBuddyWindow();

    bool PostSetup();
    void HandleMessage( MsgEntry* me );

    bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* widget, int status );
    void OnStringEntered(const char *name,int param,const char *value);

    virtual void Show();

    virtual void OnResize();

private:
    pawsListBox* buddyList;  

    /// Name of the currently selected buddy (NB! alias, not the real name)
    csString currentBuddy;
    pawsChatWindow* chatWindow;

    /// Real name of the buddy that is being edited
    csString editBuddy;

    csStringArray onlineBuddies;
    csStringArray offlineBuddies;

    /// Alias/name table
    csHash<csString, csString> aliases;

    /// Returns the alias for the name or the name itself if there is no alias for it.
    csString GetAlias(const csString & name) const;

    /// Reverse (slow) search for the real name.
    csString GetRealName(const csString & alias) const;

    /// Loads aliases from the xml file.
    void LoadAliases(const csString & charName);

    /// Saves aliases to the xml file.
    void SaveAliases(const csString & charName) const;

    /// Changes the alias for the given name.
    void ChangeAlias(const csString & name, const csString & oldAlias, const csString & newAlias);

    /**
     * Populates the buddy listbox with names from online and offline buddy list arrays.
     *
     * Clears the current buddy list, sorts names and then re-populates the listbox with names
     * from both buddy lists. Also clears the currently selected buddy in currentBuddy.
     */
    void FillBuddyList();

    /**
     * Verifies that the alias is unique
     * @return True if the alias is unique
     */
    bool IsUniqueAlias(const csString & alias) const;

};


CREATE_PAWS_FACTORY( pawsBuddyWindow );
#endif    
