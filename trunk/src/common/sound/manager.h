/*
 * manager.h
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

#ifndef _SOUND_MANAGER_H_
#define _SOUND_MANAGER_H_

enum
{
    LOOP        = true,
    DONT_LOOP   = false
};

enum
{
    VOLUME_MIN  =   0,
    VOLUME_NORM =   1,
    VOLUME_MAX  =   2
};

/*
 * TODO merge Play* functions
 */

class SoundSystemManager
{
    public:
    bool             Initialised;                   /* is initialized ? */
    SoundControl    *mainSndCtrl;                   /* sound control for this manager */
    SoundControl    *guiSndCtrl;                    /* sound control for paws / gui */
    SoundControl    *effectSndCtrl;                 /* sound control for effects / actions */
    
    /* initialize */
    void Initialize(iObjectRegistry* objectReg);     

    /* update sound, do fading, remove sounds */
    void UpdateSound ();                            

    /* play a 2D sound with the given parameters */
    bool Play2DSound (const char *name, bool loop, size_t loopstart,
                      size_t loopend, float volume_preset,
                      SoundControl* &sndCtrl, SoundHandle * &handle);
    
    /* play a 3D sound with the givven parameters */
    bool Play3DSound (const char *name, bool loop, size_t loopstart,
                      size_t loopend, float volume_preset,
                      SoundControl* &sndCtrl, csVector3 pos, csVector3 dir,
                      float mindist, float maxdist, float rad, int type3d,
                      SoundHandle * &handle);

    /* update listener position to the following cords */
    void UpdateListener (csVector3 v, csVector3 f, csVector3 t);
    
    /* calls and throttles calls on updatesound */
    void Update ();
    /* get a new soundcontrol */
    SoundControl* GetSoundControl ();

    private:
    csArray<SoundControl *> soundController;    /* array which contains  all soundocontrollers */
    
    csArray<SoundHandle *>  soundHandles;       /* array which contains all handles */

    csTicks                 SndTime;            /* current csticks */
    csTicks                 LastUpdateTime;     /* when the last update happened */
};

#endif /*_SOUND_MANAGER_H_*/
