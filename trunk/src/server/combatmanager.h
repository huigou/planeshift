/*
 * combatmanager.h
 *
 * Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef __COMBATMANAGER_H__
#define __COMBATMANAGER_H__

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"


#define SECONDS_BEFORE_SPARING_DEFEATED 30

class MathScriptVar;
class psCombatGameEvent;

enum COMBATMANAGER_ATTACKTYPE
{
    ATTACK_NOTCALCULATED = -1,
    ATTACK_DAMAGE,
    ATTACK_DODGED,
    ATTACK_BLOCKED,
    ATTACK_MISSED,
    ATTACK_OUTOFRANGE,
    ATTACK_BADANGLE,
    ATTACK_OUTOFAMMO
};

class LocationType;
class MathScriptEngine;
class MathScript;

/**
 *  This class handles all calculations around combat, using statistics
 *  and information from the pspccharacterinfo Prop Classes for both
 *  the attacker and the target.
 */
class CombatManager: public MessageManager
{
public:

    CombatManager();
    bool InitializePVP();

    virtual ~CombatManager();

    void HandleMessage(MsgEntry *me, Client *client) { }

    /// This is how you start an attack sequence
    void AttackSomeone(gemActor *attacker, gemObject *target, Stance stance);

    /// This is how you break an attack sequence off, through death or user command.
    void StopAttack(gemActor *attacker);

    bool InPVPRegion(csVector3& pos, iSector* sector);

    void HandleCombatEvent(psCombatGameEvent *event);

    /***********************
     * Not implemented yet *
     ***********************
     int  GetQueuedAction(gemActor *attacker);
     int  GetDefaultModeAction(gemActor *attacker);
     int  GetAttackDelay(gemActor *attacker, int action);*/

    csArray<INVENTORY_SLOT_NUMBER> targetLocations;

private:
    //    psSpawnManager *spawnmanager;
    csRandomGen* randomgen;
    LocationType* pvp_region;

    MathScriptEngine *script_engine; /// Scripting engine handles all RPG calculations.
    MathScript *calc_damage; /// This is the particular calculation for damage.
    MathScriptVar *var_IAH; /// IAH == If Attack Hit
    MathScriptVar *var_AHR; /// AHR == Attack Hit Roll
    MathScriptVar *var_Blocked; /// Blocked == Blocked by weapon
    MathScriptVar *var_QOH; /// QOH == Quality Of Hit
    MathScriptVar *var_FinalDmg; /// Actual Damage done, if any
    MathScriptVar *var_AttackWeapon;
    MathScriptVar *var_AttackWeaponSecondary;
    MathScriptVar *var_TargetAttackWeapon;
    MathScriptVar *var_DefenseWeaponSecondary;
    MathScriptVar *var_Target;
    MathScriptVar *var_Attacker;
    MathScriptVar *var_AttackLocationItem;

    // if the player is too tired, stop fighting. We stop if we don't have enough stamina to make an attack with the current stance.
    MathScript* staminacombat;
    // Output
    MathScriptVar* PhyDrain;
    MathScriptVar* MntDrain;
    // Input
    MathScriptVar* actorVar;
    MathScriptVar* weaponVar;

    void HandleDeathEvent(MsgEntry *me,Client *client);

    bool ValidDistance(gemObject *attacker, gemObject *target, psItem *Weapon);
    void SetCombat(gemActor *combatant, Stance stance);

    bool ValidCombatAngle(gemObject *attacker, gemObject *target,
            psItem *Weapon);
    void NotifyTarget(gemActor *attacker, gemObject *target);
    void QueueNextEvent(psCombatGameEvent *event);
    void QueueNextEvent(gemObject *attacker, INVENTORY_SLOT_NUMBER weaponslot,
            gemObject *target, int attackerCID, int targetCID,
            int previousResult = ATTACK_NOTCALCULATED);

    void ApplyCombatEvent(psCombatGameEvent *event, int attack_result);
    void DebugOutput(psCombatGameEvent *event);
    int CalculateAttack(psCombatGameEvent *event, psItem* subWeapon = NULL);
};

class psSpareDefeatedEvent: public psGameEvent
{
public:
    psSpareDefeatedEvent(gemActor *losr);
    void Trigger();

protected:
    csWeakRef<Client> loser;

};

#endif
