/*
 * questmanager.cpp
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
#include <ctype.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/log.h"
#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psxmlparser.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/dictionary.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "questmanager.h"
#include "gem.h"
#include "playergroup.h"
#include "cachemanager.h"
#include "netmanager.h"
#include "psserver.h"
#include "client.h"
#include "events.h"
#include "entitymanager.h"
#include "globals.h"



QuestManager::QuestManager()
{
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<QuestManager>(this,&QuestManager::HandleQuestInfo),  MSGTYPE_QUESTINFO,REQUIRE_READY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<QuestManager>(this,&QuestManager::HandleQuestReward),MSGTYPE_QUESTREWARD,REQUIRE_READY_CLIENT|REQUIRE_ALIVE);
}

QuestManager::~QuestManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_QUESTINFO);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_QUESTREWARD);
    if (dict) delete dict;
}


bool QuestManager::Initialize()
{
    if (!dict)
    {
        dict = new NPCDialogDict;

        if (!dict->Initialize(db))
        {
            delete dict;
            dict=NULL;
            return false;
        }
    }

    return LoadQuestScripts();
}


bool QuestManager::LoadQuestScripts()
{
    // Load quest scripts from database.
    Result quests(db->Select("SELECT * from quest_scripts order by quest_id"));
    if (quests.IsValid())
    {
        int i,count=quests.Count();

        for (i=0; i<count; i++)
        {
            int quest_id = quests[i].GetInt("quest_id");

            if (quest_id == -1) continue; // -1 for quest id, than it is a KAs so don't load quest.

            psQuest* currQuest = CacheManager::GetSingleton().GetQuestByID(quest_id);
            if (!currQuest) 
            {
                Error3("ERROR quest %s not found for quest script %s!  ",quests[i]["quest_id"], quests[i]["id"]); 
                return false;
            }
            
            if (!currQuest->PostLoad())
            {
                Error2("ERROR Loading quest prerequisites for quest %s!  ",quests[i]["quest_id"] );
                return false;
            }
        }
        for (i=0; i<count; i++)
        {
            int line = ParseQuestScript(quests[i].GetInt("quest_id"),quests[i]["script"]);
            if (line)
            {
                Error3("ERROR Parsing quest script %s, line %d!  ",quests[i]["quest_id"], line );
                return false;
            }
        }
        return true;
    }

    return false;
}

bool QuestManager::LoadQuestScript(int id)
{
    // Load quest scripts from database.
    Result quests(db->Select("SELECT * from quest_scripts where quest_id=%d", id));
    if (quests.IsValid())
    {
        int i,count=quests.Count();

        for (i=0; i<count; i++)
        {
            int line = ParseQuestScript(id,quests[i]["script"]);
            if (line)
            {
                Error3("ERROR Parsing quest script %d, line %d!  ",id, line );
                return false;
            }
        }
        return true;
    }
    return false;
}

void QuestManager::GetNextScriptLine(psString& scr, csString& block, size_t& start, int& line_number)
{
    csString line;

    // Get the first line
    scr.GetLine(start,line); 
    line_number++;
    block = line;
    start += line.Length();

    // If there is more in the script, check for subsequent lines that are indented and append
    if (start < scr.Length())
    {
        start += scr.GetAt(start)=='\r';
        start += scr.GetAt(start)=='\n';

        // now get subsequent lines if indented
        while (start < scr.Length() && isspace(scr.GetAt(start)))
        {
            scr.GetLine(start,line); 
            line_number++;
            block.Append(line);
            start += line.Length();
            if (start < scr.Length())
            {
                start += scr.GetAt(start)=='\r';
                start += scr.GetAt(start)=='\n';
            }
        }
    }
}

bool QuestManager::PrependPrerequisites(csString &substep_requireop, 
                                        csString &response_requireop,
                                        bool quest_assigned_already,
                                        NpcResponse *last_response,
                                        psQuest *mainQuest)
{
    // Prepend prerequisites for this trigger.

    // First is the quest needed to be assigned or not
    csString op = "<pre>";
    csString post = "</pre>";

    if (substep_requireop || response_requireop)
    {
        op.Append("<and>");
        post = "</and></pre>";
    }

    if (quest_assigned_already)
    {
        // If quest has been assigned in the script, we need to have every response
        // verify that the quest have been assigned.
        op.AppendFmt("<assigned quest=\"%s\" />", mainQuest->GetName());
    } 
    else
    {
        // If quest has not been assigned in the script, we need to have every response
        // verify that the quest have not been assigned.
        op.AppendFmt("<not><assigned quest=\"%s\" /></not>", mainQuest->GetName());
    }

    if (substep_requireop) 
    {
        op.Append(substep_requireop);
    }

    if (response_requireop) 
    {
        op.Append(response_requireop);

        // Response require op only valid for one response.
        response_requireop.Free();
    }

    op.Append(post);


    // Will insert the new op in and new "and" list if not present already in the script.
    if (!last_response->ParsePrerequisiteScript(op,true)) // Insert at start of list
    {
        Error2("Could not append '%s' to prerequisite script!",op.GetData());
        return false;
    }
    else
    {
        Debug2( LOG_QUESTS, 0,"Parsed '%s' successfully.",op.GetData() );
    }
    return true;
}

bool QuestManager::HandlePlayerAction(csString& block, size_t& which_trigger,csString& current_npc,csStringArray& pending_triggers)
{
    WordArray words(block);
    if (words[1].CompareNoCase("gives"))
    {
        which_trigger = 0;

        int numwords = GetNPCFromBlock(words,current_npc);
        if (numwords==-1) 
        {
            Error3("NPC '%s' is not present in db, but used in %s!",words[2].GetData(),block.GetData());
        }

        csString itemlist;
        if (ParseItemList(words.GetTail(2+numwords), itemlist))
        {
            pending_triggers.Empty();
            pending_triggers.Push(itemlist);  // next response will use this itemlist
        }
        else
            return false;
    }
    else 
    {
        Error2("Unknown Player function in '%s' !",block.GetData());
        return false;
    }
    return true;
}

bool QuestManager::HandleScriptCommand(csString& block,
                                       csString& response_requireop,
                                       csString& substep_requireop,
                                       NpcResponse *last_response,
                                       psQuest *mainQuest,
                                       bool& quest_assigned_already,
                                       psQuest *quest)
{
    csString op;
    csString previous;

    block.Trim();

    // Check for multiple commands separated by dots
    csArray<csString> commands;

    // Loop through and find all dot separated commands.
    size_t dot = block.Find(".");
    size_t l = block.Length();
    while (dot != SIZET_NOT_FOUND && dot+1 < l)
    {
        csString first = block.Slice(0,block.Find("."));
        commands.Push(first.Trim());

        block = block.Slice(block.Find(".")+1,block.Length());
        dot = block.Find(".");
    }

    // If there are more last command didn't have a dot
    // make sure we include that command to.
    if (block.Length())
        commands.Push(block.Trim());

    for (size_t i = 0 ; i < commands.GetSize () ; i++)
    {
        block = commands.Get(i);
        // Take off trailing dots(.)
        if (block[block.Length()-1] == '.')
        {
            block.DeleteAt(block.Length()-1);
        }
                
        if (!strncasecmp(block,"Assign Quest",12))
        {
            op.Format("<assign q1=\"%s\" />",mainQuest->GetName());
            quest_assigned_already = true;
        }
        
        else if ( !strncasecmp(block, "FireEvent", 9) )
        {
            csString eventname = block.Slice(9,block.Length()-1).Trim();
            op.Format("<fire_event name='%s'/>", eventname.GetData());
        }         
        
        else if (!strncasecmp(block,"Complete",8))
        {
            csString questname = block.Slice(8,block.Length()-1).Trim();
            op.Format("<complete quest_id=\"%s\" />",questname.GetData());
        }
        else if (!strncasecmp(block,"Give",4))
        {
            WordArray words(block);
            if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("tria")) // give tria money
            {
                op.Format("<money value=\"0,0,0,%d\" />",words.GetInt(1) );
            }
            else if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("hexa")) // give hexa money
            {
                op.Format("<money value=\"0,0,%d,0\" />",words.GetInt(1) );
            }
            else if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("octa")) // give octa money
            {
                op.Format("<money value=\"0,%d,0,0\" />",words.GetInt(1) );
            }
            else if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("circle")) // give circle money
            {
                op.Format("<money value=\"%d,0,0,0\" />",words.GetInt(1) );
            }
            else if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("exp")) // give experience points
            {
                op.Format("<run scr=\"give_exp\" param0=\"%d\" />",words.GetInt(1) );  
            }
            else if (words.GetInt(1) != 0 && words.Get(2).CompareNoCase("faction") && words.GetCount() > 3) // give faction points
            {
                op.Format("<faction name=\"%s\" value=\"%d\" />", words.GetTail(3).GetDataSafe(), words.GetInt(1) );  
            }
            else
            {
                if (words.FindStr("or") != SIZET_NOT_FOUND)
                {
                    op.Format("<offer>");
                    size_t start = 1,end;
                    while (start < words.GetCount() )
                    {
                        end = words.FindStr("or",(int)start);
                        if (end == SIZET_NOT_FOUND)
                            end = words.GetCount();
                        csString item = words.GetWords(start,end);
                        op.AppendFmt("<item name=\"%s\" />",item.GetData() );
                        start = end+1;
                    }
                    op.Append("</offer>");
                }
                else
                {
                    csString count;
                    int item_start = 1;
                    if (words.GetInt(1) != 0)
                    {
                        count.Format("count=\"%d\" ",words.GetInt(1));
                        item_start+=1;
                    }
                    op.Format("<give item=\"%s\" %s/>",words.GetTail(item_start).GetData(),count.GetDataSafe() );
                }
            }

        }
        else if (!strncasecmp(block,"Require completion of",21)) 
        {
            csString questname = block.Slice(21,block.Length()-1).Trim();
            response_requireop.AppendFmt("<completed quest=\"%s\" />", questname.GetData() );
        }
        else if (!strncasecmp(block,"Require no completion of",24)) 
        {
            csString questname = block.Slice(24,block.Length()-1).Trim();
            response_requireop.AppendFmt("<not><completed quest=\"%s\" /></not>", questname.GetData() );
        }
        else if (!strncasecmp(block,"NoRepeat",8)) 
        {
            substep_requireop.AppendFmt("<not><completed quest=\"%s\" /></not>", quest->GetName() );
        }
        else if (!strncasecmp(block,"Run script",10)) 
        {
            csString script = block.Slice(10,block.Length()-1).Trim();

            // Find params if any, up to 3
            csString param[3];
            int p = 0;
            size_t start = script.FindStr("(");
            size_t end   = script.FindStr(")");
            if (start != SIZET_NOT_FOUND && end != SIZET_NOT_FOUND && 
                start == 0 && end > start)
            {
                csString params = script.Slice(start+1,end-start-1).Trim();
                script.DeleteAt(start,end-start+1).Trim();
                size_t next;
                do 
                {
                    next = params.FindStr(",");
                    if (next == SIZET_NOT_FOUND)
                    {
                        param[p] = params.Trim();
                    }
                    else
                    {
                        param[p] = params.Slice(0,next).Trim();
                        params.DeleteAt(0,next+1);
                    }
                    p++;
                } while (next != SIZET_NOT_FOUND && p < 3);

            }

            // Build the op
            op.Format("<run scr=\"%s\"",script.GetDataSafe());
            for (int i = 0; i < p; i++)
            {
                op.AppendFmt(" param%d=\"%s\"",i,param[i].GetDataSafe());
            }
            op.Append("/>");
        }
        else if (!strncasecmp(block,"DoAdminCmd",10)) 
        {
            csString command = block.Slice(11).Trim();
            op.Format("<doadmincmd command=\"%s\"/>", command.GetData() ); 
        }
        else if (!strncasecmp(block,"Require time of day",19)) 
        {
            csString data = block.Slice(20);
            csArray<csString> timeinfo = psSplit(data, '-');
            if (timeinfo.GetSize() == 2)
            {
                response_requireop.AppendFmt("<timeofday min=\"%s\" max=\"%s\" />", timeinfo[0].GetData(), timeinfo[1].GetData() );
            }
        }
        else if (!strncasecmp(block,"Require not time of day",23)) 
        {
            csString data = block.Slice(24);
            csArray<csString> timeinfo = psSplit(data, '-');
            if (timeinfo.GetSize() == 2)
            {
                response_requireop.AppendFmt("<not><timeofday min=\"%s\" max=\"%s\" /></not>", timeinfo[0].GetData(), timeinfo[1].GetData() );
            }
        }
        else if (!strncasecmp(block,"Require guild",13))  //NOTE: the both argument is implictly defined
        {
            csString type = block.Slice(14,block.Length()).Trim();
            response_requireop.AppendFmt("<guild type=\"%s\" />", type.Length() ? type.GetData() : "both" );
        }
        else if (!strncasecmp(block,"Require not guild",17))  //NOTE: the both argument is implictly defined
        {
            csString type = block.Slice(18,block.Length()).Trim();
            response_requireop.AppendFmt("<not><guild type=\"%s\" /></not>", type.Length() ? type.GetData() : "both" );
        }
        else if (!strncasecmp(block,"Require active magic",20)) 
        {
            csString magicname = block.Slice(21,block.Length()).Trim();
            response_requireop.AppendFmt("<activemagic name=\"%s\" />", magicname.GetData() );
        }
        else if (!strncasecmp(block,"Require not active magic",24)) 
        {
            csString magicname = block.Slice(25,block.Length()).Trim();
            response_requireop.AppendFmt("<not><activemagic name=\"%s\" /></not>", magicname.GetData() );
        }
        else if (!strncasecmp(block,"Require known spell",19))
        {
            csString magicname = block.Slice(20,block.Length()).Trim();
            response_requireop.AppendFmt("<knownspell name=\"%s\" />", magicname.GetData() );
        }
        else if (!strncasecmp(block,"Require not known spell",23)) 
        {
            csString magicname = block.Slice(24,block.Length()).Trim();
            response_requireop.AppendFmt("<not><knownspell name=\"%s\" /></not>", magicname.GetData() );
        }
        else if (!strncasecmp(block,"Require race",12)) 
        {
            csString racename = block.Slice(13,block.Length()).Trim();
            response_requireop.AppendFmt("<race name=\"%s\" />", racename.GetData() );
        }
        else if (!strncasecmp(block,"Require not race",16)) 
        {
            csString racename = block.Slice(17,block.Length()).Trim();
            response_requireop.AppendFmt("<not><race name=\"%s\" /></not>", racename.GetData() );
        }
        else if (!strncasecmp(block,"Require gender",14))
        {
            csString gender = block.Slice(15,block.Length()).Trim();
            
            if(gender == "male") gender = "M";
            else if(gender == "female") gender = "F";
            else if(gender == "neutral") gender = "N";

            response_requireop.AppendFmt("<gender type=\"%s\" />", gender.GetData() );
        }
        else if (!strncasecmp(block,"Require not gender",18)) 
        {
            csString gender = block.Slice(19,block.Length()).Trim();

            if(gender == "male") gender = "M";
            else if(gender == "female") gender = "F";
            else if(gender == "neutral") gender = "N";

            response_requireop.AppendFmt("<not><gender type=\"%s\" /></not>", gender.GetData() );
        }
        else if (!strncasecmp(block,"Require married",15)) 
        {
            response_requireop.AppendFmt("<married />");
        }
        else if (!strncasecmp(block,"Require not married",19)) 
        {
            response_requireop.AppendFmt("<not><married /></not>");
        }
        else if (!strncasecmp(block,"Introduce",9)) 
        {
            csString charname = block.Slice(10).Trim();
            if (!charname.IsEmpty())
                op.Format("<introduce name=\"%s\"/>", charname.GetData() ); 
            else
                op.Format("<introduce/>"); 
        }
        else // unknown block
        {
            Error2("Unknown command '%s' !",block.GetData());
            return false;
        }

        previous.Append(op);
        op.Empty(); // Don't want to include the same op multiple times.

    } // end for() commands

    op = "<response>";
    op.Append(previous);
    op.Append("</response>");

    // add script to last response
    if (!last_response->ParseResponseScript(op))
    {
        Error2("Could not append '%s' to response script!",op.GetData());
        return false;
    }
    else
    {
        Debug2( LOG_QUESTS, 0,"Parsed successfully and added to last response: %s .", op.GetData() );
    }
    return true;
}


int QuestManager::ParseQuestScript(int quest_id, const char *script)
{
    psString scr(script);
    size_t start = 0;
    csString line,block;
    csStringArray pending_triggers;
    int last_response_id=-1,next_to_last_response_id=-1;
    size_t which_trigger=0;
    int step_count=1; // Main quest is step 1
    csString current_npc;
    csString response_text,file_path;
    NpcResponse *last_response=NULL,*error_response=NULL;
    bool quest_assigned_already = false;
    csString response_requireop; // Accumulate prerequisites for next response
    csString substep_requireop;  // Accumulate prerequisites for current substep
    psQuest *mainQuest = CacheManager::GetSingleton().GetQuestByID(quest_id);
    psQuest *quest = mainQuest; // Substep is main step until substep is defined.
    int line_number = 0;
	NpcDialogMenu *pending_menu = NULL;
    NpcDialogMenu *last_menu    = NULL;

    if (!mainQuest && quest_id > 0)
    {
      CPrintf(CON_CMDOUTPUT,"No main quest for quest_script with quest_id %d\n", quest_id);
      lastError.Format("No Main quest could be found.");
      return -1;
    }
      

    while (start < scr.Length())
    {
        GetNextScriptLine(scr,block,start,line_number);

        // now we have the block to do something with
        if (!strncasecmp(block,"#",1)) // comment, skip it
          continue;

        if (!strncasecmp(block,"P:",2))  // P: is Player:, which means this is a trigger
        {
            pending_triggers.Empty();
            if (!BuildTriggerList(block,pending_triggers))
            {
                Error3("Could not determine triggers in script '%s', in line <%s>",
                       mainQuest->GetName(),block.GetData());
                lastError.Format("Could not determine triggers in script '%s', in line <%s>", mainQuest->GetName(),block.GetData());
                       
                return line_number;
            }
            // When parsing responses, this tracks which one goes with which
            which_trigger = 0;

            for (size_t i=0; i<pending_triggers.GetSize(); i++)
            {
                Debug2( LOG_QUESTS, 0,"Player says '%s'", pending_triggers[i]);
            }
        }
		else if (!strncasecmp(block,"Menu:",5))  // Menu: is an nice way of represention the various P: triggers
        {
			NpcDialogMenu *menu = new NpcDialogMenu();

			if (!BuildMenu(block, pending_triggers, mainQuest, menu))
            {
                Error3("Could not determine menu triggers in script '%s', in line <%s>",
                       mainQuest->GetName(),block.GetData());
                lastError.Format("Could not determine triggers in script '%s', in line <%s>", mainQuest->GetName(),block.GetData());
                       
                return line_number;
            }

			if (last_response)  // popup is part of a dialog chain
            {
				last_response->menu = menu;  // attach the menu to the prior response for easy access in chains
                last_menu = menu;           // save so we can add prerequisites later
            }
			else
            {
				pending_menu = menu;  // save for when we know the npc.  cannot attach to anything yet.
            }
        }
        else if (strchr(block,':')) // text response 
        {
            csString him,her,it,them,npc_name;
            block.SubString(npc_name,0,block.FindFirst(':'));  // pull out name before colon
            if (current_npc.Find(npc_name) == 0)  // if npc_name is the beginning of the current npc name, then it is a repeat
            {
                // Need to add this trigger menu to the generic menu for this npc if same npc follows a "..."
//   				if (last_response_id == -1 && pending_menu)
//				{
//					dict->AddMenu(current_npc, pending_menu);
//					pending_menu = NULL;
//				}

            }
            else // switch NPCs here
            {
                current_npc = npc_name;
                next_to_last_response_id = last_response_id = -1;  // When you switch NPCs, the prior responses must be reset also.
//   				if (pending_menu)
//				{
//					dict->AddMenu(current_npc, pending_menu);
//					pending_menu = NULL;
//				}
            }
            if (!GetResponseText(block,response_text,file_path,him,her,it,them))
            {
                Error2("Could not get response text out of <%s>!  Failing.",block.GetData() );
                lastError.Format("Could not get response text out of <%s>!  Failing.",block.GetData() );
                return line_number;
            }
            if (pending_triggers.GetSize() == 0 || which_trigger >= pending_triggers.GetSize() )
            {
                Error2("Found response <%s> without a preceding trigger to match it.",response_text.GetData() );
                lastError.Format("Found response <%s> without a preceding trigger to match it.",response_text.GetData() );
                return line_number;
            }

            Debug4( LOG_QUESTS, 0,"NPC %s responds with '%s', with the voice file '%s'", current_npc.GetData(),
                                response_text.GetData(), file_path.GetDataSafe() );

            // Now add this response to the npc dialog dict
            if (which_trigger == 0) // new sequence
                next_to_last_response_id = last_response_id;

            last_response = AddResponse(current_npc,response_text,last_response_id,quest,him,her,it,them,file_path);
            if (last_response)
            {
                bool ret = AddTrigger(current_npc,pending_triggers[which_trigger++],next_to_last_response_id,last_response_id, quest, "");
                if (!ret)
                {
                    lastError.Format("Trigger could not be added on line %d", line_number);                
                    return line_number;
                }                    

                if (mainQuest) // Prerequisites only apply to quest scripts, not KA scripts.
                {
                    if (!PrependPrerequisites(substep_requireop, response_requireop, quest_assigned_already,last_response,mainQuest))
                    {
                        lastError.Format("PrependPrerequistes failed on line %d", line_number);
                        return line_number;
                    }
                }
				if (pending_menu)
				{
                    // Now go back to the previous menu
                    pending_menu->SetPrerequisiteScript( last_response->GetPrerequisiteScript() );
					dict->AddMenu(current_npc, pending_menu);
					pending_menu = NULL;
				}
                else if (last_menu)
                {
                    last_menu->SetPrerequisiteScript( last_response->GetPrerequisiteScript() );
                    last_menu = NULL;
                }
            }
            else
            {
                return line_number;
            }
        }
        else if (!strncasecmp(block,"Player ",7)) // player does something
        {
            if (!HandlePlayerAction(block,which_trigger,current_npc,pending_triggers))
                return line_number;
        }
        else if (!strncmp(block,"...",3)) // New substep. Syntax: "... [NoRepeat]"
        {
            if (block.Length() > 3 && block.GetAt(3) != ' ')
            {
                Error4("No space after ... for quest '%s' at line %d: %s",
                       mainQuest->GetName(), line_number, block.GetDataSafe());
                lastError.Format("No space after ... for quest '%s' at line %d: %s", mainQuest->GetName(), line_number, block.GetDataSafe());                       
                return line_number;
            }

            // This clears out prior responses whether there is a quest or a KA script here
            next_to_last_response_id = last_response_id = -1;
			last_response = NULL;  // This is used for popup menus
            substep_requireop.Free();

            if (mainQuest)
            {
                WordArray words(block);

                // generate a sub step quest for the next block
                step_count++; // increment substep count
                csString newquestname;
                newquestname.Format("%s Step %d",mainQuest->GetName(),step_count);
                Debug2( LOG_QUESTS, 0,"Quest <%s> is getting added dynamically.",newquestname.GetData());
                quest = CacheManager::GetSingleton().AddDynamicQuest(newquestname, mainQuest, step_count);
                quest_id = quest->GetID();
            
                // Check if this is a non repeatable substep. 
                // Note: NoRepeat can either be an option to ... or a separate command.
                if (words[1].CompareNoCase("NoRepeat"))
                {
                    substep_requireop.AppendFmt("<not><completed quest=\"%s\" /></not>", quest->GetName() );
                }
            }
        }
        else // command
        {
            Debug2( LOG_QUESTS, 0,"Got command '%s'", block.GetData() );

            if (!HandleScriptCommand(block,
                                    response_requireop,substep_requireop,
                                    last_response,
                                    mainQuest,quest_assigned_already,quest))
                return line_number;
        }
    }

    if (quest_assigned_already && last_response)
    {
        // Make sure the quest is 'completed' at the end of the script.
        csString op;
        op.Format("<response><complete quest_id=\"%s\" /></response>",mainQuest->GetName());
        if (!last_response->ParseResponseScript(op))
        {
            Error2("Could not append '%s' to response script!",op.GetData());
            lastError.Format("Could not append '%s' to response script! ( line %d )",op.GetData(), line_number);
            return line_number;
        }
        else
        {
            Debug2( LOG_QUESTS, 0,"Parsed %s successfully.", op.GetData() );
        }
    }
    else if (quest_id>0) // negative numbers mean generic KA dialog
    {
        Error2("Quest script <%s> never assigned a quest or had any responses.",mainQuest->GetName());
        return line_number;
    }
    return 0;  // 0 is success!
}

int QuestManager::GetNPCFromBlock(WordArray words,csString& current_npc)
{
    csString select;

    // First check single name: "Player gives Sharven ..."
    csString first = words.Get(2);
    select.Format ("SELECT * from characters where name='%s' and lastname='' and npc_master_id!=0",first.GetData() );
    // check if NPC exists
    Result npc_db(db->Select(select));
    if (npc_db.IsValid() && npc_db.Count()>0) {
      current_npc = first;
      return 1;
    }
    else // Than check double name: "Player gives Menlil Toresun ..."
    {
        csString last = words.Get(3);
        
        select.Format("SELECT * from characters where name='%s' and lastname='%s' and npc_master_id!=0",first.GetData(),last.GetData());
        Result npc_db(db->Select(select));
        if (npc_db.IsValid() && npc_db.Count()>0) 
        {
            current_npc.Format("%s %s",first.GetData(),last.GetData() );
            return 2;
        }
    }
    return -1;
}

bool QuestManager::ParseItemList(const csString & input, csString & parsedItemList)
{
    // Eat the trailing period...
    csString tidyInput(input);
    tidyInput.RTrim();
    if (tidyInput.Length() > 1 && tidyInput[tidyInput.Length()-1] == '.')
        tidyInput.DeleteAt(tidyInput.Length()-1);

    // Syntax is a comma separated list... "Player gives Smith 5 Gold Ore, 4 Iron Ore."
    csStringArray inputItems;
    inputItems.SplitString(tidyInput, ",", csStringArray::delimIgnore);

    psStringArray xmlItems;
    psMoney money;

    for (size_t i = 0; i < inputItems.GetSize(); i++)
    {
        if (!ParseItem(inputItems[i], xmlItems, money))
            return false;
    }

    // We sort the list of items, and expect ExchangeManager to do the same...
    // That way, items can be offered in any order.
    xmlItems.Sort();

    parsedItemList.Format("<l money=\"%s\">", money.ToString().GetData());
    for (size_t i = 0; i < xmlItems.GetSize(); i++)
    {
        parsedItemList.Append(xmlItems[i]);
    }
    parsedItemList.Append("</l>");

    Debug2(LOG_QUESTS, 0, "Item list parsing created this: %s", parsedItemList.GetData());
    return true;
}

bool QuestManager::ParseItem(const char *text, psStringArray & xmlItems, psMoney & money)
{
    WordArray words(text);
    int itemCount;
    csString itemName;

    if (words.GetCount() < 1)
        return false;

    // Get the number of items required, if specified...otherwise assume 1.
    // The rest of the text is the item name.
    if (words.GetInt(0))
    {
        itemCount = words.GetInt(0);
        itemName = words.GetTail(1);
    }
    else
    {
        itemCount = 1;
        itemName = words.GetTail(0);
    }
    itemName.Collapse();

    // check if the item exists in db
    if (itemName.IsEmpty() || !CacheManager::GetSingleton().GetBasicItemStatsByName(itemName))
    {
        Error2("ERROR Loading quests: Item %s doesn't exist in database", itemName.GetDataSafe());
        lastError.Format("Item %s does not exist", itemName.GetDataSafe());
        return false;
    }

    if (itemName.CompareNoCase("Circle"))
    {
        money.AdjustCircles(itemCount);
    }
    else if (itemName.CompareNoCase("Octa"))
    {
        money.AdjustOctas(itemCount);
    }
    else if (itemName.CompareNoCase("Hexa"))
    {
        money.AdjustHexas(itemCount);
    }
    else if (itemName.CompareNoCase("Tria"))
    {
        money.AdjustTrias(itemCount);
    }
    else
    {
        xmlItems.FormatPush("<item n=\"%s\" c=\"%zu\"/>", itemName.GetData(), itemCount);
    }

    return true;
}

bool QuestManager::BuildTriggerList(csString& block,csStringArray& list)
{
    size_t start=0,end;
    csString response;

    while (start < block.Length())
    {
        start = block.Find("P:",start);
        if (start == SIZET_NOT_FOUND)
            return true;
        start += 2;  // skip the actual P:

        // Now find next P:, if any
        end = block.Find("P:",start);
        if (end == SIZET_NOT_FOUND)
            end = block.Length();

        block.SubString(response,start,end-start);
        response.Trim();
        if (response[response.Length()-1] == '.')  // take off trailing .
        {
            response.DeleteAt(response.Length()-1);
        }

        // This isn't truely a "any trigger" but will work
        if (response == "*")
            response = "error";

        list.Push(response);
        
        start = end; // Start at next P: or exit loop
    }
    return true;
}

bool QuestManager::BuildMenu(const csString& block,const csStringArray& list, psQuest *quest, NpcDialogMenu *menu)
{
    size_t start=0, end, counter = 0;
    csString response;

    while (start < block.Length())
    {
        start = block.Find("Menu:",start);
        if (start == SIZET_NOT_FOUND)
            return true;
        start += 5;  // skip the actual Menu:

        // Now find next Menu: tag, if any
        end = block.Find("Menu:",start);
        if (end == SIZET_NOT_FOUND)
            end = block.Length();

        block.SubString(response,start,end-start);
        response.Trim();

		menu->AddTrigger( response, list[ counter++ ], quest );
        
        start = end; // Start at next Menu: or exit loop
    }
    return true;
}



void QuestManager::CutOutParenthesis(csString &response, csString &within,char start_char,char end_char)
{
    // now look for error msg in parenthesis
    size_t start = response.FindLast(start_char);
    if (start != SIZET_NOT_FOUND)
    {
        size_t end = response.FindLast(end_char);
        if (end != SIZET_NOT_FOUND && end > start)
        {
            response.SubString(within,start+1,end-start-1);
            within.Trim();
            response.DeleteAt(start,end-start+1);  // cut out parenthesis.
        }
    }
    else
    {
        within.Clear();
    }
}


bool QuestManager::GetResponseText(csString& block,csString& response,csString& file_path,
                                   csString& him, csString& her, csString& it, csString& them)
{
    size_t start;
    csString pron;

    start = block.FindFirst(':');
    if (start == SIZET_NOT_FOUND)
        return false;

    start++;  // skip colon
    block.SubString(response,start,block.Length()-start);

    CutOutParenthesis(response,file_path,'(',')');
    CutOutParenthesis(response,pron,'{','}');
    him.Clear(); her.Clear(); it.Clear(); them.Clear();
    if (pron.Length())
    {
        csArray<csString> prons = psSplit(pron,',');
        for (size_t i = 0; i < prons.GetSize(); i++)
        {
            csArray<csString> tmp = psSplit(prons[i],':');
            if (tmp.GetSize() == 2)
            {
                if (tmp[0] == "him" || tmp[0] == "he")   him  = tmp[1];
                if (tmp[0] == "her" || tmp[0] == "she")  her  = tmp[1];
                if (tmp[0] == "it")                      it   = tmp[1];
                if (tmp[0] == "them"|| tmp[0] == "they") them = tmp[1];
            }
            else
            {
                Error2("Pronoun(%s) doesn't have the form pron:name",
                      pron.GetDataSafe());
            }
        }
    }

    response.Trim();

    return true;
}

NpcResponse *QuestManager::AddResponse(csString& current_npc,const char *response_text,int& last_response_id, psQuest * quest, 
									   csString& him, csString& her, csString& it, csString& them, csString& file_path)
{
    last_response_id = 0;  // let AddResponse autoset this if set to 0
    Debug2( LOG_QUESTS, 0,"Adding response %s to dictionary...", response_text );
    return dict->AddResponse(response_text,him,her,it,them,current_npc,last_response_id,quest,file_path);
}

bool QuestManager::AddTrigger(csString& current_npc,const char *trigger,int prior_response_id,int trig_response, psQuest * quest, const psString& postfix)
{
    // Now that this npc has a trigger associated, we need to make sure it has a KA for itself.
    // These have been added manually in the past, but we will do it here automatically now.
    gemNPC *npc = dynamic_cast<gemNPC *>(psserver->entitymanager->GetGEM()->FindObject(current_npc));
    if (npc) // specific KA named here
    {
        // First check for no dialog at all on this npc, and create it if needed
        if (!npc->GetNPCDialogPtr())
        {
            npc->SetupDialog(npc->GetPID(), true);  // force init even though potentially no KA's in database
        }
        //npc->GetNPCDialogPtr()->AddKnowledgeArea(current_npc);
    }

    // search for multiple triggers
    csString temp(trigger);
    temp.Downcase();

    csArray<csString> array = psSplit(temp,'.');
    bool result = false;
    for (size_t i=0;i<array.GetSize();i++)
    {
        csString new_trigger(array[i]);
        new_trigger.Trim();
        new_trigger += postfix;

        Debug5( LOG_QUESTS, 0,"Adding trigger '%s' to dictionary for npc '%s', "
                            "with prior response %d and trigger response %d...",
                            new_trigger.GetData(), current_npc.GetData(),
                            prior_response_id, trig_response);
        
        NpcTrigger *npcTrigger = dict->AddTrigger(current_npc,new_trigger,prior_response_id,trig_response);
        
        if(!npcTrigger)
        {
            return false;
        }
        else
        {
            if (quest)
                quest->AddTriggerResponse(npcTrigger, trig_response);
            result = true;
        }
    }

    return result;
}

void QuestManager::HandleQuestInfo(MsgEntry *me,Client *who)
{
    psQuestInfoMessage msg(me);
    QuestAssignment *q = who->GetActor()->GetCharacterData()->IsQuestAssigned(msg.id);

    if (q)
    {
        if (msg.command == psQuestInfoMessage::CMD_DISCARD)
        {
            // Discard quest assignment on request
            who->GetActor()->GetCharacterData()->DiscardQuest(q);
        }
        else
        {
            psQuestInfoMessage response(me->clientnum,psQuestInfoMessage::CMD_INFO,
                q->GetQuest()->GetID(),q->GetQuest()->GetName(),q->GetQuest()->GetTask());
            response.SendMessage();
        }
    }
    else
    {
        if (msg.command == psQuestInfoMessage::CMD_DISCARD)
        {
            Error3("Client %s requested discard of unassigned quest id #%u!",who->GetName(),msg.id);
        }
        else
        {
            Error3("Client %s requested unassigned quest id #%u!",who->GetName(),msg.id);
        }
    }
}

void QuestManager::HandleQuestReward(MsgEntry *me,Client *who)
{
    psQuestRewardMessage msg(me);

    if (msg.msgType==psQuestRewardMessage::selectReward)
    {
        // verify that this item was really offered to the client as a 
        // possible reward
        for (size_t z=0;z<offers.GetSize();z++)
        {
            QuestRewardOffer* offer = offers[z]; 
            if (offer->clientID==me->clientnum)
            {
                for (size_t x=0;x<offer->items.GetSize();x++)
                {
                    uint32 itemID = (uint32)atoi(msg.newValue.GetData());

                    if (offer->items[x]->GetUID()==itemID)
                    {
                        // this item has indeed been offered to the client
                        // so the item can now be given to client (player)
                        GiveRewardToPlayer(who, offer->items[x]);

                        // remove the offer from the list
                        offers.DeleteIndex(z);
                        return;                     
                    }
                }
            }
        }
    }
}



void QuestManager::OfferRewardsToPlayer(Client *who, csArray<psItemStats*> &offer,csTicks &timeDelay)
{
    csString rewardList;

    // create a xml string that will be used to generate the listbox (on
    // the client side) from which a user can select a reward
    rewardList="<rewards>";
    for (size_t x=0;x<offer.GetSize();x++)
    {
        csString image = offer[x]->GetImageName();
        csString name  = offer[x]->GetName();
        csString desc  = offer[x]->GetDescription();
        int      id    = offer[x]->GetUID();

        psString temp;
        csString escpxml_image = EscpXML(image);
        csString escpxml_name = EscpXML(name);
        csString escpxml_desc = EscpXML(desc);
        temp.Format( "<image icon=\"%s\"/><name text=\"%s\"/><id text=\"%d\"/><desc text=\"%s\"/>", 
                     escpxml_image.GetData(), escpxml_name.GetData(), id, escpxml_desc.GetData());

        rewardList += "<L>";
        rewardList += temp;        
        rewardList += "</L>"; 
    }
    rewardList+="</rewards>";

    // CPrintf(CON_DEBUG, "REWARD: %s\n",rewardList.GetData());

    // store the combination of client and reward offers (temporarily) 
    QuestRewardOffer* rewardOffer = new QuestRewardOffer;
    rewardOffer->clientID = who->GetClientNum();
    rewardOffer->items    = offer;
    offers.Push(rewardOffer);
    
    // send a message, containing the rewardlist xml string, to the client,
    psQuestRewardMessage message(who->GetClientNum(), rewardList, psQuestRewardMessage::offerRewards);
    psserver->GetEventManager()->SendMessageDelayed(message.msg,timeDelay);
}


bool QuestManager::GiveRewardToPlayer(Client *who, psItemStats* itemstat)
{
    // check for valid item
    if (itemstat==NULL)
        return false;

    psCharacter* chardata = who->GetActor()->GetCharacterData();
    if (chardata==NULL)
        return false;

    // create the item
    psItem *item = itemstat->InstantiateBasicItem();
    if (item==NULL)
        return false;

    item->SetLoaded();  // Item is fully created

    csString itemName = item->GetName();

    psSystemMessage given(who->GetClientNum(),MSG_INFO,"%s has received a %s!",who->GetName(),itemName.GetData());
    chardata->Inventory().AddOrDrop(item);
    
    // player got his reward
    return true;
}

void QuestManager::Assign(psQuest *quest, Client *who,gemNPC *assigner)
{
    who->GetActor()->GetCharacterData()->AssignQuest(quest, assigner->GetPID());
    psserver->SendSystemOK(who->GetClientNum(),"You got a quest!");
    psserver->SendSystemInfo(who->GetClientNum(),"You now have the %s quest.",quest->GetName() );

    // Post tutorial event
    psGenericEvent evt(who->GetClientNum(), psGenericEvent::QUEST_ASSIGN);
    evt.FireEvent();
}


bool QuestManager::Complete(psQuest *quest, Client *who)
{
    Debug3(LOG_QUESTS, who->GetAccountID().Unbox(), "Completing quest '%s' for character %s.", quest->GetName(), who->GetName());

    bool ret = who->GetActor()->GetCharacterData()->CompleteQuest(quest);

    // if it's a substep don't send additional info
    if (quest->GetParentQuest())
        return true;

    if (ret)
    {
        psserver->SendSystemOK(who->GetClientNum(),"Quest Completed!");
        psserver->SendSystemInfo(who->GetClientNum(),"You have completed the %s quest!", quest->GetName() );
        // TOFIX: we should clean all substeps of this quest from the character_quests db table.

    } 
    else 
    {
        Error3("Cannot complete quest %s for player %s ",quest->GetName(), who->GetName());
    }
    return true;
}

