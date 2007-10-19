/*
 * pawsconfigpvp.cpp - Author: Christian Svensson
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "../globals.h"
#include "pscelclient.h"

// PAWS INCLUDES
#include "gui/chatwindow.h"
#include "pawsconfigchat.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "paws/pawstextbox.h"
#include "paws/pawsradio.h"

#define SET_CHAT_VALUE(x) settings.x##Color = PawsManager::GetSingleton().GetGraphics2D()->FindRGB(atoi(x##R->GetText()),atoi(x##G->GetText()),atoi(x##B->GetText()));
#define SET_WIDGET_VALUE(x) x##R->SetText(r); x##G->SetText(g); x##B->SetText(b);

pawsConfigChat::pawsConfigChat()
{
    chatWindow=NULL;
    systemR = NULL;
    systemG = NULL;
    systemB = NULL;
    adminR = NULL;
    adminG = NULL;
    adminB = NULL;
    playerR = NULL;
    playerG = NULL;
    playerB = NULL;
    chatR = NULL;
    chatG = NULL;
    chatB = NULL;
    tellR = NULL;
    tellG = NULL;
    tellB = NULL;
    shoutR = NULL;
    shoutG = NULL;
    shoutB = NULL;
    guildR = NULL;
    guildG = NULL;
    guildB = NULL;
    yourR = NULL;
    yourG = NULL;
    yourB = NULL;
    groupR = NULL;
    groupG = NULL;
    groupB = NULL;
    auctionR = NULL;
    auctionG = NULL;
    auctionB = NULL;
    helpR = NULL;
    helpG = NULL;
    helpB = NULL;
    loose = NULL;
    for (int i = 0; i < CHAT_NLOG; i++)
        logEnable[i] = NULL;
    badwordsIncoming = NULL;
    badwordsOutgoing = NULL;
    echoScreenInSystem = NULL;
}

bool pawsConfigChat::Initialize()
{
    if ( ! LoadFromFile("data/gui/configchat.xml"))
        return false;
       
    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    return true;
}

bool pawsConfigChat::PostSetup()
{

    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }

    systemR = (pawsEditTextBox*)FindWidget("systemtextr");
    systemG = (pawsEditTextBox*)FindWidget("systemtextg");
    systemB = (pawsEditTextBox*)FindWidget("systemtextb");
    adminR = (pawsEditTextBox*)FindWidget("admintextr");
    adminG = (pawsEditTextBox*)FindWidget("admintextg");
    adminB = (pawsEditTextBox*)FindWidget("admintextb");
    playerR = (pawsEditTextBox*)FindWidget("playernametextr");
    playerG = (pawsEditTextBox*)FindWidget("playernametextg");
    playerB = (pawsEditTextBox*)FindWidget("playernametextb");
    chatR = (pawsEditTextBox*)FindWidget("chattextr");
    chatG = (pawsEditTextBox*)FindWidget("chattextg");
    chatB = (pawsEditTextBox*)FindWidget("chattextb");
    tellR = (pawsEditTextBox*)FindWidget("telltextr");
    tellG = (pawsEditTextBox*)FindWidget("telltextg");
    tellB = (pawsEditTextBox*)FindWidget("telltextb");
    shoutR = (pawsEditTextBox*)FindWidget("shouttextr");
    shoutG = (pawsEditTextBox*)FindWidget("shouttextg");
    shoutB = (pawsEditTextBox*)FindWidget("shouttextb");
    guildR = (pawsEditTextBox*)FindWidget("guildtextr");
    guildG = (pawsEditTextBox*)FindWidget("guildtextg");
    guildB = (pawsEditTextBox*)FindWidget("guildtextb");
    yourR = (pawsEditTextBox*)FindWidget("yourtextr");
    yourG = (pawsEditTextBox*)FindWidget("yourtextg");
    yourB = (pawsEditTextBox*)FindWidget("yourtextb");
    groupR = (pawsEditTextBox*)FindWidget("grouptextr");
    groupG = (pawsEditTextBox*)FindWidget("grouptextg");
    groupB = (pawsEditTextBox*)FindWidget("grouptextb");
    auctionR = (pawsEditTextBox*)FindWidget("auctiontextr");
    auctionG = (pawsEditTextBox*)FindWidget("auctiontextg");
    auctionB = (pawsEditTextBox*)FindWidget("auctiontextb");
    helpR = (pawsEditTextBox*)FindWidget("helptextr");
    helpG = (pawsEditTextBox*)FindWidget("helptextg");
    helpB = (pawsEditTextBox*)FindWidget("helptextb");
    loose = (pawsCheckBox*)FindWidget("loosefocus");
    for (int i = 0; i < CHAT_NLOG; i++)
        logEnable[i] = (pawsCheckBox*)FindWidget(logWidgetName[i]);
    badwordsIncoming = (pawsCheckBox*)FindWidget("badwordsincoming");
    badwordsOutgoing = (pawsCheckBox*)FindWidget("badwordsoutgoing");
    selectTabStyleGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("selecttabstyle"));
    echoScreenInSystem = (pawsCheckBox*)FindWidget("echoscreeninsystem");
                
    return true;
}

bool pawsConfigChat::LoadConfig()
{

    // Need to reload settings
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,chatNode, colorNode, optionNode, filtersNode, badWordsNode;
    csString option;
    
    csString fileName = CONFIG_CHAT_FILE_NAME;
    if (!psengine->GetVFS()->Exists(fileName))
    {
        fileName = CONFIG_CHAT_FILE_NAME_DEF;
    }

    doc = ParseFile(psengine->GetObjectRegistry(), fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("chat.xml has no XML root");
        return false;
    }
    chatNode = root->GetNode("chat");
    if (chatNode == NULL)
    {
        Error1("chat.xml has no <chat> tag");
        return false;
    }
    
    // Load options such as loose after sending
    optionNode = chatNode->GetNode("chatoptions");
    if (optionNode != NULL)
    {
        csRef<iDocumentNodeIterator> oNodes = optionNode->GetNodes();
        while(oNodes->HasNext())
        {
            csRef<iDocumentNode> option = oNodes->Next();
            
            csString nodeName (option->GetValue());
            if (nodeName == "loose")
                loose->SetState((option->GetAttributeValueAsInt("value")? true : false));
            else if (nodeName == "selecttabstyle")
                {
                 switch (option->GetAttributeValueAsInt("value"))
                 {
                  case 0: selectTabStyleGroup->SetActive("none");
                          break;
                  case 1: selectTabStyleGroup->SetActive("fitting");
                          break;
                  case 2: selectTabStyleGroup->SetActive("alltext");
                          break;
                 }
                }
            else if (nodeName == "echoscreeninsystem")
                echoScreenInSystem->SetState(option->GetAttributeValueAsInt("value") ? true : false);
            else 
            {
                for (int i = 0; i < CHAT_NLOG; i++)
                    if (nodeName == logWidgetName[i])    // Take the same name for simplicity
                        logEnable[i]->SetState(option->GetAttributeValueAsInt("value") ? true : false);
            }
        }
    }

    // Load colors
    colorNode = chatNode->GetNode("chatcolors");
    if (colorNode != NULL)
    {
        csRef<iDocumentNodeIterator> cNodes = colorNode->GetNodes();
        while(cNodes->HasNext())
        {
            csRef<iDocumentNode> color = cNodes->Next();
            
            csString r,g,b;
            r.Format("%d", color->GetAttributeValueAsInt( "r" ));            
            g.Format("%d", color->GetAttributeValueAsInt( "g" ));            
            b.Format("%d", color->GetAttributeValueAsInt( "b" ));            
            
            csString nodeName ( color->GetValue() );
            if ( nodeName == "systemtext" ) 
            {
                SET_WIDGET_VALUE(system);
            }
            if ( nodeName == "admintext"  ) 
            {
                SET_WIDGET_VALUE(admin);
            }
            if ( nodeName == "playernametext" ) 
            {
                SET_WIDGET_VALUE(player);
            }
            if ( nodeName == "chattext") 
            {
                SET_WIDGET_VALUE(chat);
            }
            if ( nodeName == "telltext") 
            {
                SET_WIDGET_VALUE(tell);
            }
            if ( nodeName == "shouttext") 
            {
                SET_WIDGET_VALUE(shout);
            }
            if ( nodeName == "guildtext") 
            {
                SET_WIDGET_VALUE(guild);
            }
            if ( nodeName == "yourtext") 
            {
                SET_WIDGET_VALUE(your);
            }
            if ( nodeName == "grouptext") 
            {
                SET_WIDGET_VALUE(group);
            }
            if ( nodeName == "auctiontext") 
            {
                SET_WIDGET_VALUE(auction);
            }
            if ( nodeName == "helptext") 
            {
                SET_WIDGET_VALUE(help);
            }
         }
    }

    // load bad words options
    filtersNode = chatNode->GetNode("filters");
    if (filtersNode != NULL) 
    {
        badWordsNode = filtersNode->GetNode("badwords");
        if (badWordsNode != NULL) 
        {
            badwordsIncoming->SetState((bool)badWordsNode->GetAttributeValueAsBool("incoming"));
            badwordsOutgoing->SetState((bool)badWordsNode->GetAttributeValueAsBool("outgoing"));
        }
    }

    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChat::SaveConfig()
{
    int i;
    
    ChatSettings settings = chatWindow->GetSettings();
    SET_CHAT_VALUE(admin);
    SET_CHAT_VALUE(system);
    SET_CHAT_VALUE(player);
    SET_CHAT_VALUE(auction);
    SET_CHAT_VALUE(help);
    SET_CHAT_VALUE(group);
    SET_CHAT_VALUE(your);
    SET_CHAT_VALUE(guild);
    SET_CHAT_VALUE(shout);
    SET_CHAT_VALUE(tell);
    SET_CHAT_VALUE(chat);
    settings.looseFocusOnSend = loose->GetState();
    for (i = 0; i < CHAT_NLOG; i++)
        settings.logChannel[i] = logEnable[i]->GetState();
    settings.enableBadWordsFilterIncoming = badwordsIncoming->GetState();
    settings.enableBadWordsFilterOutgoing = badwordsOutgoing->GetState();    

    if (selectTabStyleGroup->GetActive() == "none") settings.selectTabStyle = 0;
    else if (selectTabStyleGroup->GetActive() == "fitting") settings.selectTabStyle = 1;
    else settings.selectTabStyle = 2;

    settings.echoScreenInSystem = echoScreenInSystem->GetState();

    chatWindow->SetSettings(settings);

    // Save to file
    chatWindow->SaveChatSettings();
    
    return true;
}

void pawsConfigChat::Show()
{
    // Check boxes doesn't send OnChange :(
    dirty = true;
    pawsWidget::Show();
}

void pawsConfigChat::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

bool pawsConfigChat::OnChange(pawsWidget * widget)
{
    return true;
}
