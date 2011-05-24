/*
* npcbehave.h
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

/* This file holds definitions for ALL global variables in the planeshift
* server, normally you should move global variables into the psServer class
*/
#ifndef __NPCBEHAVE_H__
#define __NPCBEHAVE_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/matrix3.h>
#include <csutil/array.h>
#include <iutil/document.h>

struct iSector;

//=============================================================================
// Library Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/gameevent.h"
#include "util/consoleout.h"
#include "util/pspath.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "perceptions.h"
#include "walkpoly.h"
#include "npcoperations.h"

class ScriptOperation;
class Perception;
class Reaction;
class Behavior;
class NPC;
class EventManager;
class BehaviorSet;
class Waypoint;
class psResumeScriptEvent;

/** This is the set of Behaviors available for an NPC
 *
 * This is a collection of behaviors for a particular
 * type of NPC. They represents the behaviors that
 * this NPC is capable of.
 *
 */
class BehaviorSet
{
protected:
    csPDelArray<Behavior>  behaviors; ///< The set of behavoirs for this NPCType.
    Behavior*              active;    ///< Points to the current active behavior.
        
    EventManager*          eventmgr;  ///< Cached pointer to the event manger.

public:
    /** Constructor
     */
    BehaviorSet(EventManager *eventmanager);

    /** Add a behavior to the brain
     *
     * Add a new behavior to the set. If a behavior with the same name exists,
     * than the new behavior will substitude the existing behavior.
     * This to support the nested structure of the behavoir scripts.
     *
     * @param behavior The new behavior to be added to this BehaviorSet.
     *
     * @return Returns true if the behavior didn't already exist. Otherwise returns false and substitude the existing duplicate behavior.
     */
    bool Add(Behavior *behavior);

    /** Find a behavior in the set.
     *
     *  Search the set for a behavior maching the given name.
     */
    Behavior *Find(const char *name);

    /** Do a deap copy
     *
     * Will do a deap copy of the BehaviorSet. The NPCType will contain a complete
     * BehaviorSet for each type. When a new NPC is instantiated a deap copy of the
     * set will be done before assigning it to the new NPC.
     *
     * @param other The source to be copied into this set.
     */
    void DeepCopy(BehaviorSet& other);

    /** Prepare the set after use
     *
     * Called when the npc actor is deleted. Should reset all session
     * depended stats. So that a new instance of the actor for the
     * npc can start with a fresh brain.
     *
     * @param npc The NPC that own this BehaviorSet.
     */
    void ClearState(NPC *npc);

    /** Advances the behaviors
     *
     * Will calculate the need and select the active behavior.
     *
     * @param delta The numbers of ticks to advance this set.
     * @param npc   The NPC that own this BehaviorSet.
     *
     */
    void Advance(csTicks delta,NPC *npc);

    /** Interrupt the current active behavior.
     *
     * If there is an active behavior in this set that
     * behavior will be interrupted by calling this function.
     *
     * @param npc   The NPC that own this BehaviorSet.
     */
    void Interrupt(NPC *npc);

    /** Resume the current active behavior.
     *
     * Resume the given behavior. This is witch is equal to
     * the current active behavior and witch is
     * applicable to the current npc state.
     *
     * @param npc   The NPC that own this BehaviorSet.
     * @param witch The behavior to resume.
     */
    void ResumeScript(NPC *npc, Behavior *which);
    
    /** Return current active behavior
     */
    Behavior *GetCurrentBehavior() { return active; }

    /** Dump the behavior list for debug.
     *
     * @param npc   The NPC that own this BehaviorSet.
     */
    void DumpBehaviorList(NPC *npc);

    /** Info about the behavior list for debug.
     *
     * @param npc   The NPC that own this BehaviorSet.
     */
    csString InfoBehaviors(NPC *npc);

};


/** A collection of behaviors and reactions will represent a type of npc.
 *
 * Each npc will be assigned one of these types.  This lets us
 * reuse the same script for many mobs at once--each one keeping its
 * own state information.
 */
class NPCType
{
    enum VelSource {
        VEL_DEFAULT,
        VEL_USER,
        VEL_WALK,
        VEL_RUN
    };
    
protected:
    csString              name;      ///< The name of this NPC type.
    csPDelArray<Reaction> reactions; ///< The reactions available for this NPCType.
    BehaviorSet           behaviors; ///< The set of behaviors available for this NPCType.
    float                 ang_vel;   ///< Default ang_vel for this NPCType.
                                     ///< Will be used for all behaviors unless overriden
                                     ///< by each behavior.
    float                 vel;       ///< Default vel for this NPCType.
                                     ///< Will be used for all behaviors unless overriden
                                     ///< by each behavior.
    VelSource             velSource; ///< 

    csString              collisionPerception;   ///< Global perception value for falling
    csString              outOfBoundsPerception; ///< Global perception value for falling
    csString              inBoundsPerception;    ///< Global perception value for falling
    csString              fallingPerception;     ///< Global perception value for falling

public:
    NPCType(psNPCClient* npcclient, EventManager* eventmanager);
    NPCType(NPCType& other, EventManager* eventmanager): behaviors(eventmanager) { DeepCopy(other); }

    ~NPCType();
    void DeepCopy(NPCType& other);
    void ClearState(NPC *npc);

    bool Load(iDocumentNode *node);
    bool Load(iResultRow &node);
    const char* GetName(){ return name.GetDataSafe(); }

    /** Find a behavior in the set.
     *
     *  Search the set for a behavior maching the given name.
     */
    Behavior *Find(const char *name) { return behaviors.Find(name); }

    void Advance(csTicks delta,NPC *npc);
    void Interrupt(NPC *npc);
    void ResumeScript(NPC *npc,Behavior *which);
    void FirePerception(NPC *npc,Perception *pcpt);

    void DumpBehaviorList(NPC *npc) { behaviors.DumpBehaviorList(npc); }
    csString InfoBehaviors(NPC *npc) { return behaviors.InfoBehaviors(npc); }
    void DumpReactionList(NPC *npc);

    Behavior *GetCurrentBehavior()
    {
        return behaviors.GetCurrentBehavior();
    }
    
    float GetAngularVelocity(NPC *npc);
    float GetVelocity(NPC *npc);

    const csString& GetCollisionPerception() const;
    const csString& GetOutOfBoundsPerception() const;
    const csString& GetInBoundsPerception() const;
    const csString& GetFallingPerception() const;
    
private:
    static psNPCClient* npcclient;
};


/** A set of operations building a generic behavior for a NPC.
 *
 * A Behavior is a general activity of an NPC.  Guarding,
 * smithing, talking to a player, fighting, fleeing, eating,
 * sleeping are all behaviors. They consists of a set of
 * ScriptOperations loaded from file.
 *
 */
class Behavior
{
protected:
    csString name;                      ///< The name of this behavior

    csPDelArray<ScriptOperation> sequence; ///< Sequence of ScriptOperations.
    size_t   current_step;              ///< The ScriptOperation in the sequence that is currently executed.
    bool     loop;                      ///< True if this behavior should start over when completed all operations.
    bool     isActive;                  ///< Set to true when this behavior is active
    bool     is_applicable_when_dead;
    float    need_decay_rate;           ///< need lessens while performing behavior
    float    need_growth_rate;          ///< need grows while not performing behavior
    float    completion_decay;          ///< need lessens AFTER behavior script is complete. Use -1 to remove all need
    float    init_need;                 ///< starting need, also used in ClearState resets
    bool     resume_after_interrupt;    ///< Resume at active step after interrupt.
        
    float    current_need;              ///< The current need of this behavior after last advance
    float    new_need;                  ///< The accumulated change to the need after last advance
    
    bool     interrupted;               ///< Set to true if this behavior is interruped by another in a BehaviorSet
    
    bool     minLimitValid;             ///< True if a minimum limit for the need for this bahavior has been set
    float    minLimit;                  ///< The minimum value to limit the need if minLimitValid has been set true.
    bool     maxLimitValid;             ///< True if a maximum limit for the need for this bahavior has been set
    float    maxLimit;                  ///< The maximum value to limit the need if maxLimitValid has been set true.
    
public:
    Behavior();
    Behavior(const char *n);
    Behavior(Behavior& other);
    
    virtual ~Behavior() { };

    const char *GetName() { return name; }

    void DeepCopy(Behavior& other);
    bool Load(iDocumentNode *node);

    bool LoadScript(iDocumentNode *node,bool top_level=true);

    void  Advance(csTicks delta,NPC *npc,EventManager *eventmgr);
    float CurrentNeed() { return current_need; }
    float NewNeed() { return new_need; }

    /** Update the current_need of the behavour after it has been advanced.
     *
     * If the new_need has been updating during advance make the new_need the
     * current need of this behavior. The current_need will allways be the
     * value held right after last advance of the behavior. During advance
     * and between advance the new_need will be updated by the advance function
     * and reactions based on matched perceptions.
     *
     */
    void  CommitAdvance();

    /** Update the new need of the behavior with a delta desire.
     *
     * The need is adjusted by the given delta value.
     *
     * @param delta_desire The delta in desire for this behavior
     *
     * \note Will limit the new_need if limits has been given for the
     *       need of this behavior.
     */
    void ApplyNeedDelta(NPC *npc, float deltaDesire);

    
    /** Set the new need of the behavior to the given desire.
     *
     * The need is set to the given desire. Ignoring whatever
     * value the new need of the behavoir might have had previous.
     *
     * @param absolute_desire The absolute desire to be used for this behavior
     *
     * \note Will limit the new_need if limits has been given for the
     *       need of this behavior.
     */
    void ApplyNeedAbsolute(NPC *npc, float absoluteDesire);
    
    void SetIsActive(bool flag) { isActive = flag; }

    /** Used to check if the behavior is active.
     *
     * An active behavior is an behavior that will
     * be advanced and decay in need.
     *
     */
    bool IsActive(){ return isActive; }
    
    void SetCurrentStep(int step) { current_step = step; }
    size_t GetCurrentStep(){ return current_step; }
    void ResetNeed() { current_need = new_need = init_need; }

    bool ApplicableToNPCState(NPC* npc);
    void DoCompletionDecay(NPC* npc);
    bool StartScript(NPC* npc, EventManager* eventmgr);
    bool RunScript(NPC* npc, EventManager* eventmgr, bool interruped);
    bool ResumeScript(NPC* npc, EventManager* eventmgr);
    void InterruptScript(NPC* npc, EventManager* eventmgr);
    bool IsInterrupted(){ return interrupted; }
    void ClearInterrupted() { interrupted = false; }
    Behavior* SetCompletionDecay(float completion_decay) { this->completion_decay = completion_decay; return this; }
    
    inline bool operator==(const Behavior& other)
    {    return (current_need == other.current_need &&
                name == other.name  );    }
    bool operator<(Behavior& other)
    {
        if (current_need > other.current_need)
            return true;
        if (current_need < other.current_need)
            return false;
        if (strcmp(name,other.name)>0)
            return true;
        return false;
    }
    
    // For testing purposes only.
    
    Behavior* SetDecay(float need_decay_rate) { this->need_decay_rate = need_decay_rate; return this; }
    Behavior* SetGrowth(float need_growth_rate) { this->need_growth_rate = need_growth_rate; return this; }
    Behavior* SetInitial(float init_need) { this->init_need = init_need; return this; }
};




class psResumeScriptEvent : public psGameEvent
{
protected:
    NPC *npc;
    EventManager    *eventmgr;
    Behavior        *behavior;
    ScriptOperation *scriptOp;

public:
    psResumeScriptEvent(int offsetticks, NPC *c,EventManager *mgr,Behavior *which,ScriptOperation * script);
    virtual void Trigger();  // Abstract event processing function
    virtual csString ToString() const;
};


class psGameObject
{
public:
    /** Helper function to return position of a gemNPCObject
     *
     * @param object The object to thet the position of
     * @param pos    Return the position of object
     * @param yrot   Return the Y Rotation of the object
     * @param sector Return the sector of object
     */
    static void GetPosition(gemNPCObject* object, csVector3 &pos, float &yrot, iSector* &sector);
    
    /** Helper function to return position of a gemNPCObject
     *
     * @param object The object to thet the position of
     * @param pos    Return the position of object
     * @param sector Return the sector of object
     */
    static void GetPosition(gemNPCObject* object, csVector3 &pos, iSector* &sector);
    
    static void SetPosition(gemNPCObject* objecty, const csVector3 &pos, iSector* = NULL);
    static void SetRotationAngle(gemNPCObject* object, float angle);
    static void GetRotationAngle(gemNPCObject* object, float &yrot)
    {
        csVector3 pos;
        iSector *sector;
        GetPosition(object,pos,yrot,sector);
    }
    /**
     * @return The angle in the range [-PI,PI]
     */
    static float CalculateIncidentAngle(const csVector3& pos, const csVector3& dest);
    
    // Clamp the angle within 0 to 2*PI
    static void ClampRadians(float &target_angle);
    
    // Normalize angle within -PI to PI
    static void NormalizeRadians(float &target_angle);

    static csVector3 DisplaceTargetPos(const iSector* mySector, const csVector3& myPos,
                                       const iSector* targetSector, const csVector3& targetPos,
                                       float offset);
    static csVector3 DisplaceTargetPos(const iSector* mySector, const csVector3& myPos,
                                       const iSector* targetSector, const csVector3& targetPos,
                                       float offset, float angle);

    static float Calc2DDistance(const csVector3 & a, const csVector3 & b);


};


#endif

