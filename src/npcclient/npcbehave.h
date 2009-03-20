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

class ScriptOperation;
class Perception;
class Reaction;
class Behavior;
class NPC;
class EventManager;
class BehaviorSet;
class Waypoint;
class psResumeScriptEvent;

/**
* This is just a collection of behaviors for a particular
* type of NPC.
*/
class BehaviorSet
{
protected:
    // BinaryTree<Behavior> behaviors;
    csPDelArray<Behavior> behaviors;
    Behavior *active;
    float max_need;
    EventManager *eventmgr;

public:
    BehaviorSet(EventManager *eventmanager) { active=NULL; eventmgr = eventmanager; }

    /// Returns true if the behavior didn't already exist. Otherwise returns false and removes the existing duplicate behavior.
    bool Add(Behavior *b);
    Behavior *Find(Behavior *key);
    
    Behavior *Find(const char *name);
    void DeepCopy(BehaviorSet& other);
    void ClearState(NPC *npc);

    /// Advances the behaviors and returns the active one.
    Behavior* Advance(csTicks delta,NPC *npc);
    void Interrupt(NPC *npc);
    void ResumeScript(NPC *npc,Behavior *which);
    Behavior *GetCurrentBehavior() { return active; }
    void DumpBehaviorList(NPC *npc);
};


/**
* A collection of behaviors and reactions will represent a type of
* npc.  Each npc will be assigned one of these types.  This lets us
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
    csArray<Reaction*> reactions;
    BehaviorSet behaviors;
    csString name;
    float    ang_vel,vel;
    VelSource velSource;

public:
    NPCType(psNPCClient* npcclient, EventManager* eventmanager);
    NPCType(NPCType& other, EventManager* eventmanager): behaviors(eventmanager) { DeepCopy(other); }

    ~NPCType();
    void DeepCopy(NPCType& other);
    void ClearState(NPC *npc);

    bool Load(iDocumentNode *node);
    const char* GetName(){ return name.GetDataSafe(); }

    void Advance(csTicks delta,NPC *npc);
    void Interrupt(NPC *npc);
    void ResumeScript(NPC *npc,Behavior *which);
    void FirePerception(NPC *npc,Perception *pcpt);

    void DumpBehaviorList(NPC *npc) { behaviors.DumpBehaviorList(npc); }
    void DumpReactionList(NPC *npc);

    Behavior *GetCurrentBehavior()
    {
        return behaviors.GetCurrentBehavior();
    }
    
    float GetAngularVelocity(NPC *npc);
    float GetVelocity(NPC *npc);
    
private:
    static psNPCClient* npcclient;
};


/**
* A Behavior is a general activity of an NPC.  Guarding,
* smithing, talking to a player, fighting, fleeing, eating,
* sleeping are all behaviors I expect to be scripted
* for our npcs.
*/
class Behavior
{
protected:
    csArray<ScriptOperation*> sequence; /// Sequence of ScriptOperations.
    size_t  current_step;               /// The ScriptOperation in the sequence
                                        /// that is currently executed.
    bool loop,is_active,is_applicable_when_dead;
    float need_decay_rate;  // need lessens while performing behavior
    float need_growth_rate; // need grows while not performing behavior
    float completion_decay; // need lessens AFTER behavior script is run once
                            // Use -1 to remove all need
    float init_need;        // starting need, also used in ClearState resets
    csString name;
    bool  resume_after_interrupt; // Resume at active step after interrupt.
        
    float current_need;
    float new_need;
    csTicks last_check;
    bool  interrupted;
    
public:
    Behavior();
    Behavior(const char *n);
    Behavior(Behavior& other) { DeepCopy(other); }
    
    virtual ~Behavior() { };

    const char *GetName() { return name; }

    void DeepCopy(Behavior& other);
    bool Load(iDocumentNode *node);

    bool LoadScript(iDocumentNode *node,bool top_level=true);

    void  Advance(csTicks delta,NPC *npc,EventManager *eventmgr);
    float CurrentNeed()
    { return current_need; }
    float NewNeed()
    { return new_need; }
    void  CommitAdvance()
    {
        if (new_need!=-999)
        {
            current_need = new_need;
        }
    }
    void ApplyNeedDelta(float delta_desire)
    {
        if (new_need==-999)
        {
            new_need = current_need;
        }
        new_need += delta_desire;
    }
    void ApplyNeedAbsolute(float absolute_desire)
    {
        new_need = absolute_desire;
    }
    void SetActive(bool flag)
    { is_active = flag; }
    bool GetActive()
    { return is_active; }
    void SetCurrentStep(int step) { current_step = step; }
    size_t GetCurrentStep(){ return current_step; }
    void ResetNeed() { current_need = new_need = init_need; }

    bool ApplicableToNPCState(NPC *npc);
    bool StartScript(NPC *npc,EventManager *eventmgr);
    bool RunScript(NPC *npc,EventManager *eventmgr,bool interruped);
    bool ResumeScript(NPC *npc,EventManager *eventmgr);
    void InterruptScript(NPC *npc,EventManager *eventmgr);
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
    static void GetPosition(gemNPCObject* object, csVector3& pos, float& yrot,iSector*& sector);
    static void SetPosition(gemNPCObject* objecty, csVector3& pos, iSector* = NULL);
    static void SetRotationAngle(gemNPCObject* object, float angle);
    static void GetRotationAngle(gemNPCObject* object, float& yrot)
    {
        csVector3 pos;
        iSector *sector;
        GetPosition(object,pos,yrot,sector);
    }
    static float CalculateIncidentAngle(csVector3& pos, csVector3& dest);
    
    // Clamp the angle within 0 to 2*PI
    static void ClampRadians(float &target_angle);
    
    // Normalize angle within -PI to PI
    static void NormalizeRadians(float &target_angle);

};


#endif

