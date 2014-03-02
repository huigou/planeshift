/*
 * psattackrange.cpp              creator hartra344@gmail.com
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
#include "psattackrange.h"
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

psAttackRange::psAttackRange() :
    aoeRadius(0),
    aoeAngle(0),
    successChance(0),
    attackRange(0)
{
    calc_decay   = psserver->GetMathScriptEngine()->FindScript("Calculate Decay");
    if(!calc_decay)
    {
        Error1("Calculate Decay Script could not be found.  Check the math_scripts DB table.");
    }
}

psAttackRange::~psAttackRange()
{
    delete attackRange;
    delete aoeRadius;
    delete aoeAngle;
    delete successChance;
}

bool psAttackRange::Load(iResultRow &row)
{
    id = row.GetInt("id");
    name = row["name"];
    image = row["image_name"];
    Animation = row["attack_anim"];
    description = row["attack_description"];
    speed = row.GetFloat("speed");

    type = psserver->GetCacheManager()->GetAttackTypeByName(row["attackType"]);

    attackRange = MathExpression::Create(row["range"]);
    aoeRadius   = MathExpression::Create(row["aoe_radius"]);
    aoeAngle    = MathExpression::Create(row["aoe_angle"]);
    outcome = psserver->GetProgressionManager()->FindScript(row["outcome"]);
    CS_ASSERT(aoeRadius && aoeAngle && outcome);

    successChance = MathExpression::Create(row["successchance"]);

    LoadPrerequisiteXML(requirements,NULL,row["requirements"]);

    TypeRequirements.AttachNew(new psPrereqOpAttackType(type));

    return true;
}

bool psAttackRange::CanAttack(Client* client)
{
    if(TypeRequirements)
    {
        if(TypeRequirements->Check(client->GetCharacterData()))
        {
            if(requirements)
                return requirements->Check(client->GetCharacterData());
            else
                return true;
        }
        else
            return false;
    }
    else
    {
        if(requirements)
            return requirements->Check(client->GetCharacterData());
        else
            return true;
    }
}

void psAttackRange::Decay(psCharacter* attacker_data, psCharacter* target_data,psCombatAttackGameEvent* event, bool hit, bool affectattacker)
{
    MathVar* weaponDecay = NULL;
    MathVar* blockDecay = NULL;
    MathVar* armorDecay = NULL;
    MathEnvironment env;

    psItem* weapon         = attacker_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot());
    psItem* blockingWeapon = target_data->Inventory().GetEffectiveWeaponInSlot(event->GetWeaponSlot(),true);
    event->AttackLocation = (INVENTORY_SLOT_NUMBER) psserver->GetCombatManager()->targetLocations[psserver->rng->Get((int) psserver->GetCombatManager()->targetLocations.GetSize())];
    psItem* struckArmor    = target_data->Inventory().GetEffectiveArmorInSlot(event->AttackLocation);

    // we are guaranteed some armor is present - real one, race one or base one
    CS_ASSERT(struckArmor);
    float ArmorVsWeapon = weapon->GetArmorVSWeaponResistance(struckArmor->GetBaseStats());

    // clamp value between 0 and 1
    ArmorVsWeapon = ArmorVsWeapon > 1.0F ? 1.0F : ArmorVsWeapon < 0.0F ? 0.0F : ArmorVsWeapon;

    env.Define("Weapon", weapon);                             // weapon that was used to attack
    env.Define("BlockingWeapon", blockingWeapon);             // weapon that blocked the attack
    env.Define("Armor", struckArmor);                         // armor hit
    env.Define("ArmorVsWeapon", ArmorVsWeapon);               // armor vs weapon effectiveness
    env.Define("Blocked", !hit); // identifies whether this attack was blocked

    calc_decay->Evaluate(&env);

    weaponDecay = env.Lookup("WeaponDecay");
    blockDecay  = env.Lookup("BlockingDecay");
    armorDecay  = env.Lookup("ArmorDecay");

    if(hit)
    {
        if(weapon)
        {
            if(affectattacker)
                weapon->AddDecay(weaponDecay->GetValue());
        }
        if(struckArmor)
        {
            struckArmor->AddDecay(armorDecay->GetValue());
        }
    }
    else
    {
        if(weapon)
        {
            if(affectattacker)
                weapon->AddDecay(weaponDecay->GetValue());
        }
        if(blockingWeapon)
        {
            blockingWeapon->AddDecay(blockDecay->GetValue());
        }
    }
}
bool psAttackRange::IsDualWield(psCharacter* attacker)
{
    if(type->OneHand)
        return true;
    else
        return false;
}

bool psAttackRange::Attack(gemObject* attacker, gemObject* target, INVENTORY_SLOT_NUMBER slot)
{
    psCharacter* Character=attacker->GetCharacterData();
    psItem* Weapon=Character->Inventory().GetEffectiveWeaponInSlot(slot);
    float latency = Weapon->GetLatency();
    int delay = 0;
    if(speed>0)
        delay = (int)(((speed+latency)/2)*1000); //each attack has a speed value, each weapon has a latency, the average of the 2 will give the speed of the attack
    else
        delay = (int)((latency)*1000); //however if the attack don't have a speed value, hten weapon latency is used.
    //^this is useful for making some attacks slower, but allowing weapon speed and choice to still be important

    psCombatAttackGameEvent* event;
    event = new psCombatAttackGameEvent(delay,this,attacker,target,slot,attacker->GetClientID(),target->GetClientID());
    event->GetAttackerData()->TagEquipmentObject(slot,event->id);
    psserver->GetEventManager()->Push(event);

    return true;
}

void psAttackRange::Affect(psCombatAttackGameEvent* event)
{
    gemObject* attacker = event->attacker;
    gemActor* attker = dynamic_cast<gemActor*>(attacker);
    gemObject* target = event->target;

    float range = event->weapon->GetRange();
    // Look for targets
    float power = csMin((float)MaxPower, PowerLevel(attacker->GetCharacterData(), event->weapon, range));

    MathEnvironment env;
    env.Define("Power",       power);
    env.Define("WeaponSkill",    event->attacker->GetCharacterData()->GetSkillRank(event->weapon->GetWeaponType()->skill).Current());
    env.Define("RelatedStat", event->attacker->GetCharacterData()->GetSkillRank(type->related_stat).Current());
    env.Define("Range", range);
    float radius = aoeRadius->Evaluate(&env);
    float angle  = aoeAngle->Evaluate(&env);
    //checks the chances of the attack being successful, will implement a script, if it is successful then the attack goes through, if its not then it dont
    if(!CalculateSuccess(event, range))
    {
        psserver->SendSystemInfo(attacker->GetClientID(),"Your Attack was blocked");//may make some more of thsi later
        Decay(event->GetAttackerData(),event->GetTargetData(),event,false);
        return;
    }
    bool hasAmmo = true;
    if(event->weapon->GetUsesAmmo())
    {
        //would ilke to add a dedicated ammo slot, but haven't gotten that far yet
        INVENTORY_SLOT_NUMBER otherHand = event->GetWeaponSlot() == PSCHARACTER_SLOT_RIGHTHAND ? PSCHARACTER_SLOT_LEFTHAND:PSCHARACTER_SLOT_RIGHTHAND;

        psItem* otherItem = event->GetAttackerData()->Inventory().GetInventoryItem(otherHand);
        if(otherItem == NULL)
        {
            hasAmmo = false;
        }
        else if(otherItem->GetIsContainer())  // Is it a quiver?
        {
            // Pick the first ammo we can shoot from the container
            // And set it as the active ammo
            bool bFound = false;
            for(size_t i=1; i<event->GetAttackerData()->Inventory().GetInventoryIndexCount() && !bFound; i++)
            {
                psItem* currItem = event->GetAttackerData()->Inventory().GetInventoryIndexItem(i);
                if(currItem &&
                        currItem->GetContainerID() == otherItem->GetUID() &&
                        event->weapon->GetAmmoTypeID().In(currItem->GetBaseStats()->GetUID()))
                {
                    otherItem = currItem;
                    bFound = true;
                }
            }
            if(!bFound)
                hasAmmo = false;
        }
        else if(!event->weapon->GetAmmoTypeID().In(otherItem->GetBaseStats()->GetUID()))
        {
            hasAmmo = false;
        }

        if(hasAmmo)
        {
            psItem* usedAmmo = event->GetAttackerData()->Inventory().RemoveItemID(otherItem->GetUID(), 1);
            if(usedAmmo)
            {
                usedAmmo->Destroy();
                psserver->GetCharManager()->UpdateItemViews(attacker->GetClient()->GetClientNum());
            }
        }
        else
            return;

        int affectedCount = 0;
        if(radius < 0.01f)  // single target
        {
            AffectTarget(attacker, target, target, power,event);
            affectedCount++;
        }
        else // AOE (Area of Effect)
        {
            csVector3 attackerPos;
            csVector3 targetPos;
            float yrot; // in radians
            iSector* sector;

            target->GetPosition(targetPos, yrot, sector);
            attacker->GetPosition(attackerPos, yrot, sector);

            // directional vector for a line from attacker to original target
            csVector3 attackerToTarget;
            attackerToTarget = targetPos - attackerPos;

            if(angle <= SMALL_EPSILON || angle > 360)
                angle = 360;

            angle = (angle/2)*(PI/180); // convert degrees to radians

            csArray<gemObject*> nearby = psserver->entitymanager->GetGEM()->FindNearbyEntities(sector, targetPos, radius);
            for(size_t i = 0; i < nearby.GetSize(); i++)
            {
                csString msg;
                if(!attker->IsAllowedToAttack(nearby[i],msg))
                    continue;

                if(angle < PI)
                {
                    csVector3 attackerToAffected;
                    nearby[i]->GetPosition(attackerToAffected, sector);

                    // obtain a directional line for the vector from attacker to affacted target
                    // note that this line does not originate at the original target because the
                    // cone that shall include the hittable area shall originate at the attacker
                    attackerToAffected -= attackerPos;

                    // Angle between the line original target->attacker and original target->affected target
                    float cosATAngle = attackerToAffected*attackerToTarget
                                       /(attackerToAffected.Norm()*attackerToTarget.Norm());

                    // cap the value to meaningful ones to account for rounding issues
                    if(cosATAngle > 1.0f || CS::IsNaN(cosATAngle))
                        cosATAngle = 1.0f;
                    if(cosATAngle < -1.0f)
                        cosATAngle = -1.0f;

                    // use the absolute value of the angle here to account for both sides equally - see above
                    if(fabs(acosf(cosATAngle)) >= angle)
                        continue;
                }

                AffectTarget(attacker, target, nearby[i], power,event);
                affectedCount++;
            }

            if(affectedCount > 0)
            {
                psserver->SendSystemInfo(attacker->GetClientID(), "%s affected %d %s.", name.GetData(), affectedCount, (affectedCount == 1) ? "target" : "targets");
            }
            else
            {
                psserver->SendSystemInfo(attacker->GetClientID(), "%s has no effect.", name.GetData());
            }
        }


    }
}
bool psAttackRange::CalculateSuccess(psCombatAttackGameEvent* event, float range)
{

    //This is a trial thing, will probably change up later, relies heavily on the script always coming up with a number between 1 and 100, otherwise it breaks(over 100 would work but be pointless)
    if(!successChance)
        return true;

    MathEnvironment env;
    env.Define("Actor", event->attacker->GetCharacterData());
    env.Define("WeaponSkill",event->attacker->GetCharacterData()->GetSkillRank(event->weapon->GetWeaponType()->skill).Current());
    env.Define("Range",range);

    float chance = successChance->Evaluate(&env);

    int randomnum = psserver->rng->Get(100);

    if(randomnum <= chance)
        return true;

    return false;
}
float psAttackRange::PowerLevel(psCharacter* attacker, psItem* weapon, float range) const
{
    static MathScript* script = NULL;
    if(!script)
    {
        script = psserver->GetMathScriptEngine()->FindScript("CalculateAttackPowerLevel");
        CS_ASSERT(script);
    }

    MathEnvironment env;
    env.Define("RelatedStat", attacker->GetSkillRank(type->related_stat).Current());
    env.Define("WeaponSkill", attacker->GetSkillRank(weapon->GetWeaponType()->skill).Current());
    env.Define("Range", range);
    script->Evaluate(&env);

    MathVar* power = env.Lookup("PowerLevel");
    CS_ASSERT(power);
    return power->GetValue();

}
bool psAttackRange::AffectTarget(gemObject* attacker, gemObject* origTarget, gemObject* target, float power, psCombatAttackGameEvent* event)
{
    gemActor* attackee = dynamic_cast<gemActor*>(target);
    gemActor* attker = dynamic_cast<gemActor*>(attacker);

    csString msg;
    if(!attker->IsAllowedToAttack(target,msg))
        return false;

    if(attackee)
    {
        gemNPC* targetNPC = dynamic_cast<gemNPC*>(target);
        if(targetNPC)
            psserver->GetNPCManager()->QueueAttackPerception(attker, targetNPC);
    }

    // Attack hit successfully.  Run the script.
    MathEnvironment env;
    env.Define("Actor", attacker);
    env.Define("Target", target);
    env.Define("OrigTarget", origTarget); // the epicentre of an AOE attack/original cast target
    env.Define("Power",  power);

    outcome->Run(&env);
    if(origTarget == target)
        Decay(attacker->GetCharacterData(), target->GetCharacterData(),event,true,true);
    else
        Decay(attacker->GetCharacterData(), target->GetCharacterData(),event,true,false);
    return true;
}
