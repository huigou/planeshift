/*
 * serverconsole.cpp - author: Matze Braun <matze@braunis.de>
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
#include <csutil/csstring.h>
#include <csutil/xmltiny.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/sleep.h"
#include "util/eventmanager.h"
#include "util/psxmlparser.h"

#include "bulkobjects/psguildinfo.h"
//=============================================================================
// Local Includes
//=============================================================================
#include "serverstatus.h"
#include "psserver.h"
#include "playergroup.h"
#include "netmanager.h"
#include "gem.h"
#include "clients.h"
#include "entitymanager.h"
#include "globals.h"
#include "clientstatuslogger.h"


/*****************************************************************
*                psServerStatusRunEvent
******************************************************************/

class psServerStatusRunEvent : public psGameEvent
{
public:
    psServerStatusRunEvent(csTicks interval);
    void Trigger();
    void ReportClient(Client * curr, ClientStatusLogger & clientLogger, csString & reportString);
};

psServerStatusRunEvent::psServerStatusRunEvent(csTicks interval)
         : psGameEvent(0, interval, "psServerStatusRunEvent")
{

}

void psServerStatusRunEvent::Trigger ()
{
    struct tm currentTime;
    time_t now;
            
    csString reportString;
    csString timeString;
    
    csRef<iDocumentSystem> docSystem = csPtr<iDocumentSystem> (new csTinyDocumentSystem ());
    csRef<iDocument> doc = docSystem->CreateDocument();
    csRef<iDocumentNode> rootNode = doc->CreateRoot();

    // create ClientStatusLogger object to log info under node
    ClientStatusLogger clientLogger(rootNode);

    time( &now );
    currentTime = *gmtime( &now );
    timeString = asctime( &currentTime );
    
    ClientConnectionSet * clients = EntityManager::GetSingleton().GetClients();
    reportString.Format("<server_report time=\"%s\" now=\"%ld\" number=\"%u\" client_count=\"%zu\" mob_births=\"%u\" mob_deaths=\"%u\" player_deaths=\"%u\" sold_items=\"%u\" sold_value=\"%u\">\n",
        timeString.GetData(), now, ServerStatus::count, clients->Count(), ServerStatus::mob_birthcount, ServerStatus::mob_deathcount, ServerStatus::player_deathcount, ServerStatus::sold_items, ServerStatus::sold_value );
    ClientIterator i(*clients);
    Client* curr;
    for (curr = i.First(); curr; curr = i.Next())
        ReportClient(curr, clientLogger, reportString);
    reportString.Append( "</server_report>" );
    
    csRef<iFile> logFile = psserver->vfs->Open( ServerStatus::reportFile, VFS_FILE_WRITE );            
    logFile->Write( reportString, reportString.Length() );                         
    logFile->Flush();

    // write XML log to file
    csRef<iFile> logFileTest = psserver->vfs->Open(csString("/this/testlog.xml"), VFS_FILE_WRITE);
    doc->Write(logFileTest);

    ServerStatus::count++;
    ServerStatus::ScheduleNextRun();
}

void psServerStatusRunEvent::ReportClient(Client * curr, ClientStatusLogger & clientLogger, csString & reportString)
{
    psGuildInfo * guild = 0;
    csString guildTitle;
    csString guildName;
    csString format("%s"); // Player name
    csString guildSecret="no";

    if (curr->IsSuperClient() || !curr->GetActor()) 
        return;

    // log this client's info with the clientLogger
    clientLogger.LogClientInfo(curr);

    guild = curr->GetActor()->GetGuild();

    if (guild != NULL)
    {
        if (guild->id)
        {
            psGuildLevel * level = curr->GetActor()->GetGuildLevel();
            if (level)
            {
                format.Append(", %s in %s"); // Guild level title
                guildTitle = level->title;
            }
            else
            {
                format.Append(", %s");
            }
            guildName = guild->name;
        }       
    }
    
    csString player;
    csString escpxml_name = EscpXML(curr->GetName());
    csString escpxml_guildname = EscpXML(guildName);
    csString escpxml_guildtitle = EscpXML(guildTitle);
    
    if ( guild  && guild->IsSecret() == true)
    {       
        escpxml_guildname = EscpXML("");
        escpxml_guildtitle = EscpXML("");
    }                    
    player.Format("<player name=\"%s\" guild=\"%s\" title=\"%s\" security=\"%d\"  />\n", 
                    escpxml_name.GetData(), escpxml_guildname.GetData(), escpxml_guildtitle.GetData(),
                    curr->GetSecurityLevel());
    
    reportString.Append( player ); 
}

/*****************************************************************
*                ServerStatus
******************************************************************/

csTicks      ServerStatus::reportRate;
csString     ServerStatus::reportFile;
unsigned int          ServerStatus::count;
unsigned int          ServerStatus::mob_birthcount;
unsigned int          ServerStatus::mob_deathcount;
unsigned int          ServerStatus::player_deathcount;
unsigned int          ServerStatus::sold_items;
unsigned int          ServerStatus::sold_value;

bool ServerStatus::Initialize (iObjectRegistry* objreg)
{
    csRef<iConfigManager> configmanager =  csQueryRegistry<iConfigManager> (objreg);
    if (!configmanager)
        return false;
    
    bool reportOn = configmanager->GetInt ("PlaneShift.Server.Status.Report", 0) ? true : false;  
    if(!reportOn)
        return true;
    
    reportRate = configmanager->GetInt ("PlaneShift.Server.Status.Rate", 1000);
    reportFile = configmanager->GetStr ("PlaneShift.Server.Status.LogFile", "/this/serverfile");
       
    ScheduleNextRun();
    return true;
}

void ServerStatus::ScheduleNextRun()
{
    psserver->GetEventManager()->Push(new psServerStatusRunEvent(reportRate));
}


