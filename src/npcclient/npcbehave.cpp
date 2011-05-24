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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csgeom/transfrm.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <csutil/xmltiny.h>



//=============================================================================
// Project Includes
//=============================================================================
#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

#include "util/log.h"
#include "util/location.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/eventmanager.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcoperations.h"
#include "npcbehave.h"
#include "npc.h"
#include "perceptions.h"
#include "networkmgr.h"
#include "npcmesh.h"
#include "gem.h"

extern bool running;

psNPCClient* NPCType::npcclient = NULL;

NPCType::NPCType(psNPCClient* npcclient, EventManager* eventmanager)
    :behaviors(eventmanager),ang_vel(999),vel(999),velSource(VEL_DEFAULT)
{
    this->npcclient = npcclient;
}

NPCType::~NPCType()
{
}

void NPCType::DeepCopy(NPCType& other)
{
    npcclient             = other.npcclient;
    name                  = other.name;
    ang_vel               = other.ang_vel;
    velSource             = other.velSource;
    vel                   = other.vel;
    collisionPerception   = other.collisionPerception;
    outOfBoundsPerception = other.outOfBoundsPerception;
    inBoundsPerception    = other.inBoundsPerception;
    fallingPerception     = other.fallingPerception;

    behaviors.DeepCopy(other.behaviors);

    for (size_t x=0; x<other.reactions.GetSize(); x++)
    {
        reactions.Push( new Reaction(*other.reactions[x],behaviors) );
    }
}

bool NPCType::Load(iResultRow &row)
{
    csString parents = row.GetString("parents");
    if(!parents.IsEmpty()) // this npctype is a subclass of another npctype
    {
        csArray<csString> parent = psSplit(parents,',');
        for(size_t i = 0; i < parent.GetSize(); i++)
        {
            NPCType *superclass = npcclient->FindNPCType(parent[i]);
            if(superclass)
            {
                DeepCopy(*superclass);  // This pulls everything from the parent into this one.
            }
            else
            {
                Error2("Specified parent npctype '%s' could not be found.",
                       parent[i].GetDataSafe());
                return false;
            }
        }
    }

    name = row.GetString("name");
    if(name.Length() == 0)
    {
        Error1("NPCType has no name attribute. Error in DB");
        return false;
    }

    ang_vel = row.GetFloat("ang_vel");

    csString velStr = row.GetString("vel");
    velStr.Upcase();

    if(velStr.IsEmpty())
    {
        // Do nothing. Use velSource from constructor default value
        // or as inherited from superclass.
    }
    else if(velStr == "$WALK")
    {
        velSource = VEL_WALK;
    }
    else if (velStr == "$RUN")
    {
        velSource = VEL_RUN;
    }
    else if(row.GetFloat("vel"))
    {
        velSource = VEL_USER;
        vel = row.GetFloat("vel");
    }

    collisionPerception   = row.GetString("collision");
    outOfBoundsPerception = row.GetString("out_of_ounds");
    inBoundsPerception    = row.GetString("in_bounds");
    fallingPerception     = row.GetString("falling");

    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(row.GetString("script"));
    if(error)
    {
        Error3("NPCType script parsing error:%s in %s", error, name.GetData());
        return false;
    }
    csRef<iDocumentNode> node = doc->GetRoot();
    if(!node)
    {
        Error2("No XML root in npc type script of %s", name.GetData());
        return false;
    }
    
    // Now read in behaviors and reactions
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        if(strcmp(node->GetValue(), "behavior") == 0)
        {
            Behavior *b = new Behavior;
            if(!b->Load(node))
            {
                Error3("Could not load behavior '%s'. Error in DB XML in node '%s'.",
                       b->GetName(),node->GetValue());
                delete b;
                return false;
            }
            behaviors.Add(b);
            Debug3(LOG_STARTUP,0, "Added behavior '%s' to type %s.\n",b->GetName(),name.GetData() );
        }
        else if(strcmp( node->GetValue(), "react" ) == 0)
        {
            Reaction *r = new Reaction;
            if(!r->Load(node,behaviors))
            {
                Error1("Could not load reaction. Error in DB XML");
                delete r;
                return false;
            }
            // check for duplicates and keeps the last one
            for(size_t i=0; i<reactions.GetSize(); i++)
            {
                // Same event with same type
                if(!strcmp(reactions[i]->GetEventType(),r->GetEventType())&&
                    (reactions[i]->type == r->type)&&
                    (reactions[i]->values == r->values))
                {
                    // Check if there is a mach in affected
                    for(size_t k=0; k< r->affected.GetSize(); k++)
                    {
                        for(size_t j=0; j< reactions[i]->affected.GetSize(); j++)
                        {
                            if(!strcmp(r->affected[k]->GetName(),reactions[i]->affected[j]->GetName()))
                            {
                                // Should probably delete and clear out here
                                // to allow for overiding of event,affected pairs.
                                // Though now give error, until needed.
                                Error4("Reaction of type '%s' already connected to '%s' in '%s'",
                                       r->GetEventType(),reactions[i]->affected[j]->GetName(), name.GetDataSafe());
                                return false;
                                // delete reactions[i];
                                //reactions.DeleteIndex(i);
                                //break;
                            }
                        }
                    }
                }
            }

            reactions.Insert(0,r);  // reactions get inserted at beginning so subclass ones take precedence over superclass.
        }
        else
        {
            Error1("Node under NPCType is not 'behavior' or 'react'. Error in DB XML");
            return false;
        }
    }
    return true; // success
}

bool NPCType::Load(iDocumentNode *node)
{
    csString parents = node->GetAttributeValue("parent");
    if (!parents.IsEmpty()) // this npctype is a subclass of another npctype
    {
        csArray<csString> parent = psSplit(parents,',');
        for (size_t i = 0; i < parent.GetSize(); i++)
        {
            NPCType *superclass = npcclient->FindNPCType(parent[i]);
            if (superclass)
            {
                DeepCopy(*superclass);  // This pulls everything from the parent into this one.
            }
            else
            {
                Error2("Specified parent npctype '%s' could not be found.",
                       parent[i].GetDataSafe());
                return false;
            }
        }
    }

    name = node->GetAttributeValue("name");
    if ( name.Length() == 0 )
    {
        Error1("NPCType has no name attribute. Error in XML");
        return false;
    }

    if (node->GetAttributeValueAsFloat("ang_vel") )
        ang_vel = node->GetAttributeValueAsFloat("ang_vel");
    else
        ang_vel = 999;

    csString velStr = node->GetAttributeValue("vel");
    velStr.Upcase();

    if (velStr.IsEmpty())
    {
        // Do nothing. Use velSource from constructor default value
        // or as inherited from superclass.
    }
    else if (velStr == "$WALK")
    {
        velSource = VEL_WALK;
    }
    else if (velStr == "$RUN")
    {
        velSource = VEL_RUN;
    }
    else if (node->GetAttributeValueAsFloat("vel") )
    {
        velSource = VEL_USER;
        vel = node->GetAttributeValueAsFloat("vel");
    }

    collisionPerception   = node->GetAttributeValue("collision");
    outOfBoundsPerception = node->GetAttributeValue("out_of_bounds");
    inBoundsPerception    = node->GetAttributeValue("in_bounds");
    fallingPerception     = node->GetAttributeValue("falling");

    // Now read in behaviors and reactions
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "behavior" ) == 0 )
        {
            Behavior *b = new Behavior;
            if (!b->Load(node))
            {
                Error3("Could not load behavior '%s'. Error in XML in node '%s'.",
                       b->GetName(),node->GetValue());
                delete b;
                return false;
            }
            behaviors.Add(b);
            Debug3(LOG_STARTUP,0, "Added behavior '%s' to type %s.\n",b->GetName(),name.GetData() );
        }
        else if ( strcmp( node->GetValue(), "react" ) == 0 )
        {
            Reaction *r = new Reaction;
            if (!r->Load(node,behaviors))
            {
                Error1("Could not load reaction. Error in XML");
                delete r;
                return false;
            }
            // check for duplicates and keeps the last one
            for (size_t i=0; i<reactions.GetSize(); i++)
            {
                // Same event with same type
                if (!strcmp(reactions[i]->GetEventType(),r->GetEventType())&&
                    (reactions[i]->type == r->type)&&
                    (reactions[i]->values == r->values))
                {
                    // Check if there is a mach in affected
                    for (size_t k=0; k< r->affected.GetSize(); k++)
                    {
                        for (size_t j=0; j< reactions[i]->affected.GetSize(); j++)
                        {
                            if (!strcmp(r->affected[k]->GetName(),reactions[i]->affected[j]->GetName()))
                            {
                                // Should probably delete and clear out here
                                // to allow for overiding of event,affected pairs.
                                // Though now give error, until needed.
                                Error4("Reaction of type '%s' already connected to '%s' in '%s'",
                                       r->GetEventType(),reactions[i]->affected[j]->GetName(), name.GetDataSafe());
                                return false;
                                // delete reactions[i];
                                //reactions.DeleteIndex(i);
                                //break;
                            }
                            
                        }
                        
                    }
                }
            }

            reactions.Insert(0,r);  // reactions get inserted at beginning so subclass ones take precedence over superclass.
        }
        else
        {
            Error1("Node under NPCType is not 'behavior' or 'react'. Error in XML");
            return false;
        }
    }
    return true; // success
}

void NPCType::FirePerception(NPC *npc, Perception *pcpt)
{
    for (size_t x=0; x<reactions.GetSize(); x++)
    {
        reactions[x]->React(npc,pcpt);
    }
}

void NPCType::DumpReactionList(NPC *npc)
{
    CPrintf(CON_CMDOUTPUT, "%-25s %-25s %-5s %-10s %-20s %s\n","Reaction","Type","Range","Value","Last","Affects");
    for (size_t i=0; i<reactions.GetSize(); i++)
    {
        


        CPrintf(CON_CMDOUTPUT, "%-25s %-25s %5.1f %-10s %-20s %s\n",
                reactions[i]->GetEventType(),reactions[i]->GetType().GetDataSafe(),
                reactions[i]->GetRange(),
                reactions[i]->GetValue().GetDataSafe(),
                reactions[i]->GetLastTriggerd().GetDataSafe(),
                reactions[i]->GetAffectedBehaviors().GetDataSafe());
    }
}

void NPCType::ClearState(NPC *npc)
{
    behaviors.ClearState(npc);
}

void NPCType::Advance(csTicks delta, NPC *npc)
{
    behaviors.Advance(delta,npc);
}

void NPCType::ResumeScript(NPC *npc, Behavior *which)
{
    behaviors.ResumeScript(npc, which);
}

void NPCType::Interrupt(NPC *npc)
{
    behaviors.Interrupt(npc);
}

float NPCType::GetAngularVelocity(NPC * /*npc*/)
{
    if (ang_vel != 999)
    {
        return ang_vel;
    }
    else
    {
        return 360*TWO_PI/360;
    }
}

float NPCType::GetVelocity(NPC *npc)
{
    switch (velSource){
    case VEL_DEFAULT:
        return 1.5;
    case VEL_USER:
        return vel;
    case VEL_WALK:
        return npc->GetWalkVelocity();
    case VEL_RUN:
        return npc->GetRunVelocity();
    }
    return 0.0; // Should not return
}

const csString& NPCType::GetCollisionPerception() const
{
    return collisionPerception;
}

const csString& NPCType::GetOutOfBoundsPerception() const
{
    return outOfBoundsPerception;
}

const csString& NPCType::GetInBoundsPerception() const
{
    return inBoundsPerception;
}

const csString& NPCType::GetFallingPerception() const
{
    return fallingPerception;
}

//---------------------------------------------------------------------------
BehaviorSet::BehaviorSet(EventManager *eventmanager)
{
    active=NULL;
    eventmgr = eventmanager;
}

void BehaviorSet::ClearState(NPC *npc)
{
    // Ensure any existing script is ended correctly.
    Interrupt(npc);
    for (size_t i = 0; i<behaviors.GetSize(); i++)
    {
        behaviors[i]->ResetNeed();
        behaviors[i]->SetIsActive(false);
        behaviors[i]->ClearInterrupted();
    }
    active = NULL;
}

bool BehaviorSet::Add(Behavior *behavior)
{
    // Search for dublicates
    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        if (!strcmp(behaviors[i]->GetName(),behavior->GetName()))
        {
            behaviors[i] = behavior;  // substitute
            return false;
        }
    }
    // Insert as new behavior
    behaviors.Push(behavior);
    return true;
}

void BehaviorSet::Advance(csTicks delta,NPC *npc)
{
    while (true)
    {
        float max_need = -999.0;
        bool behaviours_changed = false;

        // Go through and update needs based on time
        for (size_t i=0; i<behaviors.GetSize(); i++)
        {
            Behavior * b = behaviors[i];
            if (b->ApplicableToNPCState(npc))
            {
                b->Advance(delta,npc,eventmgr);

                if (behaviors[i]->CurrentNeed() != behaviors[i]->NewNeed())
                {
                    npc->Printf(4, "Advancing %-30s:\t%1.1f ->%1.1f",
                                behaviors[i]->GetName(),
                                behaviors[i]->CurrentNeed(),
                                behaviors[i]->NewNeed() );
                }

                if (b->NewNeed() > max_need) // the advance causes re-ordering
                {
                    if (i!=0)  // trivial swap if same element
                    {
                        behaviors[i] = behaviors[0];
                        behaviors[0] = b;  // now highest need is elem 0
                        behaviours_changed = true;
                    }
                    max_need = b->NewNeed();
                }
                b->CommitAdvance();   // Update key to correct value
            }
        }

        // now that behaviours are correctly sorted, select the first one
        Behavior *new_behaviour = behaviors[0];

        // use it only if need > 0
        if (new_behaviour->CurrentNeed()<=0 || !new_behaviour->ApplicableToNPCState(npc))
        {
            if (npc->IsDebugging(3))
            {
                npc->DumpBehaviorList();
            }
            npc->Printf(15,"NO Active or no applicable behavior." );
            return;
        }

        if (new_behaviour != active)
        {
            if (active)  // if there is a behavior allready assigned to this npc
            {
                npc->Printf(1,"Switching behavior from '%s' to '%s'",
                            active->GetName(),
                            new_behaviour->GetName() );

                // Interrupt and stop current behaviour
                active->InterruptScript(npc,eventmgr);
                active->SetIsActive(false);
            }
            else
            {
                npc->Printf(1,"Activating behavior '%s'",
                            new_behaviour->GetName() );
            }
            

            // Set the new active behaviour
            active = new_behaviour;
            // Activate the new behaviour
            active->SetIsActive(true);

            // Dump bahaviour list if changed
            if (npc->IsDebugging(3))
            {
                npc->DumpBehaviorList();
            }

            // Run the new active behavior
            if (active->StartScript(npc,eventmgr))
            {
                // This behavior is done so set it inactive
                active->SetIsActive(false);
                active = NULL;
                break;
            }
            else
            {
                // This behavior isn't done yet so break and continue later
                break;
            }
        }
        else
        {
            break;
        }
    }

    npc->Printf(15,"Active behavior is '%s'", (active?active->GetName():"(null)") );
    return;
}

void BehaviorSet::ResumeScript(NPC *npc, Behavior *which)
{
    if (which == active && which->ApplicableToNPCState(npc))
    {
        if (active->ResumeScript(npc,eventmgr))
        {
            active->SetIsActive(false);
            active = NULL;
        }
        
    }
}

void BehaviorSet::Interrupt(NPC *npc)
{
    if (active)
    {
        active->InterruptScript(npc,eventmgr);
    }
}

void BehaviorSet::DeepCopy(BehaviorSet& other)
{
    Behavior *b,*b2;
    for (size_t i=0; i<other.behaviors.GetSize(); i++)
    {
        b  = other.behaviors[i];
        b2 = new Behavior(*b);
        behaviors.Push(b2);
    }
}

Behavior *BehaviorSet::Find(const char *name)
{
    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        if (!strcmp(behaviors[i]->GetName(),name))
            return behaviors[i];
    }
    return NULL;
}

void BehaviorSet::DumpBehaviorList(NPC *npc)
{
    CPrintf(CON_CMDOUTPUT, "Appl. IA %-30s %5s %5s\n","Behavior","Curr","New");

    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        char applicable = 'N';
        if (npc && behaviors[i]->ApplicableToNPCState(npc))
        {
            applicable = 'Y';
        }

        CPrintf(CON_CMDOUTPUT, "%c     %s%s %-30s %5.1f %5.1f\n",applicable,
                (behaviors[i]->IsInterrupted()?"!":" "),
                (behaviors[i]->IsActive()?"*":" "),
                behaviors[i]->GetName(),behaviors[i]->CurrentNeed(),
                behaviors[i]->NewNeed());
    }
}

csString BehaviorSet::InfoBehaviors(NPC *npc)
{
    csString reply;

    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        reply.AppendFmt("%s(%s%s%.1f) ",
                        behaviors[i]->GetName(),
                        (behaviors[i]->IsInterrupted()?"!":""),
                        (behaviors[i]->IsActive()?"*":""),
		        behaviors[i]->CurrentNeed());
    }
    return reply;
}

//---------------------------------------------------------------------------

Behavior::Behavior()
{
    name                    = "";
    loop                    = false;
    isActive                = false;
    is_applicable_when_dead = false;
    need_decay_rate         = 0;
    need_growth_rate        = 0;
    completion_decay        = 0;
    init_need               = 0;
    resume_after_interrupt  = false;
    current_need            = init_need;
    new_need                = -999;
    interrupted             = false;
    current_step            = 0;
    minLimitValid           = false;
    minLimit                = 0.0;
    maxLimitValid           = false;
    maxLimit                = 0.0;
}

Behavior::Behavior(const char *n)
{
    name                    = n;
    loop                    = false;
    isActive                = false;
    is_applicable_when_dead = false;
    need_decay_rate         = 0;
    need_growth_rate        = 0;
    completion_decay        = 0;
    init_need               = 0;
    resume_after_interrupt  = false;
    current_need            = init_need;
    new_need                =-999;
    interrupted             = false;
    current_step            = 0;
    minLimitValid           = false;
    minLimit                = 0.0;
    maxLimitValid           = false;
    maxLimit                = 0.0;
}

Behavior::Behavior(Behavior& other)
{
    DeepCopy(other);
}


void Behavior::DeepCopy(Behavior& other)
{
    name                    = other.name;
    loop                    = other.loop;
    isActive                = other.isActive;
    is_applicable_when_dead = other.is_applicable_when_dead;
    need_decay_rate         = other.need_decay_rate;  // need lessens while performing behavior
    need_growth_rate        = other.need_growth_rate; // need grows while not performing behavior
    completion_decay        = other.completion_decay;
    init_need               = other.init_need;
    resume_after_interrupt  = other.resume_after_interrupt;
    current_need            = other.current_need;
    new_need                = -999;
    interrupted             = false;
    minLimitValid           = other.minLimitValid;
    minLimit                = other.minLimit;
    maxLimitValid           = other.maxLimitValid;
    maxLimit                = other.maxLimit;

    for (size_t x=0; x<other.sequence.GetSize(); x++)
    {
        sequence.Push( other.sequence[x]->MakeCopy() );
    }

    // Instance local variables. No need to copy.
    current_step = 0;
}

bool Behavior::Load(iDocumentNode *node)
{
    // This function can be called recursively, so we only get attributes at top level
    name = node->GetAttributeValue("name");
    if ( name.Length() == 0 )
    {
        Error1("Behavior has no name attribute. Error in XML");
        return false;
    }

    loop                    = node->GetAttributeValueAsBool("loop",false);
    need_decay_rate         = node->GetAttributeValueAsFloat("decay");
    completion_decay        = node->GetAttributeValueAsFloat("completion_decay");
    need_growth_rate        = node->GetAttributeValueAsFloat("growth");
    init_need               = node->GetAttributeValueAsFloat("initial");
    is_applicable_when_dead = node->GetAttributeValueAsBool("when_dead");
    resume_after_interrupt  = node->GetAttributeValueAsBool("resume",false);
    if (node->GetAttributeValue("min"))
    {
        minLimitValid = true;
        minLimit = node->GetAttributeValueAsFloat("min");
    }
    else
    {
        minLimitValid = false;
    }
    if (node->GetAttributeValue("max"))
    {
        maxLimitValid = true;
        maxLimit = node->GetAttributeValueAsFloat("max");
    }
    else
    {
        maxLimitValid = false;
    }
    

    current_need            = init_need;

    return LoadScript(node,true);
}

bool Behavior::LoadScript(iDocumentNode *node,bool top_level)
{
    // Now read in script for this behavior
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        ScriptOperation * op = NULL;

        // Some Responses need post load functions.
        bool postLoadBeginLoop = false;

        // Needed by the post load of begin loop
        int beginLoopWhere = -1;

        // This is a operation so read it's factory to create it.
        if ( strcmp( node->GetValue(), "chase" ) == 0 )
        {
            op = new ChaseOperation;
        }
        else if ( strcmp( node->GetValue(), "circle" ) == 0 )
        {
            op = new CircleOperation;
        }
        else if ( strcmp( node->GetValue(), "debug" ) == 0 )
        {
            op = new DebugOperation;
        }
        else if ( strcmp( node->GetValue(), "dequip" ) == 0 )
        {
            op = new DequipOperation;
        }
        else if ( strcmp( node->GetValue(), "dig" ) == 0 )
        {
            op = new DigOperation;
        }
        else if ( strcmp( node->GetValue(), "drop" ) == 0 )
        {
            op = new DropOperation;
        }
        else if ( strcmp( node->GetValue(), "eat" ) == 0 )
        {
            op = new EatOperation;
        }
        else if ( strcmp( node->GetValue(), "emote" ) == 0 )
        {
            op = new EmoteOperation;
        }
        else if ( strcmp( node->GetValue(), "equip" ) == 0 )
        {
            op = new EquipOperation;
        }
        else if ( strcmp( node->GetValue(), "invisible" ) == 0 )
        {
            op = new InvisibleOperation;
        }
        else if ( strcmp( node->GetValue(), "locate" ) == 0 )
        {
            op = new LocateOperation;
        }
        else if ( strcmp( node->GetValue(), "loop" ) == 0 )
        {
            op = new LoopBeginOperation;
            beginLoopWhere = (int)sequence.GetSize(); // Where will sequence be pushed
            postLoadBeginLoop = true;
        }
        else if ( strcmp( node->GetValue(), "melee" ) == 0 )
        {
            op = new MeleeOperation;
        }
        else if ( strcmp( node->GetValue(), "memorize" ) == 0 )
        {
            op = new MemorizeOperation;
        }
        else if ( strcmp( node->GetValue(), "move" ) == 0 )
        {
            op = new MoveOperation;
        }
        else if ( strcmp( node->GetValue(), "movepath" ) == 0 )
        {
            op = new MovePathOperation;
        }
        else if ( strcmp( node->GetValue(), "moveto" ) == 0 )
        {
            op = new MoveToOperation;
        }
        else if ( strcmp( node->GetValue(), "navigate" ) == 0 )
        {
            op = new NavigateOperation;
        }
        else if ( strcmp( node->GetValue(), "percept" ) == 0 )
        {
            op = new PerceptOperation;
        }
        else if ( strcmp( node->GetValue(), "pickup" ) == 0 )
        {
            op = new PickupOperation;
        }
        else if ( strcmp( node->GetValue(), "reproduce" ) == 0 )
        {
            op = new ReproduceOperation;
        }
        else if ( strcmp( node->GetValue(), "resurrect" ) == 0 )
        {
            op = new ResurrectOperation;
        } 
        else if ( strcmp( node->GetValue(), "reward" ) == 0 )
        {
            op = new RewardOperation;
        }
        else if ( strcmp( node->GetValue(), "rotate" ) == 0 )
        {
            op = new RotateOperation;
        }
        else if ( strcmp( node->GetValue(), "sequence" ) == 0 )
        {
            op = new SequenceOperation;
        }
        else if ( strcmp( node->GetValue(), "share_memories" ) == 0 )
        {
            op = new ShareMemoriesOperation;
        }
        else if ( strcmp( node->GetValue(), "sit" ) == 0 )
        {
	    op = new SitOperation(true);
        }
        else if ( strcmp( node->GetValue(), "standup" ) == 0 )
        {
            op = new SitOperation(false);
        }
        else if ( strcmp( node->GetValue(), "talk" ) == 0 )
        {
            op = new TalkOperation;
        }
        else if ( strcmp( node->GetValue(), "teleport" ) == 0 )
        {
            op = new TeleportOperation;
        }
        else if ( strcmp( node->GetValue(), "transfer" ) == 0 )
        {
            op = new TransferOperation;
        }
        else if ( strcmp( node->GetValue(), "tribe_home" ) == 0 )
        {
            op = new TribeHomeOperation;
        }
        else if ( strcmp( node->GetValue(), "tribe_type" ) == 0 )
        {
            op = new TribeTypeOperation;
        }
        else if ( strcmp( node->GetValue(), "visible" ) == 0 )
        {
            op = new VisibleOperation;
        }
        else if ( strcmp( node->GetValue(), "wait" ) == 0 )
        {
            op = new WaitOperation;
        }
        else if ( strcmp( node->GetValue(), "wander" ) == 0 )
        {
            op = new WanderOperation;
        }
        else if ( strcmp( node->GetValue(), "watch" ) == 0 )
        {
            op = new WatchOperation;
        }
        else
        {
            Error2("Node '%s' under Behavior is not a valid script operation name. Error in XML",
                   node->GetValue() );
            return false;
        }

        if (!op->Load(node))
        {
            Error2("Could not load <%s> ScriptOperation. Error in XML",op->GetName());
            delete op;
            return false;
        }
        sequence.Push(op);

        // Execute any outstanding post load operations.
        if (postLoadBeginLoop)
        {
            LoopBeginOperation * blop = dynamic_cast<LoopBeginOperation*>(op);
            if (!LoadScript(node,false)) // recursively load within loop
            {
                Error1("Could not load within Loop Operation. Error in XML");
                return false;
            }

            LoopEndOperation *op2 = new LoopEndOperation(beginLoopWhere,blop->iterations);
            sequence.Push(op2);
        }
    }

    return true; // success
}

void Behavior::Advance(csTicks delta, NPC *npc, EventManager *eventmgr)
{
    // Initialize new_need if not updated before.
    if (new_need == -999)
    {
        new_need = current_need;
    }

    float d = .001 * delta;

    if (isActive)
    {
        // Apply delta to need, will check for limits as well
        ApplyNeedDelta(npc, -d * need_decay_rate );

        if (current_step < sequence.GetSize())
        {
            npc->Printf(10,"%s - Advance active delta: %.3f Need: %.2f Decay Rate: %.2f",
                        name.GetData(),d,new_need,need_decay_rate);

            if (!sequence[current_step]->HasCompleted())
            {
                sequence[current_step]->Advance(d,npc,eventmgr);
            }
        }
    }
    else
    {
        // Apply delta to need, will check for limits as well
        ApplyNeedDelta(npc, d * need_growth_rate );

        npc->Printf(11,"%s - Advance none active delta: %.3f Need: %.2f Growth Rate: %.2f",
                    name.GetData(),d,new_need,need_growth_rate);
    }
}


void Behavior::CommitAdvance()
{
    // Only update the current_need if new_need has been initialized.
    if (new_need!=-999)
    {
        current_need = new_need;
    }
}

void Behavior::ApplyNeedDelta(NPC *npc, float deltaDesire)
{
    // Initialize new_need if not updated before.
    if (new_need==-999)
    {
        new_need = current_need;
    }

    // Apply the delta to new_need
    new_need += deltaDesire;

    // Handle min desire limit
    if (minLimitValid && (new_need < minLimit))
    {
        npc->Printf(5,"%s - ApplyNeedDelta limited new_need of %.3f to min value %.3f.",
                    name.GetData(),new_need,minLimit);
        
        new_need = minLimit;
    }
    
    // Handle max desire limit
    if (maxLimitValid && (new_need > maxLimit))
    {
        npc->Printf(5,"%s - ApplyNeedDelta limited new_need of %.3f to max value %.3f.",
                    name.GetData(),new_need,maxLimit);

        new_need = maxLimit;
    }
}

void Behavior::ApplyNeedAbsolute(NPC *npc, float absoluteDesire)
{
    new_need = absoluteDesire;

    // Handle min desire limit
    if (minLimitValid && (new_need < minLimit))
    {
        npc->Printf(5,"%s - ApplyNeedAbsolute limited new_need of %.3f to min value %.3f.",
                    name.GetData(),new_need,minLimit);

        new_need = minLimit;
    }
    
    // Handle max desire limit
    if (maxLimitValid && (new_need > maxLimit))
    {
        npc->Printf(5,"%s - ApplyNeedAbsolute limited new_need of %.3f to max value %.3f.",
                    name.GetData(),new_need,maxLimit);

        new_need = maxLimit;
    }
}


bool Behavior::ApplicableToNPCState(NPC* npc)
{
    return npc->IsAlive() || (!npc->IsAlive() && is_applicable_when_dead);
}

void Behavior::DoCompletionDecay(NPC* npc)
{
    if (completion_decay)
    {
        float delta_decay = completion_decay;
        
        if (completion_decay == -1)
        {
            delta_decay = current_need;
        }
        
        npc->Printf(10, "Subtracting completion decay of %.2f from behavior '%s'.",delta_decay,GetName() );
        new_need = current_need - delta_decay;
    }
}

bool Behavior::StartScript(NPC *npc, EventManager *eventmgr)
{
    if (interrupted && resume_after_interrupt)
    {
        npc->Printf(3,"Resuming behavior %s after interrupt at step %d - %s.",
                    name.GetData(), current_step, sequence[current_step]->GetName());
        interrupted = false;
        return RunScript(npc,eventmgr,true);
    }
    else
    {
        // Start at the first step of the script.
        current_step = 0;

        if (interrupted)
        {
            // We don't resume_after_interrupt, but the flag needs to be cleared
            npc->Printf(3,"Restarting behavior %s after interrupt at step %d - %s.",
                        name.GetData(), current_step, sequence[current_step]->GetName());
            interrupted = false;
        }

        return RunScript(npc,eventmgr,false);
    }
}

bool Behavior::RunScript(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    size_t start_step = current_step;

    // Without this, we will get an infinite loop.
    if (start_step >= sequence.GetSize())
    {
        start_step = 0;
    }

    while (true)
    {
        if (current_step < sequence.GetSize() )
        {
            npc->Printf(2, "Running %s step %d - %s operation%s",
                        name.GetData(),current_step,sequence[current_step]->GetName(),
                        (interrupted?" Interrupted":""));
            sequence[current_step]->SetCompleted(false);

            // Run the script
            ScriptOperation::OperationResult result;
            result = sequence[current_step]->Run(npc, eventmgr, interrupted);
            interrupted = false; // Reset the interrupted flag after operation has run
            
            // Check the result from the script run operation
            switch (result)
            {
                case ScriptOperation::OPERATION_NOT_COMPLETED:
                {
                    // Operation not completed and should relinquish
                    npc->Printf(2, "Behavior %s step %d - %s will complete later...",
                                name.GetData(),current_step,sequence[current_step]->GetName());
                    return false; // This behavior isn't done yet
                    break;
                }
                case ScriptOperation::OPERATION_COMPLETED:
                {
                    current_step++;  // Continue to the next script operation
                    break;
                }
                case ScriptOperation::OPERATION_FAILED:
                {
                    sequence[current_step]->Failure(npc);
                    current_step = 0; // Restart operation next time
                    DoCompletionDecay(npc);
                    npc->Printf(1, "End of behaviour '%s'",GetName());
                    return true; // This behavior is done
                    break;
                }
            }
        }

        if (current_step >= sequence.GetSize())
        {
            current_step = 0; // behaviors automatically loop around to the top

            if (loop)
            {
                npc->Printf(1, "Loop back to start of behaviour '%s'",GetName());
            }
            else
            {
                DoCompletionDecay(npc);
                npc->Printf(1, "End of non looping behaviour '%s'",GetName());
                return true; // This behavior is done
            }
        }

        // Only loop once per run
        if (start_step == current_step)
        {
            npc->Printf(3,"Terminating behavior '%s' since it has looped all once.",GetName());
            return true; // This behavior is done
        }

    }
    return true; // This behavior is done
}

void Behavior::InterruptScript(NPC *npc,EventManager *eventmgr)
{
    if (current_step < sequence.GetSize() )
    {
        npc->Printf(2,"Interrupting behaviour %s at step %d - %s",
                    name.GetData(),current_step,sequence[current_step]->GetName());

        sequence[current_step]->InterruptOperation(npc,eventmgr);
        interrupted = true;
    }
}

bool Behavior::ResumeScript(NPC *npc,EventManager *eventmgr)
{
    if (current_step < sequence.GetSize())
    {
        npc->Printf(3, "Resuming behavior %s at step %d - %s.",
                    name.GetData(),current_step,sequence[current_step]->GetName());

        if (sequence[current_step]->CompleteOperation(npc,eventmgr))
        {
            npc->Printf(2,"Completed step %d - %s of behavior %s",
                        current_step,sequence[current_step]->GetName(),name.GetData());
            current_step++;
            return RunScript(npc, eventmgr, false);
        }
        else
        {
            return false; // This behavior isn't done yet
        }

    }
    else
    {
        Error2("No script operation to resume for behavior '%s'",GetName());
        return true;
    }
}

//---------------------------------------------------------------------------

psResumeScriptEvent::psResumeScriptEvent(int offsetticks, NPC *which,EventManager *mgr,Behavior *behave,ScriptOperation * script)
: psGameEvent(0,offsetticks,"psResumeScriptEvent")
{
    npc = which;
    eventmgr = mgr;
    behavior = behave;
    scriptOp = script;
}

void psResumeScriptEvent::Trigger()
{
    if (running)
    {
        scriptOp->ResumeTrigger(this);
        npc->ResumeScript(behavior);
    }
}

csString psResumeScriptEvent::ToString() const
{
    csString result;
    result.Format("Resuming script operation %s",scriptOp->GetName());
    if (npc)
    {
        result.AppendFmt("for %s (%s)", npc->GetName(), ShowID(npc->GetEID()));
    }
    return result;
}

//---------------------------------------------------------------------------

void psGameObject::GetPosition(gemNPCObject* object, csVector3& pos, float& yrot,iSector*& sector)
{
    npcMesh * pcmesh = object->pcmesh;

    // Position
    if(!pcmesh->GetMesh())
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR OBJECT %s!\n",object->GetName());
        return;
    }
    
    iMovable* npcMovable = pcmesh->GetMesh()->GetMovable();

    pos = npcMovable->GetPosition();

    // rotation
    csMatrix3 transf = npcMovable->GetTransform().GetT2O();
    yrot = psWorld::Matrix2YRot(transf);
    if (CS::IsNaN(yrot))
    {
        yrot = 0;
    }

    // Sector
    if (npcMovable->GetSectors()->GetCount())
    {
        sector = npcMovable->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }
}

void psGameObject::GetPosition(gemNPCObject* object, csVector3& pos,iSector*& sector)
{
    npcMesh * pcmesh = object->pcmesh;

    // Position
    if(!pcmesh->GetMesh())
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR OBJECT %s!\n",object->GetName());
        return;
    }

    iMovable* npcMovable = pcmesh->GetMesh()->GetMovable();

    pos = npcMovable->GetPosition();

    // Sector
    if (npcMovable->GetSectors()->GetCount())
    {
        sector = npcMovable->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }
}


void psGameObject::SetPosition(gemNPCObject* object, const csVector3& pos, iSector* sector)
{
    npcMesh * pcmesh = object->pcmesh;
    pcmesh->MoveMesh(sector,pos);
}

void psGameObject::SetRotationAngle(gemNPCObject* object, float angle)
{
    npcMesh * pcmesh = object->pcmesh;

    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (angle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);
}

float psGameObject::CalculateIncidentAngle(const csVector3& pos, const csVector3& dest)
{
    csVector3 diff = dest-pos;  // Get vector from player to desired position

    if (!diff.x)
        diff.x = .000001F; // div/0 protect

    float angle = atan2(-diff.x,-diff.z);

    return angle;
}

void psGameObject::ClampRadians(float &target_angle)
{
    if (CS::IsNaN(target_angle)) return;

    // Clamp the angle witin 0 to 2*PI
    while (target_angle < 0)
        target_angle += TWO_PI;
    while (target_angle > TWO_PI)
        target_angle -= TWO_PI;
}

void psGameObject::NormalizeRadians(float &target_angle)
{
    if (CS::IsNaN(target_angle)) return;

    // Normalize angle within -PI to PI
    while (target_angle < PI)
        target_angle += TWO_PI;
    while (target_angle > PI)
        target_angle -= TWO_PI;
}

csVector3 psGameObject::DisplaceTargetPos(const iSector* mySector, const csVector3& myPos, const iSector* targetSector, const csVector3& targetPos , float offset)
{
    csVector3 displace;

    // This prevents NPCs from wanting to occupy the same physical space as something else
    if(mySector == targetSector)
    {
        csVector3 displacement = targetPos - myPos;
        displacement.y = 0;
        displace = offset*displacement.Unit();
    }
    return displace;
}

csVector3 psGameObject::DisplaceTargetPos(const iSector* mySector, const csVector3& myPos, const iSector* targetSector, const csVector3& targetPos , float offset, float angle)
{
    csVector3 displace;

    // This prevents NPCs from wanting to occupy the same physical space as something else
    if(mySector == targetSector)
    {
        csVector3 displacement = targetPos - myPos;
        displacement.y = 0;
        
        csTransform transform;
        displacement = csMatrix3 (0.0,1.0,0.0,angle)*displacement;

        displace = offset*displacement.Unit();
    }
    return displace;
}


float psGameObject::Calc2DDistance(const csVector3 & a, const csVector3 & b)
{
    csVector3 diff = a-b;
    diff.y = 0;
    return diff.Norm();
}


//---------------------------------------------------------------------------
