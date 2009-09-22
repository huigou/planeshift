/*
 * pstrade.h
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

#ifndef __PSTRADE_H__
#define __PSTRADE_H__
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


class psTradeCombinations;

//-----------------------------------------------------------------------------

/**
 * This class holds the master list of all trade combinations possible in the game.
 */
class psTradeCombinations
{
 public:
    psTradeCombinations();
    ~psTradeCombinations();

    bool Load(iResultRow& row);

    uint32 GetId() const { return id; }
    uint32 GetPatternId() const { return patternId; }
    uint32 GetResultId() const { return resultId; }
    int GetResultQty() const { return resultQty; }
    uint32 GetItemId() const { return itemId; }
    int GetMinQty() const { return minQty; }
    int GetMaxQty() const { return maxQty; }

 protected:
    uint32 id;
    uint32 patternId;
    uint32 resultId;
    int resultQty;
    uint32 itemId;
    int minQty;
    int maxQty;
};

/**
 * This class holds the master list of all trade transformatations possible in the game.
 * This class is read only since it is cached and shared by multiple users.
 */
class psTradeTransformations
{
 public:
    psTradeTransformations();
    psTradeTransformations(uint32 rId, int rQty, uint32 iId, int iQty, int tPoints);
    ~psTradeTransformations();

    bool Load(iResultRow& row);

    uint32 GetId() const { return id; }
    uint32 GetPatternId() const { return patternId; }
    uint32 GetProcessId() const { return processId; }
    uint32 GetResultId() const { return resultId; }

    int GetResultQty() const { return resultQty; }

    uint32 GetItemId() const { return itemId; }
    void SetItemId(uint32 newItemId) { itemId = newItemId; }

    int GetItemQty() const { return itemQty; }
    void SetItemQty(int newItemQty) { itemQty = newItemQty; }

    float GetItemQualityPenaltyPercent() const { return penaltyPct; }

    int GetTransPoints() const { return transPoints; }

    // Cache flag is used for garbage collection
    // If true transformation is cached and should not be deleted after use
    //  otherwise it needs to be cleaned up
    int GetTransformationCacheFlag() { return transCached; }

 protected:
    uint32 id;
    uint32 patternId;
    uint32 processId;
    uint32 resultId;
    int resultQty;
    uint32 itemId;
    int itemQty;
    float penaltyPct;
    int transPoints;

private:
    // Cache flag is used for garbage collection
    // If true transformation is cached and should not be deleted after use
    //  otherwise it needs to be cleaned up
    bool transCached;
};

/**
 * This class holds the master list of all trade processes possible in the game.
 * This class is read only since it is cached and shared by multiple users.
 */
class psTradeProcesses
{
 public:
    psTradeProcesses();
    ~psTradeProcesses();

    bool Load(iResultRow& row);

    uint32 GetProcessId() const { return processId; }

    csString GetName() const { return name; }
    csString GetAnimation() const { return animation; }
    uint32 GetWorkItemId() const { return workItemId; }
    uint32 GetEquipementId() const { return equipmentId; }
    const char* GetConstraintString() const { return constraints; }
    uint32 GetGarbageId() const { return garbageId; }
    int GetGarbageQty() const { return garbageQty; }
    int GetPrimarySkillId() const { return priSkillId; }
    unsigned int GetMinPrimarySkill() const { return minPriSkill; }
    unsigned int GetMaxPrimarySkill() const { return maxPriSkill; }
    int GetPrimaryPracticePts() const { return priPracticePts; }
    int GetPrimaryQualFactor() const { return priQualFactor; }
    int GetSecondarySkillId() const { return secSkillId; }
    unsigned int GetMinSecondarySkill() const { return minSecSkill; }
    unsigned int GetMaxSecondarySkill() const { return maxSecSkill; }
    int GetSecondaryPracticePts() const { return secPracticePts; }
    int GetSecondaryQualFactor() const { return secQualFactor; }
    csString& GetRenderEffect() { return renderEffect; }
    
 protected:
    uint32 processId;
    int subprocess;
    csString name;
    csString animation;
    uint32 workItemId;
    uint32 equipmentId;
    csString constraints;
    uint32 garbageId;
    int garbageQty;
    int priSkillId;
    int minPriSkill;
    int maxPriSkill;
    int priPracticePts;
    int priQualFactor;
    int secSkillId;
    int minSecSkill;
    int maxSecSkill;
    int secPracticePts;
    int secQualFactor;
    csString renderEffect;    
};

/**
 * This class holds the master list of all trade patterns possible in the game.
 */
class psTradePatterns
{
 public:
    psTradePatterns();
    ~psTradePatterns();
    bool Load(iResultRow& row);

    uint32 GetId() const { return id; }
    uint32 GetGroupPatternId() const { return groupPatternId; }
    const char* GetPatternString() const { return patternName; }
    uint32 GetDesignItemId() const { return designItemId; }
    float GetKFactor() const { return KFactor; }

 protected:
    uint32 id;
    uint32 groupPatternId;
    csString patternName;
    uint32 designItemId;
    float KFactor;
};

//-----------------------------------------------------------------------------

/** Each item has a list of items required for its construction.*/
struct CombinationConstruction
{
    uint32 resultItem;     
    int resultQuantity;                             
    csPDelArray<psTradeCombinations> combinations;
};

//-----------------------------------------------------------------------------

/** Each item contains the craft skills for the craft step.*/
struct CraftSkills
{
    int priSkillId;
    int minPriSkill;
    int secSkillId;
    int minSecSkill;
};

/** Each item contains craft information about a craft transformation step.*/
struct CraftTransInfo
{
    int priSkillId;
    int minPriSkill;
    int secSkillId;
    int minSecSkill;
    csString craftStepDescription;
};

/** Each item contains craft information about a craft combination.*/
struct CraftComboInfo
{
    csArray<CraftSkills*>* skillArray;
    csString craftCombDescription;
};


#endif
