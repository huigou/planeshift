/*
* perceptions.cpp by Keith Fulton <keith@paqrat.com>
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csgeom/transfrm.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

#include "util/log.h"
#include "util/location.h"
#include "util/strutil.h"
#include "util/psutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcbehave.h"
#include "perceptions.h"
#include "npc.h"
#include "npcclient.h"
#include "networkmgr.h"
#include "globals.h"
#include "gem.h"



Reaction::Reaction()
{
    desireValue   = 0.0f;
    desireType    = DESIRE_GUARANTIED;
    range         = 0;
    activeOnly    = false;
    inactiveOnly  = false;
}

bool Reaction::Load(iDocumentNode *node,BehaviorSet& behaviors)
{
    // Default to guarantied
    desireType = DESIRE_GUARANTIED;
    desireValue = 0.0f;
    
    if (node->GetAttribute("delta"))
    {
        desireValue = node->GetAttributeValueAsFloat("delta");
        desireType = DESIRE_DELTA;

        if (fabs(desireValue)<SMALL_EPSILON)  // 0 means no change
        {
            desireType = DESIRE_NONE;
        }
    }

    if (node->GetAttribute("absolute"))
    {
        if (desireType == DESIRE_NONE || desireType == DESIRE_DELTA)
        {
            Error1("Reaction can't be both absolute and delta reaction");
            return false;
        }
        desireValue = node->GetAttributeValueAsFloat("absolute");
        desireType = DESIRE_ABSOLUTE;
    }

    // Handle hooking up to the right behavior
    csString name = node->GetAttributeValue("behavior");
    csArray<csString> names = psSplit(name,',');
    for (size_t i = 0; i < names.GetSize(); i++)
    {
        Behavior * behavior = behaviors.Find(names[i]);
        if (!behavior)
        {
            Error3("Reaction for event type '%s' specified unknown behavior of '%s'. Error in XML.",GetEventType(NULL).GetDataSafe(),(const char *)names[i]);
            return false;
        }
        affected.Push(behavior);
    }

    // Handle hooking up to the perception
    eventType              = node->GetAttributeValue("event");
    if (eventType.IsEmpty())
    {
        Error2("No event type specefied for node: %s",node->GetValue());
        return false;
    }
    
    range                  = node->GetAttributeValueAsFloat("range");
    weight                 = node->GetAttributeValueAsFloat("weight");
    factionDiff            = node->GetAttributeValueAsInt("faction_diff");
    oper                   = node->GetAttributeValue("oper");

    // Decode the value field, It is in the form value="1,2,,4"
    csString valueAttr = node->GetAttributeValue("value");
    csArray<csString> valueStr = psSplit( valueAttr , ',');
    for (size_t ii=0; ii < valueStr.GetSize(); ii++)
    {
        if (!valueStr[ii].IsEmpty())
        {
            values.Push(atoi(valueStr[ii]));
            valuesValid.Push(true);
        }
        else
        {
            values.Push(0);
            valuesValid.Push(false);
        }
    }
    // Decode the random field, It is in the form random="1,2,,4"
    csString randomAttr = node->GetAttributeValue("random");
    csArray<csString> randomStr = psSplit( randomAttr, ',');
    for (size_t ii=0; ii < randomStr.GetSize(); ii++)
    {
        if (!randomStr[ii].IsEmpty())
        {
            randoms.Push(atoi(randomStr[ii]));
            randomsValid.Push(true);
        }
        else
        {
            randoms.Push(0);
            randomsValid.Push(false);
        }
    }

    type                   = node->GetAttributeValue("type");
    activeOnly             = node->GetAttributeValueAsBool("active_only");
    inactiveOnly           = node->GetAttributeValueAsBool("inactive_only");
    reactWhenDead          = node->GetAttributeValueAsBool("when_dead",false);
    reactWhenInvisible     = node->GetAttributeValueAsBool("when_invisible",false);
    reactWhenInvincible    = node->GetAttributeValueAsBool("when_invincible",false);
    csString tmp           = node->GetAttributeValue("only_interrupt");
    if (tmp.Length())
    {
        onlyInterrupt = psSplit(tmp,',');
    }
    tmp                    = node->GetAttributeValue("do_not_interrupt");
    if (tmp.Length())
    {
        doNotInterrupt = psSplit(tmp,',');
    }

    return true;
}

void Reaction::DeepCopy(Reaction& other,BehaviorSet& behaviors)
{
    desireValue            = other.desireValue;
    desireType             = other.desireType;
    for (size_t i = 0; i < other.affected.GetSize(); i++)
    {
        Behavior * behavior = behaviors.Find(other.affected[i]->GetName());
        affected.Push(behavior);
    }
    eventType              = other.eventType;
    range                  = other.range;
    factionDiff            = other.factionDiff;
    oper                   = other.oper;
    weight                 = other.weight;
    values                 = other.values;
    valuesValid            = other.valuesValid;
    randoms                = other.randoms;
    randomsValid           = other.randomsValid;
    type                   = other.type;
    activeOnly             = other.activeOnly;
    inactiveOnly           = other.inactiveOnly;
    reactWhenDead          = other.reactWhenDead;
    reactWhenInvisible     = other.reactWhenInvisible;
    reactWhenInvincible    = other.reactWhenInvincible;
    onlyInterrupt          = other.onlyInterrupt;
    doNotInterrupt         = other.doNotInterrupt;

    // For now depend on that each npc do a deep copy to create its instance of the reaction
    for (uint ii=0; ii < values.GetSize(); ii++)
    {
        if (GetRandomValid((int)ii))
        {
            values[ii] += psGetRandom(GetRandom((int)ii));
        }
    }

    // Some special cases to handle
    if (eventType == "time")
    {
        TimePerception::NormalizeReaction(this);
    }
}

void Reaction::React(NPC *who, Perception *pcpt)
{
    CS_ASSERT(who);

    // Check if the perception is a match for this reaction
    if (!pcpt->ShouldReact(this,who))
    {
        if (who->IsDebugging(20))
        {
            who->Printf(20, "Reaction '%s' skipping perception %s", GetEventType(who).GetDataSafe(), pcpt->ToString(who).GetDataSafe());
        }
        return;
    }

    // If dead we should not react unless reactWhenDead is set
    if (!(who->IsAlive() || reactWhenDead))
    {
        who->Printf(5, "Only react to '%s' when alive", GetEventType(who).GetDataSafe());
        return;
    }

    // Check if the active behavior should not be interrupted.
    if (who->GetCurrentBehavior() && DoNotInterrupt(who->GetCurrentBehavior()))
    {
        who->Printf(5,"Prevented from reacting to '%s' while not interrupt behavior '%s' is active",
                    GetEventType(who).GetDataSafe(),who->GetCurrentBehavior()->GetName());
        return; 
    } 

    // Check if this reaction is limited to only interrupt some given behaviors.
    if (who->GetCurrentBehavior() && OnlyInterrupt(who->GetCurrentBehavior()))
    {
        who->Printf(5,"Prevented from reacting to '%s' since behavior '%s' should not be interrupted",
                    GetEventType(who).GetDataSafe(),who->GetCurrentBehavior()->GetName());
        return; 
    } 



    // We should no react and triggerd all affected behaviors

    // For debug get the time this reaction was triggered
    GetTimeOfDay(lastTriggered);


    // Adjust the needs for the triggered behaviors
    for (size_t i = 0; i < affected.GetSize(); i++)
    {

        // When activeOnly flag is set we should do nothing
        // if the affected behaviour is inactive.
        if (activeOnly && !affected[i]->IsActive() )
            break;
        
        // When inactiveOnly flag is set we should do nothing
        // if the affected behaviour is active.
        if (inactiveOnly && affected[i]->IsActive() )
            break;


        who->Printf(2, "Reaction '%s[%s]' reacting to perception: %s", GetEventType(who).GetDataSafe(), type.GetDataSafe(), pcpt->ToString(who).GetDataSafe());
        switch (desireType)
        {
        case DESIRE_NONE:
            who->Printf(10, "No change to need for behavior %s.", affected[i]->GetName());
            break;
        case DESIRE_ABSOLUTE:
            who->Printf(10, "Setting %1.1f need to behavior %s", desireValue, affected[i]->GetName());
            affected[i]->ApplyNeedAbsolute(who, desireValue);
            break;
        case DESIRE_DELTA:
            who->Printf(10, "Adding %1.1f need to behavior %s", desireValue, affected[i]->GetName());
            affected[i]->ApplyNeedDelta(who, desireValue);
            break;
        case DESIRE_GUARANTIED:
            who->Printf(10, "Guarantied need to behavior %s", affected[i]->GetName());
            
            float highest = who->GetBrain()->GetHighestNeed(who);
            if (who->GetCurrentBehavior() != affected[i])
            {
                affected[i]->ApplyNeedAbsolute(who, highest + 25);
                affected[i]->SetCompletionDecay(-1);
            }
            break;
        }
    }
    
    // Execute the perception
    pcpt->ExecutePerception(who,weight);

    Perception *p = pcpt->MakeCopy();
    who->SetLastPerception(p);

}

bool Reaction::ShouldReact(gemNPCObject* actor )
{
    if (!actor) return false;

    if (!(!actor->IsInvisible() || reactWhenInvisible))
    {
        return false;
    }
    
    if (!(!actor->IsInvincible() || reactWhenInvincible))
    {
        return false;
    }

    return true;
}

bool Reaction::DoNotInterrupt(Behavior* behavior)
{
    if (doNotInterrupt.GetSize())
    {
        for (size_t i = 0; i < doNotInterrupt.GetSize(); i++)
        {
            if (doNotInterrupt[i].CompareNoCase(behavior->GetName()))
            {
                return true;
            }
        }
    }
    return false;
}

bool Reaction::OnlyInterrupt(Behavior* behavior)
{
    if (onlyInterrupt.GetSize())
    {
        for (size_t i = 0; i < onlyInterrupt.GetSize(); i++)
        {
            if (onlyInterrupt[i].CompareNoCase(behavior->GetName()))
            {
                return false; // The behavior is legal to interrupt
            }
        }
        return true; // The behavior isn't on the list of behaviors to interrupt
    }
    return false; // There are no limitation on who to interrupt
}

csString Reaction::GetEventType(NPC* npc)
{
    // If npc is given subsitute variables
    if (npc)
    {
        csString result = psGameObject::ReplaceNPCVariables(npc, eventType );
        return result;
    }
    
    // Return the raw eventType if no NPC is given.
    return eventType;
}


int Reaction::GetValue(int i)
{
    if (i < (int)values.GetSize())
    {
        return values[i];
        
    }
    return 0;
}

bool Reaction::GetValueValid(int i)
{
    if (i < (int)valuesValid.GetSize())
    {
        return valuesValid[i];
        
    }
    return false;
}

int Reaction::GetRandom(int i)
{
    if (i < (int)randoms.GetSize())
    {
        return randoms[i];
        
    }
    return 0;
}

bool Reaction::GetRandomValid(int i)
{
    if (i < (int)randomsValid.GetSize())
    {
        return randomsValid[i];
        
    }
    return false;
}

bool Reaction::SetValue(int i, int value)
{
    if (i < (int)values.GetSize())
    {
        values[i] = value;
        return true;
    }
    return false;
}

char Reaction::GetOp()
{
    if (oper.Length())
    {
        return oper.GetAt(0);
    }
    else
    {
        return 0;
    }
}


csString Reaction::GetValue()
{
    csString result;
    for (int i = 0; i < (int)valuesValid.GetSize(); i++)
    {
        if (i != 0)
        {
            result.Append(",");
        }
        
        if (valuesValid[i])
        {
            result.AppendFmt("%d",values[i]);
        }
    }
    return result;
}

csString Reaction::GetAffectedBehaviors()
{
    csString result;
    for (size_t i = 0; i < affected.GetSize(); i++)
    {
        if (i != 0)
        {
            result.Append(", ");
        }
        result.Append(affected[i]->GetName());
    }
    if (!doNotInterrupt.IsEmpty())
    {
        result.Append(" No Int: ");
        for (size_t i = 0; i < doNotInterrupt.GetSize(); i++)
        {
            if (i != 0)
            {
                result.Append(", ");
            }
            result.Append(doNotInterrupt[i].GetDataSafe());
        }
    }
    if (!onlyInterrupt.IsEmpty())
    {
        result.Append(" Only Int: ");
        for (size_t i = 0; i < onlyInterrupt.GetSize(); i++)
        {
            if (i != 0)
            {
                result.Append(", ");
            }
            result.Append(onlyInterrupt[i].GetDataSafe());
        }
    }

    return result;
}



/*----------------------------------------------------------------------------*/

bool Perception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if ((GetName(npc) == reaction->GetEventType(npc)) &&
        (reaction->GetType().IsEmpty() || type == reaction->GetType()))
    {
        return true;
    }
    return false;
}

Perception *Perception::MakeCopy()
{
    Perception *p = new Perception(name,type);
    return p;
}

csString Perception::GetName(NPC* /* npc */)
{
    return name;
}


csString Perception::ToString(NPC* npc)
{
    csString result;
    result.Format("Name: '%s[%s]'",GetName(npc).GetDataSafe(), type.GetDataSafe());
    return result;
}


/*----------------------------------------------------------------------------*/

bool RangePerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if (name == reaction->GetEventType(npc) && range < reaction->GetRange())
    {
        return true;
    }
    else
    {
        return false;
    }
}

Perception *RangePerception::MakeCopy()
{
    RangePerception *p = new RangePerception(name,range);
    return p;
}

csString RangePerception::ToString(NPC* npc)
{
    csString result;
    result.Format("Name: '%s' Range: '%.2f'",GetName(npc).GetDataSafe(), range );
    return result;
}

//---------------------------------------------------------------------------------


bool FactionPerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if (name == reaction->GetEventType(npc))
    {
        if (player)
        {
            if (!reaction->ShouldReact(player))
            {
                return false;
            }
        }
        
        if (reaction->GetOp() == '>' )
        {
            npc->Printf(15, "Checking %d > %d.",factionDelta,reaction->GetFactionDiff() );
            if (factionDelta > reaction->GetFactionDiff() )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (reaction->GetOp() == '<' )
        {
            npc->Printf(15, "Checking %d < %d.",factionDelta,reaction->GetFactionDiff() );
            if (factionDelta < reaction->GetFactionDiff() )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            npc->Printf(15, "Skipping faction check.");            
            return true;
        }
        
    }
    return false;
}

bool FactionPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (player)
    {
        psGameObject::GetPosition(player,pos,sector);
        return true;
    }
    return false;
}

Perception *FactionPerception::MakeCopy()
{
    FactionPerception *p = new FactionPerception(name,factionDelta,player);
    return p;
}

void FactionPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)player,weight*-factionDelta);
}

//---------------------------------------------------------------------------------


bool ItemPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (item)
    {
        psGameObject::GetPosition(item,pos,sector);
        return true;
    }
    return false;
}

Perception *ItemPerception::MakeCopy()
{
    ItemPerception *p = new ItemPerception(name,item);
    return p;
}

//---------------------------------------------------------------------------------


bool LocationPerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if (name == reaction->GetEventType(npc) && (reaction->GetType().IsEmpty() || type == reaction->GetType()))
    {
        return true;
    }
    return false;
}

bool LocationPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (location)
    {
        pos = location->pos;
        sector = location->GetSector(engine);
        return true;
    }
    return false;
}

Perception *LocationPerception::MakeCopy()
{
    LocationPerception *p = new LocationPerception(name,type,location, engine);
    return p;
}

float LocationPerception::GetRadius() const
{
    return location->radius; 
}

//---------------------------------------------------------------------------------


bool PositionPerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if (name == reaction->GetEventType(npc) && (reaction->GetType().IsEmpty() || type == reaction->GetType()))
    {
        return true;
    }
    return false;
}

bool PositionPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    pos = this->pos;
    sector = this->sector;
    return true;
}

Perception *PositionPerception::MakeCopy()
{
    PositionPerception *p = new PositionPerception(name,type,instance,sector,pos,yrot,radius);
    return p;
}

float PositionPerception::GetRadius() const
{
    return radius; 
}

//---------------------------------------------------------------------------------

Perception *AttackPerception::MakeCopy()
{
    AttackPerception *p = new AttackPerception(name,attacker);
    return p;
}

void AttackPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)attacker,weight);
}
//---------------------------------------------------------------------------------

Perception *GroupAttackPerception::MakeCopy()
{
    GroupAttackPerception *p = new GroupAttackPerception(name,attacker_ents,bestSkillSlots);
    return p;
}

void GroupAttackPerception::ExecutePerception(NPC *npc,float weight)
{
    for(size_t i=0;i<attacker_ents.GetSize();i++)
        npc->AddToHateList((gemNPCActor*)attacker_ents[i],bestSkillSlots[i]*weight);
}

//---------------------------------------------------------------------------------

Perception *DamagePerception::MakeCopy()
{
    DamagePerception *p = new DamagePerception(name,attacker,damage);
    return p;
}

void DamagePerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)attacker,damage*weight);
}

//---------------------------------------------------------------------------------

SpellPerception::SpellPerception(const char *name,
                                 gemNPCObject *caster, gemNPCObject *target, 
                                 const char *type, float severity)
                                 : Perception(name)
{
    this->caster = (gemNPCActor*) caster;
    this->target = (gemNPCActor*) target;
    this->spell_severity = severity;
    this->type = type;
}

csString SpellPerception::GetName(NPC* npc)
{
    csString event(name);
    event.Append(':');

    if (npc->GetEntityHate((gemNPCActor*)caster) || npc->GetEntityHate((gemNPCActor*)target))
    {
        event.Append("target");
    }
    else if (target == npc->GetActor())
    {
        event.Append("self");
    }
    else
    {
        event.Append("unknown");
    }

    return event;
}

bool SpellPerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    csString event(name);
    event.Append(':');

    if (npc->GetEntityHate((gemNPCActor*)caster) || npc->GetEntityHate((gemNPCActor*)target))
    {
        event.Append("target");
    }
    else if (target == npc->GetActor())
    {
        event.Append("self");
    }
    else
    {
        event.Append("unknown");
    }

    csString eventName = GetName(npc);

    npc->Printf(20,"Spell percpetion checking for match beween %s and %s", eventName.GetData(), reaction->GetEventType(npc).GetDataSafe());

    if (eventName == reaction->GetEventType(npc))
    {
        npc->Printf(15, "%s spell cast by %s on %s, severity %1.1f.",
            eventName.GetData(), (caster)?caster->GetName():"(Null caster)", (target)?target->GetName():"(Null target)", spell_severity);

        return true;
    }
    return false;
}

Perception *SpellPerception::MakeCopy()
{
    SpellPerception *p = new SpellPerception(name,caster,target,type,spell_severity);
    return p;
}

bool SpellPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (caster)
    {
        psGameObject::GetPosition(caster,pos,sector);
        return true;
    }
    return false;
}

void SpellPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)caster,spell_severity*weight);
}

//---------------------------------------------------------------------------------

bool TimePerception::ShouldReact(Reaction *reaction, NPC *npc)
{
    if (name == reaction->GetEventType(npc) )
    {
        if (npc->IsDebugging(15))
        {
            csString dbgOut;
            dbgOut.AppendFmt("Time is now %d:%02d %d-%d-%d and I need ",
                          gameHour,gameMinute,gameYear,gameMonth,gameDay);
            // Hours
            if (reaction->GetValueValid(0))
            {
                dbgOut.AppendFmt("%d:",reaction->GetValue(0));
            }
            else
            {
                dbgOut.Append("*:");
            }
            // Minutes
            if (reaction->GetValueValid(1))
            {
                dbgOut.AppendFmt("%02d ",reaction->GetValue(1));
            }
            else
            {
                dbgOut.Append("* ");
            }
            // Year
            if (reaction->GetValueValid(2))
            {
                dbgOut.AppendFmt("%d-",reaction->GetValue(2));
            }
            else
            {
                dbgOut.Append("*-");
            }
            // Month
            if (reaction->GetValueValid(3))
            {
                dbgOut.AppendFmt("%d-",reaction->GetValue(3));
            }
            else
            {
                dbgOut.Append("*-");
            }
            // Day
            if (reaction->GetValueValid(4))
            {
                dbgOut.AppendFmt("%d",reaction->GetValue(4));
            }
            else
            {
                dbgOut.Append("*");
            }

            npc->Printf(15,dbgOut);
        }
        
        if ((!reaction->GetValueValid(0) || reaction->GetValue(0) == gameHour) &&
            (!reaction->GetValueValid(1) || reaction->GetValue(1) == gameMinute) &&
            (!reaction->GetValueValid(2) || reaction->GetValue(2) == gameYear) &&
            (!reaction->GetValueValid(3) || reaction->GetValue(3) == gameMonth) &&
            (!reaction->GetValueValid(4) || reaction->GetValue(4) == gameDay))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

Perception *TimePerception::MakeCopy()
{
    TimePerception *p = new TimePerception(gameHour,gameMinute,gameYear,gameMonth,gameDay);
    return p;
}

csString TimePerception::ToString(NPC* npc)
{
    csString result;
    result.Format("Name: '%s' : '%d:%02d %d-%d-%d'",GetName(npc).GetDataSafe(),
                  gameHour, gameMinute, gameYear, gameMonth, gameDay );
    return result;
}

void TimePerception::NormalizeReaction(Reaction* reaction)
{
    // Minutes
    if (reaction->GetValueValid(1))
    {
        int minutes = reaction->GetValue(1)%60;
        int hours = reaction->GetValue(1)/60;
        reaction->SetValue(1,minutes);
        if (reaction->GetValueValid(0))
	{
	    reaction->SetValue(0,reaction->GetValue(0)+hours);
	}
    }
    // Hours
    if (reaction->GetValueValid(0))
    {
        int hours = reaction->GetValue(0)%24;
        int days = reaction->GetValue(0)/24;
        reaction->SetValue(0,hours);
        if (reaction->GetValueValid(2))
	{
	    reaction->SetValue(2,reaction->GetValue(2)+days);
	}
    }
    // Day
    if (reaction->GetValueValid(2))
    {
        int days = reaction->GetValue(2)%32;
        int months = reaction->GetValue(2)/32;
        reaction->SetValue(2,days);
        if (reaction->GetValueValid(3))
	{
	    reaction->SetValue(3,reaction->GetValue(3)+months);
	}
    }
    // Month
    if (reaction->GetValueValid(3))
    {
        int months = reaction->GetValue(3)%10;
        int years = reaction->GetValue(3)/10;
        reaction->SetValue(3,months);
        if (reaction->GetValueValid(4))
	{
	    reaction->SetValue(4,reaction->GetValue(4)+years);
	}
    }
}

//---------------------------------------------------------------------------------

Perception *DeathPerception::MakeCopy()
{
    DeathPerception *p = new DeathPerception(who);
    return p;
}

void DeathPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->RemoveFromHateList(who);
}

//---------------------------------------------------------------------------------
Perception *InventoryPerception::MakeCopy()
{
    InventoryPerception *p = new InventoryPerception(name,type,count,pos,sector,radius);
    return p;
}


/// NPC Pet Perceptions ===========================================================
//---------------------------------------------------------------------------------

OwnerCmdPerception::OwnerCmdPerception( const char *name,
                                        psPETCommandMessage::PetCommand_t command,
                                        gemNPCObject *owner,
                                        gemNPCObject *pet,
                                        gemNPCObject *target ) : Perception(BuildName(command))
{
    this->command = command;
    this->owner = owner;
    this->pet = pet;
    this->target = (gemNPCActor *) target;
}

bool OwnerCmdPerception::ShouldReact( Reaction *reaction, NPC *npc )
{
    if (name == reaction->GetEventType(npc))
    {
        return true;
    }
    return false;
}

Perception *OwnerCmdPerception::MakeCopy()
{
    OwnerCmdPerception *p = new OwnerCmdPerception( name, command, owner, pet, target );
    return p;
}

void OwnerCmdPerception::ExecutePerception( NPC *pet, float weight )
{
    gemNPCObject * t = NULL;
    if (target)
    {
        t = npcclient->FindEntityID(target->GetEID());
    }
    pet->SetTarget(t);
    
    switch ( this->command )
    {
    case psPETCommandMessage::CMD_FOLLOW : // Follow
        break;
    case psPETCommandMessage::CMD_STAY : // Stay
        break;
    case psPETCommandMessage::CMD_GUARD : // Guard
        break;
    case psPETCommandMessage::CMD_ASSIST : // Assist
        break;
    case psPETCommandMessage::CMD_NAME : // Name
        break; 
    case psPETCommandMessage::CMD_TARGET : // Target
        break;
    case psPETCommandMessage::CMD_SUMMON : // Summon
        break;
    case psPETCommandMessage::CMD_DISMISS : // Dismiss
        break;
    case psPETCommandMessage::CMD_ATTACK : // Attack
        if (pet->GetTarget())
        {
            pet->AddToHateList((gemNPCActor*)target, 1 * weight );
        }
        else
        {
            pet->Printf("No target to add to hate list");
        }
        
        break;

    case psPETCommandMessage::CMD_STOPATTACK : // StopAttack
        break;
    }
}

csString OwnerCmdPerception::BuildName(psPETCommandMessage::PetCommand_t command)
{
    csString event("ownercmd");
    event.Append(':');

    switch ( command )
    {
    case psPETCommandMessage::CMD_FOLLOW: 
        event.Append( "follow" );
        break;
    case psPETCommandMessage::CMD_STAY :
        event.Append( "stay" );
        break;
    case psPETCommandMessage::CMD_SUMMON :
        event.Append( "summon" );
        break;
    case psPETCommandMessage::CMD_DISMISS :
        event.Append( "dismiss" );
        break;
    case psPETCommandMessage::CMD_ATTACK :
        event.Append( "attack" );
        break;
    case psPETCommandMessage::CMD_STOPATTACK :
        event.Append( "stopattack" );
        break;
    case psPETCommandMessage::CMD_GUARD :
        event.Append( "guard" );
        break;
    case psPETCommandMessage::CMD_ASSIST :
        event.Append( "assist" );
        break;
    case psPETCommandMessage::CMD_NAME :
        event.Append( "name" );
        break; 
    case psPETCommandMessage::CMD_TARGET :
        event.Append( "target" );
        break;
    }
    return event;
}


//---------------------------------------------------------------------------------

OwnerActionPerception::OwnerActionPerception( const char *name  ,
                                              int action,
                                              gemNPCObject *owner ,
                                              gemNPCObject *pet   ) : Perception(name)
{
    this->action = action;
    this->owner = owner;
    this->pet = pet;
}

bool OwnerActionPerception::ShouldReact( Reaction *reaction, NPC *npc )
{
    csString event("ownercmd");
    event.Append(':');

    switch ( this->action )
    {
    case 1: 
        event.Append("attack");
        break;
    case 2: 
        event.Append("damage");
        break;
    case 3: 
        event.Append("logon");
        break;
    case 4: 
        event.Append("logoff");
        break;
    default:
        event.Append("unknown");
        break;
    }

    if (event == reaction->GetEventType(npc))
    {
        return true;
    }
    return false;
}

Perception *OwnerActionPerception::MakeCopy()
{
    OwnerActionPerception *p = new OwnerActionPerception( name, action, owner, pet );
    return p;
}

void OwnerActionPerception::ExecutePerception( NPC *pet, float weight )
{
    switch ( this->action )
    {
    case 1: // Find and Set owner
        break;
    case 2: // Clear Owner and return to astral plane
        break;
    }
}

//---------------------------------------------------------------------------------
/// NPC Pet Perceptions ===========================================================


//---------------------------------------------------------------------------------

NPCCmdPerception::NPCCmdPerception( const char *command, NPC * self ) : Perception("npccmd")
{
    this->cmd = command;
    this->self = self;
}

bool NPCCmdPerception::ShouldReact( Reaction *reaction, NPC *npc )
{
    csString global_event("npccmd:global:");
    global_event.Append(cmd);

    if (global_event.CompareNoCase(reaction->GetEventType(npc)))
    {
        npc->Printf(15,"Matched reaction '%s' to perception '%s'.",reaction->GetEventType(npc).GetDataSafe(), global_event.GetData() );
        return true;
    }
    else
    {
        npc->Printf(16,"No matched reaction '%s' to perception '%s'.",reaction->GetEventType(npc).GetDataSafe(), global_event.GetData() );
    }
    

    csString self_event("npccmd:self:");
    self_event.Append(cmd);

    if (self_event.CompareNoCase(reaction->GetEventType(npc)) && npc == self)
    {
        npc->Printf(15,"Matched reaction '%s' to perception '%s'.",reaction->GetEventType(npc).GetDataSafe(), self_event.GetData() );
        return true;
    }
    else
    {
        npc->Printf(16,"No matched reaction '%s' to perception '%s' for self(%s) with npc(%s).",
                    reaction->GetEventType(npc).GetDataSafe(), self_event.GetData(), 
                    self->GetName(), npc->GetName() );
    }
    
    return false;
}

Perception *NPCCmdPerception::MakeCopy()
{
    NPCCmdPerception *p = new NPCCmdPerception( cmd, self );
    return p;
}

//---------------------------------------------------------------------------------
