/*
* bartender.h - Author: Andrew Dai
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

#ifndef PAWS_BARTENDER_WINDOW
#define PAWS_BARTENDER_WINDOW

//=============================================================================
// Library Includes
//=============================================================================
#include "paws/pawswidget.h"

//=============================================================================
// Forward Declarations
//=============================================================================

//=============================================================================
// Defines
//=============================================================================


//=============================================================================
// Classes
//=============================================================================

/**
 *
 */
class pawsBartenderWindow :public pawsWidget
{
public:
    ///Constructor
    pawsBartenderWindow();
    ///Prepares the widget by loading the commands
    bool PostSetup();
    ///Destructor. Saves the widget commands
    virtual ~pawsBartenderWindow();

protected:
};
CREATE_PAWS_FACTORY( pawsBartenderWindow );
#endif
