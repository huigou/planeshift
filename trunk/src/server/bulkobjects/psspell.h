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
#include "iserver/idal.h"
#include "util/scriptvar.h"
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"
#include "psitemstats.h"
#include "deleteobjcallback.h"

class Client;
class gemObject;
class gemActor;
class ProgressionScript;
class MathExpression;

struct psWay
{
    unsigned int     id;
    PSSKILL          skill;    // for example, Crystal Way
    PSITEMSTATS_STAT related_stat; // for example, Charisma
    csString         name;
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

    bool Load(iResultRow& row);
    int GetID() const { return id; }
    const csString& GetName() const { return name; }
    const csString& GetImage() const { return image; }
    const csString& GetDescription() const { return description; }
    float ManaCost(psCharacter *caster, float kFactor) const;
    float PowerLevel(psCharacter *caster, float kFactor) const;
    float ChanceOfCastSuccess(psCharacter *caster, float kFactor) const;
    float ChanceOfResearchSuccess(psCharacter *researcher);

    /** Takes a list of glyphs and compares them to the correct sequence to
      * construct this spell.
      */
    bool MatchGlyphs(const csArray<psItemStats*> & glyphs);

    /** Performs the necessary checks on the player to make sure they meet
     *  the requirements to cast this spell.
     *  1) The character is in PEACE or COMBAT modes.
     *  2) The player has the required glyphs (or "cast all spells" privs)
     *  3) The player has the required mana (or infinitemana set).
     */
    bool CanCast(Client *client, float kFactor, csString & reason);

    /** Creates a new instance of this spell.
     *  @param mgr The main PS Spell Manager.
     *  @param client The client that cast the spell.
     *  @param effectName [CHANGES] Filled in with this spell's effect.
     *  @param offset [CHANGES] Filled in with the offset( ie how off target ) this spell is.
     *  @param anchorID [CHANGES] The entity that the spell should be attached to ( in case of movement )
     *  @param targetID [CHANGES] Filled in with the ID of the target.
     */
    void Cast(Client *client, float kFactor) const;
    void Affect(gemActor *caster, gemObject *target, float range, float kFactor, float power) const;

    int GetRealm() { return realm; }
    psWay* GetWay() { return way; }
    csArray<psItemStats*>& GetGlyphList() { return glyphList; }

    /// iScriptableVar Implementation
    /// This is used by the math scripting engine to get various values.
    double GetProperty(const char *ptr);
    double CalcFunction(const char * functionName, const double * params);
    const char* ToString() { return name.GetDataSafe(); }

protected:
    bool AffectTarget(gemActor* caster, gemObject* origTarget, gemObject* target, float power) const;

    int id;
    csString name;
    psWay *way;
    int realm;
    csString image;
    csString description;
    csString castingEffect;
    bool offensive;

    /// The Power (P) cap.
    int maxPower;

    /// Bit field if valid target types for this spell
    int targetTypes;

    /// Whether or not to exclude the target when AOE Radius > 0.
    bool excludeTarget;

    /// Math for various properties.
    /// Casting duration: (Power, WaySkill, RelatedStat) -> Seconds
    MathExpression *castDuration;
    /// Maximum range to target allowed: (Power, WaySkill, RelatedStat) -> Meters
    MathExpression *range;
    /// AOE Radius: (Power, WaySkill, RelatedStat) -> Meters
    MathExpression *aoeRadius;
    /// AOE Angle: (Power, WaySkill, RelatedStat) -> Degrees
    MathExpression *aoeAngle;
    /// The progression script: (Power, Caster, Target) -> (side effects)
    ProgressionScript *outcome;

    /// List of glyphs required to assemble the technique.
    csArray<psItemStats*> glyphList;

    /// Name of category of spell, which will sent to npc perception system
    csString npcSpellCategory;

    /// Hash ID of category of spell, use in network compression to npc perception system
    uint32_t npcSpellCategoryID;

    /// Relative Power of spell, used as a hint to npc perception system
    float    npcSpellRelativePower;
};

//-----------------------------------------------------------------------------

/**
 * This event actually triggers a spell, after the casting wait time.
 */
class psSpellCastGameEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    Client        *caster; ///< Entity who casting this spell
    gemObject     *target; ///< Entity who is target of this spell
    const psSpell *spell;  ///< The spell that is casted

    float max_range;
    float kFactor;
    float powerLevel;
    csTicks duration;

    psSpellCastGameEvent(const psSpell *spell,
                         Client *caster,
                         gemObject *target,
                         csTicks castingDuration,
                         float max_range,
                         float kFactor,
                         float power);

    ~psSpellCastGameEvent();

    void Interrupt();

    virtual void Trigger();  // Abstract event processing function
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);
};

#endif
