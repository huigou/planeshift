/*
* pawsconfigchattabs.cpp - Author: Enar Vaikene
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "globals.h"

#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigchattabs.h"


pawsConfigChatTabs::pawsConfigChatTabs()
{
    chatWindow = NULL;

}

bool pawsConfigChatTabs::Initialize()
{
    if (!LoadFromFile("data/gui/configchattabs.xml"))
        return false;

    return true;
}

bool pawsConfigChatTabs::PostSetup()
{
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }

    return true;
}

bool pawsConfigChatTabs::LoadConfig()
{
    chatWindow->LoadChatSettings();

    ChatSettings settings = chatWindow->GetSettings();

    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChatTabs::SaveConfig()
{
    ChatSettings settings = chatWindow->GetSettings();

    chatWindow->SetSettings(settings);

    // Save to file
    chatWindow->SaveChatSettings();

    return true;
}

void pawsConfigChatTabs::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

pawsCheckBox *pawsConfigChatTabs::FindCheckbox(const char *name)
{
    pawsCheckBox *t = dynamic_cast<pawsCheckBox *>(FindWidget(name));
    if (!t)
        Error2("Couldn't find widget %s", name);

    return t;
}
