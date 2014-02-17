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


#include <psconfig.h>
#include "psattack.h"
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

//=============================================================================
// Local Includes
//=============================================================================
#include "psquestprereqops.h"



/**********************************************************************************************************/

psCombatAttackGameEvent::psCombatAttackGameEvent(int delayticks,
        psAttack* attack,
        gemObject* attacker,
        gemObject* target,
        INVENTORY_SLOT_NUMBER weaponslot,
        int attackerCID,
        int targetCID) : psGameEvent(0,delayticks,"psCombatAttackGameEvent")
{
    this->attacker = attacker;
    this->attack = attack;
    this->target = target;
    this->attackerdata = attacker->GetCharacterData();
    this->targetdata = target->GetCharacterData();
    this->WeaponSlot = weaponslot;
    this->weapon = attackerdata->Inventory().GetEffectiveWeaponInSlot(weaponslot);
    this->AttackerCID = attackerCID;
    this->TargetCID = targetCID;

    AttackLocation=PSCHARACTER_SLOT_NONE;
    FinalDamage=-1;
    AttackResult=ATTACK_NOTCALCULATED;

    target->RegisterCallback(this);
    attacker->RegisterCallback(this);

}
psCombatAttackGameEvent::~psCombatAttackGameEvent()
{
    if(target)
    {
        target->UnregisterCallback(this);
    }
    if(attacker)
    {
        attacker->UnregisterCallback(this);
    }
}
void psCombatAttackGameEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    if(target)
    {
        target->UnregisterCallback(this);
    }
    if(attacker)
    {
        attacker->UnregisterCallback(this);
    }

    target = NULL;
    attacker = NULL;
}
void psCombatAttackGameEvent::Trigger()
{
    if(!attacker.IsValid() || !target.IsValid())
        return;

    psserver->GetCombatManager()->HandleCombatEvent(this);
}
