/*
* perceptions.h
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
#ifndef __PERCEPTIONS_H__
#define __PERCEPTIONS_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/matrix3.h>
#include <csutil/weakref.h>
#include <iutil/document.h>
#include <csutil/array.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/prb.h"
#include "util/psconst.h"
#include "net/npcmessages.h"
#include "gem.h"

class ScriptOperation;
class Perception;
class Behavior;
class NPC;
class EventManager;
class BehaviorSet;
class Location;
struct iSector;
class gemNPCActor;
class gemNPCItem;


/**
* A reaction embodies the change in desire which occurs
* in an NPC when he perceives something.
*/
class Reaction
{
protected:
    // members making up the "if statement"
    csString  event_type;
    float     range;
    int       faction_diff;
    csString  oper;
    bool      active_only;
    bool      inactive_only;
    bool      react_when_dead;
    bool      react_when_invisible;
    bool      react_when_invincible;
    csArray<bool> valuesValid;
    csArray<int> values;
    csArray<bool>randomsValid;
    csArray<int> randoms;
    csString  type;
    csArray<csString> only_interrupt;

    // members making up the "then statement"
    csArray<Behavior*> affected;

    enum Desire_Type
    {
        DESIRE_NONE,
        DESIRE_DELTA,
        DESIRE_ABSOLUTE,
        DESIRE_GUARANTIED
    };
    
    Desire_Type desireType; // Indicate the type of desire change this reaction has.
    float desireValue;        // The value to use for the desire type.
    float weight;             // The weight to apply to deltas.

public:
    Reaction();
    Reaction(Reaction& other,BehaviorSet& behaviors) 
    { DeepCopy(other,behaviors); }

    void DeepCopy(Reaction& other,BehaviorSet& behaviors);

    bool Load(iDocumentNode *node,BehaviorSet& behaviors);
    void React(NPC *who,Perception *pcpt);

    // Caled by perception related to entitis to check
    // for reaction against visibility and invincibility
    bool ShouldReact(gemNPCObject* entity, Perception *pcpt);

    const char     *GetEventType()      { return event_type;   }
    float           GetRange()          { return range;        }
    int             GetFactionDiff()    { return faction_diff; }
    bool            GetValueValid(int i);
    int             GetValue(int i);
    bool            GetRandomValid(int i);
    int             GetRandom(int i);
    const csString& GetType()           { return type;         }
    char            GetOp()             { if (oper.Length()) return oper.GetAt(0); else return 0; }
};

//-----------------------------------------------------------------------------

/**
* This embodies any perception an NPC might have,
* or any game event of interest.  Reaction objects
* below will subscribe to these events, and the
* networking system will publish them.  Examples
* would be "attacked", "collided", "talked to",
* "hear cry".
*/
class Perception
{
protected:
    /// The name of this perception.
    csString name;

    /// Values used by perseptions. Usally they correspond to the same value in a reaction.  
    csString  type;

public:
    Perception(const char *n) { name = n; }
    Perception(const char *n, const char *t) { name = n; type = t; }
    virtual ~Perception() {}


    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight) { }
  
    virtual gemNPCObject *GetTarget() { return NULL; }

    const char* GetName() { return name; }
    const char* GetType() { return type; }
    void SetType(const char* type) { this->type = type; }
    virtual bool GetLocation(csVector3& pos, iSector*& sector) { return false; }
    virtual float GetRadius() const { return 0.0; }

    virtual csString ToString();
};

//-----------------------------------------------------------------------------

class RangePerception : public Perception
{
protected:
    float range;

public:
    RangePerception(const char *n,float r)
    : Perception(n), range(r)  {    }
    virtual ~RangePerception() {}

    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual csString ToString();
};

//-----------------------------------------------------------------------------

class TimePerception : public Perception
{
protected:
    int gameHour,gameMinute,gameYear,gameMonth,gameDay;

public:
    TimePerception(int hour, int minute, int year, int month, int day)
    : Perception("time"), gameHour(hour),gameMinute(minute),gameYear(year),gameMonth(month),gameDay(day)  {    }
    virtual ~TimePerception() {}

    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual csString ToString();
};

//-----------------------------------------------------------------------------

class FactionPerception : public Perception
{
protected:
    int faction_delta;
    csWeakRef<gemNPCActor> player;

public:
    FactionPerception(const char *n,int d,gemNPCObject *p)
    : Perception(n), faction_delta(d), player((gemNPCActor *)p)  {    }
    virtual ~FactionPerception() {}

    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual bool GetLocation(csVector3& pos, iSector*& sector);
    virtual gemNPCObject *GetTarget() { return player; }
    virtual void ExecutePerception(NPC *npc,float weight);
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPC is close to an item, this perception is
 * passed to the npc.
 */
class ItemPerception : public Perception
{
protected:
    csWeakRef<gemNPCObject> item;

public:
    ItemPerception(const char *n,gemNPCObject *i)
    : Perception(n), item(i)  {    }
    virtual ~ItemPerception() {}

    virtual Perception *MakeCopy();
    virtual bool GetLocation(csVector3& pos, iSector*& sector);
    virtual gemNPCObject *GetTarget() { return item; }
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPC is close to an location, this perception is
 * passed to the npc.
 */
class LocationPerception : public Perception
{
protected:
    Location *location;

public:
    LocationPerception(const char *n, const char*type, Location * location, iEngine* engine)
    : Perception(n,type), location(location), engine(engine)  {}
    virtual ~LocationPerception() {}

    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual bool GetLocation(csVector3& pos, iSector*& sector);
    virtual float GetRadius() const;
    
private:
    iEngine* engine;
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPC is attacked by a player, this perception is
 * passed to the attacked npc.  The npc should use this to build
 * an initial hate list for the player and his group (if any).
 * If the designer wants to cheat, he can start attacking back
 * on this event instead of on first damage, and save a few seconds
 * in response time.
 */
class AttackPerception : public Perception
{
protected:
    csWeakRef<gemNPCActor> attacker;

public:
    AttackPerception(const char *n,gemNPCObject *attack)
        : Perception(n), attacker((gemNPCActor*) attack) { }

    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight);
    virtual gemNPCObject *GetTarget() { return attacker; }
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPC is attacked by a grouop, this perception is
 * passed to the attacked npc.  The npc should use this to build
 * an initial hate list for the player group.
 * If the designer wants to cheat, he can start attacking back
 * on this event instead of on first damage, and save a few seconds
 * in response time.
 */
class GroupAttackPerception : public Perception
{
protected:
    csWeakRef<gemNPCObject> attacker;
    csArray<gemNPCObject *> attacker_ents;
    csArray<int> bestSkillSlots;

public:
    GroupAttackPerception(const char *n,csArray<gemNPCObject *> & ents, csArray<int> & slots)
        : Perception(n), attacker_ents(ents), bestSkillSlots(slots) { }

    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight);
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPC is hit for damage by a melee hit or a spell,
 * this perception is passed to the damaged npc.  Right now,
 * it affects his/her hate list according to the weight supplied.
 */
class DamagePerception : public Perception
{
protected:
    csWeakRef<gemNPCActor> attacker;
    float damage;

public:
    DamagePerception(const char *n,gemNPCObject *attack,float dmg)
        : Perception(n), attacker((gemNPCActor*)attack), damage(dmg) { }

    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight);
};

//-----------------------------------------------------------------------------

/**
 * Whenever a player casts a spell on himself, another player,
 * or any npc, this perception is passed to npc's in the vicinity.
 * We cannot only use targeted NPCs because an npc in combat may
 * want to react to a player healing his group mate, which helps
 * his team and hurts the NPC indirectly.  Right now, reactions
 * are allowed by type of spell, and depending on the severity,
 * it affects his/her hate list according to the weight supplied.
 */
class SpellPerception : public Perception
{
protected:
    csWeakRef<gemNPCActor> caster;
    csWeakRef<gemNPCActor> target;
    float       spell_severity;

public:
    SpellPerception(const char *name,gemNPCObject *caster,gemNPCObject *target, const char *spell_type, float severity);

    virtual bool ShouldReact(Reaction *reaction,NPC *npc);
    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight);
};

//-----------------------------------------------------------------------------

/**
 * The network layer is notified of any deaths from players
 * or NPCs on the server.  This perception is passed to all
 * NPCs so they can react, by removing the player from hate
 * lists, stopping their combat or whatever.
 */
class DeathPerception : public Perception
{
protected:
    EID who;

public:
    DeathPerception(EID ent_id)
        : Perception("death"), who(ent_id) { }

    virtual Perception *MakeCopy();
    virtual void ExecutePerception(NPC *npc,float weight);
};

//-----------------------------------------------------------------------------

/**
 * This perception is used when a item is added or removed from inventory.
 */
class InventoryPerception : public Perception
{
protected:
    csVector3 pos;
    iSector * sector;
    float     radius;
    int       count;

public:
    InventoryPerception(const char *n,const char *t,const int c, const csVector3& p, iSector* s, float r)
        : Perception(n), pos(p), radius(r), count(c) { type = t; sector = s; }

    virtual Perception *MakeCopy();
    virtual bool GetLocation(csVector3& pos, iSector*& sector)
        { pos = this->pos; sector = this->sector; return true; }

    virtual float GetRadius() const { return radius; }
    virtual int GetCount() const { return count; }
    
};

//-----------------------------------------------------------------------------

/// NPC Pet Perceptions ============================================
/**
 * Whenever an NPCPet is told by it's owner to stay,
 * this perception is passed to the NPCPet.  Right now,
 * it changes the current behavior of the NPCPet.
 */
class OwnerCmdPerception : public Perception
{
protected:
    psPETCommandMessage::PetCommand_t command;
    csWeakRef<gemNPCObject> owner;
    csWeakRef<gemNPCObject> pet;
    csWeakRef<gemNPCActor> target;

public:
    OwnerCmdPerception(const char *n, psPETCommandMessage::PetCommand_t command, gemNPCObject *owner, gemNPCObject *pet, gemNPCObject *target);

    virtual bool ShouldReact( Reaction *reaction, NPC *pet );
    virtual Perception *MakeCopy();
    virtual void ExecutePerception( NPC *pet, float weight );
    virtual gemNPCObject *GetTarget() { return target; }
    static csString BuildName(psPETCommandMessage::PetCommand_t command);
};

//-----------------------------------------------------------------------------

/**
 * Whenever an NPCPet is told by it's owner to stay,
 * this perception is passed to the NPCPet.  Right now,
 * it changes the current behavior of the NPCPet.
 */
class OwnerActionPerception : public Perception
{
protected:
    int action;
    csWeakRef<gemNPCObject> owner;
    csWeakRef<gemNPCObject> pet;

public:
    OwnerActionPerception(const char *n, int action, gemNPCObject *owner, gemNPCObject *pet );

    virtual bool ShouldReact( Reaction *reaction, NPC *pet );
    virtual Perception *MakeCopy();
    virtual void ExecutePerception( NPC *pet, float weight );
};

//-----------------------------------------------------------------------------

/// NPC Pet Perceptions ============================================

/**
 * Whenever an NPC Cmd is received this perception is fired.
 * This is most typicaly from a response script like:
 * <npccmd cmd="test_cmd"/>
 */
class NPCCmdPerception : public Perception
{
protected:
    csString cmd;
    NPC * self;

public:
    NPCCmdPerception(const char *command, NPC * self);

    virtual bool ShouldReact( Reaction *reaction, NPC *npc );
    virtual Perception *MakeCopy();
};

#endif

