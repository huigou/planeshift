/*
 * psemitter.cpp
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
 
psEmitter::psEmitter ()
{
    minvol = VOLUME_ZERO;
    maxvol = VOLUME_ZERO;
    fadedelay = 0;
    minrange = 0.0f;
    maxrange = 0.0f;
    factory_prob = 0;
    position = csVector3(0);
    direction = csVector3(0);
    active = false;
    timeofday = -1;
    timeofdayrange = -1;
    handle = NULL;
}

psEmitter::psEmitter (psEmitter* const &emitter)
{
    // FIXME, this copy constructor is incomplete
    resource = csString(emitter->resource);
    maxvol = emitter->maxvol;
    minvol = emitter->minvol;
    minrange = emitter->minrange;
    maxrange = emitter->maxrange;
    timeofday = emitter->timeofday;
    timeofdayrange = emitter->timeofdayrange;
}

psEmitter::~psEmitter ()
{
    // stop deletes the handle
    Stop();
}

bool psEmitter::CheckRange (csVector3 playerpos)
{
    csVector3 rangeVec;
    float range;
    
    rangeVec = position - playerpos;
    range = rangeVec.Norm();

    if (!range) /* if range is NAN */
    {
        return false;
    }
    else if (range <= maxrange)
    {
        return true;
    }

    return false;
}

bool psEmitter::CheckTimeOfDay (int time)
{
    if ((timeofday <= time)
         && (timeofdayrange >= time))
    {
        return true;        
    }

    return false;
}

bool psEmitter::Play (SoundControl* &ctrl)
{
    Stop(); // stop any previous play
    if (SndSysMgr->Play3DSound (resource, LOOP, 0, 0,
                                maxvol, ctrl,
                                position, direction,
                                minrange, maxrange,
                                VOLUME_ZERO, CS_SND3D_ABSOLUTE,
                                handle))
    {
        active = true;
        handle->SetCallback(this, &StopCallback);
        return true;
    }
    
    return false;
}

void psEmitter::Stop ()
{
    active = false;

    if (handle != NULL)
    {
        SndSysMgr->StopSound(handle);
    }
}

void psEmitter::StopCallback(void* object)
{
    psEmitter *which = (psEmitter *) object;
    which->active = false;
    which->handle = NULL;
}

