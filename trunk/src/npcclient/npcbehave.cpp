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


//=============================================================================
// Project Includes
//=============================================================================
#include "net/msghandler.h"
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
    npcclient = other.npcclient;
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
            Error2("Specified parent npctype '%s' could not be found.",
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

    if (velStr.IsEmpty())
    {
        // Do nothing. Use velSource from constructor default value
        // or as inherited from superclass.
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

void NPCType::FirePerception(NPC *npc, Perception *pcpt)
{
    for (size_t x=0; x<reactions.GetSize(); x++)
    {
        reactions[x]->React(npc,pcpt);
    }
}

void NPCType::DumpReactionList(NPC *npc)
{
    CPrintf(CON_CMDOUTPUT, "%-30s %-20s %-5s\n","Reaction","Type","Range");
    for (size_t i=0; i<reactions.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT, "%-30s %-20s %5.1f\n",
                reactions[i]->GetEventType(),reactions[i]->GetType().GetDataSafe(),
                reactions[i]->GetRange());
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
        return 90*TWO_PI/360;
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

//---------------------------------------------------------------------------

void BehaviorSet::ClearState(NPC *npc)
{
	// Ensure any existing script is ended correctly.
	Interrupt(npc);
    for (size_t i = 0; i<behaviors.GetSize(); i++)
    {
        behaviors[i]->ResetNeed();
        behaviors[i]->SetActive(false);
        behaviors[i]->ClearInterrupted();
    }
    active = NULL;
}

bool BehaviorSet::Add(Behavior *b)
{
    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        if (!strcmp(behaviors[i]->GetName(),b->GetName()))
        {
            behaviors[i] = b;  // substitute
            return false;
        }
    }
    behaviors.Push(b);
    return true;
}

Behavior* BehaviorSet::Advance(csTicks delta,NPC *npc)
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
            npc->Printf(15,"NO Active applicable behavior." );
            return active;
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
                active->SetActive(false);
            }

            // Set the new active behaviour
            active = new_behaviour;
            // Activate the new behaviour
            active->SetActive(true);
            if (active->StartScript(npc,eventmgr))
            {
                // This behavior is done so set it inactive
                active->SetActive(false);
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

    npc->Printf(15,"Active behavior is '%s'", active->GetName() );
    return active;
}

void BehaviorSet::ResumeScript(NPC *npc,Behavior *which)
{
    if (which == active && which->ApplicableToNPCState(npc))
    {
        active->ResumeScript(npc,eventmgr);
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
    CPrintf(CON_CMDOUTPUT, "Appl. %-30s %5s %5s\n","Behavior","Curr","New");

    for (size_t i=0; i<behaviors.GetSize(); i++)
    {
        char applicable = 'N';
        if (npc && behaviors[i]->ApplicableToNPCState(npc))
        {
            applicable = 'Y';
        }

        CPrintf(CON_CMDOUTPUT, "%c    %s%-30s %5.1f %5.1f\n",applicable,
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
    init_need = 0;
    current_need            = init_need;
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
    init_need = 0;
    current_need            = init_need;
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
            op = new BeginLoopOperation;
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
        else if ( strcmp( node->GetValue(), "moveto" ) == 0 )
        {
            op = new MoveToOperation;
        }
        else if ( strcmp( node->GetValue(), "movepath" ) == 0 )
        {
            op = new MovePathOperation;
        }
        else if ( strcmp( node->GetValue(), "navigate" ) == 0 )
        {
            op = new NavigateOperation;
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
        else if ( strcmp( node->GetValue(), "rotate" ) == 0 )
        {
            op = new RotateOperation;
        }
        else if ( strcmp( node->GetValue(), "sequence" ) == 0 )
        {
            op = new SequenceOperation;
        }
        else if ( strcmp( node->GetValue(), "talk" ) == 0 )
        {
            op = new TalkOperation;
        }
        else if ( strcmp( node->GetValue(), "transfer" ) == 0 )
        {
            op = new TransferOperation;
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
        new_need = new_need + (d * need_growth_rate);
        npc->Printf(10,"%s - Advance none active delta: %.3f Need: %.2f Growth Rate: %.2f",
                    name.GetData(),d,new_need,need_growth_rate);
    }
}

bool Behavior::ApplicableToNPCState(NPC *npc)
{
    return npc->IsAlive() || (!npc->IsAlive() && is_applicable_when_dead);
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
        current_step = 0;
        return RunScript(npc,eventmgr,false);
    }
}

Behavior* BehaviorSet::Find(Behavior *key)
{
    size_t found = behaviors.Find(key);
    return (found = SIZET_NOT_FOUND) ? NULL : behaviors[found];
}

bool Behavior::RunScript(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    size_t start_step = current_step;
    while (true)
    {
        if (current_step < sequence.GetSize() )
        {
            npc->Printf(2, "Running %s step %d - %s operation%s",
                        name.GetData(),current_step,sequence[current_step]->GetName(),
                        (interrupted?" Interrupted":""));
            sequence[current_step]->SetCompleted(false);
            if (!sequence[current_step]->Run(npc,eventmgr,interrupted)) // Run returning false means that
            {                                                           // op is not finished but should
                                                                        // relinquish
                npc->Printf(2, "Behavior %s step %d - %s will complete later...",
                            name.GetData(),current_step,sequence[current_step]->GetName());
                return false; // This behavior isn't done yet
            }
            interrupted = false; // Only the first script operation should be interrupted.
            current_step++;
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
                npc->Printf(1, "End of non looping behaviour '%s'",GetName());
                break; // This behavior is done
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
    result.Format("Resuming script operation %s for %s",scriptOp->GetName(),npc->GetName());
    if (npc)
    {
        result.AppendFmt("(%s)", ShowID(npc->GetEID()));
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

    pos = pcmesh->GetMesh()->GetMovable()->GetPosition();

    // rotation
    csMatrix3 transf = pcmesh->GetMesh()->GetMovable()->GetTransform().GetT2O();
    yrot = psWorld::Matrix2YRot(transf);

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

void psGameObject::SetPosition(gemNPCObject* object, csVector3& pos, iSector* sector)
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
