/*
 * progressionmanager.cpp
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
#include <stdlib.h>
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>
#include <csutil/scfstr.h>
#include <iengine/engine.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/skillcache.h"
#include "util/eventmanager.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/psxmlparser.h"

#include "rpgrules/factions.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/pstrainerinfo.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pstrait.h"

#include "net/messages.h"
#include "net/npcmessages.h"

#include "engine/psworld.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "clients.h"
#include "psserver.h"
#include "events.h"
#include "psserverchar.h"
#include "playergroup.h"
#include "gem.h"
#include "progressionmanager.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "spellmanager.h"
#include "weathermanager.h"
#include "actionmanager.h"
#include "workmanager.h"
#include "npcmanager.h"
#include "usermanager.h"
#include "introductionmanager.h"
#include "adminmanager.h"


class ScriptOp;


class psScriptGameEvent : public psGEMEvent, public iDeathCallback
{
public:
    psScriptGameEvent(csTicks offsetticks,
                      ProgressionEvent * script,
                      gemActor *actor,
                      gemObject *target,
                      psItem *item,
                      int persistentID);
    ~psScriptGameEvent();

    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);
    virtual void DeathCallback( iDeathNotificationObject * object );

    void Trigger();

protected:
    ProgressionEvent * script;
    int persistentID;
    csWeakRef<gemActor> actor;
    csWeakRef<gemObject> target;
    csWeakRef<psItem> item;
    bool disconnected;
    csArray<MathScriptVar *> state;
};

/*-------------------------------------------------------------*/

class ProgressionOperation
{
protected:
    MathScript *value_script;
    MathScriptVar *valuevar,*targetvar,*actorvar;
    csString script_text,delay_text,result_var_name;
    ProgressionEvent *my_script;

    MathScript *delay_script;
    MathScriptVar *delayvar,*delaytargetvar,*delayactorvar;

    /// has an undo script already been queued
    bool undoQueued;

    float result;

    int ticksElapsed;

    bool LoadValue(iDocumentNode *node, ProgressionEvent *script);
    float GetValue(gemActor * actor, gemObject *target);
    bool LoadDelay(iDocumentNode *node, ProgressionEvent *script);
    int GetDelay(gemActor * actor, gemObject *target);

public:
    csString * eventName;
    ProgressionOperation() { my_script=NULL; valuevar=targetvar=actorvar=NULL; value_script=delay_script=NULL; result =  0.0F; undoQueued = false;}
    virtual ~ProgressionOperation() {if (value_script) delete value_script; if (delay_script) delete delay_script; }
    void SetTicksElapsed(int ticks) { ticksElapsed = ticks; }

    /// The return value from Run is very important. If it is false it prevents the rest of the script from executing.
    virtual bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)=0;
    virtual bool Load(iDocumentNode *node, ProgressionEvent *script)=0;
    virtual csString ToString()=0;
    MathScript *GetMathScript() { return value_script; }
    virtual void LoadVariables(csArray<MathScriptVar*> & variables);
    virtual float GetResult() { return result; };
    virtual csString Absolute();

    void SubstituteVars(gemActor * actor, gemObject *target, psString& str)
    {
        int where = (int) str.FindFirst('$');
        while (where != -1)
        {
            psString word;
            str.GetWord(where+1,word,false);
            MathScriptVar *pv = my_script->FindVariable(word);
            if (pv)
            {
                csString buff;
                buff.Format("%1.0f",pv->GetValue() );
                str.ReplaceSubString(word,buff);
                str.DeleteAt(where);
            }
            else
            {
                if (word.CompareNoCase("target"))
                {
                    str.ReplaceSubString( word, target->GetName() );
                    str.DeleteAt( where );
                }
                else if (word.CompareNoCase("actor"))
                {
                    str.ReplaceSubString( word, actor->GetName() );
                    str.DeleteAt( where );
                }
            }
            where = (int) str.FindFirst('$',where+1);
        }
    }
};

csString ProgressionOperation::Absolute()
{
    if(undoQueued)
        return "";
    return ToString();
}

void ProgressionOperation::LoadVariables(csArray<MathScriptVar*> & variables)
{
    MathScriptVar *var;
    size_t i;

    for (i=0; i<variables.GetSize(); i++)
    {
        if (value_script)
        {
            var = value_script->GetOrCreateVar(variables[i]->name);
            var->SetValue(variables[i]->GetValue() );
        }

        if (delay_script)
        {
            var = delay_script->GetOrCreateVar(variables[i]->name);
            var->SetValue(variables[i]->GetValue() );
        }
    }
}

bool ProgressionOperation::LoadValue(iDocumentNode *node, ProgressionEvent *prg_script)
{
    // check if value is valid
    if (node->GetAttributeValue("value")==NULL || strlen(node->GetAttributeValue("value")) == 0) {
        Error1("Script MUST contain an attribute called \"Value\"");
        return false;
    }

    csString script("Value = ");
    script.Append(node->GetAttributeValue("value"));
    script_text = node->GetAttributeValue("value");  // save for persisting later

    value_script = new MathScript(prg_script->name.GetData(),script);

    valuevar = value_script->GetVar("Value");  // always required and supplied
    targetvar = value_script->GetOrCreateVar("Target");
    actorvar  = value_script->GetOrCreateVar("Actor");

    my_script = prg_script;

    const char *varname = node->GetAttributeValue("save");
    if (varname)
    {
        if (!prg_script->FindVariable(varname))
        {
            MathScriptVar *pv = new MathScriptVar;
            pv->name = varname;
            pv->SetValue(0);
            prg_script->AddVariable(pv);
        }
        result_var_name=varname;
    }
    return true;
};

bool ProgressionOperation::LoadDelay(iDocumentNode *node, ProgressionEvent *prg_script)
{
    // check if value is valid
    if (node->GetAttributeValue("delay") != NULL)
        delay_text = node->GetAttributeValue("delay");
    else
        delay_text = "0";

    csString script("Delay = ");
    script.Append(delay_text);

    delay_script = new MathScript(prg_script->name.GetData(),script);

    delayvar = delay_script->GetVar("Delay");  // always required and supplied
    delaytargetvar = delay_script->GetOrCreateVar("Target");
    delayactorvar  = delay_script->GetOrCreateVar("Actor");
    return true;
};

float ProgressionOperation::GetValue(gemActor *actor, gemObject *target)
{
    if (!value_script)
    {
        Error2("Invalid value script in Progression Event '%s'.",this->eventName->GetData() );
        return 0.0;
    }

    targetvar->SetObject(target ? target->GetCharacterData() : NULL);
    actorvar->SetObject(actor ? actor->GetCharacterData() : NULL );

    value_script->Execute();

    if (result_var_name.Length())
    {
        MathScriptVar *pv = my_script->FindVariable(result_var_name);
        pv->SetValue(valuevar->GetValue());
    }

    return valuevar->GetValue();
}

int ProgressionOperation::GetDelay(gemActor *actor, gemObject *target)
{
    if (!delay_script)
    {
        Error2("Invalid delay script in Progression Event '%s'.",this->eventName->GetData() );
        return 0;
    }
    delaytargetvar->SetObject(target ? target->GetCharacterData() : NULL);
    delayactorvar->SetObject(actor ? actor->GetCharacterData() : NULL);

    delay_script->Execute();

    return (int) delayvar->GetValue() - ticksElapsed;
}


class FireEventOp : public ProgressionOperation
{
protected:
    csString event;

public:
    FireEventOp() : ProgressionOperation() {}
    virtual ~FireEventOp() {}

    bool Load(iDocumentNode* node, ProgressionEvent* script)
    {
        event = node->GetAttributeValue( "event" );
        return LoadValue(node, script);
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<fire_event name='%s' />", event.GetData() );
        return xml;
    }

    bool Run(gemActor *actor, gemObject* target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        // Cannot be run if there is no actor
        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) FireEventOp needs an actor\n",eventName->GetData());
            return true;
        }

        psCharacter *data = actor->GetCharacterData();
        data->FireEvent(event);
        return true;
    }
};

/*-------------------------------------------------------------*/

/** TraitChangeOp
  * Used to create a change in character traits.
  * This is used to change a character's appearance live in game.
  * It takes the trait ID number and sends a broadcast out to all
  * the players in range about the change.
  *
  * Syntax:
  *     <trait value="#"/>
  *         value = "#" index id key into traits table
  * Examples:
  *    You apply trait 100 (grey hair) to actor and send message:
  *        <trait value="100" /><msg aim="actor" text="You drop the liquid on your hair."/>
  *
  * On the Run it checks to make sure the selected trait is allowed
  * for that race and rejects with an error message if it is not.
  */
class TraitChangeOp : public ProgressionOperation
{
protected:
    int traitID;

public:
    TraitChangeOp() : ProgressionOperation() {}
    virtual ~TraitChangeOp() {}

    bool Load(iDocumentNode* node, ProgressionEvent* script)
    {
        traitID = node->GetAttributeValueAsInt( "value" );
        return LoadValue(node, script);
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<trait value=\"%d\" />", traitID );
        return xml;
    }

    bool Run(gemActor *actor, gemObject* target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        // Cannot be run if there is no actor
        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) TraitChangeOp needs an actor\n",eventName->GetData());
            return true;
        }

        int clientID = actor->GetClientID();
        psCharacter *data = actor->GetCharacterData();
        psRaceInfo* raceInfo = data->GetRaceInfo();

        psTrait* trait = CacheManager::GetSingleton().GetTraitByID( traitID );

        // Validate that the selected trait can be applied to the player's race.
        if ( trait->raceID != raceInfo->uid )
        {
            psserver->SendSystemInfo(clientID,"Doesn't work on you, try a better brand.");
            return false;
        }

        data->SetTraitForLocation( trait->location, trait );

        // Send out updated information to all clients on prox. list
        csString str( "<traits>" );
        str.Append(trait->ToXML() );
        str.Append("</traits>");
        psTraitChangeMessage message((uint32_t) clientID, actor->GetEID(), str);
        message.Multicast( actor->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE );
        return true;
    }
};

/*-------------------------------------------------------------*/

/**
 * ExperienceOp
 * Adjust the experience of the target.
 *
 * Syntax:
 *    <exp type="allocate_last" value="#"/>
 *        allocate_type = "allocate_last" means allocate damage last otherwise damage is allocated
 *        value = "#" ammount of experience to give single player or share with others
 *
 */
class ExperienceOp : public ProgressionOperation
{
protected:
    csString type;

public:
    ExperienceOp() : ProgressionOperation() { };
    virtual ~ExperienceOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        type = node->GetAttributeValue("type");
        return LoadValue(node, script);
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<exp type=\"%s\" value=\"%s\" />",type.GetData(),script_text.GetData() );
        return xml;
    }


    float AllocateKillDamage(gemObject *target, int exp)
    {
        // Convert to gemActor
        gemActor* targetAct = target->GetActorPtr();
        if(!targetAct)
            return 0.0f;

        csArray<gemActor*> attackers;
        unsigned int timeOfDeath = csGetTicks(); // TODO: Should be recorded on death
                                                 //       in the targetAct.

        // Last timestamp, used for breaking the loop when > 10 secs had gone
        unsigned int lastTimestamp = 0;
        float        totalDamage   = 0; // The denominator for the percentages

        // First build list of attackers and determine how far to go back and what total dmg was.
        int i;
        for (i=(int)targetAct->GetDamageHistoryCount(); i>0; i--)
        {
            DamageHistory* history = targetAct->GetDamageHistory(i-1);

            // 15 secs passed
            if(lastTimestamp - history->timestamp > 15000 && lastTimestamp != 0)
            {
                Debug1(LOG_COMBAT, 0,"15 secs passed between hits, breaking loop\n");
                break;
            }
            lastTimestamp = history->timestamp;

            // Special check for DoT adjustments if the target died before the DoT expired
            if(history->damageRate != 0)
            {
                csTicks duration = -(int)(history->damage/history->damageRate);
                if (duration > timeOfDeath - history->timestamp)
                {
                    // Since damageRate should always be negative:
                    history->damage = -(history->damageRate * (timeOfDeath - history->timestamp));
                }
            }

            totalDamage += history->damage;

            bool found = false;

            if (!history->attacker_ref.IsValid())
                continue;  // This attacker has disconnected since he did this damage.

            // Have we already added that player?
            for(size_t x=0;x < attackers.GetSize();x++)
            {
                if(attackers[x] == history->attacker_ref)
                {
                    found = true;
                    break;
                }
            }

            // New player, add to list
            if(!found)
            {
                attackers.Push(dynamic_cast<gemActor*>((gemObject *) history->attacker_ref)); // This is ok because it is ONLY used in this function
            }
        }
        int lastHistory = i;

        for(size_t i=0;i < attackers.GetSize();i++)
        {
            gemActor* attacker = attackers[i];
            if(!attacker)
                continue;  // should not happen with new safe ref system.

            float dmgMade = 0;
            float mod = 0;

            for (int x = (int)targetAct->GetDamageHistoryCount();x > lastHistory; x--)
            {
                const DamageHistory* history = targetAct->GetDamageHistory(x-1);
                if(history->attacker_ref == attacker)
                {
                    dmgMade += history->damage;
                }
            }

            if (!totalDamage)
            {
                Error2("%s was found to have zero totalDamage in damagehistory!",targetAct->GetName() );
                continue;
            }
            // Use the latest HP (needs to be redesigned when NPCs can cast heal spells on eachoter)
            mod = dmgMade / totalDamage; // Get a 0.something value or 1 if we did all dmg
            if (mod > 1.0)
                mod = 1.0;

            int final = int(exp * mod);

            psserver->SendSystemInfo(attacker->GetClientID(),"You gained %d experience points.",final);
            if (int pp = attacker->GetCharacterData()->AddExperiencePoints(final))
            {
                psserver->SendSystemInfo(attacker->GetClientID(),"You gained %d progression points.",pp);
            }
        }

        targetAct->ClearDamageHistory();

        return 0.0f;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        int exp = (int) GetValue(actor, target);

        // allocate_dmg used to be in the if statement, but it should rightfully be the default
        // since we almost always want it and it is almost never in the data :)-- KWF
        if (type == "allocate_last")
        {
            if ( actor )
            {
                psserver->SendSystemInfo(actor->GetClientID(),"You gained %d experience points.",exp);
                actor->GetCharacterData()->AddExperiencePoints(exp);
            }
        }
        else
        {
            result = AllocateKillDamage(target, exp);
        }
        return true;
    }
};

/*-------------------------------------------------------------*/

/**
 * FactionOp
 * Adjust the faction of the actor relative to the target.
 *
 * Syntax:
 *    <faction [aim="target"] name="%s" value="#" />
 *        aim = "target" adjust actor relative to target instead of actor
 *        name = "%s" faction name
 *        value = "#" amount to adjust faction
 */
class FactionOp : public ProgressionOperation
{
protected:
    Faction *faction;
    // True if its the actors stat that should be used.
    bool aimIsActor;

public:
    FactionOp() : ProgressionOperation() { };
    virtual ~FactionOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        if (node->GetAttributeValue("aim"))
        {
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        }
        else
            aimIsActor = true; // Default

        faction = CacheManager::GetSingleton().GetFaction(node->GetAttributeValue("name"));
        if (!faction)
        {
            Error2("Error: FactionOp faction(%s) not found\n",node->GetAttributeValue("name"));
            return false;
        }

        return LoadValue(node, script);
    }

    virtual csString ToString()
    {
        psString xml;
        xml.Format("<faction ");
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");
        csString escpxml = EscpXML(faction->name);
        xml.AppendFmt("name=\"%s\" value=\"%s\" />",
                   escpxml.GetData(), script_text.GetData());
        return xml;
    }


    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (!aimIsActor && !target)
        {
            Error2("Error: ProgressionEvent(%s)  FactionOp need a target\n",eventName->GetData());
            return true;
        }

        if (aimIsActor && !actor)
        {
            Error2("Error: ProgressionEvent(%s)  FactionOp need an actor\n",eventName->GetData());
            return true;
        }

        psCharacter * character;

        if (aimIsActor)
            character = actor->GetCharacterData();
        else
            character = target->GetCharacterData();

        if (!character)
        {
            Error2("Error: ProgressionEvent(%s)  FactionOp aim isn't a character\n",eventName->GetData());
            return true;
        }

        int delta = (int) GetValue(actor, target);

        if ( inverse )
            delta = -delta;

        character->UpdateFaction(faction, delta);

        return true;
    }
};

/*-------------------------------------------------------------*/

/**
 * StatsOp
 * Adjust the specified stat of the target.
 *
 * Syntax:
 *    <hp|mana|pstamina|mstamina|str|agi|end|int|wil|cha|con|sta|attack|defense|hprate|mrate|pstamrate|mstamrate
 *      [aim="target"] [adjust="add"] adjust="set|mul|pct" base="yes" delay="#" undomsg="%s" value="#"/>
 *        hp = hit points, mana = mana points, pstamina = physical stamina, mstamina = mental stamina,
 *          str = strength, agi = agility, end = endurance, int = intelligence, wil = will, cha = charisma,
 *          con = constitution, sta = stanima, attack = attack modifier, defense = defense modifier,
 *          hprate = hit point recovery rate, mrate = mana recovery rate, pstamrate = physical stamina recovery rate,
 *          mstamrate = mental stamina recovery rate.
 *        aim = "target" adjust target instead of default actor
 *        adjust = "add" add given value to this attribute (default)
 *        adjust = "set" set this attribute to the given value
 *        adjust = "mul" mutliply attibute by given value
 *        adjust = "pct" adjust by a percentage of the base stats (unlike mul which uses overall stats)
 *        base = "yes" apply to base value (change to max value)
 *        delay = "#" ammount of time before attibute returns to normal
 *        undomsg = "%s" message player gets when attibute returns to normal
 *        value = "#" the ammount used to adjust attribute
 * Example:
 *    The effect of energy arrow to reduce target 5 HP:
 *        <hp aim="target" value="-5"/><msg text="You deal 5HP damage."/>
 *    Taking the path of a street warrior adds 35% of character points to strength:
 *        <str adjust="add" value="0.35*CharPoints" />
 *    Effect of darkness spell on actor is to reduce attack modifier by mutiplying by 1 minus 2% of powerlevel
 *        <attack adjust="mul" aim="target" value="1-0.02*PowerLevel" delay="10000*PowerLevel"
 *        undomsg="Your vision clears as the globe of darkness fades." />
 */
class StatsOp : public ProgressionOperation
{
public:

    typedef enum {HP,MANA,PSTAMINA,MSTAMINA,STR,AGI,END,INT,WIL,CHA,CON,STA,MSTA,ATTACK,DEFENSE,HPRATE,MRATE,PSTAMRATE,MSTAMRATE} Stat_t;
    typedef enum {adjust_set, adjust_add, adjust_mul, adjust_pct} adjust_t;

    static const char * statToString[];
    static const PSITEMSTATS_STAT statToAttrib[];

    StatsOp(Stat_t state):ProgressionOperation() {stat=state;};
    virtual ~StatsOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        adjust = adjust_add;
        if (node->GetAttributeValue("adjust"))
        {
            csString adjustStr = node->GetAttributeValue("adjust");
            if (adjustStr == "set")
                adjust = adjust_set;
            else if (adjustStr == "mul")
                adjust = adjust_mul;
            else if (adjustStr == "pct")
                adjust = adjust_pct;
        }

        if (node->GetAttributeValue("aim"))
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        else
            aimIsActor = true; // Default

        base = (node->GetAttributeValue("base") && !strcmp(node->GetAttributeValue("base"), "yes"));

        undoMsg = node->GetAttributeValue("undomsg");

        return LoadValue(node, script) && LoadDelay(node, script);
    }

    virtual csString ToString()
    {
        psString xml;
        psString adjustStr, delayStr;

        xml.Format("<%s ",statToString[stat]);
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");

        switch (adjust)
        {
            case adjust_set: adjustStr = "set"; break;
            case adjust_add: adjustStr = "add"; break;
            case adjust_mul: adjustStr = "mul"; break;
            case adjust_pct: adjustStr = "pct"; break;
        }

        xml.AppendFmt("adjust=\"%s\" ", adjustStr.GetData());
        if (base)
            xml.AppendFmt("base=\"yes\" ");
        xml.AppendFmt("delay=\"%s\" ", delay_text.GetData());
        if (undoMsg.GetData() != NULL)
            xml.AppendFmt("undomsg=\"%s\" ", undoMsg.GetData());
        xml.AppendFmt("value=\"%s\" />",script_text.GetData() );

        return xml;
    }

    float GetCurrentValue(psCharacter * targetChar, bool findBase = false)
    {
        switch (stat)
        {
        case HP:
            if (base)
                return targetChar->GetHitPointsMax();
            else
                return targetChar->GetHP();
            break;
        case MANA:
            if (base)
                return targetChar->GetManaMax();
            else
                return targetChar->GetMana();
           break;
        case PSTAMINA:
            if (base)
                return targetChar->GetStaminaMax(true);
            else
                return targetChar->GetStamina(true);
            break;
        case MSTAMINA:
            if (base)
                return targetChar->GetStaminaMax(false);
            else
                return targetChar->GetStamina(false);
            break;
        case STR:
        case AGI:
        case END:
        case INT:
        case WIL:
        case CHA:
        case CON:
        case STA:
            if (base || findBase)
                return targetChar->Stats().GetStat(statToAttrib[stat], false);
            else
                return targetChar->Stats().GetStat(statToAttrib[stat], true);
            break;
        case ATTACK:
            return targetChar->GetAttackValueModifier();
            break;
        case DEFENSE:
            return targetChar->GetDefenseValueModifier();
            break;
        case HPRATE:
            return targetChar->AdjustHitPointsRate(0.0);
            break;
        case MRATE:
            return targetChar->AdjustManaRate(0.0);
            break;
        case PSTAMRATE:
            return targetChar->AdjustStaminaRate(0.0, true);
            break;
        case MSTAMRATE:
            return targetChar->AdjustStaminaRate(0.0, false);
            break;
        default:
            return 0.0;
        }
        return 0.0;
    }

    float CalcNewValue(float oldValue, float adjustValue, bool inverse, float baseValue)
    {
        switch (adjust)
        {
            case adjust_set: return adjustValue;
            case adjust_add: if (inverse) return oldValue - adjustValue; else return oldValue + adjustValue;
            case adjust_mul: if (inverse) return oldValue / adjustValue; else return oldValue * adjustValue;
            case adjust_pct:
            {
                float pct = baseValue * adjustValue/100.0f;
                if ( inverse )
                    return oldValue-pct;
                else
                    return oldValue+pct;
            }
        }

        return 0.0;
    }

    bool SetValue(gemActor * actor, psCharacter * targetChar, float oldValue, float newValue, int duration)
    {
        switch (stat)
        {
            case HP:
            {
                if (base)
                {
                    targetChar->AdjustHitPointsMaxModifier(newValue - oldValue);
                }
                else
                {
                    if (newValue < oldValue)
                    {
                        targetChar->GetActor()->DoDamage(actor, targetChar->GetHP()- newValue);
                    }
                    else
                    {
                        if (oldValue == targetChar->GetHitPointsMax())
                        {
                            if (!strcmp(actor->GetName(), targetChar->GetActor()->GetName()))
                            {
                                psserver->SendSystemInfo(actor->GetClient()->GetClientNum(),
                                                     "Your attempt doesn't have any effect since you don't have any wounds.");
                            }
                            else
                            {
                                psserver->SendSystemInfo(actor->GetClient()->GetClientNum(),
                                                     "Your attempt doesn't have any effect since %s doesn't have any wounds.",
                                                     targetChar->GetActor()->GetName());
                            }
                            //return false;
                        }
                        else
                        {
                            targetChar->SetHitPoints(newValue);
                        }
                    }
                }
                break;
            }

            case MANA:
            {
                if (base)
                {
                    targetChar->AdjustManaMaxModifier(newValue - oldValue);
                }
                else
                {
                    targetChar->SetMana(newValue);
                }
                break;
            }

            case PSTAMINA:
            {
                if (base)
                {
                    targetChar->AdjustStaminaMaxModifier(newValue - oldValue, true);
                }
                else
                {
                    targetChar->SetStamina(newValue, true);
                }
                break;
            }

            case MSTAMINA:
            {
                if (base)
                    targetChar->AdjustStaminaMaxModifier(newValue - oldValue, false);
                else
                    targetChar->SetStamina(newValue, false);
                break;
            }

            case STR:
            case AGI:
            case END:
            case INT:
            case WIL:
            case CHA:
            case CON:
            case STA:
            {
                if (base)
                {
                    targetChar->Stats().SetStat(statToAttrib[stat], (unsigned) newValue);
                    targetChar->CalculateEquipmentModifiers();
                }
                else
                {
                    targetChar->Stats().BuffStat(statToAttrib[stat], unsigned(newValue-oldValue));
                    targetChar->CalculateEquipmentModifiers();
                }
                break;
            }

            case ATTACK:
            {
                targetChar->AdjustAttackValueModifier(newValue/oldValue);
                break;
            }

            case DEFENSE:
            {
                targetChar->AdjustDefenseValueModifier(newValue/oldValue);
                break;
            }

            case HPRATE:
            {
                targetChar->GetActor()->DoDamage(actor, 0.0, newValue-oldValue, duration);
                break;
            }

            case MRATE:
            {
                targetChar->AdjustManaRate(newValue);
                break;
            }

            case PSTAMRATE:
            {
                targetChar->AdjustStaminaRate(newValue, true);
                break;
            }

            case MSTAMRATE:
            {
                targetChar->AdjustStaminaRate(newValue, false);
                break;
            }

            default:
            {
                break;
            }
        }

        // This function recalculates the target HP, Mana and Stamina
        targetChar->RecalculateStats();

        Client *client = actor ? actor->GetClient() : NULL;
        if (client && client->GetCharacterData())
            psserver->GetProgressionManager()->SendSkillList( client, false );

        return true;
    }

    csString CreateUndoScript(float oldValue, float finalValue)
    {
        csString script;

        script.Format("<evt><%s adjust=\"add\" aim=\"%s\" base=\"%s\" value=\"%f\" />",
                      statToString[stat], aimIsActor ? "actor" : "target", base ? "yes" : "no", oldValue - finalValue);
        if (undoMsg.Length() > 0)
            script.AppendFmt("<msg aim=\"%s\" text=\"%s\"/>", aimIsActor ? "actor" : "target", undoMsg.GetData());
        script += "</evt>";

        return script;
    }

    csString Absolute()
    {
        if(undoQueued)
            return "";
        csString script;
        script.Format("<%s adjust=\"add\" aim=\"%s\" base=\"%s\" value=\"%f\" undomsg=\"%s\" />",
                      statToString[stat], aimIsActor ? "actor" : "target", base ? "yes" : "no", newValue - oldValue, undoMsg.GetData());
        return script;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        psCharacter * targetChar;
        gemActor * object;
        // return 0.0f; // Disabled to stop crashes when target is an invalid pointer
        object = dynamic_cast <gemActor*> (aimIsActor ? actor : target);
        if (!object)
        {
            Error2("Error: ProgressionEvent(%s)  StatsOp need a target\n",eventName->GetData());
            return true;
        }

        targetChar = object->GetCharacterData();
        if (!targetChar)
        {
            Error3("Error: ProgressionEvent(%s)  StatsOp aim %s isn't a character\n",eventName->GetData(),object->GetName() );
            return true;
        }

        Client* client = NULL;
        if ( actor )
        {
           client = psserver->GetConnections()->Find( actor->GetClientID() );
        }

        if ( client )
        {
            object->SendTargetStatDR( client );
        }

        int delay = GetDelay(actor, target);
        if (delay < 0)
        {
            return false; // No time left for the effect to apply, so don't bother.
        }

        oldValue = GetCurrentValue(targetChar);
        float adjustValue = GetValue(actor, target);
        float baseValue = GetCurrentValue( targetChar, true );
        newValue = CalcNewValue(oldValue, adjustValue, inverse, baseValue);

        if ( !SetValue(actor, targetChar, oldValue, newValue, delay) )
        {
            Notify2(LOG_SCRIPT,"Error: ProgressionEvent(%s): StatsOp SetValue not possible.\n",eventName->GetData());
            return false;
        }

        if(inverse && undoMsg.Length() > 0)
        {
            // print the undo message
            psString sendtext(undoMsg);
            SubstituteVars(actor, target, sendtext);
            if(object->GetClientID())
                psserver->SendSystemInfo(object->GetClientID(),sendtext);
        }



        if ( client )
        {
           object->SendTargetStatDR( client );
        }


        if (delay > 0)
        {
            float finalValue = GetCurrentValue(targetChar);
            csString undoScript = CreateUndoScript(oldValue, finalValue);

            int persistentID = targetChar->RegisterProgressionEvent(ToString(), ticksElapsed);
            psserver->GetProgressionManager()->QueueUndoScript(undoScript.GetData(), delay, actor, object, item, persistentID);
            undoQueued = true;
        }
        return true;
    }

protected:
    float oldValue;
    float newValue;

    adjust_t adjust;

    bool base; ///< True if it is the base value that should be used.

    bool aimIsActor; ///< True if its the actors stat that should be used.

    csString undoMsg;

    Stat_t stat;
};

const char *StatsOp::statToString[] = {"hp","mana","pstamina","mstamina","str","agi","end","int","wil","cha","con","sta","msta", "attack", "defense", "hpRate", "mRate", "pStamRate", "mStamRate"};
const PSITEMSTATS_STAT StatsOp::statToAttrib[] =
    {PSITEMSTATS_STAT_NONE,             ///< hp
     PSITEMSTATS_STAT_NONE,             ///< mana
     PSITEMSTATS_STAT_NONE,             ///< stamina
     PSITEMSTATS_STAT_NONE,             ///< stamina
     PSITEMSTATS_STAT_STRENGTH,
     PSITEMSTATS_STAT_AGILITY,
     PSITEMSTATS_STAT_ENDURANCE,
     PSITEMSTATS_STAT_INTELLIGENCE,
     PSITEMSTATS_STAT_WILL,
     PSITEMSTATS_STAT_CHARISMA,
     PSITEMSTATS_STAT_CONSTITUTION,
     PSITEMSTATS_STAT_STAMINA,
     PSITEMSTATS_STAT_NONE,             ///< attack modifier
     PSITEMSTATS_STAT_NONE,             ///< defense modifier
     PSITEMSTATS_STAT_NONE,             ///< hp rate
     PSITEMSTATS_STAT_NONE,             ///< mrate
     PSITEMSTATS_STAT_NONE,             ///< pstam rate
     PSITEMSTATS_STAT_NONE              ///< mstam rate
     };

/*-------------------------------------------------------------*/

/**
 * SkillOp
 * Adjust the specified skill level of the target.
 *
 * Syntax:
 *    <skill name="%s" buff="no" [aim="target"] adjust="set" value="#" />
 *        name = "%s" skill name
 *        buff = "no" no buff
 *        aim = "target" adjust target instead of default actor
 *        adjust = "set" set skill value as opposed to adjust
 *        value = "#" value to set skill
 * Examples:
 *    Increase skill "Red Way" by 2 for 120 seconds and give message when potion wears off
 *        <skill name="Red Way" value="2" delay="120000" aim="actor" attribute="adjust" base="no"
 *          undomsg="The potion wears off." />
 *    An item increases the herbal skill by 3 with no buf:
 *        <skill name="herbal" buff="no" attribute="adjust" value="3"/>
 */
class SkillOp : public ProgressionOperation
{
public:

    SkillOp() : ProgressionOperation() { };
    virtual ~SkillOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        if (node->GetAttributeValue("aim"))
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        else
            aimIsActor = true; // Default

        if (!node->GetAttributeValue("name"))
        {
            Error1("Error: No name for skill\n");
            return false;
        }

        csString name = node->GetAttributeValue("name");
        skill = CacheManager::GetSingleton().ConvertSkillString(name);

        isBuff = !(node->GetAttributeValue("buff") && !strcmp(node->GetAttributeValue("buff"),"no"));
        setValue = node->GetAttributeValue("adjust") && !strcmp(node->GetAttributeValue("adjust"),"set");

        return LoadValue(node, script) && LoadDelay(node, script);
    }

    bool CreateUndoScript( float adjustValue, psString& undoScript )
    {
        psSkillInfo* info = CacheManager::GetSingleton().GetSkillByID(skill);
        if ( info )
        {
            undoScript = "<evt>";
            csString escname = EscpXML( info->name );
            undoScript.AppendFmt("<skill name=\"%s\" ", escname.GetData() );
            if (!isBuff)
                undoScript.Append("buff=\"no\" ");
            if (!aimIsActor)
                undoScript.Append("aim=\"target\" ");
            if (setValue)
                undoScript.AppendFmt("adjust=\"set\" value=\"%u\" />", oldSkillRank);
            else
                undoScript.AppendFmt("value=\"%f\" />", -adjustValue);
            undoScript.Append("</evt>");
            return true;
        }
        Error2("Error: ProgressionEvent(%s) SkillOp  no info for skill\n",eventName->GetData());
        return false;
    }


    virtual csString ToString()
    {
        psString xml;
        psSkillInfo * info = CacheManager::GetSingleton().GetSkillByID(skill);

        if (info)
        {
            csString escpxml = EscpXML(info->name);
            xml.Format("<skill name=\"%s\" ", escpxml.GetData());
            if (!isBuff)
                xml.AppendFmt("buff=\"no\" ");
            if (!aimIsActor)
                xml.AppendFmt("aim=\"target\" ");
            if (setValue)
                xml.AppendFmt("adjust=\"set\" ");
            xml.AppendFmt("value=\"%s\" />",script_text.GetData() );
        }
        else
        {
            xml.Format ("<skill name=\"dummy\" />");
        }

        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (setValue && isBuff)
        {
            Error2("Error: ProgressionEvent(%s) SkillOp  set and buff are incompatible\n",eventName->GetData());
            return true;
        }
        if (setValue && inverse)
        {
            Error2("Error: ProgressionEvent(%s) SkillOp  set and inverse are incompatible\n",eventName->GetData());
            return true;
        }
        if (aimIsActor && !actor)
        {
            Error2("Error: ProgressionEvent(%s) SkillOp  need an actor\n",eventName->GetData());
            return true;
        }
        if (!aimIsActor && !target)
        {
            Error2("Error: ProgressionEvent(%s) SkillOp  need a target\n",eventName->GetData());
            return true;
        }

        psCharacter * character;

        if (aimIsActor)
            character = actor->GetCharacterData();
        else
            character = target->GetCharacterData();

        if (!character)
        {
            Error2("Error: ProgressionEvent(%s)  SkillOp target isn't a character\n",eventName->GetData());
            return true;
        }

        int adjustValue = (int) GetValue(actor, target);
        if (inverse)
            adjustValue = -adjustValue;

        if (isBuff)
        {
            character->Skills().BuffSkillRank(skill, adjustValue);
            result = adjustValue;
        }
        else
        {
            oldSkillRank = character->Skills().GetSkillRank(skill, false);
            result = adjustValue + (int) (setValue ? 0 : oldSkillRank);
            character->Skills().SetSkillRank(skill, (int) result);
        }

        int delay = GetDelay(actor, target);
        if (delay < 0)
            return false; // No time left for the effect to apply, so don't bother.

        if (delay > 0)
        {
            psString undoScript;
            if (CreateUndoScript(adjustValue, undoScript))
            {
                undoQueued = true;
                gemActor *object = dynamic_cast <gemActor*> (aimIsActor ? actor : target);
                int persistentID = character->RegisterProgressionEvent(ToString(), ticksElapsed);
                psserver->GetProgressionManager()->QueueUndoScript(undoScript.GetData(), delay, actor, object, item, persistentID);
            }
        }

        return true;
    }
protected:
    unsigned int oldSkillRank;

    /// True if the return from the expression shall be used to
    /// set the skill value. Otherwise it will just adjust it.
    bool setValue;
    /// True if its the actors is the aim that should be used.
    bool aimIsActor;

    bool isBuff; ///< True if setting a buff, rather than a permanent skill rank change
    PSSKILL skill;
};

/*-------------------------------------------------------------*/

/**
 * MsgOp
 * Send a message to the target OR the actor.
 *
 * Syntax:
 *    <msg [aim="target"] text="%s" />
 *        aim = "target" send message to target instead of default actor
 *        text = "%s" text to send
 * Examples:
 *    Send message to actor and target about spell effects:
 *          <msg aim="actor" text="You fire sharp ice blades from your fingers, hurting your opponents."/>
 *          <msg aim="target" text="You are hit by sharp ice blades."/></area>
*/
class MsgOp : public ProgressionOperation
{
public:
    MsgOp() : ProgressionOperation() { };
    virtual ~MsgOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        my_script = script;

        if (node->GetAttributeValue("aim"))
        {
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        }
        else
            aimIsActor = true; // Default

        text = node->GetAttributeValue("text");
        return true;
    }

    virtual csString ToString()
    {
        psString xml;
        xml.Format("<msg ");
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");
    csString escpxml = EscpXML(text);
        xml.AppendFmt("text=\"%s\" />",escpxml.GetData());
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        if (!aimIsActor && !target)
        {
            Error2("Error: ProgressionEvent(%s) MsgOp need a target\n",eventName->GetData());
            return true;
        }

        if (aimIsActor && !actor)
        {
            Error2("Error: ProgressionEvent(%s)  MsgOp need an actor\n",eventName->GetData());
            return true;
        }

        int clientID;
        if (aimIsActor)
            clientID = actor->GetClientID();
        else
            clientID = target->GetClientID();


        if (!clientID)
        {
            // Spell on NPC instead of PC--not really an error in the script.
            // CPrintf(CON_ERROR, "Error: ProgressionEvent(%s)  MsgOp aim isn't connected to a client.",
            //        eventName->GetData());
            return true;
        }

        psString sendtext(text);
        SubstituteVars(actor, target, sendtext);

        psserver->SendSystemInfo(clientID,sendtext);


        return true;
    }
protected:

    csString text;
    bool aimIsActor; ///< True if its the actors is the aim that should be used.
};

/*-------------------------------------------------------------*/

/**
 * BlockOp
 * Block a category of spells from being recast again for a
 * specified time delay.
 *
 * Syntax:
 *    <block operation="BLOCK_ADD" category="%s" delay="#" />
 *        operation = "BLOCK_ADD" add the blocking otherwise remove it
 *        category = "%s" catagory of spell to block
 *        delay = "#" amount of time to block spells
 * Examples:
 *    Blocks the cast of weakness protection spell for 2 plus 5 times PowerLevel seconds
 *        <block category="-Weakness" delay="2000+5000*PowerLevel" />
 *    Blocks the cast of darkness protection for 10 times PowerLeven seconds
 *        <block category="-Darkness" delay="10000*PowerLevel" />
 *    Blocks taking another Red potion for 120 seconds
 *        <block category="PotionRed" delay="120000" />
 */
class BlockOp : public ProgressionOperation
{
public:
    typedef enum {BLOCK_ADD, BLOCK_REMOVE} Operation_t;

    BlockOp() : ProgressionOperation() { };
    virtual ~BlockOp() {};

    static const char * operationToString[];

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        my_script = script;
        if ( node->GetAttribute("operation") )
        {
            csString operationStr = node->GetAttributeValue("operation");
            if ( operationStr.CompareNoCase(operationToString[BLOCK_ADD]) )
            {
                operation = BLOCK_ADD;
            }
            else
            {
                operation = BLOCK_REMOVE;
            }
        }
        else
        {
            operation = BLOCK_ADD;
        }
        category = node->GetAttributeValue("category");
        return LoadDelay(node, script);
    }

    virtual csString ToString()
    {
        psString xml;
        csString escpxml = EscpXML(operationToString[operation]);
        xml.Format( "<block " );
        xml.AppendFmt( "operation=\"%s\" ", escpxml.GetData() );
        escpxml = EscpXML(category);
        xml.AppendFmt( "category=\"%s\" ", escpxml.GetData() );
        xml.AppendFmt( "delay=\"%s\" ", delay_text.GetData() );
        xml.AppendFmt( " /> " );
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if ( inverse )
        {
            if ( operation == BLOCK_ADD )
            {
                operation = BLOCK_REMOVE;
            }
            else if ( operation == BLOCK_REMOVE )
            {
                operation = BLOCK_ADD;
            }
        }

        gemActor *targetActor;

        if ( !target )
        {
            if(!actor)
            {
                Error2("Error: ProgressionEvent(%s) BlockOp needs a target\n",eventName->GetData());
                return false;
            }
            // Target is the actor if no target is set.
            target = actor;
            targetActor = actor;
        }
        else
        {
            targetActor = target->GetActorPtr();
        }

        switch ( operation )
        {
            case BLOCK_ADD:
            {

                if (targetActor->AddActiveMagicCategory(category))
                {
                    int delay = GetDelay(actor, target);

                    if ( delay > 0 )
                    {
                        int persistentID = targetActor->GetCharacterData() ? targetActor->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed) : 0;
                        psString undoscript = CreateUndoScript();
                        undoQueued = true;
                        psserver->GetProgressionManager()->QueueUndoScript(undoscript.GetData(), delay, actor, target, item, persistentID);
                    }
                }
                else
                    // Spell is blocked so the rest of the script must not execute.
                    return false;
                break;
            }

            case BLOCK_REMOVE:
            {
                targetActor->RemoveActiveMagicCategory(category);
                break;
            }
        }

        return true;
    }

protected:

    psString CreateUndoScript( void )
    {
        psString script;

        script = "<evt>";
        script.Append("<block ");
        csString escpxml = EscpXML(operationToString[BLOCK_REMOVE]);
        script.AppendFmt( "operation=\"%s\" ", escpxml.GetData() );
        escpxml = EscpXML(category);
        script.AppendFmt( "category=\"%s\" ", escpxml.GetData() );
        script.AppendFmt( "delay=\"%s\" ", delay_text.GetData() );
        script.Append(" /> ");
        script.Append("</evt>");

        return script;
    }

    Operation_t operation;
    csString category;
};
const char *BlockOp::operationToString[] = {"add","remove"};

/*-------------------------------------------------------------*/

/**
 * AttachScriptOp
 * Attach scripts reacting on gemActor events to a gemActor
 *
 * Syntax:
 *    <attachscript [aim="target"] delay="#" scriptName="%s" event="attack" undomsg="%s" />
 *        aim = "target" attach to target instead of default actor
 *        delay = "#" ammount of time to have script attached
 *        scriptName = "%s" name of progression script to attach
 *        event = "attack" attach attack scripts otherwise they are damage scripts
 *        undomsg = "%s" message to send when attachment is finished
 * Examples:
 *    Cast a Flame Spire spell by running defensive damage script "apply Flame Spire" for 2 times PowerLevel seconds
 *        <attachscript aim="actor" delay="2000+PowerLevel" scriptName="apply Flame Spire" event="defense"
 *         undomsg="The flame spire disappears."/>
 */
class AttachScriptOp : public ProgressionOperation
{
public:
    AttachScriptOp() : ProgressionOperation() { };
    virtual ~AttachScriptOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        my_script = script;

        if (node->GetAttributeValue("aim"))
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        else
            aimIsActor = true; // Default

        scriptName = node->GetAttributeValue("scriptName");
        event = node->GetAttributeValue("event");
        undoMsg = node->GetAttributeValue("undomsg");
        return LoadDelay(node, script);
    }

    virtual csString ToString()
    {
        psString xml;
        xml.Format("<attachscript ");
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");
        xml.AppendFmt("delay=\"%s\" ", delay_text.GetData());
        xml.AppendFmt("scriptName=\"%s\" ", scriptName.GetData());
        xml.AppendFmt("event=\"%s\" ", event.GetData());
        xml.AppendFmt("undomsg=\"%s\" ", undoMsg.GetData());
        xml += "/>";
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        gemActor * object;
        int scriptID;

        if (aimIsActor)
            object = dynamic_cast<gemActor*> (actor);
        else
            object = dynamic_cast<gemActor*> (target);

        if (!object)
        {
            Error2("Error: ProgressionEvent(%s) DetachScriptOp need a target\n",eventName->GetData());
            return true;
        }

        if (event == "attack")
            scriptID = object->AttachAttackScript(scriptName);
        else
            scriptID = object->AttachDamageScript(scriptName);

        int delay = GetDelay(actor, target);
        if (delay < 0)
            return false; // No time left for the effect to apply, so don't bother.

        if (delay > 0)
        {
            psString undoScript;
            undoScript.AppendFmt("<evt><detachscript aim=\"target\" event=\"%s\" scriptID=\"%i\"/> ", event.GetData(), scriptID);
            if (undoMsg.Length() > 0)
                undoScript.AppendFmt("<msg aim=\"target\" text=\"%s\"/>", undoMsg.GetData());
            undoScript += "</evt>";

            int persistentID = object->GetCharacterData() ? object->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed) : 0;
            psserver->GetProgressionManager()->QueueUndoScript(undoScript.GetData(), delay, actor, object, item, persistentID);
        }

        return true;
    }
protected:

    bool aimIsActor;
    csString event;
    csString scriptName;
    csString undoMsg;
};

/*-------------------------------------------------------------*/

/**
 * DetachScriptOp
 * Detach scripts reacting on gemActor events from a gemActor
 *
 * Syntax:
 *    <detachscript [aim="target"] scriptID="#" event="attack" />
 *        aim = "target" detach from target rather then default actor
 *        scriptID = "#" the index number of the script to detach
 *        event = "attack" detach attack scripts rather then damage scripts
 */
class DetachScriptOp : public ProgressionOperation
{
public:
    DetachScriptOp() : ProgressionOperation() { };
    virtual ~DetachScriptOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        my_script = script;

        if (node->GetAttributeValue("aim"))
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        else
            aimIsActor = true; // Default

        scriptID = node->GetAttributeValueAsInt("scriptID");
        event = node->GetAttributeValue("event");
        return true;
    }

    virtual csString ToString()
    {
        psString xml;
        xml.Format("<detachscript ");
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");
        xml.AppendFmt("scriptID=\"%i\" ", scriptID);
        xml.AppendFmt("event=\"%s\" ", event.GetData());
        xml += "/>";
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        gemActor * object;

        if (aimIsActor)
            object = dynamic_cast<gemActor*> (actor);
        else
            object = dynamic_cast<gemActor*> (target);

        if (!object)
        {
            Error2("Error: ProgressionEvent(%s) DetachScriptOp need a target\n",eventName->GetData());
            return false;
        }

        if (event == "attack")
            object->DetachAttackScript(scriptID);
        else
            object->DetachDamageScript(scriptID);

        return true;
    }
protected:

    bool aimIsActor;
    csString event;
    int scriptID;
};

/*-------------------------------------------------------------*/

/**
 * IdentifyMagicOp
 * Determine if 'target' is magical item and tell 'actor' the outcome
 *
 * Syntax:
 *    <identifymagic/>
 */
class IdentifyMagicOp : public ProgressionOperation
{
public:
    IdentifyMagicOp() : ProgressionOperation() { };
    virtual ~IdentifyMagicOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        return true;
    }

    virtual csString ToString()
    {
        psString xml;
        xml.Format("<identifymagic");
        xml += "/>";
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        gemItem * gItem;
        int clientnum;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) IdentifyMagicOp needs an actor\n",eventName->GetData());
            return true;
        }

        clientnum = actor->GetClientID();
        if (clientnum == 0)
        {
            Error2("Error: ProgressionEvent(%s) IdentifyMagicOp needs a client\n",eventName->GetData());
            return true;
        }

        gItem = dynamic_cast <gemItem*> (target);
        if (!gItem)
        {
            psserver->SendSystemError(clientnum,"You must have an item selected");
            return true;
        }

        psItemStats * stats = gItem->GetItem()->GetBaseStats();
        // this is really bad.. should have specific flag?
        if (stats->GetProgressionEventEquip().Length() > 0 || stats->GetProgressionEventUnEquip().Length() > 0)
        {
            psserver->SendSystemInfo(clientnum,"You found magical properties in this item !");
        }
        else
        {
            psserver->SendSystemInfo(clientnum,"This is an ordinary item without any magical powers.");
        }

        return true;
    }
};

/*-------------------------------------------------------------*/

/**
 * ScriptOp
 * Queue another named script to run after a specifed delay.
 *
 * Syntax:
 *    <script delay="#" persistent="yes" >%s</script>
 *        delay = "#" amount of time to delay before running script
 *        persistant = "yes" will create a persistant event verses a temporary one
 *        %s = the script to que up to run
 * Examples:
 *    Queue an event to give actor 5 more HP and messages after 3 seconds:
 *        <script delay="3000"><hp value="5"><msg text="You feel fresh"/></script>
 */
class ScriptOp : public ProgressionOperation
{
public:
    ScriptOp() : ProgressionOperation() { };
    virtual ~ScriptOp(){};

    bool Load(iDocumentNode *node, ProgressionEvent *prg_script)
    {
        delay = node->GetAttributeValueAsInt("delay");
        if (node->GetAttributeValue("persistent"))
        {
            persistent = !strcmp(node->GetAttributeValue("persistent"),"yes");
        }
        else persistent = false;

        script.name = *eventName;
        script.LoadScript(node);
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        csString script_str = script.ToString(false);
        xml.Format("<script delay=\"%d\" %s/>%s</script>",
                   delay,(persistent?"persistent=\"yes\" ":""),
                   script_str.GetData());
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        int persistentID = 0;
        if (persistent && target && target->GetCharacterData())
        {
            persistentID = target->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed);
        }

        psserver->GetProgressionManager()->QueueEvent(new psScriptGameEvent(delay, &script, actor, target, item, persistentID));

        return true;
    }

protected:
    int delay;
    bool persistent;
    ProgressionEvent script;
};


/*-------------------------------------------------------------*/

/**
 * AreaOp
 * Applies a script to everything of a type in an area.
 *
 * Syntax:
 *    <area type="entity|item|actor|group|hostile|friendly" range="#" anglerange="#"
 *      delaybetween="#" includetarget="yes|no" > %s </area>
 *        type = "entity" apply to everything
 *        type = "item" apply to items only
 *        type = "actor" apply to actor
 *        type = "group" apply to actor's group
 *        type = "hostile" apply to all that actor can attack
 *        type = "friendly" apply to all the actor can not attack
 *        range = "#" range in m of area
 *        anglerange = "#" angle of effect in degrees
 *        delaybetween = "#" delay in ms default to all at once
 *        includetarget = "no" include target or not
 *        %s script to apply to everything of a type in area
 * Examples:
 *    Cast Icy Blast on all all mobs within 12 by applying script to reduce HP by 2 plus PowerLevel for Powerlevel seconds
 *        <area type="hostile" range="12">
 *          <hp adjust="add" aim="target" value="-1*(2+PowerLevel)" delay="1000*PowerLevel"/>
 *          <msg aim="actor" text="You fire sharp ice blades from your fingers, hurting your opponents."/>
 *          <msg aim="target" text="You are hit by sharp ice blades."/></area>
 *
 */
class AreaOp : public ProgressionOperation
{
public:
    AreaOp() : ProgressionOperation() { };
    virtual ~AreaOp(){};

    enum target_type { ENTITY, ITEM, ACTOR, GROUP, HOSTILE, FRIENDLY };

    bool Load(iDocumentNode *node, ProgressionEvent *prg_script)
    {
        type = node->GetAttributeValue("type");
        range = node->GetAttributeValueAsInt("range");
        anglerange = node->GetAttributeValueAsInt("anglerange");
        delaybetween = node->GetAttributeValueAsInt("delaybetween");
        includetarget = node->GetAttributeValueAsBool("includetarget",true);

        if (type.IsEmpty())
        {
            Error2("ProgressionEvent(%s) AreaOp must specify an entity type\n",eventName->GetData() );
            return true;
        }
        else if (type == "entity")
            typecode = ENTITY;
        else if (type == "item")
            typecode = ITEM;
        else if (type == "actor")
            typecode = ACTOR;
        else if (type == "group")
            typecode = GROUP;
        else if (type == "hostile")
            typecode = HOSTILE;
        else if (type == "friendly")
            typecode = FRIENDLY;
        else
        {
            Error3("Invalid type in ProgressionEvent(%s) AreaOp: %s\n",eventName->GetData(), type.GetData() );
            return true;
        }

        if (range < 1 || range > 100)
        {
            Error2("Range in ProgressionEvent(%s) AreaOp must be at least 1 and less than 100\n",eventName->GetData() );
            return true;
        }

        if (anglerange < 0 || anglerange >= 360)
        {
            Error2("Angle range in ProgressionEvent(%s) AreaOp must be between 0 and 360\n",eventName->GetData() );
            return true;
        }

        if (delaybetween > 5000)
        {
            Error2("Delay between applies in ProgressionEvent(%s) AreaOp must be between 0 and 5000ms\n",eventName->GetData() );
            return true;
        }

        script.name = *eventName;
        script.LoadScript(node);
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        csString script_str = script.ToString(false);
        xml.Format("<area ");
        xml.AppendFmt("type=\"%s\" ", type.GetData() );
        xml.AppendFmt("range=\"%d\" ", range );
        if (anglerange) xml.AppendFmt("anglerange=\"%d\" ", anglerange );
        if (delaybetween) xml.AppendFmt("delaybetween=\"%u\" ", delaybetween );
        if (!includetarget) xml.AppendFmt("includetarget=\"no\" ");
        xml.Append(">");
        xml.Append(script_str);
        xml.Append("</area>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (inverse)  // No inverse
            return true;

        if (!target)
        {
            // If we don't have a target, use the actor
            target = actor;

            if (!target)
            {
                Error2("Error: ProgressionEvent(%s) AreaOp needs a target or actor\n",eventName->GetData());
                return true;
            }
        }

        Client* client = NULL;
        if (anglerange || typecode > ACTOR)  /// anglerange, GROUP, HOSTILE, and FRIENDLY need info about the actor
        {
            if (!actor)
            {
                Error2("Error: ProgressionEvent(%s) AreaOp needs an actor\n",eventName->GetData());
                return true;
            }

            if (typecode > GROUP)  /// HOSTILE and FRIENDLY need info about the actor's client
            {
                client = actor->GetClient();
                if (!client)
                {
                    Error2("Error: ProgressionEvent(%s) AreaOp actor needs a client\n",eventName->GetData());
                    return true;
                }
            }
        }

        csVector3 actor_pos;    ///< Actor's position
        csVector3 target_pos;   ///< Target's position
        iSector* actor_sector;  ///< Actor's sector
        iSector* target_sector; ///< Target's sector

#define NORMALIZE_BIG_ANGLE(a)  { if (a > TWO_PI) a -= TWO_PI; }
#define NORMALIZE_NEG_ANGLE(a)  { if (a < 0.0f) a += TWO_PI; }

        float max_angle = 0.0f;
        float min_angle = 0.0f;
        if (anglerange)
        {
            float actor_angle;
            actor->GetPosition(actor_pos,actor_angle,actor_sector);

            // angle is actually in the opposite direction the character is facing...
            actor_angle += PI;
            NORMALIZE_BIG_ANGLE(actor_angle);

            /// We get 1/2 of the given range to either side of the actor's facing angle.
            float half_range = float(anglerange) * ((PI/180.0f) * 0.5f);  // In radians
            max_angle = actor_angle + half_range;
            min_angle = actor_angle - half_range;
            NORMALIZE_BIG_ANGLE(max_angle);
            NORMALIZE_NEG_ANGLE(min_angle);
        }

        if (anglerange && target == actor)
        {
            // Just copy the values if we already have them
            target_pos = actor_pos;
            target_sector = actor_sector;
        }
        else
        {
            target->GetPosition(target_pos,target_sector);
        }

        GEMSupervisor* gem = GEMSupervisor::GetSingletonPtr();
        psWorld* world = EntityManager::GetSingleton().GetWorld();

        csArray<gemObject*> nearlist = gem->FindNearbyEntities(target_sector,target_pos,range);
        size_t count = nearlist.GetSize();
        for (size_t i=0, n=0; i<count; i++)
        {
            gemObject* nearobj = nearlist[i];

            if (!includetarget && nearobj == target)
                continue;

            switch (typecode)
            {
                case ENTITY:
                    break;  // Everything

                case ITEM:
                    if ( nearobj->GetItem() )
                        break; else continue;

                case ACTOR:
                    if (nearobj->GetPID().IsValid())
                        break; else continue;

                case GROUP:
                    if ( actor->IsGroupedWith(nearobj->GetActorPtr()) )
                        break; else continue;

                case HOSTILE:
                    if ( client->IsAllowedToAttack(nearobj,false) )
                        break; else continue;

                case FRIENDLY:
                    if ( !client->IsAllowedToAttack(nearobj,false) )
                        break; else continue;
            }

            if (anglerange && nearobj != target)
            {
                nearobj->GetPosition(target_pos,target_sector);
                world->WarpSpace(target_sector,actor_sector,target_pos);

                float dx = target_pos.x - actor_pos.x;
                float dz = target_pos.z - actor_pos.z;
                float angle = atan2f(dx,dz);
                NORMALIZE_NEG_ANGLE(angle);

                if (max_angle > min_angle)
                {
                    if (angle > max_angle || angle < min_angle)
                        continue;
                }
                else  // Wedge includes 0
                {
                    if (angle > max_angle && angle < min_angle)
                        continue;
                }
            }

            // Queue an event for each intended object
            psserver->GetProgressionManager()->QueueEvent( new psScriptGameEvent(delaybetween*n++, &script, actor, nearobj, item, true) );
        }

        return true;
    }

protected:
    int range;                ///< Range of area application, in meters
    int anglerange;           ///< Wedge of area to apply to, in degrees
    csString type;            ///< Type of entity to apply to
    target_type typecode;     ///< Code for type, from enum
    csTicks delaybetween;     ///< Delay between each apply, in miliseconds
    bool includetarget;       ///< Apply to target, or just area around?
    ProgressionEvent script;  ///< Script to apply to each
};

/*-------------------------------------------------------------*/

/**
 * ChargeOp
 * Applies a script if charges left.
 *
 * Syntax:
 *    <charge charges="#" > %s </charge>
 *        charges = "#" number of charges to use
 *        %s script to apply if charged
 * Examples:
 *    Ring of summon should only work once, item has 1 charge
 *        <charge charges="1"><createpet masterids="10000"/></charge>
 *
 */
class ChargeOp : public ProgressionOperation
{
public:
    ChargeOp() : ProgressionOperation() { };
    virtual ~ChargeOp(){};

    bool Load(iDocumentNode *node, ProgressionEvent *prg_script)
    {
        charges = node->GetAttributeValueAsInt("charges");

        script.name = *eventName;
        script.LoadScript(node);

        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        csString script_str = script.ToString(false);
        xml.Format("<charge ");
        xml.AppendFmt("charges=\"%d\" ", charges );
        xml.Append(">");
        xml.Append(script_str);
        xml.Append("</charges>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (inverse)  // No inverse
            return true;

        if (item && item->HasCharges() && item->GetCharges() >= charges)
        {
            item->SetCharges(item->GetCharges()-charges);
            item->Save(false);

            script.Run(actor, target, item, inverse);
        }

        return true;
    }

protected:
    int charges;              ///< Number of charges needed to perform this operation.
    ProgressionEvent script;  ///< Script to apply if charged
};


/*-------------------------------------------------------------*/

/**
 * RechargeOp
 * Change number of charges in an item.
 *
 * Syntax:
 *    <recharge charges="#" success="text" failure="text" />
 *        charges = "#" number of charges to recharge
 *        success = "text" Display this text if success
 *        failure = "text" Display this text if failure
 * Examples:
 *        <recharge charges="1" />
 *
 */
class RechargeOp : public ProgressionOperation
{
public:
    RechargeOp() : ProgressionOperation() { };
    virtual ~RechargeOp(){};

    bool Load(iDocumentNode *node, ProgressionEvent *prg_script)
    {
        charges = node->GetAttributeValueAsInt("charges");
        successText = node->GetAttributeValue("success");
        failureText = node->GetAttributeValue("failure");
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<recharge ");
        xml.AppendFmt("charges=\"%d\" ", charges );
        if (successText.Length())
        {
            xml.AppendFmt("success=\"%s\" ", successText.GetDataSafe() );
        }
        if (failureText.Length())
        {
            xml.AppendFmt("failure=\"%s\" ", failureText.GetDataSafe() );
        }
        xml.Append("/>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (inverse)  // No inverse
            return true;

        if (item && item->HasCharges() && item->IsRechargeable())
        {
            int new_charges = item->GetCharges() + charges;

            // Item should not be charged more than max charges.
            if (new_charges > item->GetMaxCharges())
            {
                new_charges = item->GetMaxCharges();
            }

            item->SetCharges(new_charges);
            item->Save(false);

            if (successText.Length())
            {
                psserver->SendSystemInfo(actor->GetClientID(), successText.GetDataSafe());
            }
        }
        else
        {
            if (failureText.Length())
            {
                psserver->SendSystemInfo(actor->GetClientID(), failureText.GetDataSafe());
            }
        }

        return true;
    }

protected:
    int charges;              ///< Number of charges to apply
    csString successText;
    csString failureText;
};


/*-------------------------------------------------------------*/

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
class ItemOp : public ProgressionOperation
{
public:
    ItemOp() : ProgressionOperation() { };
    virtual ~ItemOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        if (node->GetAttributeValue("aim"))
        {
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        }
        else
            aimIsActor = true; // Default

        name = node->GetAttributeValue("name");
        location = node->GetAttributeValue("location");
        stackCount = node->GetAttributeValueAsInt("count");
        if (!location.IsEmpty() && location != "inventory" &&
            location != "wallet" && location != "ground" )
        {
            Error3("Error:ProgressionEvent(%s) ItemOp Location %s not legal\n",eventName->GetData(), location.GetData());
        }
        return true;
    }

    virtual csString ToString()
    {
        psString xml;
        csString escpxml = EscpXML(name);
        xml.Format("<item name=\"%s\" ",escpxml.GetData());
        if (!location.IsEmpty())
        {
            escpxml = EscpXML(location);
            xml.AppendFmt("location=\"%s\" ",escpxml.GetData());
        }
        if (!aimIsActor)
            xml.AppendFmt("aim=\"target\" ");
        if (stackCount != 0)
            xml.AppendFmt("count=\"%d\" ",stackCount);
        xml.AppendFmt("/>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        if (!aimIsActor && !target)
        {
            Error2("Error: ProgressionEvent(%s) ItemOp need a target\n",eventName->GetData());
            return true;
        }

        if (aimIsActor && !actor)
        {
            Error2("Error: ProgressionEvent(%s)  ItemOp need an actor\n",
                    eventName->GetData());
            return true;
        }

        psCharacter * character;

        if (aimIsActor)
            character = actor->GetCharacterData();
        else
            character = target->GetCharacterData();

        if (!character)
        {
            Error3("Error: ProgressionEvent(%s) ItemOp No character data was found for target %s.\n",
                    eventName->GetData(),(aimIsActor?actor->GetName():target->GetName()));
            return true;
        }

        //This is for a player that is given some money
        if ( location == "wallet" )
        {
            psMoney money;

            if ( name == "trias" )
                money.SetTrias( stackCount );
            if ( name == "hexas" )
                money.SetHexas( stackCount );
            if ( name == "octas" )
                money.SetOctas( stackCount );
            if ( name == "circles" )
                money.SetCircles( stackCount );

            psMoney charMoney = character->Money();
            charMoney = charMoney + money;
            character->SetMoney( charMoney );
            return true;
        }
        else if (location == "inventory")
        {
            psItem * iteminstance = CreateItem(false);
            if (!iteminstance)
            {
                return true;
            }

            character->Inventory().AddOrDrop(iteminstance, false);
        }
        else if (location == "ground")
        {
            psItem * iteminstance = CreateItem(true);
            if (!iteminstance)
            {
                return true;
            }

            character->DropItem(iteminstance);
        }
        return true;
    }

    psItem *CreateItem(bool transient)
    {

        // Get the ItemStats based on the name provided.
        psItemStats *itemstats=CacheManager::GetSingleton().GetBasicItemStatsByName(name.GetData());
        if (!itemstats)
        {
            Error3("Error: ProgressionEvent(%s) ItemOp No Basic Item Template with name %s was found.\n",
                    eventName->GetData(),name.GetData());
            return NULL;
        }

        psItem *iteminstance = itemstats->InstantiateBasicItem(transient);
        if (iteminstance==NULL)
        {
            Error2("Error: ProgressionEvent(%s) ItemOp Could not instanciate item based on basic properties.\n",
                    eventName->GetData());
            return NULL;
        }

        if (stackCount != 0)
        {
            if (!iteminstance->GetIsStackable())
            {
                Error2("Error: ProgressionEvent(%s) ItemOp Item isn't statckable.\n",eventName->GetData());
            }
            else
            {
                iteminstance->SetStackCount(stackCount);
            }
        }

        iteminstance->SetLoaded();  // Item is fully created

        return iteminstance;
    }

protected:
    csString name;
    csString location;
    int stackCount;
    bool aimIsActor; ///< True if its the actors is the aim that should be used.
};

/*-------------------------------------------------------------*/

/**
 * PurifyOp
 * Purify a glyph
 *
 * Syntax:
 *    <purify glyph="#" />
 *        glyph = "#" glyph ID
 */
class PurifyOp : public ProgressionOperation
{
public:
    PurifyOp() : ProgressionOperation() { };
    virtual ~PurifyOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        glyphUID = node->GetAttributeValueAsInt("glyph");
        return true;
    }

    virtual csString ToString()
    {
        csString xml ;
        xml.Format("<purify glyph=\"%u\" />",glyphUID);
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) PurifyOp need an actor\n",eventName->GetData());
            return true;
        }

        psCharacter *character = actor->GetCharacterData();

        if (!character)
        {
            Error2("Error: ProgressionEvent(%s) PurifyOp need a character\n",eventName->GetData());
            return true;
        }

        psserver->GetSpellManager()->EndPurifying(character,glyphUID);

        return true;
    }
protected:
    uint32 glyphUID;
};

/*-------------------------------------------------------------*/

/**
 * IntroduceOp
 * Introduce 2 players
 *
 * Syntax:
 *    <introduce/>
 */
class IntroduceOp : public ProgressionOperation
{
public:
    IntroduceOp() : ProgressionOperation() { };
    virtual ~IntroduceOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        return true;
    }

    virtual csString ToString()
    {
        csString xml ;
        xml.Format("<introduce/>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (!actor || !target)
            return false;

        psCharacter* actorChar = actor->GetCharacterData();
        psCharacter* targetChar = target->GetCharacterData();

        if (inverse)
        {
            actorChar->Unintroduce(targetChar);
            return true;
        }

        actorChar->Introduce(targetChar);

        target->Send(actor->GetClientID(), false, actor->GetClient()->IsSuperClient());

        return true;
    }
};

/*-------------------------------------------------------------*/

/**
 * MorphOp
 * Morph an actor's mesh
 *
 * Syntax:
 *    <morph mesh="%s" duration="#" />
 *        mesh = "%s" is the new mesh for actor
 *        duration is amount of time before making the change
 */
class MorphOp : public ProgressionOperation
{
public:
    MorphOp() : ProgressionOperation() { };
    virtual ~MorphOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        mesh = node->GetAttributeValue("mesh");
        duration = node->GetAttributeValueAsInt("duration");
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<morph mesh=\"%s\" duration=\"%d\" />", mesh.GetData(), duration );
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) MorphOp need an actor\n",eventName->GetData());
            return true;
        }

        if (inverse || mesh == "reset")
            actor->ResetMesh();
        else
            actor->SetMesh(mesh);

        if (duration != 0)
        {
            int persistentID = actor->GetCharacterData() ? actor->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed) : 0;
            // Queue undo script
            psserver->GetProgressionManager()->QueueUndoScript("<evt><morph mesh=\"reset\" /></evt>", duration*1000, actor, actor, item, persistentID);
        }

        return true;
    }

protected:
    csString mesh;  ///< Mesh to morph into
    int duration;   ///< Duration of effect in seconds
};

/*-------------------------------------------------------------*/

/**
 * CraftOp
 * Craft an item into another item.
 *
 * Syntax:
 *    <craft pattern="%s" />
 *        pattern = "%s" pattern name
 * Examples:
 *    Perform magical transmutation crafting on item:
 *          <craft pattern="Transmutation" />
 *
 * Function sets result = 1 if craft is started
 */
class CraftOp : public ProgressionOperation
{
public:
    CraftOp() : ProgressionOperation() { };
    virtual ~CraftOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        pattern = node->GetAttributeValue("pattern");
        return true;
    }

    virtual csString ToString()
    {
        psString xml;
        csString escpxml = EscpXML(pattern);
        xml.Format("<craft pattern=\"%s\" />",escpxml.GetData());
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        result = 0;
        if (!actor->GetClient())
        {
            Error3("Error: ProgressionEvent(%s) ItemOp No client data was found for actor %s.\n",
                    eventName->GetData(),actor->GetName());
            return true;
        }

        // Do script crafting
        if (psserver->GetWorkManager()->StartScriptWork(actor->GetClient(),target,pattern))
            result = 1;
        return true;
    }


protected:
    csString pattern; ///< Craft pattern name
};

/*-------------------------------------------------------------*/

/**
 * QuestOp
 * The quest operator only has one function at the moment
 * The complete function checks for quest completion
 *
 * Syntax:
 *    <quest funct="complete" [aim="target"] prerequisite="%s" />
 *        funct = "complete" checks if quest has been completed
 *        aim = "target" will check target otherwise actor is checked
 *        prerequisite = "%s" quest name to check target has completed
 * Example:
 *    Check if actor has completed the Rescue the Princess quest and give message:
 *        <quest funct="complete" aim="actor" prerequisite="Rescue the Princess" />
 *        <msg aim="actor" text="You are trying to access restricted area."/>
 *
 * For complete function result = 1 if quest is completed otherwise 0
 */
enum QuestOpFunctions
{
    QUESTOPFUNCTIONS_UNKNOWN=0,         /// Unknown/undefined function
    QUESTOPFUNCTIONS_COMPLETE           /// Check for complete quest
};

class QuestOp : public ProgressionOperation
{
public:
    QuestOp() : ProgressionOperation() { };
    virtual ~QuestOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        aimIsActor = true; // Default
        csString functStr = node->GetAttributeValue("funct");
        if (!functStr.GetData())
        {
            Error2("Error:ProgressionEvent(%s) QuestOp Funct not present\n",eventName->GetData());
            return true;
        }
        if (!strcasecmp(functStr.GetData(),"complete"))
        {
            function = QUESTOPFUNCTIONS_COMPLETE;
            if (node->GetAttributeValue("aim"))
            {
                aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
            }
            questName = node->GetAttributeValue("prerequisite");
        }
        else
        {
            function = QUESTOPFUNCTIONS_UNKNOWN;
            Error3("Error:ProgressionEvent(%s) QuestOp Funct %s not legal\n",eventName->GetData(), functStr.GetData());
        }
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        switch (function)
        {
            case QUESTOPFUNCTIONS_COMPLETE:
            {
                xml.Format("<quest funct=\"complete\" ");
                if (!aimIsActor)
                    xml.AppendFmt("aim=\"target\" ");
                xml.AppendFmt("prerequisite=\"%s\" />", questName.GetData() );
            }

            default:
                break;
        }
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // There is no inverse to this operation
        if (inverse)
            return true;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) QuestOp need an actor\n",eventName->GetData());
            return true;
        }

        switch (function)
        {
            case QUESTOPFUNCTIONS_COMPLETE:
            {
                // Check quest assignments and return non zero if quest has been compeleted
                result = 0;
                psQuest *quest = CacheManager::GetSingleton().GetQuestByName(questName);
                if (!quest)
                {
                    Error3("Error: ProgressionEvent(%s) QuestOp can not find quest %s\n",eventName->GetData(), questName.GetData());
                    if (actor->GetClient()->GetSecurityLevel() > GM_LEVEL_0)
                    {
                        psserver->SendSystemError(actor->GetClientID(),"Error: ProgressionEvent(%s) QuestOp can not find quest %s\n",eventName->GetData(), questName.GetData());
                    }
                    return true;
                }
                if (aimIsActor)
                {
                    if (actor->GetCharacterData()->CheckQuestCompleted(quest))
                    {
                        result = 1;
                    }
                }
                else
                {
                    if (target->GetCharacterData()->CheckQuestCompleted(quest))
                    {
                        result = 1;
                    }
                }
            }
            default:
                break;
        }
        return true;
    }

protected:
    int function;               ///< Operation function
    bool aimIsActor;            ///< True if its the actors stat that should be used.
    csString questName;         ///< Quest to check for completeness
};

/*-------------------------------------------------------------*/

/**
 * TeleportOp
 * The teleport operator only has one function at the moment
 * The spawn function sends actor back to race spawn location.
 *
 * Syntax:
 *    <teleport funct="spawn" />
 *        funct = "spawn" sends actor to spawn location
 * Example:
 *    Send actor to spawn location and give message:
 *        <teleport funct="spawn" />
 *        <msg aim="actor" text="Good luck."/>
 *
 */
enum TeleportOpFunctions
{
    TELEPORTOPFUNCTIONS_UNKNOWN=0,         ///< Unknown/undefined function
    TELEPORTOPFUNCTIONS_SPAWN              ///< Send actor to spawn location
};

class TeleportOp : public ProgressionOperation
{
public:
    TeleportOp() : ProgressionOperation() { };
    virtual ~TeleportOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        csString functStr = node->GetAttributeValue("funct");
        if (!functStr.GetData())
        {
            Error2("Error:ProgressionEvent(%s) TeleportOp Funct not present\n",eventName->GetData());
            return true;
        }
        if (!strcasecmp(functStr.GetData(),"spawn"))
        {
            function = TELEPORTOPFUNCTIONS_SPAWN;
        }
        else
        {
            function = QUESTOPFUNCTIONS_UNKNOWN;
            Error3("Error:ProgressionEvent(%s) TeleportOp Funct %s not legal\n",eventName->GetData(), functStr.GetData());
        }
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        switch (function)
        {
            case TELEPORTOPFUNCTIONS_SPAWN:
            {
                xml.Format("<teleport funct=\"spawn\" />");
            }
            default:
                break;
        }
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // There is no inverse to this operation
        if (inverse)
            return true;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) TeleportOp need an actor\n",eventName->GetData());
            return true;
        }

        switch (function)
        {
            case TELEPORTOPFUNCTIONS_SPAWN:
            {
                // Get race information
                psRaceInfo *raceinfo = actor->GetCharacterData()->GetRaceInfo();
                if (!raceinfo)
                {
                    Error2("Error: ProgressionEvent(%s) TeleportOp actor has no raceinfo.\n",eventName->GetData());
                    return false;
                }

                // Setup landing coords
                float x,y,z,yrot;
                const char *sectorname;
                raceinfo->GetStartingLocation(x,y,z,yrot,sectorname);

                // Get sector info
                csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
                iSector* sector = engine->GetSectors()->FindByName(sectorname);
                if (!sector)
                {
                    Error2("Error: ProgressionEvent(%s) TeleportOp actor raceinfo has bad sector.\n",eventName->GetData());
                    return false;
                }

                // Do teleport
                actor->StopMoving(true);
                actor->SetInstance(DEFAULT_INSTANCE);
                actor->SetPosition(csVector3(x,y,z),yrot,sector);
                actor->GetCharacterData()->SaveLocationInWorld();
                actor->MulticastDRUpdate();
                actor->UpdateProxList(true);
            }
            default:
                break;
        }
        return true;
    }

protected:
    int function; ///< Operation function
};

/*-------------------------------------------------------------*/

/**
 * ActionOp
 * At this point there is only one function for this script
 * The activate function activates any inactive entrance action
 *  location of the specified entrance type and places into
 *  players inventory a key for the lock instance ID defined
 *  in that action location entrance.
 *
 * Syntax:
 *     <action funct="activate" sector="%s" stat="%s" />
 *         funct="activate" activates action location
 *         sector = "%s" sector string to qualify search for inactive entrances
 *         stat = "%s" name of item type for new key for lock
 * Examples:
 *     This quest script activates the any inactive action location for sector guildlaw and give a "Small Key" item.
 *         <action funct="activate" sector="guildlaw" stat="Small Key" />
 */
enum ActionOpFunctions
{
    ACTIONOPFUNCTIONS_UNKNOWN=0,    /// Unknown/undefined function
    ACTIONOPFUNCTIONS_ACTIVATE      /// Activate an action location
};

class ActionOp : public ProgressionOperation
{
public:
    ActionOp() : ProgressionOperation() { };
    virtual ~ActionOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        csString functStr = node->GetAttributeValue("funct");
        if (!functStr.GetData())
        {
            Error2("Error:ProgressionEvent(%s) ActionOp Funct not present\n",eventName->GetData());
            return true;
        }
        if (!strcasecmp(functStr.GetData(),"activate"))
        {
            function = ACTIONOPFUNCTIONS_ACTIVATE;
            sector = node->GetAttributeValue("sector");
            keyStat = node->GetAttributeValue("stat");
        }
        else
        {
            function = ACTIONOPFUNCTIONS_UNKNOWN;
            Error3("Error:ProgressionEvent(%s) ActionOp Funct %s not legal\n",eventName->GetData(), functStr.GetData());
        }
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        switch (function)
        {
            case ACTIONOPFUNCTIONS_ACTIVATE:
            {
                xml.Format("<action funct=\"activate\" ");
                xml.AppendFmt("sector=\"%s\" ", sector.GetData() );
                xml.AppendFmt("stat=\"%s\" />", keyStat.GetData() );
            }
            default:
                break;
        }
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // There is no inverse to this operation
        if (inverse)
            return true;

        switch (function)
        {
            case ACTIONOPFUNCTIONS_ACTIVATE:
            {
                // Get character data
                psCharacter *character = actor->GetCharacterData();
                if (!character)
                {
                    Error2("Error: ProgressionEvent(%s) ActionOP could not get character data\n",eventName->GetData());
                    return true;
                }

                // Returns the next inactive entrance action location
                psActionLocation* actionLocation = psserver->GetActionManager()->FindAvailableEntrances(sector);
                if (!actionLocation)
                {
                    Error3("Error:ProgressionEvent(%s) ActionOp No available action location entrances found for %s.\n",eventName->GetData(),sector.GetData());
                    return true;
                }

                // Activate the action location
                actionLocation->SetActive(true);
                actionLocation->Save();

                // Get lock ID for this entrance
                uint32 lockID = actionLocation->GetInstanceID();
                if (!lockID)
                {
                    Error3("Error:ProgressionEvent(%s) ActionOp No available action location entrances found for %s.\n",eventName->GetData(),sector.GetData());
                    return true;
                }

                // Get the ItemStats based on the name provided.
                psItemStats *itemstats=CacheManager::GetSingleton().GetBasicItemStatsByName(keyStat.GetData());
                if (!itemstats)
                {
                    Error3("Error: ProgressionEvent(%s) ActionOp No Basic Item Template with name %s was found.\n",
                            eventName->GetData(),keyStat.GetData());
                    return true;
                }

                // Make 1 master key item
                psItem *masterkeyItem = itemstats->InstantiateBasicItem();
                if (!masterkeyItem)
                {
                    Error2("Error: ProgressionEvent(%s) ActionOp Could not instanciate item based on basic properties.\n",
                            eventName->GetData());
                    return true;
                }

                // Assign the lock and load it
                masterkeyItem->SetIsKey(true);
                masterkeyItem->SetIsMasterKey(true);
                masterkeyItem->AddOpenableLock(lockID);
                masterkeyItem->SetMaxItemQuality(50.0);
                masterkeyItem->SetStackCount(1);

                // Give to player
                masterkeyItem->SetLoaded();
                character->Inventory().AddOrDrop(masterkeyItem, false);

                // Make 10 regular key items
                psItem *keyItem = itemstats->InstantiateBasicItem();
                if (!keyItem)
                {
                    Error2("Error: ProgressionEvent(%s) ActionOp Could not instanciate item based on basic properties.\n",
                            eventName->GetData());
                    return true;
                }

                // Assign the lock and load it
                keyItem->SetIsKey(true);
                keyItem->AddOpenableLock(lockID);
                keyItem->SetMaxItemQuality(50.0);
                keyItem->SetStackCount(10);

                // Give to player
                keyItem->SetLoaded();
                character->Inventory().AddOrDrop(keyItem, false);

/*
                // Get client info for sign
                Client* client = actor->GetClient();
                if (!client)
                {
                    Error2("Error: ProgressionEvent(%s) ActionOP could not get client data\n",eventName->GetData());
                    return true;
                }
                // Get character guild name for sign
                psGuildInfo* guild = character->GetGuild();
                if (!guild)
                {
                    Error2("Error: ProgressionEvent(%s) ActionOP could not get guild data\n",eventName->GetData());
                    return true;
                }
                csString guildName = guild->GetName();

                // Send the effect message
                csVector3 newpos;
                float yrot;
                const char* sector = NULL;
                actionLocation->GetLocationInWorld( &sector, newpos.x, newpos.y, newpos.z, yrot );
                uint32 meshID = actionLocation->GetGemObject()->GetEID();

                // Send message to attach effect 1/2 m above action location
                newpos.y += 0.5f;
                psEffectMessage newmsg(0, "chatbubble", newpos,  meshID, 0, guildName, 0);
                if (!newmsg.valid)
                {
                    Error2("Error: ProgressionEvent(%s) EffectOP could not create valid psEffectMessage\n",eventName->GetData());
                    return true;
                }
                newmsg.Multicast(actor->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
*/
                break;
            }
            default:
                break;
        }
        return true;
    }

protected:
    int function;               ///< Operation function
    csString sector;            ///< sector name of action location entrance to activate
    csString keyStat;           ///< Item stat name to use for making new key
};

/*-------------------------------------------------------------*/

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
enum KeyOpFunctions
{
    KEYOPFUNCTIONS_UNKNOWN=0,   /// Unknown/undefined function
    KEYOPFUNCTIONS_MAKE,        /// Make new key
    KEYOPFUNCTIONS_MODIFY       /// Modify existing key
};

class KeyOp : public ProgressionOperation
{
public:
    KeyOp() : ProgressionOperation() { };
    virtual ~KeyOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        csString functStr = node->GetAttributeValue("funct");
        if (!functStr.GetData())
        {
            Error2("Error:ProgressionEvent(%s) KeyOp Funct not present\n",eventName->GetData());
            return true;
        }
        if (!strcasecmp(functStr.GetData(),"make"))
        {
            function = KEYOPFUNCTIONS_MAKE;
            lockID = node->GetAttributeValueAsInt("lockID");
            keyStat = node->GetAttributeValue("stat");
            location = node->GetAttributeValue("location");
            if (!location.IsEmpty() && location != "inventory" && location != "ground" )
            {
                Error3("Error:ProgressionEvent(%s) KeyOp Location %s not legal\n",eventName->GetData(), location.GetData());
            }
        }
        else if (!strcasecmp(functStr.GetData(),"modify"))
        {
            function = KEYOPFUNCTIONS_MODIFY;
            keyID = node->GetAttributeValueAsInt("statID");
            lockID = node->GetAttributeValueAsInt("lockID");
        }
        else
        {
            function = KEYOPFUNCTIONS_UNKNOWN;
            Error3("Error:ProgressionEvent(%s) KeyOp Funct %s not legal\n",eventName->GetData(), functStr.GetData());
        }
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        switch (function)
        {
            case KEYOPFUNCTIONS_MAKE:
            {
                xml.Format("<key funct=\"make\" ");
                xml.AppendFmt("lockID=\"%u\" ", lockID);
                xml.AppendFmt("stat=\"%s\" ", keyStat.GetData());
                xml.AppendFmt("location=\"%s\" />", location.GetData());
                break;
            }
            case KEYOPFUNCTIONS_MODIFY:
            {
                xml.Format("<key funct=\"modify\" ");
                xml.AppendFmt("lockID=\"%u\" ", lockID);
                xml.AppendFmt("keyID=\"%u\" />", keyID);
                break;
            }
            default:
                break;
        }
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // There is no inverse to this operation
        if (inverse)
            return true;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) KeyOp need an actor\n",eventName->GetData());
            return true;
        }

        // Get character data
        psCharacter *character = actor->GetCharacterData();
        if (!character)
        {
            Error2("Error: ProgressionEvent(%s) KeyOP could not get character data\n",eventName->GetData());
            return true;
        }

        switch (function)
        {
            case KEYOPFUNCTIONS_MAKE:
            {
                // Get the ItemStats based on the name provided.
                psItemStats *itemstats=CacheManager::GetSingleton().GetBasicItemStatsByName(keyStat.GetData());
                if (!itemstats)
                {
                    Error3("Error: ProgressionEvent(%s) KeyOp No Basic Item Template with name %s was found.\n",
                            eventName->GetData(),keyStat.GetData());
                    return true;
                }

                psItem *keyItem = itemstats->InstantiateBasicItem();
                if (!keyItem)
                {
                    Error2("Error: ProgressionEvent(%s) KeyOp Could not instanciate item based on basic properties.\n",
                            eventName->GetData());
                    return true;
                }

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
                    character->Inventory().AddOrDrop(keyItem, false);
                }
                else
                {
                    Error2("Error: ProgressionEvent(%s) KeyOp No Lock ID.\n",eventName->GetData());
                }
                break;
            }

            case KEYOPFUNCTIONS_MODIFY:
            {
                // Get key item from bulk and assign the lock
                psItem* keyItem = character->Inventory().FindItemID(keyID);
                if (keyItem)
                {
                    keyItem->AddOpenableLock(lockID);
                    keyItem->Save(false);
                }
                else
                {
                    Error2("KEYOPFUNCTIONS_MODIFY Error, key item id %d is not found!", keyID);
                }
                break;
            }
            default:
                break;
        }
        return true;
    }

protected:
    int function;               ///< Operation function
    csString location;          ///< Location of where to create key
    uint32 lockID;              ///< Instance ID of lock to assign to key
    csString keyStat;           ///< Item stat name to use for making new key
    uint32 keyID;               ///< Key instance ID to check lock
};

/*-------------------------------------------------------------*/

/**
 * EffectOp
 * There is two functions of this script.
 *  The "attached" function will create an effect attached to a aim.
 *  The "unattached" function will create a new unattached effect in front of the aim.
 *
 * Syntax:
 *    <effect funct="attached" [aim="target"] effect="%s" />
 *        funct = "attached" makes a attached effect in front of aim
 *        aim = "target" create effect in front of target rather then default actor
 *        effect = "%s" name of the effect
 *
 *    <effect funct="unattached" [aim="target"] effect="%s" />
 *        funct = "render" makes a unattached effect in front of aim
 *        aim = "target" create effect in front of target rather then default actor
 *        effect = "%s" name of the effect
 * Example:
 *    Crate a "sparks" effect on self:
 *        <effect funct="attached" effect="sparks" />
 */
enum EffectOpFunctions
{
    EFFECTOPFUNCTIONS_UNKNOWN=0,   ///< Unknown/undefined function
    EFFECTOPFUNCTIONS_ATTACHED,    ///< Render an attached effect
    EFFECTOPFUNCTIONS_UNATTACHED   ///< Render an unattached effect
};

class EffectOp : public ProgressionOperation
{
public:
    EffectOp() : ProgressionOperation() { };
    virtual ~EffectOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        csString functStr = node->GetAttributeValue("funct");
        if (!functStr.GetData())
        {
            Error2("Error:ProgressionEvent(%s) EffectOp Funct not present\n",eventName->GetData());
            return true;
        }
        if (!strcasecmp(functStr.GetData(),"attached"))
        {
            function = EFFECTOPFUNCTIONS_ATTACHED;
        }
        else if (!strcasecmp(functStr.GetData(),"unattached"))
        {
            function = EFFECTOPFUNCTIONS_UNATTACHED;
        }
        else
        {
            function = EFFECTOPFUNCTIONS_UNKNOWN;
            Error3("Error:ProgressionEvent(%s) EffectOp Funct %s not legal\n",eventName->GetData(), functStr.GetData());
        }

        if (node->GetAttributeValue("aim"))
        {
            aimIsActor = !strcasecmp(node->GetAttributeValue("aim"),"actor");
        }
        else
        {
            aimIsActor = true; // Default
        }
        effectName = node->GetAttributeValue("effect");
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        switch (function)
        {
            case EFFECTOPFUNCTIONS_ATTACHED:
            {
                xml.Format("<effect funct=\"attached\" ");
                if (!aimIsActor) xml.AppendFmt("aim=\"target\" ");
                xml.AppendFmt("effect=\"%s\" ", effectName.GetData());
                break;
            }
            case EFFECTOPFUNCTIONS_UNATTACHED:
            {
                xml.Format("<effect funct=\"unattached\" ");
                if (!aimIsActor) xml.AppendFmt("aim=\"target\" ");
                xml.AppendFmt("effect=\"%s\" ", effectName.GetData());
                break;
            }
            default:
                break;
        }
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        psCharacter* character;
        gemObject* gem;

        // There is no inverse to this operation
        if (inverse)
        {
            return true;
        }

        // For actor based effects
        if (aimIsActor)
        {
            if (!actor)
            {
                Error2("Error: ProgressionEvent(%s) EffectOp need an actor\n",eventName->GetData());
                return true;
            }
            gem = actor;
            character = actor->GetCharacterData();
        }
        else
        {
            if (!target)
            {
                Error2("Error: ProgressionEvent(%s) EffectOp need a target\n",eventName->GetData());
                return true;
            }
            gem = target;
            character = target->GetCharacterData();
        }

        switch (function)
        {
            case EFFECTOPFUNCTIONS_ATTACHED:
            {
                if (!gem)
                {
                    Error2("Error: ProgressionEvent(%s) EffectOP could not get entity data\n",eventName->GetData());
                    return true;
                }

                // Attach effect to actor or target
                csVector3 offset(0,0,0);
                psEffectMessage newmsg(0, effectName, offset, gem->GetEID(), 0, 0);
                if (!newmsg.valid)
                {
                    Error2("Error: ProgressionEvent(%s) EffectOP could not create valid psEffectMessage\n",eventName->GetData());
                    return true;
                }
                newmsg.Multicast(gem->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
                break;
            }
            case EFFECTOPFUNCTIONS_UNATTACHED:
            {
                if (!character)
                {
                    Error2("Error: ProgressionEvent(%s) EffectOP could not get character data\n",eventName->GetData());
                    return true;
                }

                // Get character current position
                csVector3 pos;
                float yrot;
                psSectorInfo *sectorinfo;
                InstanceID instance;
                character->GetLocationInWorld(instance,sectorinfo,pos.x,pos.y,pos.z,yrot);

                // Put effect in front of actor or target where we drop stuff
                csVector3 newPos;
                newPos.x = pos.x - (DROP_DISTANCE * sinf(yrot));
                newPos.y = pos.y;
                newPos.z = pos.z - (DROP_DISTANCE * cosf(yrot));

                // Send effect message
                psEffectMessage newmsg(0, effectName, newPos, 0, 0, 0);
                if (!newmsg.valid)
                {
                    Error2("Error: ProgressionEvent(%s) EffectOP could not create valid psEffectMessage\n",eventName->GetData());
                    return true;
                }
                newmsg.Multicast(gem->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
                break;
            }
            default:
                break;
        }

        return true;
    }

protected:
    int function;               ///< Operation function
    bool aimIsActor;            ///< True if its the actor should be used.
    csString effectName;        ///< Name of effect
};

/*-------------------------------------------------------------*/

/**
 * AttributeOp
 * Set character game attributes
 *
 * Syntax:
 *    <set attrib="invincible|invisible|nofalldamage|nevertired"
 *     value="true|false" duration="#" />
 *        attrib="invincible" set actor's invincibility in combat
 *        attrib="invisible" set actor's invisibility to visible characters
 *        attrib="nofalldamage" set actor's ability to be damaged from falls
 *        attrib="nevertired" set actors ability to be fatigued
 *        value = "true" makes above attributes true
 *        value = "false" makes above attributes false
 *        duration = "#" is how many seconds before attribute change undone otherwise permanent
 * Examples:
 *    Set actors nofalldamage and never tired attributes for 120 seconds and send message:
 *        <set attrib="nofalldamage" duration="120" />
 *        <set attrib="nevertired" duration="120" />
 *        <msg text="Your legs feel more powerful!"/>
 */
class AttributeOp : public ProgressionOperation
{
public:
    AttributeOp() : ProgressionOperation() { };
    virtual ~AttributeOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        attrib = node->GetAttributeValue("attrib");
        duration = node->GetAttributeValueAsInt("duration");
        value = node->GetAttributeValueAsBool("value",true);

        if (attrib != "invincible" && attrib != "invisible" && attrib != "nofalldamage" &&
            attrib != "nevertired" && attrib != "infintemana" && attrib != "instantcast")
        {
            Error3("Invalid attribute for ProgressionEvent(%s) AttributeOp: %s\n", eventName->GetData(), attrib.GetData() );
            return true;
        }

        if (duration != 0)
        {
            // Create undo script
            undo.Format("<evt><set attrib=\"%s\" value=\"%s\" /></evt>", attrib.GetData(), (!value)?"true":"false" );
        }
        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<set attrib=\"%s\" value=\"%s\" duration=\"%d\" />", attrib.GetData(), (value)?"true":"false", duration );
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) AttributeOp need an actor\n",eventName->GetData());
            return true;
        }

        bool setvalue = false;
        if (attrib == "invincible")
        {
            setvalue = (actor->GetInvincibility() != value);
            if (setvalue)
                actor->SetInvincibility(value);
        }
        else if (attrib == "invisible")
        {
            setvalue = (actor->GetVisibility() != !value);
            if (setvalue)
                actor->SetVisibility(!value);
        }
        else if (attrib == "nofalldamage")
        {
            setvalue = (actor->safefall != value);
            if (setvalue)
                actor->safefall = value;
        }
        else if (attrib == "nevertired")
        {
            setvalue = (actor->nevertired != value);
            if (setvalue)
                actor->nevertired = value;
        }
        else if (attrib == "infinitemana")
        {
            setvalue = (actor->infinitemana != value);
            if (setvalue)
                actor->infinitemana = value;
        }
        else if (attrib == "instantcast")
        {
            setvalue = (actor->instantcast != value);
            if (setvalue)
                actor->instantcast = value;
        }

        if (duration != 0 && setvalue)
        {
            int persistentID = actor->GetCharacterData() ? actor->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed) : 0;
            // Queue undo script
            psserver->GetProgressionManager()->QueueUndoScript(undo, duration*1000, actor, actor, item, persistentID);
        }

        return true;
    }

protected:
    csString undo;    ///< Undo script
    csString attrib;  ///< Attribute we're setting
    bool value;       ///< Value we're setting to (true=on, false=off)
    int duration;     ///< Duration of effect in seconds
};

/*-------------------------------------------------------------*/

/**
 * WeatherOp
 * Change the weather in specified sector
 *
 * Syntax:
 *    <weather type="rain|snow" sector="this|%s" duration="#"
 *     density="#" fade="#" enable="true|false" />
 *        type = "rain" causes rain
 *        type = "snow" causes snow
 *        sector = "this" use actor's sector map name
 *        sector = "%s" sector map name
 *        duration = "#" amount of time in seconds for weather to last
 *        density = "#" density of rain or snow 0 (light) to 6000 (snow) or 8000 (rain)
 *        fade = "#" amount of time in seconds to fade in 0 means no fade
 *        enable = "true" turn on weather
 *        enable = "false" turn off weather
 *    <weather type="lightning" sector="this|%s" duration="#"
 *     density="#" fade="#" enable="true|false" />
 *        type = "lightning" causes lightning
 *        sector = "this" use actor's sector map name
 *        sector = "%s" sector map name
 *        duration = "#" amount of time in seconds for weather to last
 *        fade = "#" amount of time in seconds to fade in 0 means no fade
 *        enable = "true" turn on weather (default)
 *        enable = "false" turn off weather
 *    <weather type="fog" sector="%s" duration="#"
 *     fade="#" r="#" g="#" b="#" enable="true|false" />
 *        type = "fog" causes fog
 *        sector = "this" use actor's sector map name
 *        sector = "%s" sector map name
 *        duration = "#" amount of time in seconds for weather to last
 *        fade = "#" amount of time in seconds to fade in 0 means no fade
 *        r = "#" the red color component of fog
 *        g = "#" the green color component of fog
 *        b = "#" the blue color component of fog
 *        enable = "true" turn on weather
 *        enable = "false" turn off weather
 *    <weather type="auto" sector="this|%s" enable="true|false" />
 *        type = "auto" controls the default weather for sector
 *        sector = "this" use actor's sector map name
 *        sector = "%s" sector map name
 *        enable = "true" turn on weather
 *        enable = "false" turn off weather
 * Examples:
 *    Cause rain effect for this secotr for 60 seconds with 10 second faid with highest density:
 *        <weather type="rain" sector="this" duration="60" fade="10" density="8000" />
 */
class WeatherOp : public ProgressionOperation
{
public:
    WeatherOp() : ProgressionOperation() { };
    virtual ~WeatherOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        type = node->GetAttributeValue("type");
        duration = node->GetAttributeValueAsInt("duration");
        density = node->GetAttributeValueAsInt("density");
        fade = node->GetAttributeValueAsInt("fade");
        r = node->GetAttributeValueAsInt("r");
        g = node->GetAttributeValueAsInt("g");
        b = node->GetAttributeValueAsInt("b");
        enable = node->GetAttributeValueAsBool("enable",true);

        // Convert seconds to miliseconds for WeatherManager
        duration *= 1000;
        fade *= 1000;

        csString sector = node->GetAttributeValue("sector");
        if (sector.IsEmpty())
        {
            Error2("Error: ProgressionEvent(%s) WeatherOp needs a sector name or \"this\"\n",eventName->GetData() );
            return true;
        }
        else if (sector == "this")
        {
            useCurLoc = true;
            sectorinfo = NULL;
        }
        else
        {
            useCurLoc = false;
            sectorinfo = CacheManager::GetSingleton().GetSectorInfoByName(sector);
            if (!sectorinfo)
            {
                Error3("Invalid sector for ProgressionEvent(%s) WeatherOp: %s\n",eventName->GetData(), sector.GetData() );
                return true;
            }
        }

        if (!enable && (duration || density || fade || r || g || b))
        {
            Error2("ProgressionEvent(%s) WeatherOp may only have \"type\" and \"sector\" with enable=\"false\"\n",eventName->GetData() );
            return true;
        }

        if (type == "fog")
            wtype = psWeatherMessage::FOG;
        else if (type == "rain")
            wtype = psWeatherMessage::RAIN;
        else if (type == "snow")
            wtype = psWeatherMessage::SNOW;
        else if (type == "lightning")
            wtype = psWeatherMessage::LIGHTNING;
        else if (type == "auto")
            wtype = 0;
        else
            wtype = -1;

        // Check parameters
        switch (wtype)
        {
            case psWeatherMessage::FOG:
            {
                break;
            }

            case psWeatherMessage::RAIN:
            {
                if (density < 0 || density > WEATHER_MAX_RAIN_DROPS)
                {
                    Error3("ProgressionEvent(%s) WeatherOp: Rain drop density must be between 0 and %d\n",
                           eventName->GetData(), WEATHER_MAX_RAIN_DROPS );
                    return true;
                }
                else break;
            }

            case psWeatherMessage::SNOW:
            {
                if (density < 0 || density > WEATHER_MAX_SNOW_FALKES)
                {
                    Error3("ProgressionEvent(%s) WeatherOp: Snow flake density must be between 0 and %d\n",
                            eventName->GetData(), WEATHER_MAX_SNOW_FALKES );
                    return true;
                }
                else break;
            }

            case psWeatherMessage::LIGHTNING:
            {
                if (sectorinfo && sectorinfo->lightning_max_gap == 0)
                {
                    Error3("Lightning undefined for sector in ProgressionEvent(%s) WeatherOp: %s\n",
                           eventName->GetData(), sector.GetData() );
                    return true;
                }
                else break;
            }

            case 0:  // "auto"
            {
                if (duration || density || fade || r || g || b)
                {
                    Error2("ProgressionEvent(%s) WeatherOp may only have \"sector\" and \"enable\" with type=\"auto\"\n",
                           eventName->GetData() );
                    return true;
                }
                else break;
            }

            default:
            {
                Error3("Invalid type for ProgressionEvent(%s) WeatherOp: %s\n",
                       eventName->GetData(), type.GetData() );
                return true;
            }
        }

        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<weather type=\"%s\" ", type.GetData() );
        xml.AppendFmt("sector=\"%s\" ", useCurLoc ? "this" : sectorinfo->name.GetData() );
        if (duration) xml.AppendFmt("duration=\"%d\" ", duration );
        if (density) xml.AppendFmt("density=\"%d\" ", density );
        if (fade) xml.AppendFmt("fade=\"%d\" ", fade );
        if (r) xml.AppendFmt("r=\"%d\" ", r );
        if (g) xml.AppendFmt("g=\"%d\" ", g );
        if (b) xml.AppendFmt("b=\"%d\" ", b );
        if (!enable) xml.Append("enable=\"false\" ");
        xml.Append("/>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (inverse)
            enable = !enable;

        if (useCurLoc)
        {
            if (!target)
            {
                // If we don't have a target, use the actor
                target = actor;

                if (!target)
                {
                    Error2("Error: ProgressionEvent(%s) WeatherOp need a target or actor for sector=\"this\"\n",
                           eventName->GetData() );
                    return true;
                }
            }

            iSector* sector = target->GetSector();
            if (sector == NULL)
            {
                Error2("Error: ProgressionEvent(%s) WeatherOp target is in an invalid sector!\n",
                       eventName->GetData() );
                return true;
            }

            const char* sectorname = sector->QueryObject()->GetName();
            sectorinfo = CacheManager::GetSingleton().GetSectorInfoByName(sectorname);
            if (!sectorinfo)
            {
                Error3("Missing SectorInfo in ProgressionEvent(%s) WeatherOp: %s\n",
                       eventName->GetData(), sectorname );
                return true;
            }

            if (wtype == psWeatherMessage::LIGHTNING && sectorinfo->lightning_max_gap == 0)
            {
                Error3("Lightning undefined for sector in ProgressionEvent(%s) WeatherOp: %s\n",
                       eventName->GetData(), sectorname );
                return true;
            }
        }

        if (wtype)
        {
            if (enable)  // Queue the weather event
                psserver->GetWeatherManager()->QueueNextEvent(0,wtype,density,duration,fade,sectorinfo->name,sectorinfo,0,r,g,b);
            else
                psserver->GetWeatherManager()->QueueNextEvent(0,wtype,0,0,fade,sectorinfo->name,sectorinfo);
        }
        else  // "auto"
        {
            if (enable)
            {
                sectorinfo->rain_enabled = true;
                psserver->GetWeatherManager()->StartWeather(sectorinfo);
            }
            else
            {
                sectorinfo->rain_enabled = false;
                // Prevents new auto-weather, but does not need to stop anything in-progress
            }
        }

        return true;
    }

protected:
    psSectorInfo* sectorinfo;   ///< Sector to work in
    bool useCurLoc;             ///< Use target or actor's current sector
    csString type;              ///< Type of weather
    int wtype;                  ///< Type of weather code
    int duration;               ///< Duration in seconds
    int density;                ///< Density of fog/precipitation
    int fade;                   ///< Fade in/out time in seconds
    int r, g, b;                ///< Color for fog
    bool enable;                ///< Are we turning on or off?
};

/*-------------------------------------------------------------*/

/**
 * CreatePetOp
 * Create pet for actor.
 *
 * Syntax:
 *    <createpet masterids="#|#,#,#,..." controlled="true|false" />
 *        masterids = "#" specific pet ID
 *        masterids = "#,#,# ..."  range of pet IDs to pick
 *        controlled = "true|false" not yet implimented
 */
class CreatePetOp : public ProgressionOperation
{
public:

    virtual bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) CreatePetOp needs an actor\n",eventName->GetData());
            return true;
        }

        if ( !actor->GetClientID() || !actor->GetClient() )
        {
            Error2("Error: ProgressionEvent(%s) CreatePetOp needs a valid client\n",eventName->GetData());
            return true;
        }

        int familiarid = 0;
        value = (int) GetValue(actor, target);

        switch ( master_ids.GetSize() )
        {
        case 0:
            break;
        case 1:
            familiarid = master_ids[ 0 ];
            break;
        default:
            familiarid = (int) ( value / ( 100 / master_ids.GetSize() ) );
            if ( familiarid < 0 ) familiarid = 0;
            if ( familiarid > (int)master_ids.GetSize() ) familiarid =(int) master_ids.GetSize() - 1;
            familiarid = master_ids[ familiarid ];
            break;
        }

        gemNPC *Familiar = EntityManager::GetSingleton().CreatePet( actor->GetClient(), familiarid );
        if ( Familiar == NULL )
        {
            Error2("Failed to create pet %d \n",familiarid);
            return false;
        }

        return true;
    }

    virtual bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        psString masterids( node->GetAttributeValue("master_ids") );
        psString token;
        int pos = 0;

        /* Establish string and get the first token: */
        masterids.GetWord( pos, token, psString::NO_PUNCT );
        while( token.Length() != 0 )
        {
            pos += (int)token.Length() + 1;
            master_ids.Push( atoi( token ) );
            masterids.GetWord( pos, token, psString::NO_PUNCT );
        }

        controlled = node->GetAttributeValueAsBool("controlled");

        return LoadValue(node, script);
    }

    virtual csString ToString()
    {
        return "<createpet />";
    }

protected:
    csArray<int> master_ids;
    bool controlled;
    int value;

};

/*-------------------------------------------------------------*/

/**
 * CreateFamiliarOp
 * Create familiar for actor.
 *
 * Syntax:
 *    <createfamiliar/>
 * Examples:
 *    Create a familiar near actor and send message:
 *        <createfamiliar /><msg text="Your new familiar appears nearby."/>
 */
class CreateFamiliarOp : public ProgressionOperation
{
public:

    virtual bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (inverse)
            return true;

        if (!actor)
        {
            Error2( "Error: ProgressionEvent(%s) CreateFamiliar needs an actor\n",eventName->GetData());
            return true;
        }

        if ( !actor->GetClientID())
        {
            Error3( "Error: ProgressionEvent(%s) CreateFamiliar needs a valid client for actor '%s'.\n",eventName->GetData(),actor->GetName() );
            return true;
        }

        if ( actor->GetCharacterData()->GetFamiliarID() == 0 )
        {
            gemNPC *Familiar = EntityManager::GetSingleton().CreateFamiliar( actor );
            if ( Familiar == NULL )
            {
                Error2( "Failed to create familiar for %s.\n", actor->GetName() );
                return false;
            }

            return true;

        }
        else
        {
            psserver->SendSystemInfo( actor->GetClientID(), "You already have a familiar, please take care of it.");
            return false;
        }
    }

    virtual bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        return true;
    }


    virtual csString ToString()
    {
        return "<createfamiliar />";
    }
};

/*-------------------------------------------------------------*/

/**
 * AnimalAffinityOp
 * Change animal affinity.
 *
 * Syntax:
 *    <animalaffinity name="%s" attribute="adjust|set" value="#" />
 *        name = "%s" animal name
 *        attribute = "adjust" add new value to old affinity
 *        attribute = "set" set attribute to new value
 *        value = "#" floating point value to set affinity
 * Examples:
 *    An item adds 3 to the animal affinity called "daylight":
 *        <animalaffinity attribute="adjust" name="daylight" value="3"/>
 *    An item adds 3 to the animal affinity called "reptile":
 *        <animalaffinity attribute="adjust" name="reptile" value="3"/>
 */
class AnimalAffinityOp : public ProgressionOperation
{
public:

    typedef enum { adjust_add, adjust_set } adjust_t;

    virtual bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        /// Pointer to the Crystal Space iDocumentSystem.
        //csRef<iDocumentSystem>  xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

        if ( !actor )
        {
            Error2( "Error: ProgressionEvent(%s) AnimalAffinityOp needs an actor\n", eventName->GetData() );
            return true;
        }

        Debug2(LOG_CHARACTER, actor->GetClientID(),"AnimalAffinityOp: %s \n", this->ToString().GetData() );

        //if ( !actor->GetClient() )
        //{
        //    CPrintf( CON_ERROR, "Error: ProgressionEvent(%s) AnimalAffinityOp needs a valid client\n", eventName->GetData() );
        //    return true;
        //}

        psCharacter *chardata = actor->GetCharacterData();
        csString animalAffinity = chardata->GetAnimalAffinity();

        if ( animalAffinity.Length() == 0 )
        {
            animalAffinity.Append("");
        }

        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />
        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />
        //     ...
        //     <category attribute="Type|Lifecycle|AttackTool|AttackType|..." name="" value="" />

        // Parse the string into an XML document.
        csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
        CS_ASSERT(xml != NULL);
        csRef<iDocument> xmlDoc  = xml->CreateDocument();
        const char* error = xmlDoc->Parse( animalAffinity );

        csRef<iDocumentNode> node;
        bool found = false;

        if ( !error )
        {
            // Find existing node
            csRef<iDocumentNodeIterator> iter = xmlDoc->GetRoot()->GetNodes();
            while ( iter->HasNext() )
            {
                node = iter->Next();

                csString operationStr = node->GetAttributeValue( "name" );
                if ( operationStr.CompareNoCase( name ) )
                {
                    found = true;
                    Debug3( LOG_PETS, actor->GetClientID(),"AnimalAffinityOp: %s : %s \n", name.GetData(), operationStr.GetData() );
                    break;
                }
            }
        }

        // Add new node if one doesn't exist
        if ( !found )
        {
            csString attrNode = psserver->GetProgressionManager()->GetAffinityCategories().Get( name.Downcase() , "" ).GetData();
            if ( attrNode.Length() != 0 )
            {
                node = xmlDoc->GetRoot()->CreateNodeBefore( CS_NODE_ELEMENT );
                node->SetValue( "category" );
                node->SetAttribute( "name" , name );
                node->SetAttribute( "attribute", attrNode );
                node->SetAttribute( "value" , "0" );
            }
            else
            {
                Error2( "Error: ProgressionEvent(%s) AnimalAffinityOp needs a valid category\n", eventName->GetData() );
                return false;
            }
        }

        // Modify Value
        if ( node )
        {
            float oldValue = node->GetAttributeValueAsFloat( "value" );
            float adjustValue = GetValue(actor, target);
            float newValue = CalcNewValue(oldValue, adjustValue, inverse, oldValue);

            Debug4( LOG_PETS, actor->GetClientID(),"AnimalAffinityOp: %f : %f : %f\n", oldValue, adjustValue, newValue );

            node->SetAttributeAsFloat( "value", newValue );
        }

        // Save changes back
        csRef<scfString> str;
        str.AttachNew( new scfString() );
        xmlDoc->Write( str );
        chardata->SetAnimialAffinity( str->GetData() );
        Debug2( LOG_PETS, actor->GetClientID(),"AnimalAffinityOp: %s \n", str->GetData() );

        return true;
    }


    float CalcNewValue(float oldValue, float adjustValue, bool inverse, float baseValue)
    {
        switch ( attribute )
        {
            case adjust_set: return adjustValue;
            case adjust_add: if ( inverse ) return oldValue - adjustValue; else return oldValue + adjustValue;
        }

        return 0.0;
    }

    virtual bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        attribute = adjust_add;

        if (node->GetAttributeValue("attribute"))
        {
            csString adjustStr = node->GetAttributeValue("attribute");
            if (adjustStr == "set")
                attribute = adjust_set;
        }

        name = node->GetAttributeValue("name");

        return LoadValue(node, script);

        Debug2( LOG_PETS, 0,"AnimalAffinityOp: %s \n", this->ToString().GetData() );
    }


    virtual csString ToString()
    {
        psString attrStr, xml;

        switch (attribute)
        {
            case adjust_set: attrStr = "set"; break;
            case adjust_add: attrStr = "adjust"; break;
        }
        xml.Format( "<animalaffinity name='%s' attribute='%s' value='%s'/>", name.GetData(), attrStr.GetData(), script_text.GetData() );

        return xml;
    }

protected:
    adjust_t attribute;
    csString name;
};

/*-------------------------------------------------------------*/

/**
 * ShowDetailsOp
 * Show actors character details.
 *
 * Syntax:
 *    <showdetails/>
 */
class ShowDetailsOp : public ProgressionOperation
{
public:
    csString * eventName;
    virtual bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        // Remove this when adding support for the inverse operation
        if (inverse)
            return true;

        gemActor * tgtAsActor;
        gemItem * tgtAsItem;
        Client * client;

        if (!actor)
        {
            Error2( "Error: ProgressionEvent(%s)  ShowDetailsOp needs an actor\n",
                    eventName->GetData());
            return true;
        }

        if (actor->GetCharacterData() == NULL)
            return true;

        client = psserver->GetConnections()->FindPlayer(actor->GetCharacterData()->GetPID());
        if (client == NULL)
            return true;

        tgtAsActor = dynamic_cast<gemActor*>(target);
        if (tgtAsActor != NULL)
        {
            psserver->usermanager->SendCharacterDescription(client, tgtAsActor->GetCharacterData(), true, false, "ShowDetailsOp");
            return true;
        }

        tgtAsItem = dynamic_cast<gemItem*>(target);
        if (tgtAsItem != NULL)
        {
            //TOBEDONE
            return true;
        }
        return true;
    }
    virtual bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        return true;
    }
    virtual csString ToString()
    {
        return "<showdetails/>";
    }
};

/*-------------------------------------------------------------*/

/**
 * MovementOp
 * Create move modification for actor.
 *
 * Syntax:
 *    <move type="reset|add|mod|const|push" duration="#"
 *     x="#" y="#" z="#" yrot="#" />
 *        type="reset" resets move modifiers
 *        type="add" add this to movements
 *        type="mod" multiply this with movements
 *        type="const" add this to velocity until duration over
 *        type="push" execute this movement once
 *        duration = "#" amount of ms before resetting modification 0 means never
 *        x = "#" change in x coord
 *        y = "#" change in y coord
 *        z = "#" change in z coord
 *        yrot = "#" change in yrot
 */
class MovementOp : public ProgressionOperation
{
public:
    MovementOp() : ProgressionOperation() { };
    virtual ~MovementOp() {};

    bool Load(iDocumentNode *node, ProgressionEvent *script)
    {
        type = node->GetAttributeValue("type");
        if (type == "reset")
        {
            typecode = psMoveModMsg::NONE;        // Remove all mods
            duration = 0;
            return true;  // All we need
        }
        else if (type == "add")
            typecode = psMoveModMsg::ADDITION;    // Add factor to moves
        else if (type == "mod")
            typecode = psMoveModMsg::MULTIPLIER;  // Multiply factor against moves
        else if (type == "const")
            typecode = psMoveModMsg::CONSTANT;    // Add factor until disabled
        else if (type == "push")
            typecode = psMoveModMsg::PUSH;        // Execute move once
        else
        {
            Error3( "Invalid type for ProgressionEvent(%s) MovementOp: %s\n", eventName->GetData(), type.GetData() );
            return true;
        }

        duration = node->GetAttributeValueAsInt("duration");

        if (typecode == psMoveModMsg::PUSH && duration != 0)
        {
            Error2("Duration cannot be used for type=\"push\" in ProgressionEvent(%s) MovementOp\n", eventName->GetData());
            return true;
        }

        moveMod.x = node->GetAttributeValueAsFloat("x");  // x-axis motion
        moveMod.y = node->GetAttributeValueAsFloat("y");  // y-axis motion
        moveMod.z = node->GetAttributeValueAsFloat("z");  // z-axis motion
        YrotMod = node->GetAttributeValueAsFloat("yrot"); // y-axis rotation

        // Make defaults for multiplication 1.0f instead of 0.0f
        if (typecode == psMoveModMsg::MULTIPLIER)
        {
            size_t i;
            for (i=0; i<3; i++)
                if (fabsf(moveMod[i]) < SMALL_EPSILON)
                    moveMod[i] = 1.0f;
            if (fabsf(YrotMod) < SMALL_EPSILON)
                YrotMod = 1.0f;
        }

        return true;
    }

    virtual csString ToString()
    {
        csString xml;
        xml.Format("<move type=\"%s\" ", type.GetData() );
        if (duration)
            xml.AppendFmt("duration=\"%d\" ", duration );
        if (typecode != psMoveModMsg::NONE)
            xml.AppendFmt("x=\"%.02f\" y=\"%.02f\" z=\"%.02f\" yrot=\"%.02f\" ", moveMod.x, moveMod.y, moveMod.z, YrotMod );
        xml.Append("/>");
        return xml;
    }

    bool Run(gemActor *actor, gemObject *target, psItem * item, bool inverse)
    {
        if (!actor)
        {
            Error2("Error: ProgressionEvent(%s) MovementOp needs an actor\n",eventName->GetData());
            return true;
        }

        Client* client = actor->GetClient();
        if (!client)
        {
            Error2( "Error: ProgressionEvent(%s) MovementOp needs a client\n",eventName->GetData());
            return true;
        }

        // Send modifier to client
        psMoveModMsg msg(client->GetClientNum(), typecode, moveMod, YrotMod);
        msg.SendMessage();

        if (duration != 0)
        {
            int persistentID = actor->GetCharacterData() ? actor->GetCharacterData()->RegisterProgressionEvent(ToString(), ticksElapsed) : 0;
            // Queue undo script
            psserver->GetProgressionManager()->QueueUndoScript("<evt><move type=\"reset\" /></evt>", duration*1000, actor, actor, item, persistentID);
        }

        return true;
    }

protected:
    csString type;                   ///< Type of modifier
    psMoveModMsg::ModType typecode;  ///< Type code from msg enum
    int duration;                    ///< Duration of effect in seconds
    csVector3 moveMod;               ///< Movement modifier
    float YrotMod;                   ///< Rotation modifier
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

psScriptGameEvent::psScriptGameEvent(csTicks offsetticks,
                                     ProgressionEvent * script,
                                     gemActor *actor,
                                     gemObject *target,
                                     psItem *item,
                                     int persistentID)
    : psGEMEvent(0,offsetticks,target,"psScriptGameEvent"), persistentID(persistentID)
{
    this->script   = script;
    this->actor    = actor;
    this->target   = target;
    this->item     = item;

    actor->RegisterCallback( dynamic_cast<iDeathCallback *>(this) );
    gemActor *targetActor = dynamic_cast<gemActor *>( target );
    if ( targetActor )
        targetActor->RegisterCallback( dynamic_cast<iDeathCallback *>(this) );

}

psScriptGameEvent::~psScriptGameEvent()
{
    if (actor)
        actor->UnregisterCallback(dynamic_cast<iDeathCallback *>(this));
    if (target && target->GetActorPtr())
        target->GetActorPtr()->UnregisterCallback(dynamic_cast<iDeathCallback *>(this));
}

/**
 * Called when the actor or target object dies.
 */
void psScriptGameEvent::DeathCallback(iDeathNotificationObject * object)
{
    Trigger();
    SetValid(false);
}

/**
 * Called when the target object disconnect. It will than store
 * the progression script in the DB to be executed when character
 * reconnect. There are no way to remove events from the event queue
 * so by setting script to NULL, the event is than marked for
 * no execution when it is triggered.
 */
void psScriptGameEvent::DeleteObjectCallback(iDeleteNotificationObject * object)
{
    psGEMEvent::DeleteObjectCallback(object);  // This removes from callback list.
    SetValid( false );
}

void psScriptGameEvent::Trigger()
{
    if ( IsValid() )
    {
        if (target.IsValid())
        {
            gemActor* act = (actor.IsValid()) ? dynamic_cast<gemActor*>((gemObject *) actor) : NULL ;
            script->Run(act, target, item);
            if (target->GetCharacterData())
            {
                target->GetCharacterData()->UnregisterProgressionEvent(persistentID);
            }
        }
    }
}



/*-------------------------------------------------------------*/

ProgressionManager::ProgressionManager(ClientConnectionSet *ccs)
{
    clients      = ccs;

    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleSkill)      ,MSGTYPE_GUISKILL,    REQUIRE_READY_CLIENT);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleDeathEvent) ,MSGTYPE_DEATH_EVENT, NO_VALIDATION);
    psserver->GetEventManager()->Subscribe(this,new NetMessageCallback<ProgressionManager>(this,&ProgressionManager::HandleZPointEvent),MSGTYPE_ZPOINT_EVENT,NO_VALIDATION);
}


ProgressionManager::~ProgressionManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GUISKILL);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_DEATH_EVENT);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_ZPOINT_EVENT);

    csHash<ProgressionEvent *, const char *>::GlobalIterator it(events.GetIterator());

    while ( it.HasNext () )
        delete it.Next();
}

void ProgressionManager::QueueUndoScript(const char *scriptText, int delay, gemActor *actor, gemObject *target, psItem * item, int persistentID)
{
    csRef<iDocument> doc = ParseString(scriptText);
    if (doc == NULL)
    {
        Error2("Failed to parse script %s", scriptText);
        return;
    }
    ProgressionEvent * script =  new ProgressionEvent();
    if (!script->LoadScript(doc))
    {
        Error2("Failed to load script %s", scriptText);
        return;
    }
    psScriptGameEvent * event =
            new psScriptGameEvent(delay, script, actor, target, item, persistentID);
    psserver->GetEventManager()->Push(event);
}

bool ProgressionManager::Initialize()
{
    Result result_events(db->Select("SELECT * from progression_events"));

    if ( result_events.IsValid() )
    {
        csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

        for ( size_t index = 0; index < result_events.Count(); index++ )
        {
            unsigned long x = (unsigned long)index; // remove casting warnings

            ProgressionEvent* ev = new ProgressionEvent;
            ev->name = result_events[x]["name"];

            csRef<iDocument> doc = xml->CreateDocument();
            const char* error = doc->Parse( result_events[x]["event_script"] );
            if ( error )
            {
                Error2("Could not parse the event named %s. Reason: ", ev->name.GetData() );
                Error3("%s\n%s",error,result_events[x]["event_script"]);
                delete ev;
                return false;
            }
            if (ev->LoadScript(doc))
            {
                events.Put(ev->name, ev);
            }
            else
            {
                Error2("Couldn't load script %s\n",ev->name.GetData());
                delete ev;
                return false;
            }
        }
    }

    Result result_affinitycategories(db->Select("SELECT * from char_create_affinity"));

    if ( result_affinitycategories.IsValid() )
    {
        for ( unsigned int x = 0; x < result_affinitycategories.Count(); x++ )
        {
            affinitycategories.Put( csString( result_affinitycategories[(unsigned long)x]["category"]).Downcase() , csString( result_affinitycategories[(unsigned long)x]["attribute"]).Downcase() );
        }
    }

    return true;
}


void ProgressionManager::HandleZPointEvent(MsgEntry *me, Client *client)
{
    psZPointsGainedEvent evt(me);

    csString string;
    string.Format("You've gained some practice points in %s.", evt.skillName.GetData() );
    if ( evt.rankUp )
    {
        string.Append(" You've also ranked up!");
    }
    psserver->SendSystemInfo(evt.actor->GetClientID(), string);

    SendSkillList( client, false );
}


void ProgressionManager::HandleDeathEvent(MsgEntry *me, Client *notused)
{
    Debug1(LOG_COMBAT, me->clientnum,"Progression Manager handling Death Event\n");
    psDeathEvent evt(me);

    // Only do progression if dead guy is an NPC and not a pet or if a gm enabled the givexp flag
    if ((evt.deadActor->GetClientID()==0 && !evt.deadActor->GetCharacterData()->IsPet()) || evt.deadActor->givekillexp)
    {
        csString progEvent = FindEvent( "kill" )->ToString(true);

        // Convert the exp to a char
        csString buffer;
        buffer.Format("%lu",(unsigned long) evt.deadActor->GetCharacterData()->GetKillExperience());

        ChangeScript( progEvent, 0, (const char*)buffer );
        ProcessScript(progEvent,evt.killer,evt.deadActor);
    }
}


void ProgressionManager::HandleSkill(MsgEntry *me, Client * client)
{
    psGUISkillMessage msg(me);
    if (!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUISkillMessage from client %u.\n",me->clientnum);
        return;
    }

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch ( msg.command )
    {
        case psGUISkillMessage::REQUEST:
        {
            // Clear the current skill cache
            psCharacter * character = client->GetCharacterData();
            if (character)
                character->GetSkillCache()->clear();

            SendSkillList(client,false);
            break;
        }
        case psGUISkillMessage::SKILL_SELECTED:
        {
            csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

            CS_ASSERT( xml );

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse( msg.commandData);
            if ( error )
            {
                Error2("Error in XML: %s", error );
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("S");
            if(!topNode)
            {
                Error1("No <S> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");

            psSkillInfo * info = CacheManager::GetSingleton().GetSkillByName(skillName);
            Faction * faction = CacheManager::GetSingleton().GetFactionByName(skillName);
            csString buff;
            csString description;
            int cathegory;
            if (info)
            {
                description = EscpXML(info->description).GetData();
                cathegory = info->category;
            }
            else if (faction)
            {
                description = faction->description;
                cathegory = PSSKILLS_CATEGORY_FACTIONS;
            }
            else
            {
                description = "";
                cathegory = PSSKILLS_CATEGORY_VARIOUS;
            }
            buff.Format("<DESCRIPTION NAME=\"%s\" DESC=\"%s\" CAT=\"%d\"/>",
                        EscpXML(skillName).GetData(), description.GetData(),
                        cathegory);

            psCharacter* chr = client->GetCharacterData();
            psGUISkillMessage newmsg(client->GetClientNum(),
                            psGUISkillMessage::DESCRIPTION,
                            buff,
                            NULL,
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_STRENGTH)),
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_ENDURANCE)),
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_AGILITY)),
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_INTELLIGENCE)),
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_WILL)),
                            (unsigned int)(chr->Stats().GetStat(PSITEMSTATS_STAT_CHARISMA)),
                            (unsigned int)(chr->GetHP()),
                            (unsigned int)(chr->GetMana()),
                            (unsigned int)(chr->GetStamina(true)),
                            (unsigned int)(chr->GetStamina(false)),
                            (unsigned int)(chr->GetHitPointsMax()),
                            (unsigned int)(chr->GetManaMax()),
                            (unsigned int)(chr->GetStaminaMax(true)),
                            (unsigned int)(chr->GetStaminaMax(false)),
                            true,
                            PSSKILL_NONE,
                            -1,
                            false);

            if (newmsg.valid)
                SendMessage(newmsg.msg);
            else
            {
                Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
            }
            break;
        }
        case psGUISkillMessage::BUY_SKILL:
        {
            Debug1(LOG_SKILLXP, client->GetClientNum(),"---------------Buying Skill-------------\n");
            csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);

            CS_ASSERT( xml );

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse( msg.commandData);
            if ( error )
            {
                Error2("Error in XML: %s", error );
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("B");
            if(!topNode)
            {
                Error1("No <B> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");
            uint skillAmount = topNode->GetAttributeValueAsInt("AMOUNT");

            psSkillInfo * info = CacheManager::GetSingleton().GetSkillByName(skillName);
            Debug2(LOG_SKILLXP, client->GetClientNum(),"    Looking for: %s\n", (const char*)skillName);

            if (!info)
            {
                Error2("No skill with name %s found!",skillName.GetData());
                Error2("Full Data Sent from Client was: %s\n", msg.commandData.GetData() );
                return;
            }

            psCharacter * character = client->GetCharacterData();

            if (character->GetTrainer() == NULL)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "Can't buy skills when not training!");
                return;
            }

            gemActor* actorTrainer = character->GetTrainer()->GetActor();
            if ( actorTrainer )
            {
                if ( character->GetActor()->RangeTo(actorTrainer, false) > RANGE_TO_SELECT )
                {
                    psserver->SendSystemInfo(client->GetClientNum(),
                                             "Need to get a bit closer to understand the training.");
                    return;
                }
            }

            if (character->GetMode() != PSCHARACTER_MODE_PEACE)
            {
                csString err;
                err.Format("You can't train while %s.", character->GetModeStr());
                psserver->SendSystemError(client->GetClientNum(), err);
                return;
            }

            Debug2(LOG_SKILLXP, client->GetClientNum(),"    PP available: %u\n", character->GetProgressionPoints() );

            // Test for progression points
            if (character->GetProgressionPoints() < skillAmount)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have enough progression points!");
                return;
            }

            // Test for money

            if ((info->price * skillAmount) > character->Money())
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have the money to buy this skill!");
                return;
            }
            if ( !character->CanTrain( info->id ) )
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill any higher yet!");
                return;
            }

            unsigned int current = character->Skills().GetSkillRank((PSSKILL) info->id, false);
            float faction = actorTrainer->GetRelativeFaction(character->GetActor());
            if ( !character->GetTrainer()->GetTrainerInfo()->TrainingInSkill((PSSKILL)info->id, current, faction))
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill currently.");
                return;
            }

            character->UseProgressionPoints(skillAmount);
            character->SetMoney(character->Money()-(info->price * skillAmount));
            character->Train(info->id,skillAmount);
            SendSkillList(client,true,info->id);
            psserver->GetCharManager()->UpdateItemViews(client->GetClientNum());
            psserver->SendSystemInfo(client->GetClientNum(), "You've received some %s training", skillName.GetData());

            break;
        }
        case psGUISkillMessage::QUIT:
        {
            client->GetCharacterData()->SetTrainer(NULL);
            client->GetCharacterData()->GetSkillCache()->clear();
            break;
        }
    }
}

void ProgressionManager::SendSkillList(Client * client, bool forceOpen, PSSKILL focus, bool isTraining )
{
    psCharacter * character = client->GetCharacterData();
    psCharacter * trainer = character->GetTrainer();
    psTrainerInfo * trainerInfo = NULL;
    float faction = 0.0;
    int selectedSkillCat = -1; //This is used for storing the category of the selected skill
    int selectedSkillNameId = -1; // Name ID value of the selected skill

    // Get the current skill cache
    psSkillCache *skills = character->GetSkillCache();

    skills->setProgressionPoints(character->GetProgressionPoints());

    if (trainer)
    {
        trainerInfo = trainer->GetTrainerInfo();
        faction = trainer->GetActor()->GetRelativeFaction(character->GetActor());
    }


    for (int skillID = 0; skillID < (int)PSSKILL_COUNT; skillID++)
    {
        psSkillInfo * info = CacheManager::GetSingleton().GetSkillByID(skillID);
        if (!info)
        {
            Error2("Can't find skill %d",skillID);
            return;
        }

        Skill *charSkill = character->Skills().GetSkill((PSSKILL) skillID);
        if (charSkill == NULL)
        {
            Error3("Can't find skill %d in character %s", skillID, ShowID(character->GetPID()));
            return;
        }

        // If we are training, send skills that the trainer is providing education in only
        if  (
                !trainerInfo
                    ||
                trainerInfo->TrainingInSkill((PSSKILL) skillID, character->Skills().GetSkillRank((PSSKILL) skillID, false), faction)
             )
        {
            bool stat = info->id == PSSKILL_AGI ||
                        info->id == PSSKILL_CHA ||
                        info->id == PSSKILL_END ||
                        info->id == PSSKILL_INT ||
                        info->id == PSSKILL_WILL ||
                        info->id == PSSKILL_STR;

            /* Get the ID value for the skill name string and find the skill
               in the cache. If it can't be found, skip this skill.
            */
            unsigned int skillNameId =
                    CacheManager::GetSingleton().FindCommonStringID(info->name);

            if (skillNameId == 0)
            {
                Error2("Can't find skill name \"%s\" in common strings", info->name.GetData());
                continue;
            }

            psSkillCacheItem *item = skills->getItemBySkillId(skillID);

            if (info->id == focus)
            {
                selectedSkillNameId = (int)skillNameId;
                selectedSkillCat=info->category;
            }

            unsigned int actualStat;

            if (info->category != 0)
                actualStat = character->Skills().GetSkillRank((PSSKILL) skillID);
            else
            {
                if(info->name=="Strength")
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_STRENGTH);
                }
                else if(info->name== "Endurance")
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_ENDURANCE);
                }
                else if(info->name== "Agility")
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_AGILITY);
                }
                else if(info->name== "Intelligence")
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_INTELLIGENCE);
                }
                else if(info->name== "Will")
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_WILL);
                }
                else
                {
                    actualStat=character->Stats().GetStat(PSITEMSTATS_STAT_CHARISMA);
                }

            }

            if (item)
            {
                item->update(charSkill->rank, actualStat,
                             charSkill->y, charSkill->yCost,
                             charSkill->z, charSkill->zCost);
            } else
            {
                item = new psSkillCacheItem(skillID, skillNameId,
                                            charSkill->rank, actualStat,
                                            charSkill->y, charSkill->yCost,
                                            charSkill->z, charSkill->zCost,
                                            info->category, stat);
                skills->addItem(skillID, item);
            }
        } else if (trainerInfo)
        {
            // We are training, but this skill is not available for training
            psSkillCacheItem *item = skills->getItemBySkillId(skillID);
            if (item)
                item->setRemoved(true);
        }
    }

    bool training= false;
    if (isTraining)
        training= true;

    psGUISkillMessage newmsg(client->GetClientNum(),
                            psGUISkillMessage::SKILL_LIST,
                            "",
                            skills,
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_STRENGTH),
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_ENDURANCE),
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_AGILITY),
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_INTELLIGENCE),
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_WILL),
                            (unsigned int)character->Stats().GetStat(PSITEMSTATS_STAT_CHARISMA),
                            (unsigned int)character->GetHP(),
                            (unsigned int)character->GetMana(),
                            (unsigned int)character->GetStamina(true),
                            (unsigned int)character->GetStamina(false),
                            (unsigned int)character->GetHitPointsMax(),
                            (unsigned int)character->GetManaMax(),
                            (unsigned int)character->GetStaminaMax(true),
                            (unsigned int)character->GetStaminaMax(false),
                            forceOpen,
                            selectedSkillNameId,
                            selectedSkillCat,
                            training //If we are training the client must know it
                            );

    Debug2(LOG_SKILLXP, client->GetClientNum(),"Sending psGUISkillMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if (newmsg.valid)
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"Yes\n");
        SendMessage(newmsg.msg);

    }
    else
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"No\n");
        Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
    }
}

void ProgressionManager::StartTraining(Client * client, psCharacter * trainer)
{
    client->GetCharacterData()->SetTrainer(trainer);
    SendSkillList(client, true, PSSKILL_NONE, true);
}


float ProgressionManager::ProcessEvent(const char *event, gemActor * actor, gemObject *target, psItem *item, bool inverse)
{
    ProgressionEvent * ev = FindEvent(event);
    if (ev)
    {
        return ProcessEvent(ev, actor, target, item, inverse);
    }

    csString actorName = "N/A";
    csString targetName = "N/A";

    if ( actor )
        actorName = actor->GetName();
    if ( target )
        targetName = target->GetName();

    Error4( "Error: Can't find progression event: %s from actor [%s] on target [%s]\n", event, actorName.GetData(), targetName.GetData());

    return 0.0;
}

float ProgressionManager::ProcessEvent(ProgressionEvent * ev, gemActor * actor, gemObject *target, psItem *item, bool inverse)
{
    Debug5(LOG_SPELLS, actor?actor->GetClientID():0,"Process %s event %s on %s with target %s.\n",
      inverse ? "inverse" : "", ev->name.GetData(),(actor?actor->GetName():"(null)"),
            (target?target->GetName():"(null)"));

    if(inverse)
        return ev->RunInverse(actor, target, item);
    else
        return ev->Run(actor, target, item);
}

ProgressionEvent * ProgressionManager::CreateEvent(const char *name, const char *script)
{
    ProgressionEvent *ev = new ProgressionEvent;
    // Create a Uniq event name!!
    int count = 0;
    do {
        count ++;
        int uniq_number = (int)(9999*psserver->GetRandom());
        ev->name.Format("%s%04d",name,uniq_number++);
    } while (FindEvent(ev->name) != NULL && count < 100);
    CS_ASSERT_MSG("Uniq name not found. Failed to execute script.", count < 100);

    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( script );
    if ( error )
    {
        Error3("%s\n%s",error, script );
        delete ev;
        return NULL;
    }
    if (ev->LoadScript(doc))
    {
        events.Put(ev->name, ev);
    }
    else
    {
        Error2("Faild to load %s", script );
        delete ev;
        return NULL;
    }

    return ev;
}


float ProgressionManager::ProcessScript(const char *script, gemActor * actor, gemObject *target, psItem *item)
{
    ProgressionEvent * ev = CreateEvent((actor?actor->GetName():"Unknown"),script);

    if (ev)
    {
        return ProcessEvent(ev->name,actor,target,item);
    }

    return 0.0;
}

bool ProgressionManager::AddScript(const char *name, const char *script)
{
    ProgressionEvent *ev = new ProgressionEvent;

    csRef<iDocumentSystem> xml = csPtr<iDocumentSystem>(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( script );
    if ( error )
    {
        Error3("Adding script error: %s\n%s",error, script );
        delete ev;
        return false;
    }
    ev->name = name;

    if (ev->LoadScript(doc))
        events.Put(ev->name, ev);
    else
    {
        Error2("Loading script error: %s", script );
        delete ev;
        return false;
    }

    csString escText;
    db->Escape( escText, script );
    unsigned long res = db->Command("INSERT INTO progression_events "
                             "(name,event_script) "
                             "VALUES (\"%s\",\"%s\")",name, escText.GetData());
    if(res!=1)
        return false;
    else
        return true;
}


ProgressionEvent *ProgressionManager::FindEvent(char const *name)
{
    if ( name == NULL ) return NULL;

    return events.Get(name, NULL);
}

void ProgressionManager::QueueEvent(psGameEvent *event)
{
    psserver->GetEventManager()->Push(event);
}

void ProgressionManager::SendMessage(MsgEntry *me)
{
    psserver->GetEventManager()->SendMessage(me);
}

void ProgressionManager::Broadcast(MsgEntry *me)
{
    psserver->GetEventManager()->Broadcast(me, NetBase::BC_EVERYONE);
}

void ProgressionManager::ChangeScript( csString& script, int param, const char* text )
{
    psString scriptString( script );
    csString buff;
    buff.Format("$%d", param );

    scriptString.ReplaceAllSubString( buff, text );
    script.Replace( scriptString.GetData() );
}

/*-------------------------------------------------------------*/

ProgressionDelay::ProgressionDelay(ProgressionEvent * progEvent, csTicks delay, unsigned int clientnum)
                : psGameEvent(0, delay, "Progression Event Delay"),
                  progEvent(progEvent)
{
    client = clientnum;
}

ProgressionDelay::~ProgressionDelay()
{
}

bool ProgressionDelay::CheckTrigger()
{
    if ( !valid )
    {
        Client* clt = psserver->GetConnections()->FindAny(client);
        if (clt)
        {
            clt->GetCharacterData()->UnregisterDurationEvent(this);
        }
    }

    return valid;
}


void ProgressionDelay::Trigger()
{
    Client* clt = psserver->GetConnections()->FindAny(client);
    if (clt)
    {
        progEvent->ForceRun();
        clt->GetCharacterData()->UnregisterDurationEvent(this);
    }

    valid = false;
}

ProgressionEvent::ProgressionEvent()
                : triggerDelay(0), progDelay(0)
{
    durationScript = NULL;
    durationVar = NULL;
}

ProgressionEvent::~ProgressionEvent()
{
    while (sequence.GetSize())
    {
        delete sequence.Pop();
    }
    while (variables.GetSize())
    {
        delete variables.Pop();
    }

    delete triggerDelay;
    delete durationScript;

    if( progDelay )
    {
        //delete progDelay;
    }
}


bool ProgressionEvent::LoadScript(const char* data)
{
   csRef<iDocument> xmlDoc = ParseString(data);
   return LoadScript(xmlDoc);
}

bool ProgressionEvent::LoadScript(iDocument *doc)
{
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in progression script");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("evt");
    if (!topNode)
    {
        Error1("Could not find <evt> tag in progression script!");
        return false;
    }

    return LoadScript(topNode);
}

bool ProgressionEvent::LoadScript(iDocumentNode *topNode)
{
    progDelay = 0;

    // get the elapsed time, in case this is a saved event that already ran for a while
    int ticksElapsed = topNode->GetAttributeValueAsInt("elapsed");

    if ( topNode->GetAttribute("name") )
    {
        savedName = topNode->GetAttributeValue("name");
    }

    // Check to see if there is a duration script to load.
    if ( topNode->GetAttribute("duration") )
    {
        csString durationString = "Duration = ";
        durationString += topNode->GetAttributeValue("duration");

        csString scriptName = name;
        scriptName += "_duration";
        durationScript = new MathScript(scriptName, durationString);
        durationVar = durationScript->GetVar("Duration");
    }

    // get the delay between the call to Run() and actually executing the script
    csString delayText = topNode->GetAttributeValue("delay");
    if (!delayText.IsEmpty())
    {
        csString triggerDelayScript = "TriggerDelay = ";
        triggerDelayScript += delayText;

        csString scriptName = name;
        scriptName += "_triggerDelay";
        triggerDelay = new MathScript(scriptName, triggerDelayScript);
        triggerDelayVar = triggerDelay->GetVar("TriggerDelay");
    }
    else
    {
        triggerDelay = 0;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        ProgressionOperation * op = NULL;

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "agi" ) == 0 )
        {
            op = new StatsOp(StatsOp::AGI);
        }
        else if ( strcmp( node->GetValue(), "area" ) == 0 )
        {
            op = new AreaOp();
        }
        else if ( strcmp( node->GetValue(), "cha" ) == 0 )
        {
            op = new StatsOp(StatsOp::CHA);
        }
        else if ( strcmp( node->GetValue(), "con" ) == 0 )
        {
            op = new StatsOp(StatsOp::CON);
        }
        else if ( strcmp( node->GetValue(), "charge" ) == 0 )
        {
            op = new ChargeOp();
        }
        else if ( strcmp( node->GetValue(), "recharge" ) == 0 )
        {
            op = new RechargeOp();
        }
        else if ( strcmp( node->GetValue(), "end" ) == 0 )
        {
            op = new StatsOp(StatsOp::END);
        }
        else if ( strcmp( node->GetValue(), "exp" ) == 0 )
        {
            op = new ExperienceOp;
        }
        else if ( strcmp( node->GetValue(), "faction" ) == 0 )
        {
            op = new FactionOp;
        }
        else if ( strcmp( node->GetValue(), "pstamina" ) == 0 )
        {
            op = new StatsOp(StatsOp::PSTAMINA);
        }
        else if ( strcmp( node->GetValue(), "mstamina" ) == 0 )
        {
            op = new StatsOp(StatsOp::MSTAMINA);
        }
        else if ( strcmp( node->GetValue(), "hp" ) == 0 )
        {
            op = new StatsOp(StatsOp::HP);
        }
        else if ( strcmp( node->GetValue(), "int" ) == 0 )
        {
            op = new StatsOp(StatsOp::INT);
        }
        else if ( strcmp( node->GetValue(), "item" ) == 0 )
        {
            op = new ItemOp();
        }
        else if ( strcmp( node->GetValue(), "mana" ) == 0 )
        {
            op = new StatsOp(StatsOp::MANA);
        }
        else if ( strcmp( node->GetValue(), "msg" ) == 0 )
        {
             op = new MsgOp();
        }
        else if ( strcmp( node->GetValue(), "block" ) == 0 )
        {
             op = new BlockOp();
        }
        else if ( strcmp( node->GetValue(), "purify" ) == 0 )
        {
            op = new PurifyOp();
        }
        else if ( strcmp( node->GetValue(), "script" ) == 0 )
        {
            op = new ScriptOp();
        }
        else if ( strcmp( node->GetValue(), "str" ) == 0 )
        {
            op = new StatsOp(StatsOp::STR);
        }
        else if ( strcmp( node->GetValue(), "wil" ) == 0 )
        {
            op = new StatsOp(StatsOp::WIL);
        }
        else if ( strcmp( node->GetValue(), "sta" ) == 0 )
        {
            op = new StatsOp(StatsOp::STA);
        }
        else if ( strcmp( node->GetValue(), "attack" ) == 0 )
        {
            op = new StatsOp(StatsOp::ATTACK);
        }
        else if ( strcmp( node->GetValue(), "defense" ) == 0 )
        {
            op = new StatsOp(StatsOp::DEFENSE);
        }
        else if ( strcmp( node->GetValue(), "hpRate" ) == 0 )
        {
            op = new StatsOp(StatsOp::HPRATE);
        }
        else if ( strcmp( node->GetValue(), "mRate" ) == 0 )
        {
            op = new StatsOp(StatsOp::MRATE);
        }
        else if ( strcmp( node->GetValue(), "pStamRate" ) == 0 )
        {
            op = new StatsOp(StatsOp::PSTAMRATE);
        }
        else if ( strcmp( node->GetValue(), "mStamRate" ) == 0 )
        {
            op = new StatsOp(StatsOp::MSTAMRATE);
        }
        else if ( strcmp( node->GetValue(), "skill" ) == 0 )
        {
            op = new SkillOp();
        }
        else if ( strcmp( node->GetValue(), "showdetails" ) == 0 )
        {
            op = new ShowDetailsOp();
        }
        else if ( strcmp( node->GetValue(), "attachscript" ) == 0 )
        {
            op = new AttachScriptOp();
        }
        else if ( strcmp( node->GetValue(), "detachscript" ) == 0 )
        {
            op = new DetachScriptOp();
        }
        else if ( strcmp( node->GetValue(), "identifymagic" ) == 0 )
        {
            op = new IdentifyMagicOp();
        }
        else if ( strcmp( node->GetValue(), "createnpc" ) == 0 )
        {
            op = new CreatePetOp();
        }
        else if ( strcmp( node->GetValue(), "createpet" ) == 0 )
        {
            op = new CreatePetOp();
        }
        else if ( strcmp( node->GetValue(), "createfamiliar" ) == 0 )
        {
            op = new CreateFamiliarOp();
        }
        else if ( strcmp( node->GetValue(), "move" ) == 0 )
        {
            op = new MovementOp();
        }
        else if ( strcmp( node->GetValue(), "trait" ) == 0 )
        {
            op = new TraitChangeOp();
        }
        else if ( strcmp( node->GetValue(), "animalaffinity" ) == 0 )
        {
            op = new AnimalAffinityOp();
        }
        else if ( strcmp( node->GetValue(), "morph" ) == 0 )
        {
            op = new MorphOp();
        }
        else if ( strcmp( node->GetValue(), "craft" ) == 0 )
        {
            op = new CraftOp();
        }
        else if ( strcmp( node->GetValue(), "set" ) == 0 )
        {
            op = new AttributeOp();
        }
        else if ( strcmp( node->GetValue(), "weather" ) == 0 )
        {
            op = new WeatherOp();
        }
        else if ( strcmp( node->GetValue(), "quest" ) == 0 )
        {
            op = new QuestOp();
        }
        else if ( strcmp( node->GetValue(), "teleport" ) == 0 )
        {
            op = new TeleportOp();
        }
        else if ( strcmp( node->GetValue(), "key" ) == 0 )
        {
            op = new KeyOp();
        }
        else if ( strcmp( node->GetValue(), "effect" ) == 0 )
        {
            op = new EffectOp();
        }
        else if ( strcmp( node->GetValue(), "action" ) == 0 )
        {
            op = new ActionOp();
        }
        else if ( strcmp( node->GetValue(), "introduce" ) == 0 )
        {
            op = new IntroduceOp();
        }
        else if ( strcmp( node->GetValue(), "fire_event" ) == 0 )
        {
            op = new FireEventOp();
        }
        else
        {
            Error3("Unknown script operation %s in script %s.",node->GetValue(), name.GetData() );
            return false;
        }

        op->eventName = &name;

        op->SetTicksElapsed(ticksElapsed);
        if (op->Load(node, this))
            sequence.Push(op);
        else
        {
            delete op;
            return false;
        }
    }
    return true;
}

csString ProgressionEvent::ToString(bool topLevel) const
{
    csString xml;
    if (topLevel) xml.Append("<evt>");
    csArray<ProgressionOperation*>::ConstIterator seq = sequence.GetIterator();
    while (seq.HasNext())
    {
        ProgressionOperation * po = seq.Next();
        xml.Append(po->ToString());
    }
    if (topLevel) xml.Append("</evt>");
    return xml;
}

void ProgressionEvent::CopyVariables(MathScript *from)
{
    csHash<MathScriptVar*>::GlobalIterator iter = from->variables.GetIterator();
    while (iter.HasNext())
    {
        MathScriptVar *ptr = iter.Next();
        MathScriptVar *mine = FindVariable(ptr->name);
        if (!mine)
        {
            mine = new MathScriptVar;
            variables.Push(mine);
        }
        mine->Copy(ptr);
    }
}

void ProgressionEvent::LoadVariables(MathScript *script)
{
    if (!script)
        return;

    size_t i;
    for (i=0; i<variables.GetSize(); i++)
    {
        MathScriptVar *var = script->GetOrCreateVar(variables[i]->name);
        var->SetValue(variables[i]->GetValue() );
    }
}

MathScriptVar *ProgressionEvent::FindOrCreateVariable(const char *name)
{
    MathScriptVar *found = FindVariable(name);
    if (found)
        return found;

    MathScriptVar *pv = new MathScriptVar;
    pv->name = name;
    pv->SetValue(0);
    AddVariable(pv);

    return pv;
}

void ProgressionEvent::AddVariable(MathScriptVar *pv)
{
    MathScriptVar *found = FindVariable(pv->name);
    if (!found)
        variables.Push(pv);
    else
        found->Copy(pv);
}


void ProgressionEvent::SetValue( const char* name, double val )
{
    for( size_t z = 0; z < sequence.GetSize(); z++ )
    {
        MathScript* script = sequence[z]->GetMathScript();
        if ( script )
        {
            MathScriptVar* var = script->GetVar( name );
            if ( var )
            {
                var->SetValue( val );
            }
        }
    }
}

MathScriptVar *ProgressionEvent::FindVariable(const char *name)
{
    size_t i;

    for (i=0; i<variables.GetSize(); i++)
    {
        if (variables[i]->name == name)
            return variables[i];
    }
    return NULL;
}

float ProgressionEvent::ForceRun(csTicks duration)
{
    float result = 1.0f;

    csString dump_str = Dump();
    Notify5(LOG_SCRIPT,"%s is running this script %s on %s:\n%s",
            (runParamActor ? runParamActor->GetName() : ""),
            (runParamInverse ? "inversed":""),
            (runParamTarget ? runParamTarget->GetName() : "nobody"),
            dump_str.GetData());

    csArray<ProgressionOperation*>::Iterator seq = sequence.GetIterator();
    csString finalScript;
    while (seq.HasNext())
    {
        ProgressionOperation * po = seq.Next();
        po->LoadVariables(variables);

        if (!po->Run(runParamActor, runParamTarget, runParamItem, runParamInverse))
        {
            break;
        }
        // If the op already queued
        finalScript.Append(po->Absolute());

        result += po->GetResult();
        Notify3(LOG_SCRIPT,"Event: %s with result %f\n", po->ToString().GetData(), po->GetResult());
    }


    // If this script is set to run for a particular length of time. Then insert a new
    // event that is the inverse to run.
    if (duration == 0 && durationScript != NULL )
    {
        size_t a;
        size_t len = variables.GetSize();
        for (a=0; a<len; ++a)
        {
            MathScriptVar * var = durationScript->GetOrCreateVar(variables[a]->name);
            var->SetValue(variables[a]->GetValue());
        }
        durationScript->Execute();
        duration  = (csTicks)durationVar->GetValue();
    }
    if(finalScript.Length() > 0 && duration > 0)
    {

        csString scriptStr;
        scriptStr.Format("<evt>%s</evt>", finalScript.GetData());

        csRef<ProgressionEvent> script =  csPtr<ProgressionEvent> (new ProgressionEvent());
        script->LoadScript(scriptStr.GetData());
        script->name = name;
        script->runParamInverse = true;
        script->runParamActor = runParamActor;
        script->runParamTarget = runParamTarget;

        // Memory Leak?
        progDelay = new ProgressionDelay(script, duration, runParamActor->GetClientID());

        if ( savedName.Length() > 0 )
        {
            runParamActor->GetCharacterData()->RegisterDurationEvent(progDelay, savedName, duration);
        }
        else
        {
            runParamActor->GetCharacterData()->RegisterDurationEvent(progDelay, name, duration);
        }

        progDelay->QueueEvent();
    }


        /*
        // Memory Leak?
        ProgressionEvent * script =  new ProgressionEvent();
        script->LoadScript(ToString(false));
        script->name = name;
        script->runParamInverse = false;
        script->runParamActor = runParamActor;
        script->runParamTarget = runParamTarget;

        // Memory Leak?
        progDelay = new ProgressionDelay(script, duration, runParamActor->GetClientID());
        progDelay->QueueEvent();
        */



    return result;
}

float ProgressionEvent::RunScript(gemActor * actor, gemObject *target, psItem * item, bool inverse, csTicks duration)
{
    runParamActor = actor;
    runParamTarget = target;
    runParamInverse = inverse;
    runParamItem = item;

    // calculate delay if there is one
    csTicks delay = 0;
    if (triggerDelay)
    {
        size_t a;
        size_t len = variables.GetSize();
        for (a=0; a<len; ++a)
        {
            MathScriptVar * var = triggerDelay->GetOrCreateVar(variables[a]->name);
            var->SetValue(variables[a]->GetValue());
        }
        triggerDelay->Execute();
        delay = (csTicks)triggerDelayVar->GetValue();
    }

    if (delay <= 0)
    {
        return ForceRun(duration);
    }

    // schedule a delay
    progDelay = new ProgressionDelay(this, delay, actor->GetClientID());
    progDelay->QueueEvent();

    return 1.0f; // we don't have a good return at this point, so just assume it didn't fail and return non-zero
}

csString ProgressionEvent::Dump() const
{
    csString str;
    str.Append("Name  : ");
    str.Append(name.GetData());
    str.Append("\nScript:\n");
    str.Append(ToString(true));
    str.Append("\nVariables:\n");
    for (size_t i=0; i < variables.GetSize(); i++)
    {
        str.Append(variables[i]->Dump());
        str.Append("\n");
    }
    return str;
}
