/*
 * combatmanager.cpp
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

#include <psconfig.h>
//=============================================================================
// Project Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/location.h"
#include "util/mathscript.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "events.h"
#include "gem.h"
#include "entitymanager.h"
#include "npcmanager.h"
#include "combatmanager.h"
#include "netmanager.h"
#include "globals.h"
#include "psserverchar.h"
#include "cachemanager.h"

/// This #define determines how far away people will get detailed combat events.
#define MAX_COMBAT_EVENT_RANGE 30

// #define COMBAT_DEBUG

/**
 * When a player or NPC attacks someone, the first combat is queued.  When
 * that event is processed, if appropriate at the end, the next combat event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 *
 * Note that this event is just a hint that a combat event might occur at this
 * time.  Other events (like equiping items, removing items, spell effects, etc)
 * may alter this time.
 *
 * When a combat event fires, the first task is to check whether the action can actually
 * occur at this time.  If it cannot, the event should not create another event since
 * the action that caused the "delay" or "acceleration" change should have created its 
 * own event.
 *
 * TODO: psGEMEvent makes this depend only on the attacker when in fact
 * this event depends on both attacker and target being present at the time the
 * event fires.
 */
class psCombatGameEvent : public psGameEvent
{
public:

    csWeakRef<gemObject>  attacker;  ///< Entity who instigated this attack
    csWeakRef<gemObject>  target;    ///< Entity who is target of this attack
    psCharacter *attackerdata;
    psCharacter *targetdata;
    int TargetCID;                   ///< ClientID of target
    int AttackerCID;                 ///< ClientID of attacker
    INVENTORY_SLOT_NUMBER WeaponSlot; ///< Identifier of the slot for which this attack event should process
    uint32 WeaponID;                 ///< UID of the weapon used for this attack event
    //float AttackValue;               ///< Measure of quality of attack ability
    //float AttackRoll;                ///< Randomized attack value.  Used for
    //float DefenseRoll;               ///< Randomized defense effectiveness this event
    //float DodgeValue;                ///< Measure of quality of dodge ability of target
    //float BlockValue;                ///< Quality of blocking ability of all weapons target has
    //float CounterBlockValue;         ///< Difficulty of blocking attacking weapon
    //float QualityOfHit;              ///< How much of attack remains after shield protection
    //float BaseHitDamage;             ///< Hit damage without taking armor into account.
    INVENTORY_SLOT_NUMBER AttackLocation;  ///< Which slot should we check the armor of?
    //float ArmorDamageAdjustment;     ///< How much does armor in the struck spot help?
    //float FinalBaseDamage;           ///< Resulting damage after armor adjustment
    //float DamageMods;                ///< Magnifiers on damage for magic effects, etc.
    float FinalDamage;               ///< Final damage applied to target

    int   AttackResult;              ///< Code indicating the result of the attack attempt
    int   PreviousAttackResult;      ///< The code of the previous result of the attack attempt

    psCombatGameEvent(CombatManager *mgr,
                      int delayticks,
                      int action,
                      gemObject *attacker,
                      INVENTORY_SLOT_NUMBER weaponslot,
                      uint32 weaponID,
                      gemObject *target,
                      int attackerCID,
                      int targetCID,
                      int previousResult);
    ~psCombatGameEvent();                      

    virtual bool CheckTrigger();
    virtual void Trigger();  // Abstract event processing function

    gemObject* GetTarget()
    {
        return target;
    };
    gemObject* GetAttacker()
    {
        return attacker;
    };
    psCharacter *GetTargetData()
    {
        return targetdata;
    };
    psCharacter *GetAttackerData()
    {
        return attackerdata;
    };
    INVENTORY_SLOT_NUMBER GetWeaponSlot()
    {
        return WeaponSlot;
    };
    
//    virtual void Disconnecting(void * object);
    
    int GetTargetID()               { return TargetCID; };
    int GetAttackerID()             { return AttackerCID; };
    int GetAttackResult()           { return AttackResult; };

protected:
    CombatManager *combatmanager;
};

CombatManager::CombatManager(CacheManager* cachemanager, EntityManager* entitymanager) : pvp_region(NULL)
{
    randomgen = psserver->rng;
    cacheManager = cachemanager;
    entityManager = entitymanager;
  
    calc_damage  = psserver->GetMathScriptEngine()->FindScript("Calculate Damage");
    calc_decay   = psserver->GetMathScriptEngine()->FindScript("Calculate Decay");
    if ( !calc_damage )
    {
        Error1("Calculate Damage Script could not be found.  Check the math_scripts DB table.");
    }
    else if ( !calc_decay )
    {
        Error1("Calculate Decay Script could not be found.  Check the math_scripts DB table.");
    }
    else
    {
        targetLocations.Push(PSCHARACTER_SLOT_HELM);
        targetLocations.Push(PSCHARACTER_SLOT_TORSO);
        targetLocations.Push(PSCHARACTER_SLOT_ARMS);
        targetLocations.Push(PSCHARACTER_SLOT_GLOVES);
        targetLocations.Push(PSCHARACTER_SLOT_LEGS);
        targetLocations.Push(PSCHARACTER_SLOT_BOOTS);
    } 

    staminacombat = psserver->GetMathScriptEngine()->FindScript("StaminaCombat");

    Subscribe(&CombatManager::HandleDeathEvent, MSGTYPE_DEATH_EVENT, NO_VALIDATION);
}

CombatManager::~CombatManager()
{
    if (pvp_region) 
    {
        delete pvp_region;
        pvp_region = NULL;
    }
}

bool CombatManager::InitializePVP()
{
    Result rs(db->Select("select * from sc_location_type where name = 'pvp_region'"));

    if (!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError() );
        return false;
    }

    // no PVP defined
    if (rs.Count() == 0)
    {
      return true;
    }

    if (rs.Count() > 1)
    {
        Error1("More than one pvp_region defined!");
        // return false; // not really fatal
    }

    LocationType *loctype = new LocationType();
    
    if (loctype->Load(rs[0],NULL,db))
    {
        pvp_region = loctype;
    }
    else
    {
        Error2("Could not load location: %s",db->GetLastError() );            
        delete loctype;
        // return false; // not fatal
    }
    return true;
}

bool CombatManager::InPVPRegion(csVector3& pos,iSector * sector)
{
    if (pvp_region && pvp_region->CheckWithinBounds(entityManager->GetEngine(), pos, sector))
        return true;

    return false;
}


const Stance & CombatManager::GetStance(CacheManager* cachemanager, csString name)
{
    name.Downcase();
    size_t id = cachemanager->stanceID.Find(name);
    if(id == csArrayItemNotFound)
    {
        //getting gloabal default combat stance
        id = cachemanager->stanceID.Find(cachemanager->getOption("combat:default_stance")->getValue());

        if(id == csArrayItemNotFound)
            id = cachemanager->stanceID.Find("normal");
    }
    return cachemanager->stances.Get(id);
}


const Stance & CombatManager::GetLoweredActorStance(CacheManager* cachemanager, gemActor* attacker)
{
    Stance currentStance = attacker->GetCombatStance();

    if(currentStance.stance_id == (cachemanager->stances.GetSize()-1)) // The lowest possible stance is already choosen
    {
        // Propably nothing to do here
        return cachemanager->stances.Get(currentStance.stance_id);
    }

    return cachemanager->stances.Get(currentStance.stance_id+1);
}


const Stance & CombatManager::GetRaisedActorStance(CacheManager* cachemanager, gemActor* attacker)
{
    Stance currentStance = attacker->GetCombatStance();

    if(currentStance.stance_id == 1) // The greatest possible stance is already choosen
    {
        // Propably nothing to do here
        return cachemanager->stances.Get(currentStance.stance_id);
    }

    return cachemanager->stances.Get(currentStance.stance_id-1);
}

void CombatManager::AttackSomeone(gemActor *attacker,gemObject *target,Stance stance)
{
    psCharacter *attacker_character = attacker->GetCharacterData();

    //we don't allow an overweight or defeated char to fight
    if (attacker->GetMode() == PSCHARACTER_MODE_DEFEATED || 
        attacker->GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
        return;

    if (attacker->GetMode() == PSCHARACTER_MODE_COMBAT)  // Already fighting
    {
        SetCombat(attacker,stance);  // switch stance from Bloody to Defensive, etc.
        return;
    }
    else
    {
        if (attacker->GetMode() == PSCHARACTER_MODE_SIT) //we are sitting force the char to stand
            attacker->Stand();
        attacker_character->ResetSwings(csGetTicks());
    }

    // Indicator of whether any weapons are available to attack with
    bool startedAttacking=false;
    bool haveWeapon=false;

    // Step through each current slot and queue events for all that can attack
    for (int slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
    {
        // See if this slot is able to attack
        if (attacker_character->Inventory().CanItemAttack((INVENTORY_SLOT_NUMBER) slot))
        {
            INVENTORY_SLOT_NUMBER weaponSlot = (INVENTORY_SLOT_NUMBER) slot;
            // Get the data for the "weapon" that is used in this slot
            psItem *weapon=attacker_character->Inventory().GetEffectiveWeaponInSlot(weaponSlot);

            csString response;
            if (weapon!=NULL && weapon->CheckRequirements(attacker_character,response) )
            {
                haveWeapon = true;
                Debug5(LOG_COMBAT,attacker->GetClientID(),"%s tries to attack with %s weapon %s at %.2f range",
                       attacker->GetName(),(weapon->GetIsRangeWeapon()?"range":"melee"),weapon->GetName(),
                       attacker->RangeTo(target,false));
                Debug3(LOG_COMBAT,attacker->GetClientID(),"%s started attacking with %s",attacker->GetName(),
                       weapon->GetName());
                
                // start the ball rolling
                QueueNextEvent(attacker,weaponSlot,target,attacker->GetClientID(),target->GetClientID());  
                
                startedAttacking=true;
            }
            else
            {
                if( weapon  && attacker_character->GetActor())
                {
                    Debug3(LOG_COMBAT,attacker->GetClientID(),"%s tried attacking with %s but can't use it.",
                           attacker->GetName(),weapon->GetName());
#ifdef COMBAT_DEBUG
                    psserver->SendSystemError(attacker_character->GetActor()->GetClientID(), response);
#endif
                } 
            }
        }
    }

    /* Only notify the target if any attacks were able to start.  Otherwise there are
     * no available weapons with which to attack.
     */
    if (haveWeapon)
    {
        if (startedAttacking)
        {
            // The attacker should now enter combat mode
            if (attacker->GetMode() != PSCHARACTER_MODE_COMBAT)
            {
                SetCombat(attacker,stance);
            }
        }
        else
        {
            psserver->SendSystemError(attacker->GetClientID(),"You are too far away to attack!");
            return;
        }
    }
    else
    {
        psserver->SendSystemError(attacker->GetClientID(),"You have no weapons equipped!");
        return;
    }
}

void CombatManager::SetCombat(gemActor *combatant, Stance stance)
{
    // Sanity check
    if (!combatant || !combatant->GetCharacterData() || !combatant->IsAlive())
        return;

    // Change stance if new and for a player (NPCs don't have different stances)
    if (combatant->GetClientID() && combatant->GetCombatStance().stance_id != stance.stance_id)
    {
        psSystemMessage msg(combatant->GetClientID(), MSG_COMBAT_STANCE, "%s changes to a %s stance", combatant->GetName(), stance.stance_name.GetData() );
        msg.Multicast(combatant->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);

        combatant->SetCombatStance(stance);
    }

    combatant->SetMode(PSCHARACTER_MODE_COMBAT); // Set mode and multicast new mode and/or stance
    Debug3(LOG_COMBAT,combatant->GetClientID(), "%s starts attacking with stance %s", combatant->GetName(), stance.stance_name.GetData());
}

void CombatManager::StopAttack(gemActor *attacker)
{
    if (!attacker)
        return;

    // TODO: I'm not sure this is a wise idea after all...spells may not be offensive...
    switch (attacker->GetMode())
    {
        case PSCHARACTER_MODE_SPELL_CASTING:
            attacker->InterruptSpellCasting();
            break;
        case PSCHARACTER_MODE_COMBAT:
            attacker->SetMode(PSCHARACTER_MODE_PEACE);
            break;
        default:
            return;
    }

    Debug2(LOG_COMBAT, attacker->GetClientID(), "%s stops attacking", attacker->GetName());
}

void CombatManager::NotifyTarget(gemActor *attacker,gemObject *target)
{
    // Queue Attack percetion to npc clients
    gemNPC *targetnpc = dynamic_cast<gemNPC *>(target);
    if (targetnpc)
        psserver->npcmanager->QueueAttackPerception(attacker,targetnpc);

    // Interrupt spell casting
    //gemActor *targetactor = dynamic_cast<gemActor*>(target);
    //if (targetactor)
    //    targetactor->GetCharacterData()->InterruptSpellCasting();
}

void CombatManager::QueueNextEvent(psCombatGameEvent *event)
{
    QueueNextEvent(event->GetAttacker(),
                   event->GetWeaponSlot(),
                   event->GetTarget(),
                   event->GetAttackerID(),
                   event->GetTargetID(),
                   event->GetAttackResult());
}

void CombatManager::QueueNextEvent(gemObject *attacker,INVENTORY_SLOT_NUMBER weaponslot,
                                     gemObject *target,
                                     int attackerCID,
                                     int targetCID, int previousResult)
{

    /* We should check the combat queue here if a viable combat queue method is
     *  ever presented.  As things stand the combat queue is not workable with 
     *  multiple weapons being used.
     */
// Get next action from QueueOfAttacks
//    int action = GetQueuedAction(attacker);

// If no next action, create default action based on mode.
//    if (!action)
//    {
//        action = GetDefaultModeAction(attacker);
//    }

    int action=0;

    psCharacter *Character=attacker->GetCharacterData();
    psItem *Weapon=Character->Inventory().GetEffectiveWeaponInSlot(weaponslot);
    uint32 weaponID = Weapon->GetUID();

    float latency = Weapon->GetLatency();
    int delay = (int)(latency*1000);

    // Create first Combat Event and queue it.
    psCombatGameEvent *event;
    
    event = new psCombatGameEvent(this,
                                  delay,
                                  action,
                                  attacker,
                                  weaponslot,
                                  weaponID,
                                  target,
                                  attackerCID,
                                  targetCID,
                                  previousResult);
    event->GetAttackerData()->TagEquipmentObject(weaponslot,event->id);

    psserver->GetEventManager()->Push(event);
}

/* ----------------------- NOT IMPLEMENTED YET -------------------------
int CombatManager::GetQueuedAction(gemActor *attacker)
{
    (void) attacker;
    // TODO: This will eventually query the prop class for the next action
    // in the queue.

    return 0;
}

int CombatManager::GetDefaultModeAction(gemActor *attacker)
{
    (void) attacker;
    // TODO: This will eventually query the prop class for the mode
    // if nothing is in the queue.

    return 1;
}*/


/**
 * This is the meat and potatoes of the combat engine here.
 */
int CombatManager::CalculateAttack(psCombatGameEvent *event, psItem* subWeapon)
{
    INVENTORY_SLOT_NUMBER otherHand = event->GetWeaponSlot() == PSCHARACTER_SLOT_LEFTHAND ? PSCHARACTER_SLOT_RIGHTHAND : PSCHARACTER_SLOT_LEFTHAND;
    event->AttackLocation = (INVENTORY_SLOT_NUMBER) targetLocations[randomgen->Get((int) targetLocations.GetSize())];

    gemObject *attacker = event->GetAttacker();
    gemObject *target = event->GetTarget();

    // calculate difference between target and attacker location - to be used for angle validation
    csVector3 diff(0); // initialize to some big value that shows an error
    
    {
        csVector3 attackPos, targetPos;
        iSector *attackSector, *targetSector;

        attacker->GetPosition(attackPos, attackSector);
        target->GetPosition(targetPos, targetSector);

        if((attacker->GetInstance() != target->GetInstance() &&
            attacker->GetInstance() != INSTANCE_ALL && target->GetInstance() != INSTANCE_ALL) ||
            !(entityManager->GetWorld()->WarpSpace(targetSector, attackSector, targetPos)))
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

    calc_damage->Evaluate(&env);

    if (DoLogDebug2(LOG_COMBAT, event->GetAttackerData()->GetPID().Unbox()))
    {
        CPrintf(CON_DEBUG, "Variables for Calculate Damage:\n");
        env.DumpAllVars();
    }

    MathVar *badrange = env.Lookup("BadRange");    // BadRange = Target is too far away
    MathVar *badangle = env.Lookup("BadAngle");    // BadAngle = Attacker doesn't aim at enemy
    MathVar *missed   = env.Lookup("Missed");      // Missed   = Attack missed the enemy
    MathVar *dodged   = env.Lookup("Dodged");      // Dodged   = Attack dodged  by enemy
    MathVar *blocked  = env.Lookup("Blocked");     // Blocked  = Attack blocked by enemy
    MathVar *damage   = env.Lookup("FinalDamage"); // Actual damage done, if any

    if (badrange && badrange->GetValue() < 0.0)
        return ATTACK_OUTOFRANGE;
    else if (badangle && badangle->GetValue() < 0.0)
        return ATTACK_BADANGLE;
    else if (missed && missed->GetValue() < 0.0)
        return ATTACK_MISSED;
    else if (dodged && dodged->GetValue() < 0.0)
        return ATTACK_DODGED;
    else if (blocked && blocked->GetValue() < 0.0)
       return ATTACK_BLOCKED;

    event->FinalDamage = damage->GetValue();

    DebugOutput(event, env);

    return ATTACK_DAMAGE;
}

void CombatManager::ApplyCombatEvent(psCombatGameEvent *event, int attack_result)
{
    psCharacter *attacker_data = event->GetAttackerData();
    psCharacter *target_data=event->GetTargetData();

    MathVar *weaponDecay = NULL;
    MathVar *blockDecay = NULL;
    MathVar *armorDecay = NULL;
    MathEnvironment env;

    psItem *weapon         = attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot());
    psItem *blockingWeapon = target_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot(),true);
    psItem *struckArmor    = target_data->Inventory().GetEffectiveArmorInSlot(event->AttackLocation);

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

        calc_decay->Evaluate(&env);

        weaponDecay = env.Lookup("WeaponDecay");
        blockDecay  = env.Lookup("BlockingDecay");
        armorDecay  = env.Lookup("ArmorDecay");
    }

    gemActor *gemAttacker = dynamic_cast<gemActor*> ((gemObject *) event->attacker);
    gemActor *gemTarget   = dynamic_cast<gemActor*> ((gemObject *) event->target);

    switch (attack_result)
    {
        case ATTACK_DAMAGE:
        {
            bool isNearlyDead = false;
            if (target_data->GetMaxHP().Current() > 0.0 && target_data->GetHP()/target_data->GetMaxHP().Current() > 0.2)
            {
                if ((target_data->GetHP() - event->FinalDamage) / target_data->GetMaxHP().Current() <= 0.2)
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
            if (target_data!=NULL)
            {
                gemTarget->DoDamage(gemAttacker,event->FinalDamage);
                
                if (gemAttacker)
                {
                    gemAttacker->InvokeAttackScripts(gemTarget, weapon);
                }

                if (gemTarget)
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
            if (gemTarget->GetMode() == PSCHARACTER_MODE_PEACE || gemTarget->GetMode() == PSCHARACTER_MODE_WORK)
            {
                if (gemTarget->GetClient())  // Set reciprocal target
                    gemTarget->GetClient()->SetTargetObject(gemAttacker,true);

                // The default stance is 'Fully Defensive'.
                Stance initialStance = GetStance(cacheManager, "FullyDefensive");
                AttackSomeone(gemTarget,gemAttacker,initialStance);
            }

            if (weapon)
            {
                weapon->AddDecay(weaponDecay->GetValue());
            }
            if (struckArmor)
            {
                struckArmor->AddDecay(armorDecay->GetValue());
            }

            NotifyTarget(gemAttacker,gemTarget);

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
            NotifyTarget(gemAttacker,gemTarget);
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
                weapon->GetAttackAnimID( gemAttacker->GetCharacterData() ),
                (unsigned int)-1); // no defense anims yet

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);

            if (weapon)
            {
                weapon->AddDecay(weaponDecay->GetValue());
            }
            if (blockingWeapon)
            {
                blockingWeapon->AddDecay(blockDecay->GetValue());
            }

            NotifyTarget(gemAttacker,gemTarget);

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
                weapon->GetAttackAnimID( gemAttacker->GetCharacterData() ),
                (unsigned int)-1); // no defense anims yet

            ev.Multicast(gemTarget->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);
            NotifyTarget(gemAttacker,gemTarget);
            break;
        }
        case ATTACK_OUTOFRANGE:
        {
            if (event->AttackerCID)
            {
                psserver->SendSystemError(event->AttackerCID,"You are too far away to attack!");

                // Auto-stop attack is commented out below, when out of range to prevent npc kiting by jumping in and out of range
                //if (event->attacker && event->attacker.IsValid())
                //    StopAttack(dynamic_cast<gemActor*>((gemObject *) event->attacker));  // if you run away, you exit attack mode
            }
            break;
        }
        case ATTACK_BADANGLE:
        {
            if (event->AttackerCID)  // if human player
            {
                psserver->SendSystemError(event->AttackerCID,"You must face the enemy to attack!");

                // Auto-stop attack is commented out below, when out of range to prevent npc kiting by jumping in and out of range
                //if (event->attacker && event->attacker.IsValid())
                //    StopAttack(dynamic_cast<gemActor*>((gemObject *) event->attacker));  // if you run away, you exit attack mode
            }
            break;
        }
        case ATTACK_OUTOFAMMO:
        {
            psserver->SendSystemError(event->AttackerCID, "You are out of ammo!");

            if (event->attacker && event->attacker.IsValid())
                StopAttack(dynamic_cast<gemActor*>((gemObject *) event->attacker));  // if you run out of ammo, you exit attack mode
            break;
        }
    }
}

void CombatManager::HandleCombatEvent(psCombatGameEvent *event)
{
    psCharacter *attacker_data,*target_data;
    int attack_result;
    bool skipThisRound = false;

    if (!event->GetAttacker() || !event->GetTarget()) // disconnected and deleted
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID, "Combat stopped as one participant logged of.");
#endif
        return;
    }

    gemActor *gemAttacker = dynamic_cast<gemActor*> ((gemObject *) event->attacker);
    gemActor *gemTarget   = dynamic_cast<gemActor*> ((gemObject *) event->target);

    attacker_data=event->GetAttackerData();
    target_data=event->GetTargetData();

    // If the attacker is no longer in attack mode abort.
    if (gemAttacker->GetMode() != PSCHARACTER_MODE_COMBAT)
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID,
                "Combat stopped as you left combat mode.");
#endif
        return;
    }

    // If target is dead, abort.
    if (!gemTarget->IsAlive() )
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemResult(event->AttackerCID, "Combat stopped as one participant logged of.");
#endif
        return;
    }

    // If the slot is no longer attackable, abort
    if (!attacker_data->Inventory().CanItemAttack(event->GetWeaponSlot()))
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID, "Combat stopped as you have no longer an attackable item equipped.");
#endif
        return;
    }
    
    if (attacker_data->Inventory().GetEquipmentObject(event->GetWeaponSlot()).eventId != event->id)
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID, "Ignored combat event as newer is in.");
#endif
        return;
    }
   
    psItem* weapon = attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot());

    // weapon became unwieldable
    csString response;
    if(weapon!=NULL && !weapon->CheckRequirements(attacker_data,response))
    {
        Debug2(LOG_COMBAT, gemAttacker->GetClientID(),"%s has lost use of weapon", gemAttacker->GetName() );
        psserver->SendSystemError(event->AttackerCID, "You can't use your %s any more.", weapon->GetName() );
        return;
    }

    // If the weapon in the slot has been changed, skip a turn (latency for this slot may also have changed)
    if (event->WeaponID != weapon->GetUID())
    {
        Debug2(LOG_COMBAT, gemAttacker->GetClientID(),"%s has changed weapons mid battle", gemAttacker->GetName() );
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID, "Weapon changed. Skipping");
#endif
        skipThisRound = true;
    }

    Client * attacker_client = psserver->GetNetManager()->GetClient(event->AttackerCID);
    if (attacker_client)
    {
        // Input the stamina data
        MathEnvironment env;
        env.Define("Actor",  event->GetAttacker());
        env.Define("Weapon", weapon);
        staminacombat->Evaluate(&env);
        MathVar *PhyDrain = env.Lookup("PhyDrain");
        MathVar *MntDrain = env.Lookup("MntDrain");

        if ( (attacker_client->GetCharacterData()->GetStamina(true) < PhyDrain->GetValue())
            || (attacker_client->GetCharacterData()->GetStamina(false) < MntDrain->GetValue()) )
        {
           StopAttack(attacker_data->GetActor());
           psserver->SendSystemError(event->AttackerCID, "You are too tired to attack.");
           return;
        }

        // If the target has become impervious, abort and give up attacking
        if (!attacker_client->IsAllowedToAttack(gemTarget))
        {
           StopAttack(attacker_data->GetActor());
           return;
        }

        // If the target has changed, abort (assume another combat event has started since we are still in attack mode)
        if (gemTarget != attacker_client->GetTargetObject())
        {
#ifdef COMBAT_DEBUG
            psserver->SendSystemError(event->AttackerCID, "Target changed.");
#endif
            return;
        }
    }
    else
    {
        // Check if the npc's target has changed (if it has, then assume another combat event has started.)
        gemNPC* npcAttacker = dynamic_cast<gemNPC*>(gemAttacker);
        if (npcAttacker && npcAttacker->GetTarget() != gemTarget)
        {
#ifdef COMBAT_DEBUG
            psserver->SendSystemError(event->AttackerCID, "NPC's target changed.");
#endif
            return;
        }
    }

    if (gemAttacker->IsSpellCasting())
    {
        psserver->SendSystemInfo(event->AttackerCID, "You can't attack while casting spells.");
        skipThisRound = true;
    }

    if (!skipThisRound)
    {
        if (weapon->GetIsRangeWeapon() && weapon->GetUsesAmmo())
        {
            INVENTORY_SLOT_NUMBER otherHand = event->GetWeaponSlot() == PSCHARACTER_SLOT_RIGHTHAND ?
                                                                        PSCHARACTER_SLOT_LEFTHAND:
                                                                        PSCHARACTER_SLOT_RIGHTHAND;

            attack_result = ATTACK_NOTCALCULATED;

            psItem* otherItem = attacker_data->Inventory().GetInventoryItem(otherHand);
            if (otherItem == NULL)
            {
                attack_result = ATTACK_OUTOFAMMO;
            }
            else if (otherItem->GetIsContainer()) // Is it a quiver?
            {
                // Pick the first ammo we can shoot from the container
                // And set it as the active ammo
                bool bFound = false;
                for (size_t i=1; i<attacker_data->Inventory().GetInventoryIndexCount() && !bFound; i++)
                {
                    psItem* currItem = attacker_data->Inventory().GetInventoryIndexItem(i);
                    if (currItem && 
                        currItem->GetContainerID() == otherItem->GetUID() &&
                        weapon->GetAmmoTypeID().In(currItem->GetBaseStats()->GetUID()))
                    {
                        otherItem = currItem;
                        bFound = true;
                    }
                }
                if (!bFound)
                    attack_result = ATTACK_OUTOFAMMO;
            }
            else if (!weapon->GetAmmoTypeID().In(otherItem->GetBaseStats()->GetUID()))
            {
                attack_result = ATTACK_OUTOFAMMO;
            }

            if (attack_result != ATTACK_OUTOFAMMO)
            {
                psItem* usedAmmo = attacker_data->Inventory().RemoveItemID(otherItem->GetUID(), 1);
                if (usedAmmo)
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

        ApplyCombatEvent(event, attack_result);
    }

    // Queue next event to continue combat if this is an auto attack slot
    if (attacker_data->Inventory().IsItemAutoAttack(event->GetWeaponSlot()))
    {
//      CPrintf(CON_DEBUG, "Queueing Slot %d for %s's next combat event.\n",event->GetWeaponSlot(), event->attacker->GetName() );
        QueueNextEvent(event);
    }
    else
    {
#ifdef COMBAT_DEBUG
        psserver->SendSystemError(event->AttackerCID, "Item %s is not a auto attack item.",attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot())->GetName());
#endif
    }
//    else
//        CPrintf(CON_DEBUG, "Slot %d for %s not an auto-attack slot.\n",event->GetWeaponSlot(), event->attacker->GetName() );
}

void CombatManager::DebugOutput(psCombatGameEvent *event, const MathEnvironment & env)
{
    MathVar *missed   = env.Lookup("Missed");      // Missed  = Attack missed the enemy
    MathVar *dodged   = env.Lookup("Dodged");      // Dodged  = Attack dodged  by enemy
    MathVar *blocked  = env.Lookup("Blocked");     // Blocked = Attack blocked by enemy
    MathVar *damage   = env.Lookup("FinalDamage"); // Actual damage done, if any

    psItem* item = event->GetAttackerData()->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot() );

    csString debug;
    debug.Append( "-----Debug Combat Summary--------\n");
    debug.AppendFmt( "%s attacks %s with slot %d , weapon %s, quality %1.2f, basedmg %1.2f/%1.2f/%1.2f\n",
      event->attacker->GetName(),event->target->GetName(), event->GetWeaponSlot(),item->GetName(),item->GetItemQuality(),
      item->GetDamage(PSITEMSTATS_DAMAGETYPE_SLASH),item->GetDamage(PSITEMSTATS_DAMAGETYPE_BLUNT),item->GetDamage(PSITEMSTATS_DAMAGETYPE_PIERCE));
    debug.AppendFmt( "Missed: %1.6f Dodged: %1.6f Blocked: %1.6f", missed->GetValue(), dodged->GetValue(), blocked->GetValue());
    debug.AppendFmt( "Damage: %1.1f\n", damage->GetValue());
    Debug2(LOG_COMBAT, event->attacker->GetClientID(), "%s", debug.GetData());
}

void CombatManager::HandleDeathEvent(MsgEntry *me,Client *client)
{
    psDeathEvent death(me);

    Debug1(LOG_COMBAT,death.deadActor->GetClientID(),"Combat Manager handling Death Event\n");

    // Stop any duels.
    if (death.deadActor->GetClient())
        death.deadActor->GetClient()->ClearAllDuelClients();

    // Stop actor moving.
    death.deadActor->StopMoving();

    // Send out the notification of death, which plays the anim, etc.
    psCombatEventMessage die(death.deadActor->GetClientID(),
                                psCombatEventMessage::COMBAT_DEATH,
                                death.killer ? death.killer->GetEID() : 0,
                                death.deadActor->GetEID(),
                                -1, // no target location
                                0,  // no dmg on a death
                                (unsigned int)-1,  // TODO: "killing blow" matrix of mob-types vs. weapon types
                                (unsigned int)-1 ); // Death anim on client side is handled by the death mode message

    die.Multicast(death.deadActor->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);
}


/*-------------------------------------------------------------*/

psSpareDefeatedEvent::psSpareDefeatedEvent(gemActor *losr) : psGameEvent(0, SECONDS_BEFORE_SPARING_DEFEATED * 1000, "psSpareDefeatedEvent")
{
    loser = losr->GetClient();
}

void psSpareDefeatedEvent::Trigger()
{
    // Ignore stale events: perhaps the character was already killed and resurrected...
    if (!loser.IsValid() || !loser->GetActor() || loser->GetActor()->GetMode() != PSCHARACTER_MODE_DEFEATED)
        return;

    psserver->SendSystemInfo(loser->GetClientNum(), "Your opponent has spared your life.");
    loser->ClearAllDuelClients();
    loser->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
}

/*-------------------------------------------------------------*/

psCombatGameEvent::psCombatGameEvent(CombatManager *mgr,
                                     int delayticks,
                                     int act,
                                     gemObject *attacker,
                                     INVENTORY_SLOT_NUMBER weaponslot,
                                     uint32 weapon,
                                     gemObject *target,
                                     int attackerCID,
                                     int targetCID,
                                     int previousResult)
  : psGameEvent(0,delayticks,"psCombatGameEvent")
{
    combatmanager  = mgr;
    this->attacker = attacker;
    this->WeaponSlot = weaponslot;
    this->WeaponID = weapon;
    this->target   = target;
    this->AttackerCID = attackerCID;
    this->TargetCID   = targetCID;

    // Also register the target as a disconnector
//    target->Register(this);
//    attacker->Register( this ); 
    
    attackerdata = attacker->GetCharacterData();
    targetdata   = target->GetCharacterData();

    if (!attackerdata || !targetdata)
        return;

    AttackLocation=PSCHARACTER_SLOT_NONE;   
    FinalDamage=-1;
    AttackResult=ATTACK_NOTCALCULATED;
    PreviousAttackResult=previousResult;
}


psCombatGameEvent::~psCombatGameEvent()
{
}

bool psCombatGameEvent::CheckTrigger()
{
    if ( attacker.IsValid() && target.IsValid())
    {
        if ( attacker->IsAlive() && target->IsAlive() )
        {
            return true;
        }
        else
        {
            return false;
        }                    
    }        
    else
    {
        return false;
    }        
}

void psCombatGameEvent::Trigger()
{
    if (!attacker.IsValid() || !target.IsValid())
        return;

    combatmanager->HandleCombatEvent(this);
}

/************
void psCombatGameEvent::Disconnecting(void * object)
{
    psGEMEvent::Disconnecting(object);
    valid = false;
    if (attacker)
    {
        attacker->Unregister(this);
        attacker = NULL;
    }
    if ( target )
    {
        target->Unregister(this);
        target = NULL;
    }
}
*****/
