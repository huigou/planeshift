/*
* npcoperations.h
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
#ifndef __NPCOPERATIONS_H__
#define __NPCOPERATIONS_H__

#include <psstdint.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>
#include <csutil/csstring.h>

struct iDocumentNode;
struct iSector;

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psconst.h"
#include "util/pspath.h"

//=============================================================================
// Project Includes
//=============================================================================
#include "walkpoly.h"

class psResumeScriptEvent;
class EventManager;
class NPC;
class gemNPCObject;
class gemNPCActor;
class MoveOperation;
class Waypoint;

/**
* This is the base class for all operations in action scripts.
* We will use these to set animation actions, move and rotate
* the mob, attack and so forth.  These should be mostly visual
* or physical things, except for the ability to create new
* perceptions, which can kick off other actions.
*/
class ScriptOperation
{
    friend class psResumeScriptEvent;

    enum VelSource {
        VEL_DEFAULT,
        VEL_USER,
        VEL_WALK,
        VEL_RUN
    };
    
protected:
    VelSource            velSource;
    float                vel;  // Shared by many functions
    float                ang_vel;  // Shared by many functions
    
    bool                 completed; /// This flag is set to false by Run(), and set to true by CompleteOperation(), in case of multiple Complete's being called.
    psResumeScriptEvent *resumeScriptEvent;
    csString             name;
    csVector3            interrupted_position;
    iSector             *interrupted_sector;
    float                interrupted_angle;


    /////////////////////////////////////////////////////////
    // Start Check Move OK parameters

    // Configuration paramters. Set by using LoadCheckMoveOk    
    csString             collision;       ///< Perception names to use for collision detected by CheckMoveOk
    csString             outOfBounds;     ///< Perception names to use for out of bounds detected by CheckMoveOk
    csString             inBounds;        ///< Perception names to use for in bounds detected by CheckMoveOk
    bool                 checkTribeHome;  ///< Set to true if the tribe home should be checked by CheckMoveOk

    // Instance temp variables. These dosn't need to be copied.
    int                  consecCollisions; ///< Shared by move functions. Used by CheckMoveOk to detect collisions
        
    // End Check Move OK parameters
    /////////////////////////////////////////////////////////
    
    void Resume(csTicks delay, NPC *npc, EventManager *eventmgr);
    void ResumeTrigger(psResumeScriptEvent * event);
    void StopResume();

    /// This function is used by MoveTo AND Navigate operations
    int StartMoveTo(NPC *npc,EventManager *eventmgr,csVector3& dest, iSector* sector, float vel,const char *action, bool autoresume, float &angle);
    
    /// This function is used by MoveTo AND Navigate operations
    int StartTurnTo(NPC *npc,EventManager *eventmgr,float turn_end_angle, float ang_vel,const char *action, bool autoresume=true);

    void TurnTo(NPC *npc,csVector3& dest, iSector* destsect,csVector3& forward, float &angle);

    /// Utility function used by many operation to stop movement of an NPC.
    static void StopMovement(NPC *npc);

public:

    ScriptOperation(const char* sciptName);
    virtual ~ScriptOperation() {}

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted)=0;
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool AtInterruptedPosition(const csVector3& pos, const iSector* sector);
    virtual bool AtInterruptedAngle(const csVector3& pos, const iSector* sector, float angle);
    virtual bool AtInterruptedPosition(NPC *npc);
    virtual bool AtInterruptedAngle(NPC *npc);
    
    /** Check if the move where ok.
     *
     * Check if a move operation has collided, walked out of bounds, or walked in bound. Will
     * fire perceptions at these events with the names given by \se LoadCheckMoveOk
     *
     */
    virtual bool CheckMoveOk(NPC *npc, EventManager *eventmgr, csVector3 oldPos, iSector* oldSector,
                             const csVector3 & newPos, iSector* newSector, float timedelta);

    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr) { completed = true; return completed; }
    virtual bool Load(iDocumentNode *node)=0;
    virtual ScriptOperation *MakeCopy()=0;

    float GetVelocity(NPC *npc);
    float GetAngularVelocity(NPC *npc);
    bool LoadVelocity(iDocumentNode *node);
    
    /** Load attributes for the CheckMoveOk check
     *
     * @param node The node to load attributes from.
     */
    bool LoadCheckMoveOk(iDocumentNode *node);

    /** Copy CheckMoveOk paramters from the source script
     *
     * @param source The script to copy from.
     *
     */
    void CopyCheckMoveOk(ScriptOperation * source);
    
    void AddRandomRange(csVector3& dest,float radius);
    void SetAnimation(NPC *npc, const char *name);

    const char* GetName(){ return name; };
    bool HasCompleted() { return completed; }
    void SetCompleted(bool c) { completed = c; }
    
};

//---------------------------------------------------------------------------
//         Following section contain specefix NPC operations.
//         Ordered alphabeticaly, with exception for when needed
//         due to inheritance. MoveOperation is out of order compared
//         to the implementation file.
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------

/**
* Chase updates periodically and turns, moving towards a certain
* location.  This is normally used to chase a targeted player.
*/
class ChaseOperation : public ScriptOperation
{
protected:
    csString  action;
    int       type;
    float     searchRange;
    float     chaseRange;
    float offset;
    EID       target_id;
    psAPath   path;
    csVector3 localDest;
    
    enum
    {
        NEAREST,OWNER,TARGET
    };
    static const char * typeStr[];
public:

    ChaseOperation(): ScriptOperation("Chase") { target_id=(uint32_t)-1; type = NEAREST; searchRange=2.0; chaseRange=-1.0; ang_vel = 0; vel=0; };
    virtual ~ChaseOperation() {};

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
};


//-----------------------------------------------------------------------------

/**
* Moving entails a velocity vector and an animation action.
*/
class MoveOperation : public ScriptOperation
{
protected:
    csString  action;
    float     duration;

    float     angle;

    // Instance temp variables. These dosn't need to be copied.
    float remaining;
    
    MoveOperation(const char * n): ScriptOperation(n) { duration = 0; ang_vel = 0; angle = 0; }
public:

    MoveOperation(): ScriptOperation("Move") { duration = 0; ang_vel = 0; angle = 0; }
    virtual ~MoveOperation() { }
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);

};

//-----------------------------------------------------------------------------

/**
* Moving entails a circle with radius at a velocity and an animation action.
*/
class CircleOperation : public MoveOperation
{
protected:
    float radius;
public:

    CircleOperation(): MoveOperation("Circle") { radius = 0.0f; }
    virtual ~CircleOperation() { }
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
};

//-----------------------------------------------------------------------------

/**
* Debug will turn on and off debug for the npc. Used for debuging
*/
class DebugOperation : public ScriptOperation
{
protected:
    int      level;
    csString exclusive;

public:

    DebugOperation(): ScriptOperation("Debug") {};
    virtual ~DebugOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Dequip will tell the npc to dequip a item
*/
class DequipOperation : public ScriptOperation
{
protected:
    csString slot;
    
public:

    DequipOperation(): ScriptOperation("Dequip") {};
    virtual ~DequipOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/** Dig will make the NPC dig for a resource.
 *
 *  This class is the implementation of the dig operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples:
 *  <dig resource="tribe:wealth" />
 *  <dig resource="Gold Ore" />
 */
class DigOperation : public ScriptOperation
{
protected:
    csString resource; ///< The name of the resource to dig for.

public:

    DigOperation(): ScriptOperation("Dig") {};
    virtual ~DigOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Drop will make the NPC drop whatever he is
* holding.
*/
class DropOperation : public ScriptOperation
{
protected:
    csString slot;

public:

    DropOperation(): ScriptOperation("Drop") {};
    virtual ~DropOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Eat will take a bite of a nearby dead actor and add resource to tribe wealth.
*
* Examples:
* <eat resource="tribe:wealth" />
* <eat resource="Flesh" />
*
*/
class EatOperation : public ScriptOperation
{
protected:
    csString resource;

public:

    EatOperation(): ScriptOperation("Eat") {};
    virtual ~EatOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Equip will tell the npc to equip a item
*
* Examples:
* <equip item="Sword" slot="righthand" count="1" />
*
*/
class EquipOperation : public ScriptOperation
{
protected:
    csString item;
    csString slot;
    int      count; // Number of items to pick up from a stack
    
public:

    EquipOperation(): ScriptOperation("Equip") {};
    virtual ~EquipOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Invisible will make the npc invisible.
*/
class InvisibleOperation : public ScriptOperation
{
public:

    InvisibleOperation(): ScriptOperation("Invisible") {};
    virtual ~InvisibleOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Locate is a very powerful function which will find
* the nearest object of the named tag, within a range.
*/
class LocateOperation : public ScriptOperation
{
protected:
    float    range;
    csString object;
    bool     static_loc;
    bool     located;
    csVector3 located_pos;
    float     located_angle;
    iSector*  located_sector;
    Waypoint* located_wp;
    float     located_radius;

    bool      random;
    bool      locate_invisible;
    bool      locate_invincible;

public:

    LocateOperation(): ScriptOperation("Locate") { range = 0; static_loc=true; located=false; }
    virtual ~LocateOperation() { }

    /// Use -1 for located_range if range scheck sould be skipped.
    Waypoint* CalculateWaypoint(NPC *npc, csVector3 located_pos, iSector* located_sector, float located_range);

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* LoopBegin operation will only print LoopBegin for debug purpose.
* Looping will be done by the LoopEndOperation.
*/
class LoopBeginOperation : public ScriptOperation
{
public:
    int iterations;

public:

    LoopBeginOperation(): ScriptOperation("BeginLoop") { iterations=0; }
    virtual ~LoopBeginOperation() { }

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* LoopEnd operation will jump back to the beginning
* of the loop.
*/
class LoopEndOperation : public ScriptOperation
{
protected:
    int loopback_op;
    int current;
    int iterations;

public:

    LoopEndOperation(int which,int iterations): ScriptOperation("LoopEnd") { loopback_op = which; this->iterations = iterations; current = 0;}
    virtual ~LoopEndOperation() { }
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Melee will tell the npc to attack the most hated
* entity within range.
*/
class MeleeOperation : public ScriptOperation
{
protected:
    float seek_range, melee_range;
    gemNPCActor* attacked_ent;
    bool  attack_invisible,attack_invincible;
public:

    MeleeOperation(): ScriptOperation("Melee") { attacked_ent=NULL; seek_range=0; melee_range=0; }
    virtual ~MeleeOperation() {}

    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
};

//-----------------------------------------------------------------------------

/**
* Memorize will make the npc to setup a spawn point here
*/
class MemorizeOperation : public ScriptOperation
{
protected:

public:

    MemorizeOperation(): ScriptOperation("Memorize") {};
    virtual ~MemorizeOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

// MoveOperation - Definition has been moved before the first user of this
//                 class as a base class.

//-----------------------------------------------------------------------------

/**
* MovePath specifies the name of a path and an animation action.
*/
class MovePathOperation : public ScriptOperation
{
protected:
    // Parameters
    csString           anim;
    csString           pathname;
    psPath::Direction  direction;

    // Internal variables
    psPath            *path;
    psPathAnchor      *anchor;

public:

    MovePathOperation(): ScriptOperation("MovePath") { path=NULL; }
    virtual ~MovePathOperation() { }
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
};

//-----------------------------------------------------------------------------

/**
* Moving to a spot entails a position vector, a linear velocity,
* and an animation action.
*/
class MoveToOperation : public ScriptOperation
{
protected:
    csVector3 dest;
    csVector3 localDest;
    csString  action;
    psAPath   path;
public:
    MoveToOperation(): ScriptOperation("MoveTo") { dest.Set(0,0,0); vel=0; }
    virtual ~MoveToOperation() { }

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
};

//-----------------------------------------------------------------------------

/**
* Navigate moves the NPC to the position and orientation
* of the last located thing.  (See LocateOperation)
*/
class NavigateOperation : public ScriptOperation
{
protected:
    csString action;
    bool     forceEndPosition;

    float     endAngle;    ///< The angle of the target
    iSector*  endSector;   ///< The sector of the target of the navigate
    csVector3 endPos;      ///< The end position of the target of the navigate
        
public:

    NavigateOperation(): ScriptOperation("Navigate") { vel=0; };
    virtual ~NavigateOperation() {};

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
};

//-----------------------------------------------------------------------------

/** Send a custon perception from a behavior script
 * 
 * Will send the custom perception at the given time in the script.
 */
class PerceptOperation : public ScriptOperation
{
protected:
    csString perception; ///< The perception name to send
    
public:

    PerceptOperation(): ScriptOperation("Percept") {};
    virtual ~PerceptOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Pickup will tell the npc to pickup a nearby
* entity (or fake it).
*/
class PickupOperation : public ScriptOperation
{
protected:
    csString object;
    csString slot;
    int      count; // Number of items to pick up from a stack
    
public:

    PickupOperation(): ScriptOperation("Pickup") {};
    virtual ~PickupOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Reproduce will make the npc to setup a spawn point here
*/
class ReproduceOperation : public ScriptOperation
{
protected:

public:

    ReproduceOperation(): ScriptOperation("Reproduce") {};
    virtual ~ReproduceOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Resurrect will make the npc to setup a spawn point here
*/
class ResurrectOperation : public ScriptOperation
{
protected:

public:

    ResurrectOperation(): ScriptOperation("Resurrect") {};
    virtual ~ResurrectOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/** Implement the reward NPC script operation.
 *
 * Reward will add a given resource to the Tribe that the
 * current NPC is a member of.
 *
 * Examples:
 * <reward resource="Gold Ore" count="2" />
 * <reward resource="tribe:wealth" />
 */
class RewardOperation : public ScriptOperation
{
protected:
    csString resource; ///< The Resource to be rewarded to the tribe.
    int count;         ///< The number of the resource to reward to the tribe.
    
public:

    RewardOperation(): ScriptOperation("Reward") {};
    virtual ~RewardOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Rotating requires storing or determining the angle to 
* rotate to, and the animation action.
*/
class RotateOperation : public ScriptOperation
{
protected:
    enum
    {   
        ROT_UNKNOWN,
        ROT_ABSOLUTE,     // Rotate to this world angle
        ROT_RELATIVE,     // Rotate delta angle from current npd heading
        ROT_TARGET,       // Rotate to face target
        ROT_LOCATEDEST,   // Rotate to face located destination
        ROT_RANDOM,       // Rotate a random angle
        ROT_REGION,       // Rotate to an angle within the region
        ROT_TRIBE_HOME    // Rotate to an angle within tribe home
    };
    int       op_type;              // Type of rotation. See enum above.
    float     min_range, max_range; // Min,Max values for random and region rotation
    float     delta_angle;          // Value to rotate for relative rotation

    float     target_angle;         // Calculated end rotation for every rotation and
                                    // input to absolute rotation
    float     angle_delta;          // Calculated angle that is needed to rotate to target_angle
    
    csString  action;               // Animation to use in the rotation


public:

    RotateOperation(): ScriptOperation("Rotate") { vel=0; op_type=ROT_UNKNOWN; ang_vel=999; }
    virtual ~RotateOperation() { }
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);
    
    float SeekAngle(NPC* npc, float targetYRot);           // Finds an angle which won't lead to a collision
};

//-----------------------------------------------------------------------------

/**
* Sequence will control a named sequence in the world.
*/
class SequenceOperation : public ScriptOperation
{
protected:
    enum // Sequence commands, should use same values as in the psSequenceMessage
    {
        UNKNOWN = 0,
        START = 1,
        STOP = 2,
        LOOP = 3  
    };
    
    csString name;
    int      cmd;    // See enum above
    int      count;  // Number of times to run the sequence

public:

    SequenceOperation(): ScriptOperation("Sequence") {};
    virtual ~SequenceOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* ShareMemories will make the npc share memoreis with tribe
*/
class ShareMemoriesOperation : public ScriptOperation
{
protected:

public:

    ShareMemoriesOperation(): ScriptOperation("ShareMemories") {};
    virtual ~ShareMemoriesOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Talk will tell the npc to communicate to a nearby
* entity.
*/
class TalkOperation : public ScriptOperation
{
protected:
    csString text;
    csString command;
    bool target;

public:

    TalkOperation(): ScriptOperation("Talk") {};
    virtual ~TalkOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Transfer will transfer a item from the NPC to a target. The
* target might be a tribe.
*/
class TransferOperation : public ScriptOperation
{
protected:
    csString item;
    int count;
    csString target;

public:

    TransferOperation(): ScriptOperation("Transfer") {};
    virtual ~TransferOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* TribeHome will make the npc to setup a spawn point here
*/
class TribeHomeOperation : public ScriptOperation
{
protected:

public:

    TribeHomeOperation(): ScriptOperation("Tribe_Home") {};
    virtual ~TribeHomeOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Visible will make the npc visible.
*/
class VisibleOperation : public ScriptOperation
{
public:

    VisibleOperation(): ScriptOperation("Visible") {};
    virtual ~VisibleOperation() {};
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Wait will simply set the mesh animation to
* something and sit there for the desired number
* of seconds.
*/
class WaitOperation : public ScriptOperation
{
protected:
    float duration;
    csString action;

    // Instance temp variables. These dosn't need to be copied.
    float remaining;

public:

    WaitOperation(): ScriptOperation("Wait") { duration=0; }
    virtual ~WaitOperation() { }

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode *node);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual ScriptOperation *MakeCopy();
};

//-----------------------------------------------------------------------------

/**
 * Wander auto-navigates randomly between a network of waypoints specified
 * in the DB.
 */
class WanderOperation : public ScriptOperation
{
protected:
    csString  action;
    bool      random;
    bool      undergroundValid,underground;
    bool      underwaterValid,underwater;
    bool      privValid,priv;
    bool      pubValid,pub;
    bool      cityValid,city;
    bool      indoorValid,indoor;

    // Instance temp variables. These dosn't need to be copied.
    Waypoint *active_wp,*prior_wp,*next_wp;
    csVector3 dest,current_pos;
    iSector  *dest_sector,*current_sector;
    csList<Waypoint*>  waypoint_list;
    bool      turn_queued;
    float     turn_angle_vel;
    float     turn_end_angle;
    
    // Internal variables
    psPath::Direction  direction;
    psPath            *path;
    psPathAnchor      *anchor;

    /** Calculate a random position within the waypoint as destination */
    void CalculateTargetPos(csVector3& dest, iSector*&sector);

    Waypoint * GetNextRandomWaypoint(NPC *npc, Waypoint * prior_wp, Waypoint * active_wp);
    
    /** From current waypoint find next */
    bool FindNextWaypoint(NPC *npc);
    
    /** Set up move from current position to next waypoint */
    bool StartMoveToWaypoint(NPC *npc, EventManager *eventmgr);
    
public:

    WanderOperation(): ScriptOperation("Wander")
        { active_wp=NULL; prior_wp=NULL; next_wp=NULL;
          dest_sector=NULL; current_sector=NULL;
          turn_queued=false;
          turn_angle_vel=0.0f; turn_end_angle=0.0f; path=NULL, anchor=NULL;
        };
    virtual ~WanderOperation() {};

    bool CalculateWaypointList(NPC *npc);
    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);

    void WaypointListClear() { waypoint_list.DeleteAll(); }
    void WaypointListPushBack(Waypoint * wp) { waypoint_list.PushBack(wp); }
    Waypoint * WaypointListGetNext();
    bool WaypointListEmpty() { return waypoint_list.IsEmpty(); }
    csList<Waypoint*> WaypointListGet() { return waypoint_list; }
};

//-----------------------------------------------------------------------------

/**
* Watch operation will tell if the targt goes out of range.
*/
class WatchOperation : public ScriptOperation
{
protected:
    float     watchRange;
    int       type;
    float     range;  //  Used for watch of type NEAREST 
    bool      watchInvisible, watchInvincible;

    csWeakRef<gemNPCObject> watchedEnt;

    enum
    {
        NEAREST,OWNER,TARGET
    };
    static const char * typeStr[];
    
public:

    WatchOperation(): ScriptOperation("Watch") { watchedEnt=NULL; watchRange=0; }
    virtual ~WatchOperation() {}

    virtual bool Load(iDocumentNode *node);
    virtual ScriptOperation *MakeCopy();

    virtual bool Run(NPC *npc,EventManager *eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC *npc,EventManager *eventmgr);
    virtual void InterruptOperation(NPC *npc,EventManager *eventmgr);
    virtual bool CompleteOperation(NPC *npc,EventManager *eventmgr);

 private:
    bool OutOfRange(NPC *npc);
};

//-----------------------------------------------------------------------------

#endif
