/*
* chatwindow.cpp - Author: Andrew Craig
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

// chatwindow.cpp: implementation of the pawsChatWindow class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/util.h>
#include <csutil/xmltiny.h>

#include <iutil/evdefs.h>
#include <iutil/databuff.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "iclient/isoundmngr.h"

#include "paws/pawstextbox.h"
#include "paws/pawsprefmanager.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsborder.h"
#include "paws/pawstabwindow.h"

#include "gui/pawscontrolwindow.h"

#include "net/message.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

#include "util/strutil.h"
#include "util/fileutil.h"
#include "util/log.h"
#include "util/localization.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscelclient.h"
#include "chatwindow.h"

const char *logWidgetName[CHAT_NLOG] = {
    "logAllChat",
    "logSystemChat"
};

const char *logFileName[CHAT_NLOG] = {
    "chat.txt",
    "system.txt"
};

// Must match the chat types in messages.h
const char *CHAT_TYPES[] = {
		"CHAT_SYSTEM",
		"CHAT_COMBAT",
		"CHAT_SAY",
		"CHAT_TELL",
		"CHAT_GROUP",
		"CHAT_GUILD",
        "CHAT_ALLIANCE",
		"CHAT_AUCTION",
		"CHAT_SHOUT",
		"CHAT_CHANNEL",
		"CHAT_TELLSELF",
		"CHAT_REPORT",
		"CHAT_ADVISOR",
		"CHAT_ADVICE",
		"CHAT_ADVICE_LIST",
		"CHAT_SERVER_TELL",      ///< this tell came from the server, not from another player
		"CHAT_GM",
		"CHAT_SERVER_INFO",
		"CHAT_NPC",
		"CHAT_NPCINTERNAL",
		"CHAT_SYSTEM_BASE",      ///< System messages that are also shown on the "Main" tab
		"CHAT_PET_ACTION",
		"CHAT_NPC_ME",
		"CHAT_NPC_MY",
		"CHAT_NPC_NARRATE"
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsChatWindow::pawsChatWindow()
    : psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
    systemTriggers.Push("server admin");
    systemTriggers.Push("gm warning");

    havePlayerName = false;

    replyCount = 0;

    chatHistory = new pawsChatHistory();
    awayText.Clear();
    soundmgr = NULL;

    settings.meFilters = 0;
    settings.vicinityFilters = 0;
    settings.echoScreenInSystem = false;
    settings.npcColor = graphics2D->FindRGB( 255, 0, 255 );
    int white = graphics2D->FindRGB(255, 255, 255);
    // Default to white
    settings.playerColor = white;
    settings.chatColor = white;
    settings.systemColor = white;
    settings.adminColor = white;
    settings.tellColor = white;
    settings.guildColor = white;
    settings.allianceColor = white;
    settings.shoutColor = white;
    settings.channelColor = white;
    settings.gmColor = white;
    settings.yourColor = white;
    settings.groupColor = white;
    settings.auctionColor = white;
    settings.helpColor = white;
    settings.mainBrackets = true;
    settings.yourColorMix = true;
    settings.joindefaultchannel = true;


    for (int i = 0; i < CHAT_NLOG; i++)
        logFile[i] = NULL;
    
    channels.SetSize(10, 0);
}

pawsChatWindow::~pawsChatWindow()
{
    msgqueue->Unsubscribe(this, MSGTYPE_CHAT);
    msgqueue->Unsubscribe(this, MSGTYPE_CHANNEL_JOINED);
    msgqueue->Unsubscribe(this, MSGTYPE_SYSTEM);

    delete chatHistory;
    for (int i = 0; i < CHAT_NLOG; i++)
        logFile[i].Invalidate();
}

bool pawsChatWindow::PostSetup()
{

    if ( !psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler()) )
        return false;

    soundmgr = psengine->GetSoundManager();

    msgqueue->Subscribe(this, MSGTYPE_CHAT);
    msgqueue->Subscribe(this, MSGTYPE_CHANNEL_JOINED);
    msgqueue->Subscribe(this, MSGTYPE_SYSTEM);

    SubscribeCommands();

    tabs = dynamic_cast<pawsTabWindow*>(FindWidget("Chat Tabs"));

    inputText   = dynamic_cast<pawsEditTextBox*>   (FindWidget("InputText"));

    // Load the settings
    LoadChatSettings();

    IgnoredList = (pawsIgnoreWindow*)PawsManager::GetSingleton().FindWidget("IgnoreWindow");
    if ( !IgnoredList )
    {
        Error1("ChatWindow failed because IgnoredList window was not found.");
        return false;
    }
    ReplayMessages();
    if(settings.joindefaultchannel)
    	JoinChannel("Gossip");
    return true;
}

void pawsChatWindow::LoadChatSettings()
{
    // Get some default colours here and other options.

    // XML parsing time!
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,chatNode, colorNode, optionNode, filtersNode, msgFiltersNode,
                         mainTabNode, flashingNode, flashingOnCharNode;
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
        return;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("chat.xml has no XML root");
        return;
    }
    chatNode = root->GetNode("chat");
    if (chatNode == NULL)
    {
        Error1("chat.xml has no <chat> tag");
        return;
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
                settings.looseFocusOnSend = option->GetAttributeValueAsBool("value", false);

            else if (nodeName == "selecttabstyle")
                settings.selectTabStyle = (int)option->GetAttributeValueAsInt("value");
            else if (nodeName == "echoscreeninsystem")
                settings.echoScreenInSystem = option->GetAttributeValueAsBool("value", true);
            else if (nodeName == "mainbrackets")
                settings.mainBrackets = option->GetAttributeValueAsBool("value", true);
            else if (nodeName == "yourcolormix")
                settings.yourColorMix = option->GetAttributeValueAsBool("value", true);
            else if (nodeName == "joindefaultchannel")
            	settings.joindefaultchannel = option->GetAttributeValueAsBool("value", true);
            else
            {
                for (int i = 0; i < CHAT_NLOG; i++)
                    if (nodeName == logWidgetName[i])   // Take the same name for simplicity
                        settings.logChannel[i] = option->GetAttributeValueAsBool("value", false);
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

            int r = color->GetAttributeValueAsInt( "r" );
            int g = color->GetAttributeValueAsInt( "g" );
            int b = color->GetAttributeValueAsInt( "b" );

            int col = graphics2D->FindRGB( r, g, b );

            csString nodeName ( color->GetValue() );
            if ( nodeName == "systemtext" ) settings.systemColor = col;
            if ( nodeName == "admintext"  ) settings.adminColor = col;
            if ( nodeName == "playernametext" ) settings.playerColor = col;
            if ( nodeName == "chattext") settings.chatColor = col;
            if ( nodeName == "npctext") settings.npcColor = col;
            if ( nodeName == "telltext") settings.tellColor = col;
            if ( nodeName == "shouttext") settings.shoutColor = col;
            if ( nodeName == "channeltext") settings.channelColor = col;
            if ( nodeName == "gmtext") settings.gmColor = col;
            if ( nodeName == "guildtext") settings.guildColor = col;
            if ( nodeName == "alliancetext") settings.allianceColor = col;
            if ( nodeName == "yourtext") settings.yourColor = col;
            if ( nodeName == "grouptext") settings.groupColor = col;
            if ( nodeName == "auctiontext") settings.auctionColor = col;
            if ( nodeName == "helptext") settings.helpColor = col;
        }
    }

    // Load filters
    filtersNode = chatNode->GetNode("filters");
    settings.meFilters = 0;
    settings.vicinityFilters = 0;

    if (filtersNode != NULL)
    {
        csRef<iDocumentNode> badWordsNode = filtersNode->GetNode("badwords");
        if (badWordsNode == NULL) {
            settings.enableBadWordsFilterIncoming = settings.enableBadWordsFilterOutgoing = false;
        }
        else
        {
            settings.enableBadWordsFilterIncoming = badWordsNode->GetAttributeValueAsBool("incoming");
            settings.enableBadWordsFilterOutgoing = badWordsNode->GetAttributeValueAsBool("outgoing");
            csRef<iDocumentNodeIterator> oNodes = badWordsNode->GetNodes();
            while(oNodes->HasNext())
            {
                csRef<iDocumentNode> option = oNodes->Next();
                if (option->GetType() == CS_NODE_TEXT)
                {
                    csString s = option->GetValue();
                    for (char *word = strtok((char*)s.GetData(), " \r\n\t"); word; word = strtok(NULL, " \r\n\t"))
                    {
                        if ( settings.badWords.Find(word)==csArrayItemNotFound ) {
                            settings.badWords.Push(word);
                            settings.goodWords.Push("");
                        }
                    }
                }
                else if (option->GetType() == CS_NODE_ELEMENT && csString(option->GetValue()) == "replace")
                {
                    csString bad = option->GetAttributeValue("bad");
                    csString good = option->GetAttributeValue("good");
                    if ( settings.badWords.Find(bad)==csArrayItemNotFound ) {
                        settings.badWords.Push(bad);
                        settings.goodWords.Push(good);
                    }
                }
            }
        }
    }

    if ( settings.badWords.GetSize() != settings.goodWords.GetSize() )
    {
        Error1("Mismatch in the good/bad word chat filter. Different lengths");
        CS_ASSERT( false );
        return;
    }



    // Load message filters
    msgFiltersNode = chatNode->GetNode("msgfilters");
    if (msgFiltersNode != NULL)
    {
        csRef<iDocumentNodeIterator> fNodes = msgFiltersNode->GetNodes();
        while(fNodes->HasNext())
        {
            csRef<iDocumentNode> filter = fNodes->Next();

            // Extract the info
            csString range = filter->GetValue();
            csString type =  filter->GetAttributeValue("type");
            CHAT_COMBAT_FILTERS fType;

            bool value = filter->GetAttributeValueAsBool("value",true);

            if(type == "BLO")
                fType = COMBAT_BLOCKED;
            else if(type == "SUC")
                fType = COMBAT_SUCCEEDED;
            else if(type == "MIS")
                fType = COMBAT_MISSED;
            else if(type == "DOD")
                fType = COMBAT_DODGED;
            else if(type == "FAI")
                fType = COMBAT_FAILED;
            else // if(type == "STA")
                fType = COMBAT_STANCE;

            if(value)
            {
                if(range == "me")
                    settings.meFilters |= fType;
                else
                    settings.vicinityFilters |= fType;
            }
        }
    }
}

void pawsChatWindow::DetermineChatTabAndSelect(int chattype)
{
	csArray<iPAWSSubscriber*> subscribers = PawsManager::GetSingleton().ListSubscribers(CHAT_TYPES[chattype]);
	
	if(!tabs || subscribers.IsEmpty())
		return;
    
	PawsManager::GetSingleton().SetCurrentFocusedWidget( inputText );
	BringToTop( inputText );

	for(size_t i = 0; i < subscribers.GetSize(); i++)
	{
		pawsWidget* widget = dynamic_cast<pawsWidget*>(subscribers[i]);
		if(widget->IsVisible())
			return;
	}

	pawsWidget* newTab = dynamic_cast<pawsWidget*>(subscribers[0]);
	tabs->SetTab(newTab->GetName());
}

const char* pawsChatWindow::HandleCommand( const char* cmd )
{
    WordArray words(cmd);
    csString pPerson;
    csString text;
    
    // Used to hold error message between calls
    static csString error;
    int chattype = 0;
    int hotkeyChannel = 0;

    if (words.GetCount()==0)
        return NULL;

    if (words[0].GetAt(0) != '/')
    {
        pPerson.Clear();
        words.GetTail(0,text);
        chattype = CHAT_SAY;
    }
    else
    {
        if (words.GetCount() == 1)
        {
        	error = PawsManager::GetSingleton().Translate("You must enter the text");
            return error;
        }

        
        if (words[0] == "/say" || words[0] == "/s")
        {
            pPerson.Clear();
            words.GetTail(1, text);
            chattype = CHAT_SAY;
            DetermineChatTabAndSelect(CHAT_SAY);
        }
        else if (words[0] == "/tellnpcinternal")
        {
            pPerson = psengine->GetMainPlayerName();
            size_t space = pPerson.FindFirst(' ');
            if (space != SIZET_NOT_FOUND)
                pPerson.DeleteAt(space,pPerson.Length()-space);
            words.GetTail(1, text);
            chattype = CHAT_NPCINTERNAL;
        }
        else if (words[0] == "/tellnpc")
        {
            pPerson.Clear();
            words.GetTail(1, text);
            chattype = CHAT_NPC;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_NPC);
        }
        else if (words[0] == "/report")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_REPORT;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_REPORT);
        }
        else if (words[0] == "/guild" || words[0] == "/g")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_GUILD;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_GUILD);
        }
        else if (words[0] == "/alliance" || words[0] == "/a")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_ALLIANCE;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_ALLIANCE);
        }
        else if (words[0] == "/shout" || words[0] == "/sh")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_SHOUT;
            DetermineChatTabAndSelect(CHAT_SHOUT);
        }
        else if (words[0] == "/1")
        {
        	pPerson.Clear();
        	words.GetTail(1,text);
        	chattype = CHAT_CHANNEL;
        	hotkeyChannel = 1;
        }
        else if (words[0] == "/2")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 2;
		}
        else if (words[0] == "/3")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 3;
		}
        else if (words[0] == "/4")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 4;
		}
        else if (words[0] == "/5")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 5;
		}
        else if (words[0] == "/6")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 6;
		}
        else if (words[0] == "/7")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 7;
		}
        else if (words[0] == "/8")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 8;
		}
        else if (words[0] == "/9")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 9;
		}
        else if (words[0] == "/10")
		{
			pPerson.Clear();
			words.GetTail(1,text);
			chattype = CHAT_CHANNEL;
			hotkeyChannel = 10;
		}
        else if (words[0] == "/group" || words[0] == "/gr")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_GROUP;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_GROUP);
        }
        else if (words[0] == "/tell" || words[0] == "/t")
        {
            pPerson = words[1];
            if (words.GetCount() == 2)
            {
                error = PawsManager::GetSingleton().Translate("You must enter the text");
                return error;
            }
            words.GetTail(2,text);
            chattype = CHAT_TELL;
            inputText->SetText(words[0] + " " + words[1] + " ");
            DetermineChatTabAndSelect(CHAT_TELL);
        }
        else if (words[0] == "/auction")
        {
            pPerson.Clear();
            words.GetTail(1,text);
            chattype = CHAT_AUCTION;
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_AUCTION);
        }
        else if (words[0] == "/mypet")
        {
            pPerson.Clear();
            chattype = CHAT_PET_ACTION;
            words.GetTail(1,text);
            inputText->SetText(words[0] + " ");
            DetermineChatTabAndSelect(CHAT_PET_ACTION);
        }
        else if (words[0] == "/me" || words[0] == "/my")
        {
            pPerson.Clear();
            csString chatType;
            if(tabs)
            	chatType = tabs->GetActiveTab()->GetName();

            csString allowedTabs;
            csString defaultButton("Main Button");

            if (chatType == "TellText")
            {
                int i = 0;
                while (!replyList[i].IsEmpty() && i <= 4)
                    i++;
                if (i)
                {
                    if (replyCount >= i)
                        replyCount = 0;
                    pPerson=replyList[replyCount].GetData();
                    replyCount++;
                }
                words.GetTail(0,text);
                chattype = CHAT_TELL;
                DetermineChatTabAndSelect(CHAT_TELL);
            }
            else if (chatType == "GuildText")
            {
                chattype = CHAT_GUILD;
                words.GetTail(0,text);
                DetermineChatTabAndSelect(CHAT_GUILD);
            }
            else if (chatType == "AllianceText")
            {
                chattype = CHAT_ALLIANCE;
                words.GetTail(0,text);
                DetermineChatTabAndSelect(CHAT_ALLIANCE);
            }
            else if (chatType == "GroupText")
            {
                chattype = CHAT_GROUP;
                words.GetTail(0,text);
                DetermineChatTabAndSelect(CHAT_GROUP);
            }
            else if (chatType == "AuctionText")
            {
                chattype = CHAT_AUCTION;
                words.GetTail(0,text);
                DetermineChatTabAndSelect(CHAT_AUCTION);
            }
            else // when in doubt, use the normal way
            {
                chattype = CHAT_SAY;
                words.GetTail(0,text);
                DetermineChatTabAndSelect(CHAT_SAY);
            }
        }
    }

    if (settings.enableBadWordsFilterOutgoing)
        BadWordsFilter(text);

    // If a character says his/her own name, presume it's an introduction
    csString name(psengine->GetMainPlayerName());
    name.Truncate(name.FindFirst(' '));
    name.Downcase();

    csString downtext(text);
    downtext.Downcase();
    if (chattype == CHAT_SAY && downtext.Find(name) != SIZET_NOT_FOUND)
    {
        psCharIntroduction introduce;
        msgqueue->SendMessage(introduce.msg);
    }
    int channelID = 0;
    if(chattype == CHAT_CHANNEL)
    {
    	channelID = channels[hotkeyChannel-1];
		if(channelID == 0)
		{
			ChatOutput("No channel assigned to that channel hotkey!");
			return NULL;
		}
    }
    psChatMessage chat(0, 0, pPerson.GetDataSafe(), 0, text.GetDataSafe(), chattype, false, channelID);
    if (chattype != CHAT_NPCINTERNAL)
        msgqueue->SendMessage(chat.msg);  // Send all chat msgs to server...
    else
        msgqueue->Publish(chat.msg);      // ...except for the display only ones.

    return NULL;
}

void pawsChatWindow::ReplayMessages()
{
	csString filename;
	filename.Format("/planeshift/userdata/logs/%s_%s",
						psengine->GetMainPlayerName(),
						logFileName[CHAT_LOG_ALL]);
	filename.ReplaceAll(" ", "_");
	
	char buf[1001];

	// Open file and seek to 1000 bytes from the end, unlikely to need anything earlier than that.
	csRef<iFile> file = psengine->GetVFS()->Open(filename, VFS_FILE_READ);
	if(!file.IsValid())
		return;
	size_t seekPos = 0;
	if(file->GetSize() > 1000)
		seekPos = file->GetSize() - 1000;
	file->SetPos(seekPos);
	size_t readLength = file->Read(buf, 1000);
	
	// At least 5 chars
	if(readLength < 5)
		return;
	
	buf[readLength] = '\0';
	
	// Find the last 10 lines
	int lines = 0;
	char* currentPos;
	csArray<const char*> line;
	
	for(currentPos = buf + readLength - 2; currentPos != buf && lines != 10; currentPos--)
	{
		if(*currentPos == '\n')
		{
			*currentPos = '\0';
			// Useful log messages are prepended with a ( time stamp
			if(*(currentPos + 1) == '(')
			{
				line.Push(currentPos + 1);
				lines++;
			}
		}
		if(*currentPos == '\r')
			*currentPos = '\0';
	}
	// Assume CHAT_SAY and place all in one chat type to ensure they end up in the same window
	PawsManager::GetSingleton().Publish(CHAT_TYPES[CHAT_SAY], "Replaying previous chat...", settings.systemColor );
	while(!line.IsEmpty())
		PawsManager::GetSingleton().Publish(CHAT_TYPES[CHAT_SAY], line.Pop(), settings.chatColor );
}

void pawsChatWindow::LogMessage(enum E_CHAT_LOG channel, const char* message, int type)
{
    if (settings.logChannel[channel])
    {
        if (!logFile[channel])
        {
        csString filename;
        filename.Format("/planeshift/userdata/logs/%s_%s",
                            psengine->GetMainPlayerName(),
                logFileName[channel]);
            filename.ReplaceAll(" ", "_");

        logFile[channel] = psengine->GetVFS()->Open(filename, VFS_FILE_APPEND);
            if (logFile[channel])
            {
                time_t aclock;
                struct tm *newtime;
                char buf[32];

                time(&aclock);
                newtime = localtime(&aclock);
                strftime(buf, 32, "%a %d-%b-%Y %H:%M:%S", newtime);
                csString buffer;
#ifdef _WIN32
                buffer.Format(
                    "================================================\r\n"
                    "%s %s\r\n"
                    "------------------------------------------------\r\n",
                    buf, psengine->GetMainPlayerName()
                    );
#else
                buffer.Format(
                    "================================================\n"
                    "%s %s\n"
                    "------------------------------------------------\n",
                    buf, psengine->GetMainPlayerName()
                    );
#endif
                logFile[channel]->Write(buffer.GetData(), buffer.Length());
            }
            else
            {
                // Trouble opening the log file
                Error2("Couldn't create chat log file '%s'.\n", logFileName[channel]);
            }
        }
        if (logFile[channel])
        {
            time_t aclock;
            struct tm *newtime;
            char buf[32];

            time(&aclock);
            newtime = localtime(&aclock);
            strftime(buf, 32, "(%H:%M:%S)", newtime);
        csString buffer;
#ifdef _WIN32
        buffer.Format("%s %s%s\r\n", buf, GetBracket(type).GetDataSafe(), message);
#else
        buffer.Format("%s %s%s\n", buf, GetBracket(type).GetDataSafe(), message);
#endif
        logFile[channel]->Write(buffer.GetData(), buffer.Length());
        logFile[channel]->Flush();
        }
    }
}

void pawsChatWindow::CreateSettingNode(iDocumentNode* mNode,int color,const char* name)
{
    csRef<iDocumentNode> cNode = mNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    cNode->SetValue(name);
    int r,g,b,alpha;
    graphics2D->GetRGB(color,r,g,b,alpha);
    cNode->SetAttributeAsInt("r",r);
    cNode->SetAttributeAsInt("g",g);
    cNode->SetAttributeAsInt("b",b);
}

void pawsChatWindow::SaveChatSettings()
{
    // Save to file
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_CHAT_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());

    csRef<iDocument> doc = docsys->CreateDocument();
    csRef<iDocumentNode> root,chatNode, colorNode, optionNode,looseNode,filtersNode,
                         badWordsNode,badWordsTextNode,cNode, logNode, selectTabStyleNode,
                         echoScreenInSystemNode, mainBracketsNode, yourColorMixNode, joindefaultchannelNode,
                         mainTabNode, flashingNode, flashingOnCharNode, node;

    root = doc->CreateRoot();

    chatNode = root->CreateNodeBefore(CS_NODE_ELEMENT,0);
    chatNode->SetValue("chat");

    optionNode = chatNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    optionNode->SetValue("chatoptions");

    selectTabStyleNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    selectTabStyleNode->SetValue("selecttabstyle");
    selectTabStyleNode->SetAttributeAsInt("value",(int)settings.selectTabStyle);

    echoScreenInSystemNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    echoScreenInSystemNode->SetValue("echoscreeninsystem");
    echoScreenInSystemNode->SetAttributeAsInt("value",(int)settings.echoScreenInSystem);

    mainBracketsNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    mainBracketsNode->SetValue("mainbrackets");
    mainBracketsNode->SetAttributeAsInt("value",(int)settings.mainBrackets);

    yourColorMixNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    yourColorMixNode->SetValue("yourcolormix");
    yourColorMixNode->SetAttributeAsInt("value",(int)settings.yourColorMix);
    
    joindefaultchannelNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    joindefaultchannelNode->SetValue("joindefaultchannel");
    joindefaultchannelNode->SetAttributeAsInt("value",(int)settings.joindefaultchannel);

    looseNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    looseNode->SetValue("loose");
    looseNode->SetAttributeAsInt("value",(int)settings.looseFocusOnSend);

    for (int i = 0; i < CHAT_NLOG; i++)
    {
        logNode = optionNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
        logNode->SetValue(logWidgetName[i]);
        logNode->SetAttributeAsInt("value",(int)settings.logChannel[i]);
    }


    filtersNode = chatNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    filtersNode->SetValue("filters");
    badWordsNode = filtersNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    badWordsNode->SetValue("badwords");
    badWordsNode->SetAttributeAsInt("incoming",(int)settings.enableBadWordsFilterIncoming);
    badWordsNode->SetAttributeAsInt("outgoing",(int)settings.enableBadWordsFilterOutgoing);

    csString s;
    for (size_t z = 0; z < settings.badWords.GetSize(); z++)
    {
        badWordsTextNode = badWordsNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
        badWordsTextNode->SetValue("replace");
        badWordsTextNode->SetAttribute("bad",settings.badWords[z]);
        if (settings.goodWords[z].Length())
        {
            badWordsTextNode->SetAttribute("good",settings.goodWords[z]);
        }
    }

    // Now for the colors
    colorNode = chatNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    colorNode->SetValue("chatcolors");

    CreateSettingNode(colorNode,settings.systemColor,"systemtext");
    CreateSettingNode(colorNode,settings.playerColor,"playernametext");
    CreateSettingNode(colorNode,settings.auctionColor,"auctiontext");
    CreateSettingNode(colorNode,settings.helpColor,"helptext");
    CreateSettingNode(colorNode,settings.groupColor,"grouptext");
    CreateSettingNode(colorNode,settings.yourColor,"yourtext");
    CreateSettingNode(colorNode,settings.guildColor,"guildtext");
    CreateSettingNode(colorNode,settings.allianceColor,"alliancetext");
    CreateSettingNode(colorNode,settings.shoutColor,"shouttext");
    CreateSettingNode(colorNode,settings.channelColor,"channeltext");
    CreateSettingNode(colorNode,settings.npcColor, "npctext" );
    CreateSettingNode(colorNode,settings.tellColor,"telltext");
    CreateSettingNode(colorNode,settings.chatColor,"chattext");
    CreateSettingNode(colorNode,settings.adminColor,"admintext");
    CreateSettingNode(colorNode,settings.gmColor, "gmtext" );

    csRef<iDocumentNode> msgNode = chatNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
    msgNode->SetValue("msgfilters");

    // and the message filters
    for(int a = 0;a < COMBAT_TOTAL_AMOUNT;a++)
    {
        csString type;

        bool value1 = true,
             value2 = true;

        CHAT_COMBAT_FILTERS filter;

        // Check which type it is
        switch(a)
        {
            case 0:
                type = "SUC";
                filter = COMBAT_SUCCEEDED;
                break;
            case 1:
                type = "BLO";
                filter = COMBAT_BLOCKED;
                break;
            case 2:
                type = "DOD";
                filter = COMBAT_DODGED;
                break;
            case 3:
                type = "MIS";
                filter = COMBAT_MISSED;
                break;
            case 4:
                type = "FAI";
                filter = COMBAT_FAILED;
                break;
            case 5:
            default:
                type = "STA";
                filter = COMBAT_STANCE;
                break;
        }


        // Enabled?
        if(!(settings.meFilters & filter))
            value1 = false;

        if(!(settings.vicinityFilters & filter))
            value2 = false;

        // Create the nodes
        csRef<iDocumentNode> cNode = msgNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
        cNode->SetValue("me");
        cNode->SetAttribute("type",type);
        cNode->SetAttribute("value",value1?"true":"false");

        csRef<iDocumentNode> c2Node = msgNode->CreateNodeBefore(CS_NODE_ELEMENT,0);
        c2Node->SetValue("vicinity");
        c2Node->SetAttribute("type",type);
        c2Node->SetAttribute("value",value2?"true":"false");
    }

    doc->Write(file);
}

void pawsChatWindow::HandleSystemMessage(MsgEntry *me)
{
    psSystemMessage msg(me);

    if(!settings.echoScreenInSystem &&
        (msg.type == MSG_OK || msg.type == MSG_ERROR || msg.type == MSG_RESULT || msg.type == MSG_ACK))
        return;

    if ( msg.msgline != "." )
    {
        const char* start = msg.msgline.GetDataSafe();

        char* currentLine = new char[msg.msgline.Length()+1];
        csString header;//Used for storing the header in case of MSG_WHO

        //Handles messages containing the /who content
        if(msg.type == MSG_WHO)
        {
            csString msgCopy=msg.msgline;               // Make a copy for not modifying the original
            size_t nLocation = msgCopy.FindFirst('\n'); // This get the end of the first line
            csArray<csString> playerLines;              // This is used for storing the data related to the players online
            msgCopy.SubString(header,0, nLocation);     // The first line is copied, since it is the header and shouldn't be sorted.
            csString playerCount;

            if (nLocation > 0)
            {
                size_t stringStart = nLocation + 1;
                nLocation = msgCopy.FindFirst ('\n', stringStart);

                csString line;

                while (nLocation != ((size_t)-1))//Until it is possible to find "\n"
                {
                    // Extract the current string
                    msgCopy.SubString(line, stringStart , nLocation-stringStart);
                    playerLines.Push(line);

                    // Find next substring
                    stringStart = nLocation + 1;
                    nLocation = msgCopy.FindFirst ('\n', stringStart);
                }
                playerCount=playerLines.Pop();//I discard the last element -it is not a player- and copy somewhere.
                playerLines.Sort();//This sorts the list.
            }

            for (size_t i=0; i< playerLines.GetSize();i++)//Now we copy the sorted names in the header msg.
            {
                header.Append('\n');
                header.Append(playerLines[i]);
            }
            header.Append('\n');
            header.Append(playerCount);//We add the line that says how many players there are online
            start= header.GetData();//We copy to start, so it can be displayed like everything else in the chat.

        }
        const char* workingString = start;
        while(workingString)
        {
            strcpy(currentLine, workingString);
            workingString = strchr(workingString, '\n');
            if(workingString)
            {
                currentLine[workingString - start] = '\0';
                workingString++;
                start = workingString;
            }


            // Handle filtering stances.
            if (msg.type == MSG_COMBAT_STANCE)
            {
                GEMClientActor *gca =
                    (GEMClientActor*) psengine->GetCelClient()->GetMainPlayer();
                bool local = (strstr(currentLine, gca->GetName()) == currentLine);

                if ((local && !(settings.meFilters & COMBAT_STANCE))
                    || (!local && !(settings.vicinityFilters & COMBAT_STANCE)))
                {
                    return;
                }
            }

            csString buff;
            buff.Format(">%s", currentLine);

            int colour = settings.systemColor;

            csString noCaseMsg = currentLine;
            noCaseMsg.Downcase();

            if (psContain(noCaseMsg,systemTriggers))
            {
                colour = settings.adminColor;
            }

            int chatType = CHAT_SYSTEM;
            if (msg.type == MSG_INFO_SERVER)
                chatType = CHAT_SERVER_INFO;
            else if (msg.type == MSG_INFO_BASE ||
                     msg.type == MSG_COMBAT_STANCE ||
                     msg.type == MSG_COMBAT_OWN_DEATH ||
                     msg.type == MSG_COMBAT_DEATH ||
                     msg.type == MSG_COMBAT_VICTORY ||
                     msg.type == MSG_COMBAT_NEARLY_DEAD ||
                     msg.type == MSG_LOOT)
                chatType = CHAT_SYSTEM_BASE;
            else if ((msg.type & MSG_COMBAT) != 0)
            	chatType = CHAT_COMBAT;

            WordArray playerName(psengine->GetMainPlayerName());
            bool hasCharName = noCaseMsg.Find(playerName[0].Downcase().GetData()) != (size_t)-1;
            ChatOutput(buff.GetData(), colour, chatType, true, hasCharName);

            if (soundmgr && psengine->GetSoundStatus())
            {
                soundmgr->HandleSoundType(msg.type);
            }

            LogMessage((msg.type == MSG_INFO_BASE || msg.type == MSG_INFO_SERVER) ? CHAT_LOG_ALL : CHAT_LOG_SYSTEM, buff.GetData());
        }
        delete[] currentLine;
    }
    return;
}

void pawsChatWindow::HandleMessage(MsgEntry *me)
{
    bool sendAway = false;
    bool flashEnabled = true;

    if ( me->GetType() == MSGTYPE_SYSTEM )
    {
        HandleSystemMessage(me);
        return;
    }
    if(me->GetType() == MSGTYPE_CHANNEL_JOINED)
    {
    	psChannelJoinedMessage joinedMsg(me);
    	channelIDs.PutUnique(joinedMsg.channel, joinedMsg.id);
    	int hotkeyChannel = 10;
    	for(size_t i = 0; i < channels.GetSize(); i++)
    	{
    		if(channels[i] == 0)
    		{
    			hotkeyChannel = i + 1;
    			channels[i] = joinedMsg.id;
    			break;
    		}
    	}
    	csString msg;
    	msg.Format("Channel %s has been added with hotkey /%d", joinedMsg.channel.GetData(), hotkeyChannel);
        ChatOutput(msg, settings.channelColor, CHAT_CHANNEL, true, false, hotkeyChannel);

    	return;
    }

    psChatMessage msg(me);

    switch( msg.iChatType )
    {
        case CHAT_TELL:
        case CHAT_SERVER_TELL:
        case CHAT_TELLSELF:
        case CHAT_GUILD:
        case CHAT_ADVISOR:
        case CHAT_ADVICE:
        case CHAT_ADVICE_LIST:
            break;
        default:
            if( (psengine->GetCelClient()->GetActorByName(msg.sPerson, false) == NULL) &&
                (psengine->GetCelClient()->GetActorByName(msg.sPerson, true)))
            {
                msg.sPerson = "Someone";
            }
            break;
    }

    if (msg.translate)
        msg.sText = PawsManager::GetSingleton().Translate(msg.sText);

    csString buff;
    int colour = -1;
    size_t channelID = 0;

    if ( !havePlayerName )  // save name for auto-complete later
    {
        noCasePlayerName.Replace(psengine->GetMainPlayerName());
        noCasePlayerName.Downcase(); // Create lowercase string
        noCasePlayerForename = GetWordNumber(noCasePlayerName, 1);
        chatTriggers.Push(noCasePlayerForename);
        havePlayerName = true;
    }

    // Use lowercase strings
    csString noCasePerson = msg.sPerson;
    noCasePerson.Downcase();
    csString noCaseMsg = msg.sText;
    noCaseMsg.Downcase();

    if (IgnoredList->IsIgnored(msg.sPerson) && msg.iChatType != CHAT_TELLSELF && msg.iChatType != CHAT_ADVISOR && msg.iChatType != CHAT_ADVICE)
        return;

    switch( msg.iChatType )
    {
        case CHAT_GROUP:
        {
            // allows /group <person> /me sits down for a private action
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s says: %s"),
                            (const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.groupColor;
            break;
        }
        case CHAT_SHOUT:
        {
			buff.Format(PawsManager::GetSingleton().Translate("%s shouts: %s"),
					(const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.shoutColor;
            break;
        }
        case CHAT_GM:
        {
            buff.Format(PawsManager::GetSingleton().Translate("%s"), (const char *)msg.sText);
            colour = settings.gmColor;
            break;
        }

        case CHAT_GUILD:
        {
            // allows /guild <person> /me sits down for a private action
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s says: %s"),
                            (const char *)msg.sPerson,(const char *)msg.sText);
            colour = settings.guildColor;
            break;
        }
        
        case CHAT_ALLIANCE:
        {
            // allows /alliance <person> /me sits down for a private action
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s says: %s"),
                            (const char *)msg.sPerson,(const char *)msg.sText);
            colour = settings.allianceColor;
            break;
        }

        case CHAT_AUCTION:
        {
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s auctions: %s"),
                        (const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.auctionColor;
            break;
        }

        case CHAT_SAY:
        {
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s says: %s"),
                        (const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.chatColor;
            break;
        }

        case CHAT_NPCINTERNAL:
        case CHAT_NPC: 
        {
            buff.Format(PawsManager::GetSingleton().Translate("%s says: %s"),
                        (const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.npcColor;
            break;
        }

        case CHAT_NPC_ME:
        {
            buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText));
            colour = settings.npcColor;
            break;
        }

        case CHAT_NPC_MY:
        {
            buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText));
            colour = settings.npcColor;
            break;
        }

        case CHAT_NPC_NARRATE:
        {
            buff.Format("-%s-", (const char *)msg.sText);
            colour = settings.shoutColor;
            break;
        }

        case CHAT_PET_ACTION:
        {
            if (msg.sText.StartsWith("'s "))
                buff.Format("%s%s", (const char *)msg.sPerson, ((const char *)msg.sText));
            else
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText));
            colour = settings.chatColor;
            break;
        }

        case CHAT_TELL:

            //Move list of lastreplies down to make room for new lastreply if necessary
            if (replyList[0] != msg.sPerson)
            {
                int i = 0;
                while(!(replyList[i].IsEmpty()) && i<=4)
                    i++;
                for (int j=i-2; j>=0; j--)
                    replyList[j+1]=replyList[j];
                replyList[0] = msg.sPerson;
            }
            replyCount = 0;

            if ( awayText.Length() && !msg.sText.StartsWith("[auto-reply]") )
                sendAway = true;

            // no break (on purpose)

        case CHAT_SERVER_TELL:

            // allows /tell <person> /me sits down for a private action
            if ( msg.sText.StartsWith("/me ") )
                buff.Format("%s %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else if ( msg.sText.StartsWith("/my ") )
                buff.Format("%s's %s", (const char *)msg.sPerson, ((const char *)msg.sText)+4);
            else
                buff.Format(PawsManager::GetSingleton().Translate("%s tells you: %s"),
                            (const char *)msg.sPerson, (const char *)msg.sText);

            colour = settings.tellColor;
            break;

        case CHAT_TELLSELF:
        {
            if ( msg.sText.StartsWith("/me ") )
            {
                WordArray tName(psengine->GetMainPlayerName());
                buff.Format("%s %s",tName[0].GetData(),
                            ((const char *)msg.sText)+4);
            }
            else if ( msg.sText.StartsWith("/my ") )
            {
                WordArray tName(psengine->GetMainPlayerName());
                buff.Format("%s's %s",tName[0].GetData(),
                            ((const char *)msg.sText)+4);
            }
            else
                buff.Format(PawsManager::GetSingleton().Translate("You tell %s: %s"),
                            (const char *)msg.sPerson, (const char *)msg.sText);

            if (IgnoredList->IsIgnored(msg.sPerson))
                ChatOutput(PawsManager::GetSingleton().Translate("You are ignoring all replies from that person."));

            colour = settings.tellColor;
            flashEnabled = false;
            break;
        }

        case CHAT_ADVISOR:
        {
            if (strstr(noCasePlayerName,noCasePerson))
            {
                buff.Format(PawsManager::GetSingleton().Translate("You advise %s with this suggestion: %s"),
                            msg.sOther.GetDataSafe(), msg.sText.GetDataSafe());
            }
            else
            {
                csString noCaseOtherName = msg.sOther;
                noCaseOtherName.Downcase();

                if (strstr(noCasePlayerName, noCaseOtherName))
                    buff.Format(PawsManager::GetSingleton().Translate("The advisor suggests: %s"), msg.sText.GetData());
                else
                    buff.Format(PawsManager::GetSingleton().Translate("%s advises %s with this suggestion: %s"),
                                msg.sPerson.GetData(), msg.sOther.GetDataSafe(), msg.sText.GetData());
            }
            colour = settings.helpColor;
            break;
        }

        case CHAT_ADVICE:
        {
            if (strstr(noCasePlayerName, noCasePerson))
            {
                buff.Format(PawsManager::GetSingleton().Translate("You ask: %s"), msg.sText.GetData());
            }
            else
            {
                buff.Format(PawsManager::GetSingleton().Translate("%s asks: %s"),
                            (const char *)msg.sPerson, (const char *)msg.sText);
            }
            colour = settings.helpColor;
            break;
        }

        case CHAT_ADVICE_LIST:
        {
            buff.Format(PawsManager::GetSingleton().Translate("%s previously asked: %s"),
                        (const char *)msg.sPerson, (const char *)msg.sText);
            colour = settings.helpColor;
            break;
        }
        
        case CHAT_CHANNEL:
        {
        	channelID = channels.Find(msg.channelID);
        	// Not yet received channel join reply
        	if(channelID == csArrayItemNotFound)
        		return;
        	channelID++;
        	buff.Format("[%zu: %s] %s: %s", channelID, channelIDs.GetKey(msg.channelID, "").GetData(),
						(const char *)msg.sPerson, (const char *)msg.sText);
			colour = settings.channelColor;
			break;
        }
        default:
        {
            buff.Format(PawsManager::GetSingleton().Translate("Unknown Chat Type: %d"), msg.iChatType);
            colour = settings.chatColor;
            break;
        }
    }

    // Add the player to our auto-complete list
    AddAutoCompleteName(msg.sPerson);

    if (msg.iChatType != CHAT_TELL)
    {
        if ( noCasePlayerForename == noCasePerson || msg.iChatType == CHAT_TELLSELF)
        {
            //if the yourColorMix is enabled mix the yourcolour with the colour of the chat type
            //else use the normal yourcolour
            if(settings.yourColorMix)
               colour =  mixColours(colour, settings.yourColor);
            else
                colour = settings.yourColor;

            flashEnabled = false;
        }
        else if ( psContain(noCaseMsg,chatTriggers) )
        {
            colour = settings.playerColor;
        }
    }

    WordArray playerName(psengine->GetMainPlayerName());
    bool hasCharName = msg.sText.Downcase().Find(playerName[0].Downcase().GetData()) != (size_t)-1;

    if (!buff.IsEmpty())
    {
        ChatOutput(buff.GetData(), colour, msg.iChatType, flashEnabled, hasCharName, channelID);
    }

    LogMessage(CHAT_LOG_ALL, buff.GetDataSafe(), msg.iChatType);

    if ( sendAway )
    {
        csString autoResponse, clientmsg(awayText);
        if ( clientmsg.StartsWith("/me ") )
            clientmsg.Format("%s %s", psengine->GetMainPlayerName(), ((const char *)awayText)+4);
        else if ( clientmsg.StartsWith("/my ") )
            clientmsg.Format("%s's %s",psengine->GetMainPlayerName(), ((const char *)awayText)+4);

        autoResponse.Format("/tell %s [auto-reply] %s", (const char*)msg.sPerson, clientmsg.GetData());
        const char* errorMessage = cmdsource->Publish(autoResponse.GetData());
        if ( errorMessage )
            ChatOutput( errorMessage );
    }
}

void pawsChatWindow::JoinChannel(csString chan)
{
	psChannelJoinMessage cmdjoin(chan);
	cmdjoin.SendMessage();
}

bool pawsChatWindow::LeaveChannel(int hotkeyChannel)
{	
	uint16_t channelID = channels[hotkeyChannel - 1];
	if(channelID == 0)
		return false;
	channels[hotkeyChannel - 1] = 0;
	psChannelLeaveMessage cmdleave(channelID);
	cmdleave.SendMessage();
	csString msg;
	msg.Format("Left channel %d.", hotkeyChannel);
    ChatOutput(msg, settings.channelColor, CHAT_CHANNEL, true, false, hotkeyChannel);

	return true;
}


void pawsChatWindow::SubscribeCommands()
{
    cmdsource->Subscribe("/say",this);
    cmdsource->Subscribe("/s",this);
    cmdsource->Subscribe("/shout",this);
    cmdsource->Subscribe("/sh",this);
    cmdsource->Subscribe("/tell",this);
    cmdsource->Subscribe("/t",this);
    cmdsource->Subscribe("/guild",this);
    cmdsource->Subscribe("/g",this);
    cmdsource->Subscribe("/alliance",this);
    cmdsource->Subscribe("/a",this);
    cmdsource->Subscribe("/group",this);
    cmdsource->Subscribe("/gr",this);
    cmdsource->Subscribe("/tellnpc",this);
    cmdsource->Subscribe("/tellnpcinternal",this);

    cmdsource->Subscribe("/me",this);
    cmdsource->Subscribe("/my",this);
    cmdsource->Subscribe("/mypet",this);
    cmdsource->Subscribe("/auction",this);
    cmdsource->Subscribe("/report",this);
    
    // channel shortcuts
    cmdsource->Subscribe("/1", this);
    cmdsource->Subscribe("/2", this);
    cmdsource->Subscribe("/3", this);
    cmdsource->Subscribe("/4", this);
    cmdsource->Subscribe("/5", this);
    cmdsource->Subscribe("/6", this);
    cmdsource->Subscribe("/7", this);
    cmdsource->Subscribe("/8", this);
    cmdsource->Subscribe("/9", this);
    cmdsource->Subscribe("/10", this);
}

bool pawsChatWindow::InputActive()
{
    return inputText->HasFocus();
}

bool pawsChatWindow::OnMouseDown( int button, int modifiers, int x , int y )
{
    pawsWidget::OnMouseDown( button, modifiers, x, y );

    PawsManager::GetSingleton().SetCurrentFocusedWidget( inputText );

    for (size_t z = 0; z < children.GetSize(); z++ )
    {
        //children[z]->Show();
    }

    return true;
}

void pawsChatWindow::Show()
{
    pawsControlledWindow::Show();

    for (size_t x = 0; x < children.GetSize(); x++ )
    {
        children[x]->Show();
    }
    BringToTop(inputText);
}


void pawsChatWindow::OnLostFocus()
{
    hasFocus = false;

    for (size_t x = 0; x < children.GetSize(); x++ )
    {
        //children[x]->Hide();
    }

    //chatText->Show();
    //chatText->GetBorder()->Hide();

    //systemText->Show();
    //systemText->GetBorder()->Hide();


}


bool pawsChatWindow::OnKeyDown(int keyCode, int key, int modifiers )
{
    // Do not handle key presses if the text box is not focused. This can occur if the chat tabs are clicked,
    // since they handle the mouse click the input text box is not focused,
    // which allows different tabs to be checked without interrupting movement
    if (!InputActive())
    {
        if (parent)
            return parent->OnKeyDown( keyCode, key, modifiers );
        else
            return false;
    }

    switch ( key )
    {
        case CSKEY_ENTER:
        {
            if (settings.looseFocusOnSend || !strcmp(inputText->GetText(), ""))
                PawsManager::GetSingleton().SetCurrentFocusedWidget((pawsWidget*)PawsManager::GetSingleton().GetMainWidget());

            csString text = inputText->GetText();
            inputText->Clear();
            SendChatLine(text);
            break;
        }

        // History access - Previous
        case CSKEY_UP:
        {
            if (strlen(inputText->GetText()) == 0 && currLine.GetData() != NULL)
            {
                // Restore cleared line from buffer
                inputText->SetText(currLine);
                currLine.Free(); // Set to NULL
            }
            else  // Browse history
            {
                csString* cmd = chatHistory->GetPrev();
                if (cmd)
                {
                    // Save current line in buffer
                    if (currLine.GetData() == NULL)
                        currLine = inputText->GetText();

                    inputText->SetText(cmd->GetData());
                }
            }

            break;
        }

        // History access - Next
        case CSKEY_DOWN:
        {
            csString* cmd = chatHistory->GetNext();
            if (cmd)  // Browse history
            {
                inputText->SetText(cmd->GetData());
            }
            else if (currLine.GetData() != NULL)
            {
                // End of history; restore current line from buffer
                inputText->SetText(currLine);
                currLine.Free(); // Set to NULL
                chatHistory->SetGetLoc(0);
            }
            else if (strlen(inputText->GetText()) > 0)
            {
                // Pressing down at bottom clears
                currLine = inputText->GetText();
                inputText->SetText("");
            }

            break;
        }

        // Tab Completion
        case CSKEY_TAB:
        {
            // What's the command we're about to tab-complete?
            const char *cmd = inputText->GetText();

            if (!cmd)
                return true;

            // What kind of completion are we doing?
            if (cmd[0] == '/' && !strchr(cmd,' '))
            {
                // Tab complete the started command
                TabCompleteCommand(cmd);
            }
            else
            {
                // Tab complete the name
                TabCompleteName(cmd);
            }

            break;
        }

        default:
        {
            // This statement should always be true since the printable keys should be handled
            // by the child (edittextbox)
            // if ( !isprint( (char)key ))
            //{
            if (parent)
                return parent->OnKeyDown( keyCode, key, modifiers );
            else
                return false;
            //}
            break;
        }
    }

    return true;
}

void pawsChatWindow::SendChatLine(csString& textToSend)
{
    if ( textToSend.Length() )
    {
        if ( textToSend.GetAt(0) != '/' )
        {
        	csString chatType;
        	if(tabs)
            	chatType = tabs->GetActiveTab()->GetName();

            if (chatType == "TellText")
            {
                int i = 0;
                csString buf;
                while (!replyList[i].IsEmpty() && i <= 4)
                    i++;
                if (!i)
                {
                    textToSend.Insert(0, "/tell ");
                }else{
                    if (replyCount >= i)
                        replyCount = 0;
                    buf.Format("/tell %s ", replyList[replyCount].GetData());
                    textToSend.Insert(0, buf.GetData());
                    replyCount++;
                }
            }
            else if (chatType == "GuildText")
                textToSend.Insert(0, "/guild ");
            else if (chatType == "AllianceText")
                textToSend.Insert(0, "/alliance ");
            else if (chatType == "GroupText")
                textToSend.Insert(0, "/group ");
            else if (chatType == "AuctionText")
                textToSend.Insert(0, "/auction ");
            else if (chatType == "HelpText")
                textToSend.Insert(0, "/help ");
            else if (chatType == "NpcText")
                textToSend.Insert(0, "/tellnpc ");
            else
                textToSend.Insert(0, "/say ");

        }
        
        const char* errorMessage = cmdsource->Publish(textToSend);
        if (errorMessage)
            ChatOutput(errorMessage);

        // Insert the command into the history
        chatHistory->Insert(textToSend);

        // Now special handling for showing /tellnpc commands locally, since these are not reflected to the client from the server anymore
        if (textToSend.StartsWith("/tellnpc"))
        {
            textToSend.Insert(8,"internal");
            cmdsource->Publish(textToSend);
        }

        currLine.Free(); // Set to NULL
    }
}

bool pawsChatWindow::OnMenuAction(pawsWidget * widget, const pawsMenuAction & action)
{
    if (action.name == "TranslatedChat")
    {
        if (action.params.GetSize() > 0)
        {
            psChatMessage chat(0, 0, "", 0, action.params[0], CHAT_SAY, true);
            msgqueue->SendMessage(chat.msg);
        }
        else
            Error2("Menu action \"TranslatedChat\" must have one parameter (widget name is %s)", widget->GetName());
    }

    return pawsWidget::OnMenuAction(widget, action);
}

bool pawsChatWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    // We know that the calling widget is a button.
    csString name = widget->GetName();

    if ( name=="inputback" )
    {
        PawsManager::GetSingleton().SetCurrentFocusedWidget( inputText );
        BringToTop( inputText );
    }

    return true;
}


void pawsChatWindow::PerformAction( const char* action )
{
    csString temp(action);
    if ( temp == "togglevisibility" )
    {
        pawsWidget::PerformAction( action );
        PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
    }
}

void pawsChatWindow::RefreshCommandList()
{
    cmdsource->GetSubscribedCommands(commandList);
}

/// Tab completion
void pawsChatWindow::TabCompleteCommand(const char *cmd)
{
    // Sanity
    if (!cmd)
        return;

    // Make sure we have our command list
    if (commandList.IsEmpty())
        RefreshCommandList();

    psString partial(cmd);
    //psString *found = commandList.FindPartial(&partial);
    //if (found)
    //{
    //    inputText->SetText(*found);  // unique partial can be supplied
    //    return;
    //}

    //// display list of partial matches if non-unique
    //found = commandList.FindPartial(&partial,false);
    int count = 0;

    // valid but not unique
    psString list;
    const psString *last=NULL;
    size_t max_common = 50; // big number gets pulled in
    csRedBlackTree<psString>::Iterator loop(commandList.GetIterator());
    while (loop.HasNext())
    {
    	const psString& found = loop.Next();
    	
        if (!found.PartialEquals(partial))
        {
            count++;
            if(!list.IsEmpty())
                list.Append(" ");
            list.Append(*found);
            if (!last)
                last = &found;
            size_t common = found.FindCommonLength(*last);

            if (common < max_common)
            {
                max_common = common;
                last = &found;
            }
        }
        else if (list.Length())
            break;
    }
    if (count == 1)
    {
        list.Append(" ");
        inputText->SetText(list.GetData());  // unique partial can be supplied
        return;
    }
    else if(count > 1)
    {
        list.Insert(0,PawsManager::GetSingleton().Translate("Ambiguous command: "));
        ChatOutput(list);
        psString complete;
        last->GetSubString(complete,0,max_common);
        inputText->SetText(complete);
    }

    return;
}

void pawsChatWindow::TabCompleteName(const char *cmdstr)
{
    // Sanity
    if (!cmdstr || *cmdstr == 0)
        return;

    // Set cmd to point to the start of the name.
    char *cmd = (char *)(cmdstr + strlen(cmdstr) - 1);
    while (cmd != cmdstr && isalpha(*cmd))
        --cmd;
    if (!isalpha(*cmd))
        cmd++;

    // Make sure we have our auto-complete list
    if (!autoCompleteNames.GetSize() )
    {
        return;
    }

    psString partial(cmd);
    if (partial.Length () == 0)
        return;

    // valid but not unique
    psString list, last;
    size_t max_common = 50; // big number gets pulled in
    int matches = 0;
    csArray<csString>::Iterator iter = autoCompleteNames.GetIterator();
    while (iter.HasNext())
    {
        psString found = iter.Next();
        if(found.StartsWith(partial, true))
        {
            list.Append(" ");
            list.Append(found);
            if (!last)
                last = found;

            matches++;

            size_t common = found.FindCommonLength(last);

            if (common < max_common)
            {
                max_common = common;
                last = found;
            }
        }
    }

    if (matches > 1)
    {
        list.Insert(0,PawsManager::GetSingleton().Translate("Ambiguous name:"));
        ChatOutput(list);

        psString line, partial;

        last.GetSubString(partial,0,max_common);
        line = cmdstr;
        line.DeleteAt(cmd-cmdstr,line.Length()-(cmd-cmdstr) );
        line += partial;
        inputText->SetText(line);
    }
    else if (matches == 1)
    {
        psString complete(cmdstr);
        complete.DeleteAt(cmd-cmdstr,complete.Length()-(cmd-cmdstr) );
        complete.Append(last);
        complete.Append(" ");
        inputText->SetText(complete);
    }

    return;
}


void pawsChatWindow::AutoReply(void)
{
    int i = 0;
    csString buf;

    while (!replyList[i].IsEmpty() && i <= 4)
        i++;

    if (!i)
    {
        inputText->SetText("/tell ");
        return;
    }

    if (replyCount >= i)
        replyCount = 0;

    buf.Format("/tell %s ", replyList[replyCount].GetData());
    inputText->SetText(buf.GetData());

    replyCount++;
}

void pawsChatWindow::SetAway(const char* text)
{
    if ( !strcmp(text, "off") || strlen(text) == 0)
    {
        awayText.Clear();
        ChatOutput( "Auto-reply has been turned OFF" );
    }
    else
    {
        awayText = text;
        ChatOutput( "Auto-reply SET");
    }
}

void pawsChatWindow::Clear()
{
	for(int chattype = 0; chattype < CHAT_END; chattype++)
	{
		csArray<iPAWSSubscriber*> subscribers = PawsManager::GetSingleton().ListSubscribers(CHAT_TYPES[chattype]);
		for(size_t i = 0; i < subscribers.GetSize(); i++)
		{
			pawsMessageTextBox* textbox = dynamic_cast<pawsMessageTextBox*>(subscribers[i]);
			textbox->Clear();
		}
	}
}


void pawsChatWindow::BadWordsFilter(csString& s)
{
    csString mask = "$@!";
    csString lowercase;
    size_t badwordPos;

    lowercase = s;
    lowercase.Downcase();

    for (size_t i = 0; i < settings.badWords.GetSize(); i++)
    {
        size_t pos = 0;
        while (true)
        {
            csString badWord = settings.badWords[i];
            csString replace = settings.goodWords[i];

            badwordPos = lowercase.FindStr(badWord.GetDataSafe(),pos);

            // LOOP EXIT:
            if (badwordPos == SIZET_NOT_FOUND)
                break;

            // Check for whole word boundaries.  First confirm beginning of word
            if (badwordPos>0 && isalpha(lowercase[badwordPos-1]))
            {
                pos++;
                break;
            }
            // now verify end of word
            if (badwordPos+badWord.Length() < lowercase.Length() && isalpha(lowercase[badwordPos+badWord.Length()]))
            {
                pos++;
                break;
            }

            if (replace.Length() == 0)
            {
                for (size_t k = badwordPos; k < badwordPos + badWord.Length(); k++)
                    s[k] = mask[ (k-badwordPos) % mask.Length() ];
                pos = badwordPos + badWord.Length();
            }
            else
            {
                s.DeleteAt( badwordPos, badWord.Length());
                s.Insert(badwordPos,replace);
                pos = badwordPos + replace.Length();
            }
            lowercase = s;
            lowercase.Downcase();
        }
    }
}

csString pawsChatWindow::GetBracket(int type) //according to the type return the correct bracket
{
    switch (type)
    {
        case CHAT_TELL:
        case CHAT_TELLSELF:
            return "[Tell] ";
        case CHAT_GUILD:
            return "[Guild] ";
        case CHAT_ALLIANCE:
            return "[Alliance] ";
        case CHAT_GROUP:
            return "[Group] ";
        case CHAT_AUCTION:
            return "[Auction] ";
        case CHAT_ADVISOR:
        case CHAT_ADVICE:
        case CHAT_ADVICE_LIST:
            return "[Help] ";
        case CHAT_NPC:
        case CHAT_NPC_ME:
        case CHAT_NPC_MY:
        case CHAT_NPC_NARRATE:
            return "[NPC] ";
    }
    return "";
}

void pawsChatWindow::ChatOutput(const char* data, int colour, int type, bool flashEnabled, bool hasCharName, int hotkeyChannel)
{
    csString s = data;
    if (settings.enableBadWordsFilterIncoming && type != CHAT_SERVER_INFO)
    {
        BadWordsFilter(s);
    }
    csString pubName = CHAT_TYPES[type];
    if(type == CHAT_CHANNEL)
    	pubName += hotkeyChannel;
    PawsManager::GetSingleton().Publish(pubName, s, colour );
}

void pawsChatWindow::AddAutoCompleteName(const char *cname)
{
    csString name = cname;

    for (size_t i = 0; i < autoCompleteNames.GetSize(); i++)
    {
        if (autoCompleteNames[i].CompareNoCase(name))
        {
            return;
        }
    }

    autoCompleteNames.Push(name);
}

void pawsChatWindow::RemoveAutoCompleteName(const char *name)
{
    for (size_t i = 0; i < autoCompleteNames.GetSize(); i++)
    {
        if (autoCompleteNames[i].CompareNoCase(name))
        {
            autoCompleteNames.DeleteIndex(i);
            Debug2(LOG_CHAT,0, "Removed %s from autoComplete list.\n",name);
        }
    }
}

int pawsChatWindow::mixColours(int colour1, int colour2)
{
    int r,g,b;
    int r2,g2,b2;
    graphics2D->GetRGB(colour1,r,g,b); //gets the rgb values of the first colour
    graphics2D->GetRGB(colour2,r2,g2,b2); //gets the rgb values of the second colour
    return graphics2D->FindRGB((r+r2)/2,(g+g2)/2,(b+b2)/2); //does the average of the two colours and returns it back
}


//------------------------------------------------------------------------------

pawsChatHistory::pawsChatHistory()
{
    getLoc = 0;
}

pawsChatHistory::~pawsChatHistory()
{

}

// insert a string at the end of the buffer
// returns: true on success
void pawsChatHistory::Insert(const char *str)
{
    bool previous = false;

    if ( buffer.GetSize() > 0 )
    {
        // Check to see if it's the same as the last one before adding.
        if ( *buffer[ buffer.GetSize()-1] == str )
        {
            previous = true;
        }
    }

    if ( !previous )
    {
        csString * newStr = new csString( str );
        buffer.Push( newStr );
    }
    getLoc = 0;
}


// returns a copy of the stored string from 'n' commands back.
// if n is invalid (ie, more than the number of stored commands)
// then a null pointer is returned.
csString *pawsChatHistory::GetCommand(int n)
{
    // n == 0 means "this command" according to our notation
    if (n <= 0 || (size_t)n > buffer.GetSize() )
    {
        return NULL;
    }

    getLoc = n;
    csString* retval = buffer[buffer.GetSize() - (getLoc)];
    if ( retval )
    {
        return retval;
    }

    return NULL;
}

// return the next (temporally) command in history
csString *pawsChatHistory::GetNext()
{
    return GetCommand(getLoc - 1);
}

// return the prev (temporally) command from history
csString *pawsChatHistory::GetPrev()
{
    return GetCommand(getLoc + 1);
}

//------------------------------------------------------------------------------

void pawsChatWindow::SetSettings(ChatSettings& newSets)
{
    settings = newSets;
}
