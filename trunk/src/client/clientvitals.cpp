/*
 * clientvitals.cpp
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
#include "clientvitals.h"
#include "globals.h"
#include "net/msghandler.h"
#include "gui/chatwindow.h"
#include "pscharcontrol.h"

psClientVitals::psClientVitals()
{
    counter = 0;
    counter_set = false;
}

void psClientVitals::HandleDRData(psStatDRMessage& msg, const char *labelname )
{
    char buff[100];

    // Skip out of date stat dr updates
    if (counter_set && (unsigned char)(msg.counter - counter) > 127)
    {
        Error4("Skipping out of date StatDR packet for '%s', version %d not %d.", labelname, msg.counter, counter);
        return;
    }
    else
    {
        counter = msg.counter;  // update for next time.
        counter_set = true;     // accept the first counter and drop anything out of date compared to that
    }

    if (msg.statsDirty & DIRTY_VITAL_HP)       
    {
        vitals[VITAL_HITPOINTS].value = msg.hp;
        sprintf(buff,"fVitalValue%d:%s",VITAL_HITPOINTS,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].value);
    }
        
    if (msg.statsDirty & DIRTY_VITAL_HP_RATE)
    {
        vitals[VITAL_HITPOINTS].drRate = msg.hp_rate;
        sprintf(buff,"fVitalRate%d:%s",VITAL_HITPOINTS,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].drRate);
    }
        
    if (msg.statsDirty & DIRTY_VITAL_MANA)
    {
        vitals[VITAL_MANA].value = msg.mana;
        sprintf(buff,"fVitalValue%d:%s",VITAL_MANA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MANA].value);
    }
        
    if (msg.statsDirty & DIRTY_VITAL_MANA_RATE)
    {
        vitals[VITAL_MANA].drRate = msg.mana_rate;
        sprintf(buff,"fVitalRate%d:%s",VITAL_MANA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MANA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_PYSSTAMINA)
    {
        vitals[VITAL_PYSSTAMINA].value = msg.pstam;
        sprintf(buff,"fVitalValue%d:%s",VITAL_PYSSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_PYSSTAMINA].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
    {
        vitals[VITAL_PYSSTAMINA].drRate = msg.pstam_rate;
        sprintf(buff,"fVitalRate%d:%s",VITAL_PYSSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_PYSSTAMINA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_MENSTAMINA)
    {
        vitals[VITAL_MENSTAMINA].value = msg.mstam;
        sprintf(buff,"fVitalValue%d:%s",VITAL_MENSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MENSTAMINA].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
    {
        vitals[VITAL_MENSTAMINA].drRate = msg.mstam_rate;
        sprintf(buff,"fVitalRate%d:%s",VITAL_MENSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MENSTAMINA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_EXPERIENCE)
    {
        experiencePoints = (int)msg.exp;
        sprintf(buff,"fExpPts:%s",labelname);
        PawsManager::GetSingleton().Publish(buff,(float)experiencePoints/200.0F);
    }
        
    if (msg.statsDirty & DIRTY_VITAL_PROGRESSION)
    {
        progressionPoints = (int)msg.prog;
        sprintf(buff,"fProgrPts:%s",labelname);
        PawsManager::GetSingleton().Publish(buff,progressionPoints);
    }
}

void psClientVitals::HandleDeath(const char *labelname )
{
    char buff[100];

    vitals[VITAL_HITPOINTS].drRate = 0.0;
    sprintf(buff,"fVitalRate%d:%s",VITAL_HITPOINTS,labelname);
    PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].drRate);

    vitals[VITAL_HITPOINTS].value = 0.0;
    sprintf(buff,"fVitalValue%d:%s",VITAL_HITPOINTS,labelname);
    PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].value);
}

void psClientVitals::Predict( csTicks now, const char *labelname )
{
    // Find delta time since last update
    float delta = (now-lastDRUpdate)/1000.0;
    lastDRUpdate = now;

    char buff[100];

    // iterate over all fields and predict their values based on their recharge rate
    for ( int x = 0; x < VITAL_COUNT; x++ )
    {
        float oldvalue = vitals[x].value;
        vitals[x].value += vitals[x].drRate*delta;
        if ( vitals[x].value< 0 )
            vitals[x].value = 0;
            
        if ( vitals[x].value > 1.0 )
            vitals[x].value = 1.0;

        if (oldvalue != vitals[x].value) // need to republish info
        {
            sprintf(buff,"fVitalValue%d:%s",x,labelname);
            PawsManager::GetSingleton().Publish(buff,vitals[x].value);
        }
    }                
}
