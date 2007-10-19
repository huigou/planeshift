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

    isysbase = NULL;
    inpc = NULL;
    itells = NULL;
    iguild = NULL;
    igroup = NULL;
    iauction = NULL;
    isys = NULL;
    ihelp = NULL;

    fmain = NULL;
    fnpc = NULL;
    ftells = NULL;
    fguild = NULL;
    fgroup = NULL;
    fauction = NULL;
    fsys = NULL;
    fhelp = NULL;

	fcmain = NULL;
    fcnpc = NULL;
    fctells = NULL;
    fcguild = NULL;
    fcgroup = NULL;
    fcauction = NULL;
    fcsys = NULL;
    fchelp = NULL;

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

    // Find widgets
    if ((isysbase = FindCheckbox("isysbase")) == NULL)
        return false;
    if ((inpc = FindCheckbox("inpc")) == NULL)
        return false;
    if ((itells = FindCheckbox("itells")) == NULL)
        return false;
    if ((iguild = FindCheckbox("iguild")) == NULL)
        return false;
    if ((igroup = FindCheckbox("igroup")) == NULL)
        return false;
    if ((iauction = FindCheckbox("iauction")) == NULL)
        return false;
    if ((isys = FindCheckbox("isys")) == NULL)
        return false;
    if ((ihelp = FindCheckbox("ihelp")) == NULL)
        return false;

    if ((fmain = FindCheckbox("fmain")) == NULL)
        return false;
    if ((fnpc = FindCheckbox("fnpc")) == NULL)
        return false;
    if ((ftells = FindCheckbox("ftells")) == NULL)
        return false;
    if ((fguild = FindCheckbox("fguild")) == NULL)
        return false;
    if ((fgroup = FindCheckbox("fgroup")) == NULL)
        return false;
    if ((fauction = FindCheckbox("fauction")) == NULL)
        return false;
    if ((fsys = FindCheckbox("fsys")) == NULL)
        return false;
    if ((fhelp = FindCheckbox("fhelp")) == NULL)
        return false;

	if ((fcmain = FindCheckbox("fcmain")) == NULL)
	{
       return false;
	}
    if ((fcnpc = FindCheckbox("fcnpc")) == NULL)
	{
        return false;
	}
    if ((fctells = FindCheckbox("fctells")) == NULL)
	{
        return false;
	}
    if ((fcguild = FindCheckbox("fcguild")) == NULL)
	{
        return false;
	}
    if ((fcgroup = FindCheckbox("fcgroup")) == NULL)
	{
        return false;
	}
    if ((fcauction = FindCheckbox("fcauction")) == NULL)
	{
        return false;
	}
    if ((fcsys = FindCheckbox("fcsys")) == NULL)
	{
        return false;
	}
    if ((fchelp = FindCheckbox("fchelp")) == NULL)
	{
        return false;
	}



    return true;
}

bool pawsConfigChatTabs::LoadConfig()
{
    chatWindow->LoadChatSettings();

    ChatSettings settings = chatWindow->GetSettings();

    isysbase->SetState(settings.systemBaseIncluded);
    inpc->SetState(settings.npcIncluded);
    itells->SetState(settings.tellIncluded);
    iguild->SetState(settings.guildIncluded);
    igroup->SetState(settings.groupIncluded);
    iauction->SetState(settings.auctionIncluded);
    isys->SetState(settings.systemIncluded);
    ihelp->SetState(settings.helpIncluded);

    fmain->SetState(settings.mainFlashing);
    fnpc->SetState(settings.npcFlashing);
    ftells->SetState(settings.tellFlashing);
    fguild->SetState(settings.guildFlashing);
    fgroup->SetState(settings.groupFlashing);
    fauction->SetState(settings.auctionFlashing);
    fsys->SetState(settings.systemFlashing);
    fhelp->SetState(settings.helpFlashing);

    fcmain->SetState(settings.maincFlashing);
    fcnpc->SetState(settings.npccFlashing);
    fctells->SetState(settings.tellcFlashing);
    fcguild->SetState(settings.guildcFlashing);
    fcgroup->SetState(settings.groupcFlashing);
    fcauction->SetState(settings.auctioncFlashing);
    fcsys->SetState(settings.systemcFlashing);
    fchelp->SetState(settings.helpcFlashing);



    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChatTabs::SaveConfig()
{
    ChatSettings settings = chatWindow->GetSettings();

    settings.systemBaseIncluded = isysbase->GetState();
    settings.npcIncluded = inpc->GetState();
    settings.tellIncluded = itells->GetState();
    settings.guildIncluded = iguild->GetState();
    settings.groupIncluded = igroup->GetState();
    settings.auctionIncluded = iauction->GetState();
    settings.systemIncluded = isys->GetState();
    settings.helpIncluded = ihelp->GetState();

    settings.mainFlashing = fmain->GetState();
    settings.npcFlashing = fnpc->GetState();
    settings.tellFlashing = ftells->GetState();
    settings.guildFlashing = fguild->GetState();
    settings.groupFlashing = fgroup->GetState();
    settings.auctionFlashing = fauction->GetState();
    settings.systemFlashing = fsys->GetState();
    settings.helpFlashing = fhelp->GetState();

	settings.maincFlashing = fcmain->GetState();
    settings.npccFlashing = fcnpc->GetState();
    settings.tellcFlashing = fctells->GetState();
    settings.guildcFlashing = fcguild->GetState();
    settings.groupcFlashing = fcgroup->GetState();
    settings.auctioncFlashing = fcauction->GetState();
    settings.systemcFlashing = fcsys->GetState();
    settings.helpcFlashing = fchelp->GetState();


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
