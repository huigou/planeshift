/*
 * psattackdefault.cpp              creator hartra344@gmail.com
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
#ifndef psAttackDefault_HEADER
#define psAttackDefault_HEADER

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
#include "psattack.h"

using namespace CS;



/** a child class of psAttack
 * Controls the default base attack
 */
class psAttackDefault : public psAttack
{
public:

    psAttackDefault();
    ~psAttackDefault();

    /**
     * Loads data from the database, btu since this is a default attack, nothing is loaded at teh moment.
     */
    bool Load(iResultRow &row);

    /** Performs the necessary checks on the player to make sure they meet
     *  the requirements to use this attack.
     *
     *@param client the client being checked for attack eligability
     *@reason will be returned holding the reason an attack may not be executed.
     */
    bool CanAttack(Client* client);

    bool IsDualWield(psCharacter* attacker);

    bool Attack(gemObject* attacker, gemObject* target, INVENTORY_SLOT_NUMBER slot);
    void Affect(psCombatAttackGameEvent* event);
    int CalculateAttack(psCombatAttackGameEvent* event, psItem* subWeapon = NULL);
private:
    void AffectTarget(psCombatAttackGameEvent* event,int attack_result);
    /**
     * Queues up the next attack, specific to the default attack at the moment
     */
    void QueueAttack(gemObject* attacker,INVENTORY_SLOT_NUMBER weaponslot,gemObject* target,int attackerCID, int targetCID);
    csWeakRef<MathScript> calc_damage; ///< This is the particular calculation for damage.
    csWeakRef<MathScript> calc_decay; ///< This is the particular calculation for decay.

    csWeakRef<MathScript> staminacombat;///< if the player is too tired, stop fighting. We stop if we don't have enough stamina to make an attack with the current stance.

    bool IsQueuedInClient()
    {
        return false;
    }
};
#endif
