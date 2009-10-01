/*
 * pawsactivemagicwindow.h
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

#ifndef PAWS_ACTIVEMAGIC_WINDOW_HEADER
#define PAWS_ACTIVEMAGIC_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsTextBox;
class pawsListBox;
class pawsMessageTextBox;
class pawsProgressBar;

#include "net/cmdbase.h"
#include "net/subscriber.h"
#include "gui/pawscontrolwindow.h"
/** This handles all the details about how the spell cancel works.
 */
class pawsActiveMagicWindow : public pawsWidget, public psClientNetSubscriber
{
public:

    virtual ~pawsActiveMagicWindow() {};

    bool PostSetup();

    void HandleMessage( MsgEntry* me );

    virtual void Close();

private:
    pawsListBox*    buffCategories;
    pawsListBox*    debuffCategories;
};

CREATE_PAWS_FACTORY( pawsActiveMagicWindow );

#endif 
