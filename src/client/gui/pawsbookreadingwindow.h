/*
 * pawsbookreadingwindow.h - Author: Daniel Fryer, based on work by Andrew Craig
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

#ifndef PAWS_BOOK_READING_WINDOW_HEADER
#define PAWS_BOOK_READING_WINDOW_HEADER

#include "paws/pawswidget.h"

class pawsTextBox;
class pawsMultiLineTextBox;

/** A window that shows the description of an item.
 */
class pawsBookReadingWindow : public pawsWidget, public psClientNetSubscriber
{
public:
//    pawsBookReadingWindow(){};
//    virtual ~pawsBookReadingWindow() {};

    bool PostSetup();

    void HandleMessage( MsgEntry* message );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

private:
    pawsTextBox*            name;
    pawsMultiLineTextBox*   description;
    pawsMultiLineTextBox*   descriptionCraft;
    pawsWidget*             writeButton;
    pawsWidget*             saveButton;
    
    bool shouldWrite;
    int slotID;
    int containerID;
	
	csString filenameSafe(const csString &original);
	bool isBadChar(char c);
};

CREATE_PAWS_FACTORY( pawsBookReadingWindow );


#endif 


