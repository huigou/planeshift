/*
 * cmdusers.h - Author: Keith Fulton
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <ctype.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <imesh/sprite3d.h>
#include <csgeom/math3d.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"
#include "net/npcmessages.h"

#include "util/strutil.h"

#include "gui/chatwindow.h"
#include "gui/pawsdetailwindow.h"
#include "gui/pawsinfowindow.h"
#include "gui/inventorywindow.h"
#include "gui/pawsactivemagicwindow.h"
#include "gui/pawspetstatwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/psmainwidget.h"
#include "gui/pawsgameboard.h"

#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cmdusers.h"
#include "pscelclient.h"
#include "modehandler.h"
#include "pscharcontrol.h"
#include "globals.h"
#include "psclientchar.h"


psUserCommands::psUserCommands(ClientMsgHandler* mh,CmdHandler *ch,iObjectRegistry* obj)
  : psCmdBase(mh,ch,obj)
{
//    msgqueue->Subscribe(MSGTYPE_CHAT,this);
	cmdsource->Subscribe("/?",             this);
    cmdsource->Subscribe("/admin",         this);
    cmdsource->Subscribe("/advice",        this);
    cmdsource->Subscribe("/advisor",       this);
    cmdsource->Subscribe("/assist",        this);
    cmdsource->Subscribe("/attack",        this);
    cmdsource->Subscribe("/away",          this);
    cmdsource->Subscribe("/bank",          this);
    cmdsource->Subscribe("/buddy",         this); // add named player to buddy list
    cmdsource->Subscribe("/buy",           this);
    cmdsource->Subscribe("/cast",          this);
    cmdsource->Subscribe("/challenge",     this);
    cmdsource->Subscribe("/clear",         this);
    cmdsource->Subscribe("/combine",       this);
    cmdsource->Subscribe("/uncombine",     this);
    cmdsource->Subscribe("/construct",     this);
    cmdsource->Subscribe("/dequip",        this);
    cmdsource->Subscribe("/die",           this);
    cmdsource->Subscribe("/dig",           this);
    cmdsource->Subscribe("/drop",          this);
    cmdsource->Subscribe("/emote",         this);
    cmdsource->Subscribe("/equip",         this);
    cmdsource->Subscribe("/fish",          this);
    cmdsource->Subscribe("/harvest",       this);
    cmdsource->Subscribe("/game",          this);
    cmdsource->Subscribe("/give",          this);
    cmdsource->Subscribe("/guard",         this);
    cmdsource->Subscribe("/help",          this);
    cmdsource->Subscribe("/ignore",        this);
    cmdsource->Subscribe("/introduce",     this);
    cmdsource->Subscribe("/join",          this);
    cmdsource->Subscribe("/leave",         this);
    cmdsource->Subscribe("/loot",          this);
    cmdsource->Subscribe("/marriage",      this);
    cmdsource->Subscribe("/motd",          this);
    cmdsource->Subscribe("/mount",         this);
    cmdsource->Subscribe("/npcmenu",       this);
    cmdsource->Subscribe("/pet",           this);
    cmdsource->Subscribe("/picklock",      this);
    cmdsource->Subscribe("/pickup",        this);
    cmdsource->Subscribe("/pos",           this);
    //cmdsource->Subscribe("/quests",      this);
    cmdsource->Subscribe("/repair",        this);
    cmdsource->Subscribe("/roll",          this);
    cmdsource->Subscribe("/rotate",        this);
    cmdsource->Subscribe("/sell",          this);
    cmdsource->Subscribe("/show",          this);
    cmdsource->Subscribe("/sit",           this);
    //cmdsource->Subscribe("/spawn",       this);
    cmdsource->Subscribe("/stand",         this);
    cmdsource->Subscribe("/starttrading",  this);
    cmdsource->Subscribe("/stopattack",    this);
    cmdsource->Subscribe("/stoptrading",   this);
    cmdsource->Subscribe("/study",         this);
    cmdsource->Subscribe("/target",        this);
    cmdsource->Subscribe("/targetcontext", this);
    cmdsource->Subscribe("/targetinfo",    this);
    cmdsource->Subscribe("/tip",           this);
    cmdsource->Subscribe("/trade",         this);
    cmdsource->Subscribe("/train",         this);
    cmdsource->Subscribe("/unmount",       this);
    cmdsource->Subscribe("/unstick",       this);
    cmdsource->Subscribe("/use",           this);
    cmdsource->Subscribe("/who",           this); // list players on server
    cmdsource->Subscribe("/write",         this);
    cmdsource->Subscribe("/yield",         this);
}

psUserCommands::~psUserCommands()
{
//    msgqueue->Unsubscribe(MSGTYPE_CHAT,this);

    cmdsource->Unsubscribe("/admin",                 this);
    cmdsource->Unsubscribe("/advice",                this);
    cmdsource->Unsubscribe("/advisor",               this);
    cmdsource->Unsubscribe("/assist",                this);
    cmdsource->Unsubscribe("/attack",                this);
    cmdsource->Unsubscribe("/away",                  this);
    cmdsource->Unsubscribe("/bank",                  this);
    cmdsource->Unsubscribe("/buddy",                 this);
    cmdsource->Unsubscribe("/buy",                   this);
    cmdsource->Unsubscribe("/cast",                  this);
    cmdsource->Unsubscribe("/challenge",             this);
    cmdsource->Unsubscribe("/clear",                 this);
    cmdsource->Unsubscribe("/combine",               this);
    cmdsource->Unsubscribe("/uncombine",             this);
    cmdsource->Unsubscribe("/construct",             this);
    cmdsource->Unsubscribe("/dequip",                this);
    cmdsource->Unsubscribe("/die",                   this);
    cmdsource->Unsubscribe("/dig",                   this);
    cmdsource->Unsubscribe("/drop",                  this);
    cmdsource->Unsubscribe("/emote",                 this);
    cmdsource->Unsubscribe("/equip",                 this);
    cmdsource->Unsubscribe("/fish",                  this);
    cmdsource->Unsubscribe("/harvest",               this);
    cmdsource->Unsubscribe("/game",                  this);
    cmdsource->Unsubscribe("/give",                  this);
    cmdsource->Unsubscribe("/guard",                 this);
    cmdsource->Unsubscribe("/help",                  this);
    cmdsource->Unsubscribe("/ignore",                this);
    cmdsource->Unsubscribe("/introduce",             this);
    cmdsource->Unsubscribe("/join",                  this);
    cmdsource->Unsubscribe("/leave",                 this);
    cmdsource->Unsubscribe("/loot",                  this);
    cmdsource->Unsubscribe("/marriage",              this);
    cmdsource->Unsubscribe("/motd",                  this);
    cmdsource->Unsubscribe("/mount",                 this);
    cmdsource->Unsubscribe("/npcmenu",               this);
    cmdsource->Unsubscribe("/pet",                   this);
    cmdsource->Unsubscribe("/picklock",              this);
    cmdsource->Unsubscribe("/pickup",                this);
    cmdsource->Unsubscribe("/pos",                   this);
    //cmdsource->Unsubscribe("/quests",              this);
    cmdsource->Unsubscribe("/repair",                this);
    cmdsource->Unsubscribe("/roll",                  this);
    cmdsource->Unsubscribe("/rotate",                this);
    cmdsource->Unsubscribe("/sell",                  this);
    cmdsource->Unsubscribe("/show",                  this);
    cmdsource->Unsubscribe("/sit",                   this);
    //cmdsource->Unsubscribe("/spawn",               this);
    cmdsource->Unsubscribe("/stand",                 this);
    cmdsource->Unsubscribe("/starttrading",          this);
    cmdsource->Unsubscribe("/stopattack",            this);
    cmdsource->Unsubscribe("/stoptrading",           this);
    cmdsource->Unsubscribe("/study",                 this);
    cmdsource->Unsubscribe("/target",                this);
    cmdsource->Unsubscribe("/targetcontext",         this);
    cmdsource->Unsubscribe("/targetinfo",            this);
    cmdsource->Unsubscribe("/tip",                   this);
    cmdsource->Unsubscribe("/trade",                 this);
    cmdsource->Unsubscribe("/train",                 this);
    cmdsource->Unsubscribe("/unmount",               this);
    cmdsource->Unsubscribe("/unstick",               this);
    cmdsource->Unsubscribe("/use",                   this);
    cmdsource->Unsubscribe("/who",                   this);
    cmdsource->Unsubscribe("/write",                 this);
    cmdsource->Unsubscribe("/yield",                 this);



    // Unsubscribe emotes.
    for(unsigned int i=0; i < emoteList.GetSize(); i++)
    {
        cmdsource->Unsubscribe(emoteList[i].command, this);
    }
}

bool psUserCommands::LoadEmotes()
{
    iVFS* vfs = psengine->GetVFS ();
    if (!vfs)
        return false;
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iFile> file = vfs->Open("/planeshift/data/emotes.xml", VFS_FILE_READ);
    if (!file)
        return false;

    const char* error = doc->Parse( file );
    if ( error )
    {
        Error2("Error loading emotes: %s", error);
        return false;
    }

    csRef<iDocumentNodeIterator> emoteIter = doc->GetRoot()->GetNode("emotes")->GetNodes("emote");

    while(emoteIter->HasNext())
    {
        csRef<iDocumentNode> emoteNode = emoteIter->Next();

        EMOTE emote;
        emote.command = emoteNode->GetAttributeValue("command");
        emote.general = emoteNode->GetAttributeValue("general");
        emote.specific = emoteNode->GetAttributeValue("specific");
        emote.anim = emoteNode->GetAttributeValue("anim");

        emoteList.Push(emote);
    }


    // Subscribe emotes.
    for(unsigned int i=0; i < emoteList.GetSize(); i++)
    {
        cmdsource->Subscribe(emoteList[i].command, this);
    }

    return true;
}

void psUserCommands::HandleSlay(bool answeredYes, void *data)
{
    psMessageCracker *msg = (psMessageCracker*) data;
    if (answeredYes)
        msg->SendMessage();
    delete msg;
}

void psUserCommands::AskToSlayBeforeSending(psMessageCracker *msg)
{
    // If target is defeated, prompt to slay...otherwise, just send the message.
    GEMClientActor *target = dynamic_cast<GEMClientActor*> (psengine->GetCharManager()->GetTarget());
    if (target && target->GetMode() == psModeMessage::DEFEATED)
    {
        pawsYesNoBox *confirm = (pawsYesNoBox*) PawsManager::GetSingleton().FindWidget("YesNoWindow");
        confirm->SetCallBack(psUserCommands::HandleSlay, msg, "This will likely kill your opponent!  Are you sure you want to attack?");
        confirm->Show();
    }
    else
    {
        msg->SendMessage();
        delete msg;
    }
}

const char *psUserCommands::HandleCommand(const char *cmd)
{
    WordArray words(cmd);

    if (words.GetCount() == 0)
        return "";


    if (  words[0] == "/show")
    {
        if (words.GetCount() > 1)
        {
            pawsControlWindow* ctrlWindow = dynamic_cast<pawsControlWindow*>(PawsManager::GetSingleton().FindWidget("ControlWindow"));
            if(!ctrlWindow || ctrlWindow->HandleWindowName(words[1]))
                return NULL;

            else return "That window cannot be found.";
        }
        return "You need to specify a window to show";
    }
    else if ( words[0] == "/study" )
    {
        pawsWidget * widget = PawsManager::GetSingleton().FindWidget("CraftWindow");
        if ( widget )
            widget->Show();

        return NULL;
    }
    else if (words[0] == "/equip" || (words[0] == "/use" && words.GetCount() > 1))
    {
        if ( words.GetCount() < 2 )
            return "Usage: /equip  [stack count] [item name]";
        int quantity = atoi(words[1]);
        csString itemName;
        if (quantity == 0)
        {
            quantity = 1;
            itemName = words.GetTail(1);
        }
        else itemName = words.GetTail(2);
        pawsInventoryWindow* window = (pawsInventoryWindow*)PawsManager::GetSingleton().FindWidget("InventoryWindow");
        window->Equip( itemName, quantity );
    }

    else if ( words[0] == "/dequip" )
    {
        if ( words.GetCount() < 2 )
            return "Usage: /dequip [item name|slot name]";

        pawsInventoryWindow* window = (pawsInventoryWindow*)PawsManager::GetSingleton().FindWidget("InventoryWindow");
        csString itemName( words.GetTail(1) );
        window->Dequip( itemName );
    }

    else if ( words[0] == "/write" )
    {
        if ( words.GetCount() < 2 )
            return "Usage: /write [item name|slot name]";

        pawsInventoryWindow* window = (pawsInventoryWindow*)PawsManager::GetSingleton().FindWidget("InventoryWindow");
        csString itemName( words.GetTail(1) );
        window->Write( itemName );
    }
    
    else if (words[0] == "/rotate")
    {
        if ( words.GetCount() < 2 )
            return "Usage: /rotate [target] [x|reset] [y|reset] [z|reset]  or /rotate [target] [x|y|z] [angle]";
        
        bool targetGiven = false;
        
        csString targetString;
        // if the command's target is not precised, get the object currently targeted
        // isdigit(words[1][0] checks if the first argument is a number
        if (words[1] == "x" || words[1] == "y" || words[1] == "z" ||
            words[1] == "reset" || isdigit(words[1].GetAt(0)) || atoi(words[1]))
            targetString = FormatTarget();
        else
        {
            targetString = FormatTarget(words[1]);
            targetGiven = true;
        }
        
        if (!targetString.IsEmpty())
        {
            csString newCmd;
            newCmd.Append("/rotate ");
            newCmd.Append(targetString);
            newCmd.Append(" ");
            
            if(targetGiven)
                newCmd.Append(words.GetTail(2).GetDataSafe());
            else
                newCmd.Append(words.GetTail(1).GetDataSafe());
            
            psUserCmdMessage cmdmsg(newCmd);
            cmdmsg.SendMessage();
        }
        else
            return "You need to precise a target or to select one";

    }

    else if (words[0] == "/sell")
    {
    	csString buff;
        if (words.GetCount() > 1){
        csString tail = words.GetTail(1);
            buff.Format("<R TYPE=\"SELL\" TARGET=\"%s\"/>",tail.GetData());
        }
        else
        {
            buff.Format("<R TYPE=\"SELL\"/>"); // If no target specified by user use active target
        }
        psGUIMerchantMessage exchange(psGUIMerchantMessage::REQUEST,buff);
        exchange.SendMessage();
    }
    else if ( words[0] == "/buddy" )
    {
        if (words.GetCount() < 2) //if there were no arguments open the buddy window
        {
            pawsWidget* window     = PawsManager::GetSingleton().FindWidget("BuddyWindow");
            if ( !window )
                return "Buddy List Not Found";
            else
                window->Show();
        }
        else //else send the data to the server for parsing
        {
            psUserCmdMessage cmdmsg(cmd);
            cmdmsg.SendMessage();
        }
    }
    else if (words[0] == "/buy")
    {
    	csString buff;
        if (words.GetCount() > 1){
        csString tail = words.GetTail(1);
            buff.Format("<R TYPE=\"BUY\" TARGET=\"%s\"/>",tail.GetData());
        }
        else
        {
        	buff.Format("<R TYPE=\"BUY\"/>"); // If no target specified by user use active target
        }
        psGUIMerchantMessage exchange(psGUIMerchantMessage::REQUEST,buff);
        exchange.SendMessage();
    }

    else if (words[0] == "/trade")
    {
        psExchangeRequestMsg exchange(true);
        exchange.SendMessage();
    }

    else if (words[0] == "/give")
    {
        psExchangeRequestMsg exchange(false);
        exchange.SendMessage();
    }

    else if (words[0] == "/ignore")
    {
        bool onoff = false;
        bool toggle = false;

        pawsIgnoreWindow* window     = (pawsIgnoreWindow*) PawsManager::GetSingleton().FindWidget("IgnoreWindow");
        if ( !window )
            return "Ignore Window Not Found";

        if (words.GetCount() < 2) //If the player didn't provide arguments just open the ignore window
            window->Show();
        else //If the player provided a name apply the setting
        {
            if (words[2] == "add")         //The player provided an on so set ignore
                onoff = true;
            else if (words[2] == "remove") //The player provided an off so unset ignore
                onoff = false;
            else                           //The player didn't provide anything so toggle the option
                toggle = true;

            csString person(words[1]);
            //normalize the name
            person.Downcase();
            person.SetAt(0,toupper(person.GetAt(0)));

            if(toggle) //No options were sent so toggle the option
            {
                if (window->IsIgnored(person)) //If the person is ignored...
                    window->RemoveIgnore(person); //Remove his/her ignore status
                else
                    window->AddIgnore(person);//else add the person to the ignore list
            }
            else
            {
                if(onoff) //The player requested explictly to add the ignore
                    window->AddIgnore(person);
                else if(window->IsIgnored(person)) //the player requested explictly to remove the ignore
                    window->RemoveIgnore(person);
            }
        }
    }

    else if (words[0] == "/cast")
    {
       if (words.GetCount() <= 1)
            return "You must specify a spell name";


       csString spell;
       float KFactor;
       if (words.GetCount() > 2 && sscanf(words[1], "%f", &KFactor) == 1)
       {
           spell = words.GetTail(2);
       }
       else
       {
           KFactor = psengine->GetKFactor();
           spell = words.GetTail(1);
       }

       if (KFactor < 0 || KFactor > 100)
           return "Spell power must be a percentage.";

       AskToSlayBeforeSending(new psSpellCastMessage(spell, KFactor));
    }

    else if (words[0] == "/away")
    {
        pawsChatWindow* ChatWindow = (pawsChatWindow*) PawsManager::GetSingleton().FindWidget("ChatWindow");

        if (words.GetCount() > 1)
            ChatWindow->SetAway( words.GetTail(1) );
        else
            ChatWindow->SetAway("");
    }

    else if (words[0] == "/clear")
    {
        printf("Clearing  history\n");
        pawsChatWindow* ChatWindow = (pawsChatWindow*) PawsManager::GetSingleton().FindWidget("ChatWindow");
        ChatWindow->Clear();
    }

    else if (words[0] == "/target")
    {
        if (words[1].IsEmpty()) {
            return "You can use /target [self|clear] or /target [prev|next] [item|npc|player|any].";
        } else if (words[1] == "self")
            psengine->GetCharManager()->SetTarget(psengine->GetCelClient()->GetMainPlayer(),"select");
        else
        {
            csString tail;
            if(words[1] == "next" || words[1] == "prev")
                tail = words.GetTail(2);
            else
                tail = words.GetTail(1);

            SearchDirection dir = (words[1] == "prev") ? SEARCH_BACK : SEARCH_FORWARD;

            if (tail == "item")
                UpdateTarget(dir,PSENTITYTYPE_ITEM);
            else if (tail == "npc")
                UpdateTarget(dir, PSENTITYTYPE_NON_PLAYER_CHARACTER);
            else if (tail == "player" || tail == "pc")
                UpdateTarget(dir, PSENTITYTYPE_PLAYER_CHARACTER);
            else if (tail == "any")
                UpdateTarget(dir, PSENTITYTYPE_NO_TARGET);
            else if (tail == "clear")
                psengine->GetCharManager()->SetTarget(NULL,"select");
            else
                psengine->GetCharManager()->SetTarget(FindEntityWithName(tail),"select");
        }
    }

    else if (words[0] == "/targetcontext")
    {
        GEMClientObject *object = NULL;

        if (words[1].IsEmpty())
            object = psengine->GetCharManager()->GetTarget();
        else
            object = FindEntityWithName(words[1]);

        if (object)
            psengine->GetCharManager()->SetTarget(object,"context");
    }

    else if (words[0] == "/use" ||
             words[0] == "/combine" ||
             words[0] == "/uncombine" ||
             words[0] == "/construct" ||
             words[0] == "/dig" ||
             words[0] == "/fish" ||
             words[0] == "/harvest" ||
             words[0] == "/repair" )
    {
        psWorkCmdMessage work(cmd);
        work.SendMessage();
    }

    else if (words[0] == "/picklock")
    {
        psLockpickMessage lockpick("");
        lockpick.SendMessage();
    }

    else if(words[0] == "/targetinfo")
    {
        pawsDetailWindow* detail = (pawsDetailWindow*)PawsManager::GetSingleton().FindWidget("DetailWindow");
        detail->RequestDetails();
        return NULL;
    }

    else if (words[0] == "/advisor" ) //this manages all the subcases of advisor: on, off, list, listsessions, reuquests
    {
        csString pPerson;
        csString pText;

        psAdviceMessage advice(0,words[1].GetDataSafe(),pPerson.GetDataSafe(), pText.GetDataSafe());
        advice.SendMessage();

        return NULL;
    }

    else if (words[0] == "/help" ) //used to request help
    {
        //get the chatwindow for use later
        pawsChatWindow* chatWindow = dynamic_cast<pawsChatWindow*>(PawsManager::GetSingleton().FindWidget("ChatWindow"));

        csString pPerson;
        csString pText( words.GetTail( 1 ) );

        if (pText.IsEmpty())
            return "You must enter the text. e.g /help [text]";

        if (chatWindow && chatWindow->GetSettings().enableBadWordsFilterOutgoing) //check for badwords filtering
            chatWindow->BadWordsFilter(pText); //if enabled apply it

        psAdviceMessage advice(0,words[0].GetDataSafe(),pPerson.GetDataSafe(), pText.GetDataSafe());
        advice.SendMessage();
        return NULL;
    }

    else if (words[0] == "/advice") //used to give help
    {
        //get the chatwindow for use later
        pawsChatWindow* chatWindow = dynamic_cast<pawsChatWindow*>(PawsManager::GetSingleton().FindWidget("ChatWindow"));

        if (words.GetCount() < 2)
            return "You must enter the text. e.g /Advice [user] <text>";
        csString pPerson( words[1] );
        csString pText(words.GetTail(2));

        if (chatWindow && chatWindow->GetSettings().enableBadWordsFilterOutgoing) //check for badwords filtering
            chatWindow->BadWordsFilter(pText); //if enabled apply it

        psAdviceMessage advice(0,words[0],pPerson, pText);
        advice.SendMessage();
        return NULL;
    }

    else if (  words[0] == "/pet")
    {
        const char *errorMsg = "You must enter the text. e.g /pet [petname,] <follow|stay|dismiss|summon|attack|guard|assist|name|target> <options>";

        if (words.GetCount() < 2)
            return errorMsg;

        csString pCommand = words[1];
        csString target, options;

        if ( pCommand.FindFirst( ',' ) != (size_t)-1 )
        {
            // Pet name specified
            if (words.GetCount() < 3)
                return errorMsg;
            pCommand = words[2];
            target = words[1];
            options = words.GetTail( 3 );
        }
        else
        {
            // No pet specified, use default
            pCommand = words[1];
            target.Clear();
            options = words.GetTail( 2 );
        }

        pCommand.Upcase();
        int command;
        if      ( pCommand == "FOLLOW" )    { command = psPETCommandMessage::CMD_FOLLOW;      }
        else if ( pCommand == "STAY" )      { command = psPETCommandMessage::CMD_STAY;        }
        else if ( pCommand == "DISMISS" )   { command = psPETCommandMessage::CMD_DISMISS;     }
        else if ( pCommand == "SUMMON" )    { command = psPETCommandMessage::CMD_SUMMON;      }
        else if ( pCommand == "ATTACK" )    { command = psPETCommandMessage::CMD_ATTACK;      }
        else if ( pCommand == "GUARD" )     { command = psPETCommandMessage::CMD_GUARD;       }
        else if ( pCommand == "ASSIST" )    { command = psPETCommandMessage::CMD_ASSIST;      }
        else if ( pCommand == "STOPATTACK" ){ command = psPETCommandMessage::CMD_STOPATTACK;  }
        else if ( pCommand == "NAME" )      { command = psPETCommandMessage::CMD_NAME;        }
        else if ( pCommand == "TARGET" )    { command = psPETCommandMessage::CMD_TARGET;      }
        else                                { return errorMsg; }

        switch ( command )
        {
        case psPETCommandMessage::CMD_DISMISS:
            {
                // If we're dismissing the pet, we should close the stat window
                pawsPetStatWindow* petwindow = (pawsPetStatWindow*)PawsManager::GetSingleton().FindWidget("PetStatWindow");
                if ( !petwindow )
                    return "Pet stat window not found!";
                petwindow->Hide();
            }
            break;
        case psPETCommandMessage::CMD_TARGET:
            // If no name give use target name if exists
            if (options.Length() == 0 && psengine->GetCharManager()->GetTarget())
            {
                options.Format("eid:%u", psengine->GetCharManager()->GetTarget()->GetEID().Unbox());
            }
        default:
            // Most cases do nothing
            break;
        }


        psPETCommandMessage cmd(0, command, target.GetData(), options.GetData());
        cmd.SendMessage();

        return NULL;
    }

    else if (words[0] == "/guard")
    {
        unsigned int onoff = 0;
        csString targetString;

        if (words[1].IsEmpty())
            targetString = FormatTarget();
        else if (words[1] == "on" || words[1] == "off")
        {
            onoff = 1;
            targetString = FormatTarget();
        }
        else
        {
            targetString = FormatTarget(words[1]);
            if(!words[2].IsEmpty())
                onoff = 2;
        }        
        
        if (!targetString.IsEmpty())
        {
            csString newCmd;
            newCmd.Append("/guard ");
            newCmd.Append(targetString);
            if(onoff > 0)
                newCmd.Append(" ");
                newCmd.Append(words[onoff]);
            psUserCmdMessage cmdmsg(newCmd);
            cmdmsg.SendMessage();
        }
        else
            return "You need to specify a target or to select one";
    }

    else if (words[0] == "/pickup")
    {
        csString targetString;
        if (words[1].IsEmpty())
            targetString = FormatTarget();
        else
            targetString = FormatTarget(words[1]);
            
        if (!targetString.IsEmpty())
        {
            csString newCmd;
            newCmd.Append("/pickup ");
            newCmd.Append(targetString);
            psUserCmdMessage cmdmsg(newCmd);
            cmdmsg.SendMessage();
        }
        else
            return "You need to specify a target or to select one";
    }

    else if ( words[0] == "/game" )
    {
        // Find the game board window and start a new game session.
        pawsGameBoard *gameWindow = dynamic_cast<pawsGameBoard *>
                (PawsManager::GetSingleton().FindWidget("GameBoardWindow"));
        if (gameWindow)
            gameWindow->StartGame();
        else
            Error1("Couldn't find widget GameBoardWindow");

        // The window will be shown when the server responds back with the game layout.
        return NULL;
    }

    else if (words[0] == "/attack")
    {
        AskToSlayBeforeSending(new psUserCmdMessage(cmd));
    }

    else if (words[0] == "/introduce")
    {
        psCharIntroduction introduce;
        introduce.SendMessage();
    }

    else if (words[0] == "/unstick")
    {
        psUserCmdMessage cmdmsg(cmd);
        cmdmsg.SendMessage();
        psengine->GetCharControl()->GetMovementManager()->StopAllMovement();
    }
    else if (words[0] == "/drop")
    {
        if (words.GetCount() < 2)
            return "Usage: /drop  [quantity] (any) (noguard) (inplace) [item name]";
        int quantity;
        csString itemName;
        bool any = false;
        bool guard = true;
        bool inplace = false;
        int moneySlot = 0;
        
        unsigned int i = 1;
        
        // if the first word is a number, use that for quantity
        quantity = atoi(words[1]);
        if (quantity)
        {
            i++;
        }
        else if (words[1] == "all")
        {
            quantity = 65;
            i++;
        }
        else
        {
            quantity = 1;
        }

        
        if(words[i] == "any")
        {
            i++;
            any = true;
        }
        
        if(words[i] == "noguard")
        {
            i++;
            guard = false;
        }
        
        if(words[i] == "inplace")
        {
            i++;
            inplace = true;
        }
        
        if ( words[i] == "tria" || (words[i] == "hexa" && (moneySlot=1)) ||
           (words[i] == "octa" && (moneySlot=2)) || (words[i] == "circle" && (moneySlot=3)) )
        {
            psSlotMovementMsg moneydropmsg( CONTAINER_INVENTORY_MONEY, moneySlot,
                                CONTAINER_WORLD, 0, quantity, 0, 0, guard, inplace );
            moneydropmsg.SendMessage();
            return NULL;
        }
        else
        {
            itemName = words.GetTail(i);
            psCmdDropMessage cmddrop(quantity, itemName, any, guard, inplace);
            cmddrop.SendMessage();
        }
    }
    else if(words[0] == "/emote")
    {
        if(words.GetCount() < 2 || words[1] == "list")
        {
            //generate a message
            csString emotelistmsg = "List of emotes:\n";
            for(unsigned int i=0; i < emoteList.GetSize(); i++)
            {
                emotelistmsg += emoteList[i].command.Slice(1);
                emotelistmsg += ", ";
            }
            pawsChatWindow* chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
            if(chatWindow)
                chatWindow->ChatOutput(emotelistmsg);
        }
        else
        {
            for(unsigned int i=0; i < emoteList.GetSize(); i++)
            {
                if(words[1].Find(emoteList[i].command) != (size_t)-1)
                {
                    psUserCmdMessage cmdmsg(emoteList[i].command);
                    cmdmsg.SendMessage();
                    break;
                }
            }
        }
    }
    else if(words[0] == "/mount")
    {
         GEMClientObject *object = NULL;

         if (words[1].IsEmpty())
             object = psengine->GetCharManager()->GetTarget();
         else
             object = FindEntityWithName(words[1]);
         if (object)
         {
             psengine->GetCharManager()->SetTarget(object,"select");
             EID mappedID = object->GetEID();
             csString newCmd;
             newCmd.Format("/mount eid:%u", mappedID.Unbox());
             psUserCmdMessage cmdmsg(newCmd);
             cmdmsg.SendMessage();
         }
         else
            return "Mount not found!";
    }
    else if(words[0] == "/unmount")
    {
        csString newCmd;
        newCmd.Format("/unmount");
        psUserCmdMessage cmdmsg(newCmd);
        cmdmsg.SendMessage();
    }
    else if(words[0] == "/?")
    {
    	pawsWidget* widget = PawsManager::GetSingleton().FindWidget("HelpWindow");
    	if(widget)
    		widget->Show();
    }
    else if(words[0] == "/join")
    {
    	if(words.GetCount() < 2)
    	{
    		return "Please specify a channel name to join.";
    	}
    	else
    	{
    		pawsChatWindow* ChatWindow = (pawsChatWindow*) PawsManager::GetSingleton().FindWidget("ChatWindow");
    		ChatWindow->JoinChannel(words.GetTail(1));
    	}
    }
    else if(words[0] == "/leave")
	{
		if(words.GetCount() < 2)
		{
			return "Please specify a channel hotkey number to leave.";
		}
		else
		{
			pawsChatWindow* ChatWindow = (pawsChatWindow*) PawsManager::GetSingleton().FindWidget("ChatWindow");
			int hotkeyChannel = words.GetInt(1);
			if(hotkeyChannel < 1 || hotkeyChannel > 10)
			{
				return "Please specify a channel hotkey number to leave.";
			}
			if(!ChatWindow->LeaveChannel(hotkeyChannel))
				return "You have not joined that channel.";
		}
	}
    else
    {
        psUserCmdMessage cmdmsg(cmd);
        cmdmsg.SendMessage();
    }

    return NULL;  // don't display anything here
}

void psUserCommands::HandleMessage(MsgEntry *msg)
{
    (void) msg;
}

// Starting from startingEntity, this function returns the next nearest or
// furthest PC, NPC or ITEM.  If startingEntity is NULL, it returns the the nearest
// or furthest PC, NPC or ITEM from the player.  If the search doesn't find a new
// target because we are already at the furthest or nearest entity, it cycles
// around to the nearest or furthest entity respectively.
void psUserCommands::UpdateTarget(SearchDirection searchDirection,
                                  EntityTypes entityType)
{
    GEMClientObject* startingEntity = psengine->GetCharManager()->GetTarget();
    psCelClient* cel = psengine->GetCelClient();

    GEMClientObject* myEntity = cel->GetMainPlayer();
    csVector3 myPos = myEntity->GetPosition();

    float seDistance;
    csVector3 sePos;
    if (startingEntity != NULL)
    {
        sePos = startingEntity->GetPosition();
        seDistance = csSquaredDist::PointPoint(myPos, sePos);
    }
    else
    {
        seDistance = (searchDirection == SEARCH_FORWARD) ? 0 : FLT_MAX;
    }

    float max_range = NEARBY_TARGET_MAX_RANGE;

    if (entityType == PSENTITYTYPE_ITEM)
    {
       max_range = RANGE_TO_SELECT;
    }

    csArray<GEMClientObject*> entities = cel->FindNearbyEntities(myEntity->GetSector(),
                                                                 myPos,
                                                                 max_range);

    // Best entity is the next entity in a search.
    GEMClientObject* bestObject = startingEntity;
    float bestDistance = FLT_MAX;

    // Loop entity is the entity returned if we're already at the last entity
    // and need to cycle through.
    GEMClientObject* loopObject = startingEntity;
    float loopDistance = (searchDirection == SEARCH_FORWARD) ? FLT_MAX : 0;

    // Iterate through the entity list looking for the nearest one.
    size_t entityCount = entities.GetSize();

    for (size_t i = 0; i < entityCount; ++i)
    {
        GEMClientObject* object = entities[i];

        GEMClientObject* other = ( startingEntity == NULL ) ? NULL : startingEntity;

        if (object == myEntity || object == other)
        {
            continue;
        }

        CS_ASSERT( object );

        // Skip if it's not the type of entity we're looking for.
        int eType = object->GetType();

        if ((entityType == PSENTITYTYPE_PLAYER_CHARACTER && eType < 0)
            || (entityType == PSENTITYTYPE_NON_PLAYER_CHARACTER && (eType >= 0 || eType == -2))
            || (entityType == PSENTITYTYPE_ITEM && eType != -2))
            continue;

        csVector3 pos = object->GetPosition();

        // Calculate the squared distance, update if we found a better one.
        float distFromMe = csSquaredDist::PointPoint(myPos, pos);

        // This is the distance from the starting entity to the current one.
        // If it's negative, the current entity is invalid for the search
        // (it can still be a valid loop entity, but never a best entity.)
        float dist = ((startingEntity != NULL)
                      ? ((searchDirection == SEARCH_FORWARD)
                         ? ((seDistance < distFromMe)
                            ? csSquaredDist::PointPoint(sePos, pos)
                            : -1)
                         : ((seDistance > distFromMe)
                            ? csSquaredDist::PointPoint(sePos, pos)
                            : -1))
                      : seDistance - distFromMe);

        // Update loop entity.
        if ((searchDirection == SEARCH_FORWARD && distFromMe < loopDistance)
            || (searchDirection == SEARCH_BACK && distFromMe > loopDistance))
        {
            loopObject = object;
            loopDistance = distFromMe;
        }

        // If dist < 0, the entity is on the wrong side of the starting
        // entity.
        if (dist < 0)
        {
            continue;
        }

        // Update best entity.
        if (dist < bestDistance)
        {
            bestObject = object;
            bestDistance = dist;
        }
    }

    return psengine->GetCharManager()->SetTarget((bestObject == startingEntity) ? loopObject : bestObject,"select");
}

GEMClientObject* psUserCommands::FindEntityWithName(const char *name)
{
    psCelClient* cel = psengine->GetCelClient();
    csVector3 myPos = cel->GetMainPlayer()->GetPosition();

    // Find all entities within a certain radius.
    csArray<GEMClientObject*> entities = cel->FindNearbyEntities(cel->GetMainPlayer()->GetSector(),
                                                                 myPos,
                                                                 NEARBY_TARGET_MAX_RANGE);

    // Iterate through the entity list looking for one with the right name.
    size_t entityCount = entities.GetSize();

    for (size_t i = 0; i < entityCount; ++i)
    {
        // Get the next entity, skip if it's me or the starting entity.
        GEMClientObject* object = entities[i];
        CS_ASSERT( object );

        if (csString(object->GetName()).StartsWith(name, true))
            return object;
    }

    return NULL;
}

csString psUserCommands::FormatTarget(const csString& target)
{
    GEMClientObject *targetObject = NULL;
    
    if(target == "target" || target.IsEmpty())
        targetObject = psengine->GetCharManager()->GetTarget();
    else if (target.StartsWith("area:",true) ||
        target.StartsWith("eid:",true) ||
        target.StartsWith("itemid:",true) ||
        target.StartsWith("pid:",true) ||
        target ==  "me")
        return target;
    else
    {
        targetObject = FindEntityWithName(target);
    }
    
    csString targetString;
    
    if (targetObject)
    {
        psengine->GetCharManager()->SetTarget(targetObject,"select");
        EID mappedID = targetObject->GetEID();
        targetString.Format("eid:%u", mappedID.Unbox());
        return targetString;
    }

    targetString.Empty();
    return targetString;
}
