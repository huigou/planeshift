/*
 * psentity.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Andrea Rizzi <88whacko@gmail.com>
 *           Saul Leite <leite@engineer.com>
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


psEntity::psEntity()
{
    active = false;
    when = 0;
    state = 1;
    id = 0;
    handle = 0;

    minMinRange = 0.0f;
    maxMaxRange = 0.0f;
    minTimeOfDayStart = 25;
    maxTimeOfDayEnd = -1;
}

psEntity::psEntity(psEntity* const& entity)
{
    factoryName = entity->factoryName;
    meshName = entity->meshName;

    statePar = csHash<StateParameters*, int>(entity->statePar);

    // updating statePar references
    csHash<StateParameters*, int>::GlobalIterator statIter(statePar.GetIterator());
    StateParameters* sp;

    while(statIter.HasNext())
    {
        sp = statIter.Next();
        sp->references++;
    }

    active = entity->active;
    when = entity->when;
    state = entity->state;
    id = entity->id;
    handle = 0;

    minMinRange = entity->minMinRange;
    maxMaxRange = entity->maxMaxRange;
    minTimeOfDayStart = entity->minTimeOfDayStart;
    maxTimeOfDayEnd = entity->maxTimeOfDayEnd;
}

psEntity::~psEntity()
{
    if(handle != 0)
    {
        handle->RemoveCallback();
    }

    // removing state parameters but only if there are no more references
    csHash<StateParameters*, int>::GlobalIterator statIter(statePar.GetIterator());
    StateParameters* sp;

    while(statIter.HasNext())
    {
        sp = statIter.Next();

        if(sp->references == 1)
        {
            delete sp;
        }
        else
        {
            sp->references--;
        }
    }

    statePar.DeleteAll();
}

float psEntity::GetProbability() const
{
    StateParameters* sp;

    if(state < 0)
    {
        return 0.0f;
    }

    sp = statePar.Get(state, 0);

    return sp->probability;
}

uint psEntity::GetMeshID() const
{
    return id;
}

void psEntity::SetMeshID(uint identifier)
{
    id = identifier;
}

bool psEntity::DefineState(int state, const char* resource, const char* startResource,
        float volume, float minRange, float maxRange, float probability,
        int timeOfDayStart, int timeOfDayEnd, int delayAfter)
{
    StateParameters* sp;

    sp = statePar.Get(state, 0);

    if(sp != 0)
    {
        return false;
    }
    
    sp = new StateParameters();

    sp->resource = resource;
    sp->startResource = startResource;

    sp->volume = volume;
    sp->minRange = minRange;
    sp->maxRange = maxRange;
    sp->probability = probability;
    sp->timeOfDayStart = timeOfDayStart;
    sp->timeOfDayEnd = timeOfDayEnd;
    sp->delayAfter = delayAfter;

    sp->references = 1;

    statePar.Put(state, sp);

    // updating variables that keep track of the extremes values
    if(minRange < minMinRange)
    {
        minMinRange = minRange;
    }
    if(maxRange > maxMaxRange)
    {
        maxMaxRange = maxRange;
    }
    if(timeOfDayStart < minTimeOfDayStart)
    {
        minTimeOfDayStart = timeOfDayStart;
    }
    if(timeOfDayEnd > maxTimeOfDayEnd)
    {
        maxTimeOfDayEnd = timeOfDayEnd;
    }

    return true;
}

void psEntity::ReduceDelay(int interval)
{
    if(when > 0)
    {
        when -= 50;
    }
}

bool psEntity::IsTemporary() const
{
    return (id != 0);
}

bool psEntity::IsPlaying() const
{
    return (handle != 0);
}

bool psEntity::IsReadyToPlay(int time, float range) const
{
    StateParameters* sp;

    // checking if it is in the undefined state
    if(state < 0)
    {
        return true;
    }

    sp = statePar.Get(state, 0);

    // checking time, range and delay
    if(range < sp->minRange || range > sp->maxRange)
    {
        return false;
    }
    else if(time < sp->timeOfDayStart || sp->timeOfDayEnd < time)
    {
        return false;
    }
    else if(when <= 0)
    {
        return true;
    }

    return false;
}

bool psEntity::CheckTimeAndRange(int time, float range) const
{

    if(range < minMinRange || range > maxMaxRange)
    {
        return false;
    }
    if(minTimeOfDayStart <= time && time <= maxTimeOfDayEnd)
    {
        return true;
    }
    
    return false;
}

void psEntity::SetState(int stat, SoundControl* ctrl, csVector3 entityPosition, bool forceChange)
{
    StateParameters* sp;

    // check if it's already in this state or if it's defined
    if(state == stat)
    {
        return;
    }

    // stopping previous sound if any
    if(handle != 0)
    {
        handle->RemoveCallback();
        SoundSystemManager::GetSingleton().StopSound(handle->GetID());
        handle = 0;
    }
    
    // setting state
    sp = statePar.Get(stat, 0);
    if(sp == 0)
    {
        if(forceChange)
        {
            state = -1; // undefined state
        }

        return;
    }

    state = stat;

    // playing the starting sound
    if(!(sp->startResource.IsEmpty()))
    {
        if(SoundSystemManager::GetSingleton().Play3DSound(sp->startResource, DONT_LOOP, 0, 0,
            sp->volume, ctrl, entityPosition, 0, sp->minRange, sp->maxRange,
            VOLUME_ZERO, CS_SND3D_ABSOLUTE, handle))
        {
            when = sp->delayAfter * 1000;
            handle->SetCallback(this, &StopCallback);
        }
    }
}

bool psEntity::Play(SoundControl* &ctrl, csVector3 entityPosition)
{
    StateParameters* sp;

    // checking if a sound is still playing
    if(handle != NULL)
    {
        return false;
    }

    // checking if the state is defined
    if(state < 0)
    {
        return false;
    }

    sp = statePar.Get(state, 0);

    if(!(sp->resource.IsEmpty()))
    {
        if(SoundSystemManager::GetSingleton().Play3DSound(sp->resource, DONT_LOOP, 0, 0,
            sp->volume, ctrl, entityPosition, 0, sp->minRange, sp->maxRange,
            VOLUME_ZERO, CS_SND3D_ABSOLUTE, handle))
        {
            when = sp->delayAfter * 1000;
            handle->SetCallback(this, &StopCallback);
            return true;
        }
    }
    
    return false;
}


void psEntity::StopCallback(void* object)
{
    psEntity* which = (psEntity*) object;
    which->handle = NULL;
}

