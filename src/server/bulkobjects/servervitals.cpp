/*
 * servervitals.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/msghandler.h"
#include "net/netbase.h"

#include "../gem.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "servervitals.h"
#include "pscharacter.h"

psServerVitals::psServerVitals(psCharacter * character)
{
    this->character = character;
    statsDirty = 0;
    version    = 0;
}

bool psServerVitals::SendStatDRMessage(uint32_t clientnum, PS_ID eid, int flags, csRef<PlayerGroup> group)
{
    bool backup=0;
    if (flags)
    {
        backup = statsDirty ? true : false;
        statsDirty = flags;
    }
    else if (version % 10 == 0)  // every 10th msg to this person, send everything
        statsDirty = DIRTY_VITAL_ALL;

    if (!statsDirty) 
        return false;

    csArray<float> fVitals;
    csArray<int32_t> iVitals;

    if (statsDirty & DIRTY_VITAL_HP)
        fVitals.Push(vitals[VITAL_HITPOINTS].value / vitals[VITAL_HITPOINTS].max);

    if (statsDirty & DIRTY_VITAL_HP_RATE)
        fVitals.Push(vitals[VITAL_HITPOINTS].drRate / vitals[VITAL_HITPOINTS].max);

    if (statsDirty & DIRTY_VITAL_MANA)
        fVitals.Push(vitals[VITAL_MANA].value / vitals[VITAL_MANA].max);

    if (statsDirty & DIRTY_VITAL_MANA_RATE)
        fVitals.Push(vitals[VITAL_MANA].drRate / vitals[VITAL_MANA].max);

    // Pyshical Stamina
    if (statsDirty & DIRTY_VITAL_PYSSTAMINA)
        fVitals.Push(vitals[VITAL_PYSSTAMINA].value / vitals[VITAL_PYSSTAMINA].max);

    if (statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
        fVitals.Push(vitals[VITAL_PYSSTAMINA].drRate / vitals[VITAL_PYSSTAMINA].max);

    // Mental Stamina
    if (statsDirty & DIRTY_VITAL_MENSTAMINA)
        fVitals.Push(vitals[VITAL_MENSTAMINA].value / vitals[VITAL_MENSTAMINA].max);

    if (statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
        fVitals.Push(vitals[VITAL_MENSTAMINA].drRate / vitals[VITAL_MENSTAMINA].max);

    if (statsDirty & DIRTY_VITAL_EXPERIENCE)
        iVitals.Push((int32_t) GetExp());

    if (statsDirty & DIRTY_VITAL_PROGRESSION)
        iVitals.Push((int32_t) GetPP());

    psStatDRMessage msg(clientnum, eid, fVitals, iVitals, ++version, statsDirty);
    if (group == NULL)
        msg.SendMessage();
    else
        group->Broadcast(msg.msg);

    statsDirty = backup;
    return true;
}

bool psServerVitals::Update( csTicks now )
{
    csTicks drdelta = now - lastDRUpdate;
    float delta;
    /* It is necessary to check when lastDRUpdate is 0 because, if not when a character login his stats
    are significantly incremented, which is instead unnecessary. As tested, delta cannot be 0 or the actors
    with 0 HP and negative drRate will enter a loop of death, but it has to be really small in order to guarantee
    a smooth increment of the stats. Since, now can be quite a random value when lastDRUpdate is 0, it is important
    to give an high value to drdelta, so we can force a stats update.*/
    if(!lastDRUpdate) 
    {
        delta=0.1f; //The value can be adjusted but it has to be really small.
        drdelta= 11000; //drdelta must be really high, in order to avoid randomness and guarantee stats update
    }
    else
       delta= (now-lastDRUpdate)/1000.0;
    lastDRUpdate = now;

    // iterate over all fields and predict their values based on their recharge rate
    for ( int x = 0; x < VITAL_COUNT; x++ )
    {
        vitals[x].value += vitals[x].drRate*delta;            
        
        if ( vitals[x].value < 0 )
            vitals[x].value = 0;
        if ( vitals[x].value > vitals[x].max )
            vitals[x].value = vitals[x].max;                
    }        

    if (vitals[VITAL_HITPOINTS].value==0  &&  vitals[VITAL_HITPOINTS].drRate<0)
        character->GetActor()->Kill(NULL);
    
    if (drdelta > 10000)
        statsDirty = QUERY_FAILED;

    return (statsDirty) ? true : false;
}


void psServerVitals::SetExp( int W )
{
    experiencePoints = W;
    statsDirty |= DIRTY_VITAL_EXPERIENCE;
}

void psServerVitals::SetPP( int pp )
{
    progressionPoints = pp;
    statsDirty |= DIRTY_VITAL_PROGRESSION;
}

psCharVital& psServerVitals::DirtyVital( int vitalName, int dirtyFlag )
{
    statsDirty |= dirtyFlag;
    return GetVital( vitalName );    
}

