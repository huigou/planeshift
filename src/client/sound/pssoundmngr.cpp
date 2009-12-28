/*
 * pssoundmngr.cpp. Saul Leite <leite@engineer.com>
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
 * The main manager of sounds and sound resource handler.
 */
#include <psconfig.h>

// CS files
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/camera.h>
#include <iutil/cfgmgr.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/virtclk.h>
#include <iutil/vfs.h>
#include <imap/loader.h>
#include <ivaria/collider.h>
#include <ivaria/reporter.h>
#include <ivaria/keyval.h>
#include <iutil/databuff.h>
#include <iutil/stringarray.h>
#include <csver.h>
#include <cstool/collider.h>
#include <cstool/initapp.h>
#include <cstool/csview.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <csgeom/sphere.h>
#include <csutil/randomgen.h>

// PS includes
#include "pssoundmngr.h"
#include "util/psxmlparser.h"
#include "util/log.h"
#include "net/messages.h"  // system message type #defines

// Max volume is 90% of overall max.  At 100% distortion can occur
#define MAX_VOLUME 0.90f

// Minimum volume before pausing
#define MIN_VOLUME 0.001f

psSoundManager::psSoundManager(iBase* iParent)
    : scfImplementationType(this, iParent) ,  sndmngr(this)
{

    soundEnabled = true;
    musicEnabled = true;
    ambientEnabled = true;
    actionsEnabled = true;
    guiEnabled = true;
    voicesEnabled = true;
    loopBGM = true;
    combatMusicEnabled = true;

    musicCombat = false;

    musicVolume = 1.0;
    ambientVolume = 1.0;
    actionsVolume = 1.0;
    guiVolume = 1.0;
    voicesVolume = 1.0;

    eventHandler = NULL;
    currentSectorName = csString("");

    mapSoundSystem = NULL;

    currentSoundSector = NULL;
    lastSoundSector = NULL;
    lastlastSoundSector = NULL;
    backgroundSong = 0;
    ListenerInitialized = false;

    overSongName = NULL;
}

psSoundManager::~psSoundManager()
{
    csRef<iEventQueue> queue =  csQueryRegistry<iEventQueue > ( object_reg);
    if ( eventHandler && queue )
    {
        queue->RemoveListener( eventHandler );
    }


    delete backgroundSong;
    delete mapSoundSystem;
}

bool psSoundManager::Initialize(iObjectRegistry* objectReg)
{
    object_reg = objectReg;

    pslog::Initialize (object_reg);

    eventHandler = new EventHandler( this );
    if ( !eventHandler )
        return false;

    csRef<iEventQueue> queue =  csQueryRegistry<iEventQueue > ( objectReg);

    if ( !queue )
        return false;

    csEventID esub[] = {
          csevFrame (object_reg),
          CS_EVENTLIST_END
    };
    queue->RegisterListener( eventHandler, esub );

    mapSoundSystem = new psMapSoundSystem( this, objectReg );

    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (object_reg);
    if (!cfg)
    {
        return false;
    }

    soundLib = cfg->GetStr("PlaneShift.Sound.SoundLib", "/planeshift/art/soundlib.xml");
    return true;
}

void psSoundManager::StreamAddNotification(iSndSysStream *pStream)
{
//	printf("StreamAdd Notification\n");
}
void psSoundManager::StreamRemoveNotification(iSndSysStream *pStream)
{
//	printf("StreamRemove Notification\n");
}

void psSoundManager::SourceAddNotification(iSndSysSource *pSource)
{
//	printf("SourceAdd Notification\n");
}

void psSoundManager::SourceRemoveNotification(iSndSysSource *pSource)
{
//	printf("SourceRemove Notification\n");
	if (currentVoiceSource && currentVoiceSource == pSource)
	{
//		printf("Voice file ended.  Checking for another.\n");
		voicingQueue.DeleteIndex(0);

		if (voicingQueue.GetSize() > 0)
		{
//			printf("Playing next voice file.\n");
			currentVoiceSource = StartSound(voicingQueue.Get(0),voicesVolume,false);
		}
	}
}


void psSoundManager::ChangeTimeOfDay( int newTime )
{
    if ( currentSoundSector )
        currentSoundSector->ChangeTime( newTime );
}

void psSoundManager::StartMapSoundSystem()
{
    mapSoundSystem->Initialize();
}

void psSoundManager::Update( iView* view )
{
  if (!soundSystem)
    return;
  SOUND_LISTENER_TYPE *sndListener = soundSystem->GetListener();
  if (sndListener)
  {
    ListenerInitialized=true;

    // take position/direction from view->GetCamera ()
    csVector3 v = view->GetPerspectiveCamera ()->GetCamera ()->GetTransform ().GetOrigin ();
    csMatrix3 m = view->GetPerspectiveCamera ()->GetCamera ()->GetTransform ().GetT2O();
    csVector3 f = m.Col3();
    csVector3 t = m.Col2();
    sndListener->SetPosition(v);
    sndListener->SetDirection(f,t);
  }
}

void psSoundManager::Update( csVector3 pos  )
{
    static unsigned int count = 0;
    if (++count%5 != 0)  // Update emitter positions once every 5th frame
        return;

     mapSoundSystem->Update( pos );
}

void psSoundManager::UpdateWeather(int weather)
{
    if(!currentSoundSector)
        return;

    currentSoundSector->UpdateWeather(weather);
}

void psSoundManager::UpdateWeather(int weather,const char* sector)
{
    psSectorSoundManager* sec  = mapSoundSystem->GetSoundSectorByName(sector);

    if(!sec)
        return;

    sec->UpdateWeather(weather);
}

bool psSoundManager::Setup()
{
  if (soundSystem.IsValid())
    return true;

    engine =  csQueryRegistry<iEngine> (object_reg);
    if (!engine)
    {
        Error1("Couldn't find iEngine!");
        return false;
    }

    if (!sndmngr.Initialize())
    {
        Error1("Couldn't initialize sndmngr!");
        return false;
    }

    if (!sndmngr.LoadSoundLib(soundLib.GetData()))
    {
        Error1("Couldn't load Soundlib!");
        return false;
    }

    soundSystem =  csQueryRegistry<iSndSysRenderer> (object_reg);
    if (!soundSystem)
    {
        Error1("Couldn't find iSoundRender!");
        return false;
    }

	if (!soundSystem->RegisterCallback(this))
	{
		Error1("Couldn't register iSndSysRenderer callback");
		return false;
	}

    return true;
}

csRef<iSndSysSource> psSoundManager::StartMusicSound(const char* name,bool loop)
{
    if (musicEnabled)
        return StartSound(name,musicVolume,loop);
    else
        return NULL;
}

csRef<iSndSysSource> psSoundManager::StartAmbientSound(const char* name,bool loop)
{
    if (ambientEnabled)
        return StartSound(name,ambientVolume,loop);
    else
        return NULL;
}

csRef<iSndSysSource> psSoundManager::StartGUISound(const char* name,bool loop)
{
    if (guiEnabled)
        return StartSound(name,guiVolume,loop);
    else
        return NULL;
}

csRef<iSndSysSource> psSoundManager::StartVoiceSound(const char* name,bool loop)
{
    if (!voicesEnabled)
		return NULL;
	
	voicingQueue.Push(name);

	if (voicingQueue.GetSize() == 1) // this is the only voice file
	{
	    currentVoiceSource = StartSound(name,voicesVolume,loop);
		return currentVoiceSource;
	}
	else
	{
		return NULL;
	}
}

csRef<iSndSysSource> psSoundManager::StartActionsSound(const char* name,bool loop)
{
    if(actionsEnabled)
        return StartSound(name,actionsVolume,loop);
    else
        return NULL;
}

csRef<iSndSysSource> psSoundManager::StartSound(const char* name,float volume,bool loop)
{
    if ( !name )
        return NULL;

    if (!soundSystem)
        return NULL;

    if (!soundEnabled)
        return NULL;

    csRef<psSoundHandle> handle = sndmngr.CreateSound(name);
    if (!handle)
    {
        Error2("Sound '%s' not found!", name);
        return NULL;
    }
    if (!handle->snddata)
    {
        Error2("Sound '%s' was found but could not be loaded!", name);
        return NULL;
    }
    csRef<iSndSysStream> sndstream = soundSystem->CreateStream(handle->snddata, CS_SND3D_DISABLE);
    if (!sndstream)
    {
      Error2("Failed to create stream for  '%s'!", name);
      return NULL;
    }

    csRef<iSndSysSource> sndsource = soundSystem->CreateSource(sndstream);
    if (!sndsource.IsValid())
    {
      Error2("Failed to create source for  '%s'!", name);
      return NULL;
    }

    if (loop)
      sndstream->SetLoopState(CS_SNDSYS_STREAM_LOOP);
    else
      sndstream->SetLoopState(CS_SNDSYS_STREAM_DONTLOOP);
    sndstream->SetAutoUnregister(true);
    sndsource->SetVolume(volume);
    sndstream->Unpause();

    return sndsource;
}


void psSoundManager::HandleSoundType(int type)
{
    if(!actionsEnabled)      // In-game action sounds are turned off.
        return;

    switch (type)
    {
        case MSG_ERROR:             StartGUISound("sound.msg.error");    break;
        case MSG_INFO:              StartGUISound("sound.msg.info");     break;
       /* case MSG_COMBAT_OWN_DEATH:  StartMusicSound("sound.msg.death");    break;
        case MSG_COMBAT_VICTORY:    StartMusicSound("sound.msg.victory");  break;*/
        case MSG_PURCHASE:          StartGUISound("sound.msg.purchase"); break;
    }
}

void psSoundManager::SetVolume(float vol)
{
    if (!soundSystem)
        return;
    soundSystem->SetVolume(vol);
}

float psSoundManager::GetVolume()
{
    if (!soundSystem)
        return -1;
    return soundSystem->GetVolume();
}

void psSoundManager::SetMusicVolume(float vol)
{
    musicVolume = vol;
    if(backgroundSong)
    {
        backgroundSong->SetVolume(vol);
        backgroundSong->Update();
    }
    if(mapSoundSystem)
    {
        mapSoundSystem->SetMusicVolumes(musicVolume);
    }
}

float psSoundManager::GetMusicVolume()
{
    return musicVolume;
}

void psSoundManager::SetAmbientVolume(float vol)
{
    ambientVolume = vol;
    if(mapSoundSystem)
    {
        mapSoundSystem->SetAmbientVolumes(ambientVolume);
        mapSoundSystem->SetEmitterVolumes(ambientVolume);
    }
}

float psSoundManager::GetAmbientVolume()
{
    return ambientVolume;
}

void psSoundManager::SetActionsVolume(float vol)
{
    actionsVolume = vol;
}

float psSoundManager::GetActionsVolume()
{
    return actionsVolume;
}

void psSoundManager::SetGUIVolume(float vol)
{
    guiVolume = vol;
}

float psSoundManager::GetGUIVolume()
{
    return guiVolume;
}

void psSoundManager::SetVoicesVolume(float vol)
{
    voicesVolume = vol;
}

float psSoundManager::GetVoicesVolume()
{
    return voicesVolume;
}

void psSoundManager::StopOverrideBG()
{
    if ( backgroundSong )
        backgroundSong->Stop();
    delete backgroundSong;
    backgroundSong = NULL;
}


bool psSoundManager::OverrideBGSong(const char* name, bool loop, float fadeTime)
{
    if (!soundSystem || !musicEnabled)
        return false;

    overSongName = name;
    if ( !backgroundSong )
    {
      csRef<iSndSysData> snddata = GetSoundResource( name );
      if (!snddata.IsValid())
        return false;
      csRef<iSndSysStream> sndstream = soundSystem->CreateStream(snddata, CS_SND3D_DISABLE);
      if (!sndstream.IsValid())
        return false;

      backgroundSong = new psSoundObject(sndstream,mapSoundSystem, 1.0, 0.0, (int)fadeTime, 0, 0 );
      backgroundSong->SetVolume( musicVolume );
      backgroundSong->SetResource( name );
      backgroundSong->StartFade( FADE_UP );
    }
    else
    {

      csRef<iSndSysData> snddata = GetSoundResource( name );
      if (!snddata.IsValid())
        return false;
      csRef<iSndSysStream> sndstream = soundSystem->CreateStream(snddata, CS_SND3D_DISABLE);
      if (!sndstream.IsValid())
        return false;

      psSoundObject* backgroundSongNew = new psSoundObject( sndstream,mapSoundSystem, 1.0, 0.0, (int)fadeTime, 0, 0 );
      backgroundSongNew->SetVolume( musicVolume );
      backgroundSongNew->SetResource( name );

      if ( backgroundSongNew->Same(backgroundSong) )
      {
          delete backgroundSongNew; backgroundSongNew = NULL;
          return true;
      }
      else
      {
          StopOverrideBG();
          backgroundSong = backgroundSongNew;
          backgroundSong->StartFade( FADE_UP );
      }
    }

    return true;
}


void psSoundManager::ToggleMusic(bool toggle)
{
    if (!soundSystem)
        return;

    // Enable Music
    if (toggle)
    {
        musicEnabled = true;
        if ( currentSoundSector )
            currentSoundSector->StartBackground();

        if (musicCombat)
        {
            OverrideBGSong("combat",iSoundManager::LOOP_SOUND, 2.0);
            FadeSectorSounds( FADE_DOWN );
        }
      // if ( lastSoundSector ) lastSoundSector->StartBackground();
    // Disable Music
    }
    else
    {
          // stop background music

        if (backgroundSong)
        {
            backgroundSong->Stop();
        }
        musicEnabled = false;
        if ( currentSoundSector )
            currentSoundSector->StopBackground();
        if ( lastSoundSector )
            lastSoundSector->StopBackground();
        if (lastlastSoundSector )
            lastlastSoundSector->StopBackground();
    }

    mapSoundSystem->EnableMusic( musicEnabled );
}

void psSoundManager::ToggleAmbient(bool toggle)
{
    if (!soundSystem)
        return;

    // Enable Ambient Sounds
    if (toggle)
    {
        ambientEnabled = true;
        if ( currentSoundSector )
        {
            int weather = currentSoundSector->GetWeather();
            //TODO: Getting the time
            currentSoundSector->StartSounds(weather);
        }
       // if ( lastSoundSector ) lastSoundSector->StartSounds();

    // Disable Sounds
    } else {
        ambientEnabled = false;
        if ( currentSoundSector ) currentSoundSector->StopSounds( );
        if ( lastSoundSector ) lastSoundSector->StopSounds();

    }
    mapSoundSystem->EnableSounds( ambientEnabled );
}

void psSoundManager::ToggleActions(bool toggle)
{
    actionsEnabled = toggle;
}

void psSoundManager::ToggleGUI(bool toggle)
{
    guiEnabled = toggle;
}

void psSoundManager::ToggleVoices(bool toggle)
{
    voicesEnabled = toggle;
}

void psSoundManager::ToggleLoop(bool toggle)
{
    loopBGM = toggle;
}

void psSoundManager::ToggleCombatMusic(bool toggle)
{
    combatMusicEnabled = toggle;
    if (!combatMusicEnabled) musicCombat = false;
}

bool psSoundManager::HandleEvent( iEvent& event )
{
    if ( backgroundSong )
    {
        backgroundSong->Update();
        if(!backgroundSong->IsPlaying())
        {
            StopOverrideBG();
            FadeSectorSounds( FADE_UP );
        }
    }
    mapSoundSystem->Update();

    return false;
}

csRef<iSndSysData> psSoundManager::GetSoundResource(const char *name)
{
    csRef<psSoundHandle> soundhandle;
    soundhandle=sndmngr.CreateSound(name);
    if (!soundhandle.IsValid())
        return NULL;

    return soundhandle->snddata;
}

//---------------------------------------------------------------------------

struct psSoundFileInfo
{
    csString name;
    csString file;
};

psSoundManager::psSndSourceMngr::psSndSourceMngr(psSoundManager* nparent)
    : parent(nparent)
{
}

psSoundManager::psSndSourceMngr::~psSndSourceMngr()
{
    csHash<psSoundFileInfo *>::GlobalIterator it (sndfiles.GetIterator ());
    while (it.HasNext ())
    {
        psSoundFileInfo* sndfile = it.Next ();
        delete sndfile;
    }
}

bool psSoundManager::psSndSourceMngr::Initialize()
{
    sndloader =  csQueryRegistry<iSndSysLoader> (parent->object_reg);
    if (!sndloader)
    {
      Error1("psSndSourceMngr: Could not initialize. Cannot find iSndSysLoader");
        return false;
    }

    vfs =  csQueryRegistry<iVFS> (parent->object_reg);
    if (!vfs)
    {
        Error1("psSndSourceMngr: Could not initialize. Cannot find iVFS");
        return false;
    }

    return true;
}

bool psSoundManager::psSndSourceMngr::LoadSoundLib(const char* fname)
{
    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem> (parent->object_reg);
    if (!xml)
        xml = csPtr<iDocumentSystem> (new csTinyDocumentSystem);

    csRef<iDataBuffer> buff = vfs->ReadFile( fname );

    if ( !buff || !buff->GetSize() )
    {
        return false;
    }

    csRef<iDocument> doc=xml->CreateDocument();

    const char* error=doc->Parse(buff);
    if (error)
    {
        Error3("Parsing file %s gave error %s", fname, error);
        return false;
    }
    csRef<iDocumentNode> root=doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in soundlib.xml");
        return false;
    }

    csRef<iDocumentNode> topNode=root->GetNode("Sounds");
    if(!topNode)
    {
        Error1("No <sounds> tag in soundlib.xml");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if ( strcmp( node->GetValue(), "Sound" ) == 0 )
        {
            psSoundFileInfo* info = new psSoundFileInfo;
            info->name = node->GetAttributeValue("name");
            info->file = node->GetAttributeValue("file");
            sndfiles.Put(csHashCompute(info->name), info);
        }
    }
    return true;

}

csRef<iSndSysData>
psSoundManager::psSndSourceMngr::LoadSound(const char* fname)
{
    if (!parent->soundSystem)
        return NULL;
    csRef<iDataBuffer> buf = vfs->ReadFile(fname);
    if (!buf)
    {
        Error2("Error while reading file '%s'", fname);
        return NULL;
    }

    csRef<iSndSysData> sounddata = sndloader->LoadSound (buf);
    if (!sounddata)
    {
        Error2("Cannot create sound data from file '%s'", fname);
        return NULL;
    }

    return sounddata;
}

csRef<psSoundHandle> psSoundManager::psSndSourceMngr::CreateSound
    (const char* name)
{
    csRef<psTemplateRes> rest = CreateResource(name);
    csRef<psSoundHandle> res = (psSoundHandle*) ((psTemplateRes*) rest);
    if (!res)
        return NULL;

    return res;
}

/**
 * Check to see if a background song is already playing.
 *
 * Get the file info from both the resource names.
 * If they are the same file then return true.
 */
bool psSoundManager::psSndSourceMngr::CheckAlreadyPlaying( psSoundHandle* oldResourceHandle,
                                                           const char* newResource )
{
    if ( !oldResourceHandle ) return false;

    const char* oldResource = oldResourceHandle->GetName();

    csHash<psSoundFileInfo *>::Iterator i = sndfiles.GetIterator(csHashCompute(oldResource));
    psSoundFileInfo* oldResourceInfo = NULL;
    while ( (oldResourceInfo = i.Next()) )
    {
        if (!strcmp(oldResourceInfo->name, oldResource))
            break;
    }

    if ( !oldResourceInfo ) return false;

    csHash<psSoundFileInfo *>::Iterator itr = sndfiles.GetIterator(csHashCompute(newResource));

    psSoundFileInfo* newResourceInfo = NULL;
    while ( (newResourceInfo = itr.Next()) )
    {
        if (!strcmp(newResourceInfo->name, newResource))
            break;
    }
    if ( !newResourceInfo ) return false;

    if ( !strcmp(oldResourceInfo->file, newResourceInfo->file) )
        return true;

    return false;
}


csPtr<psTemplateRes> psSoundManager::psSndSourceMngr::LoadResource
                                (const char* name)
{
    csRef<iSndSysData> snddata;

    if (name[0] != '/') // not a pathname
    {
        csHash<psSoundFileInfo *>::Iterator i = sndfiles.GetIterator(csHashCompute(name));
        psSoundFileInfo* info = NULL;
        while (  i.HasNext()  )
        {
            info = i.Next();
            if (!strcmp(info->name, name))
                break;
        }
        if (!info)
        {
            Error2("Couldn't find a definition for '%s'", name);
            return NULL;
        }

        snddata = LoadSound(info->file);
        if (!snddata)
          return NULL;
    }
    else
    {
        snddata = LoadSound(name);
    }
    return csPtr <psTemplateRes> (new psSoundHandle(snddata));
}

//-------------------------------------------------------------------

void psSoundManager::EnterSector( const char* sector, int timeOfDay, int weather, csVector3& position )
{
    if ( currentSectorName == sector )
        return;
    else
        currentSectorName = sector;

    lastlastSoundSector = lastSoundSector;
    lastSoundSector = currentSoundSector;
    currentSoundSector = mapSoundSystem->GetSoundSectorByName(sector);

    // Transfer sound settings
    if ( currentSoundSector )
    {
        currentSoundSector->Music(musicEnabled);
        currentSoundSector->Sounds(ambientEnabled);
    }


    // mapSoundSystem::EnterSector must be called BEFORE entering the new sector or the new
    //  sector's emitters will be pruned to only what can be heard at the zone-in
    mapSoundSystem->EnterSector( currentSoundSector );

    if ( currentSoundSector )
        currentSoundSector->Enter( lastSoundSector, timeOfDay, weather, position );

}

void psSoundManager::FadeSectorSounds( Fade_Direction dir )
{
    if ( currentSoundSector )
    {
        currentSoundSector->Fade( dir );
    }
}


SCF_IMPLEMENT_FACTORY (psSoundManager)


//-----------------------------------------------------------------------------


psMapSoundSystem::psMapSoundSystem( psSoundManager* mngr, iObjectRegistry* object )
{
    objectReg = object;
    sndmngr = mngr;
}

psMapSoundSystem::~psMapSoundSystem()
{
	csHash<psSectorSoundManager*, csString>::GlobalIterator iter(sectors.GetIterator());

    while(iter.HasNext())
    {
    	psSectorSoundManager* sect = iter.Next();
    	delete sect;
    }
}


void psMapSoundSystem::EnableMusic( bool enable )
{
	csHash<psSectorSoundManager*, csString>::GlobalIterator iter(sectors.GetIterator());

    while(iter.HasNext())
    {
    	psSectorSoundManager* sect = iter.Next();
    	sect->Music( enable );
    }
}


void psMapSoundSystem::EnableSounds( bool enable )
{
	csHash<psSectorSoundManager*, csString>::GlobalIterator iter(sectors.GetIterator());
    Fade_Direction dir;

    if  ( enable )
        dir = FADE_UP;
    else
        dir = FADE_DOWN;

    while(iter.HasNext())
    {
    	psSectorSoundManager* sect = iter.Next();
        sect->Sounds( enable );
    }
}


psSectorSoundManager* psMapSoundSystem::GetSoundSectorByName(const char* name)
{
    psSectorSoundManager* sectora = sectors.Get(name, NULL);
    return sectora;
}

// This is when we need to create the sector sound manager before it should have been
psSectorSoundManager* psMapSoundSystem::GetPendingSoundSector(const char* name)
{
    csString sector(name);

    for(size_t i = 0; i < pendingSectors.GetSize(); i++)
    {
        psSectorSoundManager* obj = pendingSectors[i];
        if( sector == obj->GetSector() )
            return obj;
    }

    return NULL;
}

psSectorSoundManager* psMapSoundSystem::GetOrCreateSector(const char* name)
{
    psSectorSoundManager* trigSector = NULL;
    csString sec(name);

    // Check if we loaded that sector
    trigSector = GetSoundSectorByName(sec);
    if(!trigSector)
        trigSector = GetPendingSoundSector(sec); // Try pending too

    if(!trigSector) // still no? create a pending one
    {
        trigSector = new psSectorSoundManager( sec, sndmngr->GetEngine(), this );
        pendingSectors.Push(trigSector);
    }

    return trigSector;
}

int psMapSoundSystem::TriggerStringToInt(const char* str)
{
    csString trigS(str);

    // Convert to enum
    int trigger = 0;
    if(trigS.CompareNoCase("weather"))
        trigger = SOUND_EVENT_WEATHER;

    return trigger;
}

bool psMapSoundSystem::Initialize()
{
    Debug1( LOG_SOUND, 0, "Starting Map Sound System");

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> ( objectReg);
    if (!vfs)
        return false;

    csRef<iDataBuffer> xpath = vfs->ExpandPath("/planeshift/world/sound/");
    const char* dir = **xpath;
    csRef<iStringArray> files = vfs->FindFiles(dir);
    if (!files)
        return false;

    for (size_t i=0; i < files->GetSize(); i++)
    {
        csString name( files->Get(i) );
        csRef<iDocument> doc;
        csRef<iDocumentNode> root, mapNode;

        if ( (doc=ParseFile( objectReg, name )) && (root=doc->GetRoot()) && (mapNode=root->GetNode("MAP_SOUNDS")) )
        {
            csRef<iDocumentNodeIterator> sectorIter = mapNode->GetNodes("SECTOR");
            while ( sectorIter->HasNext() )
            {
                csRef<iDocumentNode> sector = sectorIter->Next();
                csString sectorName = sector->GetAttributeValue("NAME");
                Debug2(LOG_SOUND, 0, "Loading Sound System for Sector: %s", sectorName.GetData() );
                psSectorSoundManager* manager = NULL;

                // See if it's already created
                if(!(manager = GetSoundSectorByName(sectorName)))
                {
                    if( (manager = GetPendingSoundSector(sectorName)) )
                    {
                        // Delete the pending one from the list and use it
                        pendingSectors.Delete(manager);
                    }
                    else
                    {
                        // Create new
                        manager = new psSectorSoundManager( sectorName, sndmngr->GetEngine(), this );
                    }
                }

                // we can now be sure we have a manager
                sectors.Put(sectorName, manager);
                Debug2( LOG_SOUND,0, "Added sector %s to the list",manager->GetSector());

                csRef<iDocumentNodeIterator> ambientItr = sector->GetNodes("AMBIENT");
                while ( ambientItr->HasNext() )
                {
                    csRef<iDocumentNode> ambientNode = ambientItr->Next();
                    csString resource = ambientNode->GetAttributeValue("RESOURCE");
                    float minVol =  ambientNode->GetAttributeValueAsFloat("MINVOL");
                    float maxVol =  ambientNode->GetAttributeValueAsFloat("MAXVOL");
                    int fadeDelay = ambientNode->GetAttributeValueAsInt("FADEDELAY");
                    int timeOfDay = ambientNode->GetAttributeValueAsInt("TIME");
                    int timeOfDayRange = ambientNode->GetAttributeValueAsInt("TIME_RANGE");
                    int weather   = ambientNode->GetAttributeValueAsInt("WEATHER");
                    csString trigS= ambientNode->GetAttributeValue("TRIGGER");
                    csString sec  = ambientNode->GetAttributeValue("TRIG_SECTOR");
                    size_t loopStart = ambientNode->GetAttributeValueAsInt("LOOPSTART");
                    size_t loopEnd   = ambientNode->GetAttributeValueAsInt("LOOPEND");

                    int trigger = TriggerStringToInt(trigS);
                    psSectorSoundManager* trigSector = manager;
                    if(sec.Length() > 0)
                        trigSector = GetOrCreateSector(sec);


                    csRef<iSndSysData> snddata = sndmngr->GetSoundResource( resource );

                    if (!snddata.IsValid())
                    {
                      Error2("Failed to load Sound Handle: %s", resource.GetData());
                      continue;
                    }

                    csRef<iSndSysStream> sndstream = sndmngr->soundSystem->CreateStream(snddata, CS_SND3D_DISABLE);
                    if (!sndstream.IsValid())
                    {
                      Error2("Failed to load Sound Handle: %s", resource.GetData());
                      continue;
                    }

                    psSoundObject* obj = new psSoundObject (sndstream,
                                                            this,
                                                            maxVol, minVol,
                                                            fadeDelay,
                                                            timeOfDay,
                                                            timeOfDayRange,
                                                            weather,
                                                            loopStart,
                                                            loopEnd,
                                                            true,
                                                            trigSector,
                                                            trigger);
                    obj->SetResource( resource );

                    manager->NewAmbient( obj );
                }

                csRef<iDocumentNodeIterator> emitters = sector->GetNodes("EMITTER");
                while ( emitters->HasNext() )
                {
                    csRef<iDocumentNode> emitter = emitters->Next();
                    csString resource = emitter->GetAttributeValue("RESOURCE");
                    float minVol =  emitter->GetAttributeValueAsFloat("MINVOL");
                    float maxVol =  emitter->GetAttributeValueAsFloat("MAXVOL");
                    float maxRange = emitter->GetAttributeValueAsFloat("MAX_RANGE");
                    float minRange = emitter->GetAttributeValueAsFloat("MIN_RANGE");

                    csRef<SOUND_DATA_TYPE> snddata = sndmngr->GetSoundResource( resource );
                    if (!snddata)
                    {
                      Error2("Failed to load Sound: %s", resource.GetData());
                      continue;
                    }

                    csRef<SOUND_STREAM_TYPE> sndstream = sndmngr->soundSystem->CreateStream(snddata, CS_SND3D_ABSOLUTE);
                    if (!sndstream)
                    {
                      Error2("Failed to create Sound Stream: %s", resource.GetData());
                      continue;
                    }

                    psSoundObject* obj = new psSoundObject (sndstream, this,
                                                            maxVol, minVol,
                                                            true
                                                            );
                    obj->SetResource( resource );
                    obj->SetRange( maxRange, minRange );

                    csRef<iDocumentAttribute> mesh = emitter->GetAttribute("MESH");
                    csRef<iDocumentAttribute> factory = emitter->GetAttribute("FACTORY");
                    if ( mesh )
                    {
                        obj->AttachToMesh( mesh->GetValue() );
                        manager->New3DMeshSound( obj );
                    }
                    else if ( factory )
                    {
                        float prob = emitter->GetAttributeValueAsFloat("FACTORY_PROBABILITY");
                        manager->New3DFactorySound( obj, factory->GetValue(), prob );
                    }
                    else
                    {
                        csVector3 pos( emitter->GetAttributeValueAsFloat("X"),
                                       emitter->GetAttributeValueAsFloat("Y"),
                                       emitter->GetAttributeValueAsFloat("Z") );
                        obj->SetPosition( pos );
                        manager->New3DSound( obj );
                    }
                }

                csRef<iDocumentNodeIterator> backgroundIter = sector->GetNodes("BACKGROUND");
                while ( backgroundIter->HasNext() )
                {
                    csRef<iDocumentNode> background = backgroundIter->Next();
                    csString resource = background->GetAttributeValue("RESOURCE");
                    float minVol =  background->GetAttributeValueAsFloat("MINVOL");
                    float maxVol =  background->GetAttributeValueAsFloat("MAXVOL");
                    int fadeDelay = background->GetAttributeValueAsInt("FADEDELAY");
                    int timeOfDay = background->GetAttributeValueAsInt("TIME");
                    int timeOfDayRange = background->GetAttributeValueAsInt("TIME_RANGE");
                    int weather   = background->GetAttributeValueAsInt("WEATHER");
                    size_t loopStart = background->GetAttributeValueAsInt("LOOPSTART");
                    size_t loopEnd   = background->GetAttributeValueAsInt("LOOPEND");

                    csRef<SOUND_DATA_TYPE> snddata = sndmngr->GetSoundResource( resource );
                    if (!snddata)
                    {
                      Error2("Failed to load Sound: %s", resource.GetData());
                      continue;
                    }


                    csRef<SOUND_STREAM_TYPE> sndstream = sndmngr->soundSystem->CreateStream(snddata, CS_SND3D_DISABLE);
                    if ( !sndstream )
                    {
                      Error2("Failed to create Sound Stream: %s", resource.GetData());
                      continue;
                    }

                    psSoundObject* obj = new psSoundObject (sndstream, this,
                                                            maxVol, minVol,
                                                            fadeDelay,
                                                            timeOfDay,
                                                            timeOfDayRange,
                                                            weather, loopStart,loopEnd,
                                                            sndmngr->LoopBGM());
                    obj->SetResource( resource );
                    manager->NewBackground( obj );
                }
            }
        }
    }

    // Remove pending ones
    pendingSectors.DeleteAll();

    return true;
}


void psMapSoundSystem::RegisterActiveSong(psSoundObject *song)
{
    if (csArrayItemNotFound == active_songs.Find(song))
    {
        song->SetVolume(sndmngr->GetMusicVolume());
        active_songs.Push(song);
        Debug2(LOG_SOUND, 0, "Registered active song: %s", song->GetName().GetData() );
    }
}

void psMapSoundSystem::SetMusicVolumes(float volume)
{
    size_t i,l;
    l=active_songs.GetSize();
    for (i=0;i<l;i++)
    {
        active_songs[i]->SetVolume(volume);
    }
}

void psMapSoundSystem::RemoveActiveSong(psSoundObject *song)
{
    active_songs.Delete(song);
    Debug2(LOG_SOUND, 0, "Removed active song: %s", song->GetName().GetData() );
}

psSoundObject *psMapSoundSystem::FindSameActiveSong(psSoundObject *other)
{
    size_t i,l;
    l=active_songs.GetSize();
    for (i=0;i<l;i++)
    {
        if (active_songs[i]->Same(other))
            return active_songs[i];
    }
    return NULL;
}


void psMapSoundSystem::RegisterActiveAmbient(psSoundObject *ambient)
{
    if (csArrayItemNotFound == active_ambient.Find(ambient))
    {
        ambient->SetVolume(sndmngr->GetAmbientVolume());
        active_ambient.Push(ambient);
        Debug2(LOG_SOUND, 0, "Registered active ambient: %s", ambient->GetName().GetData() );
    }
}

void psMapSoundSystem::RemoveActiveAmbient(psSoundObject *ambient)
{
    active_ambient.Delete(ambient);
    Debug2(LOG_SOUND, 0, "Removed active ambient: %s", ambient->GetName().GetData() );
}

void psMapSoundSystem::SetAmbientVolumes(float volume)
{
    size_t i,l;
    l=active_ambient.GetSize();
    for (i=0;i<l;i++)
    {
        active_ambient[i]->SetVolume(volume);
    }
}

psSoundObject *psMapSoundSystem::FindSameActiveAmbient(psSoundObject *other)
{
    size_t i,l;
    l=active_ambient.GetSize();
    for (i=0;i<l;i++)
    {
        if (active_ambient[i]->Same(other))
            return active_ambient[i];
    }
    return NULL;
}

void psMapSoundSystem::RegisterActiveEmitter(psSoundObject *emitter)
{
    if (csArrayItemNotFound == active_emitters.Find(emitter))
    {
        emitter->SetVolume(sndmngr->GetAmbientVolume());
        active_emitters.Push(emitter);
        Debug3(LOG_SOUND, 0, "Registered active emitter: %s (attached to %s)", emitter->GetName().GetData(), emitter->GetMesh().GetData() );
    }
}

void psMapSoundSystem::RemoveActiveEmitter(psSoundObject *emitter)
{
    active_emitters.Delete(emitter);
    Debug3(LOG_SOUND, 0, "Removed active emitter: %s (attached to %s)", emitter->GetName().GetData(), emitter->GetMesh().GetData() );
}

void psMapSoundSystem::SetEmitterVolumes(float volume)
{
    size_t i,l;
    l=active_emitters.GetSize();
    for (i=0;i<l;i++)
    {
        active_emitters[i]->SetVolume(volume);
    }
}

psSoundObject *psMapSoundSystem::FindSameActiveEmitter(psSoundObject *other)
{
    size_t i,l;
    l=active_emitters.GetSize();
    for (i=0;i<l;i++)
    {
        if (active_emitters[i]->Same(other))
            return active_emitters[i];
    }
    return NULL;
}

void psMapSoundSystem::RemoveActiveAnyAudio(psSoundObject *soundobject)
{
    RemoveActiveSong(soundobject);
    RemoveActiveAmbient(soundobject);
    RemoveActiveEmitter(soundobject);
}

void psMapSoundSystem::EnterSector(psSectorSoundManager *enterTo)
{
    // Check the destination sector to see which (if any) sounds need to be faded out
    size_t i,l;

    l=active_emitters.GetSize();
    for ( i = 0; i<l; i++ )
    {
        if (!active_emitters[i]->IsPlaying())
        {
          RemoveActiveEmitter(active_emitters[i]);
          l--; // Length is now 1 shorter
          i--; // The index we we processing is gone
        }
        else
        {
          Debug2( LOG_SOUND, 0, "Emitter %s from previous sector is still audible in next sector", active_emitters[i]->GetName().GetData() );
        }
    }

    l=active_songs.GetSize();
    for ( i = 0; i<l; i++ )
    {
        Debug2( LOG_SOUND, 0, "Checking whether music %s is playing in next sector", active_songs[i]->GetName().GetData() );
        if (!enterTo || enterTo->CheckSong( active_songs[i] ) )
            active_songs[i]->StartFade( FADE_DOWN );
    }


    l=active_ambient.GetSize();
    for ( i = 0; i<l; i++ )
    {
        Debug2( LOG_SOUND, 0, "Checking whether ambient sound %s is playing in next sector", active_ambient[i]->GetName().GetData() );
        if (!enterTo || enterTo->CheckAmbient( active_ambient[i] )|| (active_ambient[i]->Triggered() ))
        {
           /* if(active_ambient[i]->Triggered())
                continue;*/

            active_ambient[i]->StartFade( FADE_DOWN );
        }
    }
}

void psMapSoundSystem::Update( csVector3 & pos )
{
    for ( size_t z = 0; z < active_emitters.GetSize(); z++ )
    {
        active_emitters[z]->Update(pos);
    }
}


void psMapSoundSystem::Update()
{
    size_t z;
    for ( z = 0; z < active_songs.GetSize(); z++ )
    {
        //printf("Song Update: Vol: %f\n", songs[z]->Volume() );
        active_songs[z]->Update();
        if (!active_songs[z]->IsPlaying())
        {
            RemoveActiveSong(active_songs[z]);
            z--;
        }
    }

    for ( z = 0; z < active_ambient.GetSize(); z++ )
    {
        active_ambient[z]->Update();
        if (!active_ambient[z]->IsPlaying())
        {
            RemoveActiveAmbient(active_ambient[z]);
            z--;
        }
    }
}

psSectorSoundManager::psSectorSoundManager( csString& sectorName, iEngine* engine, psMapSoundSystem *mapSS )
{
    sector = sectorName;
    sounds = true;
    music = true;
    this->engine = engine;
    mainBG = NULL;
    mapsoundsystem = mapSS;
    weather = 1;
}

psSectorSoundManager::~psSectorSoundManager()
{
    size_t i,l;

    // All sounds must be removed from the psSoundManagers active list
    l = songs.GetSize();
    for (i=0;i<l;i++)
    {
      mapsoundsystem->RemoveActiveSong(songs[i]);
      songs[i]->Stop();
    }

    l = ambient.GetSize();
    for (i=0;i<l;i++)
    {
      mapsoundsystem->RemoveActiveAmbient(ambient[i]);
      ambient[i]->Stop();
    }

    l = emitters.GetSize();
    for (i=0;i<l;i++)
    {
      mapsoundsystem->RemoveActiveEmitter(emitters[i]);
      emitters[i]->Stop();
    }
}

void psSectorSoundManager::NewBackground( psSoundObject* song )
{
    songs.Push( song );
}

void psSectorSoundManager::NewAmbient( psSoundObject* sound )
{
    ambient.Push( sound );
}

void psSectorSoundManager::New3DSound( psSoundObject* sound )
{
    emitters.Push( sound );
}

void psSectorSoundManager::New3DMeshSound( psSoundObject* sound )
{
    unAssignedEmitters.Push( sound );
}

void psSectorSoundManager::New3DFactorySound( psSoundObject* sound, const char* factname, float probability )
{
    unAssignedEmitterFactories.Push( new ps3DFactorySound(sound,factname,probability) );
}

void psSectorSoundManager::Init3DFactorySounds()
{
    if (unAssignedEmitterFactories.IsEmpty())
        return;

    for (size_t z=0; z<unAssignedEmitterFactories.GetSize(); z++)
    {
        ps3DFactorySound* sndfact = unAssignedEmitterFactories[z];

        //printf("New 3D Factory sound:  %p %s %f\n", sndfact->sound, sndfact->meshfactname.GetData(), sndfact->probability );

        iSector* searchSector = engine->FindSector( sector, NULL );
        if (!searchSector)
        {
            Error3("Sector %s not found for adding sounds for factory %s\n", sector.GetData(), sndfact->meshfactname.GetData() );
            return;
        }

        iMeshFactoryWrapper* factory = engine->GetMeshFactories()->FindByName(sndfact->meshfactname);
        if (!factory)
        {
            Error2("Could not find factory name %s", sndfact->meshfactname.GetData() );
            return;
        }

        csRef<SOUND_DATA_TYPE> snddata = mapsoundsystem->sndmngr->GetSoundResource(sndfact->sound->GetName());
        if (!snddata)
        {
            Error2("Failed to load Sound: %s", sndfact->sound->GetName().GetData() );
            continue;
        }

        // Init to exact same random pattern every time, on every system
        csRandomGen* rng = new csRandomGen(0);

        iMeshList* meshes = searchSector->GetMeshes();
        for (int i=0; i < meshes->GetCount(); i++)
        {
            iMeshWrapper* mesh = meshes->Get(i);

            if (mesh->GetFactory() == factory)
            {
                if (rng->Get() <= sndfact->probability)
                {
                    csRef<SOUND_STREAM_TYPE> sndstream = mapsoundsystem->sndmngr->soundSystem->CreateStream(snddata,CS_SND3D_ABSOLUTE);
                    if (!sndstream)
                    {
                        Error2("Failed to create Sound Stream: %s", sndfact->sound->GetName().GetData() );
                        continue;
                    }

                    psSoundObject* obj = new psSoundObject(sndfact->sound,sndstream);
                    obj->AttachToMesh(mesh->QueryObject()->GetName());
                    csVector3 pos = mesh->GetMovable()->GetPosition();
                    obj->SetPosition(pos);
                    emitters.Push(obj);

                    //printf("New sound emitter at: %f, %f, %f\n",pos.x,pos.y,pos.z);
                }
            }
        }

        delete rng;
        delete sndfact;
    }

    unAssignedEmitterFactories.Empty();
}

void psSectorSoundManager::StartBackground()
{
    music = true;

    if ( mainBG )
    {
        mapsoundsystem->RegisterActiveSong(mainBG);
        if(mapsoundsystem->sndmngr->LoopBGM())
            mainBG->SetLooping(true);
        else
            mainBG->SetLooping(false);
        mainBG->StartFade( FADE_UP );
    }
    else
    {
        if ( songs.GetSize() > 0 )
        {
            mapsoundsystem->RegisterActiveSong(songs[0]);
            if(mapsoundsystem->sndmngr->LoopBGM())
                songs[0]->SetLooping(true);
            else
                songs[0]->SetLooping(false);
            songs[0]->StartFade( FADE_UP );
            mainBG = songs[0];
        }
    }
}



void psSectorSoundManager::StopBackground()
{
    music = false;
    for ( size_t z = 0; z < songs.GetSize(); z++ )
    {
        mapsoundsystem->RemoveActiveSong(songs[z]);
        songs[z]->Stop();
    }
}

void psSectorSoundManager::StartSounds(int weather)
{
    sounds = true;
    size_t z;

    for ( z = 0; z < ambient.GetSize(); z++ )
    {
        //TODO: matching time as well
        if (!ambient[z]->Triggered() || ambient[z]->MatchWeather(weather))
        {
            mapsoundsystem->RegisterActiveAmbient(ambient[z]);
            ambient[z]->StartFade( FADE_UP );
        }
    }

    for ( z = 0; z < emitters.GetSize(); z++ )
    {
        mapsoundsystem->RegisterActiveEmitter(emitters[z]);
        emitters[z]->StartSound();
    }
}

void psSectorSoundManager::StopSounds()
{
    sounds = false;
    size_t z;
    for ( z = 0; z < ambient.GetSize(); z++ )
    {
        mapsoundsystem->RemoveActiveAmbient(ambient[z]);
        ambient[z]->Stop();
    }

    for ( z = 0; z < emitters.GetSize(); z++ )
    {
        mapsoundsystem->RemoveActiveEmitter(emitters[z]);
        emitters[z]->Stop();
    }
}

void psSectorSoundManager::Fade( Fade_Direction dir )
{
    if ( mainBG && HasMusic())//We should check that the sector can play music.
    {
        if (dir == FADE_UP)
        {
            mapsoundsystem->RegisterActiveSong(mainBG);
        }
        mainBG->StartFade( dir );
    }

    size_t z;
    if (HasSounds())//Check if the sector can play sounds
    {
        for ( z = 0; z < ambient.GetSize(); z++ )
        {
            if (dir == FADE_UP)
                mapsoundsystem->RegisterActiveSong(ambient[z]);
            if(ambient[z]->MatchWeather(weather))
                ambient[z]->StartFade( dir );
        }

         for ( z = 0; z < emitters.GetSize(); z++ )
        {
            if (dir == FADE_UP)
                mapsoundsystem->RegisterActiveEmitter(emitters[z]);
            emitters[z]->StartFade( dir );
        }
    }
}

void psSectorSoundManager::ChangeTime( int timeOfDay )
{
    csArray<psSoundObject*> bestTimeSong;
    if ( music )
    {
        for (size_t z = 0; z < songs.GetSize(); z++ )
        {
            if ( songs[z]->MatchTime( timeOfDay ) )
                bestTimeSong.Push(songs[z]);
        }
    }

    if ( bestTimeSong.GetSize() )
    {
        SetBGSong(bestTimeSong[mapsoundsystem->GetRandomNumber(bestTimeSong.GetSize())]);
    }

}

void psSectorSoundManager::SetBGSong(psSoundObject* song)
{
    Debug2( LOG_SOUND, 0, "Song now playing is: %s", song->GetName().GetData() );
    if (!mapsoundsystem->FindSameActiveSong( song ))
    {
        mapsoundsystem->RegisterActiveSong(song);
        if(mapsoundsystem->sndmngr->LoopBGM())
            song->SetLooping(true);
        else
            song->SetLooping(false);

        song->StartFade( FADE_UP );
        mainBG = song;
    }
}

void psSectorSoundManager::Enter( psSectorSoundManager* leaveFrom, int timeOfDay, int weather, csVector3& position )
{
    csArray<psSoundObject*> bestTimeSong;
    csArray<psSoundObject*> bestWeatherSong;
    csArray<psSoundObject*> noReferenceSong;
    this->weather = weather;

    size_t z;

    if ( music )
    {
        for ( z = 0; z < songs.GetSize(); z++ )
        {
            //search for time restrained songs
            if ( songs[z]->MatchTime( timeOfDay ) )
            {
                bestTimeSong.Push(songs[z]);
            }
            //search for weather restrained songs
            if ( songs[z]->MatchWeather( weather ) )
            {
                bestWeatherSong.Push(songs[z]);
            }
            //search for no restrain songs
            if ( songs[z]->HasNoTime() && songs[z]->HasNoWeather())
            {
                noReferenceSong.Push(songs[z]);
            }
        }

        if ( bestTimeSong.GetSize() )
        {
            SetBGSong(bestTimeSong[mapsoundsystem->GetRandomNumber(bestTimeSong.GetSize())]);
        }
        else if ( bestWeatherSong.GetSize() )
        {
            SetBGSong(bestWeatherSong[mapsoundsystem->GetRandomNumber(bestWeatherSong.GetSize())]);
        }
        else if ( noReferenceSong.GetSize() )
        {
            SetBGSong(noReferenceSong[mapsoundsystem->GetRandomNumber(noReferenceSong.GetSize())]);
        }
        else //all failed get a random song
        {            
            if ( songs.GetSize() > 0 ) //do we actually have a song?
            {
                SetBGSong(songs[mapsoundsystem->GetRandomNumber(songs.GetSize())]);
            }
        }
    }

    if ( sounds )
    {
        for ( z = 0; z < ambient.GetSize(); z++ )
        {
            if (!mapsoundsystem->FindSameActiveAmbient( ambient[z] ) && !ambient[z]->Triggered()) // Only start non-trigger sounds
            {
                mapsoundsystem->RegisterActiveAmbient(ambient[z]);
                ambient[z]->StartFade( FADE_UP );
            }
        }

        if ( unAssignedEmitters.GetSize() > 0 )
        {
            for ( z = 0; z < unAssignedEmitters.GetSize(); z++ )
            {
                iSector* searchSector = engine->FindSector( sector, NULL );
                if ( searchSector )
                {
                    psSoundObject* pso = unAssignedEmitters[z];
                    if(pso->Triggered()) // Only start non-trigger sounds
                        continue;

                    csRef<iMeshWrapper> mesh = searchSector->GetMeshes()->FindByName( pso->GetMesh() );
                    //csRef<iMovable> move = mesh->GetMovable();
                    csVector3 pos = mesh->GetRadius().GetCenter();
                    pso->SetPosition( pos );
                    emitters.Push( pso );
                }
            }
            unAssignedEmitters.Empty();
        }

        Init3DFactorySounds();

        for ( z = 0; z < emitters.GetSize(); z++ )
        {
            if(emitters[z]->Triggered()) // Only start non-trigger sounds
                continue;

            mapsoundsystem->RegisterActiveEmitter(emitters[z]);
            emitters[z]->Start3DSound( position );
        }

        // Update weather
        UpdateWeather(weather);
    }
}

void psSectorSoundManager::UpdateWeather(int weather)
{
    this->weather =weather;

    for(size_t i = 0; i < weatherNotify.GetSize();i++)
    {
        psSoundObject* obj = weatherNotify[i];
        obj->Notify(SOUND_EVENT_WEATHER);
    }
}

void psSectorSoundManager::ConnectObject(psSoundObject* obj,int to)
{
    switch(to)
    {
        case SOUND_EVENT_WEATHER:
        {
            weatherNotify.Push(obj);
            break;
        }
    }
}

void psSectorSoundManager::DisconnectObject(psSoundObject* obj,int from)
{
    csArray<psSoundObject*>* array = NULL;

    switch(from)
    {
        case SOUND_EVENT_WEATHER:
        {
            array = &weatherNotify;
            break;
        }
    }

    if(!array)
        return; // Not implemented event (hm?)

    // Remove the event
    array->Delete(obj);
}

bool psSectorSoundManager::CheckSong( psSoundObject* bgSound )
{
    if (!bgSound)
        return true;

    for ( size_t z = 0; z < songs.GetSize(); z++ )
    {
        //if ( songs[z]->IsPlaying() && songs[z]->Same( bgSound ) )
        if ( songs[z]->Same( bgSound ) )
            return false;
    }
    return true;
}

bool psSectorSoundManager::CheckAmbient( psSoundObject* ambientSnd )
{
    if ( !ambientSnd )
        return true;

    for ( size_t z = 0; z < ambient.GetSize(); z++ )
        if ( ambient[z]->Same( ambientSnd ) )
            return false;

    return true;
}

psSoundObject::psSoundObject(csRef<iSndSysStream> strm,
                             psMapSoundSystem* mapSys,
                             float maxVol, float minVol,
                             int fadeTime, int timeOfDay, int timeOfDayRange, int weather, size_t loopStart,
                             size_t loopEnd, bool looping,
                             psSectorSoundManager* sector,int connectWith
                             ) : stream(strm)
{
    this->maxVol = maxVol;
    this->minVol = minVol;
    isPlaying = false;
    fadeComplete = false;
    fadeDelay = fadeTime;
    this->timeOfDay = timeOfDay;
    this->timeOfDayRange = timeOfDayRange;
    this->weatherCondition = weather;
    this->loopStart = loopStart;
    this->loopEnd = loopEnd;
    currentVolume = 0.0f;
    ambientVolume = 1.0f;
    connectedSector = sector;
    mapSystem = mapSys;
    connectedWith = connectWith;
    loop = looping;

    if (strm->Get3dMode() == CS_SND3D_DISABLE)
      threeDee = false;
    else
      threeDee = true;

    if (sector)
        sector->ConnectObject(this, connectWith);
}

psSoundObject::psSoundObject(psSoundObject* other, csRef<iSndSysStream> strm) : stream(strm)
{
    isPlaying = false;
    fadeComplete = false;
    currentVolume = 0.0f;
    ambientVolume = 1.0f;

    this->maxVol           = other->maxVol;
    this->minVol           = other->minVol;
    this->fadeDelay        = other->fadeDelay;
    this->timeOfDay        = other->timeOfDay;
    this->timeOfDayRange   = other->timeOfDayRange;
    this->weatherCondition = other->weatherCondition;
    this->connectedSector  = other->connectedSector;
    this->mapSystem        = other->mapSystem;
    this->connectedWith    = other->connectedWith;
    this->threeDee         = other->threeDee;
    this->loop             = other->loop;

    this->resourceName = other->resourceName;

    this->rangeToStart = other->rangeToStart;
    this->minRange = other->minRange;
    this->rangeConstant = other->rangeConstant;

    if (connectedSector)
        connectedSector->ConnectObject(this, connectedWith);
}

psSoundObject::~psSoundObject()
{
    // Halt the sound in the sound system
    if (mapSystem && mapSystem->sndmngr && mapSystem->sndmngr->soundSystem)
    {
        stream.Stop(mapSystem->sndmngr->soundSystem);
    }

    // Disconnect
    if (connectedSector)
        connectedSector->DisconnectObject(this,connectedWith);
}

void psSoundObject::SetVolume(float vol)
{
    ambientVolume = vol;
    if (isPlaying)
        stream.SetVolume( currentVolume * ambientVolume );
}

void psSoundObject::StartSound()
{
    if ( !stream.Start(mapSystem->sndmngr->soundSystem,loop,loopStart,loopEnd) )
        return;

    if (threeDee)
    {
        soundSource3D = stream.GetSource3D();
        if (!soundSource3D)
            threeDee=false;
        else
        {
            soundSource3D->SetMinimumDistance(minRange);
            soundSource3D->SetMaximumDistance(rangeToStart);
            soundSource3D->SetPosition( position );
        }
    }
}

void psSoundObject::Start3DSound( csVector3& playerPos )
{
    StartSound();
    Update( playerPos );
}

bool psSoundObject::Triggered()
{
    if(connectedWith != 0)
        return true;

    return false;
}

void psSoundObject::Notify(int event)
{
    if(event != connectedWith)
        return;

    // Handle event
    switch(event)
    {
        case SOUND_EVENT_WEATHER:
        {
            UpdateWeather(connectedSector->GetWeather());
            break;
        }
    }
}

void psSoundObject::UpdateWeather(int weather)
{
    if(!Triggered())
        return;

    // Check old weather objects and mute if there are any
    if(mapSystem->FindSameActiveAmbient(this) && !MatchWeather(weather))
    {
        mapSystem->RemoveActiveAmbient(this);
        Stop();
        return;
    }

    // Same weather and is not playing?
    if(MatchWeather(weather) && !mapSystem->FindSameActiveAmbient(this))
    {
        if( mapSystem->sndmngr->PlayingAmbient() )
        {
            mapSystem->RegisterActiveAmbient(this); // We're playing this now
            StartFade(FADE_UP); // PLAY!
        }
        return;
    }
}

void psSoundObject::Update( csVector3& playerPos )
{
    csVector3 rangeVec = position - playerPos;
    float range = rangeVec.Norm();

    //printf("RANGE: %f\n", range );

    if ( isPlaying && range > rangeToStart )
    {
        stream.SetVolume(0.0f);
        isPlaying = false;
    }
    else if ( !isPlaying && range < rangeToStart )
    {
        isPlaying = true;

        // Emitter volume is based on 3d positional audio calculations handled inside the sound system
        currentVolume = maxVol;

        //printf("Emitter Sound Vol: %f\n", currentVolume );
        stream.SetVolume( currentVolume * ambientVolume );
    }
}

void psSoundObject::StartFade( Fade_Direction dir )
{
    Debug3( LOG_SOUND, 0, "Fading Song %s Direction %d", GetName().GetData(), dir );

    if ( dir == FADE_DOWN && currentVolume == minVol )
    {
        fadeComplete = true;
        Debug1( LOG_SOUND, 0, "Fading complete" );
        if (isPlaying)
        {
            isPlaying = false;
            stream.SetVolume(0.0f);
        }
        return;
    }
    else if ( dir == FADE_UP && currentVolume == maxVol )
    {
        Debug1( LOG_SOUND, 0, "Fading complete" );
        fadeComplete = true;
        if(!isPlaying)
        {
            isPlaying = true;
            stream.Start(mapSystem->sndmngr->soundSystem,loop,loopStart,loopEnd);
            stream.SetVolume(currentVolume);
        }
        return;
    }

    fadeDir = dir;
    startTime = csGetTicks();
    fadeComplete = false;


    if (!stream.IsValid())
    {
      currentVolume = minVol;
      StartSound();
      stream.SetVolume( minVol * ambientVolume );
      Debug2( LOG_SOUND,0, "Sound source for %s is NULL.  Setting currentVolume to minVol.",GetName().GetData());
    }


    if ( stream.IsValid() )
    {
      if (!isPlaying)
      {
        isPlaying = true;
        Debug2( LOG_SOUND, 0, "Sound Playing at: %f vol", currentVolume );
        stream.SetVolume( currentVolume*ambientVolume );
      }
      else
      {
        /*This is necessary in order to get rain fading out in the new sector, otherwise
        it doesn't*/
        Update();

      }
    }
    return;
}

void psSoundObject::SetRange( float maxRange, float minRange )
{
    rangeToStart = maxRange;
    this->minRange = minRange;
    rangeConstant = ( (maxVol-minVol)/(rangeToStart-minRange));
}

bool psSoundObject::MatchTime( int time )
{

    if(timeOfDay == -1) return false;

    int timeEnd = timeOfDay + timeOfDayRange;

    if (  time >= timeOfDay && time < timeEnd )
        return true;
    else
        return false;
}


void psSoundObject::Update()
{
    if (fadeComplete)
        return;

    csTicks currentTime = csGetTicks();
    float diff = currentTime - startTime;
    if ( diff > fadeDelay )
    {
        if  ( fadeDir == FADE_UP )
            currentVolume = maxVol;
        if ( fadeDir == FADE_DOWN )
        {
            Debug2( LOG_SOUND,0, "diff > fadeDelay for %s.  Setting currentVolume to minVol.",GetName().GetData());
            currentVolume = minVol;
        }
        fadeComplete = true;
    }
    else
    {
        float percent = (float)diff / (float)fadeDelay;
        if ( fadeDir == FADE_UP )
        {
            currentVolume = minVol + (maxVol-minVol)*percent;
            if ( currentVolume > maxVol )
            {
                currentVolume = maxVol;
                fadeComplete = true;
            }
        }
        else
        {
            currentVolume = maxVol - (maxVol-minVol)*percent;
            if ( currentVolume <= minVol )
            {
                fadeComplete = true;
                currentVolume = minVol;
                Debug2( LOG_SOUND,0, "currentVolume <= minVol for %s.  Setting currentVolume to minVol.",GetName().GetData());
            }
        }
    }


    if ( currentVolume <= minVol && fadeDir == FADE_DOWN )
    {
        isPlaying = false;
        stream.SetVolume(0.0f);
    }
    else
    {
        stream.SetVolume(currentVolume * ambientVolume);
    }
}


bool psSndStreamHandle::Start(iSndSysRenderer* renderer, bool loop, size_t loopStart, size_t loopEnd)
{
    if (!soundSource)
        soundSource = renderer->CreateSource(soundStream);

    // Failed to create source
    if (!soundSource)
        return false;

    soundStream->SetLoopState(loop);
    //soundStream->SetLoopBonduaries(loopStart, loopEnd);
    SetVolume(0.0f);
    return true;
}

void psSndStreamHandle::Stop(iSndSysRenderer* renderer)
{
    renderer->RemoveSource(soundSource);
    renderer->RemoveStream(soundStream);
}

void psSndStreamHandle::SetVolume(float volume)
{
    if ( !IsValid() )
        return;

    soundSource->SetVolume(volume);

    if (volume < MIN_VOLUME)
        soundStream->Pause();
    else
        soundStream->Unpause();
}

csRef<SOUND_SOURCE3D_TYPE> psSndStreamHandle::GetSource3D()
{
    return scfQueryInterface<SOUND_SOURCE3D_TYPE> (soundSource);
}
