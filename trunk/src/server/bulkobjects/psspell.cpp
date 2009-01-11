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
#include "util/strutil.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/psxmlparser.h"
#include "util/mathscript.h"

#include "../psserver.h"
#include "../gem.h"
#include "../client.h"
#include "../cachemanager.h"
#include "../progressionmanager.h"
#include "../npcmanager.h"
#include "../playergroup.h"
#include "../spellmanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psspell.h"
#include "psglyph.h"
#include "psguildinfo.h"
#include "commandmanager.h"

#define SPELL_TOUCH_RANGE   3.0

psSpell::psSpell()
{
    saveThrow = 0;
}

psSpell::~psSpell()
{
    delete saveThrow;
}

bool psSpell::Load(iResultRow& row)
{
    id = row.GetInt("id");

    name =row["name"];

    way = CacheManager::GetSingleton().GetWayByID(row.GetInt("way_id"));
    caster_effect     = row["caster_effect"];
    target_effect     = row["target_effect"];
    image               = row["image_name"];
    description         = row["spell_description"];
    progression_event   = row["progression_event"];
    saved_progression_event = row["saved_progression_event"];


    //casting_duration    = row.GetInt("casting_duration");
    realm               = row.GetInt("realm");
    //interval_time       = row.GetInt("interval_time");
    max_power           = row.GetInt("max_power");
    offensive           = row.GetInt("offensive") ? true : false;
    spell_target        = row.GetInt("target_type");
    
    // if spell is defensive then you must be able to cast it on friends, otherwise the entry is wrong.
    csString errorMsg;
    if(!offensive)
    {
        errorMsg.Format("Non-offensive spell '%s' cannot be cast on friends/self/item!", (const char *) name);  
        CS_ASSERT_MSG(errorMsg, (spell_target & TARGET_FRIEND) > 0 || (spell_target & TARGET_SELF) > 0 || (spell_target & TARGET_ITEM) > 0);
    }
    else
    {
        errorMsg.Format("Offensive spell '%s' cannot be cast on enemies!", (const char *) name);
        CS_ASSERT_MSG(errorMsg, (spell_target & TARGET_PVP) > 0 || (spell_target & TARGET_FOE) > 0);
    }

    npcSpellCategoryID    = row.GetInt("cstr_npc_spell_category");
    npcSpellRelativePower = row.GetFloat("npc_spell_power");
    npcSpellCategory      = CacheManager::GetSingleton().FindCommonString(npcSpellCategoryID);

    mathScript = psserver->GetMathScriptEngine()->FindScript(name);
    if ( !mathScript )
    {
        Warning2(LOG_SPELLS, "Can't find math script for: %s", name.GetData() );
        return false;
    }

    manaScript = psserver->GetMathScriptEngine()->FindScript("CalculateManaCost");
    if ( !manaScript )
    {
        Warning1(LOG_SPELLS, "Can't find mana script!");
        return false;
    }

    castSuccessScript = psserver->GetMathScriptEngine()->FindScript("CalculateChanceOfCastSuccess");
    if ( !castSuccessScript )
    {
        Warning1(LOG_SPELLS, "Can't find cast success script!");
        return false;
    }

    varRange            = mathScript->GetOrCreateVar("Range");
    varDuration         = mathScript->GetOrCreateVar("Duration");
    varCastingDuration  = mathScript->GetOrCreateVar("CastingDuration");
    varPowerLevel       = mathScript->GetOrCreateVar("PowerLevel");
    varAntiMagic        = mathScript->GetOrCreateVar("AntiMagic");
    varAffectRange      = mathScript->GetOrCreateVar("AffectRange");
    varAffectAngle      = mathScript->GetOrCreateVar("AffectAngle");
    varAffectTypes      = mathScript->GetOrCreateVar("AffectTypes");
    varProgressionDelay = mathScript->GetOrCreateVar("ProgressionDelay");
    varUseSaveThrow     = mathScript->GetOrCreateVar("UseSaveThrow");
    varWaySkill         = mathScript->GetOrCreateVar("WaySkill");

    csString paramName;
    paramName.Format("Param%d", 0 );
    int x = 1;
    MathScriptVar* param = mathScript->GetVar(paramName);
    while ( param )
    {
        varParams.Push( param );
        paramName.Format("Param%d", x++ );
        param = mathScript->GetVar(paramName);
    }


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
                Error3("Can't find item stats for item ID(%d) For Spell %s!", glyphs[i].GetInt("item_id"), name.GetData());
            }
        }
    }

    csString savingThrow = row["saving_throw"];
    if ( savingThrow.Length() > 0 )
    {
        saveThrow = new SavingThrow;
        // It is safe to do both since it will just set it to *_NONE
        saveThrow->skillSave = CacheManager::GetSingleton().ConvertSkillString( savingThrow );
        saveThrow->statSave = CacheManager::GetSingleton().ConvertAttributeString( savingThrow );
        saveThrow->value = row.GetInt("saving_throw_value");
    }

    return true;
}

float psSpell::ManaCost(float KFactor) const
{
    MathScriptVar *kVar = manaScript->GetOrCreateVar("KFactor");
    MathScriptVar *realmVar = manaScript->GetOrCreateVar("Realm");
    MathScriptVar *manaVar = manaScript->GetOrCreateVar("ManaCost");

    kVar->SetValue((double)KFactor);
    realmVar->SetValue((double)realm);
    // Calculate the mana cost
    manaScript->Execute();
    // Done
    return (float)manaVar->GetValue();
}

float psSpell::ChanceOfSuccess(float KFactor, float waySkill, float relatedStat) const {
    MathScriptVar *kVar = castSuccessScript->GetOrCreateVar("KFactor");
    MathScriptVar *realmVar = castSuccessScript->GetOrCreateVar("Realm");
    MathScriptVar *waySkillVar = castSuccessScript->GetOrCreateVar("WaySkill");
    MathScriptVar *relatedStatVar = castSuccessScript->GetOrCreateVar("RelatedStat");
    MathScriptVar *chanceOfSuccess = castSuccessScript->GetOrCreateVar("ChanceOfSuccess");

    kVar->SetValue((double) KFactor);
    realmVar->SetValue((double) realm);
    waySkillVar->SetValue((double) waySkill);
    relatedStatVar->SetValue((double) relatedStat);

    castSuccessScript->Execute();
    return (float) chanceOfSuccess->GetValue();
}

PSSKILL psSpell::GetSkill() const
{
    return way->skill;
}

PSSKILL psSpell::GetRelatedStat() const
{
    return way->related_stat;
}

bool psSpell::MatchGlyphs(const glyphList_t & assembler)
{
    if (assembler.GetSize() != glyphList.GetSize()) return false;

    for (size_t i = 0; i < glyphList.GetSize(); i++)
    {
        if (assembler[i] != glyphList[i])
        {
            return false;
        }
    }
    return true;
}

psSpellCastGameEvent *psSpell::Cast(SpellManager * mgr, Client * client, csString &effectName,
    csVector3 &offset, EID &anchorID, EID &targetID, unsigned int &castingDuration, csString & castingText) const
{
    gemActor *caster = client->GetActor();
    gemObject *target = client->GetTargetObject();
    int mode = caster->GetCharacterData()->GetMode();

    if (!target)
    {
        if ( spell_target & TARGET_SELF )
            target = caster;
        else
        {
            castingText.Format("You must select a target for %s", name.GetData());
            return NULL;
        }            
    }

    if (mode != PSCHARACTER_MODE_PEACE  &&  mode != PSCHARACTER_MODE_COMBAT)
    {
        castingText.Format("You can't cast spells while %s.", caster->GetCharacterData()->GetModeStr());
        return NULL;
    }

    if (caster->GetCharacterData()->IsSpellCasting())
    {
        castingText = "You are already casting a spell.";
        return NULL;
    }

    // Skip testing some conditions for developers and game masters
    const bool gameMaster = CacheManager::GetSingleton().GetCommandManager()->Validate(client->GetSecurityLevel(), "cast all spells");
    if (!gameMaster)
    {
        if (! caster->GetCharacterData()->CheckMagicKnowledge(GetSkill(), realm))
        {
            castingText = "You have insufficient knowledge of this magic way to cast this spell.";
            return NULL;
        }

        // Check if needed glyphs are available
        if (!caster->GetCharacterData()->Inventory().HasPurifiedGlyphs(glyphList))
        {
            castingText.Format("You don't have the purified glyphs to cast %s.",name.GetData());
            return NULL;
        }
    }

    if (!client->GetActor()->infinitemana)
    {
        // Check for available Mana
        if (caster->GetCharacterData()->GetMana() < ManaCost(caster->GetCharacterData()->GetKFactor()))
        {
            castingText.Format("You don't have the mana to cast %s.", name.GetData());
            return NULL;
        }

        if (caster->GetCharacterData()->GetStamina(false) < ManaCost(caster->GetCharacterData()->GetKFactor()))
        {
            castingText.Format("You are too tired to cast %s.", name.GetData());
            return NULL;
        }
    }

    float powerLevel = MIN(max_power, caster->GetCharacterData()->GetPowerLevel(way->skill) );
    float waySkill = caster->GetCharacterData()->GetSkillRank(way->skill);

    // Set Input variables to script
    varWaySkill->SetValue(waySkill);
    varPowerLevel->SetValue(powerLevel);
    if (target && target->GetCharacterData())
        varAntiMagic->SetValue(target->GetCharacterData()->GetSkillRank(PSSKILL_ANTIMAGIC));
    else
        varAntiMagic->SetValue(0);

    varUseSaveThrow->SetValue(0.0); // Unknown at this point. Will be set when script is executed
                                    // as part of the effect.
    mathScript->Execute();

    powerLevel = varPowerLevel->GetValue();
    float max_range = varRange->GetValue();
    csTicks duration = (csTicks)varDuration->GetValue();

    // If you can only cast it on yourself, do so.
    if ( max_range == -1 )
        target = caster;

    // Check the Target
    int target_type = client->GetTargetType( target );

    if (offensive && !client->IsAllowedToAttack(target,true))  // this function sends sys error msg
        return NULL;

    // Check if this spell needs a target and if so, it is the correct type
    // noone can override the item target limit
    if (  (((spell_target & TARGET_ITEM) == 0) && (target_type == TARGET_ITEM)) ||
        (( (spell_target & TARGET_NONE) == 0 ) && ( (spell_target & target_type) == 0) && !client->IsGM()))
    {
        csString targetTypeName;
        client->GetTargetTypeName( spell_target, targetTypeName );

        if(target)
            castingText.Format("You cannot cast %s on %s. You can only cast it on %s.", 
                               name.GetData(), target->GetName(), targetTypeName.GetData());
        else
            castingText.Format("You cannot cast %s . You can only cast it on %s.", 
                               name.GetData(), targetTypeName.GetData());
        return NULL;
    }
    
    if ( isTargetAffected( client, target, max_range, castingText ) )
    {
        // All conditions for casting this spell are met!
        // START CASTING SPELL
        
        caster->SetMode( PSCHARACTER_MODE_SPELL_CASTING );
        castingText.Format("You start casting the spell %s", name.GetData());

        effectName = caster_effect;
        offset = csVector3(0,0,0);
        anchorID = caster->GetEID();
        targetID = target->GetEID();

        // Allow developers and game masters to cast a spell immediately
        if (client->GetActor()->instantcast)
        {
            castingDuration = 0;
        }
        else
        {
            castingDuration = (int)varCastingDuration->GetValue();
        }

        return new psSpellCastGameEvent( mgr, this, client, target, castingDuration, max_range, powerLevel, duration);
    }

    return NULL;
}

bool psSpell::isTargetAffected(Client *client, gemObject *target, float max_range, csString & castingText) const
{
    gemActor *caster = client->GetActor();


    if (target)
    {
        // TODO: How to handle spell travel time?
        // Check for target within range

        if ( max_range == -1 && caster != target) // Self
        {
            castingText.Format("You can only cast %s on yourself.", name.GetData());
            return false;
        }
        else if (max_range == 0 && (caster->RangeTo(target) > SPELL_TOUCH_RANGE)) // Touch
        {
            castingText.Format("You are not in touch range of target %s to cast %s.", target->GetName(), name.GetData());
            return false;
        }
        else if (max_range > 0 && caster->RangeTo(target) > max_range)
        {
            castingText.Format("%s is too far away to cast %s.", target->GetName(), name.GetData());
            return false;
        }
    }

    return true;
}

int psSpell::checkRange( gemActor *caster, gemObject *target, float max_range) const
{
    if (max_range == 0 && caster->RangeTo(target) > SPELL_TOUCH_RANGE) // Touch
    {
        return 1;
    }
	if (max_range > 0 && caster->RangeTo(target) > max_range)
	{
		return 1;
	}
    return 0;
}

csArray< gemObject *> *psSpell::getTargetsInRange(Client * client, float max_range, float range) const
{
    gemActor *caster = client->GetActor();
    csArray< gemObject *> *targetsInRange  = caster->GetObjectsInRange( range );

    // Cycle through list and add any entities
    // that represent players to the proximity subscription list.

    csString reason;

    size_t i = 0;
    while ( i < targetsInRange->GetSize() )
    {
        gemObject *nearbyTarget = targetsInRange->Get( i );
        if (!nearbyTarget)
            continue;

        if (!isTargetAffected(client, nearbyTarget, max_range, reason))
        {
            //CPrintf(CON_DEBUG,  reason );
            targetsInRange->DeleteIndex( i );
        }
        else
        {
            i++;
        }
    }
    return targetsInRange;
}

bool psSpell::AffectTargets(SpellManager * mgr, psSpellCastGameEvent * event, csString &effectName, csVector3 &offset, 
                            EID &anchorID, EID &targetID, csString & affectText) const
{
    gemActor * caster = event->caster->GetActor();
    gemObject * target = event->target;

    float max_range = event->max_range;

    // Handle Area of Effect Spells
    float affectRange = varAffectRange->GetValue();
    if (  ( affectRange > 0 ) ) //( (spell_target & TARGET_NONE) != 0 ) &&
    {
        csVector3 pos;
        float yrot;  // in radians
        iSector *sector;

        caster->GetPosition(pos,yrot,sector);

        // For a Cone area of Effect spell.
        float affectRadians = 0;
        if ( varAffectAngle )
        {
            affectRadians = varAffectAngle->GetValue(); // Angle in Degrees
        }
        if ( ( affectRadians <= 0 ) || ( affectRadians > 360 ) ) affectRadians = 360;
        affectRadians = ( affectRadians / 2 ) * ( PI / 180 ); // Convert Degrees to Radians, half on each side of the casters yrot
        //CPrintf(CON_DEBUG, "Spell has an effect arc of %1.2f radians to either side of LOS.\n", affectRadians );

        // Get ProximityList
        csArray< gemObject *> *targetList = getTargetsInRange(event->caster, max_range, affectRange);

        if ( targetList->GetSize() == 0 )
        {
            //CPrintf(CON_DEBUG, "No targets in range.\n");
            affectText.Format("You successfully cast spell %s, however there are no targets within its range.", name.GetData());
            return false;
        }

        // For multiple targets this tracks if any of them was effected by the spell.
        int affectedCount = 0;

        // Loop through testing for rangedness
        for ( size_t i = 0; i < targetList->GetSize(); i ++)
        {
            gemObject *affectedTarget = targetList->Get( i );
            // if in range finish checks and cast

            csVector3 targetPos;
            iSector *targetSector;

            affectedTarget->GetPosition(targetPos,targetSector);

            if(affectRadians < 2 * PI)
            {
                csVector3 TP; // Target - Player pos.
                csVector3 ATP; // Affected Target - Player pos.
                target->GetPosition(TP, sector);
                TP = TP - pos;
                ATP = targetPos - pos;

                // Angle between the target fired at, and this potential "in the way" target.
                float cosATAngle = TP*ATP / (TP.Norm()*ATP.Norm());
                if(cosATAngle > 1 || csNaN(cosATAngle))
                    cosATAngle = 1.0f;
                if(cosATAngle < -1)
                    cosATAngle = -1.0f;

                /*CPrintf(CON_DEBUG, "Target %s is %1.2f radians from LOS at a range of %1.2fm, with a cosATAngle of %1.2f.\n", affectedTarget->GetName(), acosf(cosATAngle), caster->RangeTo(affectedTarget), cosATAngle);
                if ( acosf(cosATAngle) < affectRadians )
                    CPrintf(CON_DEBUG, "Target %s is in affected area and is in range.\n", affectedTarget->GetName());
                else continue;*/
                if ( acosf(cosATAngle) >= affectRadians )
                    continue;
            }

            int target_type = event->caster->GetTargetType( affectedTarget );

            if ( ( target_type & spell_target) != 0 )
            {
                EID affectedTargetID = affectedTarget->GetEID();
                event->target = affectedTarget;
                if (offensive)
                {
                    gemNPC *targetnpc = dynamic_cast<gemNPC *>(affectedTarget);
                    if (targetnpc)
                        psserver->npcmanager->QueueAttackPerception(caster, targetnpc);
                }
                if (AffectTarget(event, effectName, offset, anchorID, affectedTargetID, affectText))
                {
                    affectedCount++;
                }
            }
        }

        if ( affectedCount > 0 )
        {
            affectText.Format("%s affected %i target(s).", name.GetData(), affectedCount);
        }
        else
        {
            affectText.Format("%s has no effect.", name.GetData() );
        }
        
        // Reset the target back to the orginal.
        event->target = target;
        return affectedCount > 0;
    }
    else // only one target
    {
        csString castingText;
        if (target && isTargetAffected(event->caster, target, max_range, castingText))
        {
            if (offensive)
            {
                gemNPC *targetnpc = dynamic_cast<gemNPC *>(target);
                if (targetnpc)
                    psserver->npcmanager->QueueAttackPerception(caster, targetnpc);
            }
            return AffectTarget(event, effectName, offset, anchorID, targetID, affectText);
        }
        return false;
    }
    return true;
}


bool psSpell::AffectTarget(psSpellCastGameEvent * event, csString &effectName, csVector3 &offset,
                           EID & anchorID, EID & targetID, csString & affectText) const
{

    ////////////////////////////////////
    // Begin Check for saving throw
    gemActor * caster = event->caster->GetActor();
    gemObject * target = event->target;

    // Make sure the target has not been claimed by another attack in the time it has taken to cast this spell.
    if (offensive && !event->caster->IsAllowedToAttack(target,true))  // this function sends sys error msg
        return false;

    // Check to make sure target is still in range
    switch ( checkRange( caster, target, event->max_range) )
    {
    case 0: // in range
        break;
    case 1: // too far
        psserver->SendSystemInfo( caster->GetClientID(), "%s out of range.", target->GetName() );
        return false;
        break;
    }

    // Check for spell failure
    bool saved = false;

    if ( saveThrow )
    {
        float random = psserver->GetRandom();
        int saveRolled = (int)(0.05 * (float)saveThrow->value );
        saveRolled +=(int)(random*(float)saveThrow->value);
        int playerAttrib = -1;

        psCharacter* targetData = target->GetCharacterData();
        if (targetData)
        {
            if ( saveThrow->statSave != PSITEMSTATS_STAT_NONE )
                playerAttrib = (int) targetData->Stats().GetStat(saveThrow->statSave);
            else if ( saveThrow->skillSave != PSSKILL_NONE )
                playerAttrib = (int) targetData->Skills().GetSkillRank(saveThrow->skillSave);
        }
        if ( playerAttrib != -1.0 )
        {
            if ( saveRolled < playerAttrib )
                saved = true;
        }
    }

    // NBNB!!! Need to recalculate the global mathscript that might have been overriden
    //         by someone else that may have cast this spell.
    // Set input variables before executing script
    if ( saved )
    {
        varUseSaveThrow->SetValue(1);
    }
    else
    {
        varUseSaveThrow->SetValue(0);
    }
    varPowerLevel->SetValue(event->powerLevel);
    mathScript->Execute();
    event->powerLevel = varPowerLevel->GetValue();

    csTicks progression_delay = (csTicks)varProgressionDelay->GetValue();

    if (progression_delay)
    {
        // The affect of this spell is delayed, Create a Game event to fire when the delay is over
       psSpellAffectGameEvent *e = new psSpellAffectGameEvent( psserver->GetSpellManager(), event->spell, 
                                                               event->caster, event->target,
                                                               progression_delay,
                                                               event->max_range, saved, event->powerLevel,
                                                               event->duration) ;
       e->QueueEvent();
    }
    else
    {
        // In this case we have a spell that lasts for a period of time
        // So we want to schedule an inverse of the progression event
        // to run when the duration time is up.
        if ( !PerformResult( caster, target, event->max_range, saved, event->powerLevel, event->duration ) )
        {
            affectText.Format("%s has no effect.", name.GetData());
            return false;
        }
    }

    effectName = target_effect;
    offset = csVector3(0,0,0);
    anchorID = caster->GetEID();
    targetID = target->GetEID();

    // Spell hit successfully.  Now let npcclient's know about it.
    psserver->GetNPCManager()->QueueSpellPerception(caster,target,npcSpellCategory,npcSpellCategoryID,npcSpellRelativePower);

    return true;
}

bool psSpell::PerformResult(gemActor *caster, gemObject *target, float max_range, bool saved, float powerLevel, csTicks duration) const
{    
    float result = 0;

    // Get either the normal or saved event
    csString eventName;
    if (saved)
        eventName = saved_progression_event;
    else
        eventName = progression_event;

    // if no event is specified, do nothing
    if (eventName.Length() > 0)
    {
        ProgressionEvent *progEvent = psserver->GetProgressionManager()->FindEvent(eventName);
        if (progEvent)
        {
            // NBNB!!! Need to recalculate the global mathscript that might have been overriden
            //         by someone else that have casted this spell.
            // Set input variables before executing script
            if (saved)
            {
                varUseSaveThrow->SetValue(1.0);
            }
            else
            {
                varUseSaveThrow->SetValue(0.0);
            }

            varPowerLevel->SetValue(powerLevel);
            mathScript->Execute();

            progEvent->CopyVariables(mathScript);

            result = progEvent->Run(caster, target, caster->GetCharacterData()->Inventory().GetItemHeld(), duration);
            if ( !result )
            {
                Notify2(LOG_SCRIPT, "Couldn't run the progression event \"%s\"", eventName.GetData());
                return false;
            }
        }
        else
        {
            Error2("Failed to find progression event \"%s\"", eventName.GetData());
            return false;
        }
    }

    if (offensive)
    {
        gemActor *attackee = dynamic_cast<gemActor*>(target);
        if ( attackee )
        {
            attackee->AddAttackerHistory( caster, result );
        }
    }

    return true;
}

csString psSpell::SpellToXML() const
{
    csString glyphs;
    for (size_t i = 0; i < glyphList.GetSize(); i++)
    {
        if (i!=0) glyphs.Append(",");
        glyphs.Append(glyphList[i]->GetImageName());
    }
    csString xml;
    csString escpxml_name = EscpXML(name);
    csString escpxml_wayname = EscpXML(way->name);
    csString escpxml_glyphs = EscpXML(glyphs);
    xml.Format("<SPELL NAME=\"%s\" WAY=\"%s\" REALM=\"%d\" GLYPHS=\"%s\"/>",
               escpxml_name.GetData(), escpxml_wayname.GetData(),realm,escpxml_glyphs.GetData());
    return xml;
}

csString psSpell::DescriptionToXML() const
{
    csString xml;
    csString escpxml_name = EscpXML(name);
    csString escpxml_image = EscpXML(image);
    csString escpxml_description = EscpXML(description);
    xml.Format("<DESCRIPTION NAME=\"%s\" IMAGE=\"%s\" DESC=\"%s\" />",
               escpxml_name.GetData(), escpxml_image.GetData(), escpxml_description.GetData());
    return xml;
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

bool psSpell::operator==(const psSpell& other) const
{
    return id == other.id;
}

bool psSpell::operator<(const psSpell& other) const
{
    return id < other.id;
}
