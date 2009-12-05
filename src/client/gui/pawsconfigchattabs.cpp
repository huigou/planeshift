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
    if (!LoadFromFile("configchattabs.xml"))
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
    if ((ichat = FindCheckbox("ichat")) == NULL)
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
    return true;
}

bool pawsConfigChatTabs::LoadConfig()
{
    chatWindow->LoadChatSettings();

    ChatSettings settings = chatWindow->GetSettings();
    
    isysbase->SetState(false);
    ichat->SetState(false);
	inpc->SetState(false);
	itells->SetState(false);
	iguild->SetState(false);
	igroup->SetState(false);
	iauction->SetState(false);
	isys->SetState(false);
	ihelp->SetState(false);

    csArray<csString> allMainBindings = settings.bindings.GetAll("subMainText");
    for(size_t i = 0; i < allMainBindings.GetSize(); i++)
    {
    	if(allMainBindings[i] == "CHAT_SYSTEM_BASE")
    		isysbase->SetState(true);
       	if(allMainBindings[i] == "CHAT_CHANNEL1")
    		ichat->SetState(true);
    	if(allMainBindings[i] == "CHAT_NPC")
    		inpc->SetState(true);
    	if(allMainBindings[i] == "CHAT_TELL")
    		itells->SetState(true);
    	if(allMainBindings[i] == "CHAT_GUILD")
    		iguild->SetState(true);
    	if(allMainBindings[i] == "CHAT_GROUP")
    		igroup->SetState(true);
    	if(allMainBindings[i] == "CHAT_AUCTION")
    		iauction->SetState(true);
		if(allMainBindings[i] == "CHAT_SYSTEM")
			isys->SetState(true);
		if(allMainBindings[i] == "CHAT_ADVICE")
			ihelp->SetState(true);
    }
	
    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChatTabs::SaveConfig()
{
    ChatSettings& settings = chatWindow->GetSettings();
    
    settings.bindings.Delete("subMainText", "CHAT_SYSTEM_BASE");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL1");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL2");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL3");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL4");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL5");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL6");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL7");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL8");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL9");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL10");
    settings.bindings.Delete("subMainText", "CHAT_NPC");
	settings.bindings.Delete("subMainText", "CHAT_NPC_ME");
	settings.bindings.Delete("subMainText", "CHAT_NPC_MY");
	settings.bindings.Delete("subMainText", "CHAT_NPC_NARRATE");
	settings.bindings.Delete("subMainText", "CHAT_NPCINTERNAL");
	settings.bindings.Delete("subMainText", "CHAT_TELL");
	settings.bindings.Delete("subMainText", "CHAT_TELLSELF");
	settings.bindings.Delete("subMainText", "CHAT_GUILD");
	settings.bindings.Delete("subMainText", "CHAT_GROUP");
	settings.bindings.Delete("subMainText", "CHAT_AUCTION");
	settings.bindings.Delete("subMainText", "CHAT_SYSTEM");
	settings.bindings.Delete("subMainText", "CHAT_ADVICE");
    
    if(isysbase->GetState())
    {
    	settings.bindings.Put("subMainText", "CHAT_SYSTEM_BASE");
    }
    if(ichat->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_CHANNEL1");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL2");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL3");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL4");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL5");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL6");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL7");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL8");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL9");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL10");
    }
    if(inpc->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_NPC");
		settings.bindings.Put("subMainText", "CHAT_NPC_ME");
		settings.bindings.Put("subMainText", "CHAT_NPC_MY");
		settings.bindings.Put("subMainText", "CHAT_NPC_NARRATE");
		settings.bindings.Put("subMainText", "CHAT_NPCINTERNAL");
	}
    if(itells->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_TELL");
		settings.bindings.Put("subMainText", "CHAT_TELLSELF");
	}
    if(iguild->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_GUILD");
	}
    if(igroup->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_GROUP");
	}
    if(iauction->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_AUCTION");
	}
    if(isys->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_SYSTEM");
	}
    if(ihelp->GetState())
	{
		settings.bindings.Put("subMainText", "CHAT_ADVICE");
	}

    chatWindow->SetSettings(settings);

    // Save to file
    chatWindow->SaveChatSettings();
    
    // Apply settings
    chatWindow->LoadChatSettings();

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
