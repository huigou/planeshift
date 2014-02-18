/*
 * psAttackDefault.cpp              creator hartra344@gmail.com
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


#include <psconfig.h>
#include "psattackdefault.h"
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <csgeom/math.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/mathscript.h"
#include "util/psdatabase.h"

#include "psserver.h"
#include "gem.h"
#include "client.h"
#include "cachemanager.h"
#include "entitymanager.h"
#include "commandmanager.h"
#include "progressionmanager.h"
#include "combatmanager.h"
#include "npcmanager.h"
#include "../globals.h"
#include "scripting.h"
#include "netmanager.h"
#include "psserverchar.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psquestprereqops.h"

psAttackDefault::psAttackDefault()
{
    psserver->GetMathScriptEngine()->CheckAndUpdateScript(calc_damage, "Calculate Damage");
    psserver->GetMathScriptEngine()->CheckAndUpdateScript(calc_decay, "Calculate Decay");
    psserver->GetMathScriptEngine()->CheckAndUpdateScript(staminacombat, "StaminaCombat");
    if(!calc_damage)
    {
        Error1("Calculate Damage Script could not be found.  Check the math_scripts DB table.");
    }
    else if(!calc_decay)
    {
        Error1("Calculate Decay Script could not be found.  Check the math_scripts DB table.");
    }
}

psAttackDefault::~psAttackDefault()
{

}

bool psAttackDefault::IsDualWield(psCharacter* attacker)
{
    int count = 0;
    //default being a dual wield depends on whether or not the character has 2 weapons or not, it is supposed to be flexible
    for(int slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
    {
        // See if this slot is able to attack
        if(attacker->Inventory().CanItemAttack((INVENTORY_SLOT_NUMBER) slot))
        {
            count++;
        }
        if(count > 1) //if it's greater than 1 it is a dualwield attack
            break;
    }
    if(count > 1)
        return true;
    else
        return false;
}
bool psAttackDefault::Load(iResultRow &row)
{
    //no need to load anything for this particular attack
    return true;
}
bool psAttackDefault::CanAttack(Client* client)
{
    //default attack can always attack
    return true;
}

bool psAttackDefault::Attack(gemObject* attacker, gemObject* target,INVENTORY_SLOT_NUMBER slot)
{
    //queue up the attack
    QueueAttack(attacker,slot,target,attacker->GetClientID(),target->GetClientID());
    return true;
}
void psAttackDefault::QueueAttack(gemObject* attacker,INVENTORY_SLOT_NUMBER weaponslot,gemObject* target,int attackerCID, int targetCID)
{
    psCharacter* Character=attacker->GetCharacterData();
    psItem* Weapon=Character->Inventory().GetEffectiveWeaponInSlot(weaponslot);
    float latency = Weapon->GetLatency();
    int delay = (int)(latency*1000);


    psCombatAttackGameEvent* event;
    event = new psCombatAttackGameEvent(delay,this,attacker,target,weaponslot,attackerCID,targetCID);
    event->GetAttackerData()->TagEquipmentObject(weaponslot,event->id);
    psserver->GetEventManager()->Push(event);

}
void psAttackDefault::Affect(psCombatAttackGameEvent* event)
{

    psCharacter* attacker_data;
    int attack_result;
    bool skipThisRound = false;

    if(!event->GetAttacker() || !event->GetTarget())  // disconnected and deleted
    {
        Debug2(LOG_COMBAT,0,"Attacker ID: %d. Combat stopped as one participant logged off.",event->AttackerCID);
        return;
    }

    // get attacker and target objects
    gemActor* gemAttacker = dynamic_cast<gemActor*>((gemObject*) event->attacker);
    gemActor* gemTarget   = dynamic_cast<gemActor*>((gemObject*) event->target);

    attacker_data=event->GetAttackerData();

    // If the attacker is no longer in attack mode abort.
    if(gemAttacker->GetMode() != PSCHARACTER_MODE_COMBAT)
    {
        Debug2(LOG_COMBAT,0,"Combat stopped as attacker %d left combat mode.",event->AttackerCID);
        return;
    }

    // If target is dead, abort.
    if(!gemTarget->IsAlive())
    {
        Debug2(LOG_COMBAT,0,"Combat stopped as one participant logged off. Attacker ID: %d",event->AttackerCID);
        return;
    }

    // If the slot is no longer attackable, abort
    if(!attacker_data->Inventory().CanItemAttack(event->GetWeaponSlot()))
    {
        Debug2(LOG_COMBAT,0,"Combat stopped as attacker has no longer an attacking item equipped. Attacker ID: %d",event->AttackerCID);
        return;
    }

    psItem* weapon = attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot());

    // weapon became unwieldable
    csString response;
    if(weapon!=NULL && !weapon->CheckRequirements(attacker_data,response))
    {
        Debug2(LOG_COMBAT, gemAttacker->GetClientID(),"%s has lost use of weapon", gemAttacker->GetName());
        psserver->SendSystemError(event->AttackerCID, "You can't use your %s any more.", weapon->GetName());
        return;
    }

    //If the weapon in the slot has been changed, skip a turn (latency for this slot may also have changed)
    if(event->weapon->GetUID() != weapon->GetUID())
    {
        Debug2(LOG_COMBAT, gemAttacker->GetClientID(),"Skipping attack because %s has changed weapons mid battle", gemAttacker->GetName());
        skipThisRound = true;
    }

    Client* attacker_client = psserver->GetNetManager()->GetClient(event->AttackerCID);

    if(attacker_client)
    {
        // Input the stamina data
        MathEnvironment env;
        env.Define("Actor",  event->GetAttacker());
        env.Define("Weapon", weapon);

        //as this is called frequently we precheck before doing a function
        //call if we need to reload the script
        if(!staminacombat.IsValid())
        {
            psserver->GetMathScriptEngine()->CheckAndUpdateScript(staminacombat, "StaminaCombat");
        }
        //this is going to crash if the script was not found in the db.
        staminacombat->Evaluate(&env);

        MathVar* PhyDrain = env.Lookup("PhyDrain");
        MathVar* MntDrain = env.Lookup("MntDrain");

        // stop the attack if the attacker has no stamina left
        if((attacker_client->GetCharacterData()->GetStamina(true) < PhyDrain->GetValue())
                || (attacker_client->GetCharacterData()->GetStamina(false) < MntDrain->GetValue()))
        {
            psserver->GetCombatManager()->StopAttack(attacker_data->GetActor());
            psserver->SendSystemError(event->AttackerCID, "You are too tired to attack.");
            return;
        }

        // If the target has become impervious, abort and give up attacking
        csString msg;
        if(!gemAttacker->IsAllowedToAttack(gemTarget,msg))
        {
            psserver->GetCombatManager()->StopAttack(attacker_data->GetActor());
            return;
        }

        //I will be seeing how this affects things and make changes accordingly
        // If the target has changed, abort (assume another combat event has started since we are still in attack mode)
        if(gemTarget != attacker_client->GetTargetObject())
        {
            Debug2(LOG_COMBAT,0,"Skipping attack, Target changed for attacker ID: %d.",event->AttackerCID);
            return;
        }
    }
    else
    {
        // Check if the npc's target has changed (if it has, then assume another combat event has started.)
        gemNPC* npcAttacker = dynamic_cast<gemNPC*>(gemAttacker);
        if(npcAttacker && npcAttacker->GetTarget() != gemTarget)
        {
            Debug2(LOG_COMBAT,0,"Skipping attack, Target changed for attacker ID: %d.",event->AttackerCID);
            return;
        }
    }


    if(gemAttacker->IsSpellCasting())
    {
        psserver->SendSystemInfo(event->AttackerCID, "You can't attack while casting spells.");
        skipThisRound = true;
    }

    if(!skipThisRound)
    {
        if(weapon->GetIsRangeWeapon() && weapon->GetUsesAmmo())
        {
            INVENTORY_SLOT_NUMBER otherHand = event->GetWeaponSlot() == PSCHARACTER_SLOT_RIGHTHAND ?
                                              PSCHARACTER_SLOT_LEFTHAND:
                                              PSCHARACTER_SLOT_RIGHTHAND;

            attack_result = ATTACK_NOTCALCULATED;

            psItem* otherItem = attacker_data->Inventory().GetInventoryItem(otherHand);
            if(otherItem == NULL)
            {
                attack_result = ATTACK_OUTOFAMMO;
            }
            else if(otherItem->GetIsContainer())  // Is it a quiver?
            {
                // Pick the first ammo we can shoot from the container
                // And set it as the active ammo
                bool bFound = false;
                for(size_t i=1; i<attacker_data->Inventory().GetInventoryIndexCount() && !bFound; i++)
                {
                    psItem* currItem = attacker_data->Inventory().GetInventoryIndexItem(i);
                    if(currItem &&
                            currItem->GetContainerID() == otherItem->GetUID() &&
                            weapon->GetAmmoTypeID().In(currItem->GetBaseStats()->GetUID()))
                    {
                        otherItem = currItem;
                        bFound = true;
                    }
                }
                if(!bFound)
                    attack_result = ATTACK_OUTOFAMMO;
            }
            else if(!weapon->GetAmmoTypeID().In(otherItem->GetBaseStats()->GetUID()))
            {
                attack_result = ATTACK_OUTOFAMMO;
            }

            if(attack_result != ATTACK_OUTOFAMMO)
            {
                psItem* usedAmmo = attacker_data->Inventory().RemoveItemID(otherItem->GetUID(), 1);
                if(usedAmmo)
                {
                    attack_result=CalculateAttack(event, usedAmmo);
                    usedAmmo->Destroy();
                    psserver->GetCharManager()->UpdateItemViews(attacker_client->GetClientNum());
                }
                else
                    attack_result=CalculateAttack(event);
            }

        }
        else
        {
            attack_result=CalculateAttack(event);
        }

        event->AttackResult=attack_result;
        AffectTarget(event,attack_result);
    }

}

int psAttackDefault::CalculateAttack(psCombatAttackGameEvent* event, psItem* subWeapon)
{
    INVENTORY_SLOT_NUMBER otherHand = event->GetWeaponSlot() == PSCHARACTER_SLOT_LEFTHAND ? PSCHARACTER_SLOT_RIGHTHAND : PSCHARACTER_SLOT_LEFTHAND;
    event->AttackLocation = (INVENTORY_SLOT_NUMBER) psserver->GetCombatManager()->targetLocations[psserver->rng->Get((int) psserver->GetCombatManager()->targetLocations.GetSize())];

    gemObject* attacker = event->GetAttacker();
    gemObject* target = event->GetTarget();

    // calculate difference between target and attacker location - to be used for angle validation
    csVector3 diff(0); // initialize to some big value that shows an error

    {
        csVector3 attackPos, targetPos;
        iSector* attackSector, *targetSector;

        attacker->GetPosition(attackPos, attackSector);
        target->GetPosition(targetPos, targetSector);

        if((attacker->GetInstance() != target->GetInstance() &&
                attacker->GetInstance() != INSTANCE_ALL && target->GetInstance() != INSTANCE_ALL) ||
                !(psserver->GetCombatManager()->GetEntityManager()->GetWorld()->WarpSpace(targetSector, attackSector, targetPos)))
        {
            return ATTACK_OUTOFRANGE;
        }
        diff = targetPos - attackPos;
    }

    MathEnvironment env;
    env.Define("Attacker",              attacker);
    env.Define("Target",                target);
    env.Define("AttackWeapon",          event->GetAttackerData()->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot()));
    env.Define("AttackWeaponSecondary", subWeapon);
    env.Define("TargetWeapon",          event->GetTargetData()->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot(), true));
    env.Define("TargetWeaponSecondary", event->GetTargetData()->Inventory().GetEffectiveWeaponInSlot(otherHand,true));
    env.Define("AttackLocationItem",    event->GetTargetData()->Inventory().GetEffectiveArmorInSlot(event->AttackLocation));
    env.Define("DiffX",                 diff.x ? diff.x : 0.00001F); // force minimal value
    env.Define("DiffY",                 diff.y ? diff.y : 0.00001F); // force minimal value
    env.Define("DiffZ",                 diff.z ? diff.z : 0.00001F); // force minimal value

    //as this is called frequently we precheck before doing a function
    //call if we need to reload the script
    if(!calc_damage.IsValid())
    {
        psserver->GetMathScriptEngine()->CheckAndUpdateScript(calc_damage, "Calculate Damage");
    }

    //this is going to crash if the script cannot be found.
    calc_damage->Evaluate(&env);

    if(DoLogDebug2(LOG_COMBAT, event->GetAttackerData()->GetPID().Unbox()))
    {
        CPrintf(CON_DEBUG, "Variables for Calculate Damage:\n");
        env.DumpAllVars();
    }

    MathVar* badrange = env.Lookup("BadRange");    // BadRange = Target is too far away
    MathVar* badangle = env.Lookup("BadAngle");    // BadAngle = Attacker doesn't aim at enemy
    MathVar* missed   = env.Lookup("Missed");      // Missed   = Attack missed the enemy
    MathVar* dodged   = env.Lookup("Dodged");      // Dodged   = Attack dodged  by enemy
    MathVar* blocked  = env.Lookup("Blocked");     // Blocked  = Attack blocked by enemy
    MathVar* damage   = env.Lookup("FinalDamage"); // Actual damage done, if any

    if(badrange && badrange->GetValue() < 0.0)
        return ATTACK_OUTOFRANGE;
    else if(badangle && badangle->GetValue() < 0.0)
        return ATTACK_BADANGLE;
    else if(missed && missed->GetValue() < 0.0)
        return ATTACK_MISSED;
    else if(dodged && dodged->GetValue() < 0.0)
        return ATTACK_DODGED;
    else if(blocked && blocked->GetValue() < 0.0)
        return ATTACK_BLOCKED;

    event->FinalDamage = damage->GetValue();

    return ATTACK_DAMAGE;
}
void psAttackDefault::AffectTarget(psCombatAttackGameEvent* event, int attack_result)
{
    psCharacter* attacker_data = event->GetAttackerData();
    psCharacter* target_data=event->GetTargetData();

    MathVar* weaponDecay = NULL;
    MathVar* blockDecay = NULL;
    MathVar* armorDecay = NULL;
    MathEnvironment env;

    psItem* weapon         = attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot());
    psItem* blockingWeapon = target_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot(),true);
    psItem* struckArmor    = target_data->Inventory().GetEffectiveArmorInSlot(event->AttackLocation);

    // there may only be a decay if you actually hit your target by some means
    if(attack_result == ATTACK_DAMAGE || attack_result == ATTACK_BLOCKED)
    {
        // we are guaranteed some armor is present - real one, race one or base one
        CS_ASSERT(struckArmor);
        float ArmorVsWeapon = weapon->GetArmorVSWeaponResistance(struckArmor->GetBaseStats());

        // clamp value between 0 and 1
        ArmorVsWeapon = ArmorVsWeapon > 1.0F ? 1.0F : ArmorVsWeapon < 0.0F ? 0.0F : ArmorVsWeapon;

        env.Define("Weapon", weapon);                             // weapon that was used to attack
        env.Define("BlockingWeapon", blockingWeapon);             // weapon that blocked the attack
        env.Define("Armor", struckArmor);                         // armor hit
        env.Define("ArmorVsWeapon", ArmorVsWeapon);               // armor vs weapon effectiveness
        env.Define("Damage", event->FinalDamage);                 // actual damage dealt
        env.Define("Blocked", (attack_result == ATTACK_BLOCKED)); // identifies whether this attack was blocked

        //as this is called frequently we precheck before doing a function
        //call if we need to reload the script
        if(!calc_decay.IsValid())
        {
            psserver->GetMathScriptEngine()->CheckAndUpdateScript(calc_decay, "Calculate Decay");
        }

        //this is going to crash if the script cannot be found.
        calc_decay->Evaluate(&env);

        weaponDecay = env.Lookup("WeaponDecay");
        blockDecay  = env.Lookup("BlockingDecay");
        armorDecay  = env.Lookup("ArmorDecay");
    }

    gemActor* gemAttacker = dynamic_cast<gemActor*>((gemObject*) event->attacker);
    gemActor* gemTarget   = dynamic_cast<gemActor*>((gemObject*) event->target);

    switch(attack_result)
    {
        case ATTACK_DAMAGE:
        {
            bool isNearlyDead = false;
            if(target_data->GetMaxHP().Current() > 0.0 && target_data->GetHP()/target_data->GetMaxHP().Current() > 0.2)
            {
                if((target_data->GetHP() - event->FinalDamage) / target_data->GetMaxHP().Current() <= 0.2)
                    isNearlyDead = true;
            }

            psCombatEventMessage ev(event->AttackerCID,
                                    isNearlyDead ? psCombatEventMessage::COMBAT_DAMAGE_NEARLY_DEAD : psCombatEventMessage::COMBAT_DAMAGE,
                                    gemAttacker->GetEID(),
                                    gemTarget->GetEID(),
                                    event->AttackLocation,
                                    event->FinalDamage,
                                    weapon->GetAttackAnimID(gemAttacker->GetCharacterData()),
                                    gemTarget->FindAnimIndex("hit"));

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);

            // Apply final damage
            if(target_data!=NULL)
            {
                gemTarget->DoDamage(gemAttacker,event->FinalDamage);

                if(gemAttacker)
                {
                    gemAttacker->InvokeAttackScripts(gemTarget, weapon);
                }

                if(gemTarget)
                {
                    gemTarget->InvokeDefenseScripts(gemAttacker, weapon);
                    if(isNearlyDead)
                    {
                        gemTarget->InvokeNearlyDeadScripts(gemAttacker, weapon);
                    }
                }
            }

            // If the target wasn't in combat, it is now...
            // Note that other modes shouldn't be interrupted automatically
            if(gemTarget->GetMode() == PSCHARACTER_MODE_PEACE || gemTarget->GetMode() == PSCHARACTER_MODE_WORK)
            {
                if(gemTarget->GetClient())   // Set reciprocal target
                {
                    gemTarget->GetClient()->SetTargetObject(gemAttacker,true);
                    gemTarget->SendTargetStatDR(gemTarget->GetClient());
                }

                // The default stance is 'Fully Defensive'.
                Stance initialStance = psserver->GetCombatManager()->GetStance(psserver->GetCacheManager(), "FullyDefensive");
                psserver->GetCombatManager()->AttackSomeone(gemTarget,gemAttacker,initialStance);
            }

            if(weapon)
            {
                weapon->AddDecay(weaponDecay->GetValue());
            }
            if(struckArmor)
            {
                struckArmor->AddDecay(armorDecay->GetValue());
            }

            psserver->GetCombatManager()->NotifyTarget(gemAttacker,gemTarget);

            break;
        }
        case ATTACK_DODGED:
        {
            psCombatEventMessage ev(event->AttackerCID,
                                    psCombatEventMessage::COMBAT_DODGE,
                                    gemAttacker->GetEID(),
                                    gemTarget->GetEID(),
                                    event->AttackLocation,
                                    0, // no dmg on a dodge
                                    weapon->GetAttackAnimID(gemAttacker->GetCharacterData()),
                                    (unsigned int)-1); // no defense anims yet

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);
            psserver->GetCombatManager()->NotifyTarget(gemAttacker,gemTarget);
            break;
        }
        case ATTACK_BLOCKED:
        {
            psCombatEventMessage ev(event->AttackerCID,
                                    psCombatEventMessage::COMBAT_BLOCK,
                                    gemAttacker->GetEID(),
                                    gemTarget->GetEID(),
                                    event->AttackLocation,
                                    0, // no dmg on a block
                                    weapon->GetAttackAnimID(gemAttacker->GetCharacterData()),
                                    (unsigned int)-1); // no defense anims yet

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);

            if(weapon)
            {
                weapon->AddDecay(weaponDecay->GetValue());
            }
            //TODO: for now we disable decaying for bows (see PS#5181)
            if(blockingWeapon && !blockingWeapon->GetIsRangeWeapon())
            {
                blockingWeapon->AddDecay(blockDecay->GetValue());
            }

            psserver->GetCombatManager()->NotifyTarget(gemAttacker,gemTarget);

            break;
        }
        case ATTACK_MISSED:
        {
            psCombatEventMessage ev(event->AttackerCID,
                                    psCombatEventMessage::COMBAT_MISS,
                                    gemAttacker->GetEID(),
                                    gemTarget->GetEID(),
                                    event->AttackLocation,
                                    0, // no dmg on a miss
                                    weapon->GetAttackAnimID(gemAttacker->GetCharacterData()),
                                    (unsigned int)-1); // no defense anims yet

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);
            psserver->GetCombatManager()->NotifyTarget(gemAttacker,gemTarget);
            break;
        }
        case ATTACK_OUTOFRANGE:
        {
            if(event->AttackerCID)
            {
                psserver->SendSystemError(event->AttackerCID,"You are too far away to attack!");

            }
            break;
        }
        case ATTACK_BADANGLE:
        {
            if(event->AttackerCID)   // if human player
            {
                psserver->SendSystemError(event->AttackerCID,"You must face the enemy to attack!");

            }
            break;
        }
        case ATTACK_OUTOFAMMO:
        {
            psserver->SendSystemError(event->AttackerCID, "You are out of ammo!");

            if(event->attacker && event->attacker.IsValid())
                psserver->GetCombatManager()->StopAttack(dynamic_cast<gemActor*>((gemObject*) event->attacker));   // if you run out of ammo, you exit attack mode
            break;
        }
    }
}


