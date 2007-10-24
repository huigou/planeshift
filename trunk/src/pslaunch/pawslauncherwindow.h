/*
* pawslauncherwindow.h - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PAWS_LAUNCHER_WINDOW_H__
#define __PAWS_LAUNCHER_WINDOW_H__

#include "pslaunch.h"
#include "paws/pawswidget.h"

class pawsLauncherWindow : public pawsWidget
{
public:
    pawsLauncherWindow() {}
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget );
    bool PostSetup();
private:
    pawsButton* quit;
    pawsButton* launchClient;
};

CREATE_PAWS_FACTORY( pawsLauncherWindow );

#endif // __PAWS_LAUNCHER_WINDOW_H__
