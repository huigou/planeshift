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
#include <ivaria/mapnode.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "globals.h"

#include "engine/linmove.h"
#include "engine/psworld.h"

#include "util/psconst.h"
#include "util/waypoint.h"
#include "util/pspath.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/psstring.h"
#include "util/eventmanager.h"
#include "util/log.h"

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
    : // Instance parameters
      name(scriptName),
      completed(true), 
      resumeScriptEvent(NULL),
      interrupted_sector(NULL),
      interrupted_angle(0.0f),
      consecCollisions(0),
      // Shared parameters between operations
      velSource(VEL_DEFAULT),
      vel(0.0),
      ang_vel(0.0),
      collision("collision"),
      outOfBounds("out of bounds"),
      inBounds("in bounds"),
      checkTribeHome(false)
{
}

ScriptOperation::ScriptOperation(const ScriptOperation* other)
    : // Instance parameters
      name(other->name),
      completed(true), 
      resumeScriptEvent(NULL),
      interrupted_sector(NULL),
      interrupted_angle(0.0f),
      consecCollisions(0),
      // Shared parameters between operations
      failurePerception(other->failurePerception),
      velSource(other->velSource),
      vel(other->vel),
      ang_vel(other->ang_vel),
      collision(other->collision),
      outOfBounds(other->outOfBounds),
      inBounds(other->inBounds),
      checkTribeHome(other->checkTribeHome)
{
}

float ScriptOperation::GetVelocity(NPC *npc)
{
    switch (velSource)
    {
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
    if (node->GetAttribute("falling"))
    {
        falling = node->GetAttributeValue("falling");
    }
    
    if (node->GetAttribute("check_tribe_home"))
    {
        checkTribeHome = node->GetAttributeValueAsBool("check_tribe_home",false);
    }
    
    return true;
}

void ScriptOperation::CopyCheckMoveOk(ScriptOperation * source)
{
    collision = source->collision;
    outOfBounds = source->outOfBounds;
    inBounds = source->inBounds;
    falling = source->falling;
    checkTribeHome = source->checkTribeHome;
}

bool ScriptOperation::Load(iDocumentNode *node)
{
    failurePerception = node->GetAttributeValue("failure");

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

void ScriptOperation::SendCollitionPerception(NPC* npc)
{
    csString collision = GetCollisionPerception(npc);
    if (!collision.IsEmpty())
    {
        Perception perception(collision);
        npc->TriggerEvent(&perception);
    }
}

bool ScriptOperation::CheckMoveOk(NPC *npc, EventManager *eventmgr, csVector3 oldPos, iSector* oldSector, const csVector3 & newPos, iSector* newSector, float timedelta)
{
    npcMesh* pcmesh = npc->GetActor()->pcmesh;

    if (!npcclient->GetWorld()->WarpSpace(oldSector, newSector, oldPos))
    {
        npc->Printf("CheckMoveOk: new and old sectors are not connected by a portal!");
        return false;
    }

    if ((oldPos - newPos).SquaredNorm() < 0.01f) // then stopped dead, presumably by collision
    {
        // We collided. Now stop the movment and inform the server.
        StopMovement(npc);
        
        // Send perception for collision
        SendCollitionPerception(npc);
        
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

        float diffExpectedX = fabs(newPos.x - expected_pos.x);
        float diffExpectedZ = fabs(newPos.z - expected_pos.z);


        float diffPositionY = fabs(newPos.y - oldPos.y);

        // Check if this NPC is falling.
        if (fabs(diffPositionY) > 800.0 || fabs(newPos.y) > 800.0 )  
        {                         

            npc->IncrementFallCounter();

            // Send Falling perception
            csString falling = GetFallingPerception(npc);
            if (!falling.IsEmpty())
            {
                Perception perception(falling);
                npc->TriggerEvent(&perception);
            }

            Error3("NPC %s starts to fall pos %s",
                   ShowID(npc->GetPID()),
                   toString(oldPos,oldSector).GetDataSafe());
                   
            npc->Dump();

            return false;
        }

        if (diffExpectedX > EPSILON || diffExpectedZ > EPSILON)
        {
            consecCollisions++;
            npc->Printf(10,"Bang. %d consec collisions last with diffs (%1.2f,%1.2f)...",
                        consecCollisions,diffExpectedX,diffExpectedZ);
            if (consecCollisions > 8)  // allow for hitting trees but not walls
            {

                // Now we collided so may times, that we stop
                StopMovement(npc);
 
                // After a couple seconds of sliding against something
                // the npc should give up and react to the obstacle.
                // Send perception for collision
                SendCollitionPerception(npc);

                return false;
            }

            // We have not ended up in the expected positon. Send updated position to server.
            npcclient->GetNetworkMgr()->QueueDRData(npc);
        }
        else
        {
            consecCollisions = 0;
        }

        LocationType *rgn = npc->GetRegion();

        if (rgn)
        {
            npc->Printf(10, "Checking Region bounds");
            
            // check for inside/outside region bounds
            if (npc->IsInsideRegion())
            {
                if (!rgn->CheckWithinBounds(npcclient->GetEngine(),newPos,newSector))
                {
                    csString outOfBounds = GetOutOfBoundsPerception(npc);
                    if (!outOfBounds.IsEmpty())
                    {
                        Perception outbounds(outOfBounds);
                        npc->TriggerEvent(&outbounds);
                    }

                    npc->SetInsideRegion(false);
                }
            }
            else
            {
                if (rgn->CheckWithinBounds(npcclient->GetEngine(),newPos,newSector))
                {
                    csString inBounds = GetInBoundsPerception(npc);
                    if (!inBounds.IsEmpty())
                    {
                        Perception perception(inBounds);
                        npc->TriggerEvent(&perception);
                    }

                    npc->SetInsideRegion(true);
                }
            }
        }
        else
        {
            npc->Printf(10, "No region to check for bounds.");
        }
        

        // If tribehome checking has been enabled and npc is in a tribe
        if (checkTribeHome && npc->GetTribe())
        {
            npc->Printf(10, "Checking Tribe bounds");

            if (npc->IsInsideTribeHome())
            {
                if (!npc->GetTribe()->CheckWithinBoundsTribeHome(npc,newPos,newSector))
                {
                    csString outOfBounds = GetOutOfBoundsPerception(npc);
                    if (!outOfBounds.IsEmpty())
                    {
                        Perception outbounds(outOfBounds);
                        npc->TriggerEvent(&outbounds);
                    }

                    npc->SetInsideTribeHome(false);
                }
            }
            else
            {
                if (npc->GetTribe()->CheckWithinBoundsTribeHome(npc,newPos,newSector))
                {
                    csString inBounds = GetInBoundsPerception(npc);
                    if (!inBounds.IsEmpty())
                    {
                        Perception inbounds(inBounds);
                        npc->TriggerEvent(&inbounds);
                    }

                    npc->SetInsideTribeHome(true);
                }                
            }
        }
        else
        {
            npc->Printf(10, "No tribe home to check for bounds.");
        }
        
    }

    return true;
}

const csString& ScriptOperation::GetCollisionPerception(NPC* npc)
{
    if (collision.IsEmpty())
    {
        return npc->GetBrain()->GetCollisionPerception();
    }
    return collision;
}

const csString& ScriptOperation::GetOutOfBoundsPerception(NPC* npc)
{
    if (outOfBounds.IsEmpty())
    {
        return npc->GetBrain()->GetOutOfBoundsPerception();
    }
    return outOfBounds;
}

const csString& ScriptOperation::GetInBoundsPerception(NPC* npc)
{
    if (inBounds.IsEmpty())
    {
        return npc->GetBrain()->GetInBoundsPerception();
    }
    return inBounds;
}

const csString& ScriptOperation::GetFallingPerception(NPC* npc)
{
    if (falling.IsEmpty())
    {
        return npc->GetBrain()->GetFallingPerception();
    }
    return falling;
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


void ScriptOperation::TurnTo(NPC *npc, const csVector3& dest, iSector* destsect, csVector3& forward, float &angle)
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

    angle = psGameObject::CalculateIncidentAngle(pos,dest);
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


int ScriptOperation::StartMoveTo(NPC *npc, EventManager *eventmgr, const csVector3& dest, iSector* sector, float vel, const char *action, bool autoresume, float &angle)
{
    csVector3 forward;
    
    TurnTo(npc, dest, sector, forward, angle);
    
    // SetAction animation for the mesh also, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    float dist = forward.Norm();
    int msec = (int)(1000.0 * dist / GetVelocity(npc)); // Move will take this many msecs.
    
    npc->Printf(6,"MoveTo op should take approx %d msec.  ", msec);
    if (autoresume && eventmgr != NULL)
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

void ScriptOperation::AddRandomRange(csVector3& dest, float radius, float margine)
{
    // Adjust for margines.
    radius -= margine;
    if (radius < 0.0)
    {
        return; // Don't add random if there are no margine.
    }

    float angle = psGetRandom()*TWO_PI;
    float range = psGetRandom()*radius;

    dest.x += cosf(angle)*range;
    dest.z += sinf(angle)*range;
}

void ScriptOperation::SetAnimation(NPC * npc, const char*name)
{
    //npc->GetActor()->pcmesh->SetAnimation(name, false);
}

void ScriptOperation::Failure(NPC* npc)
{
    if (failurePerception.IsEmpty())
    {
        npc->Printf(5,"Operation failed with no failure perception");
    }
    else
    {
        npc->Printf(5,"Operation failed tigger failure perception: %s",
                    failurePerception.GetData());

        Perception perception(failurePerception);
        npc->TriggerEvent(&perception);
    }
}


//---------------------------------------------------------------------------

MovementOperation::MovementOperation(const char* name)
    :ScriptOperation( name ),currentDistance(0.0f)
{
}

MovementOperation::MovementOperation(const MovementOperation* other)
    :ScriptOperation( other ),
     // Instance variables
     currentDistance(0.0f),
     // Operation parameters
     action(other->action)
{
}

bool MovementOperation::Load(iDocumentNode *node)
{
    if (!ScriptOperation::Load(node))
    {
        return false;
    }

    action = node->GetAttributeValue("anim");

    return true;
}

bool MovementOperation::EndPointChanged(const csVector3 &endPos, const iSector* endSector) const
{
    iMapNode* pathEndDest = path->GetLast();
    return ((pathEndDest->GetSector() != endSector) ||
            (psGameObject::Calc2DDistance(pathEndDest->GetPosition(), endPos) > EPSILON) );
}

ScriptOperation::OperationResult MovementOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    iSector* mySector;
    csVector3 myPos;

    psGameObject::GetPosition(npc->GetActor(), myPos, mySector);

    if (!GetEndPosition(npc, myPos, mySector, endPos, endSector))
    {
        npc->Printf(5, "Failed to find target position!");
        return OPERATION_FAILED;  // This operation is complete
    }


    path = npcclient->GetNavStruct()->ShortestPath(myPos,mySector,endPos,endSector);
    if(!path || !path->HasNext())
    {
        // failed to find a path between us and the target
        npc->Printf(5, "Failed to find a path between %s and %s", 
                    toString(myPos, mySector).GetData(), 
                    toString(endPos, endSector).GetData());
        
        return OPERATION_FAILED;  // This operation is complete
    }
    else if ( path->GetDistance() < 0.5 )  // Distance allready adjusted for offset
    {
        npc->Printf(5, "We are done..");

        return OPERATION_COMPLETED; // This operation is complete
    }
    else if ( GetAngularVelocity(npc) > 0 || GetVelocity(npc) > 0 )
    { 
        float dummyAngle;
        
        // Find next local destination and start moving towords local destination
        iMapNode* dest = path->Next();
        StartMoveTo(npc, eventmgr, dest->GetPosition(), dest->GetSector(), GetVelocity(npc),
                    action, false, dummyAngle);
        currentDistance =  npcclient->GetWorld()->Distance(myPos, mySector,
                                                           dest->GetPosition(), dest->GetSector());

        return OPERATION_NOT_COMPLETED; // This behavior isn't done yet
    }

    return OPERATION_COMPLETED; // This operation is complete
}

void MovementOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{

    csVector3    myPos;
    float        myRot;
    iSector*     mySector;
    csVector3    forward;
    float        angle;

    npc->GetLinMove()->GetLastPosition(myPos, myRot, mySector);

    if (!UpdateEndPosition(npc, myPos, mySector, endPos, endSector ))
    {
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
        return;
    }

    // Check if path endpoint has changed and needs to be updated
    if(EndPointChanged( endPos, endSector ))
    {
        npc->Printf(8, "target diverged, recalculate path between %s and %s", 
                        toString(myPos, mySector).GetData(), 
                        toString(endPos, endSector).GetData());

        path = npcclient->GetNavStruct()->ShortestPath(myPos, mySector, endPos, endSector);
        if(!path || !path->HasNext())
        {
            npc->Printf(5, "Failed to find a path between %s and %s", 
                        toString(myPos, mySector).GetData(), 
                        toString(endPos, endSector).GetData());
            npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
            return;
        }
        // Start moving toward new dest
        iMapNode* dest = path->Next();
        StartMoveTo(npc, eventmgr, dest->GetPosition(), dest->GetSector(), GetVelocity(npc),
                    action, false, angle);
        currentDistance =  npcclient->GetWorld()->Distance(myPos, mySector,
                                                           dest->GetPosition(), dest->GetSector());
    }

    iMapNode* dest = path->Current();
    float distance = npcclient->GetWorld()->Distance(myPos,mySector,dest->GetPosition(),dest->GetSector());
    if (distance >= INFINITY_DISTANCE)
    {
        npc->Printf(5, "No connection found..");
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
        return;
    }
    else if (distance <= 0.5f || distance > currentDistance)
    {
        if (distance > currentDistance )
        {
            npc->Printf(6, "We passed localDest...");
        }
        else
        {
            npc->Printf(6, "We are at localDest...");
        }

        if (!path->HasNext())
        {
            npc->Printf(5, "We are done..");
            npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
            return;
        }
        else
        {
            dest = path->Next();
            StartMoveTo(npc, eventmgr, dest->GetPosition(), dest->GetSector(), GetVelocity(npc),
                        action, false, angle);
            currentDistance =  npcclient->GetWorld()->Distance(myPos, mySector,
                                                               dest->GetPosition(), dest->GetSector());
        }
    }
    else
    {
        TurnTo(npc, dest->GetPosition(), dest->GetSector(), forward, angle);
    }

    // Limit time extrapolation so we arrive near the correct place.
    float close = GetVelocity(npc)*timedelta;
    if(distance <= close)
    {
    	timedelta = distance / GetVelocity(npc);
    }

    npc->Printf(8, "advance: pos=(%f.2,%f.2,%f.2) rot=%.2f %s localDest=%s dist=%f", 
                myPos.x,myPos.y,myPos.z, myRot, mySector->QueryObject()->GetName(),
                dest->GetPosition().Description().GetData(),distance);

    {
        int ret;
        ScopedTimer st(250, "Movement extrapolate %.2f time for %s", timedelta, ShowID(npc->GetActor()->GetEID()));
        ret = npc->GetLinMove()->ExtrapolatePosition(timedelta);
    }

    {
        bool on_ground;
        float speed,ang_vel;
        csVector3 bodyVel,worldVel,myNewPos;
        iSector* myNewSector;

        npc->GetLinMove()->GetDRData(on_ground,speed,myNewPos,myRot,myNewSector,bodyVel,worldVel,ang_vel);
        npc->Printf(8,"World position bodyVel=%s worldVel=%s",
                       toString(bodyVel).GetDataSafe(),toString(worldVel).GetDataSafe());

        CheckMoveOk(npc, eventmgr, myPos, mySector, myNewPos, myNewSector, timedelta);
    }
}

void MovementOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool MovementOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    StopMovement(npc);

    completed = true;

    return true;  // Script can keep going
}

//---------------------------------------------------------------------------
//         Following section contain specefix NPC operations.
//         Ordered alphabeticaly
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

const char * ChaseOperation::typeStr[]={"nearest_actor","nearest_npc","nearest_player","owner","target"};

ChaseOperation::ChaseOperation()
    : MovementOperation("Chase"),
      // Instance variables
      targetEID(EID(0)),
      offsetAngle(0.0),
      // Operation parameters
      // Initialized in the load function
      type(NEAREST_PLAYER),
      searchRange(2.0),
      chaseRange(-1.0),
      offset(0.5),
      offsetAngleMax(0.0),
      sideOffset(0.0),
      offsetRelativeHeading(false)
{ 
}

ChaseOperation::ChaseOperation( const ChaseOperation* other )
    : MovementOperation( other ),
      // Instance variables
      targetEID(other->targetEID),
      // Operation parameters
      type(other->type),
      searchRange(other->searchRange),
      chaseRange(other->chaseRange),
      offset(other->offset),
      offsetAngleMax(other->offsetAngleMax),
      sideOffset(other->sideOffset),
      offsetRelativeHeading(other->offsetRelativeHeading)
{ 
    // Select a offset_angle for this instance of chase
    offsetAngle = (2.0f*psGetRandom()-1.0f)*offsetAngleMax;
}


bool ChaseOperation::Load(iDocumentNode *node)
{
    if (!MovementOperation::Load(node))
    {
        return false;
    }

    csString typestr = node->GetAttributeValue("type");
    if (typestr.CompareNoCase("nearest_actor"))
    {
        type = NEAREST_ACTOR;
    }
    else if (typestr.CompareNoCase("nearest_npc"))
    {
        type = NEAREST_NPC;
    }
    else if (typestr.CompareNoCase("nearest_player"))
    {
        type = NEAREST_PLAYER;
    }
    else if (typestr.CompareNoCase("owner"))
    {
        type = OWNER;
    }
    else if (typestr.CompareNoCase("target"))
    {
        type = TARGET;
    }
    else
    {
        Error2("Chase operation can't understand a type of %s",typestr.GetDataSafe());
        return false;
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
    else
    {
    	offset = 0.5f;
    }

    if ( node->GetAttributeValue("side_offset") )
    {
        sideOffset = node->GetAttributeValueAsFloat("side_offset");
    }
    else
    {
    	sideOffset = 0.0f;
    }
    offsetRelativeHeading = node->GetAttributeValueAsBool("offset_relative_heading",false);

    if ( node->GetAttributeValue("offset_angle") )
    {
        offsetAngleMax = node->GetAttributeValueAsFloat("offset_angle")*PI/180.0f;
        offsetAngle = (2.0f*psGetRandom()-1.0f)*offsetAngleMax; // Select a offset_angle for this instance
    }
    else
    {
    	offsetAngleMax = 0.0f;
    }

    LoadVelocity(node);
    LoadCheckMoveOk(node);
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");

    return true;
}

ScriptOperation *ChaseOperation::MakeCopy()
{
    ChaseOperation *op = new ChaseOperation( this );
    return op;
}

csVector3 ChaseOperation::CalculateOffsetDelta(const csVector3 &myPos, const iSector* mySector,
                                               const csVector3 &endPos, const iSector* endSector,
                                               float endRot) const
{
    csVector3 offsetDelta;

    if (offsetRelativeHeading)
    {
        // Calculate the offset relative to the heading of the end target
        offsetDelta = csYRotMatrix3(endRot)*csVector3(offset, 0.0, sideOffset);
    }
    else
    {
        // Calculate the offset relative to the tangent between myPos and endPos
        offsetDelta = psGameObject::DisplaceTargetPos(mySector, myPos,
                                                      endSector, endPos, offset);

        if (offsetAngle != 0.0)
        {
            offsetDelta = csYRotMatrix3(offsetAngle)*offsetDelta;
        }

        if (sideOffset != 0.0)
        {
            csVector3 sideOffsetDelta = psGameObject::DisplaceTargetPos(mySector, myPos,
                                                                        endSector, endPos, sideOffset);
            // Rotate 90 degree to make it a side offset to the line between NPC and target.
            sideOffsetDelta = csYRotMatrix3(PI/2.0)*sideOffsetDelta;
            offsetDelta += sideOffsetDelta;
        }

    }    
    return offsetDelta;
}


bool ChaseOperation::GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector, csVector3 &endPos, iSector* &endSector)
{
    float targetRot;
    float targetRange;
    
    gemNPCObject *targetEntity = NULL;
    targetEID = EID(0);
        
    switch (type)
    {
    case NEAREST_PLAYER:
        targetEntity = npc->GetNearestPlayer(searchRange, endPos, endSector, targetRange);
        if (targetEntity)
        {
            targetEID = targetEntity->GetEID();

            npc->Printf(6, "Targeting nearest player (%s) at %s range %.2f for chase ...",
                        targetEntity->GetName(), toString(endPos,endSector).GetDataSafe(),
                        targetRange);
        }
        break;
    case NEAREST_ACTOR:
        targetEntity = npc->GetNearestActor(searchRange, endPos, endSector, targetRange);
        if (targetEntity)
        {
            targetEID = targetEntity->GetEID();

            npc->Printf(6, "Targeting nearest actor (%s) at %s range %.2f for chase ...",
                        targetEntity->GetName(), toString(endPos,endSector).GetDataSafe(), 
                        targetRange);
        }
        break;
    case OWNER:
        {
            // Check if we have a owner
            gemNPCObject * owner = npc->GetOwner();
            if (owner)
            {
                // Assign owner as entity to chase
                targetEntity = owner;

                targetEID = targetEntity->GetEID();
                psGameObject::GetPosition(targetEntity, endPos, targetRot, endSector);
                npc->Printf(6, "Targeting owner (%s) at %s for chase ...",
                            targetEntity->GetName(), toString(endPos,endSector).GetDataSafe());
            }
        }
        break;
    case TARGET:
        {
            // Check if we have a target
            gemNPCObject * target = npc->GetTarget();
            if (target)
            {
                // Assign target as entity to chase
                targetEntity = target;

                targetEID = targetEntity->GetEID();
                psGameObject::GetPosition(targetEntity, endPos, endSector);
                npc->Printf(6, "Targeting current target (%s) at %s for chase ...",
                            targetEntity->GetName(), toString(endPos,endSector).GetDataSafe());
            }
        }
        break;
    }

    if (!targetEID.IsValid() || !targetEntity)
    {
        npc->Printf(5, "No one found to chase!");
        return false;
    }
    if (targetEntity->GetInstance() != npc->GetActor()->GetInstance())
    {
        npc->Printf(5, "Target found in different instance, no one found to chase!");
        return false;
    }


    psGameObject::GetPosition(targetEntity, endPos, targetRot, endSector);

    offsetDelta = CalculateOffsetDelta(myPos, mySector, endPos, endSector, targetRot );

    npc->Printf(5, "Chasing enemy <%s, %s> at %s with offset %s", targetEntity->GetName(),
                ShowID(targetEntity->GetEID()),
                toString(endPos,endSector).GetDataSafe(),
                toString(offsetDelta).GetDataSafe());

    // This prevents NPCs from wanting to occupy the same physical space as something else
    endPos -= offsetDelta;
    // TODO: Check if new endPos is within same sector!!!


    return true;
}

gemNPCActor* ChaseOperation::UpdateChaseTarget(NPC* npc, const csVector3 &myPos, const iSector* mySector)
{
    csVector3    targetPos;
    iSector*     targetSector;
    gemNPCActor* targetEntity = NULL;


    // If chaseing nearest and there is a new target within search range the target is changed
    if (type == NEAREST_PLAYER)
    {   
        float dummyRange;

        // Switch target if a new entity is withing search range.
        gemNPCActor* newTarget = npc->GetNearestPlayer(searchRange, targetPos, targetSector, dummyRange );

        if (newTarget)
        {
            targetEntity = newTarget;

            if (targetEID != targetEntity->GetEID())
            {
                npc->Printf(5, "Changing chase to new player %s", newTarget->GetName());

                // Calculate an offset point from the target. 
                // This point is used until new target is found
                offsetDelta = psGameObject::DisplaceTargetPos(mySector, myPos,
                                                              targetSector, targetPos, offset);
                offsetDelta = csMatrix3(0.0,1.0,0.0,offsetAngle)*offsetDelta;
            }


            // Update chase operations target
            targetEID = targetEntity->GetEID();
        }
 
    }
    else if (type == NEAREST_ACTOR)
    {   
        float dummyRange;

        // Switch target if a new entity is withing search range.
        gemNPCActor* newTarget = npc->GetNearestActor(searchRange, targetPos, targetSector, dummyRange );

        if (newTarget)
        {
            targetEntity = newTarget;

            if (targetEID != targetEntity->GetEID())
            {
                npc->Printf(5, "Changing chase to new actor %s", newTarget->GetName());

                // Calculate an offset point from the target. 
                // This point is used until new target is found
                offsetDelta = psGameObject::DisplaceTargetPos(mySector, myPos,
                                                              targetSector, targetPos, offset);
                offsetDelta = csMatrix3(0.0,1.0,0.0,offsetAngle)*offsetDelta;
            }

            // Update chase operations target
            targetEID = targetEntity->GetEID();
        }
 
    }

    // If no entity found yet find entity from targetEID
    if (!targetEntity) 
    {   
        targetEntity = dynamic_cast<gemNPCActor*>(npcclient->FindEntityID(targetEID));
    }

    return targetEntity;
}

bool ChaseOperation::UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                       csVector3 &targetPos, iSector* &targetSector)
{
    gemNPCActor* targetEntity = UpdateChaseTarget(npc, myPos, mySector);
    if (!targetEntity) // No target entity anymore
    {
        npc->Printf(5, "ChaseOp has no target now!");
        csString str;
        str.Append(typeStr[type]);
        str.Append(" out of target");
        Perception range(str);
        npc->TriggerEvent(&range);
        return false;
    }

    if (targetEntity->GetInstance() != npc->GetActor()->GetInstance())
    {
        npc->Printf(5, "Target found in different instance, no one found to chase!");
        return false;
    }

    float targetRot;
    psGameObject::GetPosition(targetEntity, targetPos, targetRot, targetSector);


    // Check if we shold stop chaseing
    float distance = npcclient->GetWorld()->Distance(myPos,mySector,targetPos,targetSector);
    if ( (distance >= INFINITY_DISTANCE) ||
         ((chaseRange > 0) && (distance > chaseRange)) )
    {
        npc->Printf(5, "Target out of chase range -> we are done..");
        csString str;
        str.Append(typeStr[type]);
        str.Append(" out of chase");
        Perception range(str);
        npc->TriggerEvent(&range);
        return false;
    }

    // This prevents NPCs from wanting to occupy the same physical space as something else
    if (offsetRelativeHeading)
    {
        offsetDelta = CalculateOffsetDelta(myPos, mySector, endPos, endSector, targetRot );
    }
    targetPos -= offsetDelta;
    // TODO: Check if new targetPos is within same sector!!!

    return true;
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

ScriptOperation::OperationResult CircleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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
        duration = fabs(angle)*radius/GetVelocity(npc);
    }
    
    if (ang_vel == 0)
    {
        ang_vel = GetVelocity(npc)/radius;
    }

    return MoveOperation::Run(npc, eventmgr, interrupted);
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

ScriptOperation::OperationResult DebugOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (exclusive.Length())
    {
        static bool debug_exclusive = false;
        
        if (level && debug_exclusive)
        {
            // Can't turn on when exclusive is set.
            return OPERATION_COMPLETED; // Nothing more to do for this op.
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

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool DequipOperation::Load(iDocumentNode *node)
{
    slot   = node->GetAttributeValue("slot");

    if (slot.IsEmpty())
    {
        Error1("No slot defined for Dequip operation");
        return false;
    }
    
    return true;
}

ScriptOperation *DequipOperation::MakeCopy()
{
    DequipOperation *op = new DequipOperation;
    op->slot   = slot;
    return op;
}

ScriptOperation::OperationResult DequipOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s Where: %s",
                npc->GetName(), slot.GetData());

    npcclient->GetNetworkMgr()->QueueDequipCommand(npc->GetActor(), slot );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool DigOperation::Load(iDocumentNode *node)
{
    resource = node->GetAttributeValue("resource");
    if (resource.IsEmpty())
    {
        Error1("No resource defined for Dig operation");
        return false;
    }
    
    return true;
}

ScriptOperation *DigOperation::MakeCopy()
{
    DigOperation *op = new DigOperation;

    op->resource = resource;

    return op;
}

ScriptOperation::OperationResult DigOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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
    

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool DropOperation::Load(iDocumentNode *node)
{
    slot = node->GetAttributeValue("slot");
    if (slot.IsEmpty())
    {
        Error1("Drop operation must have a slot defined");
        return false;
    }
    
    return true;
}

ScriptOperation *DropOperation::MakeCopy()
{
    DropOperation *op = new DropOperation;
    op->slot = slot;
    return op;
}

ScriptOperation::OperationResult DropOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueDropCommand(npc->GetActor(), slot );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool EatOperation::Load(iDocumentNode *node)
{
    resource = node->GetAttributeValue("resource");
    if (resource.IsEmpty()) return false;
    return true;
}

ScriptOperation *EatOperation::MakeCopy()
{
    EatOperation *op = new EatOperation;
    op->resource = resource;
    return op;
}

ScriptOperation::OperationResult EatOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    csString res = resource;
    
    if (resource == "tribe:wealth")
    {
        if (npc->GetTribe())
        {
            res = npc->GetTribe()->GetNeededResourceNick();
        }
    }

    gemNPCActor *ent = npc->GetNearestDeadActor(1.0);
    if (ent)
    {
        // Take a bite :)
        if (npc->GetTribe())
        {
            npc->GetTribe()->AddResource(res,1);
        }
    }

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool EmoteOperation::Load(iDocumentNode *node)
{
    cmd    = node->GetAttributeValue("cmd");
    if (cmd.IsEmpty())
    {
        Error1("Emote operation needs an cmd");
        return false;
    }
    return true;
}

ScriptOperation *EmoteOperation::MakeCopy()
{
    EmoteOperation *op = new EmoteOperation;
    op->cmd    = cmd;
    return op;
}

ScriptOperation::OperationResult EmoteOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s Emote: %s", npc->GetName(), cmd.GetData());

    npcclient->GetNetworkMgr()->QueueEmoteCommand(npc->GetActor(), npc->GetTarget(), cmd);

    return OPERATION_COMPLETED;
}

//---------------------------------------------------------------------------

bool EquipOperation::Load(iDocumentNode *node)
{
    item    = node->GetAttributeValue("item");
    if (item.IsEmpty())
    {
        Error1("Equip operation needs an item");
        return false;
    }
    slot    = node->GetAttributeValue("slot");
    if (slot.IsEmpty())
    {
        Error1("Equip operation needs a slot");
        return false;
    }
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

ScriptOperation::OperationResult EquipOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s What: %s Where: %s Count: %d",
                npc->GetName(),item.GetData(), slot.GetData(), count);

    npcclient->GetNetworkMgr()->QueueEquipCommand(npc->GetActor(), item, slot, count);

    return OPERATION_COMPLETED;
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

ScriptOperation::OperationResult InvisibleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetActor(), false);
    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

LocateOperation::LocateOperation()
    : ScriptOperation("Locate"),
      // Instance variables
      located(false)
      // Operation parameters
      // Initialized in the load function
{
}

LocateOperation::LocateOperation(const LocateOperation* other)
    : ScriptOperation(other),
      // Instance variables
      located(false),
      // Operation parameters
      object(other->object),
      range(other->range),
      static_loc(other->static_loc),
      random(other->random),
      locate_invisible(other->locate_invisible),
      locate_invincible(other->locate_invincible)
{
}

bool LocateOperation::Load(iDocumentNode *node)
{
    // Load common stuff from script operation
    if (!ScriptOperation::Load(node))
    {
        Error1("Locate operation failed to load common script operation parameters");
        return false;
    }

    object = node->GetAttributeValue("obj");
    if (object.IsEmpty())
    {
        Error1("Locate operation need obj attribute");
        return false;
    }
    
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

    // Some more validation checks on obj.
    csArray<csString> split_obj = psSplit(object,':');   
    if (split_obj[0] == "entity")
    {
        if (split_obj.GetSize() < 2)
        {
            Error1("Locate operation with obj attribute of entity without sub type.");
            return false;
        }
        else if (split_obj[1] == "name")
        {
            if (split_obj[2].IsEmpty())
            {
                Error1("Locate operation with obj attribute entity:name:<name> is missing the name.");
                return false;
            }
            return true;
        }
        else if (split_obj[1] == "pid")
        {
            if (split_obj[2].IsEmpty())
            {
                Error1("Locate operation with obj attribute entity:pid:<pid> is missing the pid.");
                return false;
            }
            else if (atoi(split_obj[2]) == 0)
            {
                Error2("Locate operation with obj attribute entity:pid:<pid> where pid '%s' isn't a number or is zero.",split_obj[2].GetDataSafe() );
                return false;
            }

            return true;
        }
        return false;
    }
    else if (split_obj[0] == "perception")
    {
        return true;
    }
    else if (split_obj[0] == "target")
    {
        if (range <= 0)
        {
            range = 10.0;
        }
        return true;
    }
    else if (split_obj[0] == "tribe_target")
    {
        return true;
    }
    else if (split_obj[0] == "owner")
    {
        return true;
    }
    else if (split_obj[0] == "region")
    {
        return true;
    }
    else if (split_obj[0] == "self")
    {
        return true;
    }
    else if (split_obj[0] == "spawn")
    {
        return true;
    }
    else if (split_obj[0] == "tribe")
    {
        if (split_obj.GetSize() < 2)
        {
            Error1("Locate operation with obj attribute of tribe without sub type.");
            return false;
        }
        else if (split_obj[1] == "home")
        {
            return true;
        }
        else if (split_obj[1] == "memory")
        {
            if (split_obj.GetSize() < 2 || split_obj[2].IsEmpty())
            {
                Error1("Locate operation with obj attribute tribe:memory:<name> is missing the name.");
                return false;
            }
            return true;
        }
        else if (split_obj[1] == "resource")
        {
            return true;
        }
        else
        {
            Error2("Locate operation with wrong obj attribute: %s",object.GetDataSafe());
            return false;
        }
    }
    else if (split_obj[0] == "friend")
    {
        return true;
    }
    else if (split_obj[0] == "waypoint" )
    {
        if (split_obj.GetSize() > 1)
        {
            if (split_obj[1] == "group")
            {
                if (split_obj.GetSize() < 3)
                {
                    Error1("Locate operation with obj attribute of waypoint:group:<name> without name");
                    return false;
                }
                return true;
            }
            else if (split_obj[1] == "name")
            {
                if (split_obj.GetSize() < 3)
                {
                    Error1("Locate operation with obj attribute of waypoint:name:<name> without name");
                    return false;
                }
                return true;
            }
        }
        // OK with waypoint without any sub types.
        return true; 
    }
    else
    {
        // Ok without any keywords just name of a location.
        // Length has been check when reading.
    }

    return true;
}

ScriptOperation *LocateOperation::MakeCopy()
{
    LocateOperation *op = new LocateOperation( this );
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


ScriptOperation::OperationResult LocateOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    // Reset old target
    npc->SetTarget(NULL);

    located_pos = csVector3(0.0f,0.0f,0.0f);
    located_angle = 0.0f;
    located_sector = NULL;
    located_wp = NULL;
    located_radius = 0.0f;

    float start_rot;
    iSector *start_sector;
    csVector3 start_pos;
    psGameObject::GetPosition(npc->GetActor(),start_pos,start_rot,start_sector);

    ReplaceVariables(object,npc);

    csArray<csString> split_obj = psSplit(object,':');
 
    if (split_obj[0] == "entity")
    {
        npc->Printf(5,"LocateOp - Entity");

        gemNPCObject *entity = NULL;

        if (split_obj[1] == "name")
        {
            entity = npcclient->FindEntityByName(split_obj[2]);
        }
        else if (split_obj[1] == "pid")
        {
            entity = npcclient->FindCharacterID(atoi(split_obj[2]));
        }

        if(entity)
        {
            npc->SetTarget(entity);
        }
        else
        {
            return OPERATION_FAILED; // Nothing more to do for this op.
        }

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(entity,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "perception")
    {
        npc->Printf(5,"LocateOp - Perception");

        if (!npc->GetLastPerception())
        {
            return OPERATION_FAILED;  // Nothing more to do for this op.
        }
        if (!npc->GetLastPerception()->GetLocation(located_pos,located_sector))
        {
            return OPERATION_FAILED;  // Nothing more to do for this op.
        }
        located_angle = 0; // not used in perceptions
    }
    else if (split_obj[0] == "target")
    {
        npc->Printf(5,"LocateOp - Target");

        // Since we don't have a current enemy targeted, find one!
        gemNPCActor* ent = npc->GetMostHated(range,locate_invisible,locate_invincible);

        if(ent)
        {
            npc->SetTarget(ent);
        }
        else
        {
            return OPERATION_FAILED;  // Nothing more to do for this op.
        }

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "tribe_target")
    {
        npc->Printf(5,"LocateOp - Tribe Target");

        if (!npc->GetTribe())
        {
            return OPERATION_FAILED; //  Nothing more to do for this op.
        }

        // Since we don't have a current enemy targeted, find one!
        gemNPCActor* ent = npc->GetTribe()->GetMostHated(npc, range, locate_invisible, locate_invincible);

        if(ent)
        {
            npc->SetTarget(ent);
        }
        else
        {
            return OPERATION_FAILED;  // Nothing more to do for this op.
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
            npc->Printf("Failed to find owner");
            return OPERATION_FAILED; // Nothing more to do for this op.
        }

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(owner,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "region")
    {
        npc->Printf(5,"LocateOp - Region");
        LocationType* region = npc->GetRegion();
        if (!region)
        {
            npc->Printf("Failed to locate region");
            return OPERATION_FAILED; // Nothing more to do for this op.
        }

        iSector *sector = NULL;
        csVector3 pos = csVector3(0.0f,0.0f,0.0f);

        if (region->GetRandomPosition(npcclient->GetEngine(),pos,sector))
        {
            located_pos = pos;
            located_angle = 0;
            located_sector = sector;

            // Find closest waypoint to the random location in the region
            located_wp = CalculateWaypoint(npc,located_pos,located_sector,-1);
        }
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
            return OPERATION_FAILED; // Nothing more to do for this op.
        }

        float rot;
        iSector *sector;
        csVector3 pos;
        psGameObject::GetPosition(ent,pos,rot,sector);

        located_pos = pos;
        located_angle = 0;
        located_sector = sector;
    }
    else if (split_obj[0] == "spawn")
    {
        npc->Printf(5,"LocateOp - Spawn");

        located_pos = npc->GetSpawnPosition();
        located_angle = 0;
        located_sector = npc->GetSpawnSector();
    }
    else if (split_obj[0] == "tribe")
    {
        npc->Printf(5,"LocateOp - Tribe");

        if (!npc->GetTribe())
        {
            return OPERATION_FAILED; // Nothing more to do for this op.
        }

        if (split_obj[1] == "home")
        {
            float radius;
            csVector3 pos;
            npc->GetTribe()->GetHome(pos,radius,located_sector);
            
            AddRandomRange(pos, radius, 0.5);
            
            located_pos = pos;
            located_angle = 0;
            located_radius = radius;
            
        }
        else if (split_obj[1] == "memory")
        {
            float located_range=0.0;
            Tribe::Memory * memory;

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
                return OPERATION_FAILED; // Nothing more to do for this op.
            }
            located_pos = memory->pos;
            located_sector = memory->sector;
            located_radius = memory->radius;
            
            AddRandomRange(located_pos, memory->radius, 0.5);
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
            return OPERATION_FAILED; // Nothing more to do for this op.
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
            }
            else if (split_obj[1] == "name")
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
            return OPERATION_FAILED; // Nothing more to do for this op.
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
            return OPERATION_FAILED; // Nothing more to do for this op.
        }
        located_pos = location->pos;
        located_angle = location->rot_angle;
        located_sector = location->GetSector(npcclient->GetEngine());
        located_radius = location->radius;

        AddRandomRange(located_pos, location->radius, 0.5);
        
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
    npc->SetActiveLocateRadius(located_radius);

    npc->Printf(5, "LocateOp - Active location: pos %s rot %.2f wp %s",
                toString(located_pos,located_sector).GetData(),located_angle,
                (located_wp?located_wp->GetName():"(NULL)"));

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool LoopBeginOperation::Load(iDocumentNode *node)
{
    iterations = node->GetAttributeValueAsInt("iterations");
    return true;
}

ScriptOperation *LoopBeginOperation::MakeCopy()
{
    LoopBeginOperation *op = new LoopBeginOperation;
    op->iterations = iterations;
    return op;
}

ScriptOperation::OperationResult LoopBeginOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    return OPERATION_COMPLETED;
}

//---------------------------------------------------------------------------

bool LoopEndOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *LoopEndOperation::MakeCopy()
{
    LoopEndOperation *op = new LoopEndOperation(loopback_op,iterations);
    return op;
}

ScriptOperation::OperationResult LoopEndOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Behavior * behavior = npc->GetCurrentBehavior();

    current++;

    if (current < iterations)
    {
        behavior->SetCurrentStep(loopback_op-1);
        npc->Printf(5, "LoopEnd - Loop %d of %d",current,iterations);
        return OPERATION_COMPLETED;
    }

    current = 0; // Make sure we will loop next time to

    npc->Printf(5, "LoopEnd - Exit %d %d",current,iterations);
    return OPERATION_COMPLETED;
}

//---------------------------------------------------------------------------

bool MeleeOperation::Load(iDocumentNode *node)
{
    seek_range   = node->GetAttributeValueAsFloat("seek_range");
    if (node->GetAttributeValue("melee_range"))
    {
        melee_range  = node->GetAttributeValueAsFloat("melee_range");
        // The server will check for the melee_range and that limit is set to 3.0
        // so to prevent conflicts make sure the melee_range never is geater than 3.0.
        if (melee_range > 3.0)
        {
            melee_range = 3.0;
        }
        // Will never hit with a melee range of 0 so require a pratical minum of 0.5.
        if (melee_range < 0.5)
        {
            melee_range = 0.5;
        }
    }
    else
    {
        // Using the maximum allwed melee range of the server
        // if none has been given by this operatoin.
        melee_range  = 3.0f;
    }
    
    attack_invisible = node->GetAttributeValueAsBool("invisible",false);
    attack_invincible= node->GetAttributeValueAsBool("invincible",false);
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

ScriptOperation::OperationResult MeleeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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

    return OPERATION_NOT_COMPLETED; // This behavior isn't done yet
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
            npc->GetCurrentBehavior()->ApplyNeedDelta(npc, -5);

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
            npc->GetCurrentBehavior()->ApplyNeedDelta(npc, -5); // don't want to fight as badly
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
    
    // Make sure our rotation is still correct
    if(attacked_ent)
    {
    	float rot, npc_rot, new_npc_rot, angle;
        iSector *sector;
        csVector3 pos;
        
        // Get current rot
        psGameObject::GetPosition(npc->GetActor(),pos,npc_rot,sector);
	
        // Get target pos
        psGameObject::GetPosition(attacked_ent,pos,rot,sector);
        
        // Make sure we still face the target
        csVector3 forward;
	
        TurnTo(npc, pos, sector, forward,angle);
        // Needed because TurnTo automatically starts moving.
        csVector3 velvector(0,0,  0 );
        npc->GetLinMove()->SetVelocity(velvector);
        npc->GetLinMove()->SetAngularVelocity( 0 );
        // Check new rot
        psGameObject::GetPosition(npc->GetActor(),pos,new_npc_rot,sector);
	
        // If different broadcast the new rot
        if (npc_rot != new_npc_rot)
        {
            npcclient->GetNetworkMgr()->QueueDRData(npc);
        }
    }
}

void MeleeOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);
}

bool MeleeOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    npcclient->GetNetworkMgr()->QueueAttackCommand(npc->GetActor(),NULL);

    completed = true;

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

ScriptOperation::OperationResult MemorizeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Perception * percept = npc->GetLastPerception();
    if (!percept)
    {
        npc->Printf(5, ">>> Memorize No Perception.");
        return OPERATION_COMPLETED; // Nothing more to do for this op.
    }
    
    npc->Printf(5, ">>> Memorize '%s' '%s'.",percept->GetType(),percept->GetName());

    Tribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return OPERATION_COMPLETED; // Nothing more to do for this op.

    tribe->Memorize(npc, percept );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool MoveOperation::Load(iDocumentNode *node)
{
    LoadVelocity(node);
    LoadCheckMoveOk(node);

    action = node->GetAttributeValue("anim");
    duration = node->GetAttributeValueAsFloat("duration");
    ang_vel = node->GetAttributeValueAsFloat("ang_vel");
    angle = node->GetAttributeValueAsFloat("angle");
    
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

    op->CopyCheckMoveOk(this);
    
    return op;
}

ScriptOperation::OperationResult MoveOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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
    consecCollisions = 0;

    if (!interrupted)
    {
        remaining = duration;
    }

    if(remaining > 0)
    {
        Resume((int)(remaining*1000.0),npc,eventmgr);
    }

    return OPERATION_NOT_COMPLETED;
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
    iSector*  oldSector;
    iSector*  newSector;
    int       ret;

    npc->GetLinMove()->GetLastPosition(oldPos, oldRot, oldSector);

    npc->Printf(10,"Old position: %s",toString(oldPos,oldSector).GetDataSafe());

    ret = npc->GetLinMove()->ExtrapolatePosition(timedelta);

    npc->GetLinMove()->GetLastPosition(newPos, newRot, newSector);
    CheckMoveOk(npc, eventmgr, oldPos, oldSector, newPos, newSector, timedelta);

    npc->Printf(10,"New position: %s",toString(newPos,newSector).GetDataSafe());
}


//---------------------------------------------------------------------------

bool MovePathOperation::Load(iDocumentNode *node)
{
    pathname = node->GetAttributeValue("path");
    if (pathname.IsEmpty())
    {
        Error1("MovePath operation must have a path");
        return false;
    }
    
    anim = node->GetAttributeValue("anim");
    csString dirStr = node->GetAttributeValue("direction");
    if (dirStr.CompareNoCase("REVERSE"))
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

ScriptOperation::OperationResult MovePathOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (!path)
    {
        path = npcclient->FindPath(pathname);
    }

    if (!path)
    {
        Error2("Could not find path '%s'",pathname.GetDataSafe());
        return OPERATION_COMPLETED;  // Nothing more to do for this op.
    }

    anchor = path->CreatePathAnchor();

    // Get Going at the right velocity
    csVector3 velvector(0,0,  -GetVelocity(npc) );
    npc->GetLinMove()->SetVelocity(velvector);
    npc->GetLinMove()->SetAngularVelocity( 0 );

    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return OPERATION_NOT_COMPLETED;
}

void MovePathOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr)
{
    int ret;
    ret = npc->GetLinMove()->ExtrapolatePosition(timedelta);

    if (!anchor->Extrapolate(npcclient->GetWorld(),npcclient->GetEngine(),
                             timedelta*GetVelocity(npc),
                             direction, npc->GetMovable()))
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

MoveToOperation::MoveToOperation()
    :MovementOperation("MoveTo")
{ 
    destPos.Set(0,0,0);
    destSector = NULL;
}

MoveToOperation::MoveToOperation(const MoveToOperation* other )
    :MovementOperation( other ), destPos(other->destPos),
     destSector(other->destSector), action(other->action)
{
    
}


bool MoveToOperation::Load(iDocumentNode *node)
{
    if (!MovementOperation::Load(node))
    {
        return false;
    }

    LoadVelocity(node);
    LoadCheckMoveOk(node);

    destPos.x = node->GetAttributeValueAsFloat("x");
    destPos.y = node->GetAttributeValueAsFloat("y");
    destPos.z = node->GetAttributeValueAsFloat("z");
    const char * sector = node->GetAttributeValue("sector");
    if (sector)
    {
        destSector = npcclient->GetEngine()->FindSector(sector);
    }
    else
    {
        destSector = NULL; // We will later assume sector current sector
    }
    

    action = node->GetAttributeValue("anim");

    return true;
}

ScriptOperation *MoveToOperation::MakeCopy()
{
    MoveToOperation *op = new MoveToOperation(this);

    return op;
}

bool MoveToOperation::GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                     csVector3 &endPos, iSector* &endSector)
{
    endPos = destPos;
    endSector = destSector;

    if (destSector)
    {
        endSector = destSector;
    }
    else
    {
        // Guess that the sector is current sector if no sector is given
        endSector = const_cast<iSector*>(mySector);
    }
    
    
    return true;
}

bool MoveToOperation::UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                        csVector3 &endPos, iSector* &endSector)
{
    return true;
}

//---------------------------------------------------------------------------

NavigateOperation::NavigateOperation()
    : MovementOperation("Navigate"),
      // Operation parameters
      forceEndPosition(false),
      // Instance variables
      endSector(NULL)
{
}

NavigateOperation::NavigateOperation(const NavigateOperation* other)
    : MovementOperation(other),
      // Operation parameters
      action(other->action),
      forceEndPosition(other->forceEndPosition),
      // Instance variables
      endSector(NULL)
{
}

bool NavigateOperation::Load(iDocumentNode *node)
{
    if (!MovementOperation::Load(node))
    {
        return false;
    }

    LoadVelocity(node);
    LoadCheckMoveOk(node);

    action = node->GetAttributeValue("anim");
    forceEndPosition = node->GetAttributeValueAsBool("force", false);
    
    return true;
}

ScriptOperation *NavigateOperation::MakeCopy()
{
    NavigateOperation *op = new NavigateOperation(this);

    return op;
}

bool NavigateOperation::GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                       csVector3 &endPos, iSector* &endSector)
{
    float dummyRot;

    npc->GetActiveLocate(endPos,endSector,dummyRot);

    npc->Printf(5, "Located %s at %1.2f m/sec.",toString(endPos,endSector).GetData(),
                GetVelocity(npc) );

    return true;
}

bool NavigateOperation::UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                          csVector3 &endPos, iSector* &endSector)
{
    return true;
}


bool NavigateOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
    if (forceEndPosition)
    {
        // Set position to where it is supposed to go
        npc->GetLinMove()->SetPosition(endPos,endAngle,endSector);
    }

    return MovementOperation::CompleteOperation(npc, eventmgr);
}

//---------------------------------------------------------------------------

bool PerceptOperation::Load(iDocumentNode *node)
{
    perception = node->GetAttributeValue("event");
    if (perception.IsEmpty())
    {
        Error1("Percept operation need an event attribute");
        return false;
    }
    
    maxRange = node->GetAttributeValueAsFloat("range");

    csString targetStr = node->GetAttributeValue("target");
    if (targetStr.IsEmpty() || targetStr.CompareNoCase("self"))
    {
        target = SELF;
        maxRange = 0.0; // Turn off range limit on self.
    }
    else if (targetStr.CompareNoCase("all"))
    {
        target = ALL;
    }
    else if (targetStr.CompareNoCase("tribe"))
    {
        target = TRIBE;
    }
    else if (targetStr.CompareNoCase("target"))
    {
        target = TARGET;
    }
    else
    {
        Error2("Unkown target type of '%s' for perception operation",
               targetStr.GetDataSafe());
        return false;
    }

    return true;
}

ScriptOperation *PerceptOperation::MakeCopy()
{
    PerceptOperation *op = new PerceptOperation;

    op->perception = perception;
    op->target     = target;
    op->maxRange   = maxRange;

    return op;
}

ScriptOperation::OperationResult PerceptOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Perception pcpt(perception);

    csVector3 basePos;
    iSector*  baseSector = NULL;
    float     baseYRot;

    if (maxRange >= 0.0 && npc->GetActor())
    {
        // Only get position if range based perception.
        psGameObject::GetPosition(npc->GetActor(), basePos, baseYRot, baseSector);
    }
    else
    {
        if (maxRange >= 0.0)
        {
            npc->Printf(1,"Perception not sent based on range since no known position");
            return OPERATION_COMPLETED; // Nothing more to do for this op.
        }
        
    }
    
    if (target == SELF)
    {
        // No point checking range for self, so just send the event.
        npc->TriggerEvent(&pcpt);
    }
    else if (target == TRIBE)
    {
        Tribe* tribe = npc->GetTribe();
        if (!tribe)
        {
            npc->Printf("No tribe to percept");
            return OPERATION_COMPLETED; // Nothing more to do for this op.
        }
        
        tribe->TriggerEvent(&pcpt, maxRange, &basePos, baseSector);
    }
    else if (target == ALL)
    {
        npcclient->TriggerEvent(&pcpt, maxRange, &basePos, baseSector);
    }
    else if (target == TARGET)
    {
        if (!npc->GetTarget())
        {
            npc->Printf(1,"Failed to percept since no target");
            return OPERATION_COMPLETED; // Nothing more to do for this op.
        }
        
        NPC *targetNPC = npc->GetTarget()->GetNPC();
        if (targetNPC)
        {
            targetNPC->TriggerEvent(&pcpt, maxRange, &basePos, baseSector);
        }
        else
        {
            npc->Printf(1,"Failed to percept since target isn't a NPC");
        }
    }
    else
    {
        Error1("Unkown target type for Percept operation");
    }

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool PickupOperation::Load(iDocumentNode *node)
{
    object = node->GetAttributeValue("obj");
    if (object.IsEmpty())
    {
        object = "perception";
    }
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

ScriptOperation::OperationResult PickupOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    gemNPCObject *item = NULL;

    if (object == "perception")
    {
        if (!npc->GetLastPerception())
        {
            npc->Printf(5,"Pickup operation. No perception for NPC");
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
        }
        
        if (!(item = npc->GetLastPerception()->GetTarget()))
        {
            npc->Printf(5,"Pickup operation. No target in perception for NPC.");
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
        }
        
        if (!item->IsPickable())
        {
            npc->Printf(1,"Pickup operation failed since object %s isn't pickable",
                        item->GetName());
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
        }
        
        
    }
    else
    {
        // TODO: Insert code to find the nearest item
        //       with name given by object.
        return OPERATION_FAILED;  // Nothing more to do for this op.
    }

    npc->Printf(5, "   Who: %s What: %s Count: %d",npc->GetName(),item->GetName(), count);

    npcclient->GetNetworkMgr()->QueuePickupCommand(npc->GetActor(), item, count);

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
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

ScriptOperation::OperationResult ReproduceOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if(!npc->GetTarget())
        return OPERATION_COMPLETED;  // Nothing more to do for this op.

    NPC * friendNPC = npc->GetTarget()->GetNPC();
    if(friendNPC)
    {
        npc->Printf(5, "Reproduce");
        npcclient->GetNetworkMgr()->QueueSpawnCommand(friendNPC->GetActor(), npc->GetActor(), 0);
    }

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
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

ScriptOperation::OperationResult ResurrectOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Tribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return OPERATION_COMPLETED;  // Nothing more to do for this op.

    csVector3 where;
    float     radius;
    iSector*  sector;
    float     rot = psGetRandom() * TWO_PI;

    tribe->GetHome(where,radius,sector);

    float x, z, xDist, zDist;
    do {
        // Pick random point in circumscribed rectangle.
        x = psGetRandom() * (radius*2.0);
        z = psGetRandom() * (radius*2.0);
        xDist = radius - x;
        zDist = radius - z;
        // Keep looping until the point is inside a circle.
    } while(xDist * xDist + zDist * zDist > radius * radius);
    
    where.x += x - radius;
    where.z += z - radius;

    npcclient->GetNetworkMgr()->QueueResurrectCommand(where, rot, sector, npc->GetPID());

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool RewardOperation::Load(iDocumentNode *node)
{
    // Load attributes
    resource = node->GetAttributeValue("resource");
    count = node->GetAttributeValueAsInt("count");

    // Resource is mandatory for this operation
    if (resource.IsEmpty())
    {
        return false;
    }

    // Default count to 1 if not pressent
    if (csString(node->GetAttributeValue("count")).IsEmpty())
    {
        count = 1;
    }
    
    return true;
}

ScriptOperation *RewardOperation::MakeCopy()
{
    RewardOperation *op = new RewardOperation;
    op->resource = resource;
    op->count = count;
    return op;
}

ScriptOperation::OperationResult RewardOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    csString res = resource;
    
    if (resource == "tribe:wealth")
    {
        if (npc->GetTribe())
        {
            res = npc->GetTribe()->GetNeededResource();
        }
    }

    if (npc->GetTribe())
    {
        npc->GetTribe()->AddResource(res,count);
    }

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
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
    else if (type == "tribe_home")
    {
        op_type = ROT_TRIBE_HOME;
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
        Error2("Rotate Op type '%s' must be 'inregion', 'random', 'absolute', 'relative', "
               "'target', 'tribe_home' or 'locatedest' right now.",type.GetDataSafe());
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

ScriptOperation::OperationResult RotateOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
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

            npc->Printf(5, "Region found in between %.2f and %.2f",min_angle*180.0f/PI,max_angle*180.0f/PI);

            if (max_angle-min_angle  > PI )
            {
                float temp=max_angle;  
                max_angle=min_angle+TWO_PI;  
                min_angle=temp;
            }

            npc->Printf(5, "Region found in between %.2f and %.2f",min_angle*180.0f/PI,max_angle*180.0f/PI);

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
    else if (op_type == ROT_TRIBE_HOME)
    {
        Tribe * tribe = npc->GetTribe();
        
        if (tribe)
        {
            csVector3 tribePos;
            float tribeRadius;
            iSector* tribeSector;

            tribe->GetHome(tribePos,tribeRadius,tribeSector);
            
            target_angle = psGameObject::CalculateIncidentAngle(pos,tribePos);
            target_angle += psGetRandom() * (max_range-min_range) + min_range;

            angle_delta = target_angle-rot;
        }
        else
        {
            angle_delta = PI; // Just turn around if now tribe.
        }
        
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
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
        }
        
        target_angle = psGameObject::CalculateIncidentAngle(pos,dest);

        angle_delta = target_angle-rot;

        // If the angle is close enough don't worry about it and just go to next command.
        if (fabs(angle_delta) < TWO_PI/60.0)
        {
            npc->Printf(5, "Rotation at destination angle. Ending rotation.");
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
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
        return OPERATION_COMPLETED;  // Nothing more to do for this op.
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

    return OPERATION_NOT_COMPLETED;
}

void RotateOperation::Advance(float timedelta, NPC *npc, EventManager *eventmgr) 
{
    int ret;
    ret = npc->GetLinMove()->ExtrapolatePosition(timedelta);
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

bool SequenceOperation::Load(iDocumentNode *node)
{
    sequenceName = node->GetAttributeValue("name");
    if (name.IsEmpty())
    {
        Error1("Sequence operation must have a name attribute");
        return false;
    }
    
    csString cmdStr = node->GetAttributeValue("cmd");
    
    if (strcasecmp(cmdStr,"start") == 0)
    {
        cmd = START;
    }
    else if (strcasecmp(cmdStr,"stop") == 0)
    {
        cmd = STOP;
    }
    else if (strcasecmp(cmdStr,"loop") == 0)
    {
        cmd = LOOP;
    }
    else
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
    op->sequenceName = sequenceName;
    op->cmd = cmd;
    op->count = count;
    return op;
}

ScriptOperation::OperationResult SequenceOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{

    npcclient->GetNetworkMgr()->QueueSequenceCommand(sequenceName, cmd, count );
    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool ShareMemoriesOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *ShareMemoriesOperation::MakeCopy()
{
    ShareMemoriesOperation *op = new ShareMemoriesOperation;
    return op;
}

ScriptOperation::OperationResult ShareMemoriesOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{

    npc->Printf("ShareMemories with tribe.");

    Tribe * tribe = npc->GetTribe();
    if ( !tribe ) return OPERATION_COMPLETED; // Nothing more to do for this op.

    tribe->ShareMemories( npc );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool SitOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *SitOperation::MakeCopy()
{
    SitOperation *op = new SitOperation(sit);
    return op;
}

ScriptOperation::OperationResult SitOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "   Who: %s %s", npc->GetName(), sit?"sit":"stand" );

    npcclient->GetNetworkMgr()->QueueSitCommand(npc->GetActor(), npc->GetTarget(), sit);

    return OPERATION_COMPLETED;
}

//---------------------------------------------------------------------------

bool TalkOperation::Load(iDocumentNode *node)
{
    talkText = node->GetAttributeValue("text");
    target   = node->GetAttributeValueAsBool("target",true);
    command  = node->GetAttributeValue("command");
    talkPublic = node->GetAttributeValueAsBool("public",true);
    csString type = node->GetAttributeValue("type");
    if (type.IsEmpty() || type.CompareNoCase("say"))
    {
        talkType = psNPCCommandsMessage::TALK_SAY;
    }
    else if (type.CompareNoCase("me"))
    {
        talkType = psNPCCommandsMessage::TALK_ME;
    }
    else if (type.CompareNoCase("my"))
    {
        talkType = psNPCCommandsMessage::TALK_MY;
    }
    else if (type.CompareNoCase("narrate"))
    {
        talkType = psNPCCommandsMessage::TALK_NARRATE;
    }
    else
    {
        Error2("Type of '%s' is unkown for talk",type.GetDataSafe());
        return false;
    }

    return true;
}

ScriptOperation *TalkOperation::MakeCopy()
{
    TalkOperation *op = new TalkOperation;
    op->talkText   = talkText;
    op->talkType   = talkType;
    op->talkPublic = talkPublic;
    op->target     = target;
    op->command    = command;
    return op;
}

ScriptOperation::OperationResult TalkOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    gemNPCActor* talkTarget = NULL;

    // Check if this talk is to a target
    if (target)
    {
        talkTarget = dynamic_cast<gemNPCActor*>( npc->GetTarget() );
        
        if (!talkTarget)
        {
            npc->Printf(1,"No target for talk operation.");
            return OPERATION_COMPLETED;  // Nothing more to do for this op.
        }
    }
            
    // Queue the talk to the server
    npcclient->GetNetworkMgr()->QueueTalkCommand(npc->GetActor(), talkTarget,
                                                 talkType, talkPublic, talkText);

    if (talkTarget && !command.IsEmpty())
    {
        NPC* friendNPC = talkTarget->GetNPC(); // Check if we have a NPC for this
        if(friendNPC)
        {
            Perception perception("friend:" + command);
            friendNPC->TriggerEvent(&perception);
        }
    }

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool TeleportOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *TeleportOperation::MakeCopy()
{
    TeleportOperation *op = new TeleportOperation;
    return op;
}

ScriptOperation::OperationResult TeleportOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    gemNPCActor* actor = npc->GetActor();
    
    if (!actor)
    {
        npc->Printf(1,"No actor for telport operation.");
        return OPERATION_COMPLETED;  // Nothing more to do for this op.
    }
    
    csVector3 pos;
    iSector*  sector;
    float     rot;

    npc->GetActiveLocate(pos, sector, rot);
    npc->Printf(5, "Teleport to %s",toString(pos,sector).GetDataSafe());

    psGameObject::SetPosition(npc->GetActor(), pos, sector);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return OPERATION_COMPLETED;  // Nothing more to do for this op.
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

ScriptOperation::OperationResult TransferOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    csString transferItem = item;

    csArray<csString> splitItem = psSplit(transferItem,':');
    if (splitItem[0] == "tribe")
    {
        if (!npc->GetTribe())
        {
            npc->Printf(5, "No tribe");
            return OPERATION_COMPLETED; // Nothing more to do for this op. 
        }
        
        if (splitItem[1] == "wealth")
        {
            transferItem = npc->GetTribe()->GetNeededResource();
        }
        else
        {
            Error2("Transfer operation for tribe with unknown sub type %s",splitItem[1].GetDataSafe())
            return OPERATION_COMPLETED; // Nothing more to do for this op.
        }
        
    }
    
    npcclient->GetNetworkMgr()->QueueTransferCommand(npc->GetActor(), transferItem, count, target );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}
//---------------------------------------------------------------------------

bool TribeHomeOperation::Load(iDocumentNode *node)
{
    return true;
}

ScriptOperation *TribeHomeOperation::MakeCopy()
{
    TribeHomeOperation *op = new TribeHomeOperation;
    return op;
}

ScriptOperation::OperationResult TribeHomeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    Tribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return OPERATION_COMPLETED; // Nothing more to do for this op.

    csVector3 pos;
    iSector*  sector;
    float     rot;
    float     radius = 10.0;

    npc->GetActiveLocate(pos,sector,rot);
    radius = npc->GetActiveLocateRadius();
    
    npc->Printf("Moves tribe home to: %s",toString(pos,sector).GetData());

    tribe->SetHome(pos,radius,sector);

    Perception move("tribe:home moved");
    tribe->TriggerEvent(&move);

    return OPERATION_COMPLETED; // Nothing more to do for this op.
}

//---------------------------------------------------------------------------

bool TribeTypeOperation::Load(iDocumentNode *node)
{
    tribeType = node->GetAttributeValueAsInt("type");
    return true;
}

ScriptOperation *TribeTypeOperation::MakeCopy()
{
    TribeTypeOperation *op = new TribeTypeOperation;
    op->tribeType = tribeType;
    return op;
}

ScriptOperation::OperationResult TribeTypeOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    // Check if NPC is part of a tribe
    Tribe * tribe = npc->GetTribe();
    
    if ( !tribe ) return OPERATION_COMPLETED; // Nothing more to do for this op.

  
    npc->SetTribeMemberType(tribeType);  
    npc->Printf("Change tribe type to : %d", tribeType );

    return OPERATION_COMPLETED; // Nothing more to do for this op.
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

ScriptOperation::OperationResult VisibleOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npcclient->GetNetworkMgr()->QueueVisibilityCommand(npc->GetActor(), true);
    return OPERATION_COMPLETED;  // Nothing more to do for this op.
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

ScriptOperation::OperationResult WaitOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (!interrupted)
    {
        remaining = duration;
    }

    // SetAction animation for the mesh, so it looks right
    SetAnimation(npc, action);

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return OPERATION_NOT_COMPLETED;
}

void WaitOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    remaining -= timedelta;
    if(remaining <= 0)
    	npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior() );
    npc->Printf(10, "waiting... %.2f",remaining);
}

//---------------------------------------------------------------------------

WanderOperation::WanderOperation()
    : ScriptOperation("Wander"),
      // Instance states
      // Instance states
      wanderRouteFilter(this),
      currentEdge(NULL),
      currentPathPointIterator(NULL),
      currentDistance(0.0f)
      // Operation parameters
      // Initialized in the load function
{ 
}

WanderOperation::WanderOperation(const WanderOperation* other)
    : ScriptOperation(other),
      // Instance states
      wanderRouteFilter(this),
      currentEdge(NULL),
      currentPathPointIterator(NULL),
      currentDistance(0.0f),
      // Operation parameters
      action(other->action),
      random(other->random),
      undergroundValid(other->undergroundValid),
      underground(other->underground),
      underwaterValid(other->underwaterValid),
      underwater(other->underwater),
      privValid(other->privValid),
      priv(other->priv),
      pubValid(other->pubValid),
      pub(other->pub),
      cityValid(other->cityValid),
      city(other->city),
      indoorValid(other->indoorValid),
      indoor(other->indoor),
      pathValid(other->pathValid),
      path(other->path),
      roadValid(other->roadValid),
      road(other->road),
      groundValid(other->groundValid),
      ground(other->ground)
{ 
}


WanderOperation::~WanderOperation()
{
}


bool WanderOperation::WanderRouteFilter::Filter( const Waypoint * waypoint ) const
{
    return ( !((!parent->undergroundValid || waypoint->underground == parent->underground) &&
               (!parent->underwaterValid || waypoint->underwater == parent->underwater) &&
               (!parent->privValid || waypoint->priv == parent->priv) &&
               (!parent->pubValid || waypoint->pub == parent->pub) &&
               (!parent->cityValid || waypoint->city == parent->city) &&
               (!parent->indoorValid || waypoint->indoor == parent->indoor) &&
               (!parent->pathValid || waypoint->path == parent->path) &&
               (!parent->roadValid || waypoint->road == parent->road) &&
               (!parent->groundValid || waypoint->ground == parent->ground)) );
}

bool WanderOperation::StartMoveTo(NPC *npc, psPathPoint* point)
{
    float dummyAngle;

    csVector3 destPos = point->GetPosition();
    destPos += currentPointOffset;

    ScriptOperation::StartMoveTo(npc, NULL, destPos,
                                 point->GetSector(npcclient->GetEngine()),
                                 GetVelocity(npc), action.GetDataSafe(), false, dummyAngle);

    return true;
}

bool WanderOperation::MoveTo(NPC *npc, psPathPoint* point)
{
    StopMovement( npc );

    csVector3 destPos = point->GetPosition();
    destPos += currentPointOffset;

    psGameObject::SetPosition(npc->GetActor(),destPos,
                              point->GetSector(npcclient->GetEngine()));

    //now persist
    npcclient->GetNetworkMgr()->QueueDRData(npc);

    return true;
}

ScriptOperation::OperationResult WanderOperation::CalculateEdgeList(NPC *npc)
{
    // Get the current waypoint
    Waypoint *start = npcclient->FindNearestWaypoint(npc->GetActor(), 5.0, NULL);
    if (!start)
    {
        npc->Printf(5,"Failed, not at a waypoint nearby.");
        return OPERATION_FAILED;
    }


    // Cleare old values
    if (currentPathPointIterator)
    {
        delete currentPathPointIterator;
        currentPathPointIterator = NULL;
    }
    EdgeListClear();
    currentPathPoint = NULL;
    currentEdge = NULL;

    if (random)
    {
        // We don't calculate a list for random movement, only currentEdge.
        currentEdge = start->GetRandomEdge( &wanderRouteFilter );
        if (!currentEdge)
        {
            npc->Printf(5, "Failed, to find route from waypoint %s.",start->GetName());
            return OPERATION_FAILED;
        }
    }
    else
    {
        Waypoint* end;

        // Get the end waypoint
        npc->GetActiveLocate(end);
        if (!end)
        {
            npc->Printf(5,"Failed to find active locate waypoint");
            return OPERATION_FAILED;
        }
        
        if (start && end)
        {
            npc->Printf(6, "Start WP: %30s at %s",start->GetName(),toString(start->loc.pos,start->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            npc->Printf(6, "End WP  : %30s at %s",end->GetName(),toString(end->loc.pos,end->loc.GetSector(npcclient->GetEngine())).GetDataSafe());
            
            if (start == end)
            {
                return OPERATION_COMPLETED;
            }
            else
            {
                csList<Waypoint*> wps;
                
                edgeList = npcclient->FindEdgeRoute(start, end, &wanderRouteFilter );
                if (edgeList.IsEmpty())
                {
                    npc->Printf(5, "Can't find route...");
                    return OPERATION_FAILED;
                }
            }
            
            // Start print edge list
            if (npc->IsDebugging(5))
            {
                psString wp_str;
                bool first = true;
                csList<Edge*>::Iterator iter(edgeList);
                while (iter.HasNext())
                {
                    Edge* edge = iter.Next();
                    if (first)
                    {
                        wp_str.Append(edge->GetStartWaypoint()->GetName());
                        first = false;
                    }
                    wp_str.Append(" -> ");
                    wp_str.Append(edge->GetEndWaypoint()->GetName());
                }
                npc->Printf(5, "Waypoint list: %s",wp_str.GetDataSafe());
            }
            // End print waypoint list

            edgeIterator = csList<Edge*>::Iterator(edgeList);
            currentEdge = edgeIterator.Next();

            if (currentEdge)
            {
                npc->Printf(5,"Running from %s to %s",currentEdge->GetStartWaypoint()->GetName(),
                            currentEdge->GetEndWaypoint()->GetName());
            }

        }
    }
    
    SetPathPointIterator(currentEdge->GetIterator());

    return OPERATION_NOT_COMPLETED;
}

Edge* WanderOperation::GetNextEdge(NPC* npc)
{
    if (random)
    {
        currentEdge = currentEdge->GetRandomEdge( &wanderRouteFilter );
    }
    else
    {
        if (edgeIterator.HasNext())
        {
            currentEdge = edgeIterator.Next();
        }
        else
        {
            currentEdge = NULL;
        }
    }

    if (currentEdge)
    {
        SetPathPointIterator(currentEdge->GetIterator());

        npc->Printf(5,"Running from %s to %s",currentEdge->GetStartWaypoint()->GetName(),
                    currentEdge->GetEndWaypoint()->GetName());
    }

    return currentEdge;
}

void WanderOperation::SetPathPointIterator(Edge::Iterator* iterator)
{
    if (currentPathPointIterator)
    {
        delete currentPathPointIterator;
    }

    currentPathPointIterator = iterator;

    if (currentPathPointIterator)
    {
        currentPathPoint = currentPathPointIterator->Next(); // Skip first point
    }
}

psPathPoint* WanderOperation::GetNextPathPoint(NPC* npc, bool &teleport)
{
    // We are at the end of the path, get next edge.
    if (!currentPathPointIterator || !currentPathPointIterator->HasNext())
    {
        npc->Printf(5,"No more path points, changing edge...");

        Edge* edge = GetNextEdge(npc); // Indirect sets the currentEdge
        if (!edge)
        {
            currentPathPoint = NULL;
            SetPathPointIterator( NULL );

            return NULL; // Found no edge to wander, so no more path points either.
        }
    }

    // Check if we should teleport this edge
    if (currentEdge->IsTeleport())
    {
        teleport = true;
    }
    
    if (currentPathPointIterator->HasNext())
    {
        currentPathPoint = currentPathPointIterator->Next();

	currentPointOffset = csVector3(0.0,0.0,0.0);
	AddRandomRange(currentPointOffset,  currentPathPoint->GetRadius(), 0.5);

        npc->Printf(6,"Next point %s with offset %s",
                    toString(currentPathPoint->GetPosition(),
                             currentPathPoint->GetSector(npcclient->GetEngine())).GetData(),
		    toString(currentPointOffset).GetData());


    }
    else
    {
        currentPathPoint = NULL;
    }


    return currentPathPoint; // Return the found path point
}

psPathPoint* WanderOperation::GetCurrentPathPoint(NPC* npc)
{
    return currentPathPoint;
}

float WanderOperation::DistanceToDestPoint( NPC* npc, const csVector3& destPos, const iSector* destSector )
{
    csVector3    myPos;
    float        myRot;
    iSector*     mySector;

    npc->GetLinMove()->GetLastPosition(myPos, myRot, mySector);

    return  npcclient->GetWorld()->Distance(myPos, mySector, destPos, destSector);
}


ScriptOperation::OperationResult WanderOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    if (interrupted && AtInterruptedPosition(npc))
    {
        // Restart current behavior
        StartMoveTo(npc, GetCurrentPathPoint(npc));

        return OPERATION_NOT_COMPLETED; // This behavior isn't done yet
    }

    OperationResult result = CalculateEdgeList(npc);
    if (result != OPERATION_NOT_COMPLETED)
    {
        return result;
    }

    bool teleport = false;
    psPathPoint* destPoint = GetNextPathPoint(npc, teleport);

    if (!destPoint)
    {
        npc->Printf(5,">>>WanderOp NO pathpoints, %s cannot move.",npc->GetName());
        return OPERATION_FAILED;
    }

    csVector3 destPos = destPoint->GetPosition();
    destPos += currentPointOffset;

    // Store current distance to the next local point. Used to detect when we pass the waypoint.
    currentDistance = DistanceToDestPoint(npc, destPos, destPoint->GetSector(npcclient->GetEngine()));

    if (teleport)
    {
        MoveTo(npc, destPoint);
    }
    else if (!StartMoveTo(npc, destPoint))
    {
        npc->Printf(5,">>>Failed to start move to.");
        return OPERATION_FAILED;
    }

    return OPERATION_NOT_COMPLETED;
}

void WanderOperation::Advance(float timedelta,NPC *npc,EventManager *eventmgr)
{
    csVector3    myPos;
    float        myRot;
    iSector*     mySector;
    csVector3    forward;
    float        angle;

    npc->GetLinMove()->GetLastPosition(myPos, myRot, mySector);

    psPathPoint* destPoint = GetCurrentPathPoint(npc);
    csVector3 destPos = destPoint->GetPosition();
    destPos += currentPointOffset;

    float distance = npcclient->GetWorld()->Distance(myPos, mySector,
                                                     destPos, destPoint->GetSector(npcclient->GetEngine()));

    npc->Printf(6,"Current localDest %s at %s dist %.2f",
                destPoint->GetName().GetData(),
                toString(destPos, destPoint->GetSector(npcclient->GetEngine())).GetData(),
                distance);
                

    if (distance >= INFINITY_DISTANCE)
    {
        npc->Printf(5, "No connection found..");
        npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
        return;
    }
    else if (distance <= 0.5f || distance > currentDistance )
    {
        if (distance > currentDistance )
        {
            npc->Printf(6, "We passed localDest...");
        }
        else
        {
            npc->Printf(6, "We are at localDest...");
        }

        bool teleport = false;

        // We reached the path point, check for next
        destPoint = GetNextPathPoint(npc, teleport);

        if (destPoint)
        {
            destPos = destPoint->GetPosition();
            destPos += currentPointOffset;

            currentDistance =  npcclient->GetWorld()->Distance(myPos, mySector,
                                                               destPos, destPoint->GetSector(npcclient->GetEngine()));

            npc->Printf(6,"New localDest %s at range %.2f",
                        toString(destPos,
                                 destPoint->GetSector(npcclient->GetEngine())).GetData(),
                        currentDistance);

            if (teleport)
            {

                MoveTo(npc, destPoint);
            }
            else if (!StartMoveTo(npc, destPoint))
            {
                npc->Printf(5, "We are done..Failed to start to next localDest.");
                npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
                return;
            }
        }
        else
        {
            npc->Printf(5, "We are done..");
            npc->ResumeScript(npc->GetBrain()->GetCurrentBehavior());
            return;
        }
    }
    else
    {
        TurnTo(npc, destPos, destPoint->GetSector(npcclient->GetEngine()),
               forward, angle);

	// Store current distance, next advance we can check if we still approch the waypoint or not.
	currentDistance = distance;
    }

    // Limit time extrapolation so we arrive near the correct place.
    float close = GetVelocity(npc)*timedelta;
    if(distance <= close)
    {
    	timedelta = distance / GetVelocity(npc);
    }

    npc->Printf(8, "advance: pos=(%f.2,%f.2,%f.2) rot=%.2f %s localDest=%s dist=%f", 
                myPos.x,myPos.y,myPos.z, myRot, mySector->QueryObject()->GetName(),
                destPoint->GetPosition().Description().GetData(),distance);

    {
        int ret;
        ScopedTimer st(250, "Movement extrapolate %.2f time for %s", timedelta, ShowID(npc->GetActor()->GetEID()));
        ret = npc->GetLinMove()->ExtrapolatePosition(timedelta);
        if (ret != PS_MOVE_SUCCEED)
        {
            npc->Printf("Extrapolated didn't success: %d",ret);
        }
    }

    {
        bool on_ground;
        float speed,ang_vel;
        csVector3 bodyVel,worldVel,myNewPos;
        iSector* myNewSector;

        npc->GetLinMove()->GetDRData(on_ground,speed,myNewPos,myRot,myNewSector,bodyVel,worldVel,ang_vel);
        npc->Printf(8,"World position bodyVel=%s worldVel=%s",
                       toString(bodyVel).GetDataSafe(),toString(worldVel).GetDataSafe());

        CheckMoveOk(npc, eventmgr, myPos, mySector, myNewPos, myNewSector, timedelta);
    }
}

void WanderOperation::InterruptOperation(NPC *npc,EventManager *eventmgr)
{
    ScriptOperation::InterruptOperation(npc,eventmgr);

    StopMovement(npc);
}

bool WanderOperation::CompleteOperation(NPC *npc,EventManager *eventmgr)
{
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
    path = LoadAttributeBool(node,"path",false,&pathValid);
    road = LoadAttributeBool(node,"road",false,&roadValid);
    ground = LoadAttributeBool(node,"ground",false,&groundValid);

    return true;
}

ScriptOperation *WanderOperation::MakeCopy()
{
    WanderOperation *op  = new WanderOperation(this);
    
    return op;
}

//---------------------------------------------------------------------------

const char * WatchOperation::typeStr[]={"nearest_actor","nearest_npc","nearest_player","owner","target"};

bool WatchOperation::Load(iDocumentNode *node)
{
    watchRange  = node->GetAttributeValueAsFloat("range");
    watchInvisible = node->GetAttributeValueAsBool("invisible",false);
    watchInvincible= node->GetAttributeValueAsBool("invincible",false);

    csString typestr = node->GetAttributeValue("type");
    if (typestr == "nearest_actor" )
    {
        type = NEAREST_ACTOR;
    }
    else if (typestr == "nearest_npc")
    {
        type = NEAREST_NPC;
    }
    else if (typestr == "nearest_player")
    {
        type = NEAREST_PLAYER;
    }
    else if (typestr== "owner")
    {
        type = OWNER;
    }
    else if (typestr == "target")
    {
        type = TARGET;
    }
    else
    {
        Error2("Watch operation can't understand a type of %s",typestr.GetDataSafe());
        return false;
    }

    if (node->GetAttributeValue("search_range"))
    {
        searchRange = node->GetAttributeValueAsFloat("search_range");
    }
    else
    {
        searchRange = 2.0f;
    }    

    watchedEnt = NULL;

    return true;
}

ScriptOperation *WatchOperation::MakeCopy()
{
    WatchOperation *op = new WatchOperation;
    op->watchRange      = watchRange;
    op->type            = type;
    op->searchRange     = searchRange; // Used for watch of type NEAREST_* 
    op->watchInvisible  = watchInvisible;
    op->watchInvincible = watchInvincible;
    watchedEnt = NULL;
    return op;
}

ScriptOperation::OperationResult WatchOperation::Run(NPC *npc, EventManager *eventmgr, bool interrupted)
{
    npc->Printf(5, "WatchOperation starting with watch range (%.2f) will watch:%s%s.",
                watchRange,(watchInvisible?" Invisible":" Visible"),
                (watchInvincible?" Invincible":""));

    iSector*  targetSector;
    csVector3 targetPos;
    float     targetRange;

    EID targetEID = EID(0);

    switch (type)
    {
        case NEAREST_ACTOR:
            watchedEnt = npc->GetNearestActor(searchRange, targetPos, targetSector, targetRange);
            if (watchedEnt)
            {
                targetEID = watchedEnt->GetEID();
                npc->Printf(5, "Targeting nearest actor (%s) at (%1.2f,%1.2f,%1.2f) range %.2f for watch ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z,targetRange);
            }
            break;
        case NEAREST_PLAYER:
            watchedEnt = npc->GetNearestActor(searchRange, targetPos, targetSector, targetRange);
            if (watchedEnt)
            {
                targetEID = watchedEnt->GetEID();
                npc->Printf(5, "Targeting nearest player (%s) at (%1.2f,%1.2f,%1.2f) range %.2f for watch ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z,targetRange);
            }
            break;
        case NEAREST_NPC:
            watchedEnt = npc->GetNearestNPC(searchRange, targetPos, targetSector, targetRange);
            if (watchedEnt)
            {
                targetEID = watchedEnt->GetEID();
                npc->Printf(5, "Targeting nearest NPC (%s) at (%1.2f,%1.2f,%1.2f) range %.2f for watch ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z,targetRange);
            }
            break;
        case OWNER:
            watchedEnt = npc->GetOwner();
            if (watchedEnt)
            {
                targetEID = watchedEnt->GetEID();
                psGameObject::GetPosition(watchedEnt, targetPos, targetSector);
                npc->Printf(5, "Targeting owner (%s) at (%1.2f,%1.2f,%1.2f) for watch ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z );

            }
            break;
        case TARGET:
            watchedEnt = npc->GetTarget();
            if (watchedEnt)
            {
                targetEID = watchedEnt->GetEID();
                psGameObject::GetPosition(watchedEnt, targetPos, targetSector);
                npc->Printf(5, "Targeting current target (%s) at (%1.2f,%1.2f,%1.2f) for chase ...",
                            watchedEnt->GetName(),targetPos.x,targetPos.y,targetPos.z );
            }
            break;
    }

    if (!watchedEnt)
    {
        npc->Printf(5,"No entity to watch");
        return OPERATION_COMPLETED; // Nothing to do for this behaviour.
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
        return OPERATION_COMPLETED; // Nothing to do for this behavior.
    }
    */
    
    return OPERATION_NOT_COMPLETED; // This behavior isn't done yet
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

