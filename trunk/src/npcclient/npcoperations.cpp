/*
* npcoperations.cpp
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <iengine/movable.h>
#include <cstool/collider.h>
#include <iengine/mesh.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "globals.h"

#include "engine/linmove.h"
#include "engine/psworld.h"

#include "util/waypoint.h"
#include "util/pspath.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/psstring.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcoperations.h"
#include "npcclient.h"
#include "networkmgr.h"
#include "npc.h"
#include "tribe.h"
#include "perceptions.h"
#include "gem.h"
#include "npcmesh.h"

//---------------------------------------------------------------------------

ScriptOperation::ScriptOperation(const char* scriptName)
    : velSource(VEL_DEFAULT),vel(0.0), ang_vel(0.0), completed(true), 
      resumeScriptEvent(NULL), name(scriptName), 
      interrupted_angle(0.0f),collision("collision"),
      outOfBounds("out of bounds"),inBounds("in bounds"),
      consec_collisions(0), inside_rgn(true)
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
    
    if (velStr.IsEmpty())
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

bool ScriptOperation::LoadCheckMoveOk(iDocumentNode *node)
{
    if (node->GetAttribute("collision"))
    {
        collision = node->GetAttributeValue("collision");
    }
    if (node->GetAttribute("out_of_bounds"))
    {
        outOfBounds = node->GetAttributeValue("out_of_bounds");
    }
    if (node->GetAttribute("in_bounds"))
    {
        inBounds = node->GetAttributeValue("in_bounds");
    }
    return true;
}



void ScriptOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr) 
{
}

void ScriptOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)      
{
    StopResume();

    gemNPCObject * entity = npc->GetActor();
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

    psGameObject::GetPosition(npc->GetActor(),pos,angle,sector);
    return AtInterruptedPosition(pos,sector);
}

bool ScriptOperation::AtInterruptedAngle(NPC *npc)
{
    float      angle;
    csVector3  pos;
    iSector   *sector;

    psGameObject::GetPosition(npc->GetActor(),pos,angle,sector);
    return AtInterruptedAngle(pos,sector,angle);
}

bool ScriptOperation::CheckMovedOk(NPC *npc, EventManager *eventmgr, csVector3 oldPos, iSector* oldSector, const csVector3 & newPos, iSector* newSector, float timedelta)
{
    npcMesh* pcmesh = npc->GetActor()->pcmesh;

    if (!npcclient->GetWorld()->WarpSpace(oldSector, newSector, oldPos))
    {
        npc->Printf("CheckMovedOk: new and old sectors are not connected by a portal!");
        return false;
    }

    if ((oldPos - newPos).SquaredNorm() < 0.01f) // then stopped dead, presumably by collision
    {
        Perception collisionPerception(collision);
        npc->TriggerEvent(&collisionPerception);
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
            npc->Printf(10,"Bang. %d consec collisions last with diffs (%1.2f,%1.2f)...",
                        consec_collisions,diffx,diffz);
            if (consec_collisions > 8)  // allow for hitting trees but not walls
            {
                // after a couple seconds of sliding against something
                // the npc should give up and react to the obstacle.
                Perception collisionPerception(collision);
                npc->TriggerEvent(&collisionPerception);
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
                    Perception outbounds(outOfBounds);
                    npc->TriggerEvent(&outbounds);
                    inside_rgn = false;
                }
            }
            else
            {
                if (rgn->CheckWithinBounds(npcclient->GetEngine(),newPos,newSector))
                {
                    Perception inbounds(inBounds);
                    npc->TriggerEvent(&inbounds);
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
    npc->Printf(6,"TurnTo localDest=%s",toString(dest,destsect).GetData());

    npcMesh* pcmesh = npc->GetActor()->pcmesh;

    // Turn to face the direction we're going.
    csVector3 pos, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);

    if (!npcclient->GetWorld()->WarpSpace(sector, destsect, pos))
    {
        npc->Printf("Current and TurnTo destination sectors are not connected!");
        return;
    }
    forward = dest-pos;

    npc->Printf(6,"Forward is %s",toString(forward).GetDataSafe());

    up.Set(0,1,0);
    
    forward.y = 0;

    float angle = psGameObject::CalculateIncidentAngle(pos,dest);
    if (angle < 0) angle += TWO_PI;

    pcmesh->GetMesh()->GetMovable()->GetTransform().LookAt (-forward.Unit(), up.Unit());

    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);
    if (rot < 0) rot += TWO_PI;

    // Get Going at the right velocity
    csVector3 velvector(0,0,  -GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvector);
    npc->GetLinMove()->SetAngularVelocity( 0 );
}

void ScriptOperation::StopMovement(NPC *npc)
{
    if (!npc->GetActor()) return; // No enity to stop, returning

    // Stop the movement
    
    // Set Vel to zero again
    npc->GetLinMove()->SetVelocity( csVector3(0,0,0) );
    npc->GetLinMove()->SetAngularVelocity( 0 );

    npc->Printf(5,"Movement stopped.");

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);
}


int ScriptOperation::StartMoveTo(NPC *npc,EventManager *eventmgr, csVector3& dest, iSector* sector, float vel,const char *action, bool autoresume)
{
    csVector3 forward;
    
    TurnTo(npc, dest, sector, forward);
    
    // SetAction animation for the mesh also, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    float dist = forward.Norm();
    int msec = (int)(1000.0 * dist / GetVelocity(npc)); // Move will take this many msecs.
    
    npc->Printf(6,"MoveTo op should take approx %d msec.  ", msec);
    if (autoresume)
    {
        // wake me up when it's over
        Resume(msec,npc,eventmgr);
        npc->Printf(7,"Waking up in %d msec.", msec);
    }
    else
    {
        npc->Printf(7,"NO autoresume here.", msec);
    }

    return msec;
}

int ScriptOperation::StartTurnTo(NPC *npc, EventManager *eventmgr, float turn_end_angle, float angle_vel,const char *action, bool autoresume)
{
    csVector3 pos, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);


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

    npc->Printf(6,"TurnTo op should take approx %d msec for a turn of %.2f deg in %.2f deg/s.  ",
                msec,delta_rot*180.0/PI,angle_vel*180.0/PI);
    if (autoresume)
    {
        // wake me up when it's over
        Resume(msec,npc,eventmgr);
        npc->Printf(7,"Waking up in %d msec.", msec);
    }
    else
    {
        npc->Printf(7,"NO autoresume here.", msec);
    }
    
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
    //npc->GetActor()->pcmesh->SetAnimation(name, false);
}

//---------------------------------------------------------------------------
//         Following section contain specefix NPC operations.
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

bool MoveOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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

    completed = true;

    return true;  // Script can keep going
}

void MoveOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{
    remaining -= timedelta;

    // This updates the position of the entity every 1/2 second so that 
    // range and distance calculations will work when interrupted.

    csVector3 oldPos,newPos;
    float     oldRot,newRot;
    iSector  *oldSector,*newSector;

    npc->GetLinMove()->GetLastPosition(oldPos, oldRot, oldSector);
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
    npc->GetLinMove()->GetLastPosition(newPos, newRot, newSector);

    psGameObject::SetPosition(npc->GetActor(), newPos, newSector);

    CheckMovedOk(npc, eventmgr, oldPos, oldSector, newPos, newSector, timedelta);
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

bool CircleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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

    return MoveOperation::Run(npc, eventmgr, interrupted);
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

bool MoveToOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5,"MoveToOp Start dest=(%1.2f,%1.2f,%1.2f) at %1.2f m/sec.",
                dest.x,dest.y,dest.z,GetVelocity(npc));

    csVector3 pos, forward, up;
    float rot;
    iSector *sector;

    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);
    
    path.SetMaps(npcclient->GetMaps());
    path.SetDest(dest);
    path.CalcLocalDest(pos, sector, localDest);
    
    // Using "true" teleports to dest location after proper time has 
    // elapsed and is therefore more tolerant of CD errors.
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
    
    npc->Printf(10,"advance: pos=(%1.2f,%1.2f,%1.2f) rot=%.2f localDest=(%.2f,%.2f,%.2f) dest=(%.2f,%.2f,%.2f) dist=%f", 
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
            npc->Printf(8,"MoveTo within minimum acceptable range...Stopping him now.");
            // npc->ResumeScript(eventmgr, npc->GetBrain()->GetCurrentBehavior() );
            CompleteOperation(npc, eventmgr);
        }
        else
        {
            npc->Printf(8,"we are at localDest... WHAT DOES THIS MEAN?");
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
            npc->TriggerEvent(&collision);
        }
    }
}

bool MoveToOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    // Get the rot and sector here so they don't change in the SetPosition call
    float rot;
    iSector *sector;
    csVector3 pos;
    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);

    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(dest,rot,sector);

    // Stop the movement
    StopMovement(npc);
    
    npc->Printf(5,"MoveToOp - Completed. pos=(%1.2f,%1.2f,%1.2f) rot=%.2f dest set=(%1.2f,%1.2f,%1.2f)",
                pos.x,pos.y,pos.z, rot,
                dest.x,dest.y,dest.z);
    completed = true;

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
               "'target' or 'locatedest' right now.");
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

bool RotateOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    float rot;
    csVector3 pos;
    iSector* sector;
    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);

    float ang_vel = GetAngularVelocity(npc);

    if (interrupted)
    {
        npc->Printf(5,"Interrupted rotation to %.2f deg",target_angle*180.0f/PI);

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
                    min_angle = rot_angle;
                if (max_angle < rot_angle)
                    max_angle = rot_angle; 
            }

            if (max_angle-min_angle  > PI )
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
            npc->Printf(5,"At located destination, end rotation.");
            return true;
        }
        
        target_angle = psGameObject::CalculateIncidentAngle(pos,dest);

        angle_delta = target_angle-rot;

        // If the angle is close enough don't worry about it and just go to next command.
        if (fabs(angle_delta) < TWO_PI/60.0)
        {
            npc->Printf(5, "Rotation at destination angle. Ending rotation.");
            return true;
        }
    }
    else if (op_type == ROT_ABSOLUTE)
    {
        npc->Printf(5, "Absolute rotation to %.2f deg",target_angle*180.0f/PI);
        
        angle_delta =  target_angle - rot;
    }
    else if (op_type == ROT_RELATIVE)
    {
        npc->Printf(5, "Relative rotation by %.2f deg",delta_angle*180.0f/PI);

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

    npc->Printf(5,"Rotating %1.2f deg from %1.2f to %1.2f at %1.2f deg/sec in %.3f sec.",
                angle_delta*180/PI,rot*180.0f/PI,target_angle*180.0f/PI,ang_vel*180.0f/PI,msec/1000.0f);

    return false;
}

void RotateOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr) 
{
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
}

float RotateOperation::SeekAngle(NPC* npc, float targetYRot)
{
    // Try to avoid big ugly stuff in our path
    float rot;
    iSector *sector;
    csVector3 pos;
    psGameObject::GetPosition(npc->GetActor(),pos,rot,sector);

    csVector3 isect,start,end,dummy,box,legs;

    // Construct the feeling broom

    // Calculate the start and end poses
    start = pos;
    npc->GetLinMove()->GetCDDimensions(box,legs,dummy);

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
                npc->Printf(6,"Turning left!");
                turn = left;
                break;
            }

            // Do again for the other side
            dist = csColliderHelper::TraceBeam (npcclient->GetCollDetSys(), sector,
                broomStart[1], broomEnd[1], true, closest_tri, isect, &sel);

            if(dist < 0)
            {
                npc->Printf(6,"Turning right!");
                turn = right;
                break;
            }


        }
        if (turn==0.0)
        {
            npc->Printf(5,"Possible ERROR: turn value was 0");
        }
        // Apply turn
        targetYRot = turn;
    }
    return targetYRot;
}

void RotateOperation::InterruptOperation(NPC *npc, EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool RotateOperation::CompleteOperation(NPC *npc, EventManager *eventmgr)
{
    // Set target angle and stop the turn
    psGameObject::SetRotationAngle(npc->GetActor(),target_angle);
    StopMovement(npc);

    completed = true;
    
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
        npc->Printf(5,"Located WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        return end;
    }

    return NULL;
}

void ReplaceVariables(csString & object,NPC *npc)
{
    object.ReplaceAll("$name",npc->GetName());
    if (npc->GetRaceInfo())
    {
        object.ReplaceAll("$race",npc->GetRaceInfo()->GetName());
    }
    if (npc->GetTribe())
    {
        object.ReplaceAll("$tribe",npc->GetTribe()->GetName());
    }
    if (npc->GetOwner())
    {
        object.ReplaceAll("$owner",npc->GetOwnerName());
    }
    if (npc->GetTarget())
    {
        object.ReplaceAll("$target",npc->GetTarget()->GetName());
    }
}


bool LocateOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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
    psGameObject::GetPosition(npc->GetActor(),start_pos,start_rot,start_sector);

    ReplaceVariables(object,npc);

    csArray<csString> split_obj = psSplit(object,':');

    if (split_obj[0] == "perception")
    {
        npc->Printf(5,"LocateOp - Perception");

        if (!npc->GetLastPerception())
        {
            return true;
        }
        if (!npc->GetLastPerception()->GetLocation(located_pos,located_sector))
        {
            return true;
        }
        located_angle = 0; // not used in perceptions
    }
    else if (split_obj[0] == "target")
    {
        npc->Printf(5,"LocateOp - Target");

        gemNPCActor *ent;
        // Since we don't have a current enemy targeted, find one!
        if (range)
        {
            ent = npc->GetMostHated(range,locate_invisible,locate_invincible);
        }
        else
        {
            ent = npc->GetMostHated(10.0f,locate_invisible,locate_invincible);     // Default enemy range
        }

        if(ent)
        {
            npc->SetTarget(ent);
        }
        else
        {
            return true;
        }

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
        npc->Printf(5,"LocateOp - Owner");

        gemNPCObject *owner;
        // Since we don't have a current enemy targeted, find one!
        owner = npc->GetOwner();

        if(owner)
        {
            npc->SetTarget(owner);
        }
        else
        {
            return true;
        }

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(owner,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "self")
    {
        npc->Printf(5,"LocateOp - Self");

        gemNPCActor *ent;

        ent = npc->GetActor();

        if(ent)
        {
            npc->SetTarget(ent);
        }
        else
        {
            return true;
        }

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
        npc->Printf(5,"LocateOp - Tribe");

        if (!npc->GetTribe())
        {
            return true;
        }

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
                npc->Printf(5, "Couldn't locate any <%s> in npc script for <%s>.",
                            (const char *)object,npc->GetName() );
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
        npc->Printf(5, "LocateOp - Friend");

        gemNPCActor *ent = npc->GetNearestVisibleFriend(20);
        if(ent)
        {
            npc->SetTarget(ent);
        }
        else
        {
            return true;
        }

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
        npc->Printf(5, "LocateOp - Waypoint");

        float located_range=0.0;

        if (split_obj.GetSize() >= 2)
        {
            if (split_obj[1] == "group")
            {
                if (random)
                {
                    located_wp = npcclient->FindRandomWaypoint(split_obj[2],start_pos,start_sector,range,&located_range);
                }
                else
                {
                    located_wp = npcclient->FindNearestWaypoint(split_obj[2],start_pos,start_sector,range,&located_range);
                }
            } else if (split_obj[1] == "name")
            {
                located_wp = npcclient->FindWaypoint(split_obj[2]);
                if (located_wp)
                {
                    located_range = npcclient->GetWorld()->Distance(start_pos,start_sector,located_wp->loc.pos,located_wp->GetSector(npcclient->GetEngine()));
                }
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
            npc->Printf(5, "Couldn't locate any <%s> in npc script for <%s>.",
                (const char *)object,npc->GetName() );
            return true;
        }
        npc->Printf(5, "Located waypoint: %s at %s range %.2f",located_wp->GetName(),
                    toString(located_wp->loc.pos,located_wp->loc.GetSector(npcclient->GetEngine())).GetData(),located_range);

        located_pos = located_wp->loc.pos;
        located_angle = located_wp->loc.rot_angle;
        located_sector = located_wp->loc.GetSector(npcclient->GetEngine());

        located_wp = CalculateWaypoint(npc,located_pos,located_sector,-1);
    }
    else if (!static_loc || !located)
    {
        npc->Printf(5, "LocateOp - Location");

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
            npc->Printf(5, "Couldn't locate any <%s> in npc script for <%s>.",
                (const char *)object,npc->GetName() );
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
        npc->Printf(5, "remembered location from last time");
    }

    // Save on npc so other operations can refer to value
    npc->SetActiveLocate(located_pos,located_sector,located_angle,located_wp);

    npc->Printf(5, "LocateOp - Active location: pos %s rot %.2f wp %s",
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

bool NavigateOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    csVector3 dest;
    float rot=0;
    iSector* sector;

    npc->GetActiveLocate(dest,sector,rot);
    npc->Printf(5, "Located %s at %1.2f m/sec.",toString(dest,sector).GetData(), GetVelocity(npc) );

    StartMoveTo(npc,eventmgr,dest,sector,GetVelocity(npc),action);
    return false;
}

void NavigateOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr) 
{
    npc->GetLinMove()->ExtrapolatePosition(timedelta);
}

void NavigateOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool NavigateOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    // Set position to where it is supposed to go
    float rot=0;
    iSector *sector;
    csVector3 pos;
    npc->GetActiveLocate(pos,sector,rot);
    npc->GetLinMove()->SetPosition(pos,rot,sector);

    // Stop the movement
    StopMovement(npc);

    completed = true;

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
    // If there are only one way out don't bother to find if its legal
    if (active_wp->links.GetSize() == 1)
    {
        npc->Printf(10, "Only one way out of this waypoint");
        return active_wp->links[0];
    }

    csArray<Waypoint*> waypoints; // List of available waypoints

    // Calculate possible waypoints
    for (size_t ii = 0; ii < active_wp->links.GetSize(); ii++)
    {
        Waypoint *new_wp = active_wp->links[ii];

        npc->Printf(11, "Considering: %s ",new_wp->GetName());

        if ( ((new_wp == prior_wp && new_wp->allow_return)||(new_wp != prior_wp)) &&
             (!active_wp->prevent_wander[ii]) &&
             (!undergroundValid || new_wp->underground == underground) &&
             (!underwaterValid || new_wp->underwater == underwater) &&
             (!privValid || new_wp->priv == priv) &&
             (!pubValid || new_wp->pub == pub) &&
             (!cityValid || new_wp->city == city) &&
             (!indoorValid || new_wp->indoor == indoor))
        {
            npc->Printf(10, "Possible next waypoint: %s",new_wp->GetName());
            waypoints.Push(new_wp);
        }
    }

    // If no path out of waypoint is possible to go, just return.
    if (waypoints.GetSize() == 0)
    {
        npc->Printf(10, "No possible ways, going back");
        return prior_wp;
    }
    

    return waypoints[psGetRandom((int)waypoints.GetSize())];
}


bool WanderOperation::FindNextWaypoint(NPC *npc)
{
    float rot=0;
    psGameObject::GetPosition(npc->GetActor(),current_pos,rot,current_sector);

    if (random)
    {
        prior_wp = active_wp;
        active_wp = next_wp;
        if (!active_wp) active_wp = npcclient->FindNearestWaypoint(current_pos,current_sector);
        next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);

        npc->Printf(6, "Active waypoint: %s at %s",active_wp->GetName(),
                    toString(active_wp->loc.pos,
                             active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
        npc->Printf(6, "Next   waypoint: %s at %s",next_wp->GetName(),
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
                npc->Printf(6, "Within waypoint %s",active_wp->GetName());
                
                if (FindNextWaypoint(npc))
                {
                    return true;
                }
                else
                {
                    npc->Printf(5, "WanderOp At end of waypoint list.");
                    return false;
                }
            } else
            {
                if (!prior_wp)
                {
                    npc->Printf(7, "Make sure we have a previous wp.");
                    prior_wp = active_wp;
                    active_wp = next_wp;
                    next_wp = NULL;
                }

                npc->Printf(6, "Active waypoint: %s at %s",active_wp->GetName(),
                            toString(active_wp->loc.pos,
                                     active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                if (next_wp)
                {
                    npc->Printf(6, "Next   waypoint: %s at %s",next_wp->GetName(),
                                toString(next_wp->loc.pos,
                                         next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
                }
                return true;
            }
        } else
        {
            npc->Printf(5, ">>>WanderOp At end of waypoint list.");
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
    
    psGameObject::GetPosition(npc->GetActor(), start_pos, start_rot, start_sector);

    if (random)
    {
        npc->Printf(5,"Calculate waypoint list for random movement...");
        if (active_wp == NULL)
        {
            prior_wp = NULL;
            active_wp = npcclient->FindNearestWaypoint(start_pos,start_sector);
            next_wp = GetNextRandomWaypoint(npc,prior_wp,active_wp);

            if (active_wp)
            {
                npc->Printf(6,"Active waypoint: %s at %s",active_wp->GetName(),
                            toString(active_wp->loc.pos,
                                     active_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            }
            if (next_wp)
            {
                npc->Printf(6,"Next   waypoint: %s at %s",next_wp->GetName(),
                            toString(next_wp->loc.pos,
                                     next_wp->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            }
            
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
            npc->Printf(6, "Start WP: %30s at %s",start->GetName(),toString(start->loc.pos,start->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            npc->Printf(6, "End WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            
            if (start == end)
            {
                WaypointListPushBack(start);
            } else
            {
                csList<Waypoint*> wps;
                
                wps = npcclient->FindWaypointRoute(start,end);
                if (wps.IsEmpty())
                {
                    npc->Printf(5, "Can't find route...");
                    return false;
                }
                
                while (!wps.IsEmpty())
                {
                    WaypointListPushBack(wps.Front());
                    wps.PopFront();
                }
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
                npc->Printf(5, "Waypoint list: %s",wp_str.GetDataSafe());
            }
        }
    }

    return true;
}

bool WanderOperation::StartMoveToWaypoint(NPC *npc, EventManager *eventmgr)
{
    // now calculate new destination from new active wp
    CalculateTargetPos(dest,dest_sector);

    // Find path and return direction for that path between wps 
    path = npcclient->FindPath(prior_wp,active_wp,direction);
    
    if (!path)
    {
        Error5("%s(%s) Could not find path between '%s' and '%s'",
               npc->GetName(), ShowID(npc->GetActor()->GetEID()),
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

bool WanderOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (interrupted && AtInterruptedPosition(npc))
    {
        // Restart current behavior
        StartMoveTo(npc,eventmgr,dest,dest_sector,GetVelocity(npc),action);

        return false; // This behavior isn't done yet
    }
    // If interruped and not at interruped position we do the same
    // as we do when started.

    prior_wp = NULL;
    active_wp = NULL;
    next_wp = NULL;

    if (!CalculateWaypointList(npc))
    {
        npc->Printf(5, ">>>WanderOp no list to wander");
        return true; // Nothing more to do for this op.
    }
    

    if (!FindNextWaypoint(npc))
    {
        npc->Printf(5, ">>>WanderOp NO waypoints, %s cannot move.",npc->GetName());
        return true; // Nothing more to do for this op.
    }

    // Turn of CD and hug the ground
    npc->GetLinMove()->UseCD(false);
    npc->GetLinMove()->SetOnGround(true); // Wander is ALWAYS on_ground.  This ensures correct animation on the client.
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
        npc->Printf(5, "We are done...");

        // None linear movement so we have to queue DRData updates.
        npcclient->GetNetworkMgr()->QueueDRData(npc);

        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );

        return;
    }
    
    if (npc->IsDebugging(10))
    {
        csVector3 pos; float rot; iSector *sec;
        psGameObject::GetPosition(npc->GetActor(),pos,rot,sec);

        npc->Printf(10, "Wander Loc is %s Rot: %1.2f Vel: %.2f Dist: %.2f Index: %d Fraction %.2f",
                    toString(pos).GetDataSafe(),
                    rot,
                    GetVelocity(npc),
                    anchor->GetDistance(),
                    anchor->GetCurrentAtIndex(),
                    anchor->GetCurrentAtFraction());
        
        csVector3 anchor_pos,anchor_up,anchor_forward;

        anchor->GetInterpolatedPosition(anchor_pos);
        anchor->GetInterpolatedUp(anchor_up);
        anchor->GetInterpolatedForward(anchor_forward);
        

        npc->Printf(10, "Anchor pos: %s forward: %s up: %s",toString(anchor_pos).GetDataSafe(),
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

    if (next_wp)
    {
        npc->Printf(5, "WanderOp - Reached waypoint %s next waypoint is %s.",
                    active_wp->GetName(), next_wp->GetName() );
        psGameObject::SetPosition(npc->GetActor(),dest,dest_sector); //update position anyway
    }
    else
    {
        npc->Printf(5, "WanderOp - Reached final waypoint %s.",active_wp->GetName() );

        // Only set position if at a end point.
        psGameObject::SetPosition(npc->GetActor(),dest,dest_sector);
        current_pos = dest;  // make sure waypoint is starting point of next path
    }

    if (FindNextWaypoint(npc))
    {
        StartMoveToWaypoint(npc, eventmgr);

        return false;  // Script requeues termination event so this CompleteOp is essentially an infinite loop
    }


    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(path->GetEndPos(direction),path->GetEndRot(direction),path->GetEndSector(npcclient->GetEngine(),direction));

    if (anchor)
    {
        delete anchor;
        anchor = NULL;
    }

    // Turn on CD again
    npc->GetLinMove()->UseCD(true);
    npc->GetLinMove()->SetHugGround(false);

    // Stop the movement
    StopMovement(npc);

    completed = true;

    return true; // Script can keep going, no more waypoints.
}

bool LoadAttributeBool(iDocumentNode *node, const char * attribute, bool defaultValue, bool * valid = NULL)
{
   csRef<iDocumentAttribute> attr;
   attr = node->GetAttribute(attribute);
   if (attr)
   {
       if (valid) *valid = true;
       return node->GetAttributeValueAsBool(attribute,defaultValue);
   }
   else
   {
       if (valid) *valid = false;
       return defaultValue;
   }
}


bool WanderOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    LoadVelocity(node);
    random = node->GetAttributeValueAsBool("random",false);  // Random wander never ends

    underground = LoadAttributeBool(node,"underground",false,&undergroundValid);
    underwater = LoadAttributeBool(node,"underwater",false,&underwaterValid);
    priv = LoadAttributeBool(node,"private",false,&privValid);
    pub = LoadAttributeBool(node,"public",false,&pubValid);
    city = LoadAttributeBool(node,"city",false,&cityValid);
    indoor = LoadAttributeBool(node,"indoor",false,&indoorValid);

    // Internal variables set to defaults
    path = NULL;
    anchor = NULL;

    return true;
}

ScriptOperation *WanderOperation::MakeCopy()
{
    WanderOperation *op  = new WanderOperation;
    op->action           = action;
    op->velSource        = velSource;
    op->vel              = vel;
    op->random           = random;
    op->undergroundValid = undergroundValid;
    op->underground      = underground;
    op->underwaterValid  = underwaterValid;
    op->underwater       = underwater;
    op->privValid        = privValid;
    op->priv             = priv;
    op->pubValid         = pubValid;
    op->pub              = pub;
    op->cityValid        = cityValid;
    op->city             = city;
    op->indoorValid      = indoorValid;
    op->indoor           = indoor;

    // Internal variables set to defaults
    path = NULL;
    anchor = NULL;
    
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

//---------------------------------------------------------------------------

const char * ChaseOperation::typeStr[]={"unkown","nearest","owner","target"};

bool ChaseOperation::Load(iDocumentNode *node)
{
    action = node->GetAttributeValue("anim");
    csString typestr = node->GetAttributeValue("type");
    if (typestr == "nearest")
    {
        type = NEAREST;
    }
    else if (typestr == "boss" || typestr== "owner")
    {
        type = OWNER;
    }
    else if (typestr == "target")
    {
        type = TARGET;
    }
    else
    {
        type = UNKNOWN;
    }

    if (node->GetAttributeValue("range"))
    {
        searchRange = node->GetAttributeValueAsFloat("range");
    }
    else
    {
        searchRange = 2.0f;
    }    

    if (node->GetAttributeValue("chase_range"))
    {
        chaseRange = node->GetAttributeValueAsFloat("chase_range");
    }
    else
    {
        chaseRange = -1.0f; // Disable max chase range
    }    

    if ( node->GetAttributeValue("offset") )
    {
        offset = node->GetAttributeValueAsFloat("offset");
    }

    LoadVelocity(node);
    LoadCheckMoveOk(node);
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");

    return true;
}

ScriptOperation *ChaseOperation::MakeCopy()
{
    ChaseOperation *op = new ChaseOperation;
    op->action = action;
    op->type   = type;
    op->searchRange  = searchRange;
    op->chaseRange  = chaseRange;
    op->velSource = velSource;
    op->vel    = vel;
    op->ang_vel= ang_vel;
    op->offset = offset;

    return op;
}

bool ChaseOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    float targetRot;
    iSector* targetSector;
    csVector3 targetPos;
    
    float myRot;
    iSector* mySector;
    csVector3 myPos;

    csVector3 dest;
    csString name;
    gemNPCObject *entity = NULL;
    target_id = EID(0);
        
    switch (type)
    {
    case NEAREST:
        target_id = npc->GetNearestEntity(dest, name, searchRange);
        npc->Printf(6, "Targeting nearest entity (%s) at (%1.2f,%1.2f,%1.2f) for chase ...",
                    (const char *)name, dest.x, dest.y, dest.z);
        if (target_id.IsValid())
        {
            entity = npcclient->FindEntityID(target_id);
        }
        break;
    case OWNER:
        {
            
            gemNPCObject * owner = npc->GetOwner();
            if (owner)
            {
                entity = owner;
            }
            if (entity)
            {
                target_id = entity->GetEID();
                psGameObject::GetPosition(entity, dest, targetRot, targetSector);
                npc->Printf(6, "Targeting owner (%s) at (%1.2f,%1.2f,%1.2f) for chase ...",
                            entity->GetName(),dest.x,dest.y,dest.z );

            }
        }
        break;
    case TARGET:
        {
            gemNPCObject * target = npc->GetTarget();
            if (target)
            {
                entity = target;
            }
            if (entity)
            {
                target_id = entity->GetEID();
                psGameObject::GetPosition(entity, dest, targetRot,targetSector);
                npc->Printf(6, "Targeting current target (%s) at (%1.2f,%1.2f,%1.2f) for chase ...",
                            entity->GetName(), dest.x,dest.y, dest.z );
            }
        }
        break;
    }

    if (target_id.IsValid() && entity)
    {
        psGameObject::GetPosition(npc->GetActor(),myPos, myRot, mySector);
    
        psGameObject::GetPosition(entity, targetPos, targetRot, targetSector);

        npc->Printf(5, "Chasing enemy <%s, %s> at %s", entity->GetName(), ShowID(entity->GetEID()),
                    toString(targetPos,targetSector).GetDataSafe());

        // We need to work in the target sector space
        if (!npcclient->GetWorld()->WarpSpace(targetSector, mySector, targetPos))
        {
            npc->Printf("ChaseOperation: target's sector is not connected to ours!");
            return true;  // This operation is complete
        }
        if ( Calc2DDistance( myPos, targetPos ) < offset )
        {
            return true;  // This operation is complete
        }

        // This prevents NPCs from wanting to occupy the same physical space as something else
        csVector3 displacement = targetPos - myPos;
        displacement.y = 0;
        float factor = offset / displacement.Norm();
        csVector3 destPos = myPos + (1 - factor) * displacement;
        destPos.y = targetPos.y;

        path.SetMaps(npcclient->GetMaps());
        path.SetDest(destPos);
        path.CalcLocalDest(myPos, mySector, localDest);


        if ( GetAngularVelocity(npc) > 0 || GetVelocity(npc) > 0 )
        {
            StartMoveTo(npc, eventmgr, localDest, targetSector, GetVelocity(npc), action, false);
            return false;
        }
        else
        {
            return true;  // This operation is complete
        }
    }
    else
    {
        npc->Printf(5, "No one found to chase!");
        return true;  // This operation is complete
    }

    return true; // This operation is complete
}

void ChaseOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{

    csVector3 myPos,myNewPos,targetPos;
    float     myRot,dummyrot;
    InstanceID       myInstance, targetInstance;
    iSector * mySector, *myNewSector, *targetSector;
    csVector3 forward;
    
    npc->GetLinMove()->GetLastPosition(myPos, myRot, mySector);
    myInstance = npc->GetActor()->GetInstance();

    // Now turn towards entity being chased again
    csString name;
    
    if (type == NEAREST)
    {
        // Switch target if a new entity is withing search range.
        EID newTargetEID = npc->GetNearestEntity(targetPos, name, searchRange);
        if (newTargetEID.IsValid())
        {
            target_id = newTargetEID;
        }
    }
 
    gemNPCActor *target_entity = NULL;
    gemNPCActor * targetActor = dynamic_cast<gemNPCActor*>(npcclient->FindEntityID(target_id));
    if (targetActor)
    {
        target_entity = targetActor;
    }

    if (!targetActor || !target_entity) // no entity close to us
    {
        npc->Printf(5, "ChaseOp has no target now!");
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return;
    }
    
    if(name.IsEmpty())
    {
        name = target_entity->GetName();
    }
    
    psGameObject::GetPosition(target_entity,targetPos,dummyrot,targetSector);
    targetInstance = targetActor->GetInstance();

    // We work in our sector's space
    if (!npcclient->GetWorld()->WarpSpace(targetSector, mySector, targetPos))
    {
        npc->Printf("ChaseOperation: target's sector is not connected to ours!");
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return;
    }

    // This prevents NPCs from wanting to occupy the same physical space as something else
    csVector3 displacement = targetPos - myPos;

    displacement.y = 0;
    float distance = displacement.Norm();
    
    if ( (chaseRange > 0 && distance > chaseRange) || (targetInstance != myInstance) )
    {
        npc->Printf(5, "Target out of chase range -> we are done..");
        csString str;
        str.Append(typeStr[type]);
        str.Append(" out of chase");
        Perception range(str);
        npc->TriggerEvent(&range);
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return;
    }
    

    float factor = offset / distance;
    targetPos = myPos + (1 - factor) * displacement;
    targetPos.y = myPos.y;

    npc->Printf(10, "Still chasing %s at %s...",(const char *)name,toString(targetPos,targetSector).GetDataSafe());
    
    float angleToTarget = psGameObject::CalculateIncidentAngle(myPos, targetPos);
    csVector3 pathDest = path.GetDest();
    float angleToPath  = psGameObject::CalculateIncidentAngle(myPos, pathDest);

        
    // if the target diverged from the end of our path, we must calculate it again
    if ( fabs( AngleDiff(angleToTarget, angleToPath) ) > EPSILON  )
    {
        npc->Printf(8, "turn to target..");
        path.SetDest(targetPos);
        path.CalcLocalDest(myPos, mySector, localDest);
        StartMoveTo(npc,eventmgr,localDest, mySector, GetVelocity(npc), action, false);
    }
    

    float close = GetVelocity(npc)*timedelta; // Add 10 % to the distance moved in one tick.
    
    if (Calc2DDistance(localDest, myPos) <= close)
    {
        myPos.x = localDest.x;
        myPos.z = localDest.z;
        npc->GetLinMove()->SetPosition(myPos,myRot,mySector);
        
        if (Calc2DDistance(myPos,targetPos) <= close)
        {
            npc->Printf(5, "We are done..");
            npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
            return;
        }
        else
        {
            npc->Printf(6, "We are at localDest..");
            path.SetDest(targetPos);
            path.CalcLocalDest(myPos, mySector, localDest);
            StartMoveTo(npc, eventmgr, localDest, mySector, GetVelocity(npc), action, false);
        }
    }
    else
    {
        TurnTo(npc, localDest, mySector, forward);
    }

    npc->Printf(8, "advance: pos=(%f.2,%f.2,%f.2) rot=%.2f localDest=(%f.2,%f.2,%f.2) dist=%f", 
                myPos.x,myPos.y,myPos.z, myRot,
                localDest.x,localDest.y,localDest.z,
                Calc2DDistance(localDest, myPos));

    {
        ScopedTimer st(250, "chase extrapolate %.2f time for %s", timedelta, ShowID(npc->GetActor()->GetEID()));
        npc->GetLinMove()->ExtrapolatePosition(timedelta);
    }
    bool on_ground;
    float speed,ang_vel;
    csVector3 bodyVel,worldVel;

    npc->GetLinMove()->GetDRData(on_ground,speed,myNewPos,myRot,myNewSector,bodyVel,worldVel,ang_vel);

    npc->Printf(8,"World position bodyVel=%s worldVel=%s",toString(bodyVel).GetDataSafe(),toString(worldVel).GetDataSafe());

    CheckMovedOk(npc, eventmgr, myPos, mySector, myNewPos, myNewSector, timedelta);
}

void ChaseOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
    
    StopMovement(npc);
}

bool ChaseOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    StopMovement(npc);

    completed = true;

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

bool PickupOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    gemNPCObject *item = NULL;

    if (object == "perception")
    {
        if (!npc->GetLastPerception())
            return true;
        if (!(item = npc->GetLastPerception()->GetTarget()))
            return true;
    } else
    {
        // TODO: Insert code to find the nearest item
        //       with name given by object.
        return true;
    }

    npc->Printf(5, "   Who: %s What: %s Count: %d",npc->GetName(),item->GetName(), count);

    npcclient->GetNetworkMgr()->QueuePickupCommand(npc->GetActor(), item, count);

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

bool EquipOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s What: %s Where: %s Count: %d",
                npc->GetName(),item.GetData(), slot.GetData(), count);

    npcclient->GetNetworkMgr()->QueueEquipCommand(npc->GetActor(), item, slot, count);

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

bool DequipOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s Where: %s",
                npc->GetName(), slot.GetData());

    npcclient->GetNetworkMgr()->QueueDequipCommand(npc->GetActor(), slot );

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

bool TalkOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{

    if(!target)
    {
        npcclient->GetNetworkMgr()->QueueTalkCommand(npc->GetActor(), npc->GetName() + text);
        return true;
    }
    
    if(!npc->GetTarget())
        return true;

    NPC* friendNPC = npc->GetTarget()->GetNPC();
    if(friendNPC)
    {
        npcclient->GetNetworkMgr()->QueueTalkCommand(npc->GetActor(), npc->GetName() + text);
        
        Perception collision("friend:" + command);
        friendNPC->TriggerEvent(&collision);
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

bool SequenceOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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

bool VisibleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetActor(), true);
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

bool InvisibleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetActor(), false);
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

bool ReproduceOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if(!npc->GetTarget())
        return true;

    NPC * friendNPC = npc->GetTarget()->GetNPC();
    if(friendNPC)
    {
        npc->Printf(5, "Reproduce");
        npcclient->GetNetworkMgr()->QueueSpawnCommand(friendNPC->GetActor(), npc->GetActor());
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

bool ResurrectOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    psTribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return true;

    csVector3 where;
    float     radius;
    iSector*  sector;
    float     rot = 0; // Todo: Set to a random rotation

    tribe->GetHome(where,radius,sector);

    // Todo: Add a random delta within radius to the where value.

    npcclient->GetNetworkMgr()->QueueResurrectCommand(where, rot, sector->QueryObject()->GetName(), npc->GetPID());

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

bool MemorizeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Perception * percept = npc->GetLastPerception();
    if (!percept)
    {
        npc->Printf(5, ">>> Memorize No Perception.");
        return true; // Nothing more to do for this op.
    }
    
    npc->Printf(5, ">>> Memorize '%s' '%s'.",percept->GetType(),percept->GetName());

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

bool MeleeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "MeleeOperation starting with meele range %.2f seek range %.2f will attack:%s%s.",
                melee_range, seek_range,(attack_invisible?" Invisible":" Visible"),
                (attack_invincible?" Invincible":""));

    attacked_ent = npc->GetMostHated(melee_range,attack_invisible,attack_invincible);
    if (attacked_ent)
    {
        npc->Printf(5, "Melee starting to attack %s(%s)", attacked_ent->GetName(), ShowID(attacked_ent->GetEID()));

        npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetActor(),attacked_ent);
    }
    else
    {
        // We know who attacked us, even if they aren't in range.
        npc->SetTarget( npc->GetLastPerception()->GetTarget() );
    }

    return false; // This behavior isn't done yet
}

void MeleeOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{
    // Check hate list to make sure we are still attacking the right person
    gemNPCActor *ent = npc->GetMostHated(melee_range, attack_invisible, attack_invincible);
    if (!ent)
    {
        npc->Printf(8, "No Melee target in range (%2.2f), going to chase!", melee_range);

        // No enemy to whack on in melee range, search far
        ent = npc->GetMostHated(seek_range, attack_invisible, attack_invincible);

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
            npc->TriggerEvent(&range);
        }
        else // no hated targets around
        {
            if(npc->IsDebugging(5))
            {
                npc->DumpHateList();
            }
            npc->Printf(8, "No hated target in seek range (%2.2f)!", seek_range);
            npc->GetCurrentBehavior()->ApplyNeedDelta(-5); // don't want to fight as badly
        }
        return;
    }
    if (ent != attacked_ent)
    {
        attacked_ent = ent;
        if (attacked_ent)
        {
            npc->Printf(5, "Melee switching to attack %s(%s)", attacked_ent->GetName(), ShowID(attacked_ent->GetEID()));
        }
        else
        {
            npc->Printf(5, "Melee stop attack");
        }
        npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetActor(), ent);
    }
}

void MeleeOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
    
    npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetActor(),NULL);
}

bool MeleeOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetActor(),NULL);

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

bool BeginLoopOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
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

bool EndLoopOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Behavior * behavior = npc->GetCurrentBehavior();

    current++;

    if (current < iterations)
    {
        behavior->SetCurrentStep(loopback_op-1);
        npc->Printf(5, "EndLoop - Loop %d of %d",current,iterations);
        return true;
    }

    current = 0; // Make sure we will loop next time to

    npc->Printf(5, "EndLoop - Exit %d %d",current,iterations);
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

bool WaitOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (!interrupted)
    {
        remaining = duration;
    }

    // SetAction animation for the mesh, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return false;
}

void WaitOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    remaining -= timedelta;
    if(remaining <= 0)
    	npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
    npc->Printf(10, "waiting... %.2f",remaining);
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

bool DropOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueDropCommand(npc->GetActor(), slot );

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

bool TransferOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    csString transferItem = item;

    csArray<csString> splitItem = psSplit(transferItem,':');
    if (splitItem[0] == "tribe")
    {
        if (!npc->GetTribe())
        {
            npc->Printf(5, "No tribe");
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
    
    npcclient->GetNetworkMgr()->QueueTransferCommand(npc->GetActor(), transferItem, count, target );

    return true;
}

//---------------------------------------------------------------------------

bool DigOperation::Load(iDocumentNode *node)
{
    resource = node->GetAttributeValue("resource");
    if (resource.IsEmpty()) return false;
    return true;
}

ScriptOperation *DigOperation::MakeCopy()
{
    DigOperation *op = new DigOperation;
    op->resource = resource;
    return op;
}

bool DigOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (resource == "tribe:wealth")
    {
        if (npc->GetTribe())
        {
            npcclient->GetNetworkMgr()->QueueDigCommand(npc->GetActor(), npc->GetTribe()->GetNeededResourceNick());
        }
    }
    else
    {
        npcclient->GetNetworkMgr()->QueueDigCommand(npc->GetActor(), resource );
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

bool DebugOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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
        npc->Printf(1, "DebugOp Set debug %d",level);
    }
    
    npc->SetDebugging(level);
    
    if (level) // Print after when turning on
    {
        npc->Printf(1, "DebugOp Set debug %d",level);            
    }

    return true; // We are done
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
        npc->Printf(5, "We are done..");

        // None linear movement so we have to queue DRData updates.
        npcclient->GetNetworkMgr()->QueueDRData(npc);

        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );

        return;
    }
    
    if (npc->IsDebugging())
    {
        csVector3 pos; float rot; iSector *sec;
        psGameObject::GetPosition(npc->GetActor(),pos,rot,sec);

        npc->Printf(8, "MovePath Loc is %s Rot: %1.2f Vel: %.2f Dist: %.2f Index: %d Fraction %.2f",
                    toString(pos).GetDataSafe(),rot,GetVelocity(npc),anchor->GetDistance(),anchor->GetCurrentAtIndex(),anchor->GetCurrentAtFraction());
        
        csVector3 anchor_pos,anchor_up,anchor_forward;

        anchor->GetInterpolatedPosition(anchor_pos);
        anchor->GetInterpolatedUp(anchor_up);
        anchor->GetInterpolatedForward(anchor_forward);
        

        npc->Printf(9, "Anchor pos: %s forward: %s up: %s",toString(anchor_pos).GetDataSafe(),
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
    // Set position to where it is supposed to go
    npc->GetLinMove()->SetPosition(path->GetEndPos(direction),path->GetEndRot(direction),path->GetEndSector(npcclient->GetEngine(),direction));

    if (anchor)
    {
        delete anchor;
        anchor = NULL;
    }

    StopMovement(npc);

    completed = true;

    return true; // Script can keep going
}

//---------------------------------------------------------------------------

const char * WatchOperation::typeStr[]={"unkown","nearest","owner","target"};

bool WatchOperation::Load(iDocumentNode *node)
{
    watchRange  = node->GetAttributeValueAsFloat("range");
    watchInvisible = node->GetAttributeValueAsBool("invisible",false);
    watchInvincible= node->GetAttributeValueAsBool("invincible",false);

    csString typestr = node->GetAttributeValue("type");
    if (typestr == "nearest")
    {
        type = NEAREST;
    }
    else if (typestr == "boss" || typestr== "owner")
    {
        type = OWNER;
    }
    else if (typestr == "target")
    {
        type = TARGET;
    }
    else
    {
        type = UNKNOWN;
    }

    if (node->GetAttributeValue("range"))
    {
        range = node->GetAttributeValueAsFloat("range");
    }
    else
    {
        range = 2.0f;
    }    

    watchedEnt = NULL;

    return true;
}

ScriptOperation *WatchOperation::MakeCopy()
{
    WatchOperation *op = new WatchOperation;
    op->watchRange      = watchRange;
    op->type            = type;
    op->range           = range; // Used for watch of type NEAREST 
    op->watchInvisible  = watchInvisible;
    op->watchInvincible = watchInvincible;
    watchedEnt = NULL;
    return op;
}

bool WatchOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "WatchOperation starting with watch range (%.2f) will watch:%s%s.",
                watchRange,(watchInvisible?" Invisible":" Visible"),
                (watchInvincible?" Invincible":""));

    float targetRot;
    iSector* targetSector;
    csVector3 targetPos;

    csString name;
    EID target_id;

    switch (type)
    {
        case NEAREST:
            target_id = npc->GetNearestEntity(targetPos, name, range);
            npc->Printf(5, "Targeting nearest entity (%s) at (%1.2f,%1.2f,%1.2f) for watch ...",
                        (const char *)name,targetPos.x,targetPos.y,targetPos.z);
            if (target_id.IsValid())
            {
                watchedEnt = npcclient->FindEntityID(target_id);
            }
            break;
        case OWNER:
            watchedEnt = npc->GetOwner();
            if (watchedEnt)
            {
                target_id = watchedEnt->GetEID();
                psGameObject::GetPosition(watchedEnt, targetPos,targetRot,targetSector);
                npc->Printf(5, "Targeting owner (%s) at (%1.2f,%1.2f,%1.2f) for watch ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z );

            }
            break;
        case TARGET:
            watchedEnt = npc->GetTarget();
            if (watchedEnt)
            {
                target_id = watchedEnt->GetEID();
                psGameObject::GetPosition(watchedEnt, targetPos,targetRot,targetSector);
                npc->Printf(5, "Targeting current target (%s) at (%1.2f,%1.2f,%1.2f) for chase ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z );
            }
            break;
    }

    if (!watchedEnt)
    {
        npc->Printf(5,"No entity to watch");
        return true; // Nothing to do for this behaviour.
    }
    
    npc->SetTarget( watchedEnt );

    /*
    if (OutOfRange(npc))
    {
        csString str;
        str.Append(typeStr[type]);
        str.Append(" out of range");
        Perception range(str);
        npc->TriggerEvent(&range, eventmgr);
        return true; // Nothing to do for this behavior.
    }
    */
    
    return false; // This behavior isn't done yet
}

bool WatchOperation::OutOfRange(NPC *npc)
{
    npcMesh* pcmesh = npc->GetActor()->pcmesh;
    gemNPCActor * npcEnt = npc->GetActor();
    
    npcMesh* watchedMesh = watchedEnt->pcmesh;

    // Position
    if(!pcmesh->GetMesh() || !watchedMesh)
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR AN OBJECT %s %s!\n",npcEnt->GetName(), watchedEnt->GetName());
        return true;
    }

    if (npc->GetActor()->GetInstance() != watchedEnt->GetInstance())
    {
        npc->Printf(7,"Watched is in diffrent instance.");
        return true;
    }

    float distance = npcclient->GetWorld()->Distance(pcmesh->GetMesh(), watchedMesh->GetMesh());
    
    if (distance > watchRange)
    {
        npc->Printf(7,"Watched is %.2f away.",distance);
        return true;
    }
    
    return false;
}


void WatchOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{
    if (!watchedEnt)
    {
        npc->Printf(5,"No entity to watch");
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return; // Nothing to do for this behavior.
    }

    if (OutOfRange(npc))
    {
        csString str;
        str.Append(typeStr[type]);
        str.Append(" out of range");
        Perception range(str);
        npc->TriggerEvent(&range);
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return; // Lost track of the watched entity.
    }
}

void WatchOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
}

bool WatchOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    completed = true;

    return true;
}

//---------------------------------------------------------------------------

