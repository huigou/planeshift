/*
 * lootrandomizer.h by Stefano Angeleri
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
 */

#ifndef __LOOTRANDOMIZER_H__
#define __LOOTRANDOMIZER_H__

class RandomizedOverlay;

/**
 * This class holds one loot modifier
 * The lootRandomizer contions arrays of these
 */
struct LootModifier
{
    uint32_t id; ///< The id assigned to this modifier in the db. It's referenced by psitem.
    csString modifier_type;
    csString name;
    csString effect;
    csString equip_script;
    float    probability;
    csString stat_req_modifier;
    float    cost_modifier;
    csString mesh;
    csString not_usable_with;
};

class MathScript;
/**
 * This class stores an array of LootModifiers and randomizes
 * loot stats.
 */
class LootRandomizer
{
protected:
    csArray<LootModifier*> prefixes;
    csArray<LootModifier*> suffixes;
    csArray<LootModifier*> adjectives;

    // precalculated max values for probability. min is always 0
    float prefix_max;
    float adjective_max;
    float suffix_max;

public:
    LootRandomizer(CacheManager* cachemanager);
    ~LootRandomizer();

    /// This adds another item to the entries array
    void AddLootModifier( LootModifier *entry );
    
    LootModifier *GetModifier(uint32_t id);

    /// This randomizes the current loot item and returns the item with the modifiers applied
    psItem* RandomizeItem( psItem* item,
                                float cost,
                                bool lootTesting = false,
                                size_t numModifiers = 0 );

    float CalcModifierCostCap(psCharacter *chr);
    void ApplyModifier(psItemStats* baseItem, RandomizedOverlay* overlay, csArray<uint32_t>& modifiersIds);

protected:
    MathScript* modifierCostCalc;
    CacheManager* cacheManager;

private:
    void AddModifier( LootModifier *oper1, LootModifier *oper2 );
    /** sets an attribute to the item overlay. utility function used when parsing the loot modifiers xml */
    bool SetAttribute(const csString & op, const csString & attrName, float modifier, RandomizedOverlay* overlay);
   // void ApplyModifier(psItemStats *loot, LootModifier *mod);
};

#endif

