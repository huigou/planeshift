/*
 * pawsconfigpopup.cpp
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//==============================================================================
// CS INCLUDES
//==============================================================================
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

//==============================================================================
// COMMON INCLUDES
//==============================================================================
#include "util/log.h"

//==============================================================================
// CLIENT INCLUDES
//==============================================================================
#include "../globals.h"
#include "pscelclient.h"

//==============================================================================
// PAWS INCLUDES
//==============================================================================
#include "gui/pawsactivemagicwindow.h"
#include "pawsconfigpopup.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"

pawsConfigPopup::pawsConfigPopup(void)
{
}

bool pawsConfigPopup::Initialize()
{
    if (!LoadFromFile("configpopup.xml"))
        return false;
    
    return true;
}

bool pawsConfigPopup::PostSetup()
{
    magicWindow = (pawsActiveMagicWindow*)PawsManager::GetSingleton().FindWidget("ActiveMagicWindow");
    if(!magicWindow)
    {
        Error1("Couldn't find ActiveMagicWindow!");
        return false;
    }

    showActiveMagicConfig = (pawsCheckBox*)FindWidget("ShowActiveMagicWindowConfig");
    if (!showActiveMagicConfig)
        return false;
                
    return true;
}

bool pawsConfigPopup::LoadConfig()
{
    if (!magicWindow->LoadSetting())
        return false;

    showActiveMagicConfig->SetState(!magicWindow->showWindow->GetState());

    dirty = true;

    return true;
}

bool pawsConfigPopup::SaveConfig()
{
    magicWindow->showWindow->SetState(!showActiveMagicConfig->GetState());

    magicWindow->SaveSetting();

    return true;
}


void pawsConfigPopup::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_ACTIVEMAGIC_FILE_NAME);
    LoadConfig();
}
