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
    uint32_t id;                ///< The id assigned to this modifier in the db. It's referenced by psitem.
    csString modifier_type;     ///< The type of modifier (suffix, prefix, adjective)
    csString name;              ///< The part of name which this modifier will add when used. The position is determined by modifier_type
    csString effect;            ///< Declares modifiers to some stats like weight, weapon speed, resistance/attack types.
    csString equip_script;      ///< The part of equip_script to add to the item when this modifier is used.
    float    probability;       ///< The probability for this loot modifier to happen when generating a random item.
    csString stat_req_modifier; ///< The additional requirements to stats when this modifier is used.
    float    cost_modifier;     ///< The cost of this modifier @see CalcModifierCostCap
    csString mesh;              ///< The mesh this modifier will use for the random item generated.
    csString icon;              ///< The icon this modifier will use for the random item generated.
    csString not_usable_with;   ///< Defines which modifiers this isn't usable with.
};

class MathScript;
/**
 * This class stores an array of LootModifiers and randomizes
 * loot stats.
 */
class LootRandomizer
{
protected:
    csArray<LootModifier*> prefixes;   ///< Keeps all the loot modifiers of type "prefix"
    csArray<LootModifier*> suffixes;   ///< Keeps all the loot modifiers of type "suffix"
    csArray<LootModifier*> adjectives; ///< Keeps all the loot modifiers of type "adjective"
    csHash<LootModifier*, uint32_t> LootModifiersById;  ///< Keeps all the lootmodifiers for faster access when we know the id.

    // precalculated max values for probability. min is always 0
    float prefix_max;
    float adjective_max;
    float suffix_max;

public:
    /** Constructor.
     *  @param cachemanager A pointer to the cache manager.
     */
    LootRandomizer(CacheManager* cachemanager);
    ///Destructor
    ~LootRandomizer();

    /// This adds another item to the entries array
    void AddLootModifier( LootModifier *entry );

    /** Gets a loot modifier from it's id.
     *  @param id The id of the item we are searching for.
     *  @return A pointer to the loot modifier which is referenced by the id we were searching for.
     */
    LootModifier *GetModifier(uint32_t id);

    /** This randomizes the current loot item and returns the item with the modifiers applied.
     *  @param item The item instance which we will be randomizing.
     *  @param cost The maximum "cost" of the randomization we can apply @see CalcModifierCostCap
     *  @param lootTesting Says if we really are applying the modifiers.
     *  @param numModifiers Forces the amount of modifiers to apply.
     */
    psItem* RandomizeItem( psItem* item,
                                float cost,
                                bool lootTesting = false,
                                size_t numModifiers = 0 );

    float CalcModifierCostCap(psCharacter *chr);
    /** Applies modifications to a randomized overlay depending on the requested ids.
     *  @param baseItem The basic item which will have the overlay generated for.
     *  @param overlay A pointer to the overlay where we will save the modifications to apply to this item.
     *  @param modifiersIds An array with all the ids of the modifiers which we will need to apply to the overlay.
     */
    void ApplyModifier(psItemStats* baseItem, RandomizedOverlay* overlay, csArray<uint32_t>& modifiersIds);

protected:
    MathScript* modifierCostCalc;
    CacheManager* cacheManager;

private:
    void AddModifier( LootModifier *oper1, LootModifier *oper2 );
    /** sets an attribute to the item overlay. utility function used when parsing the loot modifiers xml
     *  @param op The operation to do with the attributes. (+, -, *)
     *  @param attrName The name of the attribute we are changing.
     *  @param modifier The amount to change of the attribute (right operand, left operand is the basic attribute)
     *  @param overlay The randomization overlay where we are applying these attributes.
     *  @param baseItem The base item of the item we are applying these attributes to
     */
    bool SetAttribute(const csString & op, const csString & attrName, float modifier, RandomizedOverlay* overlay, psItemStats* baseItem);
};

#endif

