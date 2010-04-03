/*
 * handle.h
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

#ifndef _SOUND_HANDLE_H_
#define _SOUND_HANDLE_H_

enum
{
    FADE_DOWN   =   0,
    FADE_UP     =   1,
    FADE_STOP   =   -1
};

class SoundHandle
{
    public:
    csString                                name;           /* name of the resource or the file - not unique */
    SoundControl                           *sndCtrl;        /* SoundControl */
    float                                   preset_volume;  /* the volume it _should_ have */
    int                                     fade;           /* >0 is number of steps up <0 is number of steps down, 0 is nothing */
    float                                   fade_volume;    /* volume we add or remove in each step */
    bool                                    fade_stop;      /* pause this sound after fading down */
    bool                                    autoremove;

    csRef<iSndSysData>                      snddata;        /* sound data */
    csRef<iSndSysStream>                    sndstream;      /* sound stream */
    csRef<iSndSysSource>                    sndsource;      /* sndsource if 2D */
    csRef<iSndSysSource3D>                  sndsource3d;    /* sndsource if 3D */
    csRef<iSndSysSource3DDirectionalSimple> sndsourcedir;   /* additional source if 3D and directional */
  
    SoundHandle ();                                         /* constructor */
    ~SoundHandle ();                                        /* destructor */
    void Fade (float volume, int time, int direction);      /* calculate fading for this handle */
    bool Init (const char *resname, bool loop,              /* init this handle */
               float volume_preset, int type,
               SoundControl* &ctrl);
    void ConvertTo3D (float mindist, float maxdist,         /* convert this handle to a 3D handle */
                      csVector3 pos, csVector3 dir,
                      float rad);

    void SetAutoRemove (bool toggle);
    bool GetAutoRemove ();
};

#endif /*_SOUND_HANDLE_H_*/
