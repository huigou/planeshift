/*
* tribeneed.cpp
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csutil/csstring.h>

//=============================================================================
// Local Includes
//=============================================================================
#include "tribeneed.h"
#include "tribe.h"
#include "npc.h"

// ---------------------------------------------------------------------------------

const char* TribeNeed::TribeNeedTypeName[] =
{
    "DEATH_RATE",
    "GENERIC",
    "REPRODUCE",
    "RESOURCE_AREA",
    "RESOURCE_RATE",
    "TIME_OF_DAY",
    ""
};

Tribe * TribeNeed::GetTribe() const 
{
    return parentSet->GetTribe();
}

csString TribeNeed::GetTypeAndName() const
{
    csString result;
    
    result = needName + "(" + TribeNeed::TribeNeedTypeName[needType] +")";
    
    return result;
}

// ---------------------------------------------------------------------------------

void TribeNeedSet::UpdateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        needs[i]->UpdateNeed(npc);
    }
}

TribeNeed* TribeNeedSet::CalculateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize()-1; i++)
    {
        for (size_t j=i+1; j < needs.GetSize(); j++)
        {
            if (needs[i]->GetNeedValue(npc) < needs[j]->GetNeedValue(npc))
            {
                TribeNeed *tmp = needs[i];
                needs[i] = needs[j];
                needs[j] = tmp;
            }
        }
    }

    csString log;
    log.Format("Need for %s(%s)",npc->GetName(),ShowID(npc->GetEID()));
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        log.AppendFmt("\n%20s %10.2f -> %s",needs[i]->GetTypeAndName().GetDataSafe(),needs[i]->current_need,needs[i]->GetNeed()->GetTypeAndName().GetDataSafe());
    }
    Debug2(LOG_TRIBES, GetTribe()->GetID(), "%s", log.GetData());

    needs[0]->ResetNeed();
    return needs[0];
}

void TribeNeedSet::MaxNeed(const csString& needName)
{
    TribeNeed * need = Find( needName );
    if (need)
    {
        need->current_need = 9999.0;
    }
}

void TribeNeedSet::AddNeed(TribeNeed * newNeed)
{
    newNeed->SetParent(this);
    newNeed->ResetNeed();
    needs.Push(newNeed);
}

TribeNeed* TribeNeedSet::Find(const csString& needName) const
{
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        if (needs[i]->needName.CompareNoCase(needName))
        {
            return needs[i];
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------------

void TribeNeedGeneric::UpdateNeed(NPC * npc)
{
    current_need += needGrowthValue;
}

// ---------------------------------------------------------------------------------
void TribeNeedResourceArea::UpdateNeed(NPC * npc)
{
    if (!GetTribe()->CanGrow())
    {
        current_need += 10*needGrowthValue; // Make sure tribe can grow at all time
    }
    else
    {
        current_need += needGrowthValue;
    }
}

const TribeNeed* TribeNeedResourceArea::GetNeed() const
{
    if (GetTribe()->FindMemory(GetTribe()->GetNeededResourceAreaType()))
    {
        return this;
    }
    else
    {
        return explore->GetNeed();
    }
}

// ---------------------------------------------------------------------------------
void TribeNeedReproduce::UpdateNeed(NPC * npc)
{
    if (GetTribe()->ShouldGrow())
    {
        current_need += needGrowthValue;
    }
    else
    {
        current_need = 0.0;
    }
}

const TribeNeed* TribeNeedReproduce::GetNeed() const
{
    if (GetTribe()->CanGrow())
    {
        return this;
    }
    else
    {
        return getResourceNeed->GetNeed();
    }
}

// ---------------------------------------------------------------------------------
void TribeNeedResourceRate::UpdateNeed(NPC * npc)
{
    // ResourceRate will be 0.0 until one some resources are found. So check that
    // we have a valid mesurement before comparing to limit.
    if (GetTribe()->GetResourceRate() > 0.0 && GetTribe()->GetResourceRate() < limit)
    {
        current_need += needGrowthValue;
    }
    else
    {
        current_need = 0.0;
    }
}

const TribeNeed* TribeNeedResourceRate::GetNeed() const
{
    return dependendNeed;
}

// ---------------------------------------------------------------------------------
void TribeNeedDeathRate::UpdateNeed(NPC * npc)
{
    // DeathRate will be 0.0 until one tribe member is killed. So check that
    // we have a valid mesurement before comparing to limit.
    if (GetTribe()->GetDeathRate() > 0.0 && GetTribe()->GetDeathRate() < limit)
    {
        current_need += needGrowthValue;
    }
    else
    {
        current_need = 0.0;
    }
}

const TribeNeed* TribeNeedDeathRate::GetNeed() const
{
    return dependendNeed;
}

// ---------------------------------------------------------------------------------
void TribeNeedTimeOfDay::UpdateNeed(NPC * npc)
{
    int gameTODHour = psNPCClient::npcclient->GetGameTODHour();

    // If current hour is less than start hour, add a cycle
    if (gameTODHour < startHour) gameTODHour += 24;

    if (gameTODHour >= startHour && gameTODHour <= endHour)
    {
        current_need += needGrowthValue;
    }
    else
    {
        current_need = 0.0;
    }
    
}

const TribeNeed* TribeNeedTimeOfDay::GetNeed() const
{
    return this;
}

// ---------------------------------------------------------------------------------
