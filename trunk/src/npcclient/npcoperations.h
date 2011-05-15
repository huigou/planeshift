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
#include "util/pspathnetwork.h"
#include <tools/celhpf.h>
#include "net/npcmessages.h"
#include "util/edge.h"

//=============================================================================
// Project Includes
//=============================================================================

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

public:
    // Used to indicate the result of the Run and Advance operations.
    enum OperationResult {
        OPERATION_NOT_COMPLETED, ///< Used to indicate that an operation will complete at a later stage
        OPERATION_COMPLETED,     ///< All parts of this operation has been completed
        OPERATION_FAILED         ///< The operation failed
    };
    
protected:
    ////////////////////////////////////////////////////////////
    // Start of instance temp variables. These dosn't need to be copied.
    csString             name;

    bool                 completed; /// This flag is set to false by Run(), and set to true by CompleteOperation(), in case of multiple Complete's being called.

    psResumeScriptEvent* resumeScriptEvent;

    csVector3            interrupted_position;
    iSector             *interrupted_sector;
    float                interrupted_angle;

    int                  consecCollisions; ///< Shared by move functions. Used by CheckMoveOk to detect collisions
    // End of instance temp variables.
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // Start of shared values between operation

    // Failure handling
    csString             failurePerception;    ///< The Perception to fire when a operation return OPERATION_FAILED

    // Velocity is shared for all move operations
    VelSource            velSource;            ///< Source used for velocity
    float                vel;                  ///< Shared linear velocity, used by all that moves
    float                ang_vel;              ///< Shared angular velocity, used by all that rotates

    // Start Check Move OK parameters
    // Configuration paramters. Set by using LoadCheckMoveOk    
    csString             collision;       ///< Perception names to use for collision detected by CheckMoveOk
    csString             outOfBounds;     ///< Perception names to use for out of bounds detected by CheckMoveOk
    csString             inBounds;        ///< Perception names to use for in bounds detected by CheckMoveOk
    csString             falling;        ///< Perception names to use for fall detected by CheckMoveOk
    bool                 checkTribeHome;  ///< Set to true if the tribe home should be checked by CheckMoveOk
    // End Check Move OK parameters

    
    // End of shared values between operations
    ////////////////////////////////////////////////////////////

    
    void Resume(csTicks delay, NPC* npc, EventManager* eventmgr);
    void ResumeTrigger(psResumeScriptEvent*  event);
    void StopResume();

    /// This function is used by MoveTo AND Navigate operations
    int StartMoveTo(NPC* npc, EventManager* eventmgr, const csVector3& dest, iSector* sector, float vel,const char* action, bool autoresume, float &angle);
    
    /// This function is used by MoveTo AND Navigate operations
    int StartTurnTo(NPC* npc,EventManager* eventmgr,float turn_end_angle, float ang_vel,const char* action, bool autoresume=true);

    void TurnTo(NPC* npc,const csVector3& dest, iSector* destsect, csVector3& forward, float &angle);

    /// Utility function used by many operation to stop movement of an NPC.
    static void StopMovement(NPC* npc);

public:

    ScriptOperation(const char* sciptName);
    ScriptOperation(const ScriptOperation* other);
    virtual ~ScriptOperation() {}

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted)=0;
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);

    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool AtInterruptedPosition(const csVector3& pos, const iSector* sector);
    virtual bool AtInterruptedAngle(const csVector3& pos, const iSector* sector, float angle);
    virtual bool AtInterruptedPosition(NPC* npc);
    virtual bool AtInterruptedAngle(NPC* npc);

 private:
    /** Send a collition perception to the npc.
     *
     * Get the collisiont detection for this operation.
     * Can either be part of this operation, or the npctype.
     *
     * @param npc The npc to percept.
     */
    void SendCollitionPerception(NPC* npc);
    
 public:
    /** Check if the move where ok.
     *
     * Check if a move operation has collided, walked out of bounds, or walked in bound. Will
     * fire perceptions at these events with the names given by \se LoadCheckMoveOk
     *
     */
    virtual bool CheckMoveOk(NPC* npc, EventManager* eventmgr, csVector3 oldPos, iSector* oldSector,
                             const csVector3 & newPos, iSector* newSector, float timedelta);

    /** Return the Collision perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString& GetCollisionPerception(NPC* npc); 

    /** Return the Out of Bounds perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString& GetOutOfBoundsPerception(NPC* npc); 

    /** Return the In Bounds perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString& GetInBoundsPerception(NPC* npc); 

    /** Return the Falling perception event name.
     *
     *  Will use the falling perception of the operation if defined, or it will
     *  check if a global fall perception event name is defined for the brain.
     */
    virtual const csString& GetFallingPerception(NPC* npc); 

    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr) { completed = true; return completed; }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy()=0;

    float GetVelocity(NPC* npc);
    float GetAngularVelocity(NPC* npc);
    bool LoadVelocity(iDocumentNode* node);
    
    /** Load attributes for the CheckMoveOk check
     *
     * @param node The node to load attributes from.
     */
    bool LoadCheckMoveOk(iDocumentNode* node);

    /** Copy CheckMoveOk paramters from the source script
     *
     * @param source The script to copy from.
     *
     */
    void CopyCheckMoveOk(ScriptOperation*  source);
    
    /** Move a point somwhere random within radius of the orignial point.
     *
     * Will add a random part to the dest. The margin can
     * be used to prevent using the margin around the edge of the circle.
     *
     * @param dest   The center position of the cicle. Will return with the
     *               new position.
     * @param radius The radius of the circle.
     * @param margin A margin around the circle that will not be used.
     *
     */
    void AddRandomRange(csVector3 &dest, float radius, float margin = 0.0);
    void SetAnimation(NPC* npc, const char* name);

    virtual const char* GetName() const { return name.GetDataSafe(); };
    bool HasCompleted() { return completed; }
    void SetCompleted(bool c) { completed = c; }

    /**
     * Called when the run operation return OPERATION_FAILED.
     * Default implementation will percept with the failure perception
     */
    virtual void Failure(NPC* npc);
};

//-----------------------------------------------------------------------------

/**
* Abstract common class for Move operations that use paths.
*/
class MovementOperation : public ScriptOperation
{
protected:

    // Instance variables
    csRef<iCelHPath> path;

    // Cache values for end position 
    csVector3 endPos;
    iSector*  endSector;

    float currentDistance;          ///< The distance to the current local destination

    // Operation parameters
    csString         action;        ///< The animation used during chase

    // Constructor
    MovementOperation( const MovementOperation* other );

public:

    MovementOperation( const char*  name );

    virtual ~MovementOperation() { }

    virtual bool Load(iDocumentNode* node);

    bool EndPointChanged(const csVector3 &endPos, const iSector* endSector) const;

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector) = 0;

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector) = 0;

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);

    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);

    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);

    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);
};

//---------------------------------------------------------------------------
//         Following section contain specefix NPC operations.
//         Ordered alphabeticaly, with exception for when needed
//         due to inheritance. MoveOperation is out of order compared
//         to the implementation file.
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------

/** Detect and chase a target until reached o out of bound.
*   Chase updates periodically and turns, moving towards a certain
*   location.  This is normally used to chase a targeted player.
*/
class ChaseOperation : public MovementOperation
{
protected:
    /** @name Instance varaibles
     *  These parameters are local to one instance of the operation.
     */
    //@{
    EID              targetEID;              ///< The EID of the chased target
    float            offsetAngle;            ///< The actual offset angle in radians
    csVector3        offsetDelta;            ///< The actual delta relative to target
    //@}

    /** @name Operation Parameters
     *  These parameters are initialized by the Load function
     *  and copied by the MakeCopy function.
     */
    //@{
    int              type;                   ///< The type of chase to perform
    float            searchRange;            ///< Search for targets within this range
    float            chaseRange;             ///< Chase as long targets are within this range.
                                             ///< Chase forever if set to -1.
    float            offset;                 ///< Used to stop a offset from the target.
    float            offsetAngleMax;         ///< The maximum offset angle in radians
    float            sideOffset;             ///< Add a offset to the side of the target
    bool             offsetRelativeHeading;  ///< Set to true will make the offset relative target heading
    //@}
    
    enum
    {
        NEAREST_ACTOR,   ///< Sense Players and NPC's
        NEAREST_NPC,     ///< Sense only NPC's
        NEAREST_PLAYER,  ///< Sense only players
        OWNER,           ///< Sense only the owner
        TARGET           ///< Sense only target
    };
    static const char * typeStr[];

    /** Constructor for this operation, used by the MakeCopy.
     *
     *  This constructor will copy all the Operation Parameters
     *  from the other operation and initialize all Instance Variables
     *  to default values.
     */
    ChaseOperation(const ChaseOperation* other);

public:
    /** Constructor for this operation.
     *
     *  Construct this operation and initialzie with default
     *  Instance Variables. The Operation Parameters
     */
    ChaseOperation();

    /** Destructor for this operation
     */
    virtual ~ChaseOperation() {};

    /** Calculate any offset from target to stop chase.
     *
     *  The chase support several different modes. The default is that the
     *  chaser ends up at the top of the target. This function calculate the offset
     *  as a 3D vector that represents the distance from the target to the end
     *  point of the chase.
     */
    csVector3 CalculateOffsetDelta(const csVector3 &myPos, const iSector* mySector,
                                   const csVector3 &endPos, const iSector* endSector,
                                   float endRot) const;

    /** Calculate the end position for this chase.
     *
     *  Called by the Run funciton in the MovementOperation. Will calculate
     *  end posiiton for the chase including any offset deltas.
     */
    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    /** Call to update the target to chase.
     *
     *  When called the chase target will be checked to se if a new target should be selected.
     *  This mostly apply to chases of type NEAREST_*
     */
    virtual gemNPCActor* UpdateChaseTarget(NPC* npc, const csVector3 &myPos, const iSector* mySector);

    /** Update end position for moving targets.
     *
     *  Called from advance in the MovementOperation. Will update the end position for
     *  targets that moves. This may include change of target as well.
     */
    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    /** Load Operation Parameters from xml.
     *
     *  Load this operation from the given node. This will
     *  initialzie the Operation Parameters for this operation.
     */
    virtual bool Load(iDocumentNode* node);

    /** Make a deep copy of this operation.
     *
     *  MakeCopy will make a copy of all Operation Parameters and reset each
     *  Instance Variable.
     */
    virtual ScriptOperation* MakeCopy();
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
    
    MoveOperation(const char*  n): ScriptOperation(n) { duration = 0; ang_vel = 0; angle = 0; }
public:

    MoveOperation(): ScriptOperation("Move") { duration = 0; ang_vel = 0; angle = 0; }
    virtual ~MoveOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);

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
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Emote will make the NPC show an emotion
 *
 *  This class is the implementation of the emote operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples:
 *  <emote cmd="greet" />
 */
class EmoteOperation : public ScriptOperation
{
protected:
    csString cmd; ///< The emote command

public:

    EmoteOperation(): ScriptOperation("Emote") {};
    virtual ~EmoteOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Locate is a very powerful function which will find
* the nearest object of the named tag, within a range.
*/
class LocateOperation : public ScriptOperation
{
protected:
    // Instance variables
    bool      located;
    csVector3 located_pos;
    float     located_angle;
    iSector*  located_sector;
    Waypoint* located_wp;
    float     located_radius;

    // Operation parameters
    csString  object;
    float     range;
    bool      static_loc;
    bool      random;
    bool      locate_invisible;
    bool      locate_invincible;

    
public:
    LocateOperation();
    LocateOperation(const LocateOperation* other);
    virtual ~LocateOperation() { }

    /// Use -1 for located_range if range scheck sould be skipped.
    Waypoint* CalculateWaypoint(NPC* npc, csVector3 located_pos, iSector* located_sector, float located_range);

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
private:
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

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    psPath*            path;
    psPathAnchor*      anchor;

public:

    MovePathOperation(): ScriptOperation("MovePath") { path=NULL; }
    virtual ~MovePathOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);
};

//-----------------------------------------------------------------------------

/**
* Moving to a spot entails a position vector, a linear velocity,
* and an animation action.
*/
class MoveToOperation : public MovementOperation
{
protected:
    csVector3 destPos;
    iSector*  destSector;
    
    csString  action;
public:
    MoveToOperation();
    MoveToOperation(const MoveToOperation* other);
    virtual ~MoveToOperation() { }

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Navigate moves the NPC to the position and orientation
* of the last located thing.  (See LocateOperation)
*/
class NavigateOperation : public MovementOperation
{
protected:
    csString action;
    bool     forceEndPosition;

    float     endAngle;    ///< The angle of the target
    iSector*  endSector;   ///< The sector of the target of the navigate
    csVector3 endPos;      ///< The end position of the target of the navigate
        
public:

    NavigateOperation();
protected:
    NavigateOperation(const NavigateOperation* other );
public:
    virtual ~NavigateOperation() {};

    virtual ScriptOperation* MakeCopy();

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    virtual bool Load(iDocumentNode* node);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);
};

//-----------------------------------------------------------------------------

/** Send a custon perception from a behavior script
 * 
 * Will send the custom perception at the given time in the script.
 */
class PerceptOperation : public ScriptOperation
{
protected:
    enum TargetType {
        SELF,
        ALL,
        TRIBE,
        TARGET
    };
    
    csString   perception; ///< The perception name to send
    TargetType target;     ///< Hold the target for the perception, default SELF
    float      maxRange;   ///< Is there a max range for this, 0.0 is without limit
        
public:

    PerceptOperation(): ScriptOperation("Percept") {};
    virtual ~PerceptOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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

    RotateOperation(): ScriptOperation("Rotate")
    {
        vel=0; ang_vel=999; 
        op_type=ROT_UNKNOWN;
        min_range=0; max_range=0;
        delta_angle=0;
        target_angle=0;
        angle_delta=0;
    }
    virtual ~RotateOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);
    
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
    
    csString sequenceName;
    int      cmd;    // See enum above
    int      count;  // Number of times to run the sequence

public:

    SequenceOperation(): ScriptOperation("Sequence") {};
    virtual ~SequenceOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Sit will make the NPC sit or stand
 *
 *  This class is the implementation of the sit operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples:
 *  <sit />
 *  <standup />
 */
class SitOperation : public ScriptOperation
{
protected:
    bool sit; ///< True if sit false for stand

public:

    SitOperation(bool sit): ScriptOperation("Sit"), sit(sit) {};
    virtual ~SitOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Talk will tell the npc to communicate to a nearby
* entity.
*/
class TalkOperation : public ScriptOperation
{
protected:
    typedef psNPCCommandsMessage::PerceptionTalkType TalkType;
    
    csString talkText;   ///< The text to send to the clients
    TalkType talkType;   ///< What kind of talk, default say
    bool     talkPublic; ///< Should this be public or only to the target
    bool     target;     ///< True if this is should go to the target of the NPC.
    csString command;    ///< Command to percept to target if target is a NPC.

public:

    TalkOperation(): ScriptOperation("Talk") {};
    virtual ~TalkOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Teleport will teleport the NPC to the target position.
*/
class TeleportOperation : public ScriptOperation
{
protected:

public:

    TeleportOperation(): ScriptOperation("Teleport") {};
    virtual ~TeleportOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* TribeType will change the need set used by the npc.
*/
class TribeTypeOperation : public ScriptOperation
{
protected:
    int tribeType;
public:

    TribeTypeOperation(): ScriptOperation("Tribe_Type") {};
    virtual ~TribeTypeOperation() {};
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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
    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
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

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
 * Wander auto-navigates randomly between a network of waypoints specified
 * in the DB.
 */
class WanderOperation : public ScriptOperation
{
protected:
    ////////////////////////////////////////////////////////////
    // Start of instance temp variables. These dosn't need to be copied.

    class WanderRouteFilter : public psPathNetwork::RouteFilter
    {
      public:
        WanderRouteFilter( WanderOperation*  parent ):parent(parent){};
        virtual bool Filter( const Waypoint* waypoint ) const;
      protected:
        WanderOperation*  parent;
    };

    WanderRouteFilter wanderRouteFilter;

    csList<Edge*> edgeList;

    csList<Edge*>::Iterator   edgeIterator;
    Edge*                     currentEdge;              ///< The current edge
    Edge::Iterator*           currentPathPointIterator; ///< The current iterator for path points
    psPathPoint*              currentPathPoint;         ///< The current local destination
    csVector3                 currentPointOffset;       ///< The offset used for current local dest.
    float                     currentDistance;          ///< The distance to the current local destination
//    psPath::PathPointIterator pointIterator;

    // End of instance temp variables.
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // Start of operation parameters
    csString  action;
    bool      random;
    bool      undergroundValid,underground;
    bool      underwaterValid,underwater;
    bool      privValid,priv;
    bool      pubValid,pub;
    bool      cityValid,city;
    bool      indoorValid,indoor;
    bool      pathValid,path;
    bool      roadValid,road;
    bool      groundValid,ground;
    // End of operation parameters
    ////////////////////////////////////////////////////////////

    /** Start move to point.
     *
     *  Set up a movement from current position to next local dest point.
     */
    bool StartMoveTo(NPC* npc, psPathPoint* point);

    /** Teleport to point.
     *
     *  Teleport from current position to next local dest point.
     */
    bool MoveTo(NPC* npc, psPathPoint* point);

    /** Calculate the edgeList to be used by wander.
     *
     *  Will calculate a list of edges between start and end for wander.
     *  For random wander the list is empty but still returning true. 
     */
    OperationResult CalculateEdgeList(NPC* npc);
    

    /** Set the current path point iterator.
     */
    void SetPathPointIterator(Edge::Iterator* iterator);

public:

    WanderOperation();
    WanderOperation(const WanderOperation* other);
    virtual ~WanderOperation();

    virtual ScriptOperation* MakeCopy();


    /** Clear list of edges
     */
    void EdgeListClear() { edgeList.DeleteAll(); }

    /** Get the next edge for local destination
     *
     *  Will return the next edge from the edge list or
     *  find a new radom edge for random wander.
     */
    Edge* GetNextEdge(NPC* npc);


    /** Get the next path point to use for local destination
     *
     *  Will return the next path point from the active edge.
     *  It will get next edge when at end of path. This include
     *  random edge if operation is random.
     */
    psPathPoint* GetNextPathPoint(NPC* npc, bool &teleport);

    /** Return the current path point
     *
     */
    psPathPoint* GetCurrentPathPoint(NPC* npc);


    /* Utility function to calcualte distance from npc to destPoint.
     */
    static float DistanceToDestPoint( NPC* npc, const csVector3& destPos, const iSector* destSector );


    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool Load(iDocumentNode* node);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);


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
    float     searchRange;       ///<  Used for watch of type NEAREST_* 
    bool      watchInvisible;
    bool      watchInvincible;

    csWeakRef<gemNPCObject> watchedEnt;

    enum
    {
        NEAREST_ACTOR,   ///< Sense Players and NPC's
        NEAREST_NPC,     ///< Sense only NPC's
        NEAREST_PLAYER,  ///< Sense only players
        OWNER,
        TARGET
    };
    static const char*  typeStr[];
    
public:

    WatchOperation(): ScriptOperation("Watch") { watchedEnt=NULL; watchRange=0; }
    virtual ~WatchOperation() {}

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,EventManager* eventmgr,bool interrupted);
    virtual void Advance(float timedelta,NPC* npc,EventManager* eventmgr);
    virtual void InterruptOperation(NPC* npc,EventManager* eventmgr);
    virtual bool CompleteOperation(NPC* npc,EventManager* eventmgr);

 private:
    bool OutOfRange(NPC* npc);
};

//-----------------------------------------------------------------------------

#endif
