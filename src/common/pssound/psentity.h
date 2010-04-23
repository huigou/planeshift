/*
 * psentity.h
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


#ifndef PSENTITY_H_
#define PSENTITY_H_

/**
 * This object represents a planeshift entity sound.
 * this class is a thin class as this is work in progress
 * and everything is subject to change.
 */ 

class psEntity
{
    public:
    csString      name;             ///< name of the entity
    csString      resource;         ///< resource name of the sound
    float         minvol;           ///< vol at maxrange
    float         maxvol;           ///< vol at minrange
    float         minrange;         ///< range when it should reach maxvol
    float         maxrange;         ///< range when it should reach maxvol
    int           delay_before;     ///< number of seconds till first time this sound is played
    int           delay_after;      ///< number of seconds till played again
    float         probability;      ///< how high is the probability that this entity makes this sound
    int           when;             ///< counter to keep track when it has been played - zero means i may play at any time
    bool          active;           ///< is the sound active?
    SoundHandle  *handle;           ///< pointer to the SoundHandle if active
    int           timeofday;        ///< time when this entity starts playing
    int           timeofdayrange;   ///< time when this entity stops 
    
    /**
     * Constructer 
     */
    psEntity ();
    /**
     * Destructor
     */
    ~psEntity ();
    /**
     * Copy Constructor - WARNING INCOMPLETE
     */
    psEntity (psEntity* const &entity);
    /**
     * Check time of day.
     * Checks if time is within this entitys timewindow.
     * Returns true if it is.
     * @param time <24 && >0 is resonable but can be any valid int
     */
    bool CheckTimeOfDay (int time);
    /**
     * Play this entity sound.
     * You need to supply a SoundControl and the position for this sound.
     * @param ctrl SoundControl
     * @param entityposition position of this entity
     */
    bool Play (SoundControl* &ctrl, csVector3 entityposition);
    /**
     * The Callback gets called if the SoundHandle is destroyed.
     * It sets active to false and handle to NULL
     */
    static void StopCallback(void* object);
};

#endif /*PSENTITY_H_*/
