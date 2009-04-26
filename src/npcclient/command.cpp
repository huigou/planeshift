/*
 * command.cpp
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
 * The commands, the user can type in in the console are defined here
 * if you write a new one, don't forget to add it to the list at the end of
 * the file.
 *
 * Author: Matthias Braun <MatzeBraun@gmx.de>
 */

#include <psconfig.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/vfs.h>
#include <iengine/engine.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/command.h"
#include "util/serverconsole.h"
#include "util/log.h"
#include "util/location.h"
#include "util/strutil.h"

#include "engine/psworld.h"

#include "net/connection.h"

#include "npcclient.h"
#include "npc.h"
#include "networkmgr.h"
#include "globals.h"


/* shut down the server and exit program */
int com_quit(char *)
{
    npcclient->Disconnect();

    npcclient->GetEventMgr()->Stop();

    return 0;
}

int com_list(char *arg)
{
    npcclient->ListAllNPCs(arg);
    return 0;
}

int com_entlist(char *arg)
{
    npcclient->ListAllEntities(arg,false);
    return 0;
}

int com_charlist(char *arg)
{
    npcclient->ListAllEntities(arg,true);
    return 0;
}

int com_tribelist(char *arg)
{
    npcclient->ListTribes(arg);
    return 0;
}

int com_waypointlist(char *arg)
{
    npcclient->ListWaypoints(arg);
    return 0;
}

int com_pathlist(char *arg)
{
    npcclient->ListPaths(arg);
    return 0;
}

int com_locationlist(char *arg)
{
    npcclient->ListLocations(arg);
    return 0;
}


int com_locationtest(char *line)
{
    WordArray words(line,false);

    if (words.GetCount() < 5)
    {
        CPrintf(CON_CMDOUTPUT,"Syntax: loctest loc sector x y z\n");
        return 0;
    }

    LocationType * loc = npcclient->FindRegion(words[0]);
    if (!loc)
    {
        CPrintf(CON_CMDOUTPUT,"Location '%s' not found\n",words[0].GetDataSafe());
        return 0;
    }

    iSector * sector = npcclient->GetEngine()->FindSector(words[1]);
    if (!sector)
    {
        CPrintf(CON_CMDOUTPUT,"Sector '%s' not found\n",words[1].GetDataSafe());
        return 0;
    }
    
    float x = atof(words[2]);
    float y = atof(words[3]);
    float z = atof(words[4]);

    csVector3 pos(x,y,z);
    

    if (loc->CheckWithinBounds(npcclient->GetEngine(),pos,sector))
    {
        CPrintf(CON_CMDOUTPUT,"Within loc\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT,"Outside loc\n");
    }

    return 0;
}

/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int com_help (char *arg)
{
    register int i;
    int printed = 0;
    
    CPrintf(CON_CMDOUTPUT, "\n");
    for (i = 0; commands[i].name; i++)
    {
        if (!*arg || (strcmp (arg, commands[i].name) == 0))
        {
            CPrintf (CON_CMDOUTPUT, "%-12s %s.\n", commands[i].name, commands[i].doc);
            printed++;
        }
    }
    
    if (!printed)
    {
    CPrintf (CON_CMDOUTPUT, "No commands match `%s'.  Possibilities are:\n", arg);
    
    for (i = 0; commands[i].name; i++)
    {
        /* Print in six columns. */
        if (printed == 6)
        {
        printed = 0;
        CPrintf (CON_CMDOUTPUT, "\n");
        }
        
        CPrintf (CON_CMDOUTPUT, "%s\t", commands[i].name);
        printed++;
    }
    
    if (printed)
        CPrintf(CON_CMDOUTPUT, "\n");
    }

    CPrintf(CON_CMDOUTPUT, "\n");

    return (0);
}

int com_setmaxout(char* arg)
{
    if (!arg || strlen (arg) == 0)
    {
        CPrintf (CON_CMDOUTPUT, "Use one of the following values to control output on standard output:\n");
        CPrintf (CON_CMDOUTPUT, "  1: only output of server commands\n");
        CPrintf (CON_CMDOUTPUT, "  2: 1 + bug\n");
        CPrintf (CON_CMDOUTPUT, "  3: 2 + errors\n");
        CPrintf (CON_CMDOUTPUT, "  4: 3 + warnings\n");
        CPrintf (CON_CMDOUTPUT, "  5: 4 + notifications\n");
        CPrintf (CON_CMDOUTPUT, "  6: 5 + debug messages\n");
        CPrintf (CON_CMDOUTPUT, "  7: 6 + spam\n");
        CPrintf (CON_CMDOUTPUT, "Current level: %d\n",ConsoleOut::GetMaximumOutputClassStdout());
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf (arg, "%d", &msg);
    if (msg < CON_CMDOUTPUT) msg = CON_CMDOUTPUT;
    if (msg > CON_SPAM) msg = CON_SPAM;
    ConsoleOut::SetMaximumOutputClassStdout ((ConsoleOutMsgClass)msg);
    return 0;
}

int com_setmaxfile(char* arg)
{
    if (!arg || strlen (arg) == 0)
    {
        CPrintf (CON_CMDOUTPUT, "Use one of the following values to control output on output file:\n");
        CPrintf (CON_CMDOUTPUT, "  0: no output at all\n");
        CPrintf (CON_CMDOUTPUT, "  1: only output of server commands\n");
        CPrintf (CON_CMDOUTPUT, "  2: 1 + bug\n");
        CPrintf (CON_CMDOUTPUT, "  3: 2 + errors\n");
        CPrintf (CON_CMDOUTPUT, "  4: 3 + warnings\n");
        CPrintf (CON_CMDOUTPUT, "  5: 4 + notifications\n");
        CPrintf (CON_CMDOUTPUT, "  6: 5 + debug messages\n");
        CPrintf (CON_CMDOUTPUT, "  7: 6 + spam\n");
        CPrintf (CON_CMDOUTPUT, "Current level: %d\n",ConsoleOut::GetMaximumOutputClassFile());
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf (arg, "%d", &msg);
    if (msg < CON_NONE) msg = CON_NONE;
    if (msg > CON_SPAM) msg = CON_SPAM;
    ConsoleOut::SetMaximumOutputClassFile ((ConsoleOutMsgClass)msg);
    return 0;
}

int com_showlogs(char *line)
{
    pslog::DisplayFlags(*line?line:NULL);
    return 0;
}

int com_print(char *line)
{
    if(!npcclient->DumpNPC(line))
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", line);
    }

    return 0;
}

int com_race(char *line)
{
    if(!npcclient->DumpRace(line))
    {
        CPrintf(CON_CMDOUTPUT, "No Race with id '%s' found.\n", line);
    }

    return 0;
}

int com_debugnpc(char*line)
{
    WordArray words(line);

    if (!*line)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <npc_id> [<log_level>]\n");
        return 0;
    }


    unsigned int id = atoi(words[0]);
    NPC* npc = npcclient->FindNPCByPID(id);
    if(!npc)
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", words[0].GetDataSafe());
        return 0;
    }

    if (words.GetCount() >= 2)
    {
        int level = atoi(words[1]);
        npc->SetDebugging(level);
        CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s set to %d.\n", npc->GetName(),level);
    }
    else
    {
        if(npc->SwitchDebugging())
            CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s switched ON.\n", npc->GetName());
        else
            CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s switched OFF.\n", npc->GetName());
    }
    
    return 0;
}

int com_setlog(char *line)
{
    if (!*line)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <log> <true/false> <filter_id>\n");
        CPrintf(CON_CMDOUTPUT, "            or: all <true/false> \n");
        return 0;
    }
    WordArray words(line);
    csString log(words[0]);
    csString flagword(words[1]);
    csString filter(words[2]);
    
    bool flag;
    if (tolower(flagword.GetAt(0)) == 't' || tolower(flagword.GetAt(0)) == 'y' || flagword.GetAt(0) == '1')
    {
        flag=true;
    }
    else
    {
        flag=false;
    }

    uint32 filter_id=0;
    if(filter && !filter.IsEmpty())
    {
        filter_id=atoi(filter.GetDataSafe());
    }

    pslog::SetFlag(log, flag, filter_id);
    
    return 0;
}

int com_filtermsg(char* arg)
{
    
    CPrintf(CON_CMDOUTPUT ,"%s\n",npcclient->GetNetConnection()->LogMessageFilter(arg).GetDataSafe());
    
    return 0;
}

int com_dumpwarpspace(char *)
{
    npcclient->GetWorld()->DumpWarpCache();
    return 0;
}

int com_showtime(char *)
{
    CPrintf(CON_CMDOUTPUT,"Game time is %d:%02d %d-%d-%d\n",
            npcclient->GetGameTODHour(),npcclient->GetGameTODMinute(),
            npcclient->GetGameTODYear(),npcclient->GetGameTODMonth(),npcclient->GetGameTODDay());
    return 0;
}


/* add all new commands here */
const COMMAND commands[] = {
    { "charlist",     false, com_charlist,     "List all known characters"},
    { "debugnpc",     false, com_debugnpc,     "Switches the debug mode on 1 NPC"},
    { "dumpwarpspace",true,  com_dumpwarpspace,"Dump the warp space table"},
    { "entlist",      false, com_entlist,      "List all known entities (entlist [pattern | EID]"},
    { "filtermsg",    true,  com_filtermsg,    "Add or remove messages from the LOG_MESSAGE log"},
    { "help",         false, com_help,         "Show help information" },
    { "loclist",      false, com_locationlist, "List all known locations (loclist [pattern])"},
    { "loctest",      false, com_locationtest, "Test a location (loc sector x y z"},
    { "npclist",      false, com_list,         "List all NPCs (npclist [{pattern | summary}])"},
    { "pathlist",     false, com_pathlist,     "List all known paths (pathlist)"},
    { "print",        false, com_print,        "List all behaviors/hate of 1 NPC"},
    { "quit",         false, com_quit,         "Makes the npc client exit"},
    { "racelist",     false, com_race,         "List all Race Info records"},
    { "setlog",       false, com_setlog,       "Set server log" },
    { "setmaxfile",   false, com_setmaxfile,   "Set maximum message class for output file"},
    { "setmaxout",    false, com_setmaxout,    "Set maximum message class for standard output"},
    { "showlogs",     false, com_showlogs,     "Show server logs" },
    { "showtime",     false, com_showtime,     "Show the current game time"},
    { "tribelist",    false, com_tribelist,    "List all known tribes (tribelist [pattern])"},
    { "waypointlist", false, com_waypointlist, "List all known waypoints (waypointlist [pattern])"},
    { 0, 0, 0, 0 }
};


