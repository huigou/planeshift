/*
* soundctrl.h
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

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundctrl.h>


/**
* A Volume and Sound control class.
* With this class you can control the volume and the overall state
* of a unlimited number of SoundsHandles. The whole class is pretty straith forward and selfdescribing.
* It also provides a callback functionality which calls back whenever something changes.
* You can use it by using SetCallback, please make sure that you call RemoveCallback
* before removing whether this Callback is set to.

* Each SoundHandle must have one SoundControl associated. This is done during
* SoundHandle creation @see SystemSoundManager or @see SoundHandle::Init for details.
*/

class SoundControl: public iSoundControl
{
public:

    /**
    * Constructor.
    * Defaults are unmuted, enabled and volume is set to 1.0f.
    */ 
    SoundControl(int ID, int type);

    /**
    * Destructor
    */
    virtual ~SoundControl();

    /**
    * Sets internal callbackpointers.
    * Those Callbacks are called whenever a control changes.
    * E.g. someone changes Volume or pushes a Toggle
    * @param object pointer to a instance
    * @param function pointer to a static void function within that instance
    */ 
    void SetCallback(void (*object), void (*function) (void *));

    /**
    * Removes Callback
    */
    void RemoveCallback();

    /**
    * Returns this Volume ID.
    */
    virtual int GetID() const;

    /**
     * Get the SoundControl's type.
     */
    virtual int GetType() const;

    /**
     * Set the SoundControl's type. Note that it does not affect
     * the callback. The callback must be managed separately.
     */
    void SetType(int type);

    /**
    * Returns current Volume as float.
    */
    virtual float GetVolume() const;

    /**
    * Sets volume to the given float.
    * @param vol Volume as float
    */
    virtual void SetVolume(float vol);

    /**
    * Unmute this.
    */
    virtual void Unmute();

    /**
    * Mute this
    */
    virtual void Mute();

    /**
    * Get current Toggle state.
    * Returns isEnabled 
    */
    virtual bool GetToggle() const;

    /**
    * Sets Toggle.
    * @param value true or false
    */           
    virtual void SetToggle(bool value);

    /**
    * deactivates Toggle.
    * Sets isEnabled to false
    */
    virtual void DeactivateToggle();

    /**
    * activates Toggle.
    * Sets isEnabled to true
    */
    virtual void ActivateToggle(); 

private:
    int     id;                         ///< id of this control
    int     type;                       ///< type of this control
    bool    isEnabled;                  ///< is it enabled? true or false
    float   volume;                     ///< current volume as float
    bool    isMuted;                    ///< is it muted? true or false

    void (*callbackObject);             ///< pointer to a callback object (if set)
    void (*callbackFunction) (void *);  ///< pointer to a callback function (if set)
    bool hasCallback;                   ///< true if theres a Callback set, false if not

    /**
    * Will call the Callback if set.
    * Checks @see hasCallback and call the Callback if true. 
    */
    void Callback();

};

#endif /*_SOUND_CONTROL_H_*/
