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
#include "soundmanager.h"


psEntity::psEntity(bool isFactory, const char* name)
{
    isFactoryEntity = isFactory;
    entityName = name;

    isActive = false;
    when = 0;
    state = 1;
    id = 0;
    handle = 0;

    minRange = 0.0f;
    maxRange = 0.0f;
    minTimeOfDayStart = 25;
    maxTimeOfDayEnd = -1;
}

psEntity::psEntity(psEntity* const& entity)
{
    isFactoryEntity = entity->isFactoryEntity;
    entityName = entity->entityName;

    states = csHash<EntityState*, int>(entity->states);

    // updating states references
    csHash<EntityState*, int>::GlobalIterator statIter(states.GetIterator());
    EntityState* entityState;

    while(statIter.HasNext())
    {
        entityState = statIter.Next();
        entityState->references++;
    }

    isActive = entity->isActive;
    when = entity->when;
    state = entity->state;
    id = entity->id;
    handle = 0;

    minRange = entity->minRange;
    maxRange = entity->maxRange;
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
    csHash<EntityState*, int>::GlobalIterator statIter(states.GetIterator());
    EntityState* entityState;

    while(statIter.HasNext())
    {
        entityState = statIter.Next();

        if(entityState->references == 1)
        {
            delete entityState;
        }
        else
        {
            entityState->references--;
        }
    }

    states.DeleteAll();
}

void psEntity::SetAsMeshEntity(const char* meshName)
{
    if(meshName == 0)
    {
        return;
    }

    isFactoryEntity = false;
    entityName = meshName;
}

void psEntity::SetAsFactoryEntity(const char* factoryName)
{
    if(factoryName == 0)
    {
        return;
    }

    isFactoryEntity = true;
    entityName = factoryName;
}

void psEntity::SetRange(float minDistance, float maxDistance)
{
    minRange = minDistance;
    maxRange = maxDistance;
}

float psEntity::GetProbability() const
{
    EntityState* entityState;

    if(state < 0)
    {
        return 0.0f;
    }

    entityState = states.Get(state, 0);

    return entityState->probability;
}

uint psEntity::GetMeshID() const
{
    return id;
}

void psEntity::SetMeshID(uint identifier)
{
    id = identifier;
}

bool psEntity::DefineState(csRef<iDocumentNode> stateNode)
{
    int stateID;

    EntityState* entityState;
    csRef<iDocumentNodeIterator> resourceItr;

    // checking if the state ID is legal
    stateID = stateNode->GetAttributeValueAsInt("STATE", -1);
    if(stateID < 0)
    {
        return false;
    }

    // initializing the EntityState
    entityState = states.Get(state, 0);

    if(entityState != 0) // already defined
    {
        return false;
    }
    
    entityState = new EntityState();

    // initializing resources
    resourceItr = stateNode->GetNodes("RESOURCE");
    while(resourceItr->HasNext())
    {
        const char* resourceName = resourceItr->Next()->GetAttributeValue("NAME", 0);
        if(resourceName == 0)
        {
            continue;
        }
        entityState->resources.PushSmart(resourceName);
    }

    // checking if there is at least one resource
    if(entityState->resources.IsEmpty())
    {
        delete entityState;
        return false;
    }

    // setting all the other parameters
    entityState->probability = stateNode->GetAttributeValueAsFloat("PROBABILITY", 1.0);
    entityState->volume = stateNode->GetAttributeValueAsFloat("VOLUME", VOLUME_NORM);
    entityState->delay = stateNode->GetAttributeValueAsInt("DELAY", 0);
    entityState->timeOfDayStart = stateNode->GetAttributeValueAsInt("TIME_START", 0);
    entityState->timeOfDayEnd = stateNode->GetAttributeValueAsInt("TIME_END", 24);
    entityState->fallbackState = stateNode->GetAttributeValueAsInt("FALLBACK_STATE", -1);
    entityState->fallbackProbability = stateNode->GetAttributeValueAsFloat("FALLBACK_PROBABILITY", 1.0);
    entityState->references = 1;

    // adjusting the probability on the update time
    if(entityState->probability < 1.0)
    {
        entityState->probability = entityState->probability / 1000 * SoundManager::updateTime;
    }

    states.Put(stateID, entityState);

    // updating variables that keep track of the extremes values
    if(entityState->timeOfDayStart < minTimeOfDayStart)
    {
        minTimeOfDayStart = entityState->timeOfDayStart;
    }
    if(entityState->timeOfDayEnd > maxTimeOfDayEnd)
    {
        maxTimeOfDayEnd = entityState->timeOfDayEnd;
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
    EntityState* entityState;

    // checking if it is in the undefined state
    if(state < 0)
    {
        return true;
    }

    entityState = states.Get(state, 0);

    // checking time, range and delay
    if(range < minRange || range > maxRange)
    {
        return false;
    }
    else if(time < entityState->timeOfDayStart || entityState->timeOfDayEnd < time)
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

    if(range < minRange || range > maxRange)
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
    EntityState* entityState;

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
    entityState = states.Get(stat, 0);
    if(entityState == 0)
    {
        if(forceChange)
        {
            state = -1; // undefined state
        }

        return;
    }

    state = stat;
}

bool psEntity::Play(SoundControl* &ctrl, csVector3 entityPosition)
{
    EntityState* entityState;

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

    entityState = states.Get(state, 0);

    if(!(entityState->resources.IsEmpty()))
    {
        if(SoundSystemManager::GetSingleton().Play3DSound(entityState->resources[0], DONT_LOOP, 0, 0,
            entityState->volume, ctrl, entityPosition, 0, minRange, maxRange,
            VOLUME_ZERO, CS_SND3D_ABSOLUTE, handle))
        {
            when = entityState->delay * 1000;
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

