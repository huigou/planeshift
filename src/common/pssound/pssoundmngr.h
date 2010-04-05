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

#include "sound/sound.h"

/*
 * TODO merge those sct_ structs into one class
 */

struct sct_music
{
    csString       resource;
    int            type;
    float          minvol;
    float          maxvol;
    int            fadedelay;
    int            timeofday;
    int            timeofdayrange;
    int            weather;
    size_t         loopstart;
    size_t         loopend;
    bool           active;
    SoundHandle   *handle;
};

struct sct_emitter
{
    csString       resource;
    float          minvol;
    float          maxvol;
    int            fadedelay;
    float          minrange;
    float          maxrange;
    csString       mesh;
    csString       factory;
    float          factory_prob;
    csVector3      position;
    csVector3      direction;
    bool           active;
    int            timeofday;
    int            timeofdayrange;
    SoundHandle   *handle;
};

struct sct_ambient
{
    csString       resource;
    float          minvol;
    float          maxvol;
    int            fadedelay;
    int            timeofday;
    int            timeofdayrange;
    int            weather;
    csString       trigger;
    size_t         loopstart;
    size_t         loopend;
    bool           active;
    SoundHandle   *handle;
};

struct sct_entity
{
    csString      name;
    csString      resource;
    float         minvol;
    float         maxvol;
    float         minrange;
    float         maxrange;
    int           delay_before; /* number of seconds till first time this sound is played */
    int           delay_after;  /* number of seconds till played again */
    float         probability;  /* if a found entitie plays a sound */
    float         count;        /* number of entities of this type */
    int           when;         /* last time this sound has been played */ 
    bool          active;
    SoundHandle  *handle;
    int           timeofday;
    int           timeofdayrange;
};

/*
 * TODO create a class
 */

struct sctdata
{
    csString                name;
    bool                    active;
    sct_ambient            *activeambient;
    sct_music              *activemusic; 
    csArray<sct_ambient*>   ambient;
    csArray<sct_emitter*>   emitter;
    csArray<sct_music*>     music;
    csArray<sct_entity*>    entity;
};

/*
 * Named it SoundManager but its more a Sectormanager
 * because Sectors is all we have atm
 * a sector describes a 3d
 */


class psSoundManager
{
    public:
    
    sctdata         *activesector;      /* points to our active sector */
    csVector3        playerposition;    /* current playerposition */
    SoundQueue      *voicequeue;        /* a queue for voice playback */
    SoundControl    *ambientSndCtrl;    /* soundcontrol for ambient sounds */
    SoundControl    *musicSndCtrl;      /* soundcontrol for music */
    SoundControl    *voiceSndCtrl;      /* soundcontrol for voice */
    SoundControl    *actionSndCtrl;     /* soundcontrol for action sounds */
    SoundControl    *effectSndCtrl;     /* soundcontrol for effects */
    SoundControl    *guiSndCtrl;        /* soundcontrol for gui */
    SoundControl    *mainSndCtrl;       /* soundcontrol of our soundmanager */
    csRandomGen     *rng;               /* random gen */

    psSoundManager(iObjectRegistry* objectReg);
    ~psSoundManager ();

    /* load sector data */
    bool LoadData (iObjectRegistry* objectReg, csArray<sctdata*> &sectordata);
    /* load a given sector, position can be NULL */
    void Load ( const char* sector, csVector3 position );
    /* update pssoundmanager, updates all non event based things
     * has a build in throttle */
    void Update ();
    /* update listener position */
    void UpdateListener ( iView* view );
    /* sets ingame time */

    // simple PlaySound function to handle GUI and similar 'fire and forget' Sounds
    void PlayActionSound (const char *name);
    void PlayGUISound (const char *name);


    void SetTimeOfDay (int newTimeofday);
    /* sets weather */
    void SetWeather (int newWeather);

    /* sets current combat stance */
    void SetCombatStance (int newCombatstance);
    /* get current combat stance */
    int GetCombatStance ();
    
    /* toggles to enable or disable the thing they are named after */
    void SetMusicToggle (bool toggle);
    bool GetMusicToggle ();
    void SetAmbientToggle (bool toggle);
    bool GetAmbientToggle ();
    void SetCombatToggle (bool toggle);
    bool GetCombatToggle ();
    void SetLoopBGMToggle (bool toggle);
    bool GetLoopBGMToggle ();
    void SetVoiceToggle (bool toggle);
    bool GetVoiceToggle ();
    void SetChatToggle (bool toggle);
    bool GetChatToggle ();

    /* toggle to switch listener position between player and camera position */
    bool GetListenerOnCameraPos ();
    void SetListenerOnCameraPos (bool toggle);
     
    private:
    
    csRef<iVFS>                 vfs;                /* vfs were reading from */
    csArray<sctdata *>          sectordata;         /* array which contains all sector xmls - parsed */
    csTicks                     SndTime;            /* current csticks */
    csTicks                     LastUpdateTime;     /* when the last update happend */
    bool                        loopBGM;            /* loobBGM toggle */
    int                         weather;            /* current weather state */
    int                         combat;             /* current combat stance */
    int                         timeofday;          /* time of the day */
    bool                        combatMusic;        /* toggle for combatmusic */
    bool                        listenerOnCamera;   /* toggle for listener switch between player and camera position */
    bool                        chatToggle;         /* toggle for chatsounds */


    /* parses entities nodes out of a xml */
    void GetEntityNodes (csArray<sct_entity*> &entity_sounds, csRef<iDocumentNodeIterator> Itr);
    /* parses music nodes out of a xml */
    void GetMusicNodes (csArray<sct_music*> &music_sounds, csRef<iDocumentNodeIterator> Itr);
    /* parses emitter nodes out of a xml */
    void GetEmitterNodes (csArray<sct_emitter*> &emitter_sounds, csRef<iDocumentNodeIterator> Itr);
    /* parses ambient nodes out of a xml */
    void GetAmbientNodes (csArray<sct_ambient*> &ambient_sounds, csRef<iDocumentNodeIterator> Itr);
    /* converts factory emitters to real emitters */     
    void ConvertFactoriesToEmitter (sctdata* &sector);
    /* transfers handle from a old to a new sector */
    void TransferHandles (sctdata* &oldsector, sctdata* &newsector);

    /* update ambient playback */
    void UpdateAmbient (sctdata* &sector);
    /* update music playback */
    void UpdateMusic (sctdata* &sector);
    /* update emitters */    
    void UpdateEmitter (sctdata* &sector);
    /* update entities */
    void UpdateEntity (sctdata* &sector);
    /* update whole sector */
    void UpdateSector (sctdata * &sector);
};

#endif /*_PSSOUNDMNGR_H_*/
