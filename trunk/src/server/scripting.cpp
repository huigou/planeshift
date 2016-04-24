/** @file scripting.cpp
 */

/*
 * scripting.cpp - by Kenneth Graunke <kenneth@whitecape.org>
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "scripting.h"
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/engine.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <csutil/scfstr.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/psxmlparser.h"
#include "rpgrules/factions.h"
#include "bulkobjects/pstrait.h"
#include "bulkobjects/activespell.h"
#include "bulkobjects/psraceinfo.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "actionmanager.h"
#include "adminmanager.h"
#include "cachemanager.h"
#include "entitymanager.h"
#include "progressionmanager.h"
#include "tutorialmanager.h"
#include "npcmanager.h"
#include "clients.h"
#include "events.h"
#include "gem.h"
#include "psserver.h"
#include "psproxlist.h"

static bool ParseAOps(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top, csPDelArray<AppliedOp>& ops);

/**
 * \addtogroup xmlscripting
 * @{ */

//============================================================================
// Convenience Functions
//============================================================================

gemObject* GetObject(const MathEnvironment* env, const csString &varName)
{
    MathVar* var = env->Lookup(varName);
    if(!var)
        return NULL;

    gemObject* obj = dynamic_cast<gemObject*>(var->GetObject()); // cast from iScriptableVar
    return obj;
}

gemActor* GetActor(const MathEnvironment* env, const csString &varName)
{
    MathVar* var = env->Lookup(varName);
    if(!var)
        return NULL;

    gemActor* actor = dynamic_cast<gemActor*>(var->GetObject()); // cast from iScriptableVar
    return actor;
}

gemNPC* GetNPC(const MathEnvironment* env, const csString &varName)
{
    MathVar* var = env->Lookup(varName);
    if(!var)
        return NULL;

    gemNPC* npc = dynamic_cast<gemNPC*>(var->GetObject()); // cast from iScriptableVar
    return npc;
}

psCharacter* GetCharacter(const MathEnvironment* env, const csString &varName)
{
    MathVar* var = env->Lookup(varName);
    if(!var)
        return NULL;

    // Try a direct cast...
    psCharacter* c = dynamic_cast<psCharacter*>(var->GetObject()); // cast from iScriptableVar
    if(!c)
    {
        // Maybe it's an actor - if so, we can extract it...
        gemActor* a = dynamic_cast<gemActor*>(var->GetObject());
        if(a)
            c = a->GetCharacterData();
    }
    return c;
}

//============================================================================
// Events
//============================================================================

class psCancelSpellEvent : public psGameEvent
{
public:
    psCancelSpellEvent(csTicks duration, ActiveSpell* asp) : psGameEvent(0, duration, "psCancelSpellEvent"), asp(asp) { }

    void Trigger()
    {
        if(!asp.IsValid())
            return;

        if(asp->Cancel())
            delete asp;
    }

protected:
    csWeakRef<ActiveSpell> asp;
};

//============================================================================
// Applied mode operations
//============================================================================

/** @short base class for applied script operations.
 */
class AppliedOp
{
public:
    virtual ~AppliedOp() { }
    virtual bool Load(iDocumentNode* node) = 0;
    virtual void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp) = 0;
    virtual const csString GetDescription(MathEnvironment* env)
    {
        return "";
    }
};

/// A base class with a "value" attribute backed by a MathExpression.
class Applied1 : public AppliedOp
{
public:
    Applied1() : AppliedOp(), value(NULL) { }
    virtual ~Applied1()
    {
        delete value;
    }

    bool Load(iDocumentNode* node)
    {
        value = MathExpression::Create(node->GetAttributeValue("value"));
        return value != NULL;
    }

protected:
    MathExpression* value; ///< an embedded MathExpression
};

/// A base class with both "value" and a plain text "name" attribute.
class Applied2 : public Applied1
{
public:
    Applied2() : Applied1() { }
    virtual ~Applied2() { }

    bool Load(iDocumentNode* node)
    {
        name = node->GetAttributeValue("name");
        return Applied1::Load(node);
    }

protected:
    csString name; ///< plain text - name of something, like a skill or faction
};

//----------------------------------------------------------------------------

/// Applied mode vitals.
class VitalAOp : public Applied1
{
public:
    VitalAOp() : Applied1() { }
    virtual ~VitalAOp() { }

    bool Load(iDocumentNode* node)
    {
        vital = node->GetValue();
        return Applied1::Load(node);
    }

    /**
     * retrieve a description and value of the vital value
     * @return a description prefix 
     */
    virtual const csString GetDescription(MathEnvironment* env)
    {
        csString descr;
        if(vital == "mana-rate")
            descr = "Mana Rate";
        else if(vital == "pstamina-rate")
            descr = "Physical Stamina Rate";
        else if(vital == "mstamina-rate")
            descr = "Mental Stamina Rate";
        else if(vital == "hp-max")
            descr = "Bonus Max HP";
        else if(vital == "mana-max")
            descr = "Bonus Max Mana";
        else if(vital == "pstamina-max")
            descr = "Bonus Max Physical Stamina";
        else if(vital == "mstamina-max")
            descr = "Bonus Max Mental Stamina";

        float val = value->Evaluate(env);
        descr.AppendFmt(": %0.1f\n",val);
        return descr;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        VitalBuffable* buffable = NULL;
        if(vital == "mana-rate")
            buffable = &target->GetCharacterData()->GetManaRate();
        else if(vital == "pstamina-rate")
            buffable = &target->GetCharacterData()->GetPStaminaRate();
        else if(vital == "mstamina-rate")
            buffable = &target->GetCharacterData()->GetMStaminaRate();
        else if(vital == "hp-max")
            buffable = &target->GetCharacterData()->GetMaxHP();
        else if(vital == "mana-max")
            buffable = &target->GetCharacterData()->GetMaxMana();
        else if(vital == "pstamina-max")
            buffable = &target->GetCharacterData()->GetMaxPStamina();
        else if(vital == "mstamina-max")
            buffable = &target->GetCharacterData()->GetMaxMStamina();
        CS_ASSERT(buffable);

        float val = value->Evaluate(env);
        buffable->Buff(asp, val);

        asp->Add(*buffable, "<%s value=\"%f\"/>", vital.GetData(), val);
    }

protected:
    csString vital;
};

//----------------------------------------------------------------------------

/**
 * HPRateAOp - alter HP regeneration rate over time
 *
 * Unlike other vitals, \<hp-rate\> can specify an optional attacker who will
 * be recorded in the target's damage history.
 *
 * \<hp-rate attacker="Caster" value="-0.3"/>
 * \<hp-rate value="0.5"/>
 */

class HPRateAOp : public Applied1
{
public:
    HPRateAOp() : Applied1() { }
    virtual ~HPRateAOp() { }

    bool Load(iDocumentNode* node)
    {
        attacker = node->GetAttributeValue("attacker");
        return Applied1::Load(node);
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        float val = value->Evaluate(env);

        VitalBuffable &buffable = target->GetCharacterData()->GetHPRate();
        buffable.Buff(asp, val);
        asp->Add(buffable, "<hp-rate value=\"%f\"/>", val);
        if(val < 0)
            asp->MarkAsDamagingHP();

        gemActor* atk = GetActor(env, attacker); // may be NULL
        target->AddAttackerHistory(atk, val, asp->Duration());
    }

protected:
    csString attacker; ///< optional MathScript var containing the attacker
};

//----------------------------------------------------------------------------

/**
 * Applied mode stats.
 */
class StatsAOp : public Applied1
{
public:
    StatsAOp() : Applied1() { }
    virtual ~StatsAOp() { }

    bool Load(iDocumentNode* node)
    {
        statname = node->GetValue();
        if(statname == "agi")
            stat = PSITEMSTATS_STAT_AGILITY;
        else if(statname == "end")
            stat = PSITEMSTATS_STAT_ENDURANCE;
        else if(statname == "str")
            stat = PSITEMSTATS_STAT_STRENGTH;
        else if(statname == "cha")
            stat = PSITEMSTATS_STAT_CHARISMA;
        else if(statname == "int")
            stat = PSITEMSTATS_STAT_INTELLIGENCE;
        else if(statname == "wil")
            stat = PSITEMSTATS_STAT_WILL;
        else
        {
            Error2("StatsAOp doesn't know what to do with <%s> tag.", statname.GetData());
            return false;
        }

        return Applied1::Load(node);
    }

    virtual const csString GetDescription(MathEnvironment* env)
    {
        csString descr;
        if(statname == "agi")
            descr = "Agility";
        else if(statname == "end")
            descr = "Endurance";
        else if(statname == "str")
            descr = "Strength";
        else if(statname == "cha")
            descr = "Charisma";
        else if(statname == "int")
            descr = "Intelligence";
        else if(statname == "wil")
            descr = "Will";

        int val = (int)value->Evaluate(env);
        descr.AppendFmt(": %d\n",val);
        return descr;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        int val = (int) value->Evaluate(env);
        PSSKILL skill = statToSkill(stat);
        if (skill != PSSKILL_NONE)
        {
            SkillRank &buffable = target->GetCharacterData()->GetSkillRank(statToSkill(stat));
            buffable.Buff(asp, val);

            const char* strs[] = {"str", "agi", "end", "int", "wil", "cha"};
            asp->Add(buffable, "<%s value=\"%d\"/>", strs[stat], val);
        }
    }

protected:
    PSITEMSTATS_STAT stat; ///< which stat we're actually buffing
    csString statname;     ///< used for description
};

//----------------------------------------------------------------------------

/// Applied mode skills.
class SkillAOp : public Applied2
{
public:
    SkillAOp(CacheManager* cachemanager) : Applied2(), skill(PSSKILL_NONE)
    {
        this->cachemanager = cachemanager;
    }
    virtual ~SkillAOp() { }

    bool Load(iDocumentNode* node)
    {
        skillname = node->GetAttributeValue("name");
        return Applied2::Load(node);
    }

    virtual const csString GetDescription(MathEnvironment* env)
    {
        csString descr = skillname;
        
        int val = (int)value->Evaluate(env);
        // This prevents lines like "Sword: 0" from showing up in item descriptions. They can be caused by mathscripts like: <skill name="Sword" value="if(Item:Skill1 = 0,10,0)"/>
        if (val == 0)
        {
            return "";
        }
        descr.AppendFmt(": %d\n",val);
        return descr;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        MathVar* nameVar = env->Lookup(skillname);
        csString varName;

        if (nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = skillname;

        psSkillInfo* info = cachemanager->GetSkillByName(varName);

        if (!info)
        {
            Error2("Found <skill name=\"%s\">, but no such skill exists.", varName.GetDataSafe());
            return;
        }

        skill = info->id;
        int val = (int) value->Evaluate(env);
        SkillRank &buffable = target->GetCharacterData()->GetSkillRank(skill);
        buffable.Buff(asp, val);

        asp->Add(buffable, "<skill name=\"%s\" value=\"%d\"/>", varName.GetData(), val);
    }

protected:
    PSSKILL skill;
    CacheManager* cachemanager;
    csString skillname;             /// < used for description and for run().
};

//----------------------------------------------------------------------------

/// Applied variable integers.
class VariableIntAOp : public Applied2
{
public:
    VariableIntAOp() : Applied2() { }
    virtual ~VariableIntAOp() { }

    bool Load(iDocumentNode* node)
    {
        return Applied2::Load(node);
    }
    
    virtual const csString GetDescription(MathEnvironment* env)
    {
	value->Evaluate(env);
	return "";
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        int val = (int) value->Evaluate(env);
        Buffable<int> &buffable = target->GetCharacterData()->GetBuffableVariable(name);
        buffable.Buff(asp, val);

        asp->Add(buffable, "<variable name=\"%s\" value=\"%d\"/>", name.GetData(), val);
    }
};

//----------------------------------------------------------------------------

/// Applied mode support for attack and defense modifiers.
class CombatModAOp : public Applied1
{
public:
    CombatModAOp() : Applied1() { }
    virtual ~CombatModAOp() { }

    bool Load(iDocumentNode* node)
    {
        type = node->GetValue();
        return Applied1::Load(node);
    }

    virtual const csString GetDescription(MathEnvironment* env)
    {
        csString descr;
        if(type == "atk") descr = "Attack modifier";
        else if(type == "def") descr = "Defence modifier";
        
        float val = value->Evaluate(env);
        descr.AppendFmt(": %0.2f\n",val);
        return descr;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        Multiplier* mod;
        if(type == "atk")
            mod = &target->GetCharacterData()->AttackModifier();
        else if(type == "def")
            mod = &target->GetCharacterData()->DefenseModifier();
        else
            return;

        float val = value->Evaluate(env);
        mod->Buff(asp, val);
        asp->Add(*mod, "<%s value=\"%f\"/>", type.GetData(), val);
    }

protected:
    csString type;
};

//----------------------------------------------------------------------------

/// Applied mode mesh overriding.
class RaceAOp : public AppliedOp
{
public:
    RaceAOp() : AppliedOp() { }
    virtual ~RaceAOp() { }

    bool Load(iDocumentNode* node)
    {
        value = node->GetAttributeValue("value");
        sex   = node->GetAttributeValue("sex");
        return !value.IsEmpty();
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        psRaceInfo* race = psserver->GetCacheManager()->GetRaceInfoByNameGender(value,psserver->GetCacheManager()->ConvertGenderString(sex));
        target->GetCharacterData()->GetOverridableRace().Override(asp, race);
        asp->Add(target->GetCharacterData()->GetOverridableRace(), "<race value=\"%s\" sex=\"%s\"/>", value.GetData(), sex.GetData());
    }

protected:
    csString value;
    csString sex;
};

/**
 * Apply mute to client
 */
class MuteAOp : public AppliedOp
{
public:
    MuteAOp() : AppliedOp() { }
    virtual ~MuteAOp() { }

    bool Load(iDocumentNode* node)
    {
        return true;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        MuteBuffable &buffable = target->GetClient()->GetBuffableMute();
        buffable.Buff(asp, 1);
        asp->Add(buffable, "<mute />");
    }
};

/**
 * Action to freeze client in scripts
 */
class FreezeAOp : public AppliedOp
{
public:
    FreezeAOp() : AppliedOp() { }
    virtual ~FreezeAOp() { }

    bool Load(iDocumentNode* node)
    {
        return true;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {

        FrozenBuffable &buffable = target->GetBuffableFrozen();
        buffable.Buff(asp, 1);
        asp->Add(buffable, "<freeze />");
    }
};

//----------------------------------------------------------------------------

class CanSummonFamiliarAOp : public AppliedOp
{
public:
    virtual ~CanSummonFamiliarAOp() { }

    bool Load(iDocumentNode* node)
    {
        return true;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        Buffable<int> &b = target->GetCharacterData()->GetCanSummonFamiliar();
        b.Buff(asp, 1);
        asp->Add(b, "<can-summon-familiar/>");
    }
};

//----------------------------------------------------------------------------

/// A cancel action that sends the "undo" part of \<npccmd text="..." undo="..."/\>.
class NPCCancel : public iCancelAction
{
public:
    NPCCancel(PID targetID, const csString &undo) : targetID(targetID), undo(undo) { }
    virtual ~NPCCancel() { }

    void Cancel()
    {
        gemActor* target = psserver->entitymanager->GetGEM()->FindPlayerEntity(targetID);
        if(target && !undo.IsEmpty())
        {
            psserver->GetNPCManager()->QueueNPCCmdPerception(target, undo);
        }
    }

protected:
    PID targetID;
    csString undo;
};

/**
 * NPCAOp - over time npc command sending:
 *
 * \<npccmd aim="Caster" cmd="spam spam spam spam...!"/\>
 */
class NPCAOp : public AppliedOp
{
public:
    virtual ~NPCAOp() { }

    bool Load(iDocumentNode* node)
    {
        aim = node->GetAttributeValue("aim");
        cmd = node->GetAttributeValue("cmd");
        undo = node->GetAttributeValue("undo");
        return true;
    }

    virtual void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        if(target)
        {
            csString finalCmd(cmd);
            env->InterpolateString(finalCmd);
            csString undoCmd(undo);
            env->InterpolateString(undoCmd);

            psserver->GetNPCManager()->QueueNPCCmdPerception(target, finalCmd);
            NPCCancel* cancel = new NPCCancel(target->GetPID(), undoCmd);
            asp->Add(cancel, "<npccmd aim=\"%s\" cmd=\"%s\" undo=\"%s\"/>",
                     aim.GetData(), finalCmd.GetData(), undoCmd.GetData());
        }
    }

protected:
    csString aim; ///< name of the MathScript var to aim at
    csString cmd; ///< cmd to send to the npc
    csString undo; ///< cmd to send when canceled
};

//----------------------------------------------------------------------------

/// A cancel action that handles pet remove.
class PetCancel : public iCancelAction
{
public:
    PetCancel(uint32_t clientnum, size_t inx, EID old_owner)
        : clientnum(clientnum), inx(inx), owner(old_owner)
    {
    }
    virtual ~PetCancel() { }

    void Cancel()
    {
        Client* client = psserver->GetConnections()->FindAny(clientnum);
        if(client)
        {
            gemNPC* pet = dynamic_cast <gemNPC*>(client->GetPet(inx));
            if(pet)
            {
                pet->SetOwner(NULL);
                psserver->GetNPCManager()->QueueChangeOwnerPerception(pet, owner);
            }
            client->RemovePet(inx);
        }
    }

protected:
    uint32_t clientnum;
    size_t inx;
    EID owner;
};

/**
 * PetAOp - Add target to caster's list of pets:
 *
 * \<add_pet aim="Target" owner="Caster"/\>
 */
class PetAOp : public AppliedOp
{
public:
    virtual ~PetAOp() { }

    bool Load(iDocumentNode* node)
    {
        owner = node->GetAttributeValue("owner");
        return true;
    }

    virtual void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        MathVar* var = env->Lookup(owner);
        if(!var || var->Type() != VARTYPE_OBJ)
        {
            return;
        }
        gemActor* caster = dynamic_cast<gemActor*>(var->GetObject());
        if(!caster)
        {
            return;
        }
        Client* client = caster->GetClient();
        gemNPC* pet = dynamic_cast <gemNPC*>(target);
        if(client && pet)
        {
            size_t inx = client->GetNumPets();
            client->AddPet(pet);
            EID old_owner;
            if(pet->GetOwner())
            {
                old_owner = pet->GetOwner()->GetEID();
            }
            pet->SetOwner(caster);
            psserver->GetNPCManager()->QueueChangeOwnerPerception(pet, caster->GetEID());

            PetCancel* cancel = new PetCancel(client->GetClientNum(), inx, old_owner);
            asp->Add(cancel, "<add_pet owner=\"%s\"/>", owner.GetData());
        }
    }

protected:
    csString owner; ///< name the MathScript var of the owner
};

//----------------------------------------------------------------------------

/// A cancel action that sends the "undo" part of \<msg text="..." undo="..."/\>.
class MsgCancel : public iCancelAction
{
public:
    MsgCancel(uint32_t clientnum, const csString &undo) : clientnum(clientnum), undo(undo) { }
    virtual ~MsgCancel() { }

    void Cancel()
    {
        if(!undo.IsEmpty())
            psserver->SendSystemInfo(clientnum, "%s", undo.GetData());
    }
protected:
    uint32_t clientnum;
    csString undo;
};

/// Applied mode messages - with both text and undo.
class MsgAOp : public AppliedOp
{
public:

    MsgAOp() : AppliedOp() { }
    virtual ~MsgAOp() { }

    bool Load(iDocumentNode* node)
    {
        text = node->GetAttributeValue("text");
        undo = node->GetAttributeValue("undo");
        type = node->GetAttributeValue("type");
        return true;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        if(target && target->GetClientID())
        {
            csString finalText(text);
            env->InterpolateString(finalText);

            if(type == "ok")
            {
                psserver->SendSystemOK(target->GetClientID(), finalText);
            }
            else if(type == "result")
            {
                psserver->SendSystemResult(target->GetClientID(), finalText);
            }
            else if(type == "error")
            {
                psserver->SendSystemError(target->GetClientID(), finalText);
            }
            else
            {
                psserver->SendSystemInfo(target->GetClientID(), finalText);
            }

            csString finalUndo(undo);
            env->InterpolateString(finalUndo);

            MsgCancel* cancel = new MsgCancel(target->GetClientID(), finalUndo);
            asp->Add(cancel, "<msg text=\"%s\" undo=\"%s\"/>", finalText.GetData(), finalUndo.GetData());
        }
    }

protected:
    csString text; ///< message to send immediately
    csString undo; ///< message to send when canceled
    csString type; ///< message type to send
};

//----------------------------------------------------------------------------

class FxCancel : public iCancelAction
{
public:
    FxCancel(gemObject* target, uint32_t uid) : target(target), msg(uid) { }
    virtual ~FxCancel() { }

    virtual void Cancel()
    {
        // this broadcasts to different clients than the original...
        // which...probably isn't a good thing...
        msg.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }
private:
    gemObject* target;
    psStopEffectMessage msg;
};

/**
 * FxAOp - applied effects.
 *
 * Effects have both a source (i.e. caster) and target.  For applied effects,
 * the target is always the character the ActiveSpell applies to.
 *
 * The basic form is: \<fx name="Freezing Mist"/\>.  Here, the source and target
 * are both the same entity.
 *
 * For effects coming from a different source, there are two forms:
 * 1. Coming from a gem entity:
 *    \<fx name="Freezing Mist" source="Caster"/\>
 * 2. Coming from a fixed offset from the target:
 *    \<fx name="Freezing Mist" x="-4.2" y="0.3" z="-12.7"/\>
 *
 * We cannot persist the first form, since the entity may not exist when we
 * later reload the script.  But the second serves as a good approximation:
 * the spell will appear to have come from the same position as the original
 * entity.
 */
class FxAOp : public AppliedOp
{
public:
    FxAOp(CacheManager* cachemanager) : AppliedOp()
    {
        cacheManager = cachemanager;
    }
    virtual ~FxAOp() { }

    bool Load(iDocumentNode* node)
    {
        name   = node->GetAttributeValue("name");
        source = node->GetAttributeValue("source");
        if(source.IsEmpty())
        {
            offset.x = node->GetAttributeValueAsFloat("x");
            offset.y = node->GetAttributeValueAsFloat("y");
            offset.z = node->GetAttributeValueAsFloat("z");
        }
        return !name.IsEmpty();
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        gemObject* anchor = NULL;
        // Convert from the source to an offset.
        if(!source.IsEmpty())
        {
            anchor = GetObject(env, source);

            iSector* sector;
            csVector3 sourcePos;
            csVector3 targetPos;
            anchor->GetPosition(sourcePos, sector);
            target->GetPosition(targetPos, sector);
            offset = sourcePos - targetPos;
        }

        // Get a unique identifier we can use to later cancel/stop this effect.
        uint32_t uid = cacheManager->NextEffectUID();
        /*
        if (anchor)
        {
            psEffectMessage fx(0, name, csVector3(0,0,0), anchor->GetEID(), target->GetEID(), uid);
        }
        */

        // this may be backwards
        psEffectMessage fx(0, name, offset, target->GetEID(), target->GetEID(), asp->Duration(), uid);
        if(!fx.valid)
        {
            Error1("Error: <fx> could not create valid psEffectMessage\n");
            return;
        }
        fx.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);

        // One problem with the above...it doesn't get shown to people who walk up to it later.

        FxCancel* cancel = new FxCancel(target, uid);
        if(offset == 0)
        {
            asp->Add(cancel, "<fx name=\"%s\"/>", name.GetData());
        }
        else
        {
            asp->Add(cancel, "<fx name=\"%s\" x=\"%f\" y=\"%f\" z=\"%f\"/>", name.GetData(), offset.x, offset.y, offset.z);
        }
    }

protected:
    csString name;
    csString source;  ///< The name of the MathVar containing the entity the effect originates from (optional)
    csVector3 offset; ///< An offset from the target which the effect originates from

    CacheManager* cacheManager;
};

//----------------------------------------------------------------------------

/// A cancel action that unregisters an event triggered spell.
class OnCancel : public iCancelAction
{
public:
    OnCancel(gemActor* actor, SCRIPT_TRIGGER type, ProgressionScript* script) : actor(actor), script(script), type(type) { }
    virtual ~OnCancel() { }

    void Cancel()
    {
        if(!actor.IsValid())
            return;

        actor->DetachScript(script, type);
    }
protected:
    csWeakRef<gemActor> actor;
    ProgressionScript* script;
    SCRIPT_TRIGGER type;
};

/** Support for event triggered scripts.
 *  This really deserves quite a bit of commentary.
 */
class OnAOp : public AppliedOp
{
public:
    OnAOp(EntityManager* entitymanager, CacheManager* cachemanager) : AppliedOp()
    {
        this->entitymanager = entitymanager;
        this->cachemanager = cachemanager;
    }
    virtual ~OnAOp() { }

    bool Load(iDocumentNode* node)
    {
        csString typ(node->GetAttributeValue("type"));
        if(typ == "attack")
            type = ATTACK;
        else if(typ == "defense")
            type = DEFENSE;
        else if(typ == "nearlydead")
            type = NEARLYDEAD;
        else if(typ == "move")
            type = MOVE;
        else
        {
            Error2("Invalid type in <on type=\"%s\">.", typ.GetData());
            return false;
        }

        self = node;
        return true;
    }

    void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        // Substitute any @{...} expressions.
        if(!Quasiquote(self, env))
            return;

        // Now, parse and load the script (but don't run it)...
        ProgressionScript* body = ProgressionScript::Create(entitymanager, cachemanager, "<on> body", self);
        CS_ASSERT_MSG("<on> body failed to load", body);

        // Register the triggering event
        target->AttachScript(body, type);
        OnCancel* cancel = new OnCancel(target, type, body);
        csString xml = GetNodeXML(self); // this doesn't give <hp/> style attributes...should fix
        // or find another way to do it, nobody else uses this
        asp->Add(cancel, "%s", xml.GetData());
    }

protected:
    bool Quasiquote(iDocumentNode* top, const MathEnvironment* env)
    {
        // 1. Handle quasiquoted expressions in attributes
        csRef<iDocumentAttributeIterator> it = top->GetAttributes();
        while(it->HasNext())
        {
            csRef<iDocumentAttribute> attr = it->Next();
            csString text = attr->GetValue();
            csString varName;
            size_t pos = (size_t)-1;
            // It'd probably be better to use psString::Interpolate and give it an optional char param for @ or $
            while((pos = text.Find("@{", pos+1)) != SIZET_NOT_FOUND)
            {
                size_t end = text.Find("}", pos+2);
                if(end == SIZET_NOT_FOUND)
                {
                    Error2("Error: Unterminated quasiquote found in attribute >%s<.", text.GetData());
                    return false;
                }
                if(end <= pos+3)
                {
                    Error2("Error: Empty quasiquote - @{} - found in attribute >%s<.", text.GetData());
                    return false;
                }
                text.SubString(varName, pos+2, end-(pos+2));
                MathVar* var = env->Lookup(varName);
                if(!var)
                {
                    Error2("Error: Quasiquote @{%s} not found in environment.", varName.GetData());
                    return false;
                }
                text.DeleteAt(pos, end-pos+1);
                text.Insert(pos, var->ToString());
            }
            attr->SetValue(text);
        }

        // 2. Recurse.
        csRef<iDocumentNodeIterator> nit = top->GetNodes();
        while(nit->HasNext())
        {
            csRef<iDocumentNode> child = nit->Next();
            if(!Quasiquote(child, env))
                return false;
        }
        return true;
    }

    SCRIPT_TRIGGER type;
    csRef<iDocumentNode> self;
    EntityManager* entitymanager;
    CacheManager* cachemanager;
};

/**
 * LetAOp - a way to evaluate MathScript stuff and create new bindings:
 *
 * \<let vars="Roll = 300;" /\>
 *   (Roll will be defined here, till the end of the apply block.)
 *
 * @note this works by modifying the environment of the applicative script.
 */
class LetAOp : public AppliedOp
{
public:
    LetAOp(EntityManager* entitymanager, CacheManager* cachemanager) :
        AppliedOp(),
        bindings(NULL),
        entitymanager(entitymanager),
        cachemanager(cachemanager)
    {
    }

    virtual ~LetAOp()
    {
        MathScript::Destroy(bindings);
    }

    bool Load(iDocumentNode* node)
    {
        bindings = MathScript::Create("<let> bindings", node->GetAttributeValue("vars"));
        return !!bindings && ParseAOps(entitymanager, cachemanager, node, ops);
    }

    virtual void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        MathEnvironment inner(env);
        bindings->Evaluate(&inner);
        csPDelArray<AppliedOp>::Iterator it = ops.GetIterator();
        while(it.HasNext())
        {
            AppliedOp* op = it.Next();
            op->Run(&inner, target, asp);
        }
    }

    virtual const csString GetDescription(MathEnvironment* env) {
        csString descr;
        bindings->Evaluate(env);
        csPDelArray<AppliedOp>::Iterator it = ops.GetIterator();
        while(it.HasNext())
        {
            AppliedOp* op = it.Next();
            descr.Append(op->GetDescription(env));
        }
	return descr;
    }

protected:
    MathScript* bindings; /// an embedded MathScript containing new bindings
    csPDelArray<AppliedOp> ops;
    EntityManager* entitymanager;
    CacheManager* cachemanager;
};

/**
 * IfAOp - a way to evaluate MathScript stuff and create new bindings:
 *
 * The attribute t is evaluated as a boolean expression and its
 * result is used to decide which branch (then / else) is to be
 * executed.
 *
 * - Example:
 * @code
 * <if t="Roll &lt; 40">
 *   <then>...</then>
 *   <else>...</else>
 * </if>
 * @endcode
 *
 * The "then" branch is required, but an "else" branch is optional.
 *
 */
class IfAOp : public AppliedOp
{
public:
    IfAOp(EntityManager* entitymanager, CacheManager* cachemanager) :
        AppliedOp(),
        condition(NULL),
        entitymanager(entitymanager),
        cachemanager(cachemanager)
    {
    }

    /// deletes the branches and the condition object
    virtual ~IfAOp()
    {
        delete condition;
    }

    bool Load(iDocumentNode* node)
    {
        condition = MathExpression::Create(node->GetAttributeValue("t"));
        csRef<iDocumentNode> thenNode = node->GetNode("then");
        csRef<iDocumentNode> elseNode = node->GetNode("else");

        if(!thenNode)
        {
            Error1("Missing <then> in <if>.");
            return false;
        }

        if(!ParseAOps(entitymanager, cachemanager, thenNode, then_ops))
        {
            return false;
        }

        if(elseNode &&
           !ParseAOps(entitymanager, cachemanager, elseNode, else_ops))
        {
            return false;
        }

        return condition != NULL;
    }

    virtual void Run(MathEnvironment* env, gemActor* target, ActiveSpell* asp)
    {
        csPDelArray<AppliedOp>::Iterator it =
            condition->Evaluate(env) != 0.0 ?
                then_ops.GetIterator() : else_ops.GetIterator();
        while(it.HasNext())
        {
            AppliedOp* op = it.Next();
            op->Run(env, target, asp);
        }
    }

    virtual const csString GetDescription(MathEnvironment* env)
    {
        csString descr;
        csPDelArray<AppliedOp>::Iterator it =
            condition->Evaluate(env) != 0.0 ?
                then_ops.GetIterator() : else_ops.GetIterator();
        while(it.HasNext())
        {
            AppliedOp* op = it.Next();
            descr.Append(op->GetDescription(env));
        }
        return descr;
    }

protected:
    csPDelArray<AppliedOp> then_ops;
    csPDelArray<AppliedOp> else_ops;
    MathExpression* condition; /// an embedded MathExpression - should result in a boolean
    EntityManager* entitymanager;
    CacheManager* cachemanager;
};

//============================================================================
// Applicative script implementation (progression script applied mode)
//============================================================================

ApplicativeScript::ApplicativeScript() : type(BUFF), duration(NULL)
{
}

ApplicativeScript::~ApplicativeScript()
{
    delete duration;
}

ApplicativeScript* ApplicativeScript::Create(EntityManager* entitymanager, CacheManager* cachemanager, const char* script)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(script);
    if(error)
    {
        Error2("Couldn't parse XML for applicative script: %s", script);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in applicative script: %s", script);
        return NULL;
    }
    csRef<iDocumentNode> top = root->GetNode("apply");
    if(!top)
    {
        Error2("Could not find <apply> node in: %s", script);
        return NULL;
    }
    return Create(entitymanager, cachemanager, top);
}

ApplicativeScript* ApplicativeScript::Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top)
{
    if(!top->GetAttributeValue("name"))
        return NULL;

    SPELL_TYPE type;
    csString typ(top->GetAttributeValue("type"));
    if(typ == "buff")
        type = BUFF;
    else if(typ == "debuff")
        type = DEBUFF;
    else
        return NULL;

    return Create(entitymanager, cachemanager, top, type, top->GetAttributeValue("name"), top->GetAttributeValue("duration"));
}

static bool ParseAOps(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top, csPDelArray<AppliedOp>& ops)
{
    csRef<iDocumentNodeIterator> it = top->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> node = it->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        csString elem = node->GetValue();

        AppliedOp* op = NULL;

        // buffables
        if(elem == "hp-rate")
        {
            op = new HPRateAOp;
        }
        else if(elem == "mana-rate" || elem == "pstamina-rate" || elem == "mstamina-rate" || elem == "hp-max" || elem == "mana-max" || elem == "pstamina-max" || elem == "mstamina-max")
        {
            op = new VitalAOp;
        }
        else if(elem == "atk" || elem == "def")
        {
            op = new CombatModAOp;
        }
        else if(elem == "agi" || elem == "end" || elem == "str" || elem == "cha" || elem == "int" || elem == "wil")
        {
            op = new StatsAOp;
        }
        else if(elem == "skill")
        {
            op = new SkillAOp(cachemanager);
        }
        else if(elem == "faction")
        {
            continue;
        }
        else if(elem == "animal-affinity")
        {
            continue;
        }
        else if(elem == "variable-int")
        {
            op = new VariableIntAOp();
        }
        // overridables
        else if(elem == "race")
        {
            op = new RaceAOp;
        }
        else if(elem == "mute")
        {
            op = new MuteAOp;
        }
        else if(elem == "freeze")
        {
            op = new FreezeAOp;
        }
        else if(elem == "pos")
        {
            continue;
        }
        // other
        else if(elem == "can-summon-familiar")
        {
            op = new CanSummonFamiliarAOp;
        }
        else if(elem == "npccmd")
        {
            op = new NPCAOp;
        }
        else if(elem == "add_pet")
        {
            op = new PetAOp;
        }
        else if(elem == "msg")
        {
            op = new MsgAOp;
        }
        else if(elem == "fx")
        {
            op = new FxAOp(cachemanager);
        }
        else if(elem == "on")
        {
            op = new OnAOp(entitymanager, cachemanager);
        }
        else if(elem == "let")
        {
            op = new LetAOp(entitymanager, cachemanager);
        }
        else if(elem == "if")
        {
            op = new IfAOp(entitymanager, cachemanager);
        }
        else
        {
            // complain (shouldn't happen - script should've been validated against the schema)
            Error2("Unknown operation >%s< - validate against the schema!", elem.GetData());
            return false;
        }

        if(op->Load(node))
        {
            ops.Push(op);
        }
        else
        {
            delete op;
            return false;
        }
    }
    return true;
}

ApplicativeScript* ApplicativeScript::Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top, SPELL_TYPE type, const char* name, const char* duration)
{
    CS_ASSERT(name);
    ApplicativeScript* script = new ApplicativeScript;
    if(!script)
        return NULL;

    script->aim = top->GetAttributeValue("aim");
    script->name = name;
    script->type = type;
    if( top->GetAttributeValue("image")!=NULL )
    {
        script->image = top->GetAttributeValue("image");
    }
    
    if(duration)
        script->duration = MathExpression::Create(duration);

    if(script->aim.IsEmpty() || script->name.IsEmpty() || (duration && !script->duration))
    {
        delete script;
        return NULL;
    }

    ParseAOps(entitymanager, cachemanager, top, script->ops);

    return script;
}

ActiveSpell* ApplicativeScript::Apply(MathEnvironment* env, bool registerCancelEvent)
{
    // TODO: Handle non-actor targets...
    gemActor* target = GetActor(env, aim);
    if (!target)
    {
        Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
        return NULL;
    }
    CS_ASSERT(target);

    // Attempt to find the name in the math event (might be set by <let>)
    MathVar* nameVar = env->Lookup(name);
    csString varName;

    if (nameVar) // if found, use it.
        varName = nameVar->GetString();
    else //if the variable was not found, take it at face value. (not in <let>)
        varName = name;

    csTicks dticks = duration ? (csTicks) duration->Evaluate(env) : 0;
    ActiveSpell* asp = new ActiveSpell(varName, type, dticks, image);

    csPDelArray<AppliedOp>::Iterator it = ops.GetIterator();
    while(it.HasNext())
    {
        AppliedOp* op = it.Next();
        op->Run(env, target, asp);
    }

    asp->Register(target);

    if(duration && registerCancelEvent)
    {
        psCancelSpellEvent* evt = new psCancelSpellEvent(dticks, asp);
        psserver->GetEventManager()->Push(evt);
    }

    return asp;
}

const csString &ApplicativeScript::GetDescription(MathEnvironment* env)
{
    if(!description.IsEmpty())
        return description;

    csPDelArray<AppliedOp>::Iterator it = ops.GetIterator();
    while(it.HasNext())
    {
        AppliedOp* op = it.Next();
        description += op->GetDescription(env);
    }

    return description;
}

//============================================================================
// Imperative mode operations
//============================================================================

/** @short Imperative base class
 * Imperative Operations are "one-shot".
 */
class ImperativeOp
{
public:
    /// default contructor
    ImperativeOp() { }
    /// default destructor
    virtual ~ImperativeOp() { }

    /** abstract function for loading and storing info from a xml script.
     * @param node is the node of the xml script to load from
     */
    virtual bool Load(iDocumentNode* node) = 0;
    /** abstract function for executing the xml script.
     * @param env is the MathEnvironment to use for execution
     */
    virtual void Run(MathEnvironment* env) = 0;
    /** @short default description is an empty string.
     * 
     * The full description string is created by executing the description
     * gathering for all commands in the xml script order.
     * @param env is the MathEnvironment to use for creating the description
     */
    virtual const csString GetDescription(MathEnvironment* env)
    {
	return "";
    }
};

//----------------------------------------------------------------------------

/**
 * ApplyOp - creates an applicative script and applies it to the aimed entity.
 *
 * Creates a new ActiveSpell containing the effects described within, and
 * applies it to the entity aimed at.\n
 *
 * This is the primary way to enter the applied mode.
 */
class ApplyOp : public ImperativeOp
{
public:
    /**
     * constructor storing the entitymanger and cachemanager
     * @param entitymanager pointer to store
     * @param cachemanager pointer to store
     */
    ApplyOp(EntityManager* entitymanager, CacheManager* cachemanager)
    {
        this->entitymanager = entitymanager;
        this->cachemanager = cachemanager;
        aps = NULL;
    }
    /// TODO: fix deallocation
    virtual ~ApplyOp()
    {
        if(aps)
            delete aps;
        aps = NULL;
    }
    /**
     * loads the applicative script from the supplied xml node
     * @param top is containing the applicative script
     * @return true if successful, otherwise false
     */
    bool Load(iDocumentNode* top)
    {
        aps = ApplicativeScript::Create(entitymanager, cachemanager, top);
        return aps != NULL;
    }
    /**
     * executes the stored applicative script
     * @param env is the MathEnvironment to utilize for execution
     */
    virtual void Run(MathEnvironment* env)
    {
        aps->Apply(env, true);
    }

protected:
    ApplicativeScript* aps;             ///< the applicative script to store and execute
    CacheManager* cachemanager;         ///< the global cachemanager
    EntityManager* entitymanager;       ///< the global entitymanager
};

//----------------------------------------------------------------------------

/** ApplyLinkedOp - creates a buff and debuff applicative script.
 */
class ApplyLinkedOp : public ImperativeOp
{
public:
    /**
     * constructor storing the entitymanger and cachemanager.
     * 
     * stores entitymanger and cachemanager pointers and 
     * initializes internal variables.
     * @param entitymanager pointer to store
     * @param cachemanager pointer to store
     */
    ApplyLinkedOp(EntityManager* entitymanager, CacheManager* cachemanager)
    {
        this->entitymanager = entitymanager;
        this->cachemanager = cachemanager;
        buff = NULL;
        debuff = NULL;
    }
    /// default destructor
    virtual ~ApplyLinkedOp()
    {
        delete buff;
        delete debuff;
    }
    /**
     * loads the applicative buff and debuff script from the supplied xml node
     * @param top is containing the applicative buff and debuff scripts
     * @return true if successful, otherwise false
     */ 
    bool Load(iDocumentNode* top)
    {
        csString name(top->GetAttributeValue("name"));
        if(name.IsEmpty())
        {
            Error1("<apply-linked> node is missing a name attribute.");
            return false;
        }

        csRef<iDocumentNode> buffNode   = top->GetNode("buff");
        csRef<iDocumentNode> debuffNode = top->GetNode("debuff");

        buff   = ApplicativeScript::Create(entitymanager, cachemanager, buffNode,   BUFF,   name, top->GetAttributeValue("duration"));
        debuff = ApplicativeScript::Create(entitymanager, cachemanager, debuffNode, DEBUFF, name, top->GetAttributeValue("duration"));
        return buff && debuff;
    }
    /**
     * executes the stored applicative buff and debuff scripts.
     * The buff and the debuff are applied and linked to each other.
     * @param env is the MathEnvironment to utilize for execution
     */
    virtual void Run(MathEnvironment* env)
    {
        ActiveSpell* buffAsp   =   buff->Apply(env, true);  // only bother with one cancel event;
        ActiveSpell* debuffAsp = debuff->Apply(env, false); // the link ensures both will get cancelled

        // Link the two ActiveSpells...
        buffAsp->Link(debuffAsp);
        debuffAsp->Link(buffAsp);
    }
protected:
    ApplicativeScript* buff;   ///< The buff (beneficiary) script
    ApplicativeScript* debuff; ///< The debuff (victim) script
    CacheManager* cachemanager;
    EntityManager* entitymanager;
};

//----------------------------------------------------------------------------

/**
 * LetOp - a way to evaluate MathScript stuff and create new bindings.
 * 
 * A new binding is created by the content of the vars attribute. In this
 * example the newly created variable is "Roll" and it's scope is limited
 * to the let block:
 *
 * @code
 * <let vars="Roll = rnd(100)">
 *   (Roll will be defined here, within the let block.)
 * </let>
 * @endcode
 */
class LetOp : public ImperativeOp
{
public:
    LetOp(EntityManager* entitymanager, CacheManager* cachemanager) : ImperativeOp(), bindings(NULL), body(NULL)
    {
        this->entitymanager = entitymanager;
        this->cachemanager = cachemanager;
    }
    /// default destructor deallocates the MathScript bindings and the sub xml script object.
    virtual ~LetOp()
    {
        MathScript::Destroy(bindings);
        delete body;
    }

    virtual const csString GetDescription(MathEnvironment* inner)
    {
        bindings->Evaluate(inner);
        return "";
    }
    /**
     * loads new bindings from the let head and from the body a sub xml script
     * @param node that is a let block
     */
    bool Load(iDocumentNode* node)
    {
        bindings = MathScript::Create("<let> bindings", node->GetAttributeValue("vars"));
        body = ProgressionScript::Create(entitymanager, cachemanager, "<let> body", node);
        return (bindings && body);
    }
    /**
     * adds the stored bindings to the supplied MathEnvironment and executes the sub xml script.
     * @param outer a supplied MathEnvironment
     */
    virtual void Run(MathEnvironment* outer)
    {
        MathEnvironment inner(outer);
        bindings->Evaluate(&inner);
        body->Run(&inner);
    }

protected:
    MathScript* bindings;       ///< an embedded MathScript containing new bindings
    ProgressionScript* body;    ///< the script contained in the body of the let block
    EntityManager* entitymanager;
    CacheManager* cachemanager;
};

//----------------------------------------------------------------------------

/**
 * IfOp - conditional control flow, based on MathScript.
 * 
 * The attribute t is evaluated as a boolean expression and its
 * result is used to decide which branch (then / else) is to be
 * executed.
 *
 * - Example:
 * @code
 * <if t="Roll &lt; 40">
 *   <then>...</then>
 *   <else>...</else>
 * </if>
 * @endcode
 *
 * The "then" branch is required, but an "else" branch is optional.
 */
class IfOp : public ImperativeOp
{
public:
    IfOp(EntityManager* entitymanager, CacheManager* cachemanager) : ImperativeOp(), thenBranch(NULL), elseBranch(NULL), condition(NULL)
    {
        this->entitymanager = entitymanager;
        this->cachemanager = cachemanager;
    }
    /// deletes the branches and the condition object
    virtual ~IfOp()
    {
        if(thenBranch)
            delete thenBranch;
        if(elseBranch)
            delete elseBranch;
        if(condition)
            delete condition;
    }

    bool Load(iDocumentNode* node)
    {
        condition = MathExpression::Create(node->GetAttributeValue("t"));
        csRef<iDocumentNode> thenNode = node->GetNode("then");
        csRef<iDocumentNode> elseNode = node->GetNode("else");

        if(!thenNode)
        {
            Error1("Missing <then> in <if>.");
            return false;
        }

        thenBranch = ProgressionScript::Create(entitymanager, cachemanager, "<then> clause", thenNode);

        if(elseNode)
            elseBranch = ProgressionScript::Create(entitymanager, cachemanager, "<else> clause", elseNode);

        return (condition && thenBranch && (elseBranch || !elseNode));
    }

    virtual void Run(MathEnvironment* env)
    {
        if(condition->Evaluate(env) != 0.0)
            thenBranch->Run(env);
        else if(elseBranch)
            elseBranch->Run(env);
    }

protected:
    ProgressionScript* thenBranch;
    ProgressionScript* elseBranch;
    MathExpression* condition; /// an embedded MathExpression - should result in a boolean
    EntityManager* entitymanager;
    CacheManager* cachemanager;
};

//----------------------------------------------------------------------------

/**
 * MsgOp - one-time message sending:
 *
 * @code
 * <msg aim="Caster" text="Put some text here"/>
 * @endcode
 * 
 * aim is the receiver of the message.\n
 * text is the message to send.
 */
class MsgOp : public ImperativeOp
{
public:
    virtual ~MsgOp() { }

    bool Load(iDocumentNode* node)
    {
        aim = node->GetAttributeValue("aim");
        text = node->GetAttributeValue("text");
        type = node->GetAttributeValue("type");
        return true;
    }

    virtual void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        if (!actor)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        if(actor && actor->GetClientID())
        {
            csString finalText(text);
            env->InterpolateString(finalText);

            if(type == "ok")
            {
                psserver->SendSystemOK(actor->GetClientID(), finalText);
            }
            else if(type == "result")
            {
                psserver->SendSystemResult(actor->GetClientID(), finalText);
            }
            else if(type == "error")
            {
                psserver->SendSystemError(actor->GetClientID(), finalText);
            }
            else
            {
                psserver->SendSystemInfo(actor->GetClientID(), finalText);
            }
        }
    }

protected:
    csString aim;  ///< name of the MathScript var to aim at
    csString text; ///< message to send
    csString type; ///< message type to send
};

//----------------------------------------------------------------------------

/**
 * NPCCmdOp - one-time npc command sending:
 *
 * \<npccmd aim="Caster" cmd="spam spam spam spam...!"/\>
 */
class NPCCmdOp : public ImperativeOp
{
public:
    virtual ~NPCCmdOp() { }

    bool Load(iDocumentNode* node)
    {
        aim = node->GetAttributeValue("aim");
        cmd = node->GetAttributeValue("cmd");
        return true;
    }

    virtual void Run(MathEnvironment* env)
    {
        gemNPC* npc = GetNPC(env, aim);
        if (!npc)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        if(npc)
        {
            csString finalCmd(cmd);
            env->InterpolateString(finalCmd);

            psserver->GetNPCManager()->QueueNPCCmdPerception(npc, finalCmd);
        }
    }

protected:
    csString aim; ///< name of the MathScript var to aim at
    csString cmd; ///< cmd to send to the npc
};

//----------------------------------------------------------------------------

/**
 * CancelOp - a way to cancel active spells.
 *
 * \<cancel type="all"\>
 *   \<spell type="buff" name="Defensive Wind"/\>
 *   \<spell type="buff" name="Flame Spire"/\>
 * \</cancel>
 *
 * There are three types:
 * - "all"     ... cancels all listed spells.
 * - "ordered" ... cancels the first listed spell that's actually active.
 * - "random"  ... cancels a random spell from the list (one which is active).
 */
class CancelOp : public ImperativeOp
{
public:
    virtual ~CancelOp() { }

    bool Load(iDocumentNode* cancel)
    {
        aim = cancel->GetAttributeValue("aim");

        csString typ(cancel->GetAttributeValue("type"));
        if(typ == "all")
            type = ALL;
        else if(typ == "ordered")
            type = ORDERED;
        else if(typ == "random")
            type = RANDOM;
        else
        {
            Error2("Invalid type in <cancel type=\"%s\">.", typ.GetData());
            return false;
        }

        csRef<iDocumentNodeIterator> it = cancel->GetNodes();
        while(it->HasNext())
        {
            csRef<iDocumentNode> node = it->Next();

            if(node->GetType() != CS_NODE_ELEMENT)  // not sure if this is really necessary...
                continue;

            Spell spell;
            spell.name = node->GetAttributeValue("name");

            typ = node->GetAttributeValue("type");
            if(typ == "buff")
                spell.type = BUFF;
            else if(typ == "debuff")
                spell.type = DEBUFF;
            else
            {
                Error2("Invalid type in <cancel> child: <spell type=\"%s\">.", typ.GetData());
                return false;
            }

            spells.Push(spell);
        }

        return true;
    }

    virtual void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        if (!actor)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        CS_ASSERT(actor);

        // Find which potentially cancellable spells are actually active
        csArray<ActiveSpell*> asps;
        for(size_t i = 0; i < spells.GetSize(); i++)
        {
            ActiveSpell* asp = actor->FindActiveSpell(spells[i].name, spells[i].type);
            if(asp)
                asps.Push(asp);
        }

        if(asps.IsEmpty())
            return;

        switch(type)
        {
            case ORDERED:
            {
                if(asps[0]->Cancel())
                    delete asps[0];
                break;
            }
            case RANDOM:
            {
                ActiveSpell* asp = asps[psserver->GetRandom(asps.GetSize())];
                if(asp->Cancel())
                    delete asp;
                break;
            }
            case ALL:
            {
                for(size_t i = 0; i < asps.GetSize(); i++)
                {
                    if(asps[i]->Cancel())
                        delete asps[i];
                }
                break;
            }
        }
    }

protected:
    enum { ALL, ORDERED, RANDOM } type;
    struct Spell
    {
        SPELL_TYPE type;
        csString name;
    };

    csString aim;  ///< name of the MathScript var to aim at
    csArray<Spell> spells; ///< list of spells to cancel.
};

//----------------------------------------------------------------------------

/**
 * @short creates an effect.
 * 
 * The effect is either created at the target or in front of it.
 * 
 * If the effect is not attached:
 * 
 * \<fx type="unattached" name="SomeName" source="SomeSource" target=""/\>
 * 
 * or if the effect is attached:
 * 
 * \<fx name="SomeName" source="SomeSource" target=""/\>
 * 
 * scale="value" is optional and gives the scaling factor for the effect.
 */
class FxOp : public ImperativeOp
{
public:
    FxOp() : ImperativeOp(), effectScale(NULL) { }
    virtual ~FxOp()
    {
        delete effectScale;
    }

    bool Load(iDocumentNode* top)
    {
        // type defaults to attached
        csString typ(top->GetAttributeValue("type"));
        attached = typ != "unattached";

        name      = top->GetAttributeValue("name");
        sourceVar = top->GetAttributeValue("source");
        targetVar = top->GetAttributeValue("target");
        if(top->GetAttributeValue("scale"))
        {
            effectScale = MathExpression::Create(top->GetAttributeValue("scale"));
        }

        return !name.IsEmpty() && !targetVar.IsEmpty();
    }

    virtual void Run(MathEnvironment* env)
    {
        gemObject* target = GetObject(env, targetVar);
        gemObject* source = target;
        float scale = 0.0f;
        if(!sourceVar.IsEmpty())
            source = GetObject(env, sourceVar);

        if(effectScale)
        {
            scale = effectScale->Evaluate(env);
        }

        if(attached)
        {
            psEffectMessage fx(0, name, csVector3(0,0,0), source->GetEID(), target->GetEID(), 0, scale);
            if(!fx.valid)
            {
                Error1("Error: <fx> could not create valid psEffectMessage\n");
                return;
            }
            fx.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        }
        else
        {
            // Put the effect in front of the target, where we'd drop stuff
            csVector3 pos;
            float yrot;
            iSector* sector;
            target->GetPosition(pos, yrot, sector);
            pos.x -= DROP_DISTANCE*  sinf(yrot);
            pos.z -= DROP_DISTANCE*  cosf(yrot);

            // Send effect message
            psEffectMessage fx(0, name, pos, 0, 0, 0, scale);
            if(!fx.valid)
            {
                Error1("Error: <fx> could not create valid psEffectMessage\n");
                return;
            }
            fx.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        }
    }

protected:
    bool attached;
    csString name;               ///< the name of the effect
    csString sourceVar;          ///< name of the MathVar containing where the effect starts
    csString targetVar;          ///< name of the MathVar containing where the effect is aimed
    MathExpression* effectScale; ///< the scale which this fxop will apply on the effect
};

//----------------------------------------------------------------------------

/**
 * A imperative base class supporting an aim.
 * 
 */
class ImperativeAim : public ImperativeOp
{
public:
    virtual ~ImperativeAim() { }

    /**
     * retrieves the "aim" attribute and stores it.
     * @param node the xml node to load from
     */
    bool Load(iDocumentNode* node)
    {
        aim = node->GetAttributeValue("aim");
        return !aim.IsEmpty();
    }

protected:
    csString aim;   ///< name of the MathScript var to aim at
};

/**
 * A base class supporting aim and a MathExpression value.
 * 
 * This clas stores the aim and value of the supplied xml node.
 */
class ImperativeAimValue : public ImperativeAim
{
public:
    ImperativeAimValue() : ImperativeAim(), value(NULL) { }
    virtual ~ImperativeAimValue()
    {
        delete value;
    }
    /**
     * retrieves the "aim" and "value" attributes and stores them.\n
     * Aim must be a string and value must be a MathExpression.
     * @param node the xml node to load from
     * @return true when loading attributes from xml node was successful.
     */
    bool Load(iDocumentNode* node)
    {
        value = MathExpression::Create(node->GetAttributeValue("value"));
        return value && ImperativeAim::Load(node);
    }

protected:
    MathExpression* value; ///< an embedded MathExpression
};

/**
 * A base class supporting aim, value, and name.
 * 
 * This clas stores the aim, value and name of the supplied xml node.\n
 * Aim and name are stored as strings, name is stored as a MathExpression.
 */
class ImperativeAimValueName : public ImperativeAimValue
{
public:
    ImperativeAimValueName() : ImperativeAimValue() { }
    virtual ~ImperativeAimValueName() { }
    /**
     * retrieves the "aim","value" and "name" attributes and stores them.\n
     * Aim and name must be a strings and value must be a MathExpression.
     * @param node the xml node to load from
     */
    bool Load(iDocumentNode* node)
    {
        name = node->GetAttributeValue("name");
        return ImperativeAimValue::Load(node);
    }

protected:
    csString name; ///< plain text - name of something, like a skill or faction
};

//----------------------------------------------------------------------------

/**
 * operator for destroying a specified object.
 * 
 * The object that is going to be destroyed must be defined in the supplied
 * MathEnvironment.
 * @code
 * <destroy aim="SomeObjectName"/>
 * @endcode
 * SomeObjectName needs to be replaced by the object name that is used to 
 * register the object in the MathEnvironment. So the name identifies the 
 * object to be destroyed.
 */
class DestroyOp : public ImperativeAim
{
public:
    DestroyOp(EntityManager* entitymanager) : ImperativeAim()
    {
        this->entitymanager = entitymanager;
    }
    virtual ~DestroyOp() { }
    /**
     * destroys the object from the supplied MathEnvironment.
     * Destruction is initiated as an Event to the Eventmanager.
     * @param env MathEnvironment containing the object to be destroyed.
     */
    void Run(MathEnvironment* env)
    {
        gemObject* obj = GetObject(env, aim);
        if (!obj)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        psserver->GetEventManager()->Push(new psEntityEvent(entitymanager, psEntityEvent::DESTROY, obj));
    }
protected:
    EntityManager* entitymanager;
};

//----------------------------------------------------------------------------

/**
 * TeleportOp is a teleport operation.
 * 
 * 
 * 
 * optional attributes:
 *  - delay
 *  - background
 *  - widget
 *  - x1, x2, y1, y2
 *  - location
 *  - sector
 *    + x, y, z
 *    + instance
 */
class TeleportOp : public ImperativeAim
{
public:
    TeleportOp(EntityManager* entitymanager) : ImperativeAim()
    {
        entityManager = entitymanager;
        loadDelay = NULL;
        background = "";
        point1X = NULL;
        point1Y = NULL;
        point2X = NULL;
        point2Y = NULL;
    }
    virtual ~TeleportOp()
    {
        if(loadDelay)
        {
            delete loadDelay;
        }
        if(point1X)
        {
            delete point1X;
        }
        if(point1Y)
        {
            delete point1Y;
        }
        if(point2X)
        {
            delete point2X;
        }
        if(point2Y)
        {
            delete point2Y;
        }
    }

    bool Load(iDocumentNode* node)
    {
        if(!ImperativeAim::Load(node))
            return false;

        if(node->GetAttribute("delay"))
        {
            loadDelay = MathExpression::Create(node->GetAttributeValue("delay"));
        }

        if(node->GetAttribute("background"))
        {
            background = node->GetAttributeValue("background");
        }

        if(node->GetAttribute("widget"))
        {
            widget = node->GetAttributeValue("widget");
        }

        if(node->GetAttribute("x1") && node->GetAttribute("y1") && node->GetAttribute("x2") && node->GetAttribute("y2"))
        {
            point1X = MathExpression::Create(node->GetAttributeValue("x1"));
            point1Y = MathExpression::Create(node->GetAttributeValue("y1"));
            point2X = MathExpression::Create(node->GetAttributeValue("x2"));
            point2Y = MathExpression::Create(node->GetAttributeValue("y2"));
        }

        if(node->GetAttribute("location"))
        {
            type = NAMED;
            destination = node->GetAttributeValue("location");
            return destination == "spawn";
        }
        else if(node->GetAttribute("sector"))
        {
            destination = node->GetAttributeValue("sector");
            type = SECTOR;

            if(node->GetAttribute("x") && node->GetAttribute("y") && node->GetAttribute("z"))
            {
                type |= XYZ;
                pos.x = node->GetAttributeValueAsFloat("x");
                pos.y = node->GetAttributeValueAsFloat("y");
                pos.z = node->GetAttributeValueAsFloat("z");
            }

            if(node->GetAttribute("instance"))
            {
                type |= INSTANCE;
                instance = (InstanceID) node->GetAttributeValueAsInt("instance");
            }
        }
        else
        {
            Error1("<teleport> specified with neither a location or map attribute.");
            return false;
        }
        return true;
    }

    void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        if (!actor)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        //holds the value calculated of the delay in case it's needed
        int32_t loadDelayInt = 0;
        //holds the value calculated of the origin point of the anim
        csVector2 point1 = 0;
        //holds the value calculated of the destination point of the anim
        csVector2 point2 = 0;
        CS_ASSERT(actor);


        //calculate math expressions in case they where found
        if(loadDelay)
        {
            loadDelayInt = loadDelay->Evaluate(env);
        }

        //calculate cordinates only if all math expressions are valid
        if(point1X && point1Y && point2X && point2Y)
        {
            point1.x = point1X->Evaluate(env);
            point1.y = point1Y->Evaluate(env);
            point2.x = point2X->Evaluate(env);
            point2.y = point2Y->Evaluate(env);
        }

        if(type == NAMED)
        {
            // we only handle "spawn" for now...
            actor->MoveToSpawnPos(loadDelayInt, background, point1, point2, widget);
        }
        else
        {
            iSector* sector;
            csVector3 destPos;

            if(type & XYZ)
            {
                sector = entityManager->GetEngine()->FindSector(destination);
                destPos = pos;
            }
            else
            {
                if(!psserver->GetAdminManager()->GetStartOfMap(0, destination, sector, destPos))
                {
                    Error2("Could not get start of map >%s<.", destination.GetDataSafe());
                    return;
                }
            }

            if(!sector)
            {
                Error2("Could not find sector >%s<.", destination.GetDataSafe());
                return;
            }

            if(type & INSTANCE)
                actor->Teleport(sector, destPos, 0.0, instance, loadDelayInt, background, point1, point2, widget);
            else
                actor->Teleport(sector, destPos, 0.0, loadDelayInt, background, point1, point2, widget);
        }
    }

protected:
    const static int NAMED = 0x0, SECTOR = 0x1, XYZ = 0x2, INSTANCE = 0x4;
    int type;
    csString destination;
    csVector3 pos;
    InstanceID instance;
    MathExpression* loadDelay; ///<The delay the loading screen shall have, in seconds; YOU DON'T HAVE TO DEFINE IT
    csString background;       ///<The background of the loading screen; YOU DON'T HAVE TO DEFINE IT
    csString widget;           ///< The replacement widget used for the loading screen
    MathExpression* point1X;   ///<Defines x cordinate start of animation
    MathExpression* point1Y;   ///<Defines y cordinate start of animation
    MathExpression* point2X;   ///<Defines x cordinate end of animation
    MathExpression* point2Y;   ///<Defines y cordinate end of animation

    EntityManager* entityManager;
};

//----------------------------------------------------------------------------

/**
 * VitalOp - imperative mana & stamina, but not HP
 *
 * \<mana aim="Caster" value="-5"/\>
 */
class VitalOp : public ImperativeAimValue
{
public:
    VitalOp() : ImperativeAimValue() { }
    virtual ~VitalOp() { }

    bool Load(iDocumentNode* node)
    {
        vital = node->GetValue();
        return ImperativeAimValue::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        float val = value->Evaluate(env);
        if(vital == "mana")
            c->AdjustMana(val);
        else if(vital == "pstamina")
            c->AdjustStamina(val, true);
        else if(vital == "mstamina")
            c->AdjustStamina(val, false);
        else
            CS_ASSERT(false);
    }

protected:
    csString vital;
};

/**
 * HPOp - direct HP damage.
 *
 * Unlike other vitals, \<hp\> can specify an optional attacker who will be
 * recorded in the target's damage history.\n
 *
 * - Examples:
 * @code
 * <hp attacker="Caster" aim="Target" value="-5*Power"/>
 * <hp aim="Target" value="15"/>
 * @endcode
 */
class HPOp : public ImperativeAimValue
{
public:
    HPOp() : ImperativeAimValue() { }
    virtual ~HPOp() { }

    bool Load(iDocumentNode* node)
    {
        attacker = node->GetAttributeValue("attacker");
        return ImperativeAimValue::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        gemActor* target = GetActor(env, aim);
        if (!target)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        gemActor* atk = GetActor(env, attacker); // may be NULL
        float val = value->Evaluate(env);

        if(val < 0)
            target->DoDamage(atk, -val);
        else
            target->GetCharacterData()->AdjustHitPoints(val);
    }

protected:
    csString attacker; ///< optional MathScript var containing the attacker
};

//----------------------------------------------------------------------------

/**
 * StatsOp - imperative stats (strength, agility, etc.).
 *
 * This is primarily used for character creation, as these effects are
 * permanent.  See StatsAOp for the buffed kind.\n
 * 
 * - Example:
 * @code
 * <str aim="Actor" value="55"/> 
 * @endcode
 * This adds 55 to the actor's strength.
 *
 */
class StatsOp : public ImperativeAimValue
{
public:
    StatsOp() : ImperativeAimValue() { }
    virtual ~StatsOp() { }

    bool Load(iDocumentNode* node)
    {
        csString type(node->GetValue());
        if(type == "agi")
            stat = PSITEMSTATS_STAT_AGILITY;
        else if(type == "end")
            stat = PSITEMSTATS_STAT_ENDURANCE;
        else if(type == "str")
            stat = PSITEMSTATS_STAT_STRENGTH;
        else if(type == "cha")
            stat = PSITEMSTATS_STAT_CHARISMA;
        else if(type == "int")
            stat = PSITEMSTATS_STAT_INTELLIGENCE;
        else if(type == "wil")
            stat = PSITEMSTATS_STAT_WILL;
        else
        {
            Error2("StatsOp doesn't know what to do with <%s> tag.", type.GetData());
            return false;
        }

        return ImperativeAimValue::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        int val = (int) value->Evaluate(env);
        PSSKILL skill = statToSkill(stat);
        if (skill != PSSKILL_NONE)
        {
            SkillRank &buffable = c->GetSkillRank(skill);
            buffable.SetBase(buffable.Base() + val);
        }
    }

protected:
    PSITEMSTATS_STAT stat; ///< which stat we're actually buffing
};

//----------------------------------------------------------------------------

/**
 * FactionOp - imperative factions.
 *
 * This is permanent;\n
 * see FactionAOp for the temporary buffed kind. (not implemented)
 * 
 * - Example:
 * @code
 * <faction aim="Actor" name="Klyros" value="5"/>
 * @endcode
 * This adds 5 to actor's Klyros faction.
 */
class FactionOp : public ImperativeAimValueName
{
public:
    FactionOp(CacheManager* cachemanager) : ImperativeAimValueName()
    {
        this->cachemanager = cachemanager;
    }
    virtual ~FactionOp() { }

    bool Load(iDocumentNode* node)
    {
        faction = cachemanager->GetFaction(node->GetAttributeValue("name"));
        if(!faction) //if the faction is valid we just use it else we will use the named variable during run.
        {
            variableName = node->GetAttributeValue("name");
        }
        return ImperativeAimValueName::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        Faction* currentFaction = faction;
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        int val = (int) value->Evaluate(env);
        //we didn't find a valid faction name during load: it means it's a variable.
        if(!currentFaction)
        {
            //evaluate the variable so we can get it's value
            MathVar* factionVar = env->Lookup(variableName);
            if(!factionVar)
            {
                Error2("Faction Imperative Op Run with invalid faction name variable: %s", variableName.GetData());
                return;
            }

            csString factionName = factionVar->GetString();
            currentFaction = cachemanager->GetFaction(factionName.GetData());
            if(!currentFaction) //we still didn't find a valid faction abort.
            {
                Error2("Faction Imperative Op Run with %s but no such faction exists.",  factionName.GetData());
                return;
            }
        }
        c->UpdateFaction(currentFaction, val);
    }
protected:

    ///Used to store the faction in case it's found during Load(). It's null in case it wasn't found.
    Faction* faction;
    /** Used to store the faction name math expression in case the faction couldn't be found.
     *  It's uninitialized in case it's not used as the code shouldn't use it in that case.
     */
    csString variableName;
    CacheManager* cachemanager;
};

//----------------------------------------------------------------------------

/**
 * VariableSetOp - imperative factions.
 *
 * This is permanent.\n 
 * Note that if the variable is set already it will be overwritten.\n
 *
 * - Example:
 * @code
 * \<variable aim="Actor" name="LOVEDBYKLYROS" value="true"/\>
 * @endcode
 * This sets the variable LOVEDBYKLYROS and sets it's value to "true".
 */
class VariableSetOp : public ImperativeAim
{
public:
    VariableSetOp() : ImperativeAim() { }
    virtual ~VariableSetOp() { }

    bool Load(iDocumentNode* node)
    {
        variableName = node->GetAttributeValue("name");
        variableValue = node->GetAttributeValue("value","");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        //evaluate the variables so we can get it's value
        MathVar* nameVar = env->Lookup(variableName);
        MathVar* valueVar = env->Lookup(variableValue);
        csString varName;
        csString varValue;

        if(nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = variableName;

        if(valueVar)
            varValue = valueVar->ToString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varValue = variableValue;

        c->SetVariable(varName, varValue);
    }
protected:

    /// Used to store the variable name math expression.
    csString variableName;
    /// Used to store the variable value math expression.
    csString variableValue;
};

//----------------------------------------------------------------------------

/**
 * VariableUnSetOp - imperative factions.
 *
 * \<variable aim="Actor" name="LOVEDBYKLYROS" /\> (unset the variable LOVEDBYKLYROS)
 *
 * This is permanent.
 */
class VariableUnSetOp : public ImperativeAim
{
public:
    VariableUnSetOp() : ImperativeAim() { }
    virtual ~VariableUnSetOp() { }

    bool Load(iDocumentNode* node)
    {
        variableName = node->GetAttributeValue("name");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        //evaluate the variables so we can get it's value
        MathVar* nameVar = env->Lookup(variableName);
        csString varName;
        if(nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = variableName;

        c->UnSetVariable(varName);
    }
protected:

    /// Used to store the variable name math expression.
    csString variableName;
};

//----------------------------------------------------------------------------

/**
 * SkillOp - imperative skills.
 *
 * \<skill aim="Actor" name="Sword" value="5"/\> (adds 5 to actor's sword skill.)
 *
 * This is permanent; see SkillAOp for the temporary buffed kind.
 */
class SkillOp : public ImperativeAimValueName
{
public:
    SkillOp(CacheManager* cachemanager) : ImperativeAimValueName()
    {
        this->cachemanager = cachemanager;
    }
    virtual ~SkillOp() { }

    bool Load(iDocumentNode* node)
    {
        skillName = node->GetAttributeValue("name");
        return ImperativeAimValueName::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        //evaluate the variables so we can get it's value
        MathVar* nameVar = env->Lookup(skillName);
        csString varName;

        if(nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = skillName;

        psSkillInfo* info = cachemanager->GetSkillByName(varName);
        if(!info)
        {
            Error2("Found <skill aim=\"...\" name=\"%s\">, but no such skill exists.", varName.GetData());
            return;
        }

        PSSKILL skill = info->id;
        int val = (int) value->Evaluate(env);
        SkillRank &buffable = c->GetSkillRank(skill);
        buffable.SetBase(buffable.Base() + val);
        c->SetSkillRank(info->id,buffable.Base());
    }
protected:
    CacheManager* cachemanager;
    csString skillName;
};

//----------------------------------------------------------------------------

/**
* TraitOp - imperative skills.
*
* \<trait aim="Target" name="traitX" /\> (sets traitX on actor.)
*
* This is a permanent change.
*/
class TraitOp : public ImperativeAim
{
public:
    TraitOp(CacheManager* cachemanager) : ImperativeAim()
    {
        this->cachemanager = cachemanager;
    }
    virtual ~TraitOp() { }

    bool Load(iDocumentNode* node)
    {
        traitName = node->GetAttributeValue("name");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        psCharacter* c = GetCharacter(env, aim);
        gemObject* obj = GetObject(env, aim);
        if (!c || !actor || !obj)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        // we do not want to set traits on any morphed/etc player, because the trait are tried to the player, not the overrridable race.
        if (c->GetOverridableRace().Base() != c->GetOverridableRace().Current())
        {
            return;
        }

        //evaluate the variables so we can get it's value
        MathVar* nameVar = env->Lookup(traitName);
        csString varName;

        if (nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = traitName;

        CacheManager::TraitIterator ti = psserver->GetCacheManager()->GetTraitIterator();
        while (ti.HasNext())
        {
            psTrait* currTrait = ti.Next();
            if (currTrait->gender == c->GetRaceInfo()->gender &&
                currTrait->race == c->GetRaceInfo()->race &&
                currTrait->name.CompareNoCase(traitName))
            {
                c->SetTraitForLocation(currTrait->location, currTrait);

                csString str("<traits>");
                do
                {
                    str.Append(currTrait->ToXML());
                    currTrait = currTrait->next_trait;
                } while (currTrait);
                str.Append("</traits>");

                psTraitChangeMessage message(obj->GetClient()->GetClientNum(), c->GetActor()->GetEID(), str);
                message.Multicast(c->GetActor()->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
                //update everything needed for bgloader to pick changes correctly.
                obj->UpdateProxList(true);

                psserver->SendSystemOK(obj->GetClient()->GetClientNum(), "Trait successfully changed");
                return;
            }
        }
    }
protected:
    CacheManager* cachemanager;
    csString traitName;
};

//----------------------------------------------------------------------------

/**
* AnimationOp - imperative skills.
*
* \<animation aim="Target" name="animationX" /\> (runs animationX on actor, runs once.)
*
* This is a run once animation. Animation is not validated, only client knows if it has the animation. If it does not exist, nothing happens.
*/
class AnimationOp : public ImperativeAim
{
public:
    AnimationOp(CacheManager* cachemanager) : ImperativeAim()
    {
        this->cachemanager = cachemanager;
    }
    virtual ~AnimationOp() { }

    bool Load(iDocumentNode* node)
    {
        animationName = node->GetAttributeValue("name");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        psCharacter* c = GetCharacter(env, aim);
        gemObject* obj = GetObject(env, aim);
        if (!c || !actor || !obj)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        //evaluate the variables so we can get it's value
        MathVar* nameVar = env->Lookup(animationName);
        csString varName;

        if (nameVar)
            varName = nameVar->GetString();
        else //if the variable was not found try getting the value associated directly (not in <let>)
            varName = animationName;

        psUserActionMessage msg(actor->GetClientID(), actor->GetEID(), varName);
        msg.Multicast(actor->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }
protected:
    CacheManager* cachemanager;
    csString animationName;
};

//----------------------------------------------------------------------------

/**
 * ExpOp - grant experience points.
 *
 * \<exp aim="Actor" value="200" notify="false" /\>
 */
class ExpOp : public ImperativeAimValue
{
public:
    ExpOp() : ImperativeAimValue(), notify(false) { }
    virtual ~ExpOp() { }

    bool Load(iDocumentNode* node)
    {
        notify = node->GetAttributeValueAsBool("notify", false);
        return ImperativeAimValue::Load(node);
    }

    // AllocateKillDamage blah.
    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c) {
            Error1("Error while executing ExpOp script, invalid target character");
            return;
        }
        float exp = value->Evaluate(env);
        if(notify)
        {
            c->AddExperiencePointsNotify((unsigned int)exp);
        }
        else
        {
            c->AddExperiencePoints((unsigned int)exp);
        }
    }
protected:
    bool notify;
};

//----------------------------------------------------------------------------

/**
 * AnimalAffinityOp
 *
 * \<animal-affinity aim="Actor" name="reptile" value="2"/\>
 *
 * This is primarily used for character creation, as these effects are
 * permanent.  There is no way to buff this currently.
 */
class AnimalAffinityOp : public ImperativeAimValueName
{
public:
    AnimalAffinityOp() : ImperativeAimValueName() { }
    virtual ~AnimalAffinityOp() { }

    bool Load(iDocumentNode* node)
    {
        if(!ImperativeAimValueName::Load(node))
            return false;

        name.Downcase();

        return true;
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* chr = GetCharacter(env, aim);
        if (!chr)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        // Parse the string into an XML document.
        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />
        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />
        //     ...
        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />
        csRef<iDocumentSystem> xml;
        xml.AttachNew(new csTinyDocumentSystem);
        CS_ASSERT(xml != NULL);
        csRef<iDocument> xmlDoc = xml->CreateDocument();
        const char* error = xmlDoc->Parse(chr->GetAnimalAffinity());

        csRef<iDocumentNode> node;
        bool found = false;

        if(!error)
        {
            // Find existing node
            csRef<iDocumentNodeIterator> it = xmlDoc->GetRoot()->GetNodes();
            while(it->HasNext())
            {
                node = it->Next();
                if(name.CompareNoCase(node->GetAttributeValue("name")))
                {
                    found = true;
                    break;
                }
            }
        }

        // Add new node if one doesn't exist
        if(!found)
        {
            csString attrNode = psserver->GetProgressionManager()->GetAffinityCategories().Get(name, "");
            if(!attrNode.IsEmpty())
            {
                node = xmlDoc->GetRoot()->CreateNodeBefore(CS_NODE_ELEMENT);
                node->SetValue("category");
                node->SetAttribute("name", name);
                node->SetAttribute("attribute", attrNode);
                node->SetAttribute("value", "0");
            }
            else
            {
                Error2("Error: Found <animal-affinity name=\"%s\"/>, but that isn't a valid affinity category.", name.GetDataSafe());
            }
        }

        // Modify Value
        if(node)
        {
            float oldValue = node->GetAttributeValueAsFloat("value");
            float delta = value->Evaluate(env);
            node->SetAttributeAsFloat("value", oldValue + delta);
        }

        // Save changes back
        scfString str;
        xmlDoc->Write(&str);
        chr->SetAnimialAffinity(str);
    }
};

//----------------------------------------------------------------------------

/**
 * ActionOp activate any inactive entrance and create a key for it.
 * 
 * Activates any inactive entrance action location of the specified entrance
 * type and places into players inventory a key for the lock instance ID defined
 * in that action location entrance.
 *
 * - Syntax:
 * @code
 *
 *     <action sector="%s" stat="%s"/>
 *
 * @endcode
 * sector = "%s" sector string to qualify search for inactive entrances\n
 * stat = "%s" name of item type for new key to the lock of the entrance\n
 * - Examples:
 * @code
 * 
 *    <action sector="guildlaw" stat="Small Key"/>
 *
 * @endcode
 * This quest script activates any inactive action location for sector guildlaw and gives a "Small Key" item.
 */
class ActionOp : public ImperativeAim
{
public:
    ActionOp(CacheManager* cachemanager)
    {
        cacheManager = cachemanager;
    }
    virtual ~ActionOp() { }

    
    bool Load(iDocumentNode* node)
    {
        sector = node->GetAttributeValue("sector");
        keyStat = node->GetAttributeValue("stat");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        // Get character data
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        // Returns the next inactive entrance action location
        psActionLocation* actionLocation = psserver->GetActionManager()->FindAvailableEntrances(sector);
        if(!actionLocation)
        {
            Error2("Error: <action/> - no available action location entrances found for %s.\n", sector.GetData());
            return;
        }

        // Activate the action location
        actionLocation->SetActive(true);
        actionLocation->Save();

        // Get lock ID for this entrance
        uint32 lockID = actionLocation->GetInstanceID();
        if(!lockID)
        {
            Error2("Error: <action/> - no available action location entrances found for %s.\n", sector.GetData());
            return;
        }

        // Get the ItemStats based on the name provided.
        psItemStats* itemstats=cacheManager->GetBasicItemStatsByName(keyStat.GetData());
        if(!itemstats)
        {
            Error2("Error: <action stat=\"%s\"/> specified, but no corresponding psItemStats found.\n", keyStat.GetData());
            return;
        }

        // Make 1 master key item
        psItem* masterkeyItem = itemstats->InstantiateBasicItem();
        CS_ASSERT(masterkeyItem);

        // Assign the lock and load it
        masterkeyItem->SetIsKey(true);
        masterkeyItem->SetIsMasterKey(true);
        masterkeyItem->AddOpenableLock(lockID);
        masterkeyItem->SetMaxItemQuality(50.0);
        masterkeyItem->SetStackCount(1);

        // Give to player
        masterkeyItem->SetLoaded();
        c->Inventory().AddOrDrop(masterkeyItem, false);

        // Make 10 regular key items
        psItem* keyItem = itemstats->InstantiateBasicItem();
        CS_ASSERT(keyItem);

        // Assign the lock and load it
        keyItem->SetIsKey(true);
        keyItem->AddOpenableLock(lockID);
        keyItem->SetMaxItemQuality(50.0);
        keyItem->SetStackCount(10);

        // Give to player
        keyItem->SetLoaded();
        c->Inventory().AddOrDrop(keyItem, false);
    }

protected:
    csString sector;            ///< sector name of action location entrance to activate
    csString keyStat;           ///< Item stat name to use for making new key

    CacheManager* cacheManager;
};

//----------------------------------------------------------------------------

/**
 * KeyOp
 * There are two functions of this script.  The make function
 *  will create a new master key for the specified lock.  The modify
 *  function will change existing key to work with lock.
 *
 * Syntax:
 *    \<key funct="make" lockID="#" stat="%s" location="inventory"|"ground"  /\>
 *        funct = "make" makes a key for specific lock
 *        lockID = "#" instance ID of lock to associate with key
 *        stat = "%s" name of item type to make a key for lock
 *        location = "inventory" put new key in actiors inventory
 *        location = "ground" put new key on groud
 *    \<key funct="modify" lockID="#" keyID="#" /\>
 *        funct = "modify" changes the key to work with specific lock
 *        lockID = "#" instance ID of lock to associate with key
 *        keyID = "#" instance ID of key to change to work with lock
 * Example:
 *    Crate a new Small Key and change lock instance 75 to open with new key and put key into actors inventory:
 *        \<key funct="make" lockID="75" stat="Small Key" location="inventory" /\>
 */
class KeyOp : public ImperativeAim
{
public:
    KeyOp(CacheManager* cachemanager) : ImperativeAim()
    {
        cacheManager = cachemanager;
    }
    virtual ~KeyOp() { }

    bool Load(iDocumentNode* node)
    {
        csString funct(node->GetAttributeValue("funct"));
        if(funct == "make")
        {
            function = MAKE;
            lockID = node->GetAttributeValueAsInt("lockID");
            keyStat = node->GetAttributeValue("stat");
            location = node->GetAttributeValue("location");
            if(!location.IsEmpty() && location != "inventory" && location != "ground")
            {
                Error2("Error: <key funct=\"make\" location=\"%s\"/> is not valid.\n", location.GetData());
                return false;
            }
        }
        else if(funct == "modify")
        {
            function = MODIFY;
            keyID = node->GetAttributeValueAsInt("statID");
            lockID = node->GetAttributeValueAsInt("lockID");
        }
        else
        {
            Error2("Error: <key funct=\"%s\"/> is invalid.\n", funct.GetData());
            return false;
        }
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        // Get character data
        psCharacter* c = GetCharacter(env, aim);
        if (!c)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        switch(function)
        {
            case MAKE:
            {
                // Get the ItemStats based on the name provided.
                psItemStats* itemstats = cacheManager->GetBasicItemStatsByName(keyStat.GetData());
                if(!itemstats)
                {
                    Error2("Error: <key funct=\"make\" stat=\"%s\"/> specified, but no corresponding psItemStats found.\n", keyStat.GetData());
                    return;
                }

                psItem* keyItem = itemstats->InstantiateBasicItem();
                CS_ASSERT(keyItem);

                // Assign the lock, make it a master key and load it
                if(lockID)
                {
                    keyItem->SetIsMasterKey(true);
                    keyItem->AddOpenableLock(lockID);
                    keyItem->SetLoaded();
                }

                // Now put it somewhere
                if(location == "inventory")
                {
                    c->Inventory().AddOrDrop(keyItem, false);
                }
                else // location == "ground"
                {
                    c->DropItem(keyItem);
                }
                break;
            }

            case MODIFY:
            {
                // Get key item from bulk and assign the lock
                psItem* keyItem = c->Inventory().FindItemID(keyID);
                if(keyItem)
                {
                    keyItem->AddOpenableLock(lockID);
                    keyItem->Save(false);
                }
                else
                {
                    Error2("Error: <key funct=\"modify\"/> - key ItemID %u is not found!", keyID);
                }
                break;
            }
        }
    }

protected:
    enum KeyOpFunctions
    {
        MAKE,  // Make new key
        MODIFY // Modify existing key
    } function;                 ///< Operation function
    csString location;          ///< Location of where to create key
    uint32 lockID;              ///< Instance ID of lock to assign to key
    csString keyStat;           ///< Item stat name to use for making new key
    uint32 keyID;               ///< Key instance ID to check lock

    CacheManager* cacheManager;
};

//----------------------------------------------------------------------------

/**
 * ItemOp - create items or money and give them to a player
 * @code
 * <item aim="target" name="..." location="inventory|ground" count="..."/>
 * @endcode
 * 
 * Examples:
 * - Drop a stack of 5 longswords on the groud at the foot of the targeted player:
 * @code
 *   <item aim="Target" name="Longsword" location="ground" count="5" />
 * @endcode
 * - Add 5 circles to the player's wallet:
 * @code
 *   <item aim="Target" name="Tria" location="inventory" count="5" />
 * @endcode
 */
class ItemOp : public ImperativeAim
{
public:
    ItemOp(CacheManager* cachemanager) : ImperativeAim(), count(NULL)
    {
        cacheManager = cachemanager;
    }

    virtual ~ItemOp()
    {
        if(count)
        {
            delete count;
        }
    }

    bool Load(iDocumentNode* node)
    {
        name = node->GetAttributeValue("name");
        count = MathExpression::Create(node->GetAttributeValue("count"));

        csString location = node->GetAttributeValue("location");
        if(!location.IsEmpty() && location != "inventory" && location != "ground")
        {
            Error2("Error: Invalid location in <item location=\"%s\"/>\n", location.GetData());
            return false;
        }
        placeOnGround = location == "ground";

        //loads data about the randomization status of the item.
        randomize = node->GetAttributeValueAsBool("randomize");
        randomCost = node->GetAttributeValueAsInt("randomcost", 1000);
        randomLevel = node->GetAttributeValueAsInt("randomlevel", 3);

        return !name.IsEmpty() && count && ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        psCharacter* c = GetCharacter(env, aim);
        if (!c) {
            Error2("Error while executing ItemOp script giving item %s, invalid target character",name.GetData());
            return;
        }
        int stackCount = count->Evaluate(env);
        if(stackCount <= 0)
        {
            return;
        }

        if(placeOnGround)
        {
            psItem* iteminstance = CreateItem(true, stackCount);
            if(!iteminstance)
                return;

            c->DropItem(iteminstance);
        }
        else if(name.CompareNoCase("tria") || name.CompareNoCase("hexa") || name.CompareNoCase("octa") || name.CompareNoCase("circle"))
        {
            // Handle money specially
            psMoney money;

            if(name.CompareNoCase("tria"))
                money.SetTrias(stackCount);
            else if(name.CompareNoCase("hexa"))
                money.SetHexas(stackCount);
            else if(name.CompareNoCase("octa"))
                money.SetOctas(stackCount);
            else if(name.CompareNoCase("circle"))
                money.SetCircles(stackCount);

            psMoney charMoney = c->Money();
            charMoney = charMoney + money;
            c->SetMoney(charMoney);
        }
        else
        {
            psItem* iteminstance = CreateItem(false, stackCount);
            if(!iteminstance)
                return;

            c->Inventory().AddOrDrop(iteminstance, false);
        }
    }
    
protected:
    /**
     * helper function that creates an item according to the specs
     * @param transient true to remove from world if not picked up within some time
     * @param stackCount amount of items (if stackable)
     */
    psItem* CreateItem(bool transient, int stackCount)
    {
        // Get the ItemStats based on the name provided.
        psItemStats* itemstats = cacheManager->GetBasicItemStatsByName(name.GetData());
        if(!itemstats)
        { 
            Error2("Error: <item name=\"%s\"/> specified, but no corresponding psItemStats exists.\n", name.GetData());
            return NULL;
        }

        psItem* item = itemstats->InstantiateBasicItem(transient);
        if(!item)
            return NULL;

        if(!item->GetIsStackable())
        {
            Error3("Error: <item name=\"%s\" count=\"%d\"/> specified, but that item isn't stackable.\n", name.GetData(), stackCount);
        }
        else
        {
            item->SetStackCount(stackCount);
        }


        //if we have to randomize the item we do it now.
        if(randomize)
            cacheManager->RandomizeItem(item, randomCost, randomLevel);

        item->SetLoaded();  // Item is fully created

        return item;
    }

protected:
    csString name;
    bool randomize;  ///< If true it will randomize the item upon creation.
    int randomCost;  ///< The maximum allowed cost for the item.
    int randomLevel; ///< The amount of modifiers allowed
    MathExpression* count;
    bool placeOnGround;
    CacheManager* cacheManager;
};

//----------------------------------------------------------------------------

/**
 * CreateFamiliarOp
 * Create familiar for actor.
 *
 * Syntax:
 *    \<createfamiliar aim="Actor" masterId="masterId" /\>
 * Examples:
 *    Create a familiar near actor and send message:
 *        \<createfamiliar aim="Actor" masterId="1001"/\>\<msg text="Your new familiar appears nearby."/\>
 */
class CreateFamiliarOp : public ImperativeAim
{
public:
    CreateFamiliarOp(EntityManager* entitymanager)
    {
        entityManager = entitymanager;
    }
    virtual ~CreateFamiliarOp() { }

    bool Load(iDocumentNode* node)
    {
        masterPID = node->GetAttributeValueAsInt("masterID");
        return ImperativeAim::Load(node);
    }

    void Run(MathEnvironment* env)
    {
        gemActor* actor = GetActor(env, aim);
        if (!actor)
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }
        if(!actor->GetClientID())
        {
            Error2("Error: <createfamiliar/> needs a valid client for actor '%s'.\n", actor->GetName());
            return;
        }

        gemNPC* familiar = entityManager->CreateFamiliar(actor, masterPID);
        if(!familiar)
        {
            Error2("Failed to create familiar for %s.\n", actor->GetName());
        }
    }

private:
    PID masterPID;
    EntityManager* entityManager;
};

/**
 * TutorialMsgOp activates a tutorial window on the client with the specified message.
 * The message is taken from tips table in the database.
 *
 * Syntax:
 * @code
 *    <tutorialmsg aim="Target" num="TutorialTipID"/>
 * @endcode
 */
class TutorialMsgOp : public ImperativeAim
{
public:
    TutorialMsgOp() : ImperativeAim(), expr(0) { }
    virtual ~TutorialMsgOp()
    {
        delete expr;
    }

    bool Load(iDocumentNode* top)
    {
        expr = MathExpression::Create(top->GetAttributeValue("num"));
        return ImperativeAim::Load(top) && expr!=NULL;
    }

    virtual void Run(MathEnvironment* env)
    {
        unsigned int num = (unsigned int) expr->Evaluate(env);

        gemActor* actor = GetActor(env, aim);
        if(!actor || !actor->GetClientID())
        {
            Error2("Error: <tutorialmsg/> needs a valid client for actor '%s'.\n", !actor ? "null" : actor->GetName());
            return;
        }
        //removing 1 to have correspondance with numbering 5 -> 1005
        num--;
        psserver->GetTutorialManager()->HandleScriptMessage(actor->GetClientID(), num);
    }

protected:
    MathExpression* expr;               ///< The database ID (subtracted of 1001) of the tutorial tip
};

/**
 * MechanismMsgOp - activate a mechanism:
 *
 * \<mechanism aim="Target" mesh="Mesh" move="Move" rot="Rot" /\>
 */
class MechanismMsgOp : public ImperativeAim
{
public:
    MechanismMsgOp() : ImperativeAim(), expr1(NULL), expr2(NULL), expr3(NULL)
    {
    }

    virtual ~MechanismMsgOp()
    {
        delete expr1;
        delete expr2;
        delete expr3;
    }

    bool Load(iDocumentNode* node)
    {
        mesh = node->GetAttributeValue("mesh");
        move = node->GetAttributeValue("move");
        rot = node->GetAttributeValue("rot");
        expr1 = MathExpression::Create(node->GetAttributeValue("mesh"));
        expr2 = MathExpression::Create(node->GetAttributeValue("move"));
        expr3 = MathExpression::Create(node->GetAttributeValue("rot"));

        return ImperativeAim::Load(node) && expr1 && expr2 && expr3;
    }

    virtual void Run(MathEnvironment* env)
    {
        Debug1(LOG_ACTIONLOCATION,0,"Running MechanismMsgOp");

        //evaluate the variable so we can get it's value
        MathVar* meshVar = env->Lookup("Mesh");
        if(!meshVar)
        {
            Error1("MechanismMsgOp Run with invalid 'Mesh' name variable.");
            return;
        }
        mesh = meshVar->GetString();
        Debug2(LOG_ACTIONLOCATION,0,"MechanismMsgOp Run with mesh name variable: %s", mesh.GetData());

        MathVar* moveVar = env->Lookup("Move");
        MathVar* rotVar = env->Lookup("Rot");
        if(!moveVar && !rotVar)
        {
            Error1("MechanismMsgOp Run needs at least one between move and rot parameters");
            return;
        }
        move = moveVar ? moveVar->GetString() : "";
        rot = rotVar ? rotVar->GetString() : "";
        Debug3(LOG_ACTIONLOCATION,0,"MechanismMsgOp Run with move variable: %s and rot variable: %s", move.GetData(), rot.GetData());

        gemActor* actor = GetActor(env, aim);
        if(!actor || !actor->GetClientID())
        {
            Error2("Invalid Aim >%s< - check if you mixed up Actor and Target", aim.GetData());
            return;
        }

        // send message to client about mechanism activated
        {
            psMechanismActivateMessage msg(actor->GetClientID(), mesh.GetData(), move.GetData(), rot.GetData());
            msg.SendMessage();
            Debug1(LOG_ACTIONLOCATION,0,"Running MechanismMsgOp - SENT message to client");
        }

        // send message to all players in range
        csArray<PublishDestination> proxList = actor->GetProxList()->GetClients();
        for(size_t i = 0; i < proxList.GetSize(); i++)
        {
            if(actor->GetClientID() == proxList[i].client)
            {
                continue;
            }
            psMechanismActivateMessage msg(proxList[i].client, mesh.GetData(), move.GetData(), rot.GetData());
            msg.SendMessage();
        }
    }

protected:
    csString mesh;        ///< the mesh to activate
    csString move;        ///< the movement coords x,y,z
    csString rot;         ///< the rotation matrix
    MathExpression* expr1;
    MathExpression* expr2;
    MathExpression* expr3;
};

//============================================================================
// Progression script implementation (imperative mode)
//============================================================================
ProgressionScript::~ProgressionScript()
{
    while(!ops.IsEmpty())
    {
        delete ops.Pop();
    }
}

ProgressionScript* ProgressionScript::Create(EntityManager* entitymanager,CacheManager* cachemanager, const char* name, const char* script)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(script);
    if(error)
    {
        Error2("Couldn't parse XML for progression script >%s<.", name);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in progression script >%s<.", name);
        return NULL;
    }
    csRef<iDocumentNode> top = root->GetNode("script");
    if(!top)
    {
        Error2("Could not find <script> tag in progression script >%s<!", name);
        return NULL;
    }
    return Create(entitymanager, cachemanager, name, top);
}

ProgressionScript* ProgressionScript::Create(EntityManager* entitymanager, CacheManager* cachemanager, const char* name, iDocumentNode* top)
{
    // Note that this doesn't check that top is a particular kind of node;
    // it might be a top-level <script>, or a nested <if>, <let>, or <on>...
    csRef<iDocumentNodeIterator> it = top->GetNodes();

    ProgressionScript* script = new ProgressionScript(name);
    while(it->HasNext())
    {
        csRef<iDocumentNode> node = it->Next();

        if(node->GetType() != CS_NODE_ELEMENT)  // not sure if this is really necessary...
            continue;

        csString elem = node->GetValue();

        ImperativeOp* op = NULL;

        if(elem == "let")
        {
            op = new LetOp(entitymanager, cachemanager);
        }
        else if(elem == "if")
        {
            op = new IfOp(entitymanager, cachemanager);
        }
        else if(elem == "exp")
        {
            op = new ExpOp;
        }
        else if(elem == "apply")
        {
            op = new ApplyOp(entitymanager, cachemanager);
        }
        else if(elem == "apply-linked")
        {
            op = new ApplyLinkedOp(entitymanager, cachemanager);
        }
        else if(elem == "msg")
        {
            op = new MsgOp;
        }
        else if(elem == "npccmd")
        {
            op = new NPCCmdOp;
        }
        else if(elem == "fx")
        {
            op = new FxOp;
        }
        else if(elem == "cancel")
        {
            op = new CancelOp;
        }
        else if(elem == "destroy")
        {
            op = new DestroyOp(entitymanager);
        }
        else if(elem == "teleport")
        {
            op = new TeleportOp(entitymanager);
        }
        else if(elem == "fog" || elem == "rain" || elem == "snow" || elem == "lightning" || elem == "weather")
        {
            printf("TODO: implement <%s> used in script >%s<\n", elem.GetData(), name);
            continue;
        }
        else if(elem == "hp")
        {
            op = new HPOp;
        }
        else if(elem == "mana" || elem == "pstamina" || elem == "mstamina")
        {
            op = new VitalOp;
        }
        else if(elem == "agi" || elem == "end" || elem == "str" || elem == "cha" || elem == "int" || elem == "wil")
        {
            op = new StatsOp;
        }
        else if(elem == "skill")
        {
            op = new SkillOp(cachemanager);
        }
        else if (elem == "trait")
        {
            op = new TraitOp(cachemanager);
        }
        else if (elem == "animation")
        {
            op = new AnimationOp(cachemanager);
        }
        else if(elem == "faction")
        {
            op = new FactionOp(cachemanager);
        }
        else if(elem == "variableset")
        {
            op = new VariableSetOp();
        }
        else if(elem == "variableunset")
        {
            op = new VariableUnSetOp();
        }
        else if(elem == "animal-affinity")
        {
            op = new AnimalAffinityOp;
        }
        else if(elem == "action")
        {
            op = new ActionOp(cachemanager);
        }
        else if(elem == "key")
        {
            op = new KeyOp(cachemanager);
        }
        else if(elem == "item")
        {
            op = new ItemOp(cachemanager);
        }
        else if(elem == "create-familiar")
        {
            op = new CreateFamiliarOp(entitymanager);
        }
        else if(elem == "tutorialmsg")
        {
            op = new TutorialMsgOp();
        }
        else if(elem == "mechanism")
        {
            op = new MechanismMsgOp();
        }
        else
        {
            Error3("Unknown operation >%s< in script >%s< - validate against the schema!", elem.GetData(), name);
            return NULL;
        }

        if(op->Load(node))
        {
            script->ops.Push(op);
        }
        else
        {
            Error3("Operation >%s< failed loading in script >%s<", elem.GetData(), name);
            delete op;
            delete script;
            return NULL;
        }
    }
    return script;
}

void ProgressionScript::Run(MathEnvironment* env)
{
    csArray<ImperativeOp*>::Iterator it = ops.GetIterator();
    while(it.HasNext())
    {
        ImperativeOp* op = it.Next();
        op->Run(env);
    }
}

/**
 * @} */

