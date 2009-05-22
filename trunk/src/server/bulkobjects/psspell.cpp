/*
 * psspell.cpp
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
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/math.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/mathscript.h"
#include "util/psdatabase.h"

#include "../psserver.h"
#include "../gem.h"
#include "../client.h"
#include "../cachemanager.h"
#include "../commandmanager.h"
#include "../progressionmanager.h"
#include "../npcmanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psspell.h"
#include "psglyph.h"
#include "psguildinfo.h"

#define SPELL_TOUCH_RANGE   3.0

psSpell::psSpell()
{
    castDuration = NULL;
    range        = NULL;
    aoeRadius    = NULL;
    aoeAngle     = NULL;
}

psSpell::~psSpell()
{
    if (castDuration)
    {
        delete castDuration;
        castDuration = NULL;
    }
    if (range)
    {
        delete range;
        range = NULL;
    }
    if (aoeRadius)
    {
        delete aoeRadius;
        aoeRadius = NULL;
    }
    if (aoeAngle)
    {
        delete aoeAngle;
        aoeAngle = NULL;
    }
}

bool psSpell::Load(iResultRow& row)
{
    id = row.GetInt("id");

    name = row["name"];

    way = CacheManager::GetSingleton().GetWayByID(row.GetInt("way_id"));
    image         = row["image_name"];
    description   = row["spell_description"];
    castingEffect = row["casting_effect"];
    realm         = row.GetInt("realm");
    maxPower      = row.GetInt("max_power");
    offensive     = row.GetInt("offensive")!=0;
    targetTypes   = row.GetInt("target_type");
    excludeTarget = row.GetInt("exclude_target")!=0;

    // if spell is defensive then you must be able to cast it on friends, otherwise the entry is wrong.
    csString errorMsg;
    if (!offensive)
    {
        errorMsg.Format("Non-offensive spell '%s' cannot be cast on friends/self/item!", (const char *) name);
        CS_ASSERT_MSG(errorMsg, targetTypes & TARGET_FRIEND || targetTypes & TARGET_SELF || targetTypes & TARGET_ITEM);
    }
    else
    {
        errorMsg.Format("Offensive spell '%s' cannot be cast on enemies!", (const char *) name);
        CS_ASSERT_MSG(errorMsg, targetTypes & TARGET_FOE);
    }

    // Load math expressions
    castDuration = MathExpression::Create(row["cast_duration"]);
    range        = MathExpression::Create(row["range"]);
    aoeRadius    = MathExpression::Create(row["aoe_radius"]);
    aoeAngle     = MathExpression::Create(row["aoe_angle"]);

    outcome = psserver->GetProgressionManager()->FindScript(row["outcome"]);
    CS_ASSERT(castDuration && range && aoeRadius && aoeAngle && outcome);

    // Load NPC perception data
    npcSpellCategoryID    = row.GetInt("cstr_npc_spell_category");
    npcSpellRelativePower = row.GetFloat("npc_spell_power");
    npcSpellCategory      = CacheManager::GetSingleton().FindCommonString(npcSpellCategoryID);

    // Load glyph sequence/assembler info
    Result glyphs(db->Select("SELECT * from spell_glyphs WHERE spell_id=%d ORDER BY position ASC",id));
    if (glyphs.IsValid())
    {
        unsigned int i;
        for (i=0;i<glyphs.Count();i++)
        {
            psItemStats * stats = CacheManager::GetSingleton().GetBasicItemStatsByID(glyphs[i].GetInt("item_id"));
            if (stats)
            {
                glyphList.Push(stats);
            }
            else
            {
                Error3("Can't find item stats for item ID(%d) for Spell %s!", glyphs[i].GetInt("item_id"), name.GetData());
                return false;
            }
        }
    }

    return true;
}

float psSpell::PowerLevel(psCharacter *caster, float kFactor) const
{
    static MathScript *script = NULL;
    if (!script)
    {
        script = psserver->GetMathScriptEngine()->FindScript("CalculatePowerLevel");
        CS_ASSERT(script);
    }

    MathEnvironment env;
    env.Define("KFactor", kFactor);
    env.Define("WaySkill", caster->GetSkillRank(way->skill).Current());
    script->Evaluate(&env);

    MathVar *power = env.Lookup("PowerLevel");
    CS_ASSERT(power);
    return power->GetValue();
}

float psSpell::ManaCost(psCharacter *caster, float kFactor) const
{
    static MathScript *script = NULL;
    if (!script)
    {
        script = psserver->GetMathScriptEngine()->FindScript("CalculateManaCost");
        CS_ASSERT(script);
    }

    MathEnvironment env;
    env.Define("KFactor",     kFactor);
    env.Define("Realm",       realm);
    env.Define("RelatedStat", caster->Stats()[way->related_stat].Current());
    env.Define("WaySkill",    caster->GetSkillRank(way->skill).Current());
    script->Evaluate(&env);

    MathVar *manaCost = env.Lookup("ManaCost");
    CS_ASSERT(manaCost);
    return manaCost->GetValue();
}

float psSpell::ChanceOfCastSuccess(psCharacter *caster, float kFactor) const
{
    static MathScript *script = NULL;
    if (!script)
    {
        script = psserver->GetMathScriptEngine()->FindScript("CalculateChanceOfCastSuccess");
        CS_ASSERT(script);
    }

    MathEnvironment env;
    env.Define("KFactor",     kFactor);
    env.Define("Realm",       realm);
    env.Define("RelatedStat", caster->Stats()[way->related_stat].Current());
    env.Define("WaySkill",    caster->GetSkillRank(way->skill).Current());
    script->Evaluate(&env);

    MathVar *chance = env.Lookup("ChanceOfSuccess");
    CS_ASSERT(chance);
    return chance->GetValue();
}

float psSpell::ChanceOfResearchSuccess(psCharacter *researcher)
{
    if (!researcher->CheckMagicKnowledge(way->skill, realm))
        return 0.0;

    static MathScript *script = NULL;
    if (!script)
    {
        script = psserver->GetMathScriptEngine()->FindScript("CalculateChanceOfResearchSuccess");
        CS_ASSERT(script);
    }

    MathEnvironment env;
    env.Define("Spell", this);
    env.Define("WaySkill", way->skill);
    script->Evaluate(&env);

    MathVar *chance = env.Lookup("ChanceOfSuccess");
    CS_ASSERT(chance);
    return chance->GetValue();
}

bool psSpell::MatchGlyphs(const csArray<psItemStats*> & assembler)
{
    if (assembler.GetSize() != glyphList.GetSize())
        return false;

    for (size_t i = 0; i < glyphList.GetSize(); i++)
    {
        if (assembler[i] != glyphList[i])
        {
            return false;
        }
    }
    return true;
}

bool psSpell::CanCast(Client *client, float kFactor, csString & reason)
{
    gemActor *caster = client->GetActor();
    psCharacter *casterChar = caster->GetCharacterData();

    const int mode = caster->GetMode();
    if (mode != PSCHARACTER_MODE_PEACE && mode != PSCHARACTER_MODE_COMBAT)
    {
        reason.Format("You can't cast spells while %s.", caster->GetModeStr());
        return false;
    }

    if (caster->IsSpellCasting())
    {
        reason = "You are already casting a spell.";
        return false;
    }

    // Check for sufficient Mana
    if (!caster->infinitemana)
    {
        float manaCost = ManaCost(casterChar, kFactor);
        if (casterChar->GetMana() < manaCost)
        {
            reason.Format("You don't have the mana to cast %s.", name.GetData());
            return false;
        }

        if (casterChar->GetStamina(false) < manaCost)
        {
            reason.Format("You are too tired to cast %s.", name.GetData());
            return false;
        }
    }

    // Skip testing some conditions for developers and game masters
    if (!CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "cast all spells"))
    {
        if (!casterChar->CheckMagicKnowledge(way->skill, realm))
        {
            reason = "You have insufficient knowledge of this magic way to cast this spell.";
            return false;
        }

        if (!casterChar->Inventory().HasPurifiedGlyphs(glyphList))
        {
            reason.Format("You don't have the purified glyphs to cast %s.", name.GetData());
            return false;
        }
    }

    return true;
}

void psSpell::Cast(Client *client, float kFactor) const
{
    gemActor *caster = client->GetActor();
    gemObject *target = client->GetTargetObject();

    if (offensive && !client->IsAllowedToAttack(target,true))  // this function sends sys error msg
        return;

    float power = MIN(maxPower, PowerLevel(caster->GetCharacterData(), kFactor));
    float skill = caster->GetCharacterData()->GetSkillRank(way->skill).Current();
    float stat = caster->GetCharacterData()->Stats()[way->related_stat].Current();
    
    MathEnvironment env;
    env.Define("Power",       power);
    env.Define("WaySkill",    skill);
    env.Define("RelatedStat", stat);
    float max_range = range->Evaluate(&env);
    float castDurationFloat = castDuration->Evaluate(&env);
    csTicks castingDuration = (csTicks) MAX(castDurationFloat, 0);

    if (max_range <= 0)
        target = caster;

    if (!target)
    {
        if (targetTypes & TARGET_SELF)
        {
            target = caster;
        }
        else
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You must select a target for %s", name.GetData());
            return;
        }
    }

    // Check for the right kind of target
    const int targetType = client->GetTargetType(target);
    if (!(targetTypes & targetType))
    {
        csString allowedTypes;
        client->GetTargetTypeName(targetTypes, allowedTypes);
        psserver->SendSystemInfo(client->GetClientNum(), "You cannot cast %s on %s. You can only cast it on %s.", 
                                 name.GetData(), target ? target->GetName() : "that", allowedTypes.GetData());
        return;
    }

    if (max_range > 0 && caster->RangeTo(target) > max_range)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "%s is too far away to cast %s.", target->GetName(), name.GetData());
        return;
    }

    // All conditions for casting this spell are met!
    caster->SetMode(PSCHARACTER_MODE_SPELL_CASTING, castingDuration);
    psserver->SendSystemInfo(client->GetClientNum(), "You start casting the spell %s", name.GetData());

    // Allow developers and game masters to cast a spell immediately
    if (client->GetActor()->instantcast)
    {
        castingDuration = 0;
    }

    if (!castingEffect.IsEmpty() && castingDuration > 0)
    {
        psEffectMessage fx(0, castingEffect, csVector3(0,0,0), caster->GetEID(), target->GetEID(), castingDuration, 0);
        fx.Multicast(caster->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }

    psSpellCastGameEvent *evt = new psSpellCastGameEvent(this, client, target, castingDuration, max_range, kFactor, power);
    evt->QueueEvent();
}

void psSpell::Affect(gemActor *caster, gemObject *target, float range, float kFactor, float power) const
{
    const float chanceOfSuccess = ChanceOfCastSuccess(caster->GetCharacterData(), kFactor);
    Notify4(LOG_SPELLS, "%s casting %s with a chance of success = %.2f\n", caster->GetName(), name.GetData(), chanceOfSuccess);

    if (psserver->GetRandom() * 100.0 > chanceOfSuccess)
    {
        // Spell casting failed
        psserver->SendSystemInfo(caster->GetClientID(), "You failed to cast the spell %s." , name.GetData());

        psEffectMessage fx(0, "spell_failure", csVector3(0,0,0), caster->GetEID(), target->GetEID(), 0, 0);
        fx.Multicast(caster->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);

        // Only drain 10% of mana.
        caster->DrainMana(-(ManaCost(caster->GetCharacterData(), kFactor)/10), false);

        // Spell casting complete, we are now in PEACE mode again.
        caster->SetMode(PSCHARACTER_MODE_PEACE);
        caster->SetSpellCasting(NULL);
        return;
    }

    // Drain full mana amount.
    caster->DrainMana(-ManaCost(caster->GetCharacterData(), kFactor), false);

    // Look for targets
    MathEnvironment env;
    env.Define("Power",       power);
    env.Define("WaySkill",    caster->GetCharacterData()->GetSkillRank(way->skill).Current());
    env.Define("RelatedStat", caster->GetCharacterData()->Stats()[way->related_stat].Current());
    float radius = aoeRadius->Evaluate(&env);
    float angle  = aoeAngle->Evaluate(&env);

    if (radius < 0.01f) // single target
    {
        if (target && caster->RangeTo(target) <= range)
        {
            if (AffectTarget(caster, target, power))
            {
                caster->GetCharacterData()->Skills().AddSkillPractice(way->skill, 1);
            }
        }
        else
        {
            psserver->SendSystemInfo(caster->GetClientID(), "%s is too far away for your spell to reach.", target ? target->GetName() : "Your target");
        }
    }
    else // AOE (Area of Effect)
    {
        csVector3 pos;
        float yrot; // in radians
        iSector *sector;

        caster->GetPosition(pos, yrot, sector);

        if (angle <= 0 || angle > 360)
            angle = 360;

        angle = (angle/2)*(PI/180); // convert degrees to radians, half on each side of the casters yrot
        //CPrintf(CON_DEBUG, "Spell has an effect arc of %1.2f radians to either side of LOS.\n", angle);

        int affectedCount = 0;

        csArray<gemObject*> nearby = GEMSupervisor::GetSingleton().FindNearbyEntities(sector, pos, radius);
        for (size_t i = 0; i < nearby.GetSize(); i++)
        {
            if (excludeTarget && target == nearby[i])
                continue;

            if (!(targetTypes & caster->GetClient()->GetTargetType(nearby[i])))
                continue;

            csVector3 targetPos;
            iSector *targetSector;
            nearby[i]->GetPosition(targetPos, targetSector);

            if (angle < 2*PI)
            {
                csVector3 TP; // Target - Player pos.
                csVector3 ATP; // Affected Target - Player pos.
                target->GetPosition(TP, sector);
                TP = TP - pos;
                ATP = targetPos - pos;

                // Angle between the target fired at, and this potential "in the way" target.
                float cosATAngle = TP*ATP / (TP.Norm()*ATP.Norm());
                if (cosATAngle > 1 || csNaN(cosATAngle))
                    cosATAngle = 1.0f;
                if (cosATAngle < -1)
                    cosATAngle = -1.0f;

                if (acosf(cosATAngle) >= angle)
                    continue;
            }

            if (AffectTarget(caster, nearby[i], power))
                affectedCount++;
        }

        if (affectedCount > 0)
        {
            caster->GetCharacterData()->Skills().AddSkillPractice(way->skill, 1);
            psserver->SendSystemInfo(caster->GetClientID(), "%s affected %d %s.", name.GetData(), affectedCount, (affectedCount == 1) ? "target" : "targets");
        }
        else
        {
            psserver->SendSystemInfo(caster->GetClientID(), "%s has no effect.", name.GetData());
        }
    }

    // Spell casting complete, we are now in PEACE mode again.
    caster->SetMode(PSCHARACTER_MODE_PEACE);
    caster->SetSpellCasting(NULL);
}

bool psSpell::AffectTarget(gemActor *caster, gemObject *target, float power) const
{
    if (offensive)
    {
        if (!caster->GetClient()->IsAllowedToAttack(target,true))
            return false;

        gemActor *attackee = dynamic_cast<gemActor*>(target);
        if (attackee)
        {
            attackee->AddAttackerHistory(caster, 1.0); // ???: Arbitrary number, we don't have a return value from the script like we used to

            gemNPC *targetNPC = dynamic_cast<gemNPC*>(target);
            if (targetNPC)
                psserver->GetNPCManager()->QueueAttackPerception(caster, targetNPC);
        }
    }

    // Spell hit successfully.  Run the script.
    MathEnvironment env;
    env.Define("Caster", caster);
    env.Define("Target", target);
    env.Define("Power",  power);

    outcome->Run(&env);

    // ???: If the NPC had a saving throw, I don't think we're supposed to tell them.  But we can't know if they did or not.
    psserver->GetNPCManager()->QueueSpellPerception(caster,target,npcSpellCategory,npcSpellCategoryID,npcSpellRelativePower);

    return true;
}

///
/// iScriptableVar Interface Implementation
///
double psSpell::GetProperty(const char *ptr)
{
    if (!strcasecmp(ptr,"Realm"))
    {
        return realm;
    }
    else if (!strcasecmp(ptr,"Way"))
    {
        return way->id;
    }
    Error2("Requested psSpell property not found '%s'", ptr);
    return 0;
}

double psSpell::CalcFunction(const char * functionName, const double * params)
{
    Error2("psSpell::CalcFunction(%s) failed", functionName);
    return 0;
}

//-----------------------------------------------------------------------------

psSpellCastGameEvent::psSpellCastGameEvent(const psSpell *spell,
                                           Client *caster,
                                           gemObject *target,
                                           csTicks castingDuration,
                                           float max_range,
                                           float kFactor,
                                           float powerLevel)
    : psGameEvent(0, castingDuration, "psSpellCastGameEvent")
{
    this->spell      = spell;
    this->caster     = caster;
    this->target     = target;
    valid            = true;
    this->max_range  = max_range;
    this->kFactor    = kFactor;
    this->powerLevel = powerLevel;

    target->RegisterCallback(this);
    caster->GetActor()->RegisterCallback(this);
    caster->GetActor()->SetSpellCasting(this);
}

psSpellCastGameEvent::~psSpellCastGameEvent()
{
    if (target)
    {
        target->UnregisterCallback(this);
    }
    if (caster)
    {
        caster->GetActor()->UnregisterCallback(this);
    }
}

void psSpellCastGameEvent::DeleteObjectCallback(iDeleteNotificationObject * object)
{
    if (target)
    {
        target->UnregisterCallback(this);
    }
    if (caster)
    {
        caster->GetActor()->UnregisterCallback(this);
    }

    Interrupt();

    target = NULL;
    caster = NULL;
}

void psSpellCastGameEvent::Interrupt()
{
    if (!IsValid())
        return;

    if (target && !target->IsAlive())
    {
        psserver->SendSystemError(caster->GetClientNum(), "%s is already dead.", (const char*) target->GetName());
    }
    else
    {
        psserver->SendSystemInfo(caster->GetClientNum(), "Your spell (%s) has been interrupted!", spell->GetName().GetData());
    }

    caster->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
    caster->GetActor()->SetSpellCasting(NULL);

    // Stop event from beeing executed when trigged.
    SetValid(false);
}

void psSpellCastGameEvent::Trigger()
{
    spell->Affect(caster->GetActor(), target, max_range, kFactor, powerLevel);
}

