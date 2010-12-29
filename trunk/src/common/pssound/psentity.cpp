/*
 * psentity.cpp
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
 

#include "pssound.h"

extern SoundSystemManager *SndSysMgr;

psEntity::psEntity ()
{
    minvol = VOLUME_ZERO;
    maxvol = VOLUME_ZERO;
    minrange = 0.0f;
    maxrange = 0.0f;
    probability = 0;
    active = false;
    timeofday = -1;
    timeofdayrange = -1;
    handle = NULL;
}

psEntity::psEntity (psEntity* const& /*entity*/)
{
}

psEntity::~psEntity ()
{
    if (handle != NULL)
    {
        handle->RemoveCallback();
    }
}

bool psEntity::CheckTimeOfDay (int time)
{
    if ((timeofday <= time)
         && (timeofdayrange >= time))
    {
        return true;        
    }

    return false;
}

bool psEntity::Play (SoundControl* &ctrl, csVector3 entityposition)
{
    if (handle != NULL)
    {
        handle->RemoveCallback(); // stop previous play
    }

    if (SndSysMgr->Play3DSound (resource, DONT_LOOP, 0, 0,
                                maxvol, ctrl,
                                entityposition, 0,
                                minrange, maxrange,
                                VOLUME_ZERO, CS_SND3D_ABSOLUTE,
                                handle))
    {
        active = true;
        when = (delay_after*1000);
        handle->SetCallback(this, &StopCallback);
        return true;
    }
    
    return false;
}

void psEntity::StopCallback(void* object)
{
    psEntity *which = (psEntity *) object;
    which->active = false;
    which->handle = NULL;
}

