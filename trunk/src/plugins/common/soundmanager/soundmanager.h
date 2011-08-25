/*
* soundmanager.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iutil/comp.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundmngr.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "pssound.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
struct iObjectRegistry;
class InstrumentManager;

#define DEFAULT_SECTOR_UPDATE_TIME 50
#define DEFAULT_INSTRUMENTS_PATH "/planeshift/art/instruments.xml"
#define DEFAULT_AREAS_PATH "/planeshift/soundlib/areas/"
#define DEFAULT_COMMON_SECTOR_NAME "common"

/**
 * Implement iSoundManager.
 * @see iSoundManager
 */
class SoundManager: public scfImplementation3<SoundManager, iSoundManager, iComponent, iEventHandler>
{
public:
    static uint updateTime;     ///< update throttle in milliseconds

    SoundManager(iBase* parent);
    virtual ~SoundManager();

    //From iComponent
    virtual bool Initialize(iObjectRegistry* objReg);

    //From iEventHandler
    virtual bool HandleEvent(iEvent &e);
    CS_EVENTHANDLER_NAMES("crystalspace.planeshift.sound")
    virtual const csHandlerID* GenericPrec(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &enr, csEventID id) const;
    virtual const csHandlerID* GenericSucc(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &enr, csEventID id) const { return 0; }
    CS_EVENTHANDLER_DEFAULT_INSTANCE_CONSTRAINTS

    //From iSoundManager
    //Sectors managing
    virtual bool InitializeSectors();
    virtual void LoadActiveSector(const char* sector);
    virtual void UnloadActiveSector();
    virtual void ReloadSectors();

    //SoundControls managing
    virtual iSoundControl* AddSndCtrl(int ctrlID, int type);
    virtual void RemoveSndCtrl(iSoundControl* sndCtrl);
    virtual iSoundControl* GetSndCtrl(int ctrlID);
    virtual iSoundControl* GetMainSndCtrl();

    //SoundQueue managing
    virtual bool AddSndQueue(int queueID, iSoundControl* sndCtrl);
    virtual void RemoveSndQueue(int queueID);
    virtual bool PushQueueItem(int queueID, const char* fileName);

    //State
    virtual void SetCombatStance(int newCombatStance);
    virtual int GetCombatStance() const;
    virtual void SetPlayerMovement(csVector3 playerPosition, csVector3 playerVelocity);
    virtual csVector3 GetPosition() const;
    virtual void SetTimeOfDay(int newTimeOfDay);
    virtual int GetTimeOfDay() const;
    virtual void SetWeather(int newWeather);
    virtual int GetWeather() const;
    virtual void SetEntityState(int state, iMeshWrapper* mesh, bool forceChange);

    //Toggles
    virtual void SetLoopBGMToggle(bool toggle);
    virtual bool IsLoopBGMToggleOn();
    virtual void SetCombatMusicToggle(bool toggle);
    virtual bool IsCombatMusicToggleOn();
    virtual void SetListenerOnCameraToggle(bool toggle);
    virtual bool IsListenerOnCameraToggleOn();
    virtual void SetChatToggle(bool toggle);
    virtual bool IsChatToggleOn();

    //Play sounds
    virtual bool IsSoundValid(uint soundID) const;
    virtual uint PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl);
    virtual uint PlaySound(const char* fileName, bool loop, iSoundControl* &ctrl, csVector3 pos, csVector3 dir, float minDist, float maxDist);
    virtual uint PlaySong(csRef<iDocument> musicalSheet, const char* instrument, float errorRate,
        iSoundControl* ctrl, csVector3 pos, csVector3 dir);
    virtual bool StopSound(uint soundID);
    virtual bool SetSoundSource(uint soundID, csVector3 position);

    //Updating function
    virtual void Update();
    virtual void UpdateListener(iView* view);
    

private:
    bool                        isSectorLoaded;    ///< true if the sectors are loaded
    csRef<iObjectRegistry>      objectReg;         ///< object registry
    SoundSystemManager*         sndSysMgr;         ///< the sound system manager used to play sounds
    InstrumentManager*          instrMgr;          ///< the instruments manager

    SoundControl*               mainSndCtrl;       ///< soundcontrol of our soundmanager
    SoundControl*               ambientSndCtrl;    ///< soundcontrol for ambient sounds
    SoundControl*               musicSndCtrl;      ///< soundcontrol for music
    csHash<SoundQueue*, int>    soundQueues;       ///< all the SoundQueues created by ID
    psToggle                    loopBGM;           ///< loobBGM toggle
    psToggle                    combatMusic;       ///< toggle for combatmusic
    psToggle                    listenerOnCamera;  ///< toggle for listener switch between player and camera position
    psToggle                    chatToggle;        ///< toggle for chatsounds

    csTicks                     sndTime;           ///< current csticks
    csTicks                     lastUpdateTime;    ///< csticks when the last update happend

    csArray<psSoundSector*>     sectorData;        ///< array which contains all sector xmls - parsed
    psSoundSector*              activeSector;      ///< points to our active sector
    psSoundSector*              commonSector;      ///< sector that keeps features common to all sectors
    int                         weather;           ///< current weather state from weather.h
    int                         combat;            ///< current stance

    csRandomGen                 rng;               ///< random generator

    csEventID                   evSystemOpen;      ///< ID of the 'Open' event fired on system startup

    /**
     * Initialize the SoundSystemManager and the default SoundControls and queues.
     */
    void Init();

    /**
     * Callback function that makes a music update. 
     */
    static void UpdateMusicCallback(void* object);

    /**
     * Callback function that makes a ambient music update. 
     */
    static void UpdateAmbientCallback(void* object);

    /**
    * Converts factory emitters to real emitters
    */
    void ConvertFactoriesToEmitter(psSoundSector* &sector);
    
    /**
    * Transfers handle from a psSoundSector to a another psSoundSector
    * Moves SoundHandle and takes care that everything remains valid.
    * Good example on what is possible with all those interfaces.
    * @param oldsector sector to move from
    * @param newsector sector to move to
    */
    void TransferHandles(psSoundSector* &oldsector, psSoundSector* &newsector);

    /**
    * Find sector by name.
    * @param name name of the sector youre searching
    * @param sector your sector pointer
    * @return true or false
    */
    bool FindSector(const char* name, psSoundSector* &sector);

    /**
    * Update a whole sector
    * @param sector Sector to update
    */
    void UpdateSector(psSoundSector* &sector);

    /**
    * Load all sectors
    * Loads all sound sector xmls and creates psSoundSector objects
    */
    bool LoadSectors();

    /**
    * Reload all sector xmls (DANGEROUS!)
    */
    void ReloadAllSectors();

    /**
    * Unload all sector xmls (DANGEROUS!)
    */
    void UnloadSectors();

};    

#endif // __SOUNDMANAGER_H__
