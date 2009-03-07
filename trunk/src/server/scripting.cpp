/*
 * scripting.cpp - by Kenny Graunke <kenny@whitecape.org>
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
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/psxmlparser.h"
#include "rpgrules/factions.h"
#include "bulkobjects/pstrait.h"
#include "bulkobjects/activespell.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "scripting.h"
#include "actionmanager.h"
#include "cachemanager.h"
#include "entitymanager.h"
#include "clients.h"
#include "events.h"
#include "gem.h"
#include "psserver.h"

//============================================================================
// Convenience Functions
//============================================================================

gemObject *GetObject(const MathEnvironment *env, const csString & varName)
{
    MathVar *var = env->Lookup(varName);
    if (!var)
        return NULL;

    gemObject *obj = dynamic_cast<gemObject*>(var->GetObject()); // cast from iScriptableVar
    return obj;
}

gemActor *GetActor(const MathEnvironment *env, const csString & varName)
{
    MathVar *var = env->Lookup(varName);
    if (!var)
        return NULL;

    gemActor *actor = dynamic_cast<gemActor*>(var->GetObject()); // cast from iScriptableVar
    return actor;
}

psCharacter *GetCharacter(const MathEnvironment *env, const csString & varName)
{
    MathVar *var = env->Lookup(varName);
    if (!var)
        return NULL;

    // Try a direct cast...
    psCharacter *c = dynamic_cast<psCharacter*>(var->GetObject()); // cast from iScriptableVar
    if (!c)
    {
        // Maybe it's an actor - if so, we can extract it...
        gemActor *a = dynamic_cast<gemActor*>(var->GetObject());
        if (a)
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
    psCancelSpellEvent(csTicks duration, ActiveSpell *asp) : psGameEvent(0, duration, "psCancelSpellEvent"), asp(asp) { }

    void Trigger()
    {
        if (!asp.IsValid())
            return;

        if (asp->Cancel())
            delete asp;
    }

protected:
    csWeakRef<ActiveSpell> asp;
};

//============================================================================
// Applied mode operations
//============================================================================

class AppliedOp
{
public:
    virtual bool Load(iDocumentNode *node) = 0;
    virtual void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp) = 0;
};

// A base class with a "value" attribute backed by a MathExpression.
class Applied1 : public AppliedOp
{
public:
    Applied1() : AppliedOp(), value(NULL) { }
    virtual ~Applied1()
    {
        delete value;
    }

    bool Load(iDocumentNode *node)
    {
        value = MathExpression::Create(node->GetAttributeValue("value"));
        return value != NULL;
    }

protected:
    MathExpression *value; //< an embedded MathExpression
};

// A base class with both "value" and a plain text "name" attribute.
class Applied2 : public Applied1
{
public:
    Applied2() : Applied1() { }
    virtual ~Applied2() { }

    bool Load(iDocumentNode *node)
    {
        name = node->GetAttributeValue("name");
        return Applied1::Load(node);
    }

protected:
    csString name; //< plain text - name of something, like a skill or faction
};

//----------------------------------------------------------------------------

// Applied mode vitals.
class VitalAOp : public Applied1
{
public:
    VitalAOp() : Applied1() { }
    virtual ~VitalAOp() { }

    bool Load(iDocumentNode *node)
    {
        vital = node->GetValue();
        return Applied1::Load(node);
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        VitalBuffable *buffable = NULL;
        if (vital == "hp-rate")
            buffable = &target->GetCharacterData()->GetHPRate();
        else if (vital == "mana-rate")
            buffable = &target->GetCharacterData()->GetManaRate();
        else if (vital == "pstamina-rate")
            buffable = &target->GetCharacterData()->GetPStaminaRate();
        else if (vital == "mstamina-rate")
            buffable = &target->GetCharacterData()->GetMStaminaRate();
        else if (vital == "hp-max")
            buffable = &target->GetCharacterData()->GetMaxHP();
        else if (vital == "mana-max")
            buffable = &target->GetCharacterData()->GetMaxMana();
        else if (vital == "pstamina-max")
            buffable = &target->GetCharacterData()->GetMaxPStamina();
        else if (vital == "mstamina-max")
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

// Applied mode stats.
class StatsAOp : public Applied1
{
public:
    StatsAOp() : Applied1() { }
    virtual ~StatsAOp() { }

    bool Load(iDocumentNode *node)
    {
        csString type(node->GetValue());
        if (type == "agi")
            stat = PSITEMSTATS_STAT_AGILITY;
        else if (type == "end")
            stat = PSITEMSTATS_STAT_ENDURANCE;
        else if (type == "str")
            stat = PSITEMSTATS_STAT_STRENGTH;
        else if (type == "cha")
            stat = PSITEMSTATS_STAT_CHARISMA;
        else if (type == "int")
            stat = PSITEMSTATS_STAT_INTELLIGENCE;
        else if (type == "wil")
            stat = PSITEMSTATS_STAT_WILL;
        else
            return false;

        return Applied1::Load(node);
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        int val = (int) value->Evaluate(env);
        CharStat & buffable = target->GetCharacterData()->Stats()[stat];
        buffable.Buff(asp, val);

        const char *strs[] = {"str", "agi", "end", "int", "wil", "cha"};
        asp->Add(buffable, "<%s value=\"%d\"/>", strs[stat], val);
    }

protected:
    PSITEMSTATS_STAT stat; //< which stat we're actually buffing
};

//----------------------------------------------------------------------------

// Applied mode skills.
class SkillAOp : public Applied2
{
public:
    SkillAOp() : Applied2() { }
    virtual ~SkillAOp() { }

    bool Load(iDocumentNode *node)
    {
        psSkillInfo *info = CacheManager::GetSingleton().GetSkillByName(node->GetAttributeValue("name"));
        if (!info)
            return false;
        skill = info->id;
        return Applied1::Load(node);
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        int val = (int) value->Evaluate(env);
        SkillRank & buffable = target->GetCharacterData()->GetSkillRank(skill);
        buffable.Buff(asp, val);

        asp->Add(buffable, "<skill name=\"%s\" value=\"%d\"/>", name.GetData(), val);
    }

protected:
    PSSKILL skill;
};

//----------------------------------------------------------------------------

// Applied mode support for attack and defense modifiers.
class CombatModAOp : public Applied1
{
public:
    CombatModAOp() : Applied1() { }
    virtual ~CombatModAOp() { }

    bool Load(iDocumentNode *node)
    {
        type = node->GetValue();
        return Applied1::Load(node);
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        Multiplier *mod = NULL;
        if (type == "atk")
            mod = &target->GetCharacterData()->AttackModifier();
        else if (type == "def")
            mod = &target->GetCharacterData()->DefenseModifier();
        CS_ASSERT(mod);

        float val = value->Evaluate(env);
        mod->Buff(asp, val);
        asp->Add(*mod, "<%s value=\"%f\"/>", type.GetData(), val);
    }

protected:
    csString type;
};

//----------------------------------------------------------------------------

// Applied mode mesh overriding.
class MeshAOp : public AppliedOp
{
public:
    MeshAOp() : AppliedOp() { }
    virtual ~MeshAOp() { }

    bool Load(iDocumentNode *node)
    {
        value = node->GetAttributeValue("value");
        return !value.IsEmpty();
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        target->GetOverridableMesh().Override(asp, value);
        asp->Add(target->GetOverridableMesh(), "<mesh value=\"%s\"/>", value.GetData());
    }

protected:
    csString value;
};

//----------------------------------------------------------------------------

// A cancel action that sends the "undo" part of <msg text="..." undo="..."/>.
class MsgCancel : public iCancelAction
{
public:
    MsgCancel(int clientnum, const csString & undo) : clientnum(clientnum), undo(undo) { }

    void Cancel()
    {
        if (!undo.IsEmpty())
            psserver->SendSystemInfo(clientnum, "%s", undo.GetData());
    }
protected:
    int clientnum;
    csString undo;
};

// Applied mode messages - with both text and undo.
class MsgAOp : public AppliedOp
{
public:

    MsgAOp() : AppliedOp() { }
    virtual ~MsgAOp() { }

    bool Load(iDocumentNode *node)
    {
        text = node->GetAttributeValue("text");
        undo = node->GetAttributeValue("undo");
        return true;
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        if (target && target->GetClientID())
        {
            psString finalText(text);
            finalText.Interpolate(env);

            psserver->SendSystemInfo(target->GetClientID(), finalText);

            psString finalUndo(undo);
            finalUndo.Interpolate(env);

            MsgCancel *cancel = new MsgCancel(target->GetClientID(), finalUndo);
            asp->Add(cancel, "<msg text=\"%s\" undo=\"%s\"/>", finalText.GetData(), finalUndo.GetData());
        }
    }

protected:
    csString text; //< message to send immediately
    csString undo; //< message to send when canceled
};

//----------------------------------------------------------------------------

class FxCancel : public iCancelAction
{
public:
    FxCancel(gemObject *target, uint32_t uid) : target(target), msg(uid) { }

    virtual void Cancel()
    {
        // this broadcasts to different clients than the original...
        // which...probably isn't a good thing...
        msg.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }
private:
    gemObject *target;
    psStopEffectMessage msg;
};

/**
 * FxAOp - applied effects.
 *
 * Effects have both a source (i.e. caster) and target.  For applied effects,
 * the target is always the character the ActiveSpell applies to.
 *
 * The basic form is: <fx name="Freezing Mist"/>.  Here, the source and target
 * are both the same entity.
 *
 * For effects coming from a different source, there are two forms:
 * 1. Coming from a gem entity:
 *    <fx name="Freezing Mist" source="Caster"/>
 * 2. Coming from a fixed offset from the target:
 *    <fx name="Freezing Mist" x="-4.2" y="0.3" z="-12.7"/>
 *
 * We cannot persist the first form, since the entity may not exist when we
 * later reload the script.  But the second serves as a good approximation:
 * the spell will appear to have come from the same position as the original
 * entity.
 */
class FxAOp : public AppliedOp
{
public:
    FxAOp() : AppliedOp() { }
    virtual ~FxAOp() { }

    bool Load(iDocumentNode *node)
    {
        name   = node->GetAttributeValue("name");
        source = node->GetAttributeValue("source");
        if (source.IsEmpty())
        {
            offset.x = node->GetAttributeValueAsFloat("x");
            offset.y = node->GetAttributeValueAsFloat("y");
            offset.z = node->GetAttributeValueAsFloat("z");
        }
        return !name.IsEmpty();
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        gemObject *anchor = NULL;
        // Convert from the source to an offset.
        if (!source.IsEmpty())
        {
            anchor = GetObject(env, source);

            iSector *sector;
            csVector3 sourcePos;
            csVector3 targetPos;
            anchor->GetPosition(sourcePos, sector);
            target->GetPosition(targetPos, sector);
            offset = targetPos - sourcePos;
        }

        // Get a unique identifier we can use to later cancel/stop this effect.
        uint32_t uid = CacheManager::GetSingleton().NextEffectUID();
        /*
        if (anchor)
        {
            psEffectMessage fx(0, name, csVector3(0,0,0), anchor->GetEID(), target->GetEID(), uid);
        }
        */

        // this may be backwards
        psEffectMessage fx(0, name, offset, target->GetEID(), target->GetEID(), asp->Duration(), uid);
        if (!fx.valid)
        {
            Error1("Error: <fx> could not create valid psEffectMessage\n");
            return;
        }
        fx.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);

        // One problem with the above...it doesn't get shown to people who walk up to it later.

        FxCancel *cancel = new FxCancel(target, uid);
        if (offset == 0)
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
    csString source;  //< The name of the MathVar containing the entity the effect originates from (optional)
    csVector3 offset; //< An offset from the target which the effect originates from
};

//----------------------------------------------------------------------------

// A cancel action that unregisters an event triggered spell.
class OnCancel : public iCancelAction
{
public:
    OnCancel(gemActor *actor, SCRIPT_TRIGGER type, ProgressionScript *script) : actor(actor), script(script), type(type) { }

    void Cancel()
    {
        if (!actor.IsValid())
            return;

        switch (type) {
            case ATTACK:
                actor->DetachAttackScript(script);
                break;
            case DEFENSE:
                actor->DetachDefenseScript(script);
                break;
            case NEARLYDEAD:
                // TODO.
                break;
        };
    }
protected:
    csWeakRef<gemActor> actor;
    ProgressionScript *script;
    SCRIPT_TRIGGER type;
};

// Support for event triggered scripts.
// This really deserves quite a bit of commentary.
class OnAOp : public AppliedOp
{
public:
    OnAOp() : AppliedOp() { }
    virtual ~OnAOp() { }

    bool Load(iDocumentNode *node)
    {
        csString typ(node->GetAttributeValue("type"));
        if (typ == "attack")
            type = ATTACK;
        else if (typ == "defense")
            type = DEFENSE;
        else if (typ == "nearlydead")
            type = NEARLYDEAD;
        else
        {
            Error2("Invalid type in <on type=\"%s\">.", typ.GetData());
            return false;
        }

        self = node;
        return true;
    }

    void Run(const MathEnvironment *env, gemActor *target, ActiveSpell *asp)
    {
        // Substitute any @{...} expressions.
        if (!Quasiquote(self, env))
            return;

        // Now, parse and load the script (but don't run it)...
        ProgressionScript *body = ProgressionScript::Create("<on> body", self);

        // Register the triggering event
        switch (type) {
            case ATTACK:
                target->AttachAttackScript(body);
                break;
            case DEFENSE:
                target->AttachDefenseScript(body);
                break;
            case NEARLYDEAD:
                // TODO.
                break;
        };
        OnCancel *cancel = new OnCancel(target, type, body);
        csString xml = GetNodeXML(self); // this doesn't give <hp/> style attributes...should fix
                                         // or find another way to do it, nobody else uses this
        asp->Add(cancel, "%s", xml.GetData());
    }

protected:
    bool Quasiquote(iDocumentNode *top, const MathEnvironment *env)
    {
        // 1. Handle quasiquoted expressions in attributes
        csRef<iDocumentAttributeIterator> it = top->GetAttributes();
        while (it->HasNext())
        {
            csRef<iDocumentAttribute> attr = it->Next();
            csString text = attr->GetValue();
            csString varName;
            size_t pos = (size_t)-1;
            // It'd probably be better to use psString::Interpolate and give it an optional char param for @ or $
            while ((pos = text.Find("@{", pos+1)) != SIZET_NOT_FOUND)
            {
                size_t end = text.Find("}", pos+2);
                if (end == SIZET_NOT_FOUND)
                {
                    Error2("Error: Unterminated quasiquote found in attribute >%s<.", text.GetData());
                    return false;
                }
                if (end <= pos+3)
                {
                    Error2("Error: Empty quasiquote - @{} - found in attribute >%s<.", text.GetData());
                    return false;
                }
                text.SubString(varName, pos+2, end-(pos+2));
                MathVar *var = env->Lookup(varName);
                if (!var)
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
        while (nit->HasNext())
        {
            csRef<iDocumentNode> child = nit->Next();
            if (!Quasiquote(child, env))
                return false;
        }
        return true;
    }

    SCRIPT_TRIGGER type;
    csRef<iDocumentNode> self;
};

//============================================================================
// Applicative script implementation (progression script applied mode)
//============================================================================

ApplicativeScript::ApplicativeScript()
{
}

ApplicativeScript::~ApplicativeScript()
{
}

ApplicativeScript* ApplicativeScript::Create(const char *script)
{
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char *error = doc->Parse(script);
    if (error)
    {
        Error2("Couldn't parse XML for applicative script: %s", script);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in applicative script: %s", script);
        return NULL;
    }
    csRef<iDocumentNode> top = root->GetNode("apply");
    if (!top)
    {
        Error2("Could not find <apply> node in: %s", script);
        return NULL;
    }
    return Create(top);
}

ApplicativeScript* ApplicativeScript::Create(iDocumentNode *top)
{
    if (!top->GetAttributeValue("name"))
        return NULL;

    SPELL_TYPE type;
    csString typ(top->GetAttributeValue("type"));
    if (typ == "buff")
        type = BUFF;
    else if (typ == "debuff")
        type = DEBUFF;
    else
        return NULL;

    return Create(top, type, top->GetAttributeValue("name"), top->GetAttributeValue("duration"));
}

ApplicativeScript* ApplicativeScript::Create(iDocumentNode *top, SPELL_TYPE type, const char *name, const char *duration)
{
    CS_ASSERT(name);
    ApplicativeScript *script = new ApplicativeScript;
    if (!script)
        return NULL;

    script->aim = top->GetAttributeValue("aim");
    script->name = name;
    script->type = type;

    script->duration = duration ? MathExpression::Create(duration) : NULL;

    if (script->aim.IsEmpty() || script->name.IsEmpty() || (duration && !script->duration))
    {
        delete script;
        return NULL;
    }

    csRef<iDocumentNodeIterator> it = top->GetNodes();
    while (it->HasNext())
    {
        csRef<iDocumentNode> node = it->Next();

        if (node->GetType() != CS_NODE_ELEMENT) // not sure if this is really necessary...
            continue;

        csString elem = node->GetValue();

        AppliedOp *op = NULL;

        // buffables
        if (elem == "hp-rate" || elem == "mana-rate" || elem == "pstamina-rate" || elem == "mstamina-rate" || elem == "hp-max" || elem == "mana-max" || elem == "pstamina-max" || elem == "mstamina-max")
        {
            op = new VitalAOp;
        }
        else if (elem == "atk" || elem == "def")
        {
            op = new CombatModAOp;
        }
        else if (elem == "agi" || elem == "end" || elem == "str" || elem == "cha" || elem == "int" || elem == "wil")
        {
            op = new StatsAOp;
        }
        else if (elem == "skill")
        {
            op = new SkillAOp;
        }
        else if (elem == "faction")
        {
            continue;
        }
        else if (elem == "animal-affinity")
        {
            continue;
        }
        // overridables
        else if (elem == "mesh")
        {
            op = new MeshAOp;
        }
        else if (elem == "pos")
        {
            continue;
        }
        // other
        else if (elem == "msg")
        {
            op = new MsgAOp;
        }
        else if (elem == "fx")
        {
            op = new FxAOp;
        }
        else if (elem == "on")
        {
            op = new OnAOp;
        }
        else
        {
            // complain (shouldn't happen - script should've been validated against the schema)
            Error2("Unknown operation >%s< - validate against the schema!", elem.GetData());
            return false;
        }

        if (op->Load(node))
        {
            script->ops.Push(op);
        }
        else
        {
            delete op;
            delete script;
            return NULL;
        }
    }
    return script;
}

ActiveSpell* ApplicativeScript::Apply(const MathEnvironment *env, bool registerCancelEvent)
{
    // TODO: Handle non-actor targets...
    gemActor *target = GetActor(env, aim);
    CS_ASSERT(target);

    csTicks dticks = duration ? (csTicks) duration->Evaluate(env) : 0;
    ActiveSpell *asp = new ActiveSpell(name, type, dticks);

    csPDelArray<AppliedOp>::Iterator it = ops.GetIterator();
    while (it.HasNext())
    {
        AppliedOp *op = it.Next();
        op->Run(env, target, asp);
    }

    asp->Register(target);

    if (duration && registerCancelEvent)
    {
        psCancelSpellEvent *evt = new psCancelSpellEvent(dticks, asp);
        psserver->GetEventManager()->Push(evt);
    }

    return asp;
}

//============================================================================
// Imperative mode operations
//============================================================================
class ImperativeOp
{
public:
    ImperativeOp() { }
    virtual ~ImperativeOp() { }

    virtual bool Load(iDocumentNode *node) = 0;
    virtual void Run(const MathEnvironment *env) = 0;
};

//----------------------------------------------------------------------------

/**
 * ApplyOp
 *
 * Creates a new ActiveSpell containing the effects described within, and
 * applies it to the entity aimed at.
 *
 * This is the primary way to enter applied mode.
 */
class ApplyOp : public ImperativeOp
{
public:
    ~ApplyOp()
    {
        if (aps)
            delete aps;
        aps = NULL;
    }

    bool Load(iDocumentNode *top)
    {
        aps = ApplicativeScript::Create(top);
        return aps != NULL;
    }

    virtual void Run(const MathEnvironment *env)
    {
        aps->Apply(env, true);
    }

protected:
    ApplicativeScript *aps;
};

//----------------------------------------------------------------------------

class ApplyLinkedOp : public ImperativeOp
{
public:
    ~ApplyLinkedOp()
    {
        if (buff)
            delete buff;
        if (debuff)
            delete debuff;
        buff = NULL;
        debuff = NULL;
    }

    bool Load(iDocumentNode *top)
    {
        csString name(top->GetAttributeValue("name"));
        if (name.IsEmpty())
            return false;

        csRef<iDocumentNode> buffNode   = top->GetNode("buff");
        csRef<iDocumentNode> debuffNode = top->GetNode("debuff");

        buff   = ApplicativeScript::Create(buffNode,   BUFF,   name, top->GetAttributeValue("duration"));
        debuff = ApplicativeScript::Create(debuffNode, DEBUFF, name, top->GetAttributeValue("duration"));
        return buff && debuff;
    }

    virtual void Run(const MathEnvironment *env)
    {
        ActiveSpell *buffAsp   =   buff->Apply(env, true);  // only bother with one cancel event;
        ActiveSpell *debuffAsp = debuff->Apply(env, false); // the link ensures both will get cancelled

        // Link the two ActiveSpells...
        buffAsp->Link(debuffAsp);
        debuffAsp->Link(buffAsp);
    }
protected:
    ApplicativeScript *buff;   //< The buff (beneficiary) script
    ApplicativeScript *debuff; //< The debuff (victim) script
};

//----------------------------------------------------------------------------

/**
 * LetOp - a way to evaluate MathScript stuff and create new bindings:
 *
 * <let vars="Roll = rnd(100)">
 *   (Roll will be defined here, within the let block.)
 * </let>
 */
class LetOp : public ImperativeOp
{
public:
    LetOp() : ImperativeOp(), bindings(NULL), body(NULL) { }
    virtual ~LetOp()
    {
        if (bindings)
            delete bindings;
        if (body)
            delete body;
    }

    bool Load(iDocumentNode *node)
    {
        bindings = MathScript::Create("<let> bindings", node->GetAttributeValue("vars"));
        body = ProgressionScript::Create("<let> body", node);
        return (bindings && body);
    }

    virtual void Run(const MathEnvironment *outer)
    {
        MathEnvironment inner(outer);
        bindings->Evaluate(&inner);
        body->Run(&inner);
    }

protected:
    MathScript *bindings; /// an embedded MathScript containing new bindings
    ProgressionScript *body;
};

//----------------------------------------------------------------------------

/**
 * IfOp - conditional control flow, based on MathScript:
 *
 * <if t="Roll &lt; 40">
 *   <then>...</then>
 *   <else>...</else>
 * </if>
 *
 * The "then" branch is required, but an "else" branch is optional.
 */
class IfOp : public ImperativeOp
{
public:
    IfOp() : ImperativeOp(), thenBranch(NULL), elseBranch(NULL), condition(NULL) { }
    virtual ~IfOp()
    {
        if (thenBranch)
            delete thenBranch;
        if (elseBranch)
            delete elseBranch;
        if (condition)
            delete condition;
    }

    bool Load(iDocumentNode *node)
    {
        condition = MathExpression::Create(node->GetAttributeValue("t"));
        csRef<iDocumentNode> thenNode = node->GetNode("then");
        csRef<iDocumentNode> elseNode = node->GetNode("else");

        if (!thenNode)
            return false;

        thenBranch = ProgressionScript::Create("<then> clause", thenNode);

        if (elseNode)
            elseBranch = ProgressionScript::Create("<else> clause", elseNode);

        return (condition && thenBranch && (elseBranch || !elseNode));
    }

    virtual void Run(const MathEnvironment *env)
    {
        if (condition->Evaluate(env) != 0.0)
            thenBranch->Run(env);
        else if (elseBranch)
            elseBranch->Run(env);
    }

protected:
    ProgressionScript *thenBranch;
    ProgressionScript *elseBranch;
    MathExpression *condition; /// an embedded MathExpression - should result in a boolean
};

//----------------------------------------------------------------------------

/**
 * MsgOp - one-time message sending:
 *
 * <msg aim="Caster" text="spam spam spam spam...!"/>
 */
class MsgOp : public ImperativeOp
{
public:
    bool Load(iDocumentNode *node)
    {
        aim = node->GetAttributeValue("aim");
        text = node->GetAttributeValue("text");
        return true;
    }

    virtual void Run(const MathEnvironment *env)
    {
        gemActor *actor = GetActor(env, aim);

        if (actor && actor->GetClientID())
        {
            psString finalText(text);
            finalText.Interpolate(env);

            psserver->SendSystemInfo(actor->GetClientID(), finalText);
        }
    }

protected:
    csString aim;  //< name of the MathScript var to aim at
    csString text; //< message to send
};

//----------------------------------------------------------------------------

/**
 * CancelOp - a way to cancel active spells.
 *
 * <cancel type="all">
 *   <spell type="buff" name="Defensive Wind"/>
 *   <spell type="buff" name="Flame Spire"/>
 * </cancel>
 *
 * There are three types:
 * - "all"     ... cancels all listed spells.
 * - "ordered" ... cancels the first listed spell that's actually active.
 * - "random"  ... cancels a random spell from the list (one which is active).
 */
class CancelOp : public ImperativeOp
{
public:
    bool Load(iDocumentNode *cancel)
    {
        aim = cancel->GetAttributeValue("aim");

        csString typ(cancel->GetAttributeValue("type"));
        if (typ == "all")
            type = ALL;
        else if (typ == "ordered")
            type = ORDERED;
        else if (typ == "random")
            type = RANDOM;
        else
        {
            Error2("Invalid type in <cancel type=\"%s\">.", typ.GetData());
            return false;
        }

        csRef<iDocumentNodeIterator> it = cancel->GetNodes();
        while (it->HasNext())
        {
            csRef<iDocumentNode> node = it->Next();

            if (node->GetType() != CS_NODE_ELEMENT) // not sure if this is really necessary...
                continue;

            Spell spell;
            spell.name = node->GetAttributeValue("name");

            typ = node->GetAttributeValue("type");
            if (typ == "buff")
                spell.type = BUFF;
            else if (typ == "debuff")
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

    virtual void Run(const MathEnvironment *env)
    {
        gemActor *actor = GetActor(env, aim);
        CS_ASSERT(actor);

        // Find which potentially cancellable spells are actually active
        csArray<ActiveSpell*> asps;
        for (size_t i = 0; i < spells.GetSize(); i++)
        {
            ActiveSpell *asp = actor->FindActiveSpell(spells[i].name, spells[i].type);
            if (asp)
                asps.Push(asp);
        }

        if (asps.IsEmpty())
            return;

        switch (type)
        {
            case ORDERED:
            {
                if (asps[0]->Cancel())
                    delete asps[0];
                break;
            }
            case RANDOM:
            {
                ActiveSpell *asp = asps[psserver->GetRandom(asps.GetSize())];
                if (asp->Cancel())
                    delete asp;
                break;
            }
            case ALL:
            {
                for (size_t i = 0; i < asps.GetSize(); i++)
                {
                    if (asps[i]->Cancel())
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

    csString aim;  //< name of the MathScript var to aim at
    csArray<Spell> spells; //< list of spells to cancel.
};

//----------------------------------------------------------------------------

class FxOp : public ImperativeOp
{
public:
    bool Load(iDocumentNode *top)
    {
        // type should probably be optional; I'm guessing most cases will want it attached...
        csString typ(top->GetAttributeValue("type"));
        if (typ == "attached")
            attached = true;
        else if (typ == "unattached")
            attached = false;
        else
            return false;

        name      = top->GetAttributeValue("name");
        sourceVar = top->GetAttributeValue("source");
        targetVar = top->GetAttributeValue("target");

        return !name.IsEmpty() && !targetVar.IsEmpty();
    }

    virtual void Run(const MathEnvironment *env)
    {
        gemObject *target = GetObject(env, targetVar);
        gemObject *source = target;
        if (!sourceVar.IsEmpty())
            source = GetObject(env, sourceVar);

        if (attached)
        {
            psEffectMessage fx(0, name, csVector3(0,0,0), source->GetEID(), target->GetEID(), 0);
            if (!fx.valid)
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
            iSector *sector;
            target->GetPosition(pos, yrot, sector);
            pos.x -= DROP_DISTANCE * sinf(yrot);
            pos.z -= DROP_DISTANCE * cosf(yrot);

            // Send effect message
            psEffectMessage fx(0, name, pos, 0, 0, 0);
            if (!fx.valid)
            {
                Error1("Error: <fx> could not create valid psEffectMessage\n");
                return;
            }
            fx.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        }
    }

protected:
    bool attached;
    csString name;    //< the name of the effect
    csString sourceVar;  //< name of the MathVar containing where the effect starts
    csString targetVar;  //< name of the MathVar containing where the effect is aimed
};

//----------------------------------------------------------------------------

class TeleportOp : public ImperativeOp
{
};

//----------------------------------------------------------------------------

class Imperative1 : public ImperativeOp
{
public:
    bool Load(iDocumentNode *node)
    {
        aim = node->GetAttributeValue("aim");
        return !aim.IsEmpty();
    }

protected:
    csString aim;   //< name of the MathScript var to aim at
};

// A base class supporting aim and a MathExpression value.
class Imperative2 : public Imperative1
{
public:
    Imperative2() : Imperative1(), value(NULL) { }
    virtual ~Imperative2()
    {
        delete value;
    }

    bool Load(iDocumentNode *node)
    {
        value = MathExpression::Create(node->GetAttributeValue("value"));
        return value && Imperative1::Load(node);
    }

protected:
    MathExpression *value; //< an embedded MathExpression
};

//----------------------------------------------------------------------------

// A base class supporting aim, value, and name.
class Imperative3 : public Imperative2
{
public:
    Imperative3() : Imperative2() { }
    virtual ~Imperative3() { }

    bool Load(iDocumentNode *node)
    {
        name = node->GetAttributeValue("name");
        return Imperative2::Load(node);
    }

protected:
    csString name; //< plain text - name of something, like a skill or faction
};

//----------------------------------------------------------------------------

/**
 * VitalOp - imperative vitals (HP, mana, stamina)
 *
 * <hp aim="Caster" value="-5"/>
 */
class VitalOp : public Imperative2 
{
public:
    VitalOp() : Imperative2() { }
    virtual ~VitalOp() { }

    bool Load(iDocumentNode *node)
    {
        vital = node->GetValue();
        return Imperative2::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        psCharacter *c = GetCharacter(env, aim);
        float val = value->Evaluate(env);
        if (vital == "hp")
            c->AdjustHitPoints(val);
        else if (vital == "mana")
            c->AdjustMana(val);
        else if (vital == "pstamina")
            c->AdjustStamina(val, true);
        else if (vital == "mstamina")
            c->AdjustStamina(val, false);
        else
            CS_ASSERT(false);
    }

protected:
    csString vital;
};

//----------------------------------------------------------------------------

/**
 * StatsOp - imperative stats (strength, agility, etc.)
 *
 * <str aim="Actor" value="55"/> (adds 55 to the actor's strength)
 *
 * This is primarily used for character creation, as these effects are
 * permanent.  See StatsAOp for the buffed kind.
 */
class StatsOp : public Imperative2
{
public:
    StatsOp() : Imperative2() { }
    virtual ~StatsOp() { }

    bool Load(iDocumentNode *node)
    {
        csString type(node->GetValue());
        if (type == "agi")
            stat = PSITEMSTATS_STAT_AGILITY;
        else if (type == "end")
            stat = PSITEMSTATS_STAT_ENDURANCE;
        else if (type == "str")
            stat = PSITEMSTATS_STAT_STRENGTH;
        else if (type == "cha")
            stat = PSITEMSTATS_STAT_CHARISMA;
        else if (type == "int")
            stat = PSITEMSTATS_STAT_INTELLIGENCE;
        else if (type == "wil")
            stat = PSITEMSTATS_STAT_WILL;
        else
            return false;

        return Imperative2::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        psCharacter *c = GetCharacter(env, aim);
        int val = (int) value->Evaluate(env);
        CharStat & buffable = c->Stats()[stat];
        buffable.SetBase(buffable.Base() + val);
    }

protected:
    PSITEMSTATS_STAT stat; //< which stat we're actually buffing
};

//----------------------------------------------------------------------------

/**
 * SkillOp - imperative skills.
 *
 * <skill aim="Actor" name="Sword" value="5"/> (adds 5 to actor's sword skill.)
 *
 * This is permanent; see SkillAOp for the temporary buffed kind.
 */
class SkillOp : public Imperative3
{
public:
    SkillOp() : Imperative3() { }
    virtual ~SkillOp() { }

    bool Load(iDocumentNode *node)
    {
        psSkillInfo *info = CacheManager::GetSingleton().GetSkillByName(node->GetAttributeValue("name"));
        if (!info)
            return false;
        skill = info->id;
        return Imperative3::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        psCharacter *c = GetCharacter(env, aim);
        int val = (int) value->Evaluate(env);
        SkillRank & buffable = c->GetSkillRank(skill);
        buffable.SetBase(buffable.Base() + val);
    }
protected:
    PSSKILL skill;
};

//----------------------------------------------------------------------------

/**
 * ExpOp - grant experience points.
 *
 * <exp aim="Actor" value="200"/>
 */
class ExpOp : public Imperative2
{
public:
    ExpOp() : Imperative2() { }
    virtual ~ExpOp() { }

    // AllocateKillDamage blah.
    void Run(const MathEnvironment *env)
    {
        psCharacter *c = GetCharacter(env, aim);
        float exp = value->Evaluate(env);
        c->AddExperiencePoints(exp);
    }
protected:
};

//----------------------------------------------------------------------------

/**
 * ActionOp
 * Activates any inactive entrance action location of the specified entrance
 * type and places into players inventory a key for the lock instance ID defined
 * in that action location entrance.
 *
 * Syntax:
 *     <action sector="%s" stat="%s"/>
 *         sector = "%s" sector string to qualify search for inactive entrances
 *         stat = "%s" name of item type for new key for lock
 * Examples:
 *     This quest script activates the any inactive action location for sector guildlaw and give a "Small Key" item.
 *         <action sector="guildlaw" stat="Small Key"/>
 */
class ActionOp : public Imperative1
{
public:
    bool Load(iDocumentNode *node)
    {
        sector = node->GetAttributeValue("sector");
        keyStat = node->GetAttributeValue("stat");
        return Imperative1::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        // Get character data
        psCharacter *c = GetCharacter(env, aim);

        // Returns the next inactive entrance action location
        psActionLocation *actionLocation = psserver->GetActionManager()->FindAvailableEntrances(sector);
        if (!actionLocation)
        {
            Error2("Error: <action/> - no available action location entrances found for %s.\n", sector.GetData());
            return;
        }

        // Activate the action location
        actionLocation->SetActive(true);
        actionLocation->Save();

        // Get lock ID for this entrance
        uint32 lockID = actionLocation->GetInstanceID();
        if (!lockID)
        {
            Error2("Error: <action/> - no available action location entrances found for %s.\n", sector.GetData());
            return;
        }

        // Get the ItemStats based on the name provided.
        psItemStats *itemstats=CacheManager::GetSingleton().GetBasicItemStatsByName(keyStat.GetData());
        if (!itemstats)
        {
            Error2("Error: <action stat=\"%s\"/> specified, but no corresponding psItemStats found.\n", keyStat.GetData());
            return;
        }

        // Make 1 master key item
        psItem *masterkeyItem = itemstats->InstantiateBasicItem();
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
        psItem *keyItem = itemstats->InstantiateBasicItem();
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
};

//----------------------------------------------------------------------------

/**
 * KeyOp
 * There are two functions of this script.  The make function
 *  will create a new master key for the specified lock.  The modify
 *  fucntion will change existing key to work with lock.
 *
 * Syntax:
 *    <key funct="make" lockID="#" stat="%s" location="inventory"|"ground"  />
 *        funct = "make" makes a key for specific lock
 *        lockID = "#" instance ID of lock to associate with key
 *        stat = "%s" name of item type to make a key for lock
 *        location = "inventory" put new key in actiors inventory
 *        location = "ground" put new key on groud
 *    <key funct="modify" lockID="#" keyID="#" />
 *        funct = "modify" changes the key to work with specific lock
 *        lockID = "#" instance ID of lock to associate with key
 *        keyID = "#" instance ID of key to change to work with lock
 * Example:
 *    Crate a new Small Key and change lock instance 75 to open with new key and put key into actors inventory:
 *        <key funct="make" lockID="75" stat="Small Key" location="inventory" />
 */
class KeyOp : public Imperative1
{
public:
    KeyOp() : Imperative1() { };
    virtual ~KeyOp() {};

    bool Load(iDocumentNode *node)
    {
        csString funct(node->GetAttributeValue("funct"));
        if (funct == "make")
        {
            function = MAKE;
            lockID = node->GetAttributeValueAsInt("lockID");
            keyStat = node->GetAttributeValue("stat");
            location = node->GetAttributeValue("location");
            if (!location.IsEmpty() && location != "inventory" && location != "ground" )
            {
                Error2("Error: <key funct=\"make\" location=\"%s\"/> is not valid.\n", location.GetData());
                return false;
            }
        }
        else if (funct == "modify")
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
        return Imperative1::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        // Get character data
        psCharacter *c = GetCharacter(env, aim);

        switch (function)
        {
            case MAKE:
            {
                // Get the ItemStats based on the name provided.
                psItemStats *itemstats = CacheManager::GetSingleton().GetBasicItemStatsByName(keyStat.GetData());
                if (!itemstats)
                {
                    Error2("Error: <key funct=\"make\" stat=\"%s\"/> specified, but no corresponding psItemStats found.\n", keyStat.GetData());
                    return;
                }

                psItem *keyItem = itemstats->InstantiateBasicItem();
                CS_ASSERT(keyItem);

                // Assign the lock, make it a master key and load it
                if (lockID)
                {
                    keyItem->SetIsMasterKey(true);
                    keyItem->AddOpenableLock(lockID);
                    keyItem->SetLoaded();
                }

                // Now put it somewhere
                if (location == "inventory")
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
                psItem *keyItem = c->Inventory().FindItemID(keyID);
                if (keyItem)
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
    enum KeyOpFunctions {
        MAKE,  // Make new key
        MODIFY // Modify existing key
    } function;                 ///< Operation function
    csString location;          ///< Location of where to create key
    uint32 lockID;              ///< Instance ID of lock to assign to key
    csString keyStat;           ///< Item stat name to use for making new key
    uint32 keyID;               ///< Key instance ID to check lock
};

//----------------------------------------------------------------------------

/**
 * ItemOp
 * Create item or money
 *
 * Syntax:
 *    <item aim="target" name="%s" location="wallet|inventory|ground" count="#" />
 *        aim = "target" detach from target rather then default actor
 *        name = "%" item name
 *        location ="wallet" name must be "trias","hexas","octas", or "circles"
 *        location ="inventory" put item directly into inventory
 *        location ="ground" put item onto ground
 * Examples:
 *    Drop a stack of 5 longswords on the groud at the foot of the targeted player:
 *          <item aim="target" name="Longsword" location="ground" count="5" />
 */
class ItemOp : public Imperative1
{
public:
    bool Load(iDocumentNode *node)
    {
        name = node->GetAttributeValue("name");
        location = node->GetAttributeValue("location");
        stackCount = node->GetAttributeValueAsInt("count");
        if (location != "inventory" && location != "wallet" && location != "ground")
        {
            Error2("Error: Invalid location in <item location=\"%s\"/>\n", location.GetData());
            return false;
        }

        if (location == "wallet" && name != "trias" && name != "hexas" && name != "octas" && name != "circles")
        {
            Error2("Error: <item/> specifies wallet as location but non-monetary item >%s<.\n", name.GetData());
            return false;
        }
        return !name.IsEmpty() && stackCount > 0 && Imperative1::Load(node);
    }

    void Run(const MathEnvironment *env)
    {
        psCharacter *c = GetCharacter(env, aim);

        if (location == "wallet")
        {
            psMoney money;

            if (name == "trias")
                money.SetTrias(stackCount);
            else if (name == "hexas")
                money.SetHexas(stackCount);
            else if (name == "octas")
                money.SetOctas(stackCount);
            else if (name == "circles")
                money.SetCircles(stackCount);

            psMoney charMoney = c->Money();
            charMoney = charMoney + money;
            c->SetMoney(charMoney);
        }
        else if (location == "inventory")
        {
            psItem *iteminstance = CreateItem(false);
            if (!iteminstance)
                return;

            c->Inventory().AddOrDrop(iteminstance, false);
        }
        else if (location == "ground")
        {
            psItem *iteminstance = CreateItem(true);
            if (!iteminstance)
                return;

            c->DropItem(iteminstance);
        }
    }

    psItem *CreateItem(bool transient)
    {
        // Get the ItemStats based on the name provided.
        psItemStats *itemstats = CacheManager::GetSingleton().GetBasicItemStatsByName(name.GetData());
        if (!itemstats)
        {
            Error2("Error: <item name=\"%s\"/> specified, but no corresponding psItemStats exists.\n", name.GetData());
            return NULL;
        }

        psItem *item = itemstats->InstantiateBasicItem(transient);
        if (!item)
            return NULL;

        if (!item->GetIsStackable())
        {
            Error3("Error: <item name=\"%s\" count=\"%d\"/> specified, but that item isn't stackable.\n", name.GetData(), stackCount);
        }
        else
        {
            item->SetStackCount(stackCount);
        }
        item->SetLoaded();  // Item is fully created

        return item;
    }

protected:
    csString name;
    csString location;
    int stackCount;
};

//----------------------------------------------------------------------------

/**
 * CreateFamiliarOp
 * Create familiar for actor.
 *
 * Syntax:
 *    <createfamiliar aim="Actor"/>
 * Examples:
 *    Create a familiar near actor and send message:
 *        <createfamiliar aim="Actor"/><msg text="Your new familiar appears nearby."/>
 */
class CreateFamiliarOp : public Imperative1
{
public:
    void Run(const MathEnvironment *env)
    {
        gemActor *actor = GetActor(env, aim);
        if (!actor->GetClientID())
        {
            Error2("Error: <createfamiliar/> needs a valid client for actor '%s'.\n", actor->GetName());
            return;
        }

        if (actor->GetCharacterData()->GetFamiliarID() != 0)
        {
            psserver->SendSystemInfo(actor->GetClientID(), "You already have a familiar, please take care of it.");
            return;
        }

        gemNPC *familiar = EntityManager::GetSingleton().CreateFamiliar(actor);
        if (!familiar)
        {
            Error2("Failed to create familiar for %s.\n", actor->GetName());
        }
    }
};

//============================================================================
// Progression script implementation (imperative mode)
//============================================================================
ProgressionScript::~ProgressionScript()
{
    while (!ops.IsEmpty())
    {
        delete ops.Pop();
    }
}

ProgressionScript* ProgressionScript::Create(const char *name, const char *script)
{
    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char *error = doc->Parse(script);
    if (error)
    {
        Error2("Couldn't parse XML for progression script >%s<.", name);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in progression script >%s<.", name);
        return NULL;
    }
    csRef<iDocumentNode> top = root->GetNode("script");
    if (!top)
    {
        Error2("Could not find <script> tag in progression script >%s<!", name);
        return NULL;
    }
    return Create(name, top);
}

ProgressionScript* ProgressionScript::Create(const char *name, iDocumentNode *top)
{
    // Note that this doesn't check that top is a particular kind of node;
    // it might be a top-level <script>, or a nested <if>, <let>, or <on>...
    csRef<iDocumentNodeIterator> it = top->GetNodes();

    ProgressionScript *script = new ProgressionScript(name);
    while (it->HasNext())
    {
        csRef<iDocumentNode> node = it->Next();

        if (node->GetType() != CS_NODE_ELEMENT) // not sure if this is really necessary...
            continue;

        csString elem = node->GetValue();

        ImperativeOp *op = NULL;

        if (elem == "let")
        {
            op = new LetOp;
        }
        else if (elem == "if")
        {
            op = new IfOp;
        }
        else if (elem == "exp")
        {
            op = new ExpOp;
        }
        else if (elem == "apply")
        {
            op = new ApplyOp;
        }
        else if (elem == "apply-linked")
        {
            op = new ApplyLinkedOp;
        }
        else if (elem == "msg")
        {
            op = new MsgOp;
        }
        else if (elem == "fx")
        {
            op = new FxOp;
        }
        else if (elem == "cancel")
        {
            op = new CancelOp;
        }
        else if (elem == "teleport" || elem == "create-familiar" || elem == "fog" || elem == "rain" || elem == "snow" || elem == "lightning" || elem == "weather" || elem == "animal-affinity")
        {
            printf("TODO: implement <%s> used in script >%s<\n", elem.GetData(), name);
            continue;
        }
        else if (elem == "hp" || elem == "mana" || elem == "pstamina" || elem == "mstamina")
        {
            op = new VitalOp;
        }
        else if (elem == "agi" || elem == "end" || elem == "str" || elem == "cha" || elem == "int" || elem == "wil")
        {
            op = new StatsOp;
        }
        else if (elem == "skill")
        {
            op = new SkillOp;
        }
        else if (elem == "faction")
        {
            printf("TODO: implement imperative factions\n");
            continue;
        }
        else if (elem == "action")
        {
            op = new ActionOp;
        }
        else if (elem == "key")
        {
            op = new KeyOp;
        }
        else if (elem == "item")
        {
            op = new ItemOp;
        }
        else if (elem == "createfamiliar")
        {
            op = new CreateFamiliarOp;
        }
        else
        {
            Error3("Unknown operation >%s< in script >%s< - validate against the schema!", elem.GetData(), name);
            return NULL;
        }

        if (op->Load(node))
        {
            script->ops.Push(op);
        }
        else
        {
            delete op;
            return NULL;
        }
    }
    return script;
}

void ProgressionScript::Run(const MathEnvironment *env)
{
    csArray<ImperativeOp*>::Iterator it = ops.GetIterator();
    while (it.HasNext())
    {
        ImperativeOp *op = it.Next();
        op->Run(env);
    }
}


