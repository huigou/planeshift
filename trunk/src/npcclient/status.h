#ifndef NPC_STATUS_HEADER
#define NCP_STATUS_HEADER

#include <iutil/vfs.h>

#include <csutil/threading/thread.h>
#include <csutil/csstring.h>

struct iObjectRegistry;

/** This class is used to record the status of the npcclient to display it on a website so
    people can see the status of it.
*/
class NPCStatus
{
public:
    /** Reads config files, starts periodical status generator */
    static bool Initialize (iObjectRegistry* objreg);
    
    /** Has the generator run in a while */
    static void ScheduleNextRun();
    
    /// Interval in milliseconds to generate a report file.
    static csTicks reportRate;
    
    /// File that it should log to.
    static csString reportFile;
    
    static unsigned int count;
};

#endif
