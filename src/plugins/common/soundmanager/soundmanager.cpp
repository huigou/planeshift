/*
* soundmanager.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
*
* Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>

//====================================================================================
// Project Includes
//====================================================================================
#include "util/psxmlparser.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "soundmanager.h"
#include "instrumentmngr.h"


SCF_IMPLEMENT_FACTORY(SoundManager)

uint SoundManager::updateTime = DEFAULT_SECTOR_UPDATE_TIME;

SoundManager::SoundManager(iBase* parent): scfImplementationType(this, parent)
{
    // initializing pointers to null
    mainSndCtrl = 0;
    ambientSndCtrl = 0;
    musicSndCtrl = 0;

    activeSector = 0;
    commonSector = 0;

    sndSysMgr = 0;
    instrMgr = 0;

    combat = PEACE;
    weather = 1; // 1 is sunshine
    isSectorLoaded = false;
}


SoundManager::~SoundManager()
{
    if(isSectorLoaded)
    {
        UnloadSectors();
    }

    delete instrMgr;
    delete sndSysMgr;
    // Note: sndSysMgr should take care of SoundControls .. we can ignore them

    // Deleting SoundQueues
    csHash<SoundQueue*, int>::GlobalIterator queueIter(soundQueues.GetIterator());
    SoundQueue* sq;

    while(queueIter.HasNext())
    {
        sq = queueIter.Next();
        delete sq;
    }

    soundQueues.DeleteAll();
}

//-----------------
// FROM iComponent 
//-----------------

bool SoundManager::Initialize(iObjectRegistry* objReg)
{
    objectReg = objReg;
    
    loopBGM.SetCallback(this, &UpdateMusicCallback);
    combatMusic.SetCallback(this, &UpdateMusicCallback);

    lastUpdateTime = csGetTicks();

    // Registering for event callbacks
    csRef<iEventQueue> queue = csQueryRegistry<iEventQueue>(objectReg);
    evSystemOpen = csevSystemOpen(objectReg);
    if (queue != 0)
    {
        queue->RegisterListener(this, evSystemOpen);
    }
    else
    {
        // If this cannot be registered for the event it's better to
        // initialize sndSysMgr here to prevent using a null pointer
        Init();
    }

    //TODO: NEED TO INITIALIZE ALL VARIABLES!
    return true;
}

//--------------------
// FROM iEventhandler 
//--------------------

/*
 * To get the listener (and so to initialize SoundSystemManager and
 * SoundSystem) iSndSysRenderer must receive the evSystemOpen event,
 * then we can initialize everything.
 */
bool SoundManager::HandleEvent(iEvent &e)
{
    if (e.Name == evSystemOpen) 
    {
        Init();
    }

    return false;
}

const csHandlerID* SoundManager::GenericPrec(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &/*enr*/, csEventID id) const
{
    if(id == evSystemOpen)
    {
        static csHandlerID precConstraint[2]; // TODO not thread safe

        precConstraint[0] = ehr->GetGenericID("crystalspace.sndsys.renderer");
        precConstraint[1] = CS_HANDLERLIST_END;
        return precConstraint;
    }

    return 0;
}

//--------------------
// FROM iSoundManager 
//--------------------

bool SoundManager::InitializeSectors()
{
    LoadSectors();

    return isSectorLoaded;
}

/*
* LoadActiveSector is called when when the player enters a sector (modehandler.cpp)
*
* It loads the new sector and unloads the old one
* FACTORY Emitters are converted to real EMITTERS (if not already done)
* \ this is only done ONCE per sector
*
* it also moves background and ambient music into the new sector
* if they are not changing and are already playing
*
* Old sector will be cleaned at the end of this function
* unused music and sounds will be unloaded
*
* NOTE: has nothing todo with updates on OpenAL .. those are done by CS
*
*/
void SoundManager::LoadActiveSector(const char* sector)
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    psSoundSector* oldSector;
    psSoundSector* newSector;

    /* TODO wrong way todo this */
    if(sndSysMgr->Initialised == false)
    {
        return;
    }

    /*
    * i dont want this function called if were doing nasty stuff like that
    * FIXME
    */

    if(strcmp(sector, "SectorWhereWeKeepEntitiesResidingInUnloadedMaps") == 0)
    {
        return;
    }

    /*
    * set the wanted sector to our active sector (if found)
    */

    if(FindSector(sector, newSector) == true)
    {
        /* FIXME - hack for #4268 */
        if(activeSector != NULL)
        {
            if(csStrCaseCmp(activeSector->name, sector) == 0)
            {
                return;
            }
        }

        oldSector = activeSector;
        activeSector = newSector;
        activeSector->active = true;

        /* works only on loaded sectors! */
        ConvertFactoriesToEmitter(activeSector);

        if(oldSector != NULL)
        {
            /*
            * hijack active ambient and music if they are played in the new sector
            * in the old sector there MUST be only ONE of each
            */

            TransferHandles(oldSector, activeSector);

            /* set old sector to inactive and make a update (will stop all sounds) */
            oldSector->active = false;
            UpdateSector(oldSector);

        }

        /*
        * Call Update start sound in the new sector
        */

        UpdateSector(activeSector);
        return;
    }
}


void SoundManager::ReloadSectors()
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    csString currentSector = csString(activeSector->name);
    UnloadActiveSector();
    ReloadAllSectors();
    LoadActiveSector(currentSector);
}


void SoundManager::UnloadActiveSector()
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    activeSector->active = false;
    UpdateSector(activeSector);
    activeSector = NULL;
}

iSoundControl* SoundManager::AddSndCtrl(int ctrlID, int type)
{
    SoundControl* sc = sndSysMgr->AddSoundControl(ctrlID, type);

    if(sc == 0)
    {
        return 0;
    }

    // Handle Ambient and Music SoundControls.
    switch(type)
    {
    case iSoundControl::AMBIENT:
        sc->SetCallback(this, &UpdateAmbientCallback);
        if(ambientSndCtrl != sndSysMgr->defaultSndCtrl)
        {
            ambientSndCtrl->SetType(iSoundControl::NORMAL);
            ambientSndCtrl->RemoveCallback();
        }
        ambientSndCtrl = sc;
        break;

    case iSoundControl::MUSIC:
        sc->SetCallback(this, &UpdateMusicCallback);
        if(musicSndCtrl != sndSysMgr->defaultSndCtrl)
        {
            musicSndCtrl->SetType(iSoundControl::NORMAL);
            musicSndCtrl->RemoveCallback();
        }
        musicSndCtrl = sc;
        break;
    }

    return sc;
}


void SoundManager::RemoveSndCtrl(iSoundControl* sndCtrl)
{
    if(sndCtrl == 0)
    {
        return;
    }

    // check if it is the ambientSndCtrl or the musicSndCtrl
    switch(sndCtrl->GetType())
    {
    case iSoundControl::AMBIENT:
        // use default sound control to avoid null pointers
        ambientSndCtrl = sndSysMgr->defaultSndCtrl;
        break;

    case iSoundControl::MUSIC:
        musicSndCtrl = sndSysMgr->defaultSndCtrl;
        break;
    }

    SoundControl* sndControl = static_cast<SoundControl*>(sndCtrl);
    sndSysMgr->RemoveSoundControl(sndControl);

}


iSoundControl* SoundManager::GetSndCtrl(int ctrlID)
{
    return sndSysMgr->GetSoundControl(ctrlID);
}


iSoundControl* SoundManager::GetMainSndCtrl()
{
    return mainSndCtrl;
}


bool SoundManager::AddSndQueue(int queueID, iSoundControl* sndCtrl)
{

    if(soundQueues.Get(queueID, 0) != 0 || sndCtrl == 0)
    {
        return false;
    }

    SoundControl* sndControl = static_cast<SoundControl*>(sndCtrl);
    SoundQueue* sq = new SoundQueue(sndControl, VOLUME_NORM);
    soundQueues.Put(queueID, sq);

    return true;
}


void SoundManager::RemoveSndQueue(int queueID)
{
    soundQueues.DeleteAll(queueID);
    return;
}


bool SoundManager::PushQueueItem(int queueID, const char* fileName)
{
    SoundQueue* sq = soundQueues.Get(queueID, 0);

    if(sq == 0)
    {
        return false;
    }

    sq->AddItem(fileName);
    return true;
}


void SoundManager::SetCombatStance(int newCombatStance)
{
    if(combatMusic.GetToggle() == true)
    {
        combat = newCombatStance;
    }
    else
    {
        combat = iSoundManager::PEACE;
    }

    if(activeSector != 0)
        activeSector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);
}

/*
* FIXME Remove GetCombatStance when you fix the victory effect
*/

int SoundManager::GetCombatStance() const
{
    return combat;
}


void SoundManager::SetPlayerMovement(csVector3 playerPos, csVector3 playerVelocity)
{
    if(activeSector != NULL)
    {
        activeSector->playerposition = playerPos;
    }

    sndSysMgr->SetPlayerVelocity(playerVelocity);
    sndSysMgr->SetPlayerPosition(playerPos);
}


csVector3 SoundManager::GetPosition() const
{
    return sndSysMgr->GetPlayerPosition();
}


/*
* Set functions which trigger updates
*/
void SoundManager::SetTimeOfDay(int newTimeOfDay)
{
    if(activeSector != NULL)
    {
        activeSector->timeofday = newTimeOfDay;

        activeSector->UpdateAmbient(weather, ambientSndCtrl);
        activeSector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);

    }
}

int SoundManager::GetTimeOfDay() const
{
    if(activeSector == 0)
    {
        return -1;
    }

    return activeSector->timeofday;
}


/*
* Engine calls SetWeather every frame or so
* update is only called if weather is changing
*/
void SoundManager::SetWeather(int newWeather)
{
    if(weather != newWeather && activeSector)
    {
        weather = newWeather;
        activeSector->UpdateAmbient(weather, ambientSndCtrl);
    }
}


int SoundManager::GetWeather() const
{
    return weather;
}

void SoundManager::SetEntityState(int state, iMeshWrapper* mesh, bool forceChange)
{
    if(activeSector != 0)
    {
        activeSector->SetEntityState(state, ambientSndCtrl, mesh, forceChange);
    }
}

void SoundManager::SetLoopBGMToggle(bool toggle)
{
    loopBGM.SetToggle(toggle);
}


bool SoundManager::IsLoopBGMToggleOn()
{
    return loopBGM.GetToggle();
}


void SoundManager::SetCombatMusicToggle(bool toggle)
{
    combatMusic.SetToggle(toggle);
}


bool SoundManager::IsCombatMusicToggleOn()
{
    return combatMusic.GetToggle();
}


void SoundManager::SetListenerOnCameraToggle(bool toggle)
{
    listenerOnCamera.SetToggle(toggle);
}


bool SoundManager::IsListenerOnCameraToggleOn()
{
    return listenerOnCamera.GetToggle();
}


void SoundManager::SetChatToggle(bool toggle)
{
    chatToggle.SetToggle(toggle);
}


bool SoundManager::IsChatToggleOn()
{
    return chatToggle.GetToggle();
}

bool SoundManager::IsSoundValid(uint soundID) const
{
    return sndSysMgr->IsHandleValid(soundID);
}

uint SoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl)
{
    SoundHandle* handle = 0;
    SoundControl* sndCtrl = static_cast<SoundControl*>(ctrl);

    if(sndSysMgr->Play2DSound(fileName, loop, 0, 0, VOLUME_NORM,
        sndCtrl, handle))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}


uint SoundManager::PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl, csVector3 pos, csVector3 dir, float minDist, float maxDist)
{
    SoundHandle* handle = 0;
    SoundControl* sndCtrl = static_cast<SoundControl*>(ctrl);

    if(sndSysMgr->Play3DSound(fileName, loop, 0, 0, VOLUME_NORM,
        sndCtrl, pos, dir, minDist, maxDist,
        0, CS_SND3D_ABSOLUTE, handle))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}

uint SoundManager::PlaySong(csRef<iDocument> musicalSheet, const char* instrument, float errorRate,
              iSoundControl* ctrl, csVector3 pos, csVector3 dir)
{
    SoundHandle* handle = 0;
    SoundControl* sndCtrl = static_cast<SoundControl*>(ctrl);

    if(instrMgr->PlaySong(sndCtrl, pos, dir, handle, musicalSheet, instrument, errorRate))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}


bool SoundManager::StopSound(uint soundID)
{
    if(soundID == 0)
    {
        return false;
    }

    return sndSysMgr->StopSound(soundID);
}


bool SoundManager::SetSoundSource(uint soundID, csVector3 position)
{
    if(soundID == 0)
    {
        return false;
    }

    return sndSysMgr->SetSoundSource(soundID, position);
}

/*
* Updates things that cant be triggered by
* a event (yet). E.g. Check if theres a Voice to play (TODO: callback)
* or if there are Emitters or Entities in or out of range that
* need a update
*
* Im not sure if those can ever be event based
* sure we know that theres something moving
* but ten to twenty times per second is enough
* not sure how often it would be called if
* executed by engine
* 
* FIXME csticks
*
*/
void SoundManager::Update()
{

    sndTime = csGetTicks();

    // twenty times per second
    if(lastUpdateTime + SoundManager::updateTime <= sndTime)
    {
        // Making queues work
        csHash<SoundQueue*, int>::GlobalIterator queueIter(soundQueues.GetIterator());
        SoundQueue* sq;

        while(queueIter.HasNext())
        {
            sq = queueIter.Next();
            sq->Work();
        }

        // Updating sectors if needed
        if(activeSector != 0)
        {
            activeSector->UpdateEmitter(ambientSndCtrl);
            activeSector->UpdateEntity(ambientSndCtrl, commonSector);
        }

        lastUpdateTime = csGetTicks();
    }

    // dont forget to Update our SoundSystemManager
    // remember that it has a own throttle
    sndSysMgr->Update();
}

void SoundManager::Init()
{
    const char* instrumentsPath;

    // configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        SoundManager::updateTime = configManager->GetInt("Planeshift.Sound.UpdateTime", DEFAULT_SECTOR_UPDATE_TIME);
        instrumentsPath = configManager->GetStr("Planeshift.Sound.Intruments", DEFAULT_INSTRUMENTS_PATH);
    }
    else
    {
        // updateTime is already initialized
        instrumentsPath = DEFAULT_INSTRUMENTS_PATH;
    }

    sndSysMgr = new SoundSystemManager(objectReg);
    instrMgr = new InstrumentManager(objectReg, instrumentsPath);

    // initializing ambient and music controller to something different
    // than null before AddSndCtrl is called
    ambientSndCtrl = sndSysMgr->defaultSndCtrl;
    musicSndCtrl = sndSysMgr->defaultSndCtrl;

    // Inizializing main SoundControls
    mainSndCtrl = sndSysMgr->mainSndCtrl;
    AddSndCtrl(iSoundManager::AMBIENT_SNDCTRL, iSoundControl::AMBIENT);
    AddSndCtrl(iSoundManager::MUSIC_SNDCTRL, iSoundControl::MUSIC);
    AddSndCtrl(iSoundManager::ACTION_SNDCTRL, iSoundControl::NORMAL);
    AddSndCtrl(iSoundManager::EFFECT_SNDCTRL, iSoundControl::NORMAL);
    AddSndCtrl(iSoundManager::GUI_SNDCTRL, iSoundControl::NORMAL);
    AddSndCtrl(iSoundManager::VOICE_SNDCTRL, iSoundControl::NORMAL);

    // Initializing voice queue
    AddSndQueue(iSoundManager::VOICE_QUEUE, GetSndCtrl(iSoundManager::VOICE_SNDCTRL));
}
void SoundManager::UpdateMusicCallback(void* object)
{
    SoundManager* which = (SoundManager*) object;
    if(which->activeSector != NULL)
    {
        which->activeSector->UpdateMusic(which->loopBGM.GetToggle(), which->combat, which->musicSndCtrl);
    }
}

void SoundManager::UpdateAmbientCallback(void* object)
{
    SoundManager* which = (SoundManager*) object;
    if(which->activeSector != NULL)
    {
        which->activeSector->UpdateAmbient(which->weather, which->ambientSndCtrl);
    }
}


/*
* Transfer ambient/music Handles from oldsector to newsector if needed
* if loopBGM is false then the handle might be invalid
* this is no problem because we dont touch it
* all we do is copy the address
*/

void SoundManager::TransferHandles(psSoundSector* &oldsector,
                                   psSoundSector* &newsector)
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    for(size_t j = 0; j< newsector->musicarray.GetSize(); j++)
    {
        if(oldsector->activemusic == NULL)
        {
            break;
        }

        if(csStrCaseCmp(newsector->musicarray[j]->resource,
           oldsector->activemusic->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newsector->musicarray[j]->handle = oldsector->activemusic->handle;
            /* set handle to NULL */
            oldsector->activemusic->handle = NULL;
            newsector->activemusic = newsector->musicarray[j];
            /* set the sound to active */
            newsector->musicarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activemusic->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activemusic = NULL;
            /* update callback */
            newsector->activemusic->UpdateHandleCallback();
        }
    }

    for(size_t j = 0; j< activeSector->ambientarray.GetSize(); j++)
    {
        if(oldsector->activeambient == NULL)
        {
            break;
        }

        if(csStrCaseCmp(newsector->ambientarray[j]->resource,
           oldsector->activeambient->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newsector->ambientarray[j]->handle = oldsector->activeambient->handle;
            /* set handle to NULL */
            oldsector->activeambient->handle = NULL;
            newsector->activeambient = newsector->ambientarray[j];
            /* set the sound to active */
            newsector->ambientarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldsector->activeambient->active = false;
            /* set it to NULL to avoid problems */
            oldsector->activeambient = NULL;
            /* update callback */
            newsector->activeambient->UpdateHandleCallback();
        }
    }
}




void SoundManager::ConvertFactoriesToEmitter(psSoundSector* &sector)
{
    iSector*             searchSector;
    iMeshFactoryWrapper* factory;
    iMeshWrapper*        mesh;
    iMeshList*           meshes;
    csRef<iEngine>       engine;
    psEmitter*           emitter;
    psEmitter*           factoryemitter;

    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    engine = csQueryRegistry<iEngine>(objectReg);
    if(!engine)
    {
        Error1("No iEngine plugin!");
        return;
    }

    /*
    * convert emitters with factory attributes to real emitters
    * positions are random but are only generated once
    */

    for(size_t j = 0; j< sector->emitterarray.GetSize(); j++)
    {
        if(!sector->emitterarray[j]->factory)
        {
            continue;
        }

        factoryemitter = sector->emitterarray[j];

        if(!(searchSector = engine->FindSector(sector->name, NULL)))
        {
            Error2("sector %s not found\n", (const char*) sector);
            continue;
        }

        if(!(factory = engine->GetMeshFactories()
           ->FindByName(factoryemitter->factory)))
        {
            Error2("Could not find factory name %s", (const char*) factoryemitter->factory);
            continue;
        }

        meshes = searchSector->GetMeshes();

        for(int k = 0; k < meshes->GetCount(); k++)
        {
            mesh = meshes->Get(k);

            if(mesh->GetFactory() == factory)
            {
                if(rng.Get() <= factoryemitter->factory_prob)
                {
                    emitter = new psEmitter;

                    emitter->resource = csString(factoryemitter->resource);
                    emitter->minvol   = factoryemitter->minvol;
                    emitter->maxvol   = factoryemitter->maxvol;
                    emitter->maxrange = factoryemitter->maxrange;
                    emitter->position = mesh->GetMovable()->GetPosition();
                    emitter->active   = false;

                    sector->emitterarray.Push(emitter);
                }
            }
        }
        /* delete the factory node */
        sector->DeleteEmitter(factoryemitter);
    }
}


bool SoundManager::FindSector(const char* name, psSoundSector* &sector)
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return false;
    }

    for(size_t i = 0; i< sectorData.GetSize(); i++)
    {
        if(strcmp(name, sectorData[i]->name) != 0)
        {
            continue;
        }
        else
        {
            sector = sectorData[i];
            return true;
        }
    }

    sector = NULL;
    return false;
}

/*
* Update everything in the given sector
*/

void SoundManager::UpdateSector(psSoundSector* &sector)
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    sector->UpdateMusic(loopBGM.GetToggle(), combat, musicSndCtrl);
    sector->UpdateAmbient(weather, ambientSndCtrl);
    sector->UpdateEmitter(ambientSndCtrl);
    sector->UpdateEntity(ambientSndCtrl, commonSector);
}


/*
* were always using camera height and rotation
* position can be players position or cameras position
* updated every frame? ..
*/

void SoundManager::UpdateListener(iView* view)
{
    csVector3 hearpoint;
    csMatrix3 matrix;
    csVector3 front;
    csVector3 top;

    /* TODO wrong way todo this */
    if(sndSysMgr->Initialised == false)
    {
        return;
    }

    if(listenerOnCamera.GetToggle() == false)
    {
        hearpoint = sndSysMgr->GetPlayerPosition();
    }
    else
    {
        // take position/direction from view->GetCamera()
        hearpoint = view->GetPerspectiveCamera()->GetCamera()
            ->GetTransform().GetOrigin();
    }

    matrix = view->GetPerspectiveCamera()->GetCamera()
        ->GetTransform().GetT2O();
    front = matrix.Col3();
    top   = matrix.Col2();

    sndSysMgr->UpdateListener(hearpoint, front, top);
}


/*
* Load a all sector xmls and make them usable. Will overwrite the target
* sector if it exists. It's only parsing the xml most of it is hardcoded.
*
* If a common sector it's not found, commonSector will be initialized as
* an empty psSoundSector.
*/

bool SoundManager::LoadSectors()
{
    const char* areasPath;
    const char* commonName;

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        areasPath = configManager->GetStr("Planeshift.Sound.AreasPath", DEFAULT_AREAS_PATH);
        commonName = configManager->GetStr("Planeshift.Sound.CommonSector", DEFAULT_COMMON_SECTOR_NAME);
    }
    else
    {
        areasPath = DEFAULT_AREAS_PATH;
        commonName = DEFAULT_COMMON_SECTOR_NAME;
    }

    // check if sectors are already initialized
    if(isSectorLoaded)
    {
        return true;
    }

    csRef<iDataBuffer>      xpath;
    const char*             dir;
    const char*             sectorName;
    csRef<iStringArray>     files;
    psSoundSector*          tmpsector;
    csRef<iVFS>             vfs;

    csRef<iDocumentNodeIterator> sectorIter;

    if(!(vfs = csQueryRegistry<iVFS>(objectReg)))
    {
        return false;
    }

    xpath = vfs->ExpandPath(areasPath);
    dir = **xpath;
    files = vfs->FindFiles(dir);

    if(!files)
    {
        return false;
    }

    for(size_t i=0; i < files->GetSize(); i++)
    {
        csString name(files->Get(i));
        csRef<iDocument> doc;
        csRef<iDocumentNode> root, mapNode;

        if((doc=ParseFile(objectReg, name))
           && (root=doc->GetRoot())
           && (mapNode=root->GetNode("MAP_SOUNDS")))
        {
            sectorIter = mapNode->GetNodes("SECTOR");

            while(sectorIter->HasNext())
            {
                csRef<iDocumentNode> sector = sectorIter->Next();

                sectorName = sector->GetAttributeValue("NAME");

                if(FindSector(sectorName, tmpsector) == true)
                {
                    tmpsector->Reload(sector);
                }
                else if(csStrCaseCmp(sectorName, commonName) == 0)
                {
                    commonSector = new psSoundSector(sector, objectReg);
                }
                else
                {
                    tmpsector = new psSoundSector(sector, objectReg);
                    sectorData.Push(tmpsector);
                }
            }
        }
    }

    // checking if a common sector could be found
    if(commonSector == 0)
    {
        commonSector = new psSoundSector(commonName, objectReg);
    }
    commonSector->active = true; // commonSector must always be active

    isSectorLoaded = true;
    return true;
}

void SoundManager::ReloadAllSectors()
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    UnloadSectors();
    LoadSectors();
}

/**
 * Delete both the sectors in sectorData and commonSector
 */
void SoundManager::UnloadSectors()
{
    // check if the sectors are initialized
    if(!isSectorLoaded)
    {
        return;
    }

    for(size_t i = 0; i < sectorData.GetSize(); i++)
    {
        delete sectorData[i];
    }

    sectorData.DeleteAll();
    delete commonSector;

    isSectorLoaded = false;
}
