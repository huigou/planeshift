/*
 * psattack.cpp              author: hartra344 [at] gmail [dot] com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef psAttack_HEADER
#define psAttack_HEADER
//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>
//====================================================================================
// Project Includes
//====================================================================================
#include <idal.h>
#include "util/scriptvar.h"
#include "util/gameevent.h"


//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"
#include "psitemstats.h"
#include "psitem.h"
#include "deleteobjcallback.h"

using namespace CS;

class psQuestPrereqOp;
class psCharacter;
class psAttack;
class Client;
class gemObject;
class gemActor;
class ProgressionScript;
class MathExpression;
class CombatManager;
struct Stance;
class psCombatAttackGameEvent;

/** This stuct holds data for generic attack types
 * This could be for example an assassin attack which would require maybe daggers. and require dual wielded daggers at that, so each dagger does part of the attack specifically.
 * each attack type would also be based off a specic skill, such as strength or agility.
 */
struct psAttackType
{
    unsigned int           id;/// the id number, corresponding to its id in the database
    csString               name;/// the name of the attack type
    csString               weapon;/// if it requires 1 specific weapon, maybe its an attack that is special adn specific to 1 particular weapon.
    csArray<psWeaponType*> weaponTypes;/// if it requires multiple types of weapons they would be listed here.
    bool                   OneHand;/// if it is a dual weilding based attack it willb e flagged here, that means it requires 2 weapons and each will perform part of the attack, if it is false then only one hand will execute the attack.
    PSSKILL                related_stat; /// each attack will have a stat related to it that will be part of the power computation

};


struct psAttackCost// may be expanded later if more balance is needed
{
    float physStamina;
};


/**
 * Represents an Attack.  This is mostly data that is cached in from the
 * database to represent what a attack is. It contains details such as the
 * required glyphs as well as the effect of the spell.
 *
 * This is going to be very close to how a spell works but more in touch with melee adn range style.
 */
class psAttack :  public csRefCount
{
public:

    virtual bool Load(iResultRow &row) = 0;


    /**
     * Calls an attack.
     */
    virtual bool Attack(gemObject* attacker,gemObject* target,INVENTORY_SLOT_NUMBER slot)  = 0;
    /**
     * Once the combat event is called, this meathod runs preattack checks and runs all calculations needed before the combat event is applied
     */
    virtual void Affect(psCombatAttackGameEvent* event)  = 0;
    /**
     * this is just a check to see if the attack can be used by the character
     */
    virtual bool CanAttack(Client* client) = 0;


    /** gets the id
     * @return the attack id
     */
    int GetID() const
    {
        return id;
    }
    /** Gets the Name
     * @return returns the name of attack
     */
    const csString &GetName() const
    {
        return name;
    }
    /** Gets the attack icon string
     *@return attack icon image string
     */
    const csString &GetImage() const
    {
        return image;
    }
    /** Gets the attack description
     *  @return attack description
     */
    const csString &GetDescription() const
    {
        return description;
    }

    const float GetSpeed() const
    {
        return speed;
    }

    psAttackType* GetType() const
    {
        return type;
    }
    virtual bool IsDualWield(psCharacter* attacker) = 0;

    /**
     * says if the attack has been queued from the UI as special attack
     */
    virtual bool IsQueuedInClient() = 0;

protected:

    int id; ///< The stored ID of the attack
    csString name; ///< The stored name of the attack
    csString image; ///< the address/id of the icon for the attack
    csString description;///<the attack description
    psAttackType* type; ///< the attack type
    float speed;  ///< the speed of the attack
    //I am trying to figure out which members/variables should be given to "psattack" and which ones to only the children. so this will
    // likely change a lot more

};

//-----------------------------------------------------------------------------

/**
 * This event actually triggers an attack
 */
class psCombatAttackGameEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    csWeakRef<gemObject>  attacker;  ///< Entity who instigated this attack
    csWeakRef<gemObject>  target;    ///< Entity who is target of this attack
    psCharacter* attackerdata;       ///< the attackers data
    psCharacter* targetdata;         ///< the targets data
    int TargetCID;                   ///< ClientID of target
    int AttackerCID;                 ///< ClientID of attacker

    float MaxRange; ///the maximum range the attack can reach

    psAttack* attack;  ///< The attack

    INVENTORY_SLOT_NUMBER WeaponSlot; ///< Identifier of the slot for which this attack event should process
    psItem* weapon;                   ///< the attacking weapon

    INVENTORY_SLOT_NUMBER AttackLocation;  ///< Which slot should we check the armor of?

    float FinalDamage;               ///< Final damage applied to target

    int   AttackResult;              ///< Code indicating the result of the attack attempt
    int   PreviousAttackResult;      ///< The code of the previous result of the attack attempt

    float max_range;                 ///< the maximum range of the attack
    float powerLevel;                ///< the pwoer of the attack to be used in the final damage formula

    int attackAnim;                  ///< the attackers animation
    int TargetAnim;                  ///< the targets animation, likely always going to be "hit" but could be something else...

    psCombatAttackGameEvent(int delayticks,
                            psAttack* attack,
                            gemObject* attacker,
                            gemObject* target,
                            INVENTORY_SLOT_NUMBER weaponslot,
                            int attackerCID,
                            int targetCID);
    ~psCombatAttackGameEvent();

    virtual void Trigger();  // Abstract event processing function
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    gemObject* GetTarget()
    {
        return target;
    };
    gemObject* GetAttacker()
    {
        return attacker;
    };
    psCharacter* GetTargetData()
    {
        return targetdata;
    };
    psCharacter* GetAttackerData()
    {
        return attackerdata;
    };
    INVENTORY_SLOT_NUMBER GetWeaponSlot()
    {
        return WeaponSlot;
    };


    int GetTargetID()
    {
        return TargetCID;
    };
    int GetAttackerID()
    {
        return AttackerCID;
    };
    int GetAttackResult()
    {
        return AttackResult;
    };

};
#endif
