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
#include "util/log.h"

/*
 * Initialize the SoundSystem (SndSys) and the Datamanager (SndData)
 * Load our soundlib.xml - FIXME HARDCODED
 *
 * Set Initialized to true if successfull / to false if not
 */

SoundSystemManager::SoundSystemManager(iObjectRegistry* objectReg)
{
    // Initialised to false to make sure it is ..
    Initialised = false;

    // Create a new SoundSystem, SoundData Instance and the main SoundControl
    soundSystem = new SoundSystem;
    soundData = new SoundData;
    mainSndCtrl = new SoundControl(-1, iSoundControl::NORMAL);

    if(soundSystem->Initialize(objectReg)
       && soundData->Initialize(objectReg))
    {
        //  soundLib = cfg->GetStr("PlaneShift.Sound.SoundLib", "/planeshift/art/soundlib.xml"); /* FIXME HARDCODED*/
        // also FIXME what if soundlib.xml doesnt exist?
        soundData->LoadSoundLib("/planeshift/art/soundlib.xml", objectReg);
        LastUpdateTime = csGetTicks();
        Initialised = true;
    }
}

SoundSystemManager::~SoundSystemManager()
{
    Initialised = false;

    // pause all sounds and call updatesound to remove them
    csHash<SoundHandle*, csString>::GlobalIterator handleIter(soundHandles.GetIterator());
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
    if(Initialised && LastUpdateTime + 100 <= SndTime)
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
    /* FIXME redundant code Play3DSound */

    if(Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return false;
    }

    if(name == NULL)
    {
        Error1("Error: Play2DSound got NULL as soundname\n");
        return false;
    }

    if(sndCtrl->GetToggle() == false) /* FIXME */
    {
        return false;
    }

    handle = new SoundHandle(this);

    if(!handle->Init(name, loop, volume_preset, CS_SND3D_DISABLE, sndCtrl))
    {
      delete handle;
      handle = NULL;
      return false;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume((volume_preset * sndCtrl->GetVolume()));

    handle->sndstream->Unpause();
    soundHandles.Put(name, handle);

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
            int type3d, SoundHandle* &handle)
{
    // FIXME redundant code Play2DSound
    if(Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return false;
    }

    if(name == NULL)
    {
        Error1("Error: Play2DSound got NULL as soundname\n");
        return false;
    }

    if(sndCtrl->GetToggle() == false) /* FIXME */
    {
        return false;
    }

    handle = new SoundHandle(this);

    if(!handle->Init(name, loop, volume_preset, type3d, sndCtrl))
    {
      delete handle;
      handle = NULL;
      return false;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume((volume_preset * sndCtrl->GetVolume()));

    /* make it 3d */
    handle->ConvertTo3D(mindist, maxdist, pos, dir, rad);
    handle->sndstream->Unpause();

    soundHandles.Put(name, handle);
    return true;
}

bool SoundSystemManager::StopSound(const char* fileName)
{
    SoundHandle* handle = soundHandles.Get(fileName, 0);
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

bool SoundSystemManager::SetSoundSource(const char* fileName, csVector3 position)
{
    SoundHandle* handle = soundHandles.Get(fileName, 0);
    if(handle == 0)
    {
        return false;
    }

    handle->sndsource3d->SetPosition(position);

    return true;
}

/*
 * reomves all idle sounds to save memory and sndsources
 * adjust volumes and fades music
 * TODO Split into three parts and make it event based
 */

void SoundSystemManager::UpdateSound()
{
    float vol;
    csHash<SoundHandle*, csString>::GlobalIterator handleIter(soundHandles.GetIterator());
    SoundHandle* sh;

    while(handleIter.HasNext())
    {
        sh = handleIter.Next();

        if(sh->sndstream->GetPauseState() == CS_SNDSYS_STREAM_PAUSED
           && sh->GetAutoRemove() == true)
        {
            RemoveHandle(sh->name);
            continue;
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
                RemoveHandle(sh->name);
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

void SoundSystemManager::RemoveHandle(const char* fileName)
{
    SoundHandle* handle = soundHandles.Get(fileName, 0);
    if(handle == 0)
    {
        return;
    }

    soundHandles.Delete(fileName, handle);
    delete handle;
}