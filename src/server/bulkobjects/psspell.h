/*
 * psspell.h
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

#ifndef __PSSPELL_H__
#define __PSSPELL_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "../iserver/idal.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"

class csVector3;
class SpellManager;
class Client;
class gemObject;
class gemActor;
class psGlyph;
class psSpellCastGameEvent;
class MathScript;
class MathScriptVar;

struct psWay 
{
    unsigned int id;
    PSSKILL      skill;    // for example, Crystal Way
    PSSKILL      related_stat; // for example, Charisma
    csString     name;
};


typedef csArray <psItemStats*> glyphList_t;


/**
 * Represents a spell.  This is mostly data that is cached in from the 
 * database to represent what a spell is. It contains details such as the
 * required glyphs as well as the effect of the spell.
 */
class psSpell : public iScriptableVar
{
 public:
    psSpell();
    ~psSpell();

    friend class SpellManager;

    bool Load(iResultRow& row);

    int GetID() const { return id; }    
    const csString& GetName() const { return name; }
    const csString& GetImage() const { return image; }
    const csString& GetDescription() const { return description; }
    float ManaCost(float KFactor) const;
    float ChanceOfSuccess(float KFactor, float waySkill, float relatedStat) const;    

    //csTicks GetCastingDuration() const { return casting_duration; }

    PSSKILL GetSkill() const;  // Return the needed skill to cast this spell.
    PSSKILL GetRelatedStat() const;  // Return the related stat for this way.

    /** Takes a list of glyphs and compares them to the correct sequence to 
      * construct this spell.
      */
    bool MatchGlyphs(const glyphList_t & glyphs);

    
    /** Creates a new instance of this spell.  
     *  Preforms all the necessary checks on the player to make sure they meet 
     *  the requirements to cast this spell.  
     *  1) The character is not in PSCHARACTER_MODE_PEACE mode.
     *  2) The player has the required glyphs.
     *  3) The player has the required mana.
     *  4) The player has a target. 
     *  5) The target is castable ( ie allowed to attack ).
     *  6) Check to see if self is targeted for non self spells. 
     *  7) Player is in spell range of the target. 
     *
     *  @param mgr The main PS Spell Manager.
     *  @param client The client that cast the spell. 
     *  @param effectName [CHANGES] Filled in with this spell's effect.
     *  @param offset [CHANGES] Filled in with the offset( ie how off target ) this spell is.
     *  @param anchorID [CHANGES] The entity that the spell should be attached to ( in case of movement )
     *  @param targetID [CHANGES] Filled in with the ID of the target. 
     *  @param castingDuration [CHANGES] Filled in by the time it takes to cast spell.
     *  @param castingText [CHANGES] Filled in with the text that should be sent to caster.
     *
     *  @return An array of new psSpellCastGameEvents that are ready to be pushed inot the event stream.
     */
    psSpellCastGameEvent *Cast(SpellManager * mgr, Client * client, csString &effectName, csVector3 &offset,
                           EID & anchorID, EID & targetID, unsigned int & castingDuration, csString & castingText) const;
        
    /** Find all objects in range for spell around caster
     *
     *  @param client The client that cast the spell. 
     *  @param max_range The maximum range for this spell.
     *  @param range
     *
     *  @return An array of objects in range for spell
     */
    csArray< gemObject *> *getTargetsInRange(Client * client, float max_range, float range) const;
    
    bool AffectTargets(SpellManager * mgr,psSpellCastGameEvent * event, csString &effectName, csVector3 &offset, 
                EID & anchorID, EID & targetID, csString & affectText) const;
    bool AffectTarget( psSpellCastGameEvent * event, csString &effectName, csVector3 &offset,
                       EID & anchorID, EID & targetID, csString & affectText) const;
    bool PerformResult(gemActor *caster, gemObject *target, float max_range, bool saved, float powerLevel, csTicks duration = 0) const;

    csString SpellToXML() const;
    csString DescriptionToXML() const;

    // Needed for the BinaryTree
    bool operator==(const psSpell& other) const;
    bool operator<(const psSpell& other) const;
    
    int GetRealm() { return realm; }
    psWay* GetWay() { return way; }
    csArray<psItemStats*>& GetGlyphList() { return glyphList; }

    /// iScriptableVar Implementation
    /// This is used by the math scripting engine to get various values.
    double GetProperty(const char *ptr);
    double CalcFunction(const char * functionName, const double * params);

protected:
    bool isTargetAffected(Client *client, gemObject *target, float max_range, csString & castingText) const;
    int checkRange( gemActor *caster, gemObject *target, float max_range) const;

    // Returns mathscript variable with given name - when there is no such variable, returns NULL and writes error into log
    MathScriptVar * GetScriptVar(const char * varName);

    int id;
    csString name;
    psWay* way;
    int realm;
    csArray<psItemStats*> glyphList;
    bool offensive;     //is casting of this spell restricted by PvP system ?

    /// The Power cap this spell has.
    int max_power;
    
    /// Casting paramters
    csString caster_effect;  // Any visual/sound responses on the caster for casting
                               // this spell. It is defined in an XML on the client.
                               
    //csTicks casting_duration; 
    
    /// Used for spells like 4HP every 15 Seconds. 
    //csTicks interval_time; 
    //csTicks spellDuration;
    
    // bit field if valid target types for this spell
    int spell_target;
    csString target_effect;  // Any visual/sound responses on the target for casting
                               // this spell. It is defined in an XML on the client. 
    
                               
    MathScript *mathScript;
    MathScript *manaScript;
    MathScript *castSuccessScript;
    MathScriptVar *varPowerLevel;
    MathScriptVar *varAntiMagic;
    MathScriptVar *varRange;
    MathScriptVar *varDuration;
    MathScriptVar *varCastingDuration;
    MathScriptVar *varUseSaveThrow;
    MathScriptVar *varAffectRange;
    MathScriptVar *varAffectAngle;
    MathScriptVar *varAffectTypes;
    MathScriptVar *varProgressionDelay;
    MathScriptVar *varWaySkill;
    csArray<MathScriptVar*> varParams;
                                   
    csString image;
    csString description;
    
    /** What this spell should actually do (Result).  This script event is 
        fired off when the spell is cast.
       */
    csString progression_event; 
    csString saved_progression_event;     

    /// Name of category of spell, which will sent to npc perception system
    csString npcSpellCategory;

    /// Hash ID of category of spell, use in network compression to npc perception system
    uint32_t npcSpellCategoryID;

    /// Relative Power of spell, used as a hint to npc perception system
    float    npcSpellRelativePower;

    struct SavingThrow
    {        
        PSITEMSTATS_STAT  statSave;
        PSSKILL           skillSave;  
        int value;
    };
    SavingThrow *saveThrow;            
};

#endif
