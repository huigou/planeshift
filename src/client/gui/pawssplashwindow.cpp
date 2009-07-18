/*
 * pawsSplashWindow.cpp - Author: Andrew Craig
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
#include <csutil/sysfunc.h>

#include "pawssplashwindow.h"
#include "iclient/isoundmngr.h"
#include "paws/pawsmanager.h"

#include "paws/pawsprogressbar.h"
#include "globals.h"

pawsSplashWindow::pawsSplashWindow()
{
    firstDraw = true;     
}

pawsSplashWindow::~pawsSplashWindow()
{
}

bool pawsSplashWindow::PostSetup()
{
    return true;    
}


void pawsSplashWindow::Draw()
{
    pawsWidget::Draw();
    if ( firstDraw )    
    {        
        firstDraw = false;
        return;
    }
    
    psengine->Initialize(1);
    PawsManager::GetSingleton().LoadWidget("loginwindow.xml");
    Hide();
    PawsManager::GetSingleton().GetMouse()->ChangeImage("Standard Mouse Pointer");
    PawsManager::GetSingleton().GetMouse()->Hide(false);
    delete this;
}

