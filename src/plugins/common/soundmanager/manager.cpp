/*
 * manager.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "manager.h"
#include "handle.h"
#include "soundctrl.h"
#include "system.h"
#include "data.h"

#include <util/log.h>


/*
 * Initialize the SoundSystem (SndSys) and the Datamanager (SndData)
 * Load our sound library
 *
 * Set Initialized to true if successfull / to false if not
 */

SoundSystemManager::SoundSystemManager(iObjectRegistry* objectReg)
{
    const char* soundLib;

    // Initialised to false to make sure it is ..
    Initialised = false;

    // Configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        speedOfSound = configManager->GetInt("Planeshift.Sound.SpeedOfSound", DEFAULT_SPEED_OF_SOUND);
        dopplerFactor = configManager->GetFloat("Planeshift.Sound.DopplerFactor", DEFAULT_DOPPLER_FACTOR);
        updateTime = configManager->GetInt("Planeshift.Sound.SndSysUpdateTime", DEFAULT_SNDSYS_UPDATE_TIME);
        soundLib = configManager->GetStr("Planeshift.Sound.SoundLib", DEFAULT_SOUNDLIB_PATH);
    }
    else
    {
        speedOfSound = DEFAULT_SPEED_OF_SOUND;
        dopplerFactor = DEFAULT_DOPPLER_FACTOR;
        updateTime = DEFAULT_SNDSYS_UPDATE_TIME;
        soundLib = DEFAULT_SOUNDLIB_PATH;
    }

    // Initializing the event timer
    eventTimer = csEventTimer::GetStandardTimer(objectReg);

    playerPosition.Set(0.0);
    playerVelocity.Set(0.0);

    // Create a new SoundSystem, SoundData Instance and the main SoundControl
    soundSystem = new SoundSystem;
    soundData = new SoundData;
    mainSndCtrl = new SoundControl(-1, iSoundControl::NORMAL);
    defaultSndCtrl = new SoundControl(-1, iSoundControl::NORMAL);

    if(soundSystem->Initialize(objectReg)
       && soundData->Initialize(objectReg))
    {
        // FIXME what if soundlib.xml doesnt exist?
        soundData->LoadSoundLib(soundLib, objectReg);
        LastUpdateTime = csGetTicks();
        Initialised = true;
    }
}

SoundSystemManager::~SoundSystemManager()
{
    Initialised = false;

    // pause all sounds and call updatesound to remove them
    csHash<SoundHandle*, uint>::GlobalIterator handleIter(soundHandles.GetIterator());
    SoundHandle* sh;

    while(handleIter.HasNext())
    {
        sh = handleIter.Next();
        sh->sndstream->Pause();
        sh->SetAutoRemove(true);
    }

    UpdateSound();

    // Deleting SoundControls
    csHash<SoundControl*, int>::GlobalIterator controlIter(soundControllers.GetIterator());
    SoundControl* sc;

    while(controlIter.HasNext())
    {
        sc = controlIter.Next();
        delete sc;
    }

    soundHandles.DeleteAll();
    soundControllers.DeleteAll();

    delete mainSndCtrl;
    delete defaultSndCtrl;
    delete soundSystem;
    delete soundData;

}

/*
 * Main Update function which is called by the engine
 * it wont do anything if not Initialised
 * slowdown is done here because we dont need that many updates
 *
 * there should be 50ms betweens the updates
 * and MUST be 100ms for FadeSounds
 * csTicks are not THAT accurate..
 * and because of that its now a FIXME 
 * i knew that we would run into this :/
 * but i already have a plan.. :)   
 */

void SoundSystemManager::Update()
{
    SndTime = csGetTicks();

    // call it all 100 Ticks
    if(Initialised && LastUpdateTime + updateTime <= SndTime)
    {
        UpdateSound();
        // make a update on sounddata to check if there are sounds to unload
        soundData->Update();
    }
}

/*
 * play a 2D sound
 * means: no 3D sound, no position etc needed
 * Note: You cant convert a 2D to a 3D sound
 * you need to supply a handle
 */

bool SoundSystemManager::
Play2DSound(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, SoundControl* &sndCtrl, SoundHandle* &handle)
{
    InitSoundHandle(name, loop, loopstart, loopend, volume_preset, CS_SND3D_DISABLE, sndCtrl, handle, false);

    if(handle == 0)
    {
        return false;
    }

    handle->sndstream->Unpause();

    return true;
}

/*
 * play a 3D sound
 * we need volume, position, direction, radiation, min- and maxrange.
 * 3dtype (absolute / relative
 * and direction if radiation is not 0
 * you need to supply a handle
 */

bool SoundSystemManager::
Play3DSound(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, SoundControl* &sndCtrl, csVector3 pos,
            csVector3 dir, float mindist, float maxdist, float rad,
            int type3d, SoundHandle* &handle, bool dopplerEffect)
{
    InitSoundHandle(name, loop, loopstart, loopend, volume_preset, type3d, sndCtrl, handle, dopplerEffect);

    if(handle == 0)
    {
        return false;
    }

    /* make it 3d */
    handle->ConvertTo3D(mindist, maxdist, pos, dir, rad);

    if(dopplerEffect)
    {
        // computing the delay caused by the speed of sound
        csVector3 diff = pos - soundSystem->GetListenerPosition();
        float distance = diff.Norm();
        unsigned int delay = distance * 1000 / speedOfSound;

        handle->UnpauseAfterDelay(delay);
    }
    else
    {
        handle->sndstream->Unpause();
    }

    return true;
}

bool SoundSystemManager::StopSound(uint handleID)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return false;
    }

    // Pause the sound and set autoremove
    // The handle will be removed in the next update
    handle->sndstream->Pause();
    handle->SetAutoRemove(true);

    return true;
}

bool SoundSystemManager::SetSoundSource(uint handleID, csVector3 position)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return false;
    }

    handle->sndsource3d->SetPosition(position);

    return true;
}

void SoundSystemManager::SetPlayerPosition(csVector3& pos)
{
    playerPosition = pos;
}

csVector3& SoundSystemManager::GetPlayerPosition()
{
    return playerPosition;
}

void SoundSystemManager::SetPlayerVelocity(csVector3 vel)
{
    playerVelocity = vel;
}

bool SoundSystemManager::IsHandleValid(uint handleID) const
{
    return soundHandles.Contains(handleID);
}

/*
 * reomves all idle sounds to save memory and sndsources
 * adjust volumes and fades music
 * TODO Split into three parts and make it event based
 */

void SoundSystemManager::UpdateSound()
{
    float vol;
    SoundHandle* sh;
    csArray<SoundHandle*> handles = soundHandles.GetAll();

    for(size_t i = 0; i < handles.GetSize(); i++)
    {
        sh = handles[i];

        if(sh->sndstream->GetPauseState() == CS_SNDSYS_STREAM_PAUSED
            && sh->GetAutoRemove() == true)
        {
            RemoveHandle(sh->GetID());
            continue;
        }

        // applying Doppler effect
        if(sh->Is3D() && sh->IsDopplerEffectEnabled())
        {
            ChangePlayRate(sh);
        }

        // fade in or out
        // fade >0 is number of steps up <0 is number of steps down, 0 is nothing
        if(sh->fade > 0)
        {
            sh->sndsource->SetVolume(sh->sndsource->GetVolume()
                                      + ((sh->fade_volume
                                          * sh->sndCtrl->GetVolume())
                                         * mainSndCtrl->GetVolume()));
            sh->fade--;
        }
        else if(sh->fade < 0)
        {
            /*
             *  fading down means we might want to stop the sound
             * if fade_stop is set do that (instead of the last step)
             * dont delete it here it would ruin the Array
             * our "garbage collector (UpdateSounds)" will pick it up
             *
             * also check the toggle just pause if its false
             */

            if((sh->fade == -1
               && sh->fade_stop == true)
               || sh->sndCtrl->GetToggle() == false)
            {
                RemoveHandle(sh->GetID());
                continue;
            }
            else
            {
                sh->sndsource->SetVolume(sh->sndsource->GetVolume()
                                          - ((sh->fade_volume
                                              * sh->sndCtrl->GetVolume())
                                             * mainSndCtrl->GetVolume()));
                sh->fade++;
            }
        }
        else if(sh->sndCtrl->GetToggle() == true)
        {
            if(mainSndCtrl->GetToggle() == false)
            {
                vol = VOLUME_ZERO;
            }
            else
            {
                vol = ((sh->preset_volume * sh->sndCtrl->GetVolume())
                       * mainSndCtrl->GetVolume());
            }

            // limit volume to 2.0f (VOLUME_MAX defined in manager.h)
            if(vol >= VOLUME_MAX)
            {
                sh->sndsource->SetVolume(VOLUME_MAX);
            }
            else
            {
                sh->sndsource->SetVolume(vol);
            }
        }
      LastUpdateTime = csGetTicks();
    }
}


/*
 * Update Listener position
 */

void SoundSystemManager::UpdateListener(csVector3 v, csVector3 f, csVector3 t)
{
    soundSystem->UpdateListener(v, f, t);
}

SoundControl* SoundSystemManager::AddSoundControl(int ctrlID, int type)
{
    SoundControl* newControl;

    if(soundControllers.Get(ctrlID, 0) != 0)
    {
        return 0;
    }

    newControl = new SoundControl(ctrlID, type);
    soundControllers.Put(ctrlID, newControl);

    return newControl;
}

void SoundSystemManager::RemoveSoundControl(SoundControl* sndCtrl)
{
    soundControllers.Delete(sndCtrl->GetID(), sndCtrl);
    delete sndCtrl;
}

SoundControl* SoundSystemManager::GetSoundControl(int ctrlID) const
{
    return soundControllers.Get(ctrlID, 0);
}


uint SoundSystemManager::FindHandleID()
{
    uint handleID;

    do
    {
        handleID = randomGen.Get(10000);
    } while(soundHandles.Get(handleID, 0) != 0
        || handleID == 0);

    return handleID;
}

void SoundSystemManager::RemoveHandle(uint handleID)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return;
    }

    soundHandles.Delete(handleID, handle);
    delete handle;
}

void SoundSystemManager::ChangePlayRate(SoundHandle* handle)
{
    int percentRate;
    float distance;
    float distanceAfterTimeUnit;
    float relativeSpeed;
    csVector3 sourcePosition;

    sourcePosition = handle->GetSourcePosition();
    distance = (playerPosition - sourcePosition).Norm();
    distanceAfterTimeUnit = (playerPosition + playerVelocity - sourcePosition).Norm();
    relativeSpeed = distanceAfterTimeUnit - distance;
    percentRate = (1 - dopplerFactor * relativeSpeed / speedOfSound) * 100;

    handle->sndstream->SetPlayRatePercent(percentRate);
}

void SoundSystemManager::
InitSoundHandle(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, int type3d, SoundControl* &sndCtrl, SoundHandle* &handle, bool dopplerEffect)
{
    uint handleID;

    if(Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return;
    }

    if(name == NULL)
    {
        Error1("Error: Play2DSound got NULL as soundname\n");
        return;
    }

    if(sndCtrl->GetToggle() == false) /* FIXME */
    {
        return;
    }

    handleID = FindHandleID();
    if(handle == 0)
    {
        handle = new SoundHandle();
    }
    handle->SetID(handleID);

    if(!handle->Init(name, loop, volume_preset, type3d, sndCtrl, dopplerEffect))
    {
      delete handle;
      handle = 0;
      return;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume((volume_preset * sndCtrl->GetVolume()));

    soundHandles.Put(handleID, handle);
}
