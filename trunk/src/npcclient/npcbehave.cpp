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
#include <csgeom/path.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <ivaria/collider.h>
#include <cstool/collider.h>


#include <propclass/mesh.h>
#include <physicallayer/entity.h>
#include <physicallayer/propclas.h>
#include <propclass/linmove.h>
#include <propclass/colldet.h>

#include "net/msghandler.h"
#include "net/npcmessages.h"
#include "npcbehave.h"
#include "npc.h"
#include "tribe.h"
#include "perceptions.h"
#include "npcclient.h"
#include "networkmgr.h"
#include "globals.h"
#include "util/log.h"
#include "util/location.h"
#include "util/waypoint.h"
#include "util/pspath.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "engine/psworld.h"

extern bool running;


NPCType::NPCType()
    :ang_vel(999),vel(999),velSource(VEL_DEFAULT)
{
}

NPCType::NPCType(const char *n)
    :name(n),ang_vel(999),vel(999),velSource(VEL_DEFAULT)
{ 
}

NPCType::~NPCType()
{
}

void NPCType::DeepCopy(NPCType& other)
{
    name      = other.name;
    ang_vel   = other.ang_vel;
    velSource = other.velSource;
    vel       = other.vel;

    behaviors.DeepCopy(other.behaviors);

    for (size_t x=0; x<other.reactions.GetSize(); x++)
    {
        reactions.Push( new Reaction(*other.reactions[x],behaviors) );
    }
}

bool NPCType::Load(iDocumentNode *node)
{
    const char *parent = node->GetAttributeValue("parent");
    if (parent) // this npctype is a subclass of another npctype
    {
        NPCType *superclass = npcclient->FindNPCType(parent);
        if (superclass)
        {
            DeepCopy(*superclass);  // This pulls everything from the parent into this one.
        }
        else
        {
            Error2("Specified parent npctype '%s' could not be found.\n",
                parent);
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
    
    if (velStr == "")
    {
        velSource = VEL_DEFAULT;
    } else if (velStr == "$WALK")
    {
        velSource = VEL_WALK;
    } else if (velStr == "$RUN")
    {
        velSource = VEL_RUN;
    } else if (node->GetAttributeValueAsFloat("vel") )
    {
        velSource = VEL_USER;
        vel = node->GetAttributeValueAsFloat("vel");
    }

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
                Error1("Could not load behavior. Error in XML");
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
            // EXCEPT for time reactions!
            if ( strcmp( r->GetEventType(), "time") != 0) {
              for (size_t i=0; i<reactions.GetSize(); i++)
              {
                  if (!strcmp(reactions[i]->GetEventType(),r->GetEventType()))
                  {
                      delete reactions[i];
                      reactions.DeleteIndex(i);
                      break;
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

void NPCType::FirePerception(NPC *npc,EventManager *eventmgr,Perception *pcpt)
{
    for (size_t x=0; x<reactions.GetSize(); x++)
    {
        reactions[x]->React(npc,eventmgr,pcpt);
    }
}

void NPCType::ClearState()
{
    behaviors.ClearState();
}

void NPCType::Advance(csTicks delta,NPC *npc,EventManager *eventmgr)
{
    behaviors.Advance(delta,npc,eventmgr);
}

void NPCType::ResumeScript(NPC *npc,EventManager *eventmgr,Behavior *which)
{
    behaviors.ResumeScript(npc,eventmgr,which);
}

void NPCType::Interrupt(NPC *npc,EventManager *eventmgr)
{
    behaviors.Interrupt(npc,eventmgr);
}

float NPCType::GetAngularVelocity(NPC * /*npc*/)
{
    if (ang_vel != 999)
        return ang_vel;
    else
        return 90*TWO_PI/360;
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

//---------------------------------------------------------------------------

void BehaviorSet::ClearState()
{
    for (size_t i = 0; i<behaviors.GetSize(); i++)
    {
        behaviors[i]->ResetNeed();
        behaviors[i]->SetActive(false);
        behaviors[i]->ClearInterrupted();
    }
    active = NULL;
}

void BehaviorSet::Add(Behavior *b)
{
    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        if (!strcmp(behaviors[i]->GetName(),b->GetName()))
        {
            delete behaviors[i];
            behaviors[i] = b;  // substitute
            return;
        }
    }
    behaviors.Push(b);
}

void BehaviorSet::Advance(csTicks delta,NPC *npc,EventManager *eventmgr)
{
    while (true)
    {
        max_need = -999;
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
                    npc->Printf(3,"Advancing %-30s:\t%1.1f ->%1.1f", 
                                behaviors[i]->GetName(),
                                behaviors[i]->CurrentNeed(),
                                behaviors[i]->NewNeed() );
                    behaviours_changed = true;
                }
                
                if (b->NewNeed() > max_need) // the advance causes re-ordering
                {
                    if (i!=0)  // trivial swap if same element
                    {
                        behaviors[i] = behaviors[0];
                        behaviors[0] = b;  // now highest need is elem 0
                    }
                    max_need = b->NewNeed();
                    behaviours_changed = true;
                }
                b->CommitAdvance();   // Update key to correct value
            }
        }
        // Dump bahaviour list if changed
        if (behaviours_changed && npc->IsDebugging(3))
        {
            npc->DumpBehaviorList();
        }
        
        // now that behaviours are correctly sorted, select the first one
        Behavior *new_behaviour = behaviors[0];
        
        // use it only if need > 0
        if (new_behaviour->CurrentNeed()<=0 || !new_behaviour->ApplicableToNPCState(npc))
        {
            npc->Printf(1,"NO Active applicable behavior." );
            return;
        }
        
        if (new_behaviour != active)
        {
            if (!active)  // if it's the first behaviour ever assigned to this npc
            {
                active = new_behaviour;
                active->SetActive(true);
                if (!active->StartScript(npc,eventmgr))
                {
                    break;
                }
            }
            else
            {
                npc->Printf(1,"Switching behavior from '%s' to '%s'",
                            active->GetName(), 
                            new_behaviour->GetName() );
                
                // Interrupt and stop current behaviour
                active->InterruptScript(npc,eventmgr);
                active->SetActive(false);
                // Set the new active behaviour
                active = new_behaviour;
                // Activate the new behaviour
                active->SetActive(true);
                if (!active->StartScript(npc,eventmgr))
                {
                    break;
                }
                
            }
        }
        else
        {
            break;
        }
    }
    
    npc->Printf(3,"Active behavior is '%s'", active->GetName() );
}

void BehaviorSet::ResumeScript(NPC *npc,EventManager *eventmgr,Behavior *which)
{
    if (which == active && which->ApplicableToNPCState(npc))
    {
        active->ResumeScript(npc,eventmgr);
    }
}

void BehaviorSet::Interrupt(NPC *npc,EventManager *eventmgr)
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
    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        char applicable = 'N';
        if (npc && behaviors[i]->ApplicableToNPCState(npc))
        {
            applicable = 'Y';
        }
        
        CPrintf(CON_CMDOUTPUT, "%c %s%-30s %5.1f %5.1f\n",applicable,
                (behaviors[i]->IsInterrupted()?"*":" "),
                behaviors[i]->GetName(),behaviors[i]->CurrentNeed(),
                behaviors[i]->NewNeed());
    }
}

//---------------------------------------------------------------------------

Behavior::Behavior()
{
    loop = false;
    is_active = false;
    need_decay_rate  = 0;
    need_growth_rate = 0;
    completion_decay = 0;
    new_need=-999;
    interrupted = false;
    resume_after_interrupt = false;
    current_step = 0;
}

Behavior::Behavior(const char *n)
{
    loop = false;
    is_active = false;
    need_decay_rate  = 0;
    need_growth_rate = 0;
    completion_decay = 0;
    new_need=-999;
    interrupted = false;
    resume_after_interrupt = false;
    current_step = 0;
    name = n;
}

void Behavior::DeepCopy(Behavior& other)
{
    loop                    = other.loop;
    is_active               = other.is_active;
    need_decay_rate         = other.need_decay_rate;  // need lessens while performing behavior
    need_growth_rate        = other.need_growth_rate; // need grows while not performing behavior
    completion_decay        = other.completion_decay;
    new_need                = -999;
    name                    = other.name;
    init_need               = other.init_need;
    current_need            = other.current_need;
    last_check              = other.last_check;
    is_applicable_when_dead = other.is_applicable_when_dead;
    resume_after_interrupt  = other.resume_after_interrupt;

    for (size_t x=0; x<other.sequence.GetSize(); x++)
    {
        sequence.Push( other.sequence[x]->MakeCopy() );
    }

    // Instance local variables. No need to copy.
    current_step = 0;
    interrupted             = false;
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

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "locate" ) == 0 )
        {
            op = new LocateOperation;
        }
        else if ( strcmp( node->GetValue(), "navigate" ) == 0 )
        {
            op = new NavigateOperation;
        }
        else if ( strcmp( node->GetValue(), "pickup" ) == 0 )
        {
            op = new PickupOperation;
        }
        else if ( strcmp( node->GetValue(), "equip" ) == 0 )
        {
            op = new EquipOperation;
        }
        else if ( strcmp( node->GetValue(), "dequip" ) == 0 )
        {
            op = new DequipOperation;
        }
        else if ( strcmp( node->GetValue(), "wait" ) == 0 )
        {
            op = new WaitOperation;
        }
        else if ( strcmp( node->GetValue(), "wander" ) == 0 )
        {
            op = new WanderOperation;
        }
        else if ( strcmp( node->GetValue(), "chase" ) == 0 )
        {
            op = new ChaseOperation;
        }
        else if ( strcmp( node->GetValue(), "drop" ) == 0 )
        {
            op = new DropOperation;
        }
        else if ( strcmp( node->GetValue(), "transfer" ) == 0 )
        {
            op = new TransferOperation;
        }
        else if ( strcmp( node->GetValue(), "dig" ) == 0 )
        {
            op = new DigOperation;
        }
        else if ( strcmp( node->GetValue(), "debug" ) == 0 )
        {
            op = new DebugOperation;
        }
        else if ( strcmp( node->GetValue(), "move" ) == 0 )
        {
            op = new MoveOperation;
        }
        else if ( strcmp( node->GetValue(), "circle" ) == 0 )
        {
            op = new CircleOperation;
        }
        else if ( strcmp( node->GetValue(), "moveto" ) == 0 )
        {
            op = new MoveToOperation;
        }
        else if ( strcmp( node->GetValue(), "movepath" ) == 0 )
        {
            op = new MovePathOperation;
        }
        else if ( strcmp( node->GetValue(), "rotate" ) == 0 )
        {
            op = new RotateOperation;
        }
        else if ( strcmp( node->GetValue(), "melee" ) == 0 )
        {
            op = new MeleeOperation;
        }
        else if ( strcmp( node->GetValue(), "loop" ) == 0 )
        {
            op = new BeginLoopOperation;
            beginLoopWhere = (int)sequence.GetSize(); // Where will sequence be pushed
            postLoadBeginLoop = true;
        }
        else if ( strcmp( node->GetValue(), "talk" ) == 0 )
        {
            op = new TalkOperation;
        }
        else if ( strcmp( node->GetValue(), "sequence" ) == 0 )
        {
            op = new SequenceOperation;
        }
        else if ( strcmp( node->GetValue(), "visible" ) == 0 )
        {
            op = new VisibleOperation;
        }
        else if ( strcmp( node->GetValue(), "invisible" ) == 0 )
        {
            op = new InvisibleOperation;
        }
        else if ( strcmp( node->GetValue(), "reproduce" ) == 0 )
        {
            op = new ReproduceOperation;
        }
        else if ( strcmp( node->GetValue(), "resurrect" ) == 0 )
        {
            op = new ResurrectOperation;
        } 
        else if ( strcmp( node->GetValue(), "memorize" ) == 0 )
        {
            op = new MemorizeOperation;
        }
        else
        {
            Error2("Node '%s' under Behavior is not a valid script operation name. Error in XML",node->GetValue() );
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
            BeginLoopOperation * blop = dynamic_cast<BeginLoopOperation*>(op);
            if (!LoadScript(node,false)) // recursively load within loop
            {
                Error1("Could not load within Loop Operation. Error in XML");
                return false;
            }

            EndLoopOperation *op2 = new EndLoopOperation(beginLoopWhere,blop->iterations);
            sequence.Push(op2);
        }
    }

    return true; // success
}

void Behavior::Advance(csTicks delta,NPC *npc,EventManager *eventmgr)
{
    if (new_need == -999)
    {
        new_need = current_need;
    }

    float d = .001 * delta;

    if (is_active)
    {
        new_need = new_need - (d * need_decay_rate);
        if (current_step < sequence.GetSize())
        {
            npc->Printf(4,"%s - Advance active delta: %.3f Need: %.2f Decay Rate: %.2f",
                        name.GetData(),d,new_need,need_decay_rate);

            sequence[current_step]->Advance(d,npc,eventmgr);
        }
    }
    else
    {
        new_need = new_need + (d * need_growth_rate);
        npc->Printf(4,"%s - Advance none active delta: %.3f Need: %.2f Growth Rate: %.2f",
                    name.GetData(),d,new_need,need_growth_rate);
    }
}

bool Behavior::ApplicableToNPCState(NPC *npc)
{
    return npc->IsAlive() || (!npc->IsAlive() && is_applicable_when_dead);
}



bool Behavior::StartScript(NPC *npc,EventManager *eventmgr)
{
    if (interrupted && resume_after_interrupt)
    {
        npc->Printf(1,"Resuming behavior %s after interrupt at step %d.",name.GetData(),current_step);
        interrupted = false;
        return RunScript(npc,eventmgr,true);
    }
    else
    {
        current_step = 0;
        return RunScript(npc,eventmgr,false);
    }
}
   
Behavior* BehaviorSet::Find(Behavior *key)
{
    size_t found = behaviors.Find(key);
    return (found = SIZET_NOT_FOUND) ? NULL : behaviors[found];
}
     
bool Behavior::RunScript(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    while (true)
    {
        while (current_step < sequence.GetSize() )
        {
            npc->Printf(2,">>>Step %d %s operation%s",current_step,sequence[current_step]->GetName(),
                        (interrupted?" Interrupted":""));
            if (!sequence[current_step]->Run(npc,eventmgr,interrupted)) // Run returning false means that
            {                                                           // op is not finished but should 
                                                                        // relinquish
                return false; // This behavior isn't done yet
            }
            interrupted = false; // Only the first script operation should be interrupted.
            current_step++;
        }
        if (current_step == sequence.GetSize())
        {
            if (loop)
            {
                current_step = 0;  // behaviors automatically loop around to the top
                npc->Printf(1,"Loop back to start of behaviour '%s'",GetName());
            }
            else 
            {
                if (completion_decay)
                {
                    float delta_decay = completion_decay;
                    
                    if (completion_decay == -1)
                    {
                        delta_decay = current_need;
                    }

                    npc->Printf("Subtracting completion decay of %1f from behavior '%s'.",delta_decay,GetName() );
                    new_need = current_need - delta_decay;
                }
                npc->Printf(1,"End of non looping behaviour '%s'",GetName());
                break; // This behavior is done
            }
            
        }
    }
    return true; // This behavior is done
}

void Behavior::InterruptScript(NPC *npc,EventManager *eventmgr)
{
    if (current_step < sequence.GetSize() )
    {
        sequence[current_step]->InterruptOperation(npc,eventmgr);
        interrupted = true;
    }
}

bool Behavior::ResumeScript(NPC *npc,EventManager *eventmgr)
{
    npc->Printf("Resuming behavior %s at step %d.",name.GetData(),current_step);
    if (current_step < sequence.GetSize())
    {
        if (sequence[current_step]->CompleteOperation(npc,eventmgr))
        {
            current_step++;
            return RunScript(npc,eventmgr,false);
        }
        else
        {
            return false; // This behavior isn't done yet
        }
        
    }
    else
    {
        current_step=0;
        return RunScript(npc,eventmgr,false);
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


ScriptOperation::ScriptOperation(const char* scriptName)
    : velSource(VEL_DEFAULT),vel(0.0), ang_vel(0.0), completed(true), resumeScriptEvent(NULL), name(scriptName), consec_collisions(0), inside_rgn(true)
{
}

float ScriptOperation::GetVelocity(NPC *npc)
{
    switch (velSource){
    case VEL_DEFAULT:
        return npc->GetVelocity();
    case VEL_USER:
        return vel;
    case VEL_WALK:
        return npc->GetWalkVelocity();
    case VEL_RUN:
        return npc->GetRunVelocity();
    }
    return 0.0; // Should not return
}

float ScriptOperation::GetAngularVelocity(NPC *npc)
{
    if (ang_vel==0)
        return npc->GetAngularVelocity();
    else
        return ang_vel;
}

bool ScriptOperation::LoadVelocity(iDocumentNode *node)
{
    csString velStr = node->GetAttributeValue("vel");
    velStr.Upcase();
    
    if (velStr == "")
    {
        velSource = VEL_DEFAULT;
    } else if (velStr == "$WALK")
    {
        velSource = VEL_WALK;
    } else if (velStr == "$RUN")
    {
        velSource = VEL_RUN;
    } else if (node->GetAttributeValueAsFloat("vel") )
    {
        velSource = VEL_USER;
        vel = node->GetAttributeValueAsFloat("vel");
    }
    return true;
}


void ScriptOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr) 
{
}

void ScriptOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)      
{
    npc->Printf("Interrupting %s",name.GetDataSafe());

    StopResume();

    iCelEntity * entity = npc->GetEntity();
    if (entity)
    {
        psGameObject::GetPosition(entity,interrupted_position,interrupted_angle,interrupted_sector);
    }
}

bool ScriptOperation::AtInterruptedPosition(const csVector3& pos, const iSector* sector)
{
    return (npcclient->GetWorld()->Distance(pos,sector,interrupted_position,interrupted_sector) < 0.5f);
}

bool ScriptOperation::AtInterruptedAngle(const csVector3& pos, const iSector* sector, float angle)
{
    return (npcclient->GetWorld()->Distance(pos,sector,interrupted_position,interrupted_sector) < 0.5f &&
            fabs(angle - interrupted_angle) < 0.01f);
}

bool ScriptOperation::AtInterruptedPosition(NPC *npc)
{
    float      angle;
    csVector3  pos;
    iSector   *sector;

    psGameObject::GetPosition(npc->GetEntity(),pos,angle,sector);
    return AtInterruptedPosition(pos,sector);
}

bool ScriptOperation::AtInterruptedAngle(NPC *npc)
{
    float      angle;
    csVector3  pos;
    iSector   *sector;

    psGameObject::GetPosition(npc->GetEntity(),pos,angle,sector);
    return AtInterruptedAngle(pos,sector,angle);
}

bool ScriptOperation::CheckMovedOk(NPC *npc,EventManager *eventmgr, const csVector3 & oldPos, const csVector3 & newPos, iSector* newSector, float timedelta)
{
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(npc->GetEntity()->GetPropertyClassList(), 
        iPcMesh);

    if ((oldPos - newPos).SquaredNorm() < 0.01f) // then stopped dead, presumably by collision
    {
        Perception collision("collision");
        npc->TriggerEvent(&collision, eventmgr);
        return false;
    }
    else
    {
        csVector3 velvec(0,0,-GetVelocity(npc) );
        // Check for non-stationary collisions
        csReversibleTransform rt = pcmesh->GetMesh()->GetMovable()->GetFullTransform();
        csMatrix3 mat = rt.GetT2O();
        csVector3 expected_pos;
        expected_pos = mat*(velvec*timedelta) + oldPos;

        float diffx = fabs(newPos.x - expected_pos.x);
        float diffz = fabs(newPos.z - expected_pos.z);

        if (diffx > EPSILON ||
            diffz > EPSILON)
        {
            consec_collisions++;
            npc->Printf("Bang (%1.2f,%1.2f)...",diffx,diffz);
            if (consec_collisions > 8)  // allow for hitting trees but not walls
            {
                // after a couple seconds of sliding against something
                // the npc should give up and react to the obstacle.
                Perception collision("collision");
                npc->TriggerEvent(&collision, eventmgr);
                return false;
            }
        }
        else
        {
            consec_collisions = 0;
        }

        LocationType *rgn = npc->GetRegion();

        if (rgn)
        {
            // check for inside/outside region bounds
            if (inside_rgn)
            {
                if (!rgn->CheckWithinBounds(npcclient->GetEngine(),newPos,newSector))
                {
                    Perception outbounds("out of bounds");
                    npc->TriggerEvent(&outbounds, eventmgr);
                    inside_rgn = false;
                }
            }
            else
            {
                if (rgn->CheckWithinBounds(npcclient->GetEngine(),newPos,newSector))
                {
                    Perception inbounds("in bounds");
                    npc->TriggerEvent(&inbounds, eventmgr);
                    inside_rgn = true;
                }
            }
        }
    }

    return true;
}

void ScriptOperation::Resume(csTicks delay, NPC *npc, EventManager *eventmgr)
{
    CS_ASSERT(resumeScriptEvent == NULL);
    
    resumeScriptEvent = new psResumeScriptEvent(delay, npc, eventmgr, npc->GetCurrentBehavior(), this);
    eventmgr->Push(resumeScriptEvent);
}

void ScriptOperation::ResumeTrigger(psResumeScriptEvent * event)
{
    // If we end out getting a trigger from a another event than we currently have
    // registerd something went wrong somewhere.
    CS_ASSERT(event == resumeScriptEvent);
 
    resumeScriptEvent = NULL;
}

void ScriptOperation::StopResume()
{
    if (resumeScriptEvent)
    {
        resumeScriptEvent->SetValid(false);
        resumeScriptEvent = NULL;
    }
}


void ScriptOperation::TurnTo(NPC *npc, csVector3& dest, iSector* destsect, csVector3& forward)
{
    npc->Printf("TurnTo localDest=%s\n",toString(dest,destsect).GetData());

    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(npc->GetEntity()->GetPropertyClassList(), iPcMesh);

    // Turn to face the direction we're going.
    csVector3 pos, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);

    npcclient->GetWorld()->WarpSpace(sector, destsect, pos);

    forward = dest-pos;

    npc->Printf("Forward is %s",toString(forward).GetDataSafe());

    up.Set(0,1,0);
    
    forward.y = 0;

    float angle = psGameObject::CalculateIncidentAngle(pos,dest);
    if (angle < 0) angle += TWO_PI;

    pcmesh->GetMesh()->GetMovable()->GetTransform().LookAt (-forward.Unit(), up.Unit());

    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);
    if (rot < 0) rot += TWO_PI;

    //npc->Printf("Calculated angle is %1.4f, actual is %1.4f for npc %s\n",angle,rot,npc->GetName());

    // Get Going at the right velocity
    csVector3 velvector(0,0,  -GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvector);
    npc->GetLinMove()->SetAngularVelocity( 0 );
}

void ScriptOperation::StopMovement(NPC *npc)
{
    if (!npc->GetEntity()) return; // No enity to stop, returning

    // Stop the movement
    
    // Set Vel to zero again
    npc->GetLinMove()->SetVelocity( csVector3(0,0,0) );
    npc->GetLinMove()->SetAngularVelocity( 0 );

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);
}


int ScriptOperation::StartMoveTo(NPC *npc,EventManager *eventmgr,csVector3& dest, iSector* sector, float vel,const char *action, bool autoresume)
{
    csVector3 forward;
    
    TurnTo(npc, dest, sector, forward);
    
    // SetAction animation for the mesh also, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    float dist = forward.Norm();
    int msec = (int)(1000.0 * dist / GetVelocity(npc)); // Move will take this many msecs.
    
    npc->Printf("MoveTo op should take approx %d msec.  ", msec);
    if (autoresume)
    {
        // wake me up when it's over
        Resume(msec,npc,eventmgr);
        npc->Printf("Waking up in %d msec.\n", msec);
    }
    else
        npc->Printf("NO autoresume here.\n", msec);


    return msec;
}

int ScriptOperation::StartTurnTo(NPC *npc, EventManager *eventmgr, float turn_end_angle, float angle_vel,const char *action, bool autoresume)
{
    csVector3 pos, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);


    // Get Going at the right velocity
    csVector3 velvec(0,0,-GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvec);
    npc->GetLinMove()->SetAngularVelocity(angle_vel);
    
    // SetAction animation for the mesh also, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    float delta_rot = rot-turn_end_angle;
    psGameObject::NormalizeRadians(delta_rot); // -PI to PI

    float time = fabs(delta_rot/angle_vel);

    int msec = (int)(time*1000.0);

    npc->Printf("TurnTo op should take approx %d msec for a turn of %.2f deg in %.2f deg/s.  ",
                msec,delta_rot*180.0/PI,angle_vel*180.0/PI);
    if (autoresume)
    {
        // wake me up when it's over
        Resume(msec,npc,eventmgr);
        npc->Printf("Waking up in %d msec.\n", msec);
    }
    else
        npc->Printf("NO autoresume here.\n", msec);


    return msec;
}

void ScriptOperation::AddRandomRange(csVector3& dest,float radius)
{
    float angle = psGetRandom()*TWO_PI;
    float range = psGetRandom()* radius;

    dest.x += cosf(angle)*range;
    dest.z += sinf(angle)*range;
}

void ScriptOperation::SetAnimation(NPC * npc, const char*name)
{
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(npc->GetEntity()->GetPropertyClassList(), 
        iPcMesh);
    pcmesh->SetAnimation(name, false);
}

//---------------------------------------------------------------------------

bool MoveOperation::Load(iDocumentNode *node)
{
    LoadVelocity(node);
    action = node->GetAttributeValue("anim");
    duration = node->GetAttributeValueAsFloat("duration");
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");
    return true;
}

ScriptOperation *MoveOperation::MakeCopy()
{
    MoveOperation *op = new MoveOperation;
    op->velSource = velSource;
    op->vel    = vel;
    op->action = action;
    op->duration = duration;
    op->ang_vel = ang_vel;
    op->angle   = angle;
    return op;
}

bool MoveOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    // Get Going at the right velocity
    csVector3 velvec(0,0,-GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvec);
    npc->GetLinMove()->SetAngularVelocity(angle<0?-ang_vel:ang_vel);

    // SetAction animation for the mesh also, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    // Note no "wake me up when over" event here.
    // Move just keeps moving the same direction until pre-empted by something else.
    consec_collisions = 0;

    if (!interrupted)
    {
        remaining = duration;
    }

    if(remaining > 0)
    {
        Resume((int)(remaining*1000.0),npc,eventmgr);
    }

    return false;
}

void MoveOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool MoveOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    StopMovement(npc);

    return true;  // Script can keep going
}

void MoveOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    remaining -= timedelta;

    // This updates the position of the entity every 1/2 second so that 
    // range and distance calculations will work when interrupted.

    csVector3 oldPos,newPos;
    float     oldRot,newRot;
    iSector * sector;

    npc->GetLinMove()->GetLastPosition(oldPos,oldRot,sector);
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
    npc->GetLinMove()->GetLastPosition(newPos,newRot,sector);

    psGameObject::SetPosition(npc->GetEntity(), newPos, sector);

    CheckMovedOk(npc,eventmgr,oldPos,newPos,sector,timedelta);
}

//---------------------------------------------------------------------------

bool CircleOperation::Load(iDocumentNode *node)
{
    radius = node->GetAttributeValueAsFloat("radius");
    if (radius == 0)
    {
        Error1("No radius given for Circle operation");
        return false;
    }

    LoadVelocity(node);
    action = node->GetAttributeValue("anim");
    
    angle = node->GetAttributeValueAsFloat("angle")*PI/180;// Calculated if 0 and duration != 0, default 2PI
    duration = node->GetAttributeValueAsFloat("duration"); // Calculated if 0
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");   // Calculated if 0
    return true;
}

ScriptOperation *CircleOperation::MakeCopy()
{
    CircleOperation *op = new CircleOperation;
    op->velSource = velSource;
    op->vel    = vel;
    op->action = action;
    op->duration = duration;
    op->ang_vel = ang_vel;
    op->radius = radius;
    op->angle = angle;

    return op;
}

bool CircleOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    // Calculate parameters not given
    if (angle == 0)
    {
        if (duration != 0)
        {
            angle = duration*radius/GetVelocity(npc);
        }
        else
        {
            angle = 2*PI;
        }
    }
    
    
    if (duration == 0)
    {
        if (angle != 0)
        {
            duration = fabs(angle)*radius/GetVelocity(npc);
        } else
        {
            duration = 2*PI*radius/GetVelocity(npc);
        }
    }
    
    if (ang_vel == 0)
    {
        ang_vel = GetVelocity(npc)/radius;
    }

    return MoveOperation::Run(npc,eventmgr,interrupted);
}

//---------------------------------------------------------------------------

bool MoveToOperation::Load(iDocumentNode *node)
{
    dest.x = node->GetAttributeValueAsFloat("x");
    dest.y = node->GetAttributeValueAsFloat("y");
    dest.z = node->GetAttributeValueAsFloat("z");

    LoadVelocity(node);
    action = node->GetAttributeValue("anim");

    return true;
}

ScriptOperation *MoveToOperation::MakeCopy()
{
    MoveToOperation *op = new MoveToOperation;
    op->velSource = velSource;
    op->vel    = vel;
    op->dest   = dest;
    op->action = action;
    return op;
}

bool MoveToOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("MoveToOp Start dest=(%1.2f,%1.2f,%1.2f) at %1.2f m/sec.\n",
                dest.x,dest.y,dest.z,GetVelocity(npc));

    csVector3 pos, forward, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);
    
    path.SetMaps(npcclient->GetMaps());
    path.SetDest(dest);
    path.CalcLocalDest(pos, sector, localDest);
    
    // Using "true" teleports to dest location after proper time has 
    // elapsed and is therefore more tolerant of CD errors.
    // StartMoveTo(npc,eventmgr,localDest, sector,vel,action, false);
    StartMoveTo(npc, eventmgr, localDest, sector,vel,action, true); 
    return false;
}

void MoveToOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    csVector3 pos,pos2;
    float     rot;
    iSector * sector;
    csVector3 forward;

    npc->GetLinMove()->GetLastPosition(pos,rot,sector);
    
    npc->Printf("advance: pos=(%1.2f,%1.2f,%1.2f) rot=%.2f localDest=(%.2f,%.2f,%.2f) dest=(%.2f,%.2f,%.2f) dist=%f\n", 
                pos.x,pos.y,pos.z, rot,
                localDest.x,localDest.y,localDest.z,
                dest.x,dest.y,dest.z,
                Calc2DDistance(localDest, pos));
    
    TurnTo(npc, localDest, sector, forward);
    
    //tolerance must be according to step size
    //we must ignore y
    if (Calc2DDistance(localDest, pos) <= 0.5)
    {
        pos.x = localDest.x;
        pos.z = localDest.z;
        npc->GetLinMove()->SetPosition(pos,rot,sector);
        
        if (Calc2DDistance(localDest,dest) <= 0.5) //tolerance must be according to step size, ignore y
        {
            npc->Printf("MoveTo within minimum acceptable range...Stopping him now.");
            // npc->ResumeScript(eventmgr, npc->GetBrain()->GetCurrentBehavior() );
            CompleteOperation(npc, eventmgr);
        }
        else
        {
            npc->Printf("we are at localDest... WHAT DOES THIS MEAN?");
            path.CalcLocalDest(pos, sector, localDest);
            StartMoveTo(npc,eventmgr,localDest, sector, vel,action, false);
        }
    }
    else
    {
        npc->GetLinMove()->ExtrapolatePosition(timedelta);
        npc->GetLinMove()->GetLastPosition(pos2,rot,sector);
    
        if ((pos-pos2).SquaredNorm() < SMALL_EPSILON) // then stopped dead, presumably by collision
        {
            Perception collision("collision");
            npc->TriggerEvent(&collision, eventmgr);
        }
    }
}

bool MoveToOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    // Get the rot and sector here so they don't change in the SetPosition call
    float rot;
    iSector *sector;
    csVector3 pos;
    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);

    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(dest,rot,sector);

    // Stop the movement
    StopMovement(npc);
    
    npc->Printf("MoveTo Completed. pos=(%1.2f,%1.2f,%1.2f) rot=%.2f dest set=(%1.2f,%1.2f,%1.2f)",
                pos.x,pos.y,pos.z, rot,
                dest.x,dest.y,dest.z);

    return true;  // Script can keep going
}

//---------------------------------------------------------------------------

bool RotateOperation::Load(iDocumentNode *node)
{
    csString type = node->GetAttributeValue("type");
    ang_vel       = node->GetAttributeValueAsFloat("ang_vel")*TWO_PI/360.0f;
    action        = node->GetAttributeValue("anim");

    if (type == "inregion")
    {
        op_type = ROT_REGION;
        min_range = node->GetAttributeValueAsFloat("min")*TWO_PI/360.0f;
        max_range = node->GetAttributeValueAsFloat("max")*TWO_PI/360.0f;
        return true;
    }
    else if (type == "random")
    {
        op_type = ROT_RANDOM;
        min_range = node->GetAttributeValueAsFloat("min")*TWO_PI/360.0f;
        max_range = node->GetAttributeValueAsFloat("max")*TWO_PI/360.0f;
        return true;
    }
    else if (type == "absolute")
    {
        op_type = ROT_ABSOLUTE;
        target_angle = node->GetAttributeValueAsFloat("value")*TWO_PI/360.0f;
        return true;
    }
    else if (type == "relative")
    {
        op_type = ROT_RELATIVE;
        delta_angle = node->GetAttributeValueAsFloat("value")*TWO_PI/360.0f;
        return true;
    }
    else if (type == "locatedest")
    {
        op_type = this->ROT_LOCATEDEST;
        return true;
    }
    else if (type == "target")
    {
        op_type = this->ROT_TARGET;
        return true;
    }
    else
    {
        Error1("Rotate Op type must be 'random', 'absolute', 'relative', "
               "'target' or 'locatedest' right now.\n");
    }
    return false;
}

ScriptOperation *RotateOperation::MakeCopy()
{
    RotateOperation *op = new RotateOperation;
    op->action = action;
    op->op_type = op_type;
    op->max_range = max_range;
    op->min_range = min_range;
    op->ang_vel = ang_vel;
    op->delta_angle = delta_angle;
    op->target_angle = target_angle;

    return op;
}

bool RotateOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    float rot;
    csVector3 pos;
    iSector* sector;
    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);

    float ang_vel = GetAngularVelocity(npc);

    if (interrupted)
    {
        npc->Printf("Interrupted rotation to %.2f deg",target_angle*180.0f/PI);

        angle_delta =  target_angle - rot;
    }
    else if (op_type == ROT_RANDOM || op_type == ROT_REGION)
    {
        LocationType *rgn = npc->GetRegion();

        // Start the turn
        bool verified = false;
        int count=0;
        float rot_angle=0;
        while (rgn && op_type == ROT_REGION && !verified && count<10)
        {
            // Find range of allowable angles to turn inwards to region again
            float min_angle = TWO_PI, max_angle = -TWO_PI;

            for (size_t i=0; i<rgn->locs.GetSize(); i++)
            {
                rot_angle = psGameObject::CalculateIncidentAngle(pos,rgn->locs[i]->pos);
                if (min_angle > rot_angle)
                    min_angle = rot_angle+.05;
                if (max_angle < rot_angle)
                    max_angle = rot_angle-.05;  // The .05 is so it doesn't aim straight for the corner
            }
            if (max_angle-min_angle  > 3.14159 )
            {
                float temp=max_angle;  
                max_angle=min_angle+TWO_PI;  
                min_angle=temp;
            }

            // Pick an angle in that range
            target_angle = SeekAngle(npc, psGetRandom() * (max_angle-min_angle) + min_angle);


            verified = true;
        }
        if (!rgn || op_type == ROT_RANDOM || !verified)
        {
            target_angle = SeekAngle(npc, psGetRandom() * (max_range - min_range)+min_range - PI);
        }

        // Save target angle so we can jam that in on Rotate completion.
        angle_delta = target_angle - rot;
    }
    else if (op_type == ROT_LOCATEDEST)
    {
        csVector3 dest;
        float     dest_rot;
        iSector  *dest_sector;

        npc->GetActiveLocate(dest,dest_sector,dest_rot);

        if(pos == dest && sector == dest_sector && rot == dest_rot)
        {
            npc->Printf("At located destination, end rotation.");
            return true;
        }
        
        target_angle = psGameObject::CalculateIncidentAngle(pos,dest);

        angle_delta = target_angle-rot;

        // If the angle is close enough don't worry about it and just go to next command.
        if (fabs(angle_delta) < TWO_PI/60.0)
        {
            npc->Printf("Rotation at destination angle. Ending rotation.");
            return true;
        }
    }
    else if (op_type == ROT_ABSOLUTE)
    {
        npc->Printf("Absolute rotation to %.2f deg",target_angle*180.0f/PI);
        
        angle_delta =  target_angle - rot;
    }
    else if (op_type == ROT_RELATIVE)
    {
        npc->Printf("Relative rotation by %.2f deg",delta_angle*180.0f/PI);

        angle_delta = delta_angle;
        target_angle = rot + angle_delta;
    }
    else
    {
        Error1("ERROR: No known rotation type defined");
        return true;
    }

    psGameObject::NormalizeRadians(angle_delta); // -PI to PI
    psGameObject::ClampRadians(target_angle);    // 0 to 2*PI

    npc->GetLinMove()->SetAngularVelocity( csVector3(0,(angle_delta>0)?-ang_vel:ang_vel,0) );
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    // wake me up when it's over
    int msec = (int)fabs(1000.0*angle_delta/ang_vel);
    Resume(msec,npc,eventmgr);

    npc->Printf("Rotating %1.2f deg from %1.2f to %1.2f at %1.2f deg/sec in %.3f sec.\n",
                angle_delta*180/PI,rot*180.0f/PI,target_angle*180.0f/PI,ang_vel*180.0f/PI,msec/1000.0f);

    return false;
}

void RotateOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr) 
{
    npc->Printf("RotateOp advanced\n"); 

    npc->GetLinMove()->ExtrapolatePosition(timedelta);
}

float RotateOperation::SeekAngle(NPC* npc, float targetYRot)
{
    // Try to avoid big ugly stuff in our path
    float rot;
    iSector *sector;
    csVector3 pos;
    psGameObject::GetPosition(npc->GetEntity(),pos,rot,sector);

    csVector3 isect,start,end,dummy,box,legs;
    iPcCollisionDetection* pcDummy;

    // Construct the feeling broom

    // Calculate the start and end poses
    start = pos;
    npc->GetLinMove()->GetCDDimensions(box,legs,dummy,pcDummy);

    // We can walk over some stuff
    start += csVector3(0,0.6f,0);
    end = start + csVector3(sinf(targetYRot), 0, cosf(targetYRot)) * -2;

    // Feel
    csIntersectingTriangle closest_tri;
    iMeshWrapper* sel = 0;
    float dist = csColliderHelper::TraceBeam (npcclient->GetCollDetSys(), sector,
        start, end, true, closest_tri, isect, &sel);


    if(dist > 0)
    {
        const float begin = (PI/6); // The lowest turning constant
        const float length = 2;

        float left,right,turn = 0;
        for(int i = 1; i <= 3;i++)
        {
            csVector3 broomStart[2],broomEnd[2];

            // Left and right
            left = targetYRot - (begin * float(i));
            right = targetYRot + (begin * float(i));

            // Construct the testing brooms
            broomStart[0] = start;
            broomEnd[0] = start + csVector3(sinf(left),0,cosf(left)) * -length;

            broomStart[1] = start;
            broomEnd[1] = start + csVector3(sinf(right),0,cosf(right)) * -length;

            // The broom is already 0.6 over the ground, so we need to cut that out
            //broomStart[0].y += legs.y + box.y - 0.6;
            //broomEnd[1].y += legs.y + box.y - 0.6;

            // Check if we can get the broom through where we want to go
            float dist = csColliderHelper::TraceBeam (npcclient->GetCollDetSys(), sector,
                broomStart[0], broomEnd[0], true, closest_tri, isect, &sel);

            if(dist < 0)
            {
                npc->Printf("Turning left!\n");
                turn = left;
                break;
            }

            // Do again for the other side
            dist = csColliderHelper::TraceBeam (npcclient->GetCollDetSys(), sector,
                broomStart[1], broomEnd[1], true, closest_tri, isect, &sel);

            if(dist < 0)
            {
                npc->Printf("Turning right!\n");
                turn = right;
                break;
            }


        }
        if (turn==0.0)
        {
            npc->Printf("Possible ERROR: turn value was 0");
        }
        // Apply turn
        targetYRot = turn;
    }
    return targetYRot;
}

void RotateOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool RotateOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    // Set target angle and stop the turn
    psGameObject::SetRotationAngle(npc->GetEntity(),target_angle);
    StopMovement(npc);

    return true;  // Script can keep going
}

//---------------------------------------------------------------------------

bool LocateOperation::Load(iDocumentNode *node)
{

    object = node->GetAttributeValue("obj");
    static_loc = node->GetAttributeValueAsBool("static",false);
    if (node->GetAttribute("range"))
    {
        range  = node->GetAttributeValueAsFloat("range");
    }
    else
    {
        range = -1;
    }
    random = node->GetAttributeValueAsBool("random",false);
    locate_invisible = node->GetAttributeValueAsBool("invisible",false);
    locate_invincible = node->GetAttributeValueAsBool("invincible",false);

    return true;
}

ScriptOperation *LocateOperation::MakeCopy()
{
    LocateOperation *op = new LocateOperation;
    op->range  = range;
    op->object = object;
    op->static_loc = static_loc;
    op->random = random;
    op->locate_invisible = locate_invisible;
    op->locate_invincible = locate_invincible;

    return op;
}


Waypoint* LocateOperation::CalculateWaypoint(NPC *npc, csVector3 located_pos, iSector* located_sector, float located_range)
{
    Waypoint *end;
    float end_range = 0.0;

    end   = npcclient->FindNearestWaypoint(located_pos,located_sector,-1,&end_range);

    if (end && (located_range == -1 || end_range >= located_range))
    {
        npc->Printf("Located WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        return end;
    }

    return NULL;
}


bool LocateOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    // Reset old target
    npc->SetTarget(NULL);

    located_pos = csVector3(0.0f,0.0f,0.0f);
    located_angle = 0.0f;
    located_sector = NULL;
    located_wp = NULL;

    float start_rot;
    iSector *start_sector;
    csVector3 start_pos;
    psGameObject::GetPosition(npc->GetEntity(),start_pos,start_rot,start_sector);

    csArray<csString> split_obj = psSplit(object,':');

    // Substitute variables
    for (size_t ii = 0; ii < split_obj.GetSize(); ii++)
    {
        if (strcasecmp(split_obj[ii].GetDataSafe(),"$name")== 0)
        {
            split_obj[ii] = npc->GetName();
        } else if (strcasecmp(split_obj[ii].GetDataSafe(),"$race")== 0)
        {
            split_obj[ii] = npc->GetRaceInfo()->GetName();
        } else if (strcasecmp(split_obj[ii].GetDataSafe(),"$tribe")== 0)
        {
            split_obj[ii] = npc->GetTribe()->GetName();
        }
        
    }

    if (split_obj[0] == "perception")
    {
        npc->Printf(3,"LocateOp - Perception");

        if (!npc->GetLastPerception())
            return true;
        if (!npc->GetLastPerception()->GetLocation(located_pos,located_sector))
            return true;
        located_angle = 0; // not used in perceptions
    }
    else if (split_obj[0] == "target")
    {
        npc->Printf(3,"LocateOp - Target");

        iCelEntity *ent;
        // Since we don't have a current enemy targeted, find one!
        if (range)
            ent = npc->GetMostHated(range,locate_invisible,locate_invincible);
        else
            ent = npc->GetMostHated(10.0f,locate_invisible,locate_invincible);     // Default enemy range

        if(ent)
            npc->SetTarget(ent);
        else
            return true;

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "owner")
    {
        npc->Printf(3,"LocateOp - Owner");

        iCelEntity *ent;
        // Since we don't have a current enemy targeted, find one!
        ent = npc->GetOwner();

        if(ent)
            npc->SetTarget(ent);
        else
            return true;

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "self")
    {
        npc->Printf(3,"LocateOp - Self");

        iCelEntity *ent;

        ent = npc->GetEntity();

        if(ent)
            npc->SetTarget(ent);
        else
            return true;

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "tribe")
    {
        npc->Printf(3,"LocateOp - Tribe");

        if (!npc->GetTribe())
            return true;

        if (split_obj[1] == "home")
        {
            float radius;
            csVector3 pos;
            npc->GetTribe()->GetHome(pos,radius,located_sector);
            
            AddRandomRange(pos,radius);
            
            located_pos = pos;
            located_angle = 0;
            
        }
        else if (split_obj[1] == "memory")
        {
            float located_range=0.0;
            psTribe::Memory * memory;

            if (random)
            {
                memory = npc->GetTribe()->FindRandomMemory(split_obj[2],start_pos,start_sector,range,&located_range);
            }
            else
            {
                memory = npc->GetTribe()->FindNearestMemory(split_obj[2],start_pos,start_sector,range,&located_range);
            }
            
            if (!memory)
            {
                npc->Printf("Couldn't locate any <%s> in npc script for <%s>.",
                            (const char *)object,npc->GetEntity()->GetName() );
                return true;
            }
            located_pos = memory->pos;
            located_sector = memory->sector;
            
            AddRandomRange(located_pos,memory->radius);
        }
        else if (split_obj[1] == "resource")
        {
            npc->GetTribe()->GetResource(npc,start_pos,start_sector,located_pos,located_sector,range,random);
            located_angle = 0.0;
        }

        located_wp = CalculateWaypoint(npc,located_pos,located_sector,-1);
    }
    else if(split_obj[0] == "friend")
    {
        npc->Printf(3,"LocateOp - Friend");

        iCelEntity *ent = npc->GetNearestVisibleFriend(20);
        if(ent)
            npc->SetTarget(ent);
        else
            return true;

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "waypoint" )
    {
        npc->Printf(3,"LocateOp - Waypoint");

        float located_range=0.0;

        if (split_obj.GetSize() >= 2)
        {
            located_wp = npcclient->FindWaypoint(split_obj[1]);
            if (located_wp)
            {
                located_range = npcclient->GetWorld()->Distance(start_pos,start_sector,located_wp->loc.pos,located_wp->GetSector(npcclient->GetEngine()));
            }
        }
        else if (random)
        {
            located_wp = npcclient->FindRandomWaypoint(start_pos,start_sector,range,&located_range);
        }
        else
        {
            located_wp = npcclient->FindNearestWaypoint(start_pos,start_sector,range,&located_range);
        }

        if (!located_wp)
        {
            npc->Printf("Couldn't locate any <%s> in npc script for <%s>.\n",
                (const char *)object,npc->GetEntity()->GetName() );
            return true;
        }
        npc->Printf("Located waypoint: %s at %s range %.2f",located_wp->GetName(),
                    toString(located_wp->loc.pos,located_wp->loc.GetSector(npcclient->GetEngine())).GetData(),located_range);
        located_pos = located_wp->loc.pos;
        located_angle = located_wp->loc.rot_angle;
        located_sector = located_wp->loc.GetSector(npcclient->GetEngine());

        AddRandomRange(located_pos,located_wp->loc.radius);
        
        located_wp = CalculateWaypoint(npc,located_pos,located_sector,-1);
    }
    else if (!static_loc || !located)
    {
        npc->Printf(3,"LocateOp - Location");

        float located_range=0.0;
        Location * location;

        if (split_obj.GetSize() >= 2)
        {
            location = npcclient->FindLocation(split_obj[0],split_obj[1]);
        }
        else if (random)
        {
            location = npcclient->FindRandomLocation(split_obj[0],start_pos,start_sector,range,&located_range);
        }
        else
        {
            location = npcclient->FindNearestLocation(split_obj[0],start_pos,start_sector,range,&located_range);
        }

        if (!location)
        {
            npc->Printf("Couldn't locate any <%s> in npc script for <%s>.\n",
                (const char *)object,npc->GetEntity()->GetName() );
            return true;
        }
        located_pos = location->pos;
        located_angle = location->rot_angle;
        located_sector = location->GetSector(npcclient->GetEngine());

        AddRandomRange(located_pos,location->radius);
        
        if (static_loc)
            located = true;  // if it is a static location, we only have to do this locate once, and save the answer

        located_wp = CalculateWaypoint(npc,located_pos,located_sector,located_range);

    }
    else
    {
        npc->Printf("remembered location from last time\n");
    }

    // Save on npc so other operations can refer to value
    npc->SetActiveLocate(located_pos,located_sector,located_angle,located_wp);

    npc->Printf("LocateOp - Active location: pos %s rot %.2f wp %s",
                toString(located_pos,located_sector).GetData(),located_angle,
                (located_wp?located_wp->GetName():"(NULL)"));

    return true;
}

//---------------------------------------------------------------------------

bool NavigateOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    return true;
}

ScriptOperation *NavigateOperation::MakeCopy()
{
    NavigateOperation *op = new NavigateOperation;
    op->action = action;
    op->velSource = velSource;
    op->vel    = vel;

    return op;
}

bool NavigateOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf(">>>NavigateOp ");

    csVector3 dest;
    float rot=0;
    iSector* sector;

    npc->GetActiveLocate(dest,sector,rot);
    npc->Printf("Located %s at %1.2f m/sec.\n",toString(dest,sector).GetData(), GetVelocity(npc) );

    StartMoveTo(npc,eventmgr,dest,sector,GetVelocity(npc),action);
    return false;
}

void NavigateOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr) 
{
    npc->Printf("NavigationOp advanced\n"); 

    npc->GetLinMove()->ExtrapolatePosition(timedelta);
}

void NavigateOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool NavigateOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    npc->Printf("Navigate completed. Stopping now.\n");

    // Stop the movement
    StopMovement(npc);

    // Set position to where it is supposed to go
    float rot=0;
    iSector *sector;
    csVector3 pos;
    npc->GetActiveLocate(pos,sector,rot);
    npc->GetLinMove()->SetPosition(pos,rot,sector);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return true;  // Script can keep going
}

//---------------------------------------------------------------------------

void WanderOperation::CalculateTargetPos(csVector3& dest, iSector*&sector)
{
    dest = active_wp->loc.pos;
    sector = active_wp->loc.GetSector(npcclient->GetEngine());
}

Waypoint * WanderOperation::GetNextRandomWaypoint(NPC *npc, Waypoint * prior_wp, Waypoint * active_wp)
{
    while (true)
    {
        int which_next = psGetRandom((int)active_wp->links.GetSize() );
        Waypoint *new_wp = active_wp->links[which_next];
        bool wander = !active_wp->prevent_wander[which_next];
        
        if (((new_wp != prior_wp) && wander) || 
            (new_wp == prior_wp && new_wp->allow_return) ||
            (active_wp->links.GetSize() == 1))
        {
            return new_wp;
        }
    }
    return NULL;
}


bool WanderOperation::FindNextWaypoint(NPC *npc)
{
    float rot=0;
    psGameObject::GetPosition(npc->GetEntity(),current_pos,rot,current_sector);

    if (random)
    {
        prior_wp = active_wp;
        active_wp = next_wp;
        next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);

        npc->Printf("Active waypoint: %s at %s",active_wp->GetName(),
                    toString(active_wp->loc.pos,
                             active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        npc->Printf("Next   waypoint: %s at %s",next_wp->GetName(),
                    toString(next_wp->loc.pos,
                             next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        return true;
    }
    else
    {
        prior_wp = active_wp;
        active_wp = next_wp;
        if (!active_wp) active_wp = WaypointListGetNext();
        next_wp = WaypointListGetNext();

        if (active_wp)
        {
            // Check if we are on the waypoint, in that case find next
            if (active_wp->CheckWithin(npcclient->GetEngine(),current_pos,current_sector))
            {
                if (FindNextWaypoint(npc))
                {
                    return true;
                }
                else
                {
                    npc->Printf(">>>WanderOp At end of waypoint list.");
                    return false;
                }
            } else
            {
                npc->Printf("Active waypoint: %s at %s",active_wp->GetName(),
                            toString(active_wp->loc.pos,
                                     active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                if (next_wp)
                {
                    npc->Printf("Next   waypoint: %s at %s",next_wp->GetName(),
                                toString(next_wp->loc.pos,
                                         next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                }
                return true;
            }
        } else
        {
            npc->Printf(">>>WanderOp At end of waypoint list.");
            return false;
        }
        
    }
    return false;
}

bool WanderOperation::CalculateWaypointList(NPC *npc)
{
    float start_rot;
    csVector3 start_pos;
    iSector* start_sector;
    
    psGameObject::GetPosition(npc->GetEntity(), start_pos, start_rot, start_sector);

    if (random)
    {
        if (active_wp == NULL)
        {
            active_wp = npcclient->FindNearestWaypoint(start_pos,start_sector);
            next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);
        }
        return true;
    }
    else
    {
        Waypoint *start, *end;

        npc->GetActiveLocate(end);
        if (!end)
        {
            return false;
        }
        start = npcclient->FindNearestWaypoint(start_pos,start_sector);
        
        WaypointListClear();
        
        if (start && end)
        {
            npc->Printf("Start WP: %30s at %s",start->GetName(),toString(start->loc.pos,start->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            npc->Printf("End WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            
            if (start == end)
            {
                WaypointListPushBack(start);
            } else
            {
                csList<Waypoint*> wps;
                
                wps = npcclient->FindWaypointRoute(start,end);
                if (wps.IsEmpty())
                {
                    npc->Printf("Can't find route...");
                    return false;
                }
                
                while (!wps.IsEmpty())
                {
                    WaypointListPushBack(wps.Front());
                    wps.PopFront();
                }
            }
            
            if (npc->IsDebugging(5))
            {
                npcclient->ListWaypoints("");
            }
            
            psString wp_str;
            if (!WaypointListEmpty())
            {
                csList<Waypoint*> wps = WaypointListGet();
                Waypoint * wp;
                while (!wps.IsEmpty() && (wp = wps.Front()))
                {
                    wp_str.AppendFmt("%s",wp->GetName());
                    wps.PopFront();
                    if (!wps.IsEmpty())
                    {
                        wp_str.Append(" -> ");
                    }
                }
                npc->Printf("Waypoint list: %s",wp_str.GetDataSafe());
            }
        }
    }

    return true;
}

bool WanderOperation::StartMoveToWaypoint(NPC *npc,EventManager *eventmgr)
{
    // now calculate new destination from new active wp
    CalculateTargetPos(dest,dest_sector);

    // Find path and return direction for that path between wps 
    path = npcclient->FindPath(prior_wp,active_wp,direction);
    
    if (!path)
    {
        Error4("%s Could not find path between '%s' and '%s'",
               npc->GetName(),
               (prior_wp?prior_wp->GetName():""),
               (active_wp?active_wp->GetName():""));
        return false;
    }

    if (anchor)
    {
        delete anchor;
        anchor = NULL;
    }

    anchor = path->CreatePathAnchor();

    // Get Going at the right velocity
    csVector3 velvector(0,0,  -GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvector);
    npc->GetLinMove()->SetAngularVelocity( 0 );

    npcclient->GetNetworkMgr()->QueueDRData(npc);


    return true;
}

bool WanderOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    if (interrupted && AtInterruptedPosition(npc))
    {
        // Restart current behavior
        if (turning)
        {
            StartTurnTo(npc,eventmgr,turn_end_angle,turn_angle_vel,action);
        }
        else
        {
            StartMoveTo(npc,eventmgr,dest,dest_sector,GetVelocity(npc),action);
        }
        return false; // This behavior isn't done yet
    }
    // If interruped and not at interruped position we do the same
    // as we do when started.

    active_wp = NULL;

    if (!CalculateWaypointList(npc))
    {
        npc->Printf(">>>WanderOp no list to wander");
        return true; // Nothing more to do for this op.
    }
    

    if (!FindNextWaypoint(npc))
    {
        npc->Printf(">>>WanderOp NO waypoints, %s cannot move.",npc->GetName());
        return true; // Nothing more to do for this op.
    }

    // Turn of CD and hug the ground
    npc->GetCD()->UseCD(false);
    npc->GetCD()->SetOnGround(true); // Wander is ALWAYS on_ground.  This ensures correct animation on the client.
    npc->GetLinMove()->SetHugGround(true);

    if (StartMoveToWaypoint(npc, eventmgr))
    {
        return false; // This behavior isn't done yet
    }
    
    return true; // This behavior is done
}

void WanderOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    if (!anchor->Extrapolate(npcclient->GetWorld(),npcclient->GetEngine(),
                             timedelta*GetVelocity(npc),
                             direction,npc->GetMovable()))
    {
        // At end of path
        npc->Printf("we are done..\n");

        npc->ResumeScript(eventmgr, npc->GetBrain()->GetCurrentBehavior() );
    }
    
    if (npc->IsDebugging())
    {
        csVector3 pos; float rot; iSector *sec;
        psGameObject::GetPosition(npc->GetEntity(),pos,rot,sec);

        npc->Printf("Wander Loc is %s Rot: %1.2f Vel: %.2f Dist: %.2f Index: %d Fraction %.2f\n",
                    toString(pos).GetDataSafe(),rot,GetVelocity(npc),anchor->GetDistance(),anchor->GetCurrentAtIndex(),anchor->GetCurrentAtFraction());
        
        csVector3 anchor_pos,anchor_up,anchor_forward;

        anchor->GetInterpolatedPosition(anchor_pos);
        anchor->GetInterpolatedUp(anchor_up);
        anchor->GetInterpolatedForward(anchor_forward);
        

        npc->Printf("Anchor pos: %s forward: %s up: %s",toString(anchor_pos).GetDataSafe(),
                    toString(anchor_forward).GetDataSafe(),toString(anchor_up).GetDataSafe());
        
    }
    

    // None linear movement so we have to queue DRData updates.
    npcclient->GetNetworkMgr()->QueueDRData(npc);
}

void WanderOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool WanderOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    // Wander never really completes, so this finds the next
    // waypoint and heads toward it again.
    npc->Printf("WanderOp - Waypoint %s found.",active_wp->GetName() );

    if (!next_wp)
    {
        // Only set position if at a end point.
        psGameObject::SetPosition(npc->GetEntity(),dest,dest_sector);
        current_pos = dest;  // make sure waypoint is starting point of next path
    }

    if (FindNextWaypoint(npc))
    {
        StartMoveToWaypoint(npc, eventmgr);

        return false;  // Script requeues termination event so this CompleteOp is essentially an infinite loop
    }

    // Stop the movement
    StopMovement(npc);

    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(path->GetEndPos(direction),path->GetEndRot(direction),path->GetEndSector(npcclient->GetEngine(),direction));

    if (anchor)
    {
        delete anchor;
        anchor = NULL;
    }

    // Turn on CD again
    npc->GetCD()->UseCD(true);
    npc->GetLinMove()->SetHugGround(false);

    npc->Printf("WanderOp - Completed.");
    return true; // Script can keep going, no more waypoints.
}

bool WanderOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    LoadVelocity(node);
    random = node->GetAttributeValueAsBool("random",false);  // Random wander never ends

    return true;
}

ScriptOperation *WanderOperation::MakeCopy()
{
    WanderOperation *op = new WanderOperation;
    op->action      = action;
    op->velSource = velSource;
    op->vel         = vel;
    op->random      = random;

    return op;
}

Waypoint* WanderOperation::WaypointListGetNext()
{
    if (waypoint_list.IsEmpty()) return NULL;

    Waypoint * wp;
    wp = waypoint_list.Front();
    if (wp)
    {
        waypoint_list.PopFront();
        return wp;
    }
    return NULL;
}

/*
void WanderOperation::CalculateTargetPos(csVector3& dest, iSector*&sector)
{
    dest = active_wp->loc.pos;
    sector = active_wp->loc.GetSector(npcclient->GetEngine());
    AddRandomRange(dest,active_wp->loc.radius);
}

Waypoint * WanderOperation::GetNextRandomWaypoint(NPC *npc, Waypoint * prior_wp, Waypoint * active_wp)
{
    while (true)
    {
        int which_next = psGetRandom((int)active_wp->links.GetSize() );
        Waypoint *new_wp = active_wp->links[which_next];
        bool wander = !active_wp->prevent_wander[which_next];
        
        if (((new_wp != prior_wp) && wander) || 
            (new_wp == prior_wp && new_wp->allow_return) ||
            (active_wp->links.GetSize() == 1))
        {
            return new_wp;
        }
    }
    return NULL;
}


bool WanderOperation::FindNextWaypoint(NPC *npc)
{
    float rot=0;
    psGameObject::GetPosition(npc->GetEntity(),current_pos,rot,current_sector);

    if (random)
    {
        prior_wp = active_wp;
        active_wp = next_wp;
        next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);

        npc->Printf("Active waypoint: %s at %s",active_wp->GetName(),
                    toString(active_wp->loc.pos,
                             active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        npc->Printf("Next   waypoint: %s at %s",next_wp->GetName(),
                    toString(next_wp->loc.pos,
                             next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        return true;
    }
    else
    {
        prior_wp = active_wp;
        active_wp = next_wp;
        if (!active_wp) active_wp = WaypointListGetNext();
        next_wp = WaypointListGetNext();

        if (active_wp)
        {
            // Check if we are on the waypoint, in that case find next
            if (active_wp->CheckWithin(npcclient->GetEngine(),current_pos,current_sector))
            {
                if (FindNextWaypoint(npc))
                {
                    return true;
                }
                else
                {
                    npc->Printf(">>>WanderOp At end of waypoint list.");
                    return false;
                }
            } else
            {
                npc->Printf("Active waypoint: %s at %s",active_wp->GetName(),
                            toString(active_wp->loc.pos,
                                     active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                if (next_wp)
                {
                    npc->Printf("Next   waypoint: %s at %s",next_wp->GetName(),
                                toString(next_wp->loc.pos,
                                         next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                }
                return true;
            }
        } else
        {
            npc->Printf(">>>WanderOp At end of waypoint list.");
            return false;
        }
        
    }
    return false;
}

bool WanderOperation::CalculateWaypointList(NPC *npc)
{
    float start_rot;
    csVector3 start_pos;
    iSector* start_sector;
    
    psGameObject::GetPosition(npc->GetEntity(), start_pos, start_rot, start_sector);

    if (random)
    {
        if (active_wp == NULL)
        {
            active_wp = npcclient->FindNearestWaypoint(start_pos,start_sector);
            next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);
        }
        return true;
    }
    else
    {
        Waypoint *start, *end;

        npc->GetActiveLocate(end);
        if (!end)
        {
            return false;
        }
        start = npcclient->FindNearestWaypoint(start_pos,start_sector);
        
        WaypointListClear();
        
        if (start && end)
        {
            npc->Printf("Start WP: %30s at %s",start->GetName(),toString(start->loc.pos,start->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            npc->Printf("End WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            
            if (start == end)
            {
                WaypointListPushBack(start);
            } else
            {
                csList<Waypoint*> wps;
                
                wps = npcclient->FindWaypointRoute(start,end);
                if (wps.IsEmpty)
                {
                    npc->Printf("Can't find route...");
                    return false;
                }
                
                while (!wps.IsEmpty())
                {
                    WaypointListPushBack(wps.Front());
                    wps.PopFront();
                }
            }
            
            if (npc->IsDebugging(5))
            {
                npcclient->ListWaypoints("");
            }
            
            psString wp_str;
            if (!WaypointListEmpty())
            {
                csList<Waypoint*> wps = WaypointListGet();
                Waypoint * wp;
                while (!wps.IsEmpty() && (wp = wps.Front()))
                {
                    wp_str.AppendFmt("%s",wp->GetName());
                    wps.PopFront();
                    if (!wps.IsEmpty())
                    {
                        wp_str.Append(" -> ");
                    }
                }
                npc->Printf("Waypoint list: %s",wp_str.GetDataSafe());
            }
        }
    }

    return true;
}

void WanderOperation::StartMoveToWaypoint(NPC *npc,EventManager *eventmgr)
{
    // now calculate new destination from new active wp
    CalculateTargetPos(dest,dest_sector);

    csVector3 pos;
    csVector3 dest_tmp = dest; // Destination in current sector space
    float rot;
    iSector* sector;
    psGameObject::GetPosition(npc->GetEntity(), pos, rot, sector);

    npcclient->GetWorld()->WarpSpace(dest_sector,sector,dest_tmp);
    
    if (next_wp)
    {
        csVector3 next_pos = next_wp->loc.pos;
        iSector*  next_sec = next_wp->loc.GetSector(npcclient->GetEngine());
        // Convert to current sector coordinate space
        npcclient->GetWorld()->WarpSpace(next_sec,sector,next_pos);

        float dest_rot = psGameObject::CalculateIncidentAngle(pos,dest_tmp);
        float next_rot = psGameObject::CalculateIncidentAngle(dest_tmp,next_pos);
        float radius = active_wp->loc.radius/2.0;
        float vel = GetVelocity(npc);

        float delta_rot = dest_rot-next_rot;
        psGameObject::NormalizeRadians(delta_rot); // -PI to PI

        // Don't turn if there are no or only a smal change in direction or
        // smal radius.
        if (fabs(delta_rot) > 5.0*PI/180.0 && radius > 0.25)
        {
            

            float length1 = (dest_tmp - current_pos).SquaredNorm();
            float length2 = (next_pos - dest_tmp).SquaredNorm();
            float rot_angle = (PI-delta_rot)/2.0; // 1/2 of the angle between the two legs
            float dist2 = radius*sinf(rot_angle);
            // Check that we don't start to turn before where we stand
            if (dist2 > length1)
            {
                dist2 = length1;
                radius = dist2/sinf(rot_angle); // Adjust radius down
            }
            // Check that we don't end the turn after next_point
            if (dist2 > length2)
            {
                dist2 = length2;
                radius = dist2/sinf(rot_angle); // Adjust radius down
            }
            
            
            float arc_length = radius*fabs(rot_angle);
            float time = arc_length/vel;
            float angle_vel = delta_rot/time;
            
            turn_queued = true;
            turning = false;
            turn_angle_vel = angle_vel;
            turn_end_angle = next_rot;

            npc->Printf("Turn: angle_vel: %.2f deg/s End angle: %.2f deg",
                        turn_angle_vel*180.0/PI, turn_end_angle*180.0/PI);
            
            dest -= (dest_tmp - current_pos)*(dist2/length1);
            npc->Printf("Dest position: %s",toString(dest).GetData());
        }
        else
        {
            npc->Printf("Cant turn because angle to small: %.2f or small radius: %0.2f",
                        delta_rot*180.0/PI,radius);
        }
        
    }
    else
    {
        turn_queued = false;
        turning = false;
    }

    StartMoveTo(npc,eventmgr,dest,dest_sector,GetVelocity(npc),action);
}

bool WanderOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    if (interrupted && AtInterruptedPosition(npc))
    {
        // Restart current behavior
        if (turning)
        {
            StartTurnTo(npc,eventmgr,turn_end_angle,turn_angle_vel,action);
        }
        else
        {
            StartMoveTo(npc,eventmgr,dest,dest_sector,GetVelocity(npc),action);
        }
        return false; // This behavior isn't done yet
    }
    // If interruped and not at interruped position we do the same
    // as we do when started.

    active_wp = NULL;

    if (!CalculateWaypointList(npc))
    {
        npc->Printf(">>>WanderOp no list to wander",npc->GetName());
        return true; // Nothing more to do for this op.
    }
    

    if (!FindNextWaypoint(npc))
    {
        npc->Printf(">>>WanderOp NO waypoints, %s cannot move.",npc->GetName());
        return true; // Nothing more to do for this op.
    }

    // Turn of CD and hug the ground
    npc->GetCD()->UseCD(false);
    npc->GetCD()->SetOnGround(true); // Wander is ALWAYS on_ground.  This ensures correct animation on the client.
    npc->GetLinMove()->SetHugGround(true);

    StartMoveToWaypoint(npc, eventmgr);

    return false; // This behavior isn't done yet
}

void WanderOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    // This updates the position of the entity every 1/2 second so that 
    // range and distance calculations will work when interrupted.

    csVector3 oldPos;
    float     rot;
    iSector * sector;

    npc->GetLinMove()->GetLastPosition(oldPos,rot,sector);
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
    npc->GetLinMove()->GetLastPosition(current_pos,rot,current_sector);

    psGameObject::SetPosition(npc->GetEntity(), current_pos, current_sector);

    // Perhaps check for nearby npcs and dodge them here

    npc->Printf("Waypoint updated to %s\n", 
                toString(current_pos,current_sector).GetData());

    CheckMovedOk(npc,eventmgr,oldPos,current_pos,current_sector,timedelta);
}

void WanderOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool WanderOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    if (next_wp && turn_queued)
    {
        turn_queued = false;
        turning     = true;

        StartTurnTo(npc,eventmgr,turn_end_angle,turn_angle_vel,action);

        // Note no "wake me up when over" event here.
        // Move just keeps moving the same direction until pre-empted by something else.
        consec_collisions = 0;

        return false; // Don't terminate behavior just yet, complete the turn first       
    }
    turning = false;

    // Wander never really completes, so this finds the next
    // waypoint and heads toward it again.
    npc->Printf("WanderOp - Waypoint %s found.",active_wp->GetName() );

    if (!next_wp)
    {
        // Only set position if at a end point.
        psGameObject::SetPosition(npc->GetEntity(),dest,dest_sector);
        current_pos = dest;  // make sure waypoint is starting point of next path
    }

    if (FindNextWaypoint(npc))
    {
        StartMoveToWaypoint(npc, eventmgr);

        return false;  // Script requeues termination event so this CompleteOp is essentially an infinite loop
    }

    npc->Printf("WanderOp - Stop movement.");

    // Stop the movement
    StopMovement(npc);

    // Turn on CD again
    npc->GetCD()->UseCD(true);
    npc->GetLinMove()->SetHugGround(false);

    return true; // End script if no more waypoints.
}

bool WanderOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    LoadVelocity(node);
    random = node->GetAttributeValueAsBool("random",false);  // Random wander never ends

    // Internal variables set to defaults
    anchor = NULL;
    path = NULL;
    return true;
}

ScriptOperation *WanderOperation::MakeCopy()
{
    WanderOperation *op = new WanderOperation;
    op->action      = action;
    op->velSource = velSource;
    op->vel         = vel;
    op->random      = random;

    // Internal variables set to defaults
    anchor = NULL;
    path = NULL;
    return op;
}

Waypoint* WanderOperation::WaypointListGetNext()
{
    if (waypoint_list.IsEmpty()) return NULL;

    Waypoint * wp;
    wp = waypoint_list.Front();
    if (wp)
    {
        waypoint_list.PopFront();
        return wp;
    }
    return NULL;
}
*/

//---------------------------------------------------------------------------

bool ChaseOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    csString typestr = node->GetAttributeValue("type");
    if (typestr == "nearest")
        type = NEAREST;
    else if (typestr == "boss")
        type = OWNER;
    else if (typestr == "owner")
        type = OWNER;
    else if (typestr == "target")
        type = TARGET;
    else
        type = UNKNOWN;

    if (node->GetAttributeValue("range"))
        range = node->GetAttributeValueAsFloat("range");
    else
        range = 2;

    if ( node->GetAttributeValue("offset_x") )
        offset.x = node->GetAttributeValueAsFloat("offset_x");
    else
        offset.x = 0.5F;
    
    if ( node->GetAttributeValue( "offset_z" ) )
        offset.z = node->GetAttributeValueAsFloat("offset_z");
    else
        offset.z = 0.5F;

    offset.y = 0.0f;

    LoadVelocity(node);
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");

    return true;
}

ScriptOperation *ChaseOperation::MakeCopy()
{
    ChaseOperation *op = new ChaseOperation;
    op->action = action;
    op->type   = type;
    op->range  = range;
    op->velSource = velSource;
    op->vel    = vel;
    op->ang_vel= ang_vel;
    op->offset = offset;

    return op;
}

bool ChaseOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("Starting ChaseOp");
    float targetRot;
    iSector* targetSector;
    csVector3 targetPos;
    
    float myRot;
    iSector* mySector;
    csVector3 myPos;

    csVector3 dest;
    csString name;
    iCelEntity *entity = NULL;
    target_id = (uint32_t)~0;
        
    switch (type)
    {
        case NEAREST:
            npc->GetNearestEntity(target_id,dest,name,range);
            npc->Printf("Targeting nearest entity (%s) at (%1.2f,%1.2f,%1.2f) for chase ...\n",
                        (const char *)name,dest.x,dest.y,dest.z);
            if ( target_id != (uint32_t)-1 ) entity = npcclient->FindEntity(target_id);
            break;
        case OWNER:
            entity = npc->GetOwner();
            if (entity)
            {
                target_id = entity->GetID();
                psGameObject::GetPosition(entity, dest,targetRot,targetSector);
                npc->Printf("Targeting owner (%s) at (%1.2f,%1.2f,%1.2f) for chase ...\n",
                            entity->GetName(),dest.x,dest.y,dest.z );

            }
            break;
        case TARGET:
            entity = npc->GetTarget();
            if (entity)
            {
                target_id = entity->GetID();
                psGameObject::GetPosition(entity, dest,targetRot,targetSector);
                npc->Printf("Targeting current target (%s) at (%1.2f,%1.2f,%1.2f) for chase ...\n",
                            entity->GetName(),dest.x,dest.y,dest.z );
            }
            break;
    }

    if ( ( target_id != (uint32_t)-1 ) && entity )
    {
        psGameObject::GetPosition(npc->GetEntity(),myPos,myRot,mySector);
    
        psGameObject::GetPosition(entity, targetPos,targetRot,targetSector);
        npc->Printf(5,"Chasing enemy EID: %u at %f %f %f\n",entity->GetID(), targetPos.x,targetPos.y,targetPos.z);

        // We need to work in the target sector space
        npcclient->GetWorld()->WarpSpace(mySector, targetSector, myPos);

        // This prevents NPCs from wanting to occupy the same physical space as something else
        csVector3 displacement = targetPos - myPos;
        float factor = sqrt((offset.x * offset.x)+(offset.z * offset.z)) / displacement.Norm();
        targetPos = displacement - factor * displacement;

        path.SetMaps(npcclient->GetMaps());
        path.SetDest(targetPos);
        path.CalcLocalDest(myPos, mySector, localDest);
        if ( Calc2DDistance( myPos, targetPos ) < .1 )
        {
            return true;  // This operation is complete
        }
        else if ( GetAngularVelocity(npc) > 0 || GetVelocity(npc) > 0 )
        {
            StartMoveTo(npc,eventmgr,localDest,targetSector, GetVelocity(npc),action, false);
            return false;
        }
        else
            return true;  // This operation is complete
    }
    else
    {
        npc->Printf("No one found to chase!");
        return true;  // This operation is complete
    }

    return true; // This operation is complete
}

void ChaseOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{

    csVector3 myPos,myNewPos,targetPos;
    float     myRot,dummyrot;
    iSector * mySector, *targetSector;
    csVector3 forward;
    
    npc->GetLinMove()->GetLastPosition(myPos,myRot,mySector);
    

    // Now turn towards entity being chased again
    csString name;
    
    if (type == NEAREST)
    {
        npc->GetNearestEntity(target_id,targetPos,name,range);
    }
 
    iCelEntity *target_entity = npcclient->FindEntity(target_id);

    if (!target_entity) // no entity close to us
    {
        npc->Printf("ChaseOp has no target now!");
        return;
    }
    
    if(name.IsEmpty())
    {
        name = target_entity->GetName();
    }
    
    psGameObject::GetPosition(target_entity,targetPos,dummyrot,targetSector);

    // We now work in the target sector's space
    csVector3 myOldPos(myPos);
    npcclient->GetWorld()->WarpSpace(mySector, targetSector, myPos);

    // This prevents NPCs from wanting to occupy the same physical space as something else
    csVector3 displacement = targetPos - myPos;
    float factor = sqrt((offset.x * offset.x)+(offset.z * offset.z)) / displacement.Norm();
    targetPos = myPos + (1 - factor) * displacement;

    npc->Printf("Still chasing %s at (%1.2f,%1.2f,%1.2f)...\n",(const char *)name,targetPos.x,targetPos.y,targetPos.z);
    
    float angleToTarget = psGameObject::CalculateIncidentAngle(myPos, targetPos);
    csVector3 pathDest = path.GetDest();
    float angleToPath  = psGameObject::CalculateIncidentAngle(myPos, pathDest);

        
    // if the target diverged from the end of our path, we must calculate it again
    if ( fabs( AngleDiff(angleToTarget, angleToPath) ) > EPSILON  )
    {
        npc->Printf("turn to target..\n");
        path.SetDest(targetPos);
        //        path.CalcLocalDest(myPos, mySector, localDest);
        path.CalcLocalDest(myPos, targetSector, localDest);
        StartMoveTo(npc,eventmgr,localDest, targetSector, GetVelocity(npc),action, false);
    }
    
    
    if (Calc2DDistance(localDest, myPos) <= 0.5)
    {
        myPos.x = localDest.x;
        myPos.z = localDest.z;
        npc->GetLinMove()->SetPosition(myPos,myRot,mySector);
        
        if (Calc2DDistance(myPos,targetPos) <= 0.5)
        {
            npc->Printf("we are done..\n");
            npc->ResumeScript(eventmgr, npc->GetBrain()->GetCurrentBehavior() );
            return;
        }
        else
        {
            npc->Printf("we are at localDest..\n");
            path.SetDest(targetPos);
            path.CalcLocalDest(myPos, targetSector, localDest);
            StartMoveTo(npc,eventmgr,localDest, targetSector,vel,action, false);
        }
    }
    else
        TurnTo(npc,localDest, targetSector,forward);


    npc->Printf("advance: pos=(%f.2,%f.2,%f.2) rot=%.2f localDest=(%f.2,%f.2,%f.2) dist=%f\n", 
                myPos.x,myPos.y,myPos.z, myRot,
                localDest.x,localDest.y,localDest.z,
                Calc2DDistance(localDest, myPos));

    {ScopedTimer st(250, "chase extrapolate %.2f time for EID: %u",timedelta,npc->GetEntity()->GetID());
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
    }
    npc->GetLinMove()->GetLastPosition(myNewPos,myRot,mySector);
    if((fabs(myPos.x)> 1000 || fabs(myNewPos.x)> 1000) || (fabs(myPos.y)>1000 || fabs(myNewPos.y)>1000) || (fabs(myPos.z)>1000 || fabs(myNewPos.z)>1000))
    {
        npc->Printf("Moved from %f %f %f to %f %f %f, timedelta is %f, chase error!\n", myPos.x,myPos.y,myPos.z, myNewPos.x,myNewPos.y,myNewPos.z, timedelta);
    }

    // This check must be done in our original sector's space
    if ((myOldPos - myNewPos).SquaredNorm() < SMALL_EPSILON) // then stopped dead, presumably by collision
    {
        Perception collision("collision");
        npc->TriggerEvent(&collision, eventmgr);
        npc->Printf("Collided! Moving from %f %f %f to %f %f %f, timedelta is %f!\n", myPos.x,myPos.y,myPos.z, myNewPos.x,myNewPos.y,myNewPos.z, timedelta);
    }
}

void ChaseOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
    
    StopMovement(npc);
}

bool ChaseOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    npc->Printf("Chase completed. Stopping now.\n");

    StopMovement(npc);

    return true;  // Script can keep going
}

//---------------------------------------------------------------------------

bool PickupOperation::Load(iDocumentNode *node)
{
    object = node->GetAttributeValue("obj");
    slot   = node->GetAttributeValue("equip");
    count   = node->GetAttributeValueAsInt("count");
    if (count <= 0) count = 1; // Allways pick up at least one.
    return true;
}

ScriptOperation *PickupOperation::MakeCopy()
{
    PickupOperation *op = new PickupOperation;
    op->object = object;
    op->slot   = slot;
    op->count  = count;
    return op;
}

bool PickupOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("PickupOp ");

    iCelEntity *item = NULL;

    if (object == "perception")
    {
        if (!npc->GetLastPerception())
            return true;
        if (!(item = npc->GetLastPerception()->GetEntity()))
            return true;
    } else
    {
        // TODO: Insert code to find the nearest item
        //       with name given by object.
        return true;
    }

    npc->Printf("   Who: %s What: %s Count: %d",npc->GetName(),item->GetName(), count);

    npcclient->GetNetworkMgr()->QueuePickupCommand(npc->GetEntity(), item, count);

    return true;
}

//---------------------------------------------------------------------------

bool EquipOperation::Load(iDocumentNode *node)
{
    item    = node->GetAttributeValue("item");
    slot    = node->GetAttributeValue("slot");
    count   = node->GetAttributeValueAsInt("count");
    if (count <= 0) count = 1; // Allways equip at least one.
    return true;
}

ScriptOperation *EquipOperation::MakeCopy()
{
    EquipOperation *op = new EquipOperation;
    op->item   = item;
    op->slot   = slot;
    op->count  = count;
    return op;
}

bool EquipOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("EquipOp ");

    npc->Printf("   Who: %s What: %s Where: %s Count: %d",
                npc->GetName(),item.GetData(), slot.GetData(), count);

    npcclient->GetNetworkMgr()->QueueEquipCommand(npc->GetEntity(), item, slot, count);

    return true;
}

//---------------------------------------------------------------------------

bool DequipOperation::Load(iDocumentNode *node)
{
    slot   = node->GetAttributeValue("slot");
    return true;
}

ScriptOperation *DequipOperation::MakeCopy()
{
    DequipOperation *op = new DequipOperation;
    op->slot   = slot;
    return op;
}

bool DequipOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("DequipOp ");

    npc->Printf("   Who: %s Where: %s",
                npc->GetName(), slot.GetData());

    npcclient->GetNetworkMgr()->QueueDequipCommand(npc->GetEntity(), slot );

    return true;
}

//---------------------------------------------------------------------------

bool TalkOperation::Load(iDocumentNode *node)
{
    text = node->GetAttributeValue("text");
    target = node->GetAttributeValueAsBool("target");
    command = node->GetAttributeValue("command");

    return true;
}

ScriptOperation *TalkOperation::MakeCopy()
{
    TalkOperation *op = new TalkOperation;
    op->text = text;
    op->target = target;
    op->command = command;
    return op;
}

bool TalkOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{

    if(!target)
    {
        npcclient->GetNetworkMgr()->QueueTalkCommand(npc->GetEntity(), npc->GetName() + text);
        return true;
    }
    
    if(!npc->GetTarget())
        return true;

    NPC* friendNPC = npcclient->FindAttachedNPC(npc->GetTarget());
    if(friendNPC)
    {
        npcclient->GetNetworkMgr()->QueueTalkCommand(npc->GetEntity(), npc->GetName() + text);
        
        Perception collision("friend:" + command);
        friendNPC->TriggerEvent(&collision, eventmgr);
    }
    return true;
}

//---------------------------------------------------------------------------

bool SequenceOperation::Load(iDocumentNode *node)
{
    name = node->GetAttributeValue("name");
    csString cmdStr = node->GetAttributeValue("cmd");
    
    if (strcasecmp(cmdStr,"start") == 0)
    {
        cmd = START;
    } else if (strcasecmp(cmdStr,"stop") == 0)
    {
        cmd = STOP;
    } else if (strcasecmp(cmdStr,"loop") == 0)
    {
        cmd = LOOP;
    } else
    {
        return false;
    }
        
    count = node->GetAttributeValueAsInt("count");
    if (count < 0)
    {
        count = 1;
    }

    return true;
}

ScriptOperation *SequenceOperation::MakeCopy()
{
    SequenceOperation *op = new SequenceOperation;
    op->name = name;
    op->cmd = cmd;
    op->count = count;
    return op;
}

bool SequenceOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{

    npcclient->GetNetworkMgr()->QueueSequenceCommand(name, cmd, count );
    return true;
}

//---------------------------------------------------------------------------

bool VisibleOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *VisibleOperation::MakeCopy()
{
    VisibleOperation *op = new VisibleOperation;
    return op;
}

bool VisibleOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf(">>>VisibleOp\n");
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetEntity(), true);
    return true;
}

//---------------------------------------------------------------------------

bool InvisibleOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *InvisibleOperation::MakeCopy()
{
    InvisibleOperation *op = new InvisibleOperation;
    return op;
}

bool InvisibleOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf(">>>InvisibleOp\n");
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetEntity(), false);
    return true;
}

//---------------------------------------------------------------------------

bool ReproduceOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *ReproduceOperation::MakeCopy()
{
    ReproduceOperation *op = new ReproduceOperation;
    return op;
}

bool ReproduceOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    if(!npc->GetTarget())
        return true;

    NPC * friendNPC = npcclient->FindAttachedNPC(npc->GetTarget());
    if(friendNPC)
    {
        npc->Printf(">>> Reproduce");
        npcclient->GetNetworkMgr()->QueueSpawnCommand(friendNPC->GetEntity(), friendNPC->GetTarget());
    }

    return true;
}

//---------------------------------------------------------------------------

bool ResurrectOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *ResurrectOperation::MakeCopy()
{
    ResurrectOperation *op = new ResurrectOperation;
    return op;
}

bool ResurrectOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf(">>> Resurrect");

    psTribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return true;

    csVector3 where;
    float     radius;
    iSector*  sector;
    float     rot = 0; // Todo: Set to a random rotation

    tribe->GetHome(where,radius,sector);

    // Todo: Add a random delta within radius to the where value.

    npcclient->GetNetworkMgr()->QueueResurrectCommand(where,rot,sector->QueryObject()->GetName(),npc->GetPID());

    return true;
}

//---------------------------------------------------------------------------

bool MemorizeOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *MemorizeOperation::MakeCopy()
{
    MemorizeOperation *op = new MemorizeOperation;
    return op;
}

bool MemorizeOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    Perception * percept = npc->GetLastPerception();
    if (!percept)
    {
        npc->Printf(">>> Memorize No Perception.");
        return true; // Nothing more to do for this op.
    }
    
    npc->Printf(">>> Memorize '%s' '%s'.",percept->GetType(),percept->GetName());

    psTribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return true; // Nothing more to do for this op.

    tribe->Memorize(npc, percept );

    return true; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool MeleeOperation::Load(iDocumentNode *node)
{
    seek_range   = node->GetAttributeValueAsFloat("seek_range");
    melee_range  = node->GetAttributeValueAsFloat("melee_range");
    attack_invisible = node->GetAttributeValueAsBool("invisible",false);
    attack_invincible= node->GetAttributeValueAsBool("invincible",false);
    // hardcoded in server atm to prevent npc/server conflicts
    melee_range  = 3.0f;
    return true;
}

ScriptOperation *MeleeOperation::MakeCopy()
{
    MeleeOperation *op = new MeleeOperation;
    op->seek_range  = seek_range;
    op->melee_range = melee_range;
    op->attack_invisible = attack_invisible;
    op->attack_invincible = attack_invincible;
    attacked_ent = NULL;
    return op;
}

bool MeleeOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("MeleeOperation starting with seek range (%.2f) will attack:%s%s.",
                seek_range,(attack_invisible?" Invisible":" Visible"),
                (attack_invincible?" Invincible":""));

    completed = false;

    attacked_ent = npc->GetMostHated(seek_range,attack_invisible,attack_invincible);
    if (attacked_ent)
    {
        npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetEntity(),attacked_ent);
    }
    else
    {
        // We know who attacked us, even if they aren't in range.
        npc->SetTarget( npc->GetLastPerception()->GetEntity() );
    }

    return false; // This behavior isn't done yet
}

void MeleeOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    if (completed)
    {
        npc->Printf("Melee completed");
        return;
    }

    // Check hate list to make sure we are still attacking the right person
    iCelEntity *ent = npc->GetMostHated(melee_range,attack_invisible,attack_invincible);
    if (!ent)
    {
        npc->Printf("No Melee target in range (%2.2f), going to chase!",melee_range);

        // No enemy to whack on in melee range, search far
        ent = npc->GetMostHated(seek_range,attack_invisible,attack_invincible);

        // The idea here is to save the next best target and chase
        // him if out of range.
        if (ent)
        {
            npc->SetTarget(ent);
        
            // If the chase doesn't work, it will return to fight, which still
            // may not find a target, and return to chase.  This -5 reduces
            // the need to fight as he can't find anyone and stops this infinite
            // loop.
            npc->GetCurrentBehavior()->ApplyNeedDelta(-5);

            Perception range("target out of range");
            npc->TriggerEvent(&range, eventmgr);
        }
        else // no hated targets around
        {
            if(npc->IsDebugging(5))
            {
                npc->DumpHateList();
            }
            npc->Printf("No hated target in seek range (%2.2f)!",seek_range);
            npc->GetCurrentBehavior()->ApplyNeedDelta(-5); // don't want to fight as badly
        }
        return;
    }
    if (ent != attacked_ent)
    {
        npc->Printf("Melee switching to attack %s\n",ent->GetName() );
        attacked_ent = ent;
        npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetEntity(),ent);
    }
}

void MeleeOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
    
    npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetEntity(),NULL);
}

bool MeleeOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    npc->Printf("Completing melee operation");
    npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetEntity(),NULL);
    completed = true;

    return true;
}

//---------------------------------------------------------------------------

bool BeginLoopOperation::Load(iDocumentNode *node)
{
    iterations = node->GetAttributeValueAsInt("iterations");
    return true;
}

ScriptOperation *BeginLoopOperation::MakeCopy()
{
    BeginLoopOperation *op = new BeginLoopOperation;
    op->iterations = iterations;
    return op;
}

bool BeginLoopOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("BeginLoop");
    return true;
}

//---------------------------------------------------------------------------

bool EndLoopOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *EndLoopOperation::MakeCopy()
{
    EndLoopOperation *op = new EndLoopOperation(loopback_op,iterations);
    return op;
}

bool EndLoopOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    Behavior * behavior = npc->GetCurrentBehavior();

    current++;

    if (current < iterations)
    {
        behavior->SetCurrentStep(loopback_op-1);
        npc->Printf("EndLoop - Loop %d of %d",current,iterations);
        return true;
    }

    current = 0; // Make sure we will loop next time to

    npc->Printf("EndLoop - Exit %d %d",current,iterations);
    return true;
}

//---------------------------------------------------------------------------

bool WaitOperation::Load(iDocumentNode *node)
{
    duration = node->GetAttributeValueAsFloat("duration");
    action = node->GetAttributeValue("anim");
    return true;
}

ScriptOperation *WaitOperation::MakeCopy()
{
    WaitOperation *op = new WaitOperation;
    op->duration = duration;
    op->action   = action;
    return op;
}

bool WaitOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf(">>>WaitOp\n");

    if (!interrupted)
    {
        remaining = duration;
    }

    // SetAction animation for the mesh, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    Resume((int)(remaining*1000.0),npc,eventmgr);

    return false;
}

void WaitOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    remaining -= timedelta;
    npc->Printf("waiting... %.2f",remaining);
}

//---------------------------------------------------------------------------

bool DropOperation::Load(iDocumentNode *node)
{
    slot = node->GetAttributeValue("slot");
    return true;
}

ScriptOperation *DropOperation::MakeCopy()
{
    DropOperation *op = new DropOperation;
    op->slot = slot;
    return op;
}

bool DropOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("DropOp ");

    npcclient->GetNetworkMgr()->QueueDropCommand(npc->GetEntity(), slot );

    return true;
}

//---------------------------------------------------------------------------

bool TransferOperation::Load(iDocumentNode *node)
{
    item = node->GetAttributeValue("item");
    target = node->GetAttributeValue("target");
    count = node->GetAttributeValueAsInt("count");
    if (!count) count = -1; // All items

    if (item.IsEmpty() || target.IsEmpty()) return false;

    return true;
}

ScriptOperation *TransferOperation::MakeCopy()
{
    TransferOperation *op = new TransferOperation;
    op->item = item;
    op->target = target;
    op->count = count;
    return op;
}

bool TransferOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("TransferOp ");

    csString transferItem = item;

    csArray<csString> splitItem = psSplit(transferItem,':');
    if (splitItem[0] == "tribe")
    {
        if (!npc->GetTribe())
        {
            npc->Printf("No tribe");
            return true;
        }
        
        if (splitItem[1] == "wealth")
        {
            transferItem = npc->GetTribe()->GetNeededResource();
        }
        else
        {
            Error2("Transfer operation for tribe with unknown sub type %s",splitItem[1].GetDataSafe())
            return true;
        }
        
    }
    
    npcclient->GetNetworkMgr()->QueueTransferCommand(npc->GetEntity(), transferItem, count, target );

    return true;
}

//---------------------------------------------------------------------------

bool DigOperation::Load(iDocumentNode *node)
{
    resource = node->GetAttributeValue("resource");
    if (resource == "") return false;
    return true;
}

ScriptOperation *DigOperation::MakeCopy()
{
    DigOperation *op = new DigOperation;
    op->resource = resource;
    return op;
}

bool DigOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    npc->Printf("DigOp ");

    if (resource == "tribe:wealth")
    {
        if (npc->GetTribe())
        {
            npcclient->GetNetworkMgr()->QueueDigCommand(npc->GetEntity(), npc->GetTribe()->GetNeededResourceNick());
        }
    }
    else
    {
        npcclient->GetNetworkMgr()->QueueDigCommand(npc->GetEntity(), resource );
    }
    

    return true;
}

//---------------------------------------------------------------------------

bool DebugOperation::Load(iDocumentNode *node)
{
    exclusive = node->GetAttributeValue("exclusive");
    level = node->GetAttributeValueAsInt("level");
    return true;
}

ScriptOperation *DebugOperation::MakeCopy()
{
    DebugOperation *op = new DebugOperation;
    op->exclusive = exclusive;
    op->level = level;
    return op;
}

bool DebugOperation::Run(NPC *npc,EventManager *eventmgr,bool interrupted)
{
    if (exclusive.Length())
    {
        static bool debug_exclusive = false;
        
        if (level && debug_exclusive)
        {
            // Can't turn on when exclusive is set.
            return true;
        }
        if (level)
        {
            debug_exclusive = true;
        }
        else
        {
            debug_exclusive = false;
        }
    }

    
    if (!level) // Print before when turning off
    {
        npc->Printf("DebugOp Set debug %d",level);
    }
    
    npc->SetDebugging(level);
    
    if (level) // Print after when turning on
    {
        npc->Printf("DebugOp Set debug %d",level);            
    }

    return true;
}

//---------------------------------------------------------------------------

bool MovePathOperation::Load(iDocumentNode *node)
{
    pathname = node->GetAttributeValue("path");
    anim = node->GetAttributeValue("anim");
    csString dirStr = node->GetAttributeValue("direction");
    if (strcasecmp(dirStr.GetDataSafe(),"REVERSE")==0)
    {
        direction = psPath::REVERSE;
    }
    else
    {
        direction = psPath::FORWARD;
    }

    // Internal variables set to defaults
    path = NULL;
    anchor = NULL;
    return true;
}

ScriptOperation *MovePathOperation::MakeCopy()
{
    MovePathOperation *op = new MovePathOperation;
    // Copy parameters
    op->pathname  = pathname;
    op->anim      = anim;
    op->direction = direction;

    // Internal variables set to defaults
    op->path     = NULL;
    op->anchor   = NULL;

    return op;
}

bool MovePathOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (!path)
    {
        path = npcclient->FindPath(pathname);
    }

    if (!path)
    {
        Error2("Could not find path '%s'",pathname.GetDataSafe());
        return true;
    }

    anchor = path->CreatePathAnchor();

    // Get Going at the right velocity
    csVector3 velvector(0,0,  -GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvector);
    npc->GetLinMove()->SetAngularVelocity( 0 );

    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return false;
}

void MovePathOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{

    if (!anchor->Extrapolate(npcclient->GetWorld(),npcclient->GetEngine(),
                             timedelta*GetVelocity(npc),
                             direction,npc->GetMovable()))
    {
        // At end of path
        npc->Printf("we are done..\n");

        npc->ResumeScript(eventmgr, npc->GetBrain()->GetCurrentBehavior() );
    }
    
    if (npc->IsDebugging())
    {
        csVector3 pos; float rot; iSector *sec;
        psGameObject::GetPosition(npc->GetEntity(),pos,rot,sec);

        npc->Printf("MovePath Loc is %s Rot: %1.2f Vel: %.2f Dist: %.2f Index: %d Fraction %.2f\n",
                    toString(pos).GetDataSafe(),rot,GetVelocity(npc),anchor->GetDistance(),anchor->GetCurrentAtIndex(),anchor->GetCurrentAtFraction());
        
        csVector3 anchor_pos,anchor_up,anchor_forward;

        anchor->GetInterpolatedPosition(anchor_pos);
        anchor->GetInterpolatedUp(anchor_up);
        anchor->GetInterpolatedForward(anchor_forward);
        

        npc->Printf("Anchor pos: %s forward: %s up: %s",toString(anchor_pos).GetDataSafe(),
                    toString(anchor_forward).GetDataSafe(),toString(anchor_up).GetDataSafe());
        
    }
    

    // None linear movement so we have to queue DRData updates.
    npcclient->GetNetworkMgr()->QueueDRData(npc);
}

void MovePathOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool MovePathOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    StopMovement(npc);

    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(path->GetEndPos(direction),path->GetEndRot(direction),path->GetEndSector(npcclient->GetEngine(),direction));

    if (anchor)
    {
        delete anchor;
        anchor = NULL;
    }

    npc->Printf("MovePathOp - Completed.");
    return true; // Script can keep going
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
        npc->ResumeScript(eventmgr,behavior);
    }
}

csString psResumeScriptEvent::ToString() const
{
    csString result;
    result.Format("Resuming script operation %s for %s",scriptOp->GetName(),npc->GetName());
    if (npc->GetEntity())
    {
        result.AppendFmt("(EID: %u)", npc->GetEntity()->GetID());
    }
    return result;
}

//---------------------------------------------------------------------------

void psGameObject::GetPosition(iCelEntity *entity, csVector3& pos, float& yrot,iSector*& sector)
{
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
        iPcMesh);

    // Position
    if(!pcmesh->GetMesh())
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR OBJECT %s!\n",entity->GetName());
        return;
    }

    pos = pcmesh->GetMesh()->GetMovable()->GetPosition();

    // rotation
    csMatrix3 transf = pcmesh->GetMesh()->GetMovable()->GetTransform().GetT2O();
    yrot = Matrix2YRot(transf);

    // Sector
    if (pcmesh->GetMesh()->GetMovable()->GetSectors()->GetCount())
    {
        sector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }
}

void psGameObject::SetPosition(iCelEntity *entity, csVector3& pos, iSector* sector)
{
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
        iPcMesh);

    pcmesh->MoveMesh(sector,pos);
}

float psGameObject::Matrix2YRot(const csMatrix3& mat)
{
    csVector3 vec(0,0,1);
    vec = mat * vec;
    vec.Normalize();

    return GetAngle (vec.z, vec.x);
}

float psGameObject::GetAngle(float x, float y)
{
    if ( x > 1.0 )  x = 1.0;
    if ( x < -1.0 ) x = -1.0;

    float angle = acos(x);
    if (y < 0)
        angle = 2*PI - angle;

    return angle;
}

void psGameObject::SetRotationAngle(iCelEntity *entity, float angle)
{
    csRef<iPcMesh> pcmesh = CEL_QUERY_PROPCLASS(entity->GetPropertyClassList(), 
        iPcMesh);

    csMatrix3 matrix = (csMatrix3) csYRotMatrix3 (angle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T (matrix);
}

float psGameObject::CalculateIncidentAngle(csVector3& pos, csVector3& dest)
{
    csVector3 diff = dest-pos;  // Get vector from player to desired position

    if (!diff.x)
        diff.x = .000001F; // div/0 protect

    float angle = atan2(-diff.x,-diff.z);

    return angle;
}

void psGameObject::ClampRadians(float &target_angle)
{
    // Clamp the angle witin 0 to 2*PI
    while (target_angle < 0)
        target_angle += TWO_PI;
    while (target_angle > TWO_PI)
        target_angle -= TWO_PI;
}

void psGameObject::NormalizeRadians(float &target_angle)
{
    // Normalize angle within -PI to PI
    while (target_angle < PI)
        target_angle += TWO_PI;
    while (target_angle > PI)
        target_angle -= TWO_PI;
}

//---------------------------------------------------------------------------
