/*
 * psquestprereqopts.cpp
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

#include "pscharacter.h"
#include "psquest.h"
#include "psquestprereqops.h"
#include "rpgrules/factions.h"
#include "../gem.h"

///////////////////////////////////////////////////////////////////////////////////////////

csString psQuestPrereqOp::GetScript()
{
    csString script;
    script.Append("<pre>");
    script.Append(GetScriptOp());
    script.Append("</pre>");
    return script;
}


///////////////////////////////////////////////////////////////////////////////////////////

psQuestPrereqOpList::~psQuestPrereqOpList()
{
    while (prereqlist.GetSize())
        delete prereqlist.Pop();
}


void psQuestPrereqOpList::Push(psQuestPrereqOp* prereqOp)
{
    prereqlist.Push(prereqOp);
}

void psQuestPrereqOpList::Insert(size_t n, psQuestPrereqOp* prereqOp)
{
    prereqlist.Insert(n,prereqOp);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpAnd::Check(psCharacter * character)
{
    // Check if all prereqs are valid
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if (!prereqlist[i]->Check(character)) return false;
    }

    return true;
}

csString psQuestPrereqOpAnd::GetScriptOp()
{
    csString script;
    script.Append("<and>");
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</and>");
    return script;
}

psQuestPrereqOp* psQuestPrereqOpAnd::Copy()
{
    psQuestPrereqOpAnd* copy = new psQuestPrereqOpAnd();
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }    
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpOr::Check(psCharacter * character)
{
    // Check if any of the prereqs are valid
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if (prereqlist[i]->Check(character)) return true;
    }

    return false;
}

csString psQuestPrereqOpOr::GetScriptOp()
{
    csString script;
    script.Append("<or>");
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
    script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</or>");
    return script;
}

psQuestPrereqOp* psQuestPrereqOpOr::Copy()
{
    psQuestPrereqOpOr* copy = new psQuestPrereqOpOr();
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }    
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

psQuestPrereqOpRequire::psQuestPrereqOpRequire(int min_required,int max_required)
{
    min = min_required;
    max = max_required;
}

bool psQuestPrereqOpRequire::Check(psCharacter * character)
{
    // Count the number of prereqs that is valid.
    int count=0;
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if (prereqlist[i]->Check(character)) count++;
    }
    // Verify that the appropiate numbers of prereqs was counted. 
    return ((min == -1 || count >= min) && (max == -1 || count <= max));
}

csString psQuestPrereqOpRequire::GetScriptOp()
{
    csString script;
    script.Append("<require");
    if (min != -1) script.AppendFmt(" min=\"%d\"",min);
    if (max != -1) script.AppendFmt(" max=\"%d\"",max);
    script.Append(">");
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</require>");
    return script;
}

psQuestPrereqOp* psQuestPrereqOpRequire::Copy()
{
    psQuestPrereqOpRequire* copy = new psQuestPrereqOpRequire(min,max);
    for (size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }    
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpNot::Check(psCharacter * character)
{
    return (prereqlist.GetSize() && !prereqlist[0]->Check(character));
}

csString psQuestPrereqOpNot::GetScriptOp()
{
    csString script;
    script.Append("<not>");
    if (prereqlist.GetSize())
        script.Append(prereqlist[0]->GetScriptOp());
    script.Append("</not>");
    return script;
}

psQuestPrereqOp* psQuestPrereqOpNot::Copy()
{
    psQuestPrereqOpNot* copy = new psQuestPrereqOpNot();
    copy->Push(prereqlist[0]->Copy());
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpQuestCompleted::Check(psCharacter * character)
{
    return character->CheckQuestCompleted(quest);
}

csString psQuestPrereqOpQuestCompleted::GetScriptOp()
{
    csString script;
    
    script.AppendFmt("<completed quest=\"%s\"/>",quest->GetName());

    return script;
}

psQuestPrereqOp* psQuestPrereqOpQuestCompleted::Copy()
{
    psQuestPrereqOpQuestCompleted* copy = new psQuestPrereqOpQuestCompleted(quest);
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpQuestAssigned::Check(psCharacter * character)
{
    return character->CheckQuestAssigned(quest);
}

csString psQuestPrereqOpQuestAssigned::GetScriptOp()
{
    csString script;
    
    script.AppendFmt("<assigned quest=\"%s\"/>",quest->GetName());

    return script;
}

psQuestPrereqOp* psQuestPrereqOpQuestAssigned::Copy()
{
    psQuestPrereqOpQuestAssigned* copy = new psQuestPrereqOpQuestAssigned(quest);
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpQuestCompletedCategory::Check(psCharacter * character)
{
    int count = character->NumberOfQuestsCompleted(category);

    Debug5(LOG_QUESTS,character->GetCharacterID(),"Check for category %s in range %d <= %d <= %d",
           category.GetDataSafe(),min,count,max);

    // Verify that the appropiate numbers of quest of given category is done.
    return ((min == -1 || min <= count) && (max == -1 || count <= max));
}

csString psQuestPrereqOpQuestCompletedCategory::GetScriptOp()
{
    csString script;
    
    script.AppendFmt("<completed category=\"%s\"",category.GetDataSafe());
    if (min != -1) script.AppendFmt(" min=\"%d\"",min);
    if (max != -1) script.AppendFmt(" max=\"%d\"",max);
    script.Append("/>");

    return script;
}

psQuestPrereqOp* psQuestPrereqOpQuestCompletedCategory::Copy()
{
    psQuestPrereqOpQuestCompletedCategory* copy = 
        new psQuestPrereqOpQuestCompletedCategory(category,min,max);
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpFaction::Check(psCharacter * character)
{
    if(max) // If value is max, make sure we're below it
        return !character->CheckFaction(faction,value);
    return character->CheckFaction(faction,value);
}

csString psQuestPrereqOpFaction::GetScriptOp()
{
    csString script;
    
    script.AppendFmt("<faction name=\"%s\" value=\"%d\" max=\"%d\"/>",faction->name.GetData(),value,max);

    return script;
}

psQuestPrereqOp* psQuestPrereqOpFaction::Copy()
{
    psQuestPrereqOpFaction* copy = new psQuestPrereqOpFaction(faction,value,max);
    return copy;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpActiveMagic::Check(psCharacter * character)
{
    if (character->GetActor())
        return character->GetActor()->IsMagicCategoryActive(activeMagic);
    return false;
}

csString psQuestPrereqOpActiveMagic::GetScriptOp()
{
    csString script;
    
    script.Format("<activemagic name=\"%s\"/>", activeMagic.GetData());

    return script;
}

psQuestPrereqOp* psQuestPrereqOpActiveMagic::Copy()
{
    psQuestPrereqOpActiveMagic* copy = new psQuestPrereqOpActiveMagic(activeMagic);
    return copy;
}

