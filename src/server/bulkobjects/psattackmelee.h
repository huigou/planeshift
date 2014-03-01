/*
 * psattackmelee.cpp              creator hartra344@gmail.com
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
#ifndef psAttackMelee_HEADER
#define psAttackMelee_HEADER

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



class psAttackMelee: public psAttack
{
public:

    psAttackMelee();
    ~psAttackMelee();

    /**
     * Loads data from the database, but since this is a default attack, nothing is loaded at teh moment.
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
    /**
     * NYI
     *
     */
    bool CalculateSuccess(psCombatAttackGameEvent* event);

    bool IsQueuedInClient()
    {
        return true;
    }

private:
    bool AffectTarget(gemObject* attacker, gemObject* origTarget, gemObject* target, float power, psCombatAttackGameEvent* event);
    void Decay(psCharacter* attacker_data, psCharacter* target_data,psCombatAttackGameEvent* event, bool hit,bool affectplayer = true);
    MathScript* calc_decay;
    float PowerLevel(psCharacter* attacker, psItem* weapon) const;
    csString Animation; ///< possible attack animation

    csRef< psQuestPrereqOp > requirements; ///< all non weapon based attack requirements.
    csRef< psQuestPrereqOp >  TypeRequirements; ///< all Attack Type based requirements(not handled as a script)

    int MaxPower; ///< the maximum amount of power an attack can have

    /// AOE Radius: (Power, WaySkill, RelatedStat) -> Meters
    MathExpression* aoeRadius;
    /// AOE Angle: (Power, WaySkill, RelatedStat) -> Degrees
    MathExpression* aoeAngle;
    ///Chance of Success
    MathExpression* successChance;
    /// The progression script: (Power, Caster, Target) -> (side effects)
    ProgressionScript* outcome;


};
#endif
