/*
 * control.h
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


#ifndef _SOUND_CONTROL_H_
#define _SOUND_CONTROL_H_

class SoundControl
{
    public:
    bool    isEnabled;           /* is toggle enabled? */
    float   volume;              /* volume as float */
    bool    isMuted;             /* is it muted? */
    bool    isDirty;             /* is this control dirty */
    int     id;                  /* id of this control */

    SoundControl ();             /* constructor*/
    ~SoundControl ();            /* destructor */
    bool GetState ();            /* returns isDirty */
    int GetID ();                /* returns id */

    float GetVolume ();          /* returns volume as float */
    void SetVolume (float vol);  /* set volume param is volume as float */

    void Unmute ();              /* unmute this */
    void Mute ();                /* mute this */

    bool GetToggle ();           /* returns toggle state */
    void SetToggle (bool value); /* set toggle (bool) */
    void DeactivateToggle ();    /* set toggle to false */
    void ActivateToggle ();      /* set toggle to true */
};

#endif /*_SOUND_CONTROL_H_*/
