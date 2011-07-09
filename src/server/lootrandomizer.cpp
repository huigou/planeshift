/*
 * lootrandomizer.cpp by Stefano Angeleri
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
 *
 */


#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psitem.h"
#include "bulkobjects/psitemstats.h"

#include "util/mathscript.h"
#include "util/psconst.h"
#include "util/serverconsole.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cachemanager.h"
#include "psserver.h"
#include "globals.h"
#include "lootrandomizer.h"
#include "scripting.h"

LootRandomizer::LootRandomizer(CacheManager* cachemanager)
{
    prefix_max = 0;
    adjective_max = 0;
    suffix_max = 0;
    cacheManager = cachemanager;

    // Find any math scripts that are needed
    modifierCostCalc = psserver->GetMathScriptEngine()->FindScript("LootModifierCostCap");
}

LootRandomizer::~LootRandomizer()
{
    LootModifier *e;
    while (prefixes.GetSize())
    {
        e = prefixes.Pop();
        delete e;
    }
    while (suffixes.GetSize())
    {
        e = suffixes.Pop();
        delete e;
    }
    while (adjectives.GetSize())
    {
        e = adjectives.Pop();
        delete e;
    }
}

void LootRandomizer::AddLootModifier(LootModifier *entry)
{
    if (entry->modifier_type.CompareNoCase("prefix"))
    {
        prefixes.Push( entry );
        if (entry->probability > prefix_max)
        {
            prefix_max = entry->probability;
        }
    }
    else if (entry->modifier_type.CompareNoCase("suffix"))
    {
        suffixes.Push( entry );
        if (entry->probability > suffix_max)
        {
            suffix_max = entry->probability;
        }
    }
    else if (entry->modifier_type.CompareNoCase("adjective"))
    {
        adjectives.Push( entry );
        if (entry->probability > adjective_max)
        {
            adjective_max = entry->probability;
        }
    }
    else
    {
        Error2("Unsupported modifier type %s", entry->modifier_type.GetData());
        return;
    }

    //put the lootmodifier in an hash for a faster access when we just need to look it up by id.
    LootModifiersById.Put(entry->id, entry);
}

psItem* LootRandomizer::RandomizeItem( psItem* item, float maxcost, bool lootTesting, size_t numModifiers )
{
    uint32_t rand;
    csString modifierType;
    int modifierTypePos;
    csArray< csString > selectedModifierTypes;
    float totalCost = item->GetBaseStats()->GetPrice().GetTrias();

    // Set up ModifierTypes
    // The Order of the modifiers is significant. It determines the priority of the modifiers, currently this is
    // Suffixes, Prefixes, Adjectives : So we add them in reverse order so the highest priority is applied last
    selectedModifierTypes.Push( "suffix" );
    selectedModifierTypes.Push( "prefix" );
    selectedModifierTypes.Push( "adjective" );

    // Determine Probability of number of modifiers ( 0-3 )
    if (!lootTesting)
    {
        rand = psserver->rng->Get( 100 ); // Range of 0 - 99
        if ( rand < 1 ) // 1% chance
            numModifiers = 3;
        else if ( rand < 8 ) // 7% chance
            numModifiers = 2;
        else if ( rand < 30 ) // 22% chance
            numModifiers = 1;
        else // 70% chance
            numModifiers = 0;
    }

    // If there are no additional modifiers return original stats
    if ( numModifiers == 0 )
        return item;

    if ( numModifiers != 3 )
    {
        while ( selectedModifierTypes.GetSize() != numModifiers )
        {
            rand = psserver->rng->Get( 99 );
            if ( rand < 60 )
                selectedModifierTypes.Delete( "suffix" ); // higher chance to be removed
            else if ( rand < 85 )
                selectedModifierTypes.Delete( "prefix" );
            else
                selectedModifierTypes.Delete( "adjective" ); // lower chance to be removed
        }
    }

  // for each modifiertype roll a dice to see which modifier we get
    while ( selectedModifierTypes.GetSize() != 0 )
    {
        modifierType = selectedModifierTypes.Pop();
        int newModifier, probability;
        int max_probability = 0;
        LootModifier *lootModifier = NULL;
        csArray<LootModifier *> *modifierList = NULL;

        if (modifierType.CompareNoCase("prefix"))
        {
            modifierList = &prefixes;
            max_probability=(int)prefix_max;
            modifierTypePos = 0;
        }
        else if (modifierType.CompareNoCase("suffix"))
        {
            modifierList = &suffixes;
            max_probability=(int)suffix_max;
            modifierTypePos = 1;
        }
        else if (modifierType.CompareNoCase("adjective"))
        {
            modifierList = &adjectives;
            max_probability=(int)adjective_max;
            modifierTypePos = 2;
        }
        else
        {
            //this shouldn't be reached
            Error1("BUG in randomizeItem(). Found an unsupported modifier");
            continue;
        }

        // Get min probability <= probability <= max probability in modifiers list
        //probability = psserver->rng->Get( (int)((*modifierList)[ modifierList->Length() - 1 ]->probability - (int) (*modifierList)[0]->probability ) + 1) + (int) (*modifierList)[0]->probability;
        probability = psserver->rng->Get( max_probability );
        for ( newModifier = (int)modifierList->GetSize() - 1; newModifier >= 0 ; newModifier-- )
        {
            float item_prob = ((*modifierList)[newModifier]->probability);
            if ( probability >=  item_prob)
            {
                if ( maxcost >= totalCost * (*modifierList)[newModifier]->cost_modifier ||
                    lootTesting )
                {
                    lootModifier = (*modifierList)[ newModifier ];
                    totalCost = totalCost * (*modifierList)[newModifier]->cost_modifier;
                    break;
                }
            }
        }

        // if just testing loot randomizing, then dont want equip/dequip events
        //if (lootTesting && lootModifier)
        //{
        //    lootModifier->prg_evt_equip.Empty();
        //}

        //add this modifier to the item
        if(lootModifier) item->AddLootModifier(lootModifier->id, modifierTypePos);
    }
    //regenerate the modifiers cache of the item
    item->UpdateModifiers();
    return item;
}

void LootRandomizer::AddModifier( LootModifier *oper1, LootModifier *oper2 )
{
    csString newName;
    // Change Name
    if (oper2->modifier_type.CompareNoCase("prefix"))
    {
        newName.Append( oper2->name );
        newName.Append( " " );
        newName.Append( oper1->name );
    }
    else if (oper2->modifier_type.CompareNoCase("suffix"))
    {
        newName.Append( oper1->name );
        newName.Append( " " );
        newName.Append( oper2->name );
    }
    else if (oper2->modifier_type.CompareNoCase("adjective"))
    {
        newName.Append( oper2->name );
        newName.Append( " " );
        newName.Append( oper1->name );
    }
    oper1->name = newName;

    // Adjust Price
    oper1->cost_modifier *= oper2->cost_modifier;
    oper1->effect.Append( oper2->effect );
    oper1->mesh = oper2->mesh;
    oper1->icon = oper2->icon;
    oper1->stat_req_modifier.Append( oper2->stat_req_modifier );

    // equip script
    oper1->equip_script.Append(oper2->equip_script);
}

bool LootRandomizer::SetAttribute(const csString & op, const csString & attrName, float modifier, RandomizedOverlay* overlay, psItemStats* baseItem)
{
    float* value[3] = { NULL, NULL, NULL };
    // Attribute Names:
    // item
    //        weight
    //        damage
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        protection
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        speed

    csString AttributeName(attrName);
    AttributeName.Downcase();
    if ( AttributeName.Compare( "item.weight" ) )
    {
        overlay->weight = baseItem->GetWeight();
        value[0] = &overlay->weight;
    }
    else if ( AttributeName.Compare( "item.speed" ) )
    {
        overlay->latency = baseItem->Weapon().Latency();
        value[0] = &overlay->latency;
    }
    else if ( AttributeName.Compare( "item.damage" ) )
    {
        for(int i = 0; i < 3; i++)
            overlay->damageStats[i] = baseItem->Weapon().Damage((PSITEMSTATS_DAMAGETYPE)i);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.damage.slash" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_SLASH);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if ( AttributeName.Compare( "item.damage.pierce" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_PIERCE);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.damage.blunt" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_BLUNT);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }
    else if ( AttributeName.Compare( "item.protection" ) )
    {
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
        for(int i = 0; i < 3; i++)
            overlay->damageStats[i] = baseItem->Armor().Protection((PSITEMSTATS_DAMAGETYPE)i);
    }
    else if ( AttributeName.Compare( "item.protection.slash" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_SLASH);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if ( AttributeName.Compare( "item.protection.pierce" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_PIERCE);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if ( AttributeName.Compare( "item.protection.blunt" ) )
    {
        overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_BLUNT);
        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }

    // Operations  = ADD, MUL, SET
    for (int i = 0; i < 3; i++)
    {
        if (op.CompareNoCase("ADD"))
        {
            if (value[i]) *value[i] += modifier;
        }

        if (op.CompareNoCase("MUL"))
        {
            if (value[i]) *value[i] *= modifier;
        }

        if (op.CompareNoCase("VAL"))
        {
            if (value[i]) *value[i] = modifier;
        }
    }

    return true;
}


void LootRandomizer::ApplyModifier(psItemStats* baseItem, RandomizedOverlay* overlay, csArray<uint32_t>& modifierIds)
{
    LootModifier mod;

    //set up default mod data
    mod.cost_modifier = 1;
    mod.name = baseItem->GetName();

    //creates the full lootmodifier from the ids being applied.
    //0 should be left empty in the database as it acts as NO HIT.
    for(size_t i = 0; i < modifierIds.GetSize(); i++)
    {
        uint32_t modID = modifierIds.Get(i);
        if(modID) //0 means nothing to do, no need to search for the void.
        {
            LootModifier *partialModifier = GetModifier(modID);
            if(partialModifier)
            {
                overlay->active = true;
                AddModifier(&mod,partialModifier);
            }
        }
    }
    //all is done no modifiers where found. so we just get out
    if(overlay->active == false)
        return;

    overlay->name = mod.name;
    overlay->price = psMoney(baseItem->GetPrice().GetTrias() * mod.cost_modifier);
    if ( mod.mesh.Length() > 0 )
        overlay->mesh = mod.mesh;
    if ( mod.icon.Length() > 0 )
        overlay->icon = mod.icon;

    // Apply effect
    csString xmlItemMod;

    xmlItemMod.Append( "<ModiferEffects>" );
    xmlItemMod.Append( mod.effect );
    xmlItemMod.Append( "</ModiferEffects>" );

    // Read the ModiferEffects XML into a doc*/
    csRef<iDocument> xmlDoc = ParseString( xmlItemMod );
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("ModiferEffects");//Are we sure it is "Modifer"?
    if(!topNode)
    {
        Error1("No <ModiferEffects> in Loot Randomizer");
        return;
    }

    csRef<iDocumentNodeIterator> nodeList = topNode->GetNodes("ModiferEffect");

    // For Each ModiferEffect
    csRef<iDocumentNode> node;
    while ( nodeList->HasNext() )
    {
        node = nodeList->Next();
        //Determine the Effect
        csString EffectOp = node->GetAttribute("operation")->GetValue();
        csString EffectName = node->GetAttribute("name")->GetValue();
        float EffectValue = node->GetAttribute("value")->GetValueAsFloat();
        //Add to the Attributes
        if (!SetAttribute(EffectOp, EffectName, EffectValue, overlay, baseItem))
        {
            // display error and continue
            Error2("Unable to set attribute %s on new loot item.",EffectName.GetData());
        }
    }

    // Apply stat_req_modifier
    csString xmlStatReq;

    xmlStatReq.Append( "<StatReqs>" );
    xmlStatReq.Append( mod.stat_req_modifier );
    xmlStatReq.Append( "</StatReqs>" );

    // Read the Stat_Req XML into a doc
    xmlDoc = ParseString( xmlStatReq );
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    topNode = root->GetNode("StatReqs");
    if(!topNode)
    {
        Error1("No <statreqs> in Loot Randomizer");
        return;
    }

    nodeList = topNode->GetNodes("StatReq");
    // For Each Stat_Req
    while ( nodeList->HasNext() )
    {
        node = nodeList->Next();
        //Determine the STAT
        ItemRequirement req;
        req.name = node->GetAttribute("name")->GetValue();
        req.min_value = node->GetAttribute("value")->GetValueAsFloat();
        //Add to the Requirements
        overlay->reqs.Push(req);
    }

    // Apply equip script
    if (!mod.equip_script.IsEmpty())
    {
        csString scriptXML;
        scriptXML.Format("<apply aim=\"Actor\" name=\"%s\" type=\"buff\">%s</apply>", mod.name.GetData(), mod.equip_script.GetData());
        overlay->equip_script = ApplicativeScript::Create(psserver->entitymanager, psserver->GetCacheManager(), scriptXML);
    }
    
    //clamp speed at 1.5s to keep consistency with item_stats
    if(!CS::IsNaN(overlay->latency) && overlay->latency < 1.5F)
        overlay->latency = 1.5F;
}

LootModifier *LootRandomizer::GetModifier(uint32_t id)
{
    return LootModifiersById.Get(id, NULL);
}

float LootRandomizer::CalcModifierCostCap(psCharacter *chr)
{
    // Use LootCostCap script to calculate loot modifier cost cap
    if( !modifierCostCalc )
    {
        CPrintf(CON_ERROR,"Couldn't load loot cost cap script!");
        return 1000.0;
    }

    // Use the mob's attributes to calculate modifier cost cap
    MathEnvironment env;
    env.Define("Str",     chr->GetSkillRank(PSSKILL_STR).Current());
    env.Define("End",     chr->GetSkillRank(PSSKILL_END).Current());
    env.Define("Agi",     chr->GetSkillRank(PSSKILL_AGI).Current());
    env.Define("Int",     chr->GetSkillRank(PSSKILL_INT).Current());
    env.Define("Will",    chr->GetSkillRank(PSSKILL_WILL).Current());
    env.Define("Cha",     chr->GetSkillRank(PSSKILL_CHA).Current());
    env.Define("MaxHP",   chr->GetMaxHP().Current());
    env.Define("MaxMana", chr->GetMaxMana().Current());

    modifierCostCalc->Evaluate(&env);
    MathVar *modcap = env.Lookup("ModCap");
    Debug2(LOG_LOOT,0,"DEBUG: Calculated cost cap %f\n", modcap->GetValue());
    return modcap->GetValue();
}
