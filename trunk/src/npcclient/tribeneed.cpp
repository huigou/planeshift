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
#include "npc.h"

// ---------------------------------------------------------------------------------

psTribe * psTribeNeed::GetTribe() const 
{
    return parentSet->GetTribe();
}

csString psTribeNeed::GetTypeAndName() const
{
    csString result;
    
    result = needName + "(" + psTribe::TribeNeedTypeName[needType] +")";
    
    return result;
}

// ---------------------------------------------------------------------------------

void psTribeNeedSet::UpdateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        needs[i]->UpdateNeed(npc);
    }
}

psTribeNeed* psTribeNeedSet::CalculateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize()-1; i++)
    {
        for (size_t j=i+1; j < needs.GetSize(); j++)
        {
            if (needs[i]->GetNeedValue(npc) < needs[j]->GetNeedValue(npc))
            {
                psTribeNeed *tmp = needs[i];
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

void psTribeNeedSet::MaxNeed(const csString& needName)
{
    psTribeNeed * need = Find( needName );
    if (need)
    {
        need->current_need = 9999.0;
    }
}

void psTribeNeedSet::AddNeed(psTribeNeed * newNeed)
{
    newNeed->SetParent(this);
    newNeed->ResetNeed();
    needs.Push(newNeed);
}

psTribeNeed* psTribeNeedSet::Find(const csString& needName) const
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
