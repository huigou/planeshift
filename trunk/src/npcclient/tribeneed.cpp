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



psTribe * psTribeNeed::GetTribe() const 
{
    return parentSet->GetTribe();
}

// ---------------------------------------------------------------------------------

void psTribeNeedSet::UpdateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        needs[i]->UpdateNeed(npc);
    }
}

psTribe::TribeNeed psTribeNeedSet::CalculateNeed(NPC * npc)
{
    for (size_t i=0; i < needs.GetSize()-1; i++)
    {
        for (size_t j=i+1; j < needs.GetSize(); j++)
        {
            if (needs[i]->GetNeed(npc) < needs[j]->GetNeed(npc))
            {
                psTribeNeed *tmp = needs[i];
                needs[i] = needs[j];
                needs[j] = tmp;
            }
        }
    }

    csString log;
    log.Format("Need for %s",npc->GetName());
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        log.AppendFmt("\n%20s %.2f -> %s",needs[i]->name.GetDataSafe(),needs[i]->current_need,psTribe::TribeNeedName[needs[i]->GetNeedType()]);
    }
    Debug2(LOG_TRIBES, GetTribe()->GetID(), "%s", log.GetData());

    needs[0]->ResetNeed();
    return needs[0]->GetNeedType();
}

void psTribeNeedSet::MaxNeed(const csString& needName)
{
    for (size_t i=0; i < needs.GetSize(); i++)
    {
        if (needs[i]->name == needName)
        {
            needs[i]->current_need = 9999.0;
        }
    }    
}
