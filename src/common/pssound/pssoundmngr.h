/*
 * pssoundmngr.h
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

#ifndef _PSSOUNDMNGR_H_
#define _PSSOUNDMNGR_H_

#include "pssound.h"
/*
 * Named it SoundManager but its more a Sectormanager
 * because Sectors is all we have atm
 * a sector describes a 3d
 */


class psSoundManager
{
    public:

    psSoundSector   *activesector;      ///< points to our active sector
    csVector3        playerposition;    ///< current playerposition
    SoundQueue      *voicequeue;        ///< a queue for voice playback
    SoundControl    *ambientSndCtrl;    ///< soundcontrol for ambient sounds
    SoundControl    *musicSndCtrl;      ///< soundcontrol for music
    SoundControl    *voiceSndCtrl;      ///< soundcontrol for voice
    SoundControl    *actionSndCtrl;     ///< soundcontrol for action sounds
    SoundControl    *effectSndCtrl;     ///< soundcontrol for effects
    SoundControl    *guiSndCtrl;        ///< soundcontrol for gui
    SoundControl    *mainSndCtrl;       ///< soundcontrol of our soundmanager

    psToggle         loopBGM;           ///< loobBGM toggle
    psToggle         combatMusic;       ///< toggle for combatmusic
    psToggle         listenerOnCamera;  ///< toggle for listener switch between player and camera position
    psToggle         chatToggle;        ///< toggle for chatsounds

    /**
     * Constructor initializes all needed objects.
     * Loads Sector data and get everything started.
     * Initializes @see SoundSystemManager various SoundControls
     * a @SoundQueue for voice sounds and @SoundData also
     * registers callbacks.
     */
    psSoundManager (iObjectRegistry* objectReg);
    /**
     * Destructor destroys all objects within this SoundManager.
     * All pointers to objects within this SoundManager are invalid
     * after destruction. 
     */
    ~psSoundManager ();

    /**
     * Load a given sector
     * @param sector sector by name
     */
    void Load (const char* sector);
    
    /**
     * Reloads this Soundmanager
     */
    void Reload ();
    
    /**
     * Unloads the current active sector
     */
    void Unload ();
    /**
     * Updates pssoundmanager.
     * Updates all non event based things (Entitys and Emitter)
     * also drives the SoundSystemManager loop
     * has a build in throttle
     */
    void Update ();
    /**
     * Updates Listener position.
     * Uses either the player position (entity) or the camera posiont (iView)
     * @param view iView of the camera.
     */
    void UpdateListener (iView* view);
    
    /**
     * Simple function to handle Action sounds.
     * Those sounds are 'fire and forget' and cant be managed.
     * uses @see SoundControl actionSndCtrl / effectSndCtrl
     * @param name name of the sound resource
     */
    void PlayActionSound (const char *name);
    /**
     * Simple function to handle GUI sounds.
     * Those sounds are 'fire and forget' and cant be managed.
     * uses @see SoundControl guiSndCtrl
     * @param name name of the sound resource
     */
    void PlayGUISound (const char *name);

    /**
     * Set Sound timeofday
     */
    void SetTimeOfDay (int newTimeofday);
    /**
     * Set Sound weather
     */
    void SetWeather (int newWeather);

    /**
     * Set players position
     */
    void SetPosition (csVector3 playerpos);

    /**
     * Set Sound stance.
     * Used to determine which music to play.
     */
    void SetCombatStance (int newCombatstance);
    /**
     * Returns current stance
     */
    int GetCombatStance ();

    /**
     * Set voice toggle.
     * @param toggle true or false
     */
    void SetVoiceToggle (bool toggle);
    /**
     * Get voice toggle state
     * returns true or false
     */
    bool GetVoiceToggle ();
    /**
     * Callback function that makes a music update. 
     */
    static void UpdateMusicCallback(void* object);
    /**
     * Callback function that makes a ambient music update. 
     */
    static void UpdateAmbientCallback(void* object);

    private:

    csArray<psSoundSector *>    sectordata;         ///< array which contains all sector xmls - parsed
    csTicks                     SndTime;            ///< current csticks
    csTicks                     LastUpdateTime;     ///< csticks when the last update happend

    int                         weather;            ///< current weather state
    int                         combat;             ///< current stance

    csRandomGen                 rng;                ///< random generator

    /**
     * Converts factory emitters to real emitters
     */
    void ConvertFactoriesToEmitter (psSoundSector* &sector);
    /**
     * Transfers handle from a psSoundSector to a another psSoundSector
     * Moves SoundHandle and takes care that everything remains valid.
     * Good example on what is possible with all those interfaces.
     * @param oldsector sector to move from
     * @param newsector sector to move to
     */
    void TransferHandles (psSoundSector* &oldsector, psSoundSector* &newsector);

    /**
     * Find sector by name.
     * @param name name of the sector youre searching
     * @param sector your sector pointer
     * @returns true or false
     */

    bool FindSector (const char *name, psSoundSector* &sector);

    /**
     * Update a whole sector
     * @param sector Sector to update
     */
    void UpdateSector (psSoundSector * &sector);

    /**
     * Load all sectors
     * Loads all sound sector xmls and creates psSoundSector objects
     */
    bool LoadSectors ();
    /**
     * Reload all sector xmls (DANGEROUS!)
     */
    void ReloadSectors ();
    /**
     * Unload all sector xmls (DANGEROUS!)
     */
    void UnloadSectors ();
    
};

#endif /*_PSSOUNDMNGR_H_*/

