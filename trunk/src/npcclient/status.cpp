#include <psconfig.h>

#include "globals.h"
#include "status.h"
#include "npcclient.h"

#include "csutil/csstring.h"
#include "csutil/xmltiny.h"
#include "iutil/objreg.h"
#include "iutil/cfgmgr.h"
#include "util/sleep.h"
#include "util/eventmanager.h"
#include "util/psxmlparser.h"


class psNPCStatusRunEvent : public psGameEvent
{
public:
    psNPCStatusRunEvent(csTicks interval);
    void Trigger();    
};

psNPCStatusRunEvent::psNPCStatusRunEvent(csTicks interval)
         : psGameEvent(0, interval, "psNPCStatusRunEvent")
{

}

//-----------------------------------------------------------------------------

void psNPCStatusRunEvent::Trigger ()
{
    struct tm currentTime;
    time_t now;
            
    csString reportString;
    csString timeString;
    
    time( &now );
    currentTime = *gmtime( &now );
    timeString = asctime( &currentTime );

        
    reportString.Format("<npc_report time=\"%s\" now=\"%ld\" number=\"%u\">\n",
        timeString.GetData(), now, NPCStatus::count);
    reportString.Append( "</npc_report>" );
    
    csRef<iFile> logFile = npcclient->GetVFS()->Open( NPCStatus::reportFile, VFS_FILE_WRITE );            
    logFile->Write( reportString, reportString.Length() );                         
    logFile->Flush();

        
    NPCStatus::count++;
    NPCStatus::ScheduleNextRun();
}

csTicks         NPCStatus::reportRate;
csString        NPCStatus::reportFile;
unsigned int    NPCStatus::count;


bool NPCStatus::Initialize (iObjectRegistry* objreg)
{
    csRef<iConfigManager> configmanager =  csQueryRegistry<iConfigManager> (objreg);
    if (!configmanager)
    {
        return false;
    }
    
    bool reportOn = configmanager->GetInt ("PlaneShift.NPCClient.Status.Report", 0) ? true : false;  
    if(!reportOn)
    {
        return true;
    }        
    
    reportRate = configmanager->GetInt ("PlaneShift.NPCClient.Status.Rate", 1000);
    reportFile = configmanager->GetStr ("PlaneShift.NPCClient.Status.LogFile", "/this/npcfile");
       
    ScheduleNextRun();
    return true;
}


void NPCStatus::ScheduleNextRun()
{
    npcclient->GetEventMgr()->Push(new psNPCStatusRunEvent(reportRate));
}
