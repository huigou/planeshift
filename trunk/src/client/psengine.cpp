/*
 * psengine.cpp
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

//////////////////////////////////////////////////////////////////////////////
// OS Specific Defines
//////////////////////////////////////////////////////////////////////////////
#if defined(CS_COMPILER_GCC) && defined(CS_PLATFORM_WIN32) && defined(LoadImage)
// Somewhere in the mingw includes there is a
// define LoadImage -> LoadImageA and this
// is problematic.
#undef LoadImage
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define USE_WIN32_MINIDUMP
#endif

#ifdef USE_WIN32_MINIDUMP
#include "win32/mdump.h"
// Initialise the crash dumper.
MiniDumper minidumper;
#endif

//////////////////////////////////////////////////////////////////////////////

#define CONFIGFILENAME       "/planeshift/userdata/planeshift.cfg"
#define PSAPP                "planeshift.application.client"


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////
#define PS_QUERY_PLUGIN(myref, intf, str)                    \
myref =  csQueryRegistryOrLoad<intf> (object_reg, str);      \
if (!myref)                                                  \
{                                                            \
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, PSAPP, \
       "No " str " plugin!");                                \
    return false;                                            \
}                                                            \

#define RegisterFactory(factoryclass)   \
    factory = new factoryclass();

//////////////////////////////////////////////////////////////////////////////

// CS files
#include <imap/services.h>
#include <iutil/cfgmgr.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/virtclk.h>
#include <iutil/vfs.h>
#include <iutil/stringarray.h>

//Sound
#include "iclient/isoundmngr.h"

#include <csver.h>
#include <csutil/cmdline.h>
#include <cstool/collider.h>
#include <cstool/initapp.h>
#include <csutil/event.h>

#include "globals.h"
#include "psengine.h"
#include "pscharcontrol.h"
#include "pscamera.h"
#include "psslotmgr.h"
#include "pscelclient.h"
#include "psnetmanager.h"
#include "psclientdr.h"
#include "psclientchar.h"
#include "modehandler.h"
#include "actionhandler.h"
#include "zonehandler.h"
#include "clientvitals.h"
#include "guihandler.h"

#include "pscal3dcallback.h"

#include "sound/pssoundmngr.h"

#include "net/connection.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "effects/pseffectmanager.h"

#include "psoptions.h"

#include "util/localization.h"
#include "util/pscssetup.h"
#include "util/log.h"
#include "util/strutil.h"
#include "engine/psregion.h"
#include "engine/psworld.h"
#include "util/psutil.h"
#include "util/consoleout.h"
#include "entitylabels.h"
#include "chatbubbles.h"
#include "questionclient.h"
#include "iclient/ibgloader.h"
#include "iclient/iscenemanipulate.h"

/////////////////////////////////////////////////////////////////////////////
//  PAWS Includes
/////////////////////////////////////////////////////////////////////////////
#include "paws/pawsprogressbar.h"
#include "paws/pawsmenu.h"

#include "gui/pawsloading.h"
#include "gui/pawsglyphwindow.h"
#include "gui/chatwindow.h"
#include "gui/psmainwidget.h"
#include "gui/pawsconfigwindow.h"
#include "gui/pawsconfigmouse.h"
#include "gui/pawsconfigkeys.h"
#include "gui/pawsconfigcamera.h"
#include "gui/pawsconfigdetails.h"
#include "gui/pawsconfigpvp.h"
#include "gui/pawsconfigchat.h"
#include "gui/pawsconfigchattabs.h"
#include "gui/pawsconfigsound.h"
#include "gui/pawsconfigentitylabels.h"
#include "gui/pawsconfigentityinter.h"
#include "gui/inventorywindow.h"
#include "gui/pawsitemdescriptionwindow.h"
#include "gui/pawscontainerdescwindow.h"
#include "gui/pawsinteractwindow.h"
#include "gui/pawsinfowindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/pawsglyphwindow.h"
#include "gui/pawsgroupwindow.h"
#include "gui/pawsexchangewindow.h"
#include "gui/pawsmerchantwindow.h"
#include "gui/pawspetitionwindow.h"
#include "gui/pawspetitiongmwindow.h"
#include "gui/pawsspellbookwindow.h"
#include "gui/pawssplashwindow.h"
#include "gui/shortcutwindow.h"
#include "gui/pawsloginwindow.h"
#include "gui/pawscharpick.h"
#include "gui/pawsloading.h"
#include "gui/pawsguildwindow.h"
#include "gui/pawslootwindow.h"
#include "gui/pawspetstatwindow.h"
#include "gui/pawsskillwindow.h"
#include "gui/pawsquestwindow.h"
#include "gui/pawsspellcancelwindow.h"
#include "gui/pawscharcreatemain.h"
#include "gui/pawscharbirth.h"
#include "gui/pawscharparents.h"
#include "gui/pawschild.h"
#include "gui/pawslife.h"
#include "gui/pawspath.h"
#include "gui/pawssummary.h"
#include "gui/pawsgmgui.h"
#include "gui/pawsmoney.h"
#include "gui/pawshelp.h"
#include "gui/pawsbuddy.h"
#include "gui/pawsignore.h"
#include "gui/pawsslot.h"
#include "gui/pawsactionlocationwindow.h"
#include "gui/pawsdetailwindow.h"
#include "gui/pawschardescription.h"
#include "gui/pawsquestrewardwindow.h"
#include "gui/pawscreditswindow.h"
#include "gui/pawsinventorydollview.h"
#include "gui/pawsquitinfobox.h"
#include "gui/pawsgmspawn.h"
#include "gui/pawsbookreadingwindow.h"
#include "gui/pawswritingwindow.h"
#include "gui/pawsactivemagicwindow.h"
#include "gui/pawstutorialwindow.h"
#include "gui/pawssmallinventory.h"
#include "gui/pawsconfigchatfilter.h"
#include "gui/pawsgmaction.h"
#include "gui/pawscraft.h"
#include "gui/pawsilluminationwindow.h"
#include "gui/pawsgameboard.h"
#include "gui/pawsbankwindow.h"
#include "gui/pawsconfigchatbubbles.h"
#include "gui/pawsconfigshadows.h"
#include "gui/pawsnpcdialog.h"


// ----------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

// ----------------------------------------------------------------------------

psEngine::psEngine (iObjectRegistry *objectreg, psCSSetup *CSSetup)
{
    object_reg = objectreg;
    CS_Setup = CSSetup;
    // No no, no map loaded
    loadedMap = false;
    gameLoaded = false;
    loadstate = LS_NONE;
    loadError = false;

    charmanager = NULL;
    guiHandler = NULL;
    charController = NULL;
    mouseBinds = NULL;
    camera = NULL;
    slotManager = NULL;
    questionclient = NULL;
    paws = NULL;
    mainWidget = NULL;
    inventoryCache = NULL;
    loader = NULL;

    loadtimeout = 10;  // Default load timeout

    drawScreen = true;

    cal3DCallbackLoader = csPtr<psCal3DCallbackLoader> (new psCal3DCallbackLoader(objectreg));

    BrightnessCorrection = 0.0f;

    KFactor = 0.0f;
    targetPetitioner = "None";
    confirmation = 1;  // Def

    loggedIn = false;
    numOfChars = 0;
    elapsed = 0;
    frameLimit = 0;

    muteSoundsOnFocusLoss = false;


    xmlparser =  csQueryRegistry<iDocumentSystem> (object_reg);
    stringset = csQueryRegistryTagInterface<iStringSet> (object_reg, "crystalspace.shared.stringset");
    nameRegistry = csEventNameRegistry::GetRegistry (object_reg);

    chatBubbles = 0;
    options = 0;
    gfxFeatures = 0;
}

// ----------------------------------------------------------------------------

psEngine::~psEngine ()
{
    printf("psEngine destroyed.\n");
}


void psEngine::Cleanup()
{
    if (loadstate==LS_DONE)
        QuitClient();

    delete charmanager;
    delete charController;
    delete camera;
    delete chatBubbles;

    if (queue)
    {
        if(eventHandlerLogic)
            queue->RemoveListener (eventHandlerLogic);
        if(eventHandler2D)
            queue->RemoveListener (eventHandler2D);
        if(eventHandler3D)
            queue->RemoveListener (eventHandler3D);
        if(eventHandlerFrame)
            queue->RemoveListener (eventHandlerFrame);
    }

    delete paws; // Include delete of mainWidget

    delete questionclient;
    delete slotManager;
    delete mouseBinds;
    delete guiHandler;
    delete inventoryCache;

    // celclient needs to be destroyed before the effectmanager.
    celclient.Invalidate();
    modehandler.Invalidate();

    // Effect manager needs to be destroyed before the soundmanager.
    effectManager.Invalidate();

    object_reg->Unregister ((iSoundManager*)soundmanager, "iSoundManager");

    delete options;
}

// ----------------------------------------------------------------------------

/**
 *
 * psEngine::Initialize
 * This function initializes all objects necessary for the game. It also creates a
 * suitable environment for the engine.
 *
 **/
bool psEngine::Initialize (int level)
{
    // load basic stuff that is enough to display the splash screen
    if (level == 0)
    {
        // print out some version information
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP, APPNAME);
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
            "This game uses Crystal Space Engine created by Jorrit and others");
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP, CS_VERSION);

        // Query for plugins
        PS_QUERY_PLUGIN (queue,   iEventQueue,    "iEventQueue");
        PS_QUERY_PLUGIN (vfs,     iVFS,           "iVFS");
        PS_QUERY_PLUGIN (engine,  iEngine,        "iEngine");
        PS_QUERY_PLUGIN (cfgmgr,  iConfigManager, "iConfigManager");
        PS_QUERY_PLUGIN (g3d,     iGraphics3D,    "iGraphics3D");
        PS_QUERY_PLUGIN (vc,      iVirtualClock,  "iVirtualClock");
        PS_QUERY_PLUGIN (cmdline, iCommandLineParser, "iCommandLineParser");

        g2d = g3d->GetDriver2D();

        g2d->AllowResize(false);

        // Check for configuration values for crash dump action and mode
        #ifdef USE_WIN32_MINIDUMP
            PS_CRASHACTION_TYPE crashaction=PSCrashActionPrompt;
            csString CrashActionStr(cfgmgr->GetStr("PlaneShift.Crash.Action","prompt"));
            if (CrashActionStr.CompareNoCase("off"))
                crashaction=PSCrashActionOff;
            if (CrashActionStr.CompareNoCase("prompt"))
                crashaction=PSCrashActionPrompt;
            if (CrashActionStr.CompareNoCase("always"))
                crashaction=PSCrashActionAlways;

            minidumper.SetCrashAction(crashaction);

            PS_MINIDUMP_TYPE dumptype=PSMiniDumpNormal;
            csString DumpTypeStr(cfgmgr->GetStr("PlaneShift.Crash.DumpType","normal"));
            if (DumpTypeStr.CompareNoCase("normal")) // The normal stack and back information with no data
                dumptype=PSMiniDumpNormal;
            if (DumpTypeStr.CompareNoCase("detailed")) // This includes data segments associated with modules - global variables and static members
                dumptype=PSMiniDumpWithDataSegs;
            if (DumpTypeStr.CompareNoCase("full")) // This is the entire process memory, this dump will be huge, but will include heap data as well
                dumptype=PSMiniDumpWithFullMemory;
            if (DumpTypeStr.CompareNoCase("NThandles")) // NT/2k/xp only, include system handle information in addition to normal
                dumptype=PSMiniDumpWithHandleData;
            if (DumpTypeStr.CompareNoCase("filter")) // This can trim down the dump even more than normal
                dumptype=PSMiniDumpFilterMemory;
            if (DumpTypeStr.CompareNoCase("scan")) // This may be able to help make stack corruption crashes somewhat readable
                dumptype=PSMiniDumpScanMemory;

            minidumper.SetDumpType(dumptype);

            // Report the current type
            csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP, minidumper.GetDumpTypeString() );
        #endif // #ifdef USE_WIN32_MINIDUMP


        // Initialize and tweak the Texture Manager
        txtmgr = g3d->GetTextureManager ();
        if (!txtmgr)
        {
            return false;
        }

        // Check the level of shader use.
        csString shader("Highest");
        if(shader.CompareNoCase(GetConfig()->GetStr("PlaneShift.Graphics.Shaders")))
        {
            gfxFeatures |= psRegion::useHighestShaders;
        }
        shader = "High";
        if(shader.CompareNoCase(GetConfig()->GetStr("PlaneShift.Graphics.Shaders")))
        {
            gfxFeatures |= psRegion::useHighShaders;
        }
        shader = "Medium";
        if(shader.CompareNoCase(GetConfig()->GetStr("PlaneShift.Graphics.Shaders")))
        {
            gfxFeatures |= psRegion::useMediumShaders;
        }
        shader = "Low";
        if(shader.CompareNoCase(GetConfig()->GetStr("PlaneShift.Graphics.Shaders")))
        {
            gfxFeatures |= psRegion::useLowShaders;
        }
        shader = "Lowest";
        if(shader.CompareNoCase(GetConfig()->GetStr("PlaneShift.Graphics.Shaders")))
        {
            gfxFeatures |= psRegion::useLowestShaders;
        }

        // Check if we're using real time shadows.
        if(GetConfig()->GetBool("PlaneShift.Graphics.Shadows"))
        {
            gfxFeatures |= psRegion::useShadows;
        }

        // Check if we're using meshgen.
        if(GetConfig()->GetBool("PlaneShift.Graphics.EnableGrass", true))
        {
            gfxFeatures |= psRegion::useMeshGen;
        }

        //Check if sound is on or off in psclient.cfg
        csString soundPlugin;

        soundOn = GetConfig()->KeyExists("System.PlugIns.iSndSysRenderer") &&
            strcmp(GetConfig()->GetStr("System.PlugIns.iSndSysRenderer"), "crystalspace.sndsys.renderer.null");

        if (soundOn)
        {
            psSoundManager* pssound = new psSoundManager(0);

            pssound->Initialize(object_reg);
            soundmanager.AttachNew(pssound);
            object_reg->Register ((iSoundManager*) soundmanager, "iSoundManager");

            if (!soundmanager->Setup())
            {
                csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
                    "Warning: Cannot initialize SoundManager");
                soundOn = false;
            }
        }

        LoadLogSettings();

        // Mount the selected gui first to allow overwriting of certain elements
        csString skinPath;
        skinPath += cfgmgr->GetStr("PlaneShift.GUI.Skin.Dir", "/planeshift/art/skins/");
        skinPath += cfgmgr->GetStr("PlaneShift.GUI.Skin.Selected", "default.zip");

        // This could be a file or a dir
        csString slash(CS_PATH_SEPARATOR);
        if(vfs->Exists(skinPath + slash))
        {
            skinPath += slash;
        }
        else if(vfs->Exists(skinPath + ".zip"))
        {
            skinPath += ".zip";
        }


        // Create the PAWS window manager
        csString skinPathBase = cfgmgr->GetStr("PlaneShift.GUI.Skin.Base","/planeshift/art/skins/base/client_base.zip");
        paws = new PawsManager( object_reg, skinPath, skinPathBase, "/planeshift/userdata/planeshift.cfg", gfxFeatures );

        options = new psOptions("/planeshift/userdata/options.cfg", vfs);

        // Default to maximum 71 fps
        // Actual fps get be up to 10 fps less so set a reasonably high limit
        maxFPS = cfgmgr->GetInt("Video.FrameLimit", 71);
        frameLimit = 1000 / maxFPS;

        paws->SetSoundStatus(soundOn);
        mainWidget = new psMainWidget();
        paws->SetMainWidget( mainWidget );

        paws->GetMouse()->Hide(true);

        DeclareExtraFactories();

        // Register default PAWS sounds
        if (soundmanager.IsValid() && soundOn)
        {
            paws->RegisterSound("sound.standardButtonClick",soundmanager->GetSoundResource("sound.standardButtonClick"));
            // Standard GUI stuff
            paws->RegisterSound("gui.toolbar",      soundmanager->GetSoundResource("gui.toolbar"));
            paws->RegisterSound("gui.cancel",       soundmanager->GetSoundResource("gui.cancel"));
            paws->RegisterSound("gui.ok",           soundmanager->GetSoundResource("gui.ok"));
            paws->RegisterSound("gui.scrolldown",   soundmanager->GetSoundResource("gui.scrolldown"));
            paws->RegisterSound("gui.scrollup",     soundmanager->GetSoundResource("gui.scrollup"));
            paws->RegisterSound("gui.shortcut",     soundmanager->GetSoundResource("gui.shortcut"));
            paws->RegisterSound("gui.quit",         soundmanager->GetSoundResource("gui.quit"));
            // Load sound settings
             if(!LoadSoundSettings(false))
             {
                return false;
             }
        }


        if ( ! paws->LoadWidget("splash.xml") )
            return false;
        if ( ! paws->LoadWidget("ok.xml") )
            return false;
        if ( ! paws->LoadWidget("quitinfo.xml") )
            return false;

        LoadPawsWidget("Yes / No dialog","yesno.xml" );

        //Load confirmation information for duels
        if (!LoadDuelConfirm())
            return false;

        // Register our event handlers.
        event_frame = csevFrame (object_reg);
        event_canvashidden = csevCanvasHidden(object_reg,g2d);
        event_canvasexposed = csevCanvasExposed(object_reg,g2d);
        event_focusgained = csevFocusGained(object_reg);
        event_focuslost = csevFocusLost(object_reg);
        event_mouse = csevMouseEvent (object_reg);
        event_keyboard = csevKeyboardEvent (object_reg);
        event_quit = csevQuit(object_reg);

        eventHandlerLogic = csPtr<LogicEventHandler> (new LogicEventHandler (this));
        eventHandler2D = csPtr<EventHandler2D> (new EventHandler2D (this));
        eventHandler3D = csPtr<EventHandler3D> (new EventHandler3D (this));
        eventHandlerFrame = csPtr<FrameEventHandler> (new FrameEventHandler (this));

        csEventID esublog[] = {
              event_frame,
              event_canvashidden,
              event_canvasexposed,
              event_focusgained,
              event_focuslost,
              event_mouse,
              event_keyboard,
              event_quit,
              CS_EVENTLIST_END
        };

        queue->RegisterListener(eventHandlerLogic, esublog);
        queue->RegisterListener(eventHandler2D, event_frame);
        queue->RegisterListener(eventHandler3D, event_frame);
        queue->RegisterListener(eventHandlerFrame, event_frame);

        // Inform debug that everything initialized succesfully
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
            "psEngine initialized.");


        return true;
    }
    else if (level==1)
    {
        threadedWorldLoading = psengine->GetConfig()->GetBool("PlaneShift.Loading.ThreadedWorldLoad");
        loader = csQueryRegistry<iBgLoader>(object_reg);
        scenemanipulator = scfQueryInterface<iSceneManipulate>(loader);
        csRef<iThreadManager> tm = csQueryRegistry<iThreadManager>(object_reg);
        loader->Setup(gfxFeatures, 200);

        // Fill the loader cache.
        csRef<iStringArray> dirs = vfs->FindFiles("/planeshift/models/");
        for(size_t i=0; i<dirs->GetSize(); ++i)
        {
            csRef<iStringArray> files = vfs->FindFiles(dirs->Get(i));
            for(size_t j=0; j<files->GetSize(); ++j)
            {
                csString file = files->Get(j);
                if(file.Find(".cal3d", file.Length()-7) != size_t(-1))
                {
                    if(tm->GetThreadCount() == 1)
                    {
                        loader->PrecacheDataWait(files->Get(j), false);
                    }
                    else
                    {
                        modelPrecaches.Push(loader->PrecacheData(files->Get(j), false));
                    }
                }
            }
        }

        dirs = vfs->FindFiles("/planeshift/");
        for(size_t i=0; i<dirs->GetSize(); ++i)
        {
            csRef<iStringArray> files = vfs->FindFiles(dirs->Get(i));
            for(size_t j=0; j<files->GetSize(); ++j)
            {
                csString file = files->Get(j);
                if(file.Find(".meshfact", file.Length()-10) != size_t(-1))
                {
                    if(tm->GetThreadCount() == 1)
                    {
                        loader->PrecacheDataWait(files->Get(j), false);
                    }
                    else
                    {
                        modelPrecaches.Push(loader->PrecacheData(files->Get(j), false));
                    }
                }
            }
        }

        csRef<iStringArray> maps = vfs->FindFiles("/planeshift/world/");

        // Do the _common maps first.
        for(size_t i=0; i<maps->GetSize(); ++i)
        {
            csRef<iDataBuffer> tmp = vfs->GetRealPath(maps->Get(i));
            if(csString(tmp->GetData()).Find("common") != (size_t)-1)
            {
                csString vpath(maps->Get(i));
                vpath.Append("world");
                if(tm->GetThreadCount() == 1)
                {
                    loader->PrecacheDataWait(vpath.GetData(), false);
                }
                else
                {
                    mapPrecaches.Push(loader->PrecacheData(vpath.GetData(), false));
                }
            }
        }
        tm->Wait(mapPrecaches);

        // Now everything else.
        for(size_t i=0; i<maps->GetSize(); ++i)
        {
            csRef<iDataBuffer> tmp = vfs->GetRealPath(maps->Get(i));
            if(csString(tmp->GetData()).Find("common") == (size_t)-1)
            {
                csString vpath(maps->Get(i));
                vpath.Append("world");
                mapPrecaches.Push(loader->PrecacheData(vpath.GetData(), false));
            }
        }

        // Initialize Networking
        if (!netmanager)
        {
            netmanager = csPtr<psNetManager> (new psNetManager);

            if (!netmanager->Initialize(object_reg))
            {
                lasterror = "Couldn't init Network Manager.";
                return false;
            }
            psMessageCracker::msghandler = netmanager->GetMsgHandler();
        }

        inventoryCache = new psInventoryCache();
        guiHandler = new GUIHandler();
        celclient = csPtr<psCelClient> (new psCelClient());
        slotManager = new psSlotManager();
        modehandler = csPtr<ModeHandler> (new ModeHandler (soundmanager, celclient,netmanager->GetMsgHandler(),object_reg));
        actionhandler = csPtr<ActionHandler> ( new ActionHandler ( netmanager->GetMsgHandler(), object_reg ) );
        zonehandler = csPtr<ZoneHandler> (new ZoneHandler(netmanager->GetMsgHandler(),object_reg,celclient));
        questionclient = new psQuestionClient(GetMsgHandler(), object_reg);

        if (cmdline)
        {
            celclient->IgnoreOthers(cmdline->GetBoolOption("ignore_others"));
        }

        zonehandler->SetLoadAllMaps(GetConfig()->GetBool("PlaneShift.Client.Loading.AllMaps",false));
        zonehandler->SetKeepMapsLoaded(GetConfig()->GetBool("PlaneShift.Client.Loading.KeepMaps",false));

        if (!celclient->Initialize(object_reg, GetMsgHandler(), zonehandler))
        {
            lasterror = "Couldn't init Cel Manager.";
            Error2("FATAL ERROR: %s",lasterror.GetData());
            return false;
        }


        if(!modehandler->Initialize())
        {
            lasterror = "ModeHandler failed init.";
            Error2("FATAL ERROR: %s",lasterror.GetData());
            return false;
        }

        // Init the main widget
        mainWidget->SetupMain();

        if (!camera)
        {
            camera = new psCamera();
        }

        if (!charmanager)
        {
            charmanager = new psClientCharManager(object_reg);

            if (!charmanager->Initialize( GetMsgHandler(), celclient))
            {
                lasterror = "Couldn't init Character Manager.";
                return false;
            }
        }


        // This widget requires NetManager to exist so must be in this stage
        if ( ! paws->LoadWidget("charpick.xml") )
            return false;

        // Load effects now, before we have actors
        if (!effectManager)
        {
            effectManager.AttachNew(new psEffectManager(object_reg));
        }

        if (!effectManager->LoadFromDirectory("/this/data/effects", true, camera->GetView()))
        {
            FatalError("Failed to load effects!");
            return 1;
        }

        tm->Wait(modelPrecaches);
    }

    return true;
}

void psEngine::LoadLogSettings()
{
    int count=0;
    for (int i=0; i< MAX_FLAGS; i++)
    {
        if (pslog::GetName(i))
        {
            pslog::SetFlag(pslog::GetName(i),cfgmgr->GetBool(pslog::GetSettingName(i)), 0);
            if ((cfgmgr->GetBool(pslog::GetSettingName(i))))
            {
                count++;
            }

        }
    }
    if (count==0)
    {
        CPrintf(CON_CMDOUTPUT,"All LOGS are off.\n");
    }

}

void psEngine::DeclareExtraFactories()
{
    pawsWidgetFactory* factory;

    RegisterFactory (pawsInventoryDollViewFactory);
    RegisterFactory (pawsGlyphSlotFactory);
    RegisterFactory (pawsInfoWindowFactory);
    RegisterFactory (pawsSplashWindowFactory);
    RegisterFactory (pawsLoadWindowFactory);
    RegisterFactory (pawsChatWindowFactory);
    RegisterFactory (pawsInventoryWindowFactory);
    RegisterFactory (pawsItemDescriptionWindowFactory);
    RegisterFactory (pawsContainerDescWindowFactory);
    RegisterFactory (pawsInteractWindowFactory);
    RegisterFactory (pawsControlWindowFactory);
    RegisterFactory (pawsGroupWindowFactory);
    RegisterFactory (pawsExchangeWindowFactory);
    RegisterFactory (pawsSpellBookWindowFactory);
    RegisterFactory (pawsGlyphWindowFactory);
    RegisterFactory (pawsMerchantWindowFactory);
    RegisterFactory (pawsConfigWindowFactory);
    RegisterFactory (pawsConfigKeysFactory);
    RegisterFactory (pawsConfigPvPFactory);
    RegisterFactory (pawsFingeringWindowFactory);
    RegisterFactory (pawsConfigDetailsFactory);
    RegisterFactory (pawsConfigMouseFactory);
    RegisterFactory (pawsConfigCameraFactory);
    RegisterFactory (pawsConfigChatFactory);
    RegisterFactory (pawsConfigSoundFactory);
    RegisterFactory (pawsConfigEntityLabelsFactory);
    RegisterFactory (pawsConfigEntityInteractionFactory);
    RegisterFactory (pawsPetitionWindowFactory);
    RegisterFactory (pawsPetitionGMWindowFactory);
    RegisterFactory (pawsShortcutWindowFactory);
    RegisterFactory (pawsLoginWindowFactory);
    RegisterFactory (pawsCharacterPickerWindowFactory);
    RegisterFactory (pawsGuildWindowFactory);
    RegisterFactory (pawsLootWindowFactory);
    RegisterFactory (pawsCreationMainFactory);
    RegisterFactory (pawsCharBirthFactory);
    RegisterFactory (pawsCharParentsFactory);
    RegisterFactory (pawsChildhoodWindowFactory);
    RegisterFactory (pawsLifeEventWindowFactory);
    RegisterFactory (pawsPathWindowFactory);
    RegisterFactory (pawsSummaryWindowFactory);
    RegisterFactory (pawsSkillWindowFactory);
    RegisterFactory (pawsQuestListWindowFactory);
    RegisterFactory (pawsSpellCancelWindowFactory);
    RegisterFactory (pawsGmGUIWindowFactory);
    RegisterFactory (pawsMoneyFactory);
    RegisterFactory (pawsHelpFactory);
    RegisterFactory (pawsBuddyWindowFactory);
    RegisterFactory (pawsIgnoreWindowFactory);
    RegisterFactory (pawsSlotFactory);
    RegisterFactory (pawsActionLocationWindowFactory);
    RegisterFactory (pawsDetailWindowFactory);
    RegisterFactory (pawsCharDescriptionFactory);
    RegisterFactory (pawsQuestRewardWindowFactory);
    RegisterFactory (pawsCreditsWindowFactory);
    RegisterFactory (pawsQuitInfoBoxFactory);
    RegisterFactory (pawsGMSpawnWindowFactory);
    RegisterFactory (pawsSkillIndicatorFactory);
    RegisterFactory (pawsBookReadingWindowFactory);
    RegisterFactory (pawsWritingWindowFactory);
    RegisterFactory (pawsActiveMagicWindowFactory);
    RegisterFactory (pawsSmallInventoryWindowFactory);
    RegisterFactory (pawsConfigChatFilterFactory);
    RegisterFactory (pawsConfigChatTabsFactory);
    RegisterFactory (pawsGMActionWindowFactory);
    RegisterFactory (pawsCraftWindowFactory);
    RegisterFactory (pawsPetStatWindowFactory);
    RegisterFactory (pawsTutorialWindowFactory);
    RegisterFactory (pawsTutorialNotifyWindowFactory);
    RegisterFactory (pawsSketchWindowFactory);
    RegisterFactory (pawsGameBoardFactory);
    RegisterFactory (pawsGameTileFactory);
    RegisterFactory (pawsBankWindowFactory);
    RegisterFactory (pawsConfigChatBubblesFactory);
    RegisterFactory (pawsConfigShadowsFactory);
    RegisterFactory (pawsNpcDialogWindowFactory);
}

//-----------------------------------------------------------------------------

/**
 * These functions receive and process all events that occur when the game runs.
 * An event is classified by CS as anything that occurs, whether be it mouse move,
 * mouse click, messages, etc.
 */

bool psEngine::ProcessLogic(iEvent& ev)
{
    lastEvent = &ev;

    if(ev.Name == event_frame)
    {
        if (gameLoaded)
        {
            modehandler->PreProcess();
        }

        // If any objects or actors are enqueued to be created, create the next one this frame.
        if (celclient)
        {
            celclient->CheckEntityQueues();
        }

        UpdatePerFrame();
    }
    if (ev.Name == event_canvashidden)
    {
        drawScreen = false;
        if(soundOn)
        {
            MuteAllSounds();
        }
    }
    else if (ev.Name == event_canvasexposed)
    {
        drawScreen = true;
        if(soundOn)
        {
            UnmuteAllSounds();
        }
    }
    else if (ev.Name == event_focusgained)
    {
        if(GetMuteSoundsOnFocusLoss() && soundOn)
        {
            UnmuteAllSounds();
        }
    }
    else if (ev.Name == event_focuslost)
    {
        if(GetMuteSoundsOnFocusLoss() && soundOn)
        {
            MuteAllSounds();
        }
    }
    else if (ev.Name == event_quit)
    {
        // Disconnect and quit, if this event wasn't made here
        QuitClient();
    }
    else
    {
        bool handled = false;
        if(paws->HandleEvent(ev))
        {
          handled = true;
        }

        if(slotManager && !handled && slotManager->HandleEvent(ev))
        {
          handled = true;
        }

        if(charController)
        {
            if(!handled && charController->HandleEvent(ev))
            {
              handled = true;
            }

            if(handled && paws->GetFocusOverridesControls())
            {
              charController->GetMovementManager()->StopControlledMovement();
            }
        }

        return handled;
    }

    return false;
}

static bool drawFrame;
bool psEngine::Process3D(iEvent& ev)
{
    lastEvent = &ev;

    for(size_t i=0; i<delayedLoaders.GetSize(); ++i)
    {
        csWeakRef<DelayedLoader> dl = delayedLoaders.Get(i);
        if(dl.IsValid())
        {
            if(dl->CheckLoadStatus())
                break;
        }
        else
        {
            delayedLoaders.DeleteIndexFast(0);
        }
    }

    // Loading the game
    if (loadstate != LS_DONE)
    {
        LoadGame();
    }
    // Update the sound system
    else if (GetSoundStatus())
    {
        soundmanager->Update( camera->GetView() );
        soundmanager->Update( celclient->GetMainPlayer()->Pos() );
    }

    if (celclient)
        celclient->Update(HasLoadedMap());

    if (effectManager)
        effectManager->Update();

    if (drawScreen)
    {
        // FPS limits
        drawFrame = FrameLimit();

        if(drawFrame)
        {
            if(camera)
            {
              g3d->BeginDraw(CSDRAW_3DGRAPHICS);
              camera->Draw();
              return true;
            }
        }
    }
    else
    {
        // Sleep for a bit but don't draw anything if minimized
        FrameLimit();
        return true;
    }

    return false;
}

bool psEngine::Process2D(iEvent& ev)
{
    lastEvent = &ev;

    if (drawScreen && drawFrame)
    {
        g3d->BeginDraw(CSDRAW_2DGRAPHICS);
        if (effectManager)
            effectManager->Render2D(g3d, g2d);
        paws->Draw();
    }

    return true;
}

bool psEngine::ProcessFrame(iEvent& ev)
{
    lastEvent = &ev;

    if(drawScreen && drawFrame)
        FinishFrame();

    // We need to call this after drawing was finished so
    // LoadingScreen had a chance to be displayed when loading
    // maps in-game
    if (zonehandler)
        zonehandler->OnDrawingFinished();

    return true;
}

// ----------------------------------------------------------------------------

const csHandlerID * psEngine::LogicEventHandler::GenericPrec(csRef<iEventHandlerRegistry> &handler_reg,
                                                        csRef<iEventNameRegistry> &name_reg,
                                                        csEventID e) const
{
    if (e != csevFrame(name_reg))
        return 0;
    static csHandlerID precConstraint[2] = {
        handler_reg->GetGenericID("planeshift.clientmsghandler"),
        CS_HANDLERLIST_END
    };
    return precConstraint;
}

const csHandlerID * psEngine::LogicEventHandler::GenericSucc(csRef<iEventHandlerRegistry> &handler_reg,
                                                             csRef<iEventNameRegistry> &name_reg,
                                                             csEventID e) const
{
    if (e != csevFrame(name_reg))
        return 0;
    static csHandlerID succConstraint[6] = {
        FrameSignpost_Logic3D::StaticID(handler_reg),
        FrameSignpost_3D2D::StaticID(handler_reg),
        FrameSignpost_2DConsole::StaticID(handler_reg),
        FrameSignpost_ConsoleDebug::StaticID(handler_reg),
        FrameSignpost_DebugFrame::StaticID(handler_reg),
        CS_HANDLERLIST_END
    };
    return succConstraint;
}

// ----------------------------------------------------------------------------

inline bool psEngine::FrameLimit()
{
    csTicks sleeptime;
    csTicks elapsedTime;

    // Find the time taken since we left this function
    elapsedTime = csGetTicks() - elapsed;

    static pawsWidget* loading = NULL;
    if(!loading)
        loading = paws->FindWidget("LoadWindow", false);

    // Loading competes with drawing in THIS thread so we can't sleep
    if(loading && loading->IsVisible())
    {
        sleeptime = 1000;

        // Here we sacrifice drawing time for loading time
        if(elapsedTime < sleeptime)
            return false;
        else
        {
            elapsedTime = csGetTicks();
            return true;
        }

    }

    // Define sleeptimes
    if(!camera)
    {
        // Get the window
        static pawsWidget* credits = NULL;
        if(!credits)
            credits = paws->FindWidget("CreditsWindow", false);

        if(credits && credits->IsVisible())
            sleeptime = 10;
        else
            sleeptime = 30;
    }
    else
        sleeptime = frameLimit;


    // Here we sacrifice drawing AND loading time
    if(elapsedTime < sleeptime)
        csSleep(sleeptime - elapsedTime);

    elapsed = csGetTicks();

    return true;
}

void psEngine::MuteAllSounds(void)
{
    GetSoundManager()->ToggleAmbient(false);
    GetSoundManager()->ToggleMusic(false);
    GetSoundManager()->ToggleActions(false);
    GetSoundManager()->ToggleGUI(false);
    paws->ToggleSounds(false);
}

void psEngine::UnmuteAllSounds(void)
{

    LoadSoundSettings(false);

    //If we are in the login/loading/credits/etc. screen we have to force the music to start again.
    if(loadstate != LS_DONE)
    {
        const char* bgMusic = NULL;
        bgMusic = GetSoundManager()->GetSongName();
        if (bgMusic == NULL)
            return;

        GetSoundManager()->OverrideBGSong(bgMusic);
    }
}

// ----------------------------------------------------------------------------

const char* psEngine::FindCommonString(unsigned int cstr_id)
{
   if (!celclient)
        return "";

   psClientDR * clientDR = celclient->GetClientDR();
   if (!clientDR)
        return "";

   return clientDR->GetMsgStrings()->Request(cstr_id);
}

csStringID psEngine::FindCommonStringId(const char *str)
{
    if (!celclient)
        return csInvalidStringID;

    psClientDR * clientDR = celclient->GetClientDR();
    if (!clientDR)
        return csInvalidStringID;

    return clientDR->GetMsgStrings()->Request(str);
}

// ----------------------------------------------------------------------------

inline void psEngine::UpdatePerFrame()
{
    if (!celclient)
        return;

    // Must be in PostProcess for accurate DR updates
    if (celclient->GetClientDR())
        celclient->GetClientDR()->CheckDeadReckoningUpdate();

    if (celclient->GetMainPlayer())
    {
        celclient->GetClientDR()->CheckSectorCrossing(celclient->GetMainPlayer());
        celclient->PruneEntities();    // Prune CD-intensive entities by disabling CD

        // Update Stats for Player
        celclient->GetMainPlayer()->GetVitalMgr()->Predict( csGetTicks(),"Self" );

        // Update stats for Target if Target is there and has stats
        GEMClientObject * target = psengine->GetCharManager()->GetTarget();
        if (target && target->GetType() != -2 )
        {
            GEMClientActor * actor = dynamic_cast<GEMClientActor*>(target);
            if (actor)
                actor->GetVitalMgr()->Predict(csGetTicks(),"Target");
        }
    }
}

// ----------------------------------------------------------------------------

void psEngine::QuitClient()
{
    // Only run through the shut down procedure once
    static bool alreadyQuitting = false;
    if (alreadyQuitting)
    {
        return;
    }

    alreadyQuitting = true;

    loadstate = LS_NONE;

    #if defined(USE_WIN32_MINIDUMP) && !defined(CS_DEBUG)
        // If we're in release mode, there's no sense in bothering with exit crashes...
        minidumper.SetCrashAction(PSCrashActionIgnore);
    #endif

    csRef<iConfigManager> cfg( csQueryRegistry<iConfigManager> (object_reg) );
    if (cfg)
    {
        cfg->Save();
    }

    Disconnect();

    queue->GetEventOutlet()->Broadcast(event_quit);
}

void psEngine::Disconnect()
{
    netmanager->Disconnect();
}

//----------------------------------------------------------------------------

const char* psEngine::GetMainPlayerName()
{
    return GetCelClient()->GetMainPlayer()->GetName();
}

// ----------------------------------------------------------------------------

bool psEngine::UpdateWindowTitleInformations()
{
    return CS_Setup->AddWindowInformations(GetMainPlayerName());
}

// ----------------------------------------------------------------------------

void psEngine::AddLoadingWindowMsg(const csString & msg)
{
    pawsLoadWindow* window = static_cast <pawsLoadWindow*> (paws->FindWidget("LoadWindow"));
    if (window != NULL)
    {
        window->AddText( paws->Translate(msg) );
        ForceRefresh();
    }
}

/*
 * When loading this is called several times whenever there is a spare moment
 * in the event queue.  When it is called it does one of the cases and breaks out.
 */
void psEngine::LoadGame()
{
    switch(loadstate)
    {
    case LS_ERROR:
    case LS_NONE:
        return;

    case LS_LOAD_SCREEN:
    {
        paws->LoadWidget("loadwindow.xml");
        LoadPawsWidget("Active Magic window","activemagicwindow.xml" );
        HideWindow("ActiveMagicWindow");

        pawsLoadWindow* window = dynamic_cast <pawsLoadWindow*> (paws->FindWidget("LoadWindow"));
        if (!window)
        {
            FatalError("Widget: LoadWindow could not be found or is not a pawsLoadWindow widget.");
            return;
        }

        AddLoadingWindowMsg( "Loading..." );
        ForceRefresh();

        // Request MOTD
        psMOTDRequestMessage motdRe;
        GetMsgHandler()->SendMessage(motdRe.msg);

        pawsWidget* cpw = paws->FindWidget("CharPickerWindow");
        cpw->DeleteYourself();

        loadstate = LS_INIT_ENGINE;
        break;
    }

    case LS_INIT_ENGINE:
    {
        if (charController == NULL)
        {
            charController = new psCharController(nameRegistry);
            if (!charController->Initialize())
            {
                FatalError("Couldn't initialize the character controller!");
                return;
            }
        }

        // load chat bubbles
        if (!chatBubbles)
        {
            chatBubbles = new psChatBubbles();
            chatBubbles->Initialize(this);
        }

        loadstate = LS_REQUEST_WORLD;
        break;
    }

    case LS_REQUEST_WORLD:
    {
        if (!charController->IsReady())
            return;  // Wait for character modes

        // Make sure all the maps have been precached.
        bool precached = true;
        for(size_t i=0; i<mapPrecaches.GetSize(); i++)
        {
            precached &= mapPrecaches[i]->IsFinished();
        }

        if(precached)
        {
            mapPrecaches.Empty();
            vfs->SetSyncDir("/planeshift/maps/");
        }
        else
        {
            return;
        }

        celclient->RequestServerWorld();

        loadtimeout = csGetTicks () + cfgmgr->GetInt("PlaneShift.Client.User.Persisttimeout", 60) * 1000;

        if (GetSoundStatus())
        {
            soundmanager->StartMapSoundSystem();
        }

        AddLoadingWindowMsg( "Requesting connection" );
        ForceRefresh();

        loadstate = LS_SETTING_CHARACTERS;
        break;
    }

    case LS_SETTING_CHARACTERS:
    {
        if ( !celclient->IsReady() )
        {
            if (celclient->GetRequestStatus() != 0 && csGetTicks() > loadtimeout)
            {
                csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
                    "PSLoader: timeout!");

                FatalError("Timeout waiting for the world to load");
            }

            // We don't have the main actor or world yet
            return;
        }

        // Wait for the map to be loaded
        if (!HasLoadedMap())
            return;

        // Set controlled actor and map controls
        charController->GetMovementManager()->SetActor();

        // Init camera with controlled actor
        camera->InitializeView( celclient->GetMainPlayer() );

        // Get stats
        psStatsMessage statmsg;
        GetMsgHandler()->SendMessage(statmsg.msg);

        AddLoadingWindowMsg( "Getting entities" );
        AddLoadingWindowMsg( "Loading GUI" );
        ForceRefresh();

        loadstate = LS_CREATE_GUI;
        break;
    }

    case LS_CREATE_GUI:
    {
        // Must be first!!!
        if (!paws->LoadWidget( "control.xml" ))
        {
            GetMainWidget()->LockPlayer();
            FatalError("The toolbar couldn't be loaded\nPlease check your logs");
            return;
        }

        LoadPawsWidget( "Status window",           "infowindow.xml" );
        LoadPawsWidget( "Ignore window",           "ignorewindow.xml" );
        LoadPawsWidget( "Communications window",   "chat.xml" );
        LoadPawsWidget( "Inventory window",        "inventory.xml" );
        LoadPawsWidget( "Item description window", "itemdesc.xml" );
        LoadPawsWidget( "Container description window","containerdesc.xml" );
        LoadPawsWidget( "Book Reading window", "readbook.xml" );
        LoadPawsWidget( "Interact menu",           "interact.xml" );
        LoadPawsWidget( "Group status window",     "group.xml" );
        LoadPawsWidget( "Exchange window",         "exchange.xml" );
        LoadPawsWidget( "Glyph window",            "glyph.xml" );
        LoadPawsWidget( "Merchant window",         "merchant.xml" );
        LoadPawsWidget( "Petition window",         "petition.xml" );
        LoadPawsWidget( "Petititon GM window",     "petitiongm.xml" );
        LoadPawsWidget( "Spellbook window",        "spellwindow.xml" );
        LoadPawsWidget( "Shortcut window",         "shortcutwindow.xml" );
        LoadPawsWidget( "GM GUI window",           "gmguiwindow.xml" );
        LoadPawsWidget( "Control panel",           "configwindow.xml" );
        LoadPawsWidget( "Fingering window",        "fingering.xml" );
        LoadPawsWidget( "Guild information window","guildwindow.xml" );
        LoadPawsWidget( "Loot window",             "loot.xml" );
        LoadPawsWidget( "Skills window",           "skillwindow.xml" );
        LoadPawsWidget( "PetStats window",         "petstatwindow.xml" );
        LoadPawsWidget( "Quest notebook window",   "questnotebook.xml" );
        LoadPawsWidget( "Spell cast status window","spellcancelwindow.xml" );
        LoadPawsWidget( "Help window",             "helpwindow.xml" );
        LoadPawsWidget( "Buddy window",            "buddy.xml" );
        LoadPawsWidget( "Action Location window",  "actionlocation.xml" );
        LoadPawsWidget( "Details window",          "detailwindow.xml" );
        LoadPawsWidget( "Character description window","chardescwindow.xml" );
        LoadPawsWidget( "Quest reward window",     "questrewardwindow.xml" );
        LoadPawsWidget( "GM Spawn interface",      "gmspawn.xml" );
        //LoadPawsWidget( "Active Magic window",   "activemagicwindow.xml" );
        LoadPawsWidget( "Small Inventory Window",  "smallinventory.xml" );
        LoadPawsWidget( "GM Action Location Edit", "gmaddeditaction.xml" );
        LoadPawsWidget( "Crafting",                "craft.xml");
        LoadPawsWidget( "Tutorial",                "tutorial.xml");
        LoadPawsWidget( "Sketch",                  "illumination.xml");
        LoadPawsWidget( "GameBoard",               "gameboard.xml");
        LoadPawsWidget( "Writing window",          "bookwriting.xml");
        LoadPawsWidget( "NPC dialog window",       "dialog.xml");

        LoadCustomPawsWidgets("/data/gui/customwidgetslist.xml");

        HideWindow("DescriptionEdit");
        HideWindow("ItemDescWindow");
        HideWindow("BookReadingWindow");
        HideWindow("ContainerDescWindow");
        HideWindow("GroupWindow");
        HideWindow("InteractWindow");
        HideWindow("ExchangeWindow");
        HideWindow("MerchantWindow");
        HideWindow("ShortcutEdit");
        HideWindow("FingeringWindow");
        HideWindow("QuestEdit");
        HideWindow("LootWindow");
        HideWindow("ActionLocationWindow");
        HideWindow("DetailWindow");
        HideWindow("QuestRewardWindow");
        HideWindow("SpellCancelWindow");
        HideWindow("GMSpawnWindow");
        HideWindow("SmallInventoryWindow");
        HideWindow("AddEditActionWindow");
        HideWindow("GmGUI");
        HideWindow("PetStatWindow");
        HideWindow("WritingWindow");

        paws->GetMouse()->ChangeImage("Skins Normal Mouse Pointer");

        // If we had any problems, show them
        if ( wdgProblems.GetSize() > 0 )
        {
            printf("Following widgets failed to load:\n");

            // Loop through the array
            for (size_t i = 0;i < wdgProblems.GetSize(); i++)
            {
                csString str = wdgProblems.Get(i);
                if (str.Length() > 0)
                    printf("%s\n",str.GetData());
            }

            GetMainWidget()->LockPlayer();
            FatalError("One or more widgets failed to load\nPlease check your logs");
            return;
        }

        celclient->SetPlayerReady(true);

        psClientStatusMessage statusmsg(true);
        statusmsg.SendMessage();

        //GetCmdHandler()->Execute("/who");
        GetCmdHandler()->Execute("/tip");

        // load the mouse options if not loaded allready. The GetMouseBinds
        // function will load them if they are requested before this code
        // is executed.
        if (!mouseBinds)
        {
            mouseBinds = GetMouseBinds();
        }

        // Set the focus to the main widget
        paws->SetCurrentFocusedWidget(GetMainWidget());

        loadstate = LS_DONE;
        break;
    }


    default:
        loadstate = LS_NONE;
    }

    csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
        "PSLoader: step %d: success", loadstate);

    if (loadstate==LS_DONE)
    {
        gameLoaded = true;
        paws->FindWidget("LoadWindow")->Hide();
    }
}

void psEngine::HideWindow(const csString & widgetName)
{
    pawsWidget * wnd = paws->FindWidget(widgetName);
    if (wnd != NULL)
        wnd->Hide();
}

psMouseBinds* psEngine::GetMouseBinds()
{
    // If not loaded load the mouse binds. They should normaly be loaded
    // from the load GUI step, but this function have been observerd called
    // before.
    if (!mouseBinds)
    {
        mouseBinds = new psMouseBinds();

        csString fileName = "/planeshift/userdata/options/mouse.xml";
        if (!vfs->Exists(fileName))
        {
            fileName = "/planeshift/data/options/mouse_def.xml";
        }

        if ( !mouseBinds->LoadFromFile( object_reg, fileName))
            Error1("Failed to load mouse options");
    }
    return mouseBinds;
}

size_t psEngine::GetTime()
{
    return modehandler->GetTime();
}

void psEngine::UpdateLights()
{
    modehandler->UpdateLights();
}

void psEngine::SetDuelConfirm(int confirmType)
{
    confirmation = confirmType;

    csString xml;
    xml = "<PvP>\n";
    xml += "    <Confirmation value=\"";
    xml.Append(confirmation);
    xml += "\"/>\n";
    xml += "</PvP>\n";

    vfs->WriteFile("/planeshift/userdata/options/pvp.xml", xml.GetData(), xml.Length());
}

bool psEngine::LoadDuelConfirm()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, mainNode, optionNode;

    csString fileName = "/planeshift/userdata/options/pvp.xml";
    if (!psengine->GetVFS()->Exists(fileName))
    {
        fileName = "/planeshift/data/options/pvp_def.xml";
    }

    doc = ParseFile(object_reg, fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("pvp.xml has no XML root");
        return false;
    }
    mainNode = root->GetNode("PvP");
    if (mainNode == NULL)
    {
        Error1("pvp.xml has no <PvP> tag");
        return false;
    }

    optionNode = mainNode->GetNode("Confirmation");
    if (optionNode != NULL)
    {
        confirmation = optionNode->GetAttributeValueAsInt("value");
    }

    return true;
}

bool psEngine::LoadSoundSettings(bool forceDef)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, mainNode, optionNode;

    csString fileName;
    if(!forceDef)
        fileName = "/planeshift/userdata/options/sound.xml";

    if (forceDef || !psengine->GetVFS()->Exists(fileName))
    {
        fileName = "/planeshift/data/options/sound_def.xml";
    }

    doc = ParseFile(object_reg, fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("%s has no XML root",fileName.GetData());
        return false;
    }
    mainNode = root->GetNode("sound");
    if (mainNode == NULL)
    {
        Error2("%s has no <sound> tag",fileName.GetData());
        return false;
    }

    // load and apply the settings
    optionNode = mainNode->GetNode("ambient");
    if (optionNode != NULL)
        GetSoundManager()->ToggleAmbient(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("actions");
    if (optionNode != NULL)
        GetSoundManager()->ToggleActions(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("music");
    if (optionNode != NULL)
        GetSoundManager()->ToggleMusic(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("gui");
    if (optionNode != NULL)
        paws->ToggleSounds(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("voices");
    if (optionNode != NULL)
        GetSoundManager()->ToggleVoices(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("volume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        GetSoundManager()->SetVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("musicvolume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        GetSoundManager()->SetMusicVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("ambientvolume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        GetSoundManager()->SetAmbientVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("actionsvolume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        GetSoundManager()->SetActionsVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("guivolume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        paws->SetVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("voicesvolume");
    if (optionNode != NULL)
    {
        // Failsafe
        int volume = optionNode->GetAttributeValueAsInt("value");
        if(volume == 0)
        {
            printf("Invalid sound setting, setting to 100%%\n");
            volume = 100;
        }

        GetSoundManager()->SetVoicesVolume(float(volume)/100);
    }

    optionNode = mainNode->GetNode("muteonfocusloss");
    if (optionNode != NULL)
        SetMuteSoundsOnFocusLoss(optionNode->GetAttributeValueAsBool("on", false));

    optionNode = mainNode->GetNode("loopbgm");
    if (optionNode)
        GetSoundManager()->ToggleLoop(optionNode->GetAttributeValueAsBool("on", false));

    optionNode = mainNode->GetNode("combatmusic");
    if (optionNode)
        GetSoundManager()->ToggleCombatMusic(optionNode->GetAttributeValueAsBool("on", true));

    return true;
}

bool psEngine::LoadPawsWidget(const char* title, const char* filename)
{
    bool loaded = paws->LoadWidget(filename);

    if (!loaded)
    {
        wdgProblems.Push(title);
    }

    return loaded;
}

bool psEngine::LoadCustomPawsWidgets(const char * filename)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    csRef<iDocumentSystem>  xml;
    const char* error;

    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
    if (buff == NULL)
    {
        Error2("Could not find file: %s", filename);
        return false;
    }
    xml = psengine->GetXMLParser ();
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", filename, error);
        return false;
    }
    if (doc == NULL)
        return false;

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("No root in XML %s", filename);
        return false;
    }

    csRef<iDocumentNode> customWidgetsNode = root->GetNode("custom_widgets");
    if (!customWidgetsNode)
    {
        Error2("No custom_widgets node in %s", filename);
        return false;
    }

    csRef<iDocumentNodeIterator> nodes = customWidgetsNode->GetNodes("widget");
    while (nodes->HasNext())
    {
        csRef<iDocumentNode> widgetNode = nodes->Next();

        csString name = widgetNode->GetAttributeValue("name");
        csString file = widgetNode->GetAttributeValue("file");

        LoadPawsWidget(name, file);
    }
    return true;
}

void psEngine::FatalError(const char* msg)
{
    loadstate = LS_ERROR;
    Bug2("%s\n", msg);

    pawsQuitInfoBox* quitinfo = (pawsQuitInfoBox*)(paws->FindWidget("QuitInfoWindow"));
    if (quitinfo)
    {
        quitinfo->SetBox(msg);
        quitinfo->Show();
    }
    else
    {
        // Force crash to give dump here
        int* crash = NULL;
        *crash=0;
        QuitClient();
    }
}

void psEngine::setLimitFPS(int a)
{
    maxFPS = a;
    frameLimit = ( 1000 / maxFPS );
    cfgmgr->SetInt("Video.frameLimit", maxFPS);
    cfgmgr->Save();// need to save this every time
}

// ----------------------------------------------------------------------------

/**
 * pureCallHandler()
 * Handles all Pure Virtual Function calls for MSVC.
 */
#ifdef CS_COMPILER_MSVC
void pureCallHandler()
{
   CS::Debug::AssertMessage("false", __FILE__, __LINE__, "Pure Virtual Function Call!\n");
}
#endif

/**
 * main()
 * Main function. Like any other applications, this is the entry point of the
 * program. The SysSystemDriver is created and initialized here. It also
 * creates psEngine and initialize it. Then it calls the main loop.
 **/

psEngine * psengine;

int main (int argc, char *argv[])
{
#ifdef CS_COMPILER_MSVC
    _set_purecall_handler(pureCallHandler);
#endif
    psCSSetup *CSSetup = new psCSSetup( argc, argv, "/this/psclient.cfg", CONFIGFILENAME );
    iObjectRegistry* object_reg = CSSetup->InitCS();

    pslog::Initialize (object_reg);
    pslog::disp_flag[LOG_LOAD] = true;

    // Create our application object
    psengine = new psEngine(object_reg, CSSetup);


    // Initialize engine
    if (!psengine->Initialize(0))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
                  PSAPP, "Failed to init app!");
        PS_PAUSEEXIT(1);
    }

    // start the main event loop
    csDefaultRunLoop(object_reg);

    psengine->Cleanup();


    Notify1(LOG_ANY,"Removing engine...");

    // remove engine before destroying Application
    delete psengine;
    psengine = NULL;

    delete CSSetup;

    Notify1(LOG_ANY,"Destroying application...");

    csInitializer::DestroyApplication(object_reg);

    return 0;
}
