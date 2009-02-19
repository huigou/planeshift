/*
 * isoundmngr.h -- Saul Leite <leite@engineer.com>
 *
 * Copyright (C) 2001 PlaneShift Team (info@planeshift.it,
 * http://www.planeshift.it)
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
#ifndef I_SOUND_H
#define I_SOUND_H



#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <iengine/engine.h>


#include "isndsys/ss_structs.h"
#include "isndsys/ss_source.h"
#include "isndsys/ss_stream.h"
#include "isndsys/ss_loader.h"
#include "isndsys/ss_renderer.h"
#include "isndsys/ss_listener.h"
#include "isndsys/ss_data.h"


// A few helper macros which were used during conversion to the new system
//  these can all be migrated out of source code and then undefined as convenient
#define SOUND_STREAM_TYPE iSndSysStream
#define SOUND_SOURCE_TYPE iSndSysSource
#define SOUND_SOURCE3D_TYPE iSndSysSource3D
#define SOUND_LOADER_TYPE iSndSysLoader
#define SOUND_RENDER_TYPE iSndSysRenderer
#define SOUND_LISTENER_TYPE iSndSysListener
#define SOUND_DATA_TYPE iSndSysData

enum Fade_Direction
{
    FADE_UP,
    FADE_DOWN
};


struct iView;
SCF_VERSION(iSoundManager, 0, 0, 1);

struct iSoundManager : public virtual iBase
{
    enum
    {
    NO_LOOP = 0,
    LOOP_SOUND = 1
    };
    /// Load soundlib.xml into memory and initialize the sound renderer.
    virtual bool Setup() = 0;

    virtual void ChangeTimeOfDay( int newTime ) = 0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetVolume(float vol) =0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetMusicVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetMusicVolume(float vol) =0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetAmbientVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetAmbientVolume(float vol) =0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetActionsVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetActionsVolume(float vol) =0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetGUIVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetGUIVolume(float vol) =0;

    /// Get current volume of renderer, between 0 and 1.
    virtual float GetVoicesVolume() =0;

    /// Set current volume of renderer, between 0 and 1.
    virtual void SetVoicesVolume(float vol) =0;

    /**
     * Play a named sound and optionally loop it.  If it is looped,
     * the caller must save the csRef returned by this function and
     * call Stop() later itself.
     */
    virtual csRef<SOUND_SOURCE_TYPE> StartMusicSound(const char* name,bool loop = NO_LOOP) = 0;
    virtual csRef<SOUND_SOURCE_TYPE> StartAmbientSound(const char* name,bool loop = NO_LOOP) = 0;
    virtual csRef<SOUND_SOURCE_TYPE> StartActionsSound(const char* name,bool loop = NO_LOOP) = 0;
    virtual csRef<SOUND_SOURCE_TYPE> StartGUISound(const char* name,bool loop = NO_LOOP) = 0;
    virtual csRef<SOUND_SOURCE_TYPE> StartVoiceSound(const char* name,bool loop = NO_LOOP) = 0;

    // Playback of GUI sounds can also be performed through the PawsManager.

    // This one used to be the main one. Please avoid unless necessary.
    virtual csRef<SOUND_SOURCE_TYPE> StartSound(const char* name,float volume,bool loop = NO_LOOP) = 0;


    /// Stop playing the background sound and start another one.
    virtual bool OverrideBGSong(const char* name,
                                bool loop = true,
                                float fadeTime = 2.0 ) = 0;
    virtual void StopOverrideBG() = 0;

    /// Set whether music will be played or not, based on toggle parm.
    virtual void ToggleMusic(bool toggle) = 0;

    /// Set whether ambient sounds will actually be played or not.
    virtual void ToggleAmbient(bool toggle) = 0;

    /// Set whether actions sounds will actually be played or not.
    virtual void ToggleActions(bool toggle) = 0;

    /// Set whether gui sounds will actually be played or not.
    virtual void ToggleGUI(bool toggle) = 0;

    /// Set whether voices will actually be played or not.
    virtual void ToggleVoices(bool toggle) = 0;

    virtual void ToggleLoop(bool toggle) = 0;
    virtual bool LoopBGM() = 0;

    virtual void ToggleCombatMusic(bool toggle) = 0;
    virtual bool PlayingCombatMusic() = 0;

    /// This returns if we are playing music as a setting or not
    virtual bool PlayingMusic() = 0;

    /// This returns if we are playing ambient sounds as a setting or not
    virtual bool PlayingAmbient() = 0;

    /// This returns if we are playing actions sounds as a setting or not
    virtual bool PlayingActions() = 0;

    /// This returns if we are playing gui sounds as a setting or not
    virtual bool PlayingGUI() = 0;

    /// This returns if we are playing npc voice sounds as a setting or not
    virtual bool PlayingVoices() = 0;

    /// This returns if the music is the one of a fight or not
    virtual void ChangeMusicMode(bool combat) = 0;

    ///This returns the song name that is overriding
    virtual const char* GetSongName() = 0;

    /// Play a sound effect based on message type
    virtual void HandleSoundType(int type) = 0;


    virtual void Update( csVector3 pos ) = 0;
    virtual void Update( iView* view ) = 0;
    /** Update the current weather. This will trigger WEATHER sounds in
      * the current sector
      * @param weather New weather from the WeatherSound enum (weather.h)
      */
    virtual void UpdateWeather(int weather,const char* sector) = 0;
    virtual void UpdateWeather(int weather) = 0;

    /** Retrieves a iSoundHandle for a given resource name.
      * If the resource is not loaded and cannot be loaded an invalid csRef<> is returned.
      * (ref.IsValid() == false).
      *
      * @param name The resource name of the sound file.
      *
     */
    virtual csRef<SOUND_DATA_TYPE> GetSoundResource(const char *name) = 0;

    /// Retrieves a pointer to the sound renderer - the main interface of the sound system
    virtual csRef<SOUND_RENDER_TYPE> GetSoundSystem() = 0;

    virtual void StartMapSoundSystem() = 0;
    virtual void EnterSector( const char* sectorName, int timeOfDay, int weather, csVector3& position ) = 0;

    virtual void FadeSectorSounds( Fade_Direction dir ) = 0;
};

#endif // I_SOUND_H


