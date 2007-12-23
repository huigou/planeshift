#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>

#include "globals.h"
#include "util/serverconsole.h"
#include "util/pscssetup.h"
#include "npcclient.h"
//#include "net/netpacket.h"
#include "util/log.h"

// ----------------------------------------------------------------
// this is needed for the engine to construct OS specific stuff 
// such as building WinMain() calling function in windows
CS_IMPLEMENT_APPLICATION

// ----------------------------------------------------------------

// global vars
psNPCClient* npcclient;

// ----------------------------------------------------------------

#ifdef APPNAME
#undef APPNAME
#endif
#define APPNAME "PlaneShift NPC Client"

#include "util/strutil.h"


int main(int argc, char **argv)
{

    psCSSetup* CSSetup = new psCSSetup(argc, argv, "/this/npcclient.cfg",0);
    iObjectRegistry *object_reg = CSSetup->InitCS();

    pslog::Initialize (object_reg);

    // Setup ServerConsole
    ServerConsole::Init(argv[0],"NPC Client");

    // Start the server
    npcclient = new psNPCClient;
    const char *host, *user, *pass;
    int port;
    host = (argc > 2) ? argv[2] : NULL;
    user = (argc > 3) ? argv[3] : NULL;
    pass = (argc > 4) ? argv[4] : NULL;
    port = (argc > 5) ? atoi(argv[5]) : 0;

    if (!npcclient->Initialize(object_reg,host,user,pass,port))
    {
        CPrintf (CON_ERROR, COL_RED "error while initializing NpcClient!\n" COL_NORMAL);
        return 1;
    }

    ServerConsole::SetCommandCatcher(npcclient);

    int retval = npcclient->MainLoop ();
    delete npcclient;

    // Save Configuration
    csRef<iConfigManager> cfgmgr= csQueryRegistry<iConfigManager> (object_reg);
    if (cfgmgr)
        cfgmgr->Save();
    cfgmgr = NULL;

    delete CSSetup;

    csInitializer::DestroyApplication(object_reg);

    return retval;
}

