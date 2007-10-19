/*
* npcbehave.cpp by Keith Fulton <keith@paqrat.com>
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
#include <csutil/csstring.h>
#include <csgeom/transfrm.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>

#include <propclass/mesh.h>
#include <physicallayer/entity.h>
#include <physicallayer/propclas.h>
#include <propclass/linmove.h>

#include "net/msghandler.h"
#include "net/npcmessages.h"
#include "npcbehave.h"
#include "perceptions.h"
#include "npc.h"
#include "npcclient.h"
#include "networkmgr.h"
#include "globals.h"
#include "util/log.h"
#include "util/location.h"
#include "util/strutil.h"
#include "gem.h"



Reaction::Reaction()
{
    delta_desire = 0;
    affected   = NULL;
    range      = 0;
    inactive_only = false;
}

bool Reaction::Load(iDocumentNode *node,BehaviorSet& behaviors)
{
    delta_desire = node->GetAttributeValueAsFloat("delta");

    // Handle hooking up to the right behavior
    csString name = node->GetAttributeValue("behavior");
    affected = behaviors.Find(name);
    if (!affected)
    {
        Error2("Reaction specified unknown behavior of '%s'. Error in XML.\n",(const char *)name);
        return false;
    }

    // Handle hooking up to the perception
    event_type             = node->GetAttributeValue("event");
    range                  = node->GetAttributeValueAsFloat("range");
    weight                 = node->GetAttributeValueAsFloat("weight");
    faction_diff           = node->GetAttributeValueAsInt("faction_diff");
    oper                   = node->GetAttributeValue("oper");
    value                  = node->GetAttributeValueAsInt("value");
    type                   = node->GetAttributeValue("type");
    inactive_only          = node->GetAttributeValueAsBool("inactive_only");
    react_when_dead        = node->GetAttributeValueAsBool("when_dead",false);
    react_when_invisible   = node->GetAttributeValueAsBool("when_invisible",false);
    react_when_invincible  = node->GetAttributeValueAsBool("when_invincible",false);
    only_interrupt         = node->GetAttributeValue("only_interrupt");

    return true;
}

void Reaction::DeepCopy(Reaction& other,BehaviorSet& behaviors)
{
    delta_desire           = other.delta_desire;
    affected               = behaviors.Find(other.affected->GetName());
    event_type             = other.event_type;
    range                  = other.range;
    faction_diff           = other.faction_diff;
    oper                   = other.oper;
    weight                 = other.weight;
    value                  = other.value;
    type                   = other.type;
    inactive_only          = other.inactive_only;
    react_when_dead        = other.react_when_dead;
    react_when_invisible   = other.react_when_invisible;
    react_when_invincible  = other.react_when_invincible;
    only_interrupt         = other.only_interrupt;
}

void Reaction::React(NPC *who,EventManager *eventmgr,Perception *pcpt)
{
    CS_ASSERT(who);

    // When inactive_only flag is set we should do nothing
    // if the affected behaviour is ative.
    if (inactive_only && affected->GetActive() )
        return;

    // If dead we should not react unless react_when_dead is set
    if (!(who->IsAlive() || react_when_dead))
        return;

    if (only_interrupt)
    {
        bool found = false;
        csArray<csString> strarr = psSplit( only_interrupt, ':');
        for (size_t i = 0; i < strarr.GetSize(); i++)
        {
            if (who->GetCurrentBehavior() && strarr[i] == who->GetCurrentBehavior()->GetName())
            {
                found = true;
                break;
            }
        }
        if (!found) 
            return;
    }

    if (!pcpt->ShouldReact(this,who))
        return;


    who->Printf("Adding %1.1f need to behavior %s for npc %s(EID: %u).",
                delta_desire, affected->GetName(), who->GetName(),
                (who->GetEntity()?who->GetEntity()->GetID():0));

    if (delta_desire>0 || delta_desire<-1)
    {
        affected->ApplyNeedDelta(delta_desire);
    }
    else if (fabs(delta_desire+1)>SMALL_EPSILON)  // -1 in delta means "don't react"
    {
        // zero delta means guarantee that this affected
        // need becomes the highest (and thus active) one.

        float highest = 0;

        if (who->GetCurrentBehavior())
        {
            highest = who->GetCurrentBehavior()->CurrentNeed();
        }
        
        affected->ApplyNeedDelta( highest - affected->CurrentNeed() + 25);
        affected->SetCompletionDecay(-1);
    }
    
    pcpt->ExecutePerception(who,weight);

    if (who->IsDebugging(2))
    {
        who->DumpBehaviorList( );
    }

    Perception *p = pcpt->MakeCopy();
    who->SetLastPerception(p);
}

bool Reaction::ShouldReact(iCelEntity* entity, Perception *pcpt)
{
    gemNPCObject * actor = npcclient->FindEntityID(entity->GetID());

    if (!actor) return false;

    if (!(actor->IsVisible() || react_when_invisible))
    {
        return false;
    }
    
    if (!(!actor->IsInvincible() || react_when_invincible))
    {
        return false;
    }
    return true;
}



/*----------------------------------------------------------------------------*/

Perception *Perception::MakeCopy()
{
    Perception *p = new Perception(name,type);
    return p;
}

csString Perception::ToString()
{
    csString result;
    result.Format("Name: '%s' Type: '%s'",name.GetDataSafe(),type.GetDataSafe());
    return result;
}


/*----------------------------------------------------------------------------*/

bool RangePerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType() &&
        range < reaction->GetRange())
        return true;
    else
        return false;
}

Perception *RangePerception::MakeCopy()
{
    RangePerception *p = new RangePerception(name,range);
    return p;
}

//---------------------------------------------------------------------------------


bool FactionPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType())
    {
        npc->Printf( "FactionPerception Should react? -> ");

        if (player)
        {
            if (!reaction->ShouldReact(player,this))
            {
                npc->Printf( "False\n");
                return false;
            }
        }
        
        if (reaction->GetOp() == '>' )
        {
            npc->Printf( "Checking %d > %d.",faction_delta,reaction->GetFactionDiff() );
            if (faction_delta > reaction->GetFactionDiff() )
            {
                npc->Printf( "True\n");
                return true;
            }
            else
            {
                npc->Printf( "False\n");
                return false;
            }
        }
        else
        {
            npc->Printf( "Checking %d < %d.",faction_delta,reaction->GetFactionDiff() );
            if (faction_delta < reaction->GetFactionDiff() )
            {
                npc->Printf( "True\n");
                return true;
            }
            else
            {
                npc->Printf( "False\n");
                return false;
            }
        }
    }
    return false;
}

bool FactionPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (player)
    {
        float rot;
        psGameObject::GetPosition(player,pos,rot,sector);
        return true;
    }
    return false;
}

Perception *FactionPerception::MakeCopy()
{
    FactionPerception *p = new FactionPerception(name,faction_delta,player);
    return p;
}

void FactionPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList(player,weight*-faction_delta);
}

//---------------------------------------------------------------------------------


bool ItemPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType())
    {
        return true;
    }
    return false;
}

bool ItemPerception::GetLocation(csVector3& pos, iSector*& sector)
{
    if (item)
    {
        float rot;
        psGameObject::GetPosition(item,pos,rot,sector);
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


bool LocationPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType() && (reaction->GetType() == "" || type == reaction->GetType()))
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
        sector = location->GetSector(npcclient->GetEngine());
        return true;
    }
    return false;
}

Perception *LocationPerception::MakeCopy()
{
    LocationPerception *p = new LocationPerception(name,type,location);
    return p;
}

float LocationPerception::GetRadius() const
{
    return location->radius; 
}

//---------------------------------------------------------------------------------



bool AttackPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType())
    {
        return true;
    }
    return false;
}

Perception *AttackPerception::MakeCopy()
{
    AttackPerception *p = new AttackPerception(name,attacker);
    return p;
}

void AttackPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList(attacker,weight);
}
//---------------------------------------------------------------------------------

bool GroupAttackPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType())
    {
        return true;
    }
    return false;
}

Perception *GroupAttackPerception::MakeCopy()
{
    GroupAttackPerception *p = new GroupAttackPerception(name,attacker_ents,bestSkillSlots);
    return p;
}

void GroupAttackPerception::ExecutePerception(NPC *npc,float weight)
{
    for(size_t i=0;i<attacker_ents.GetSize();i++)
        npc->AddToHateList(attacker_ents[i],bestSkillSlots[i]*weight);
}

//---------------------------------------------------------------------------------

bool DamagePerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    npc->Printf("Damage of %1.1f from player %s (EID: %u).",
                damage,attacker->GetName(),attacker->GetID() );

    if (name == reaction->GetEventType())
    {
        npc->Printf("Reacting");
        return true;
    }
    npc->Printf("Skipping");
    return false;
}

Perception *DamagePerception::MakeCopy()
{
    DamagePerception *p = new DamagePerception(name,attacker,damage);
    return p;
}

void DamagePerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList(attacker,damage*weight);
}

//---------------------------------------------------------------------------------

SpellPerception::SpellPerception(const char *name,
                                 iCelEntity *caster,iCelEntity *target, 
                                 const char *type,float severity)
                                 : Perception(name)
{
    this->caster = caster;
    this->target = target;
    this->spell_severity = severity;
    this->type = type;
}

bool SpellPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    csString event(type);
    event.Append(':');

    if (npc->GetEntityHate(caster) || npc->GetEntityHate(target))
        event.Append("target");
    else if (target == npc->GetEntity())
        event.Append("self");
    else
        event.Append("unknown");

    if (event == reaction->GetEventType())
    {
        npc->Printf( "%s spell cast by %s on %s, severity %1.1f.\n",
            event.GetData(), (caster)?caster->GetName():"(Null caster)", (target)?target->GetName():"(Null target)", spell_severity);

        return true;
    }
    return false;
}

Perception *SpellPerception::MakeCopy()
{
    SpellPerception *p = new SpellPerception(name,caster,target,type,spell_severity);
    return p;
}

void SpellPerception::ExecutePerception(NPC *npc,float weight)
{
    npc->AddToHateList(caster,spell_severity*weight);
}

//---------------------------------------------------------------------------------

bool TimePerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType() )
    {
        npc->Printf("Time is now %d o'clock and I need %d o'clock.",time,reaction->GetValue() );
        if (time == reaction->GetValue() )
        {
            npc->Printf("Reacting.");
            return true;
        }
        else
        {
            npc->Printf("Skipping.");
            return false;
        }
    }
    return false;
}

Perception *TimePerception::MakeCopy()
{
    TimePerception *p = new TimePerception(time);
    return p;
}

//---------------------------------------------------------------------------------



bool DeathPerception::ShouldReact(Reaction *reaction,NPC *npc)
{
    if (name == reaction->GetEventType())
    {
        npc->Printf( "Reacting to death perception.\n");
        return true;
    }
    return false;
}

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

OwnerCmdPerception::OwnerCmdPerception( const char *name  ,
                                        int command,
                                        iCelEntity *owner ,
                                        iCelEntity *pet   ) : Perception(name)
{
    this->command = command;
    this->owner = owner;
    this->pet = pet;
}

bool OwnerCmdPerception::ShouldReact( Reaction *reaction, NPC *npc )
{
    csString event("ownercmd");
    event.Append(':');

    switch ( this->command )
    {
    case OwnerCmdPerception::OCP_FOLLOW: event.Append( "follow" );
        break;
    case OwnerCmdPerception::OCP_STAY : event.Append( "stay" );
        break;
    case OwnerCmdPerception::OCP_SUMMON : event.Append( "summon" );
        break;
    case OwnerCmdPerception::OCP_DISMISS : event.Append( "dismiss" );
        break;
    case OwnerCmdPerception::OCP_ATTACK : event.Append( "attack" );
        break;
    case OwnerCmdPerception::OCP_STOPATTACK : event.Append( "stopattack" );
        break;
    default: event.Append("unknown");
        break;
    }

    if (event == reaction->GetEventType())
    {
        npc->Printf("Matched reaction %s to perception %s...\n",reaction->GetEventType(), event.GetData() );
        return true;
    }
    return false;
}

Perception *OwnerCmdPerception::MakeCopy()
{
    OwnerCmdPerception *p = new OwnerCmdPerception( name, command, owner, pet );
    return p;
}

void OwnerCmdPerception::ExecutePerception( NPC *pet, float weight )
{
    switch ( this->command )
    {
    case OwnerCmdPerception::OCP_SUMMON : // Summon
        break;
    case OwnerCmdPerception::OCP_DISMISS : // Dismiss
        break;
    case OwnerCmdPerception::OCP_ATTACK : // Attack

        pet->AddToHateList(pet->GetTarget(), 1 * weight );
        break;

    case OwnerCmdPerception::OCP_STOPATTACK : // StopAttack
        break;
    }
}

//---------------------------------------------------------------------------------

OwnerActionPerception::OwnerActionPerception( const char *name  ,
                                              int action,
                                              iCelEntity *owner ,
                                              iCelEntity *pet   ) : Perception(name)
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
    case 1: event.Append("attack");
        break;
    case 2: event.Append("damage");
        break;
    case 3: event.Append("logon");
        break;
    case 4: event.Append("logoff");
        break;
    default: event.Append("unknown");
        break;
    }

    if (event == reaction->GetEventType())
    {
        npc->Printf("Matched reaction %s to perception %s...\n",reaction->GetEventType(), event.GetData() );
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

    if (strcasecmp(global_event,reaction->GetEventType()) == 0)
    {
        npc->Printf(5,"Matched reaction '%s' to perception '%s'.\n",reaction->GetEventType(), global_event.GetData() );
        return true;
    }
    else
    {
        npc->Printf(6,"No matched reaction '%s' to perception '%s'.\n",reaction->GetEventType(), global_event.GetData() );
    }
    

    csString self_event("npccmd:self:");
    self_event.Append(cmd);

    if (strcasecmp(self_event,reaction->GetEventType())==0 && npc == self)
    {
        npc->Printf(5,"Matched reaction '%s' to perception '%s'.\n",reaction->GetEventType(), self_event.GetData() );
        return true;
    }
    else
    {
        npc->Printf(6,"No matched reaction '%s' to perception '%s' for self(%s) with npc(%s).\n",
                    reaction->GetEventType(), self_event.GetData(), 
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
