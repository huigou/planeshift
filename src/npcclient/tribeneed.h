/*
* tribeneed.h
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __TRIBENEED_H__
#define __TRIBENEED_H__

//=============================================================================
// Local Includes
//=============================================================================
#include "tribe.h"

class psTribeNeedSet;
class psTribe;
class NPC;

/**
 * Represent the base class for all tribe need types.
 */
class psTribeNeed 
{
public:
    psTribeNeedSet         *parentSet;    ///< Point to the need set that this need is part of
    psTribe::TribeNeedType  needType;     ///< Set by each need type to one of the tribe needs
    float                   current_need; ///< Represent current need. Will be used to sort each need.
    csString                needName;     ///< Name of need

    /**
     * Construct a basic need with the given needType and needName for debuging.
     */
    psTribeNeed(psTribe::TribeNeedType needType, csString name, csString perception,
                float needStartValue, float needGrowthValue)
        :parentSet(NULL),needType(needType),current_need(0.0),needName(name), perception(perception),
         needStartValue(needStartValue), needGrowthValue(needGrowthValue)
    {
        ResetNeed();
    }

    virtual ~psTribeNeed() {};

    /**
     * @return The tribe this need is a part of
     */
    psTribe * GetTribe() const;

    /**
     * Set the parent need. Called when a need is added to a set.
     */
    void SetParent(psTribeNeedSet * parent)
    {
        parentSet = parent;
    }

    /**
     * To be overloaded to calculate the need for each need type.
     */
    virtual void UpdateNeed(NPC * npc)
    {
    }

    /**
     * Called when a need is selected for an npc.
     */
    virtual void ResetNeed()
    {
        current_need = needStartValue;
    }

    /**
     * By default the current_need value is returned. To be overloaded by
     * specefic needs if there are gate conditions for returning the need.
     */
    virtual float GetNeedValue(NPC * npc)
    {
        return current_need;
    }

    /**
     * Get the need that is needed if this need is the most needed.
     * To be overloded if a need is selected and that need depend on some
     * other need before it can be used.
     */
    virtual psTribe::TribeNeedType GetNeedType()
    {
        return needType;
    }

    /**
     * Get the need that is needed if this need is the most needed.
     * To be overloded if a need is selected and that need depend on some
     * other need before it can be used.
     */
    virtual psTribeNeed* GetNeed()
    {
        return this;
    }

    /**
     * Get a sting with both type and name
     */
    csString GetTypeAndName() const;

    /**
     * Get the perception
     * Can be overriden if the perception should be dynamic
     */
    virtual csString GetPerception() const
    {
        return perception;
    }
    
    /**
     * Get need start value
     */
    float GetNeedStartValue() const 
    {
        return needStartValue;
    }

    /**
     * Get need growth value
     */
    float GetNeedGrowthValue() const 
    {
        return needGrowthValue;
    }
    
protected:
    csString                perception;   ///< The perception to use when executing this need
    float needStartValue;
    float needGrowthValue;
    
};

/**
 * Hold a collection of needs and calculate the highest need for the tribe.
 */
class psTribeNeedSet
{
public:

    /**
     * Iterator type
     */
    typedef csArray<psTribeNeed*>::Iterator NeedIterator;
    
    /**
     * Construct a need set for the given tribe.
     */
    psTribeNeedSet(psTribe * tribe)
        :tribe(tribe)
    {
    }

    /**
     * Called once before calculate need to update the need of all
     * needs part of the set.
     */
    void UpdateNeed(NPC * npc);

    /**
     * Will sort each need and select the highest ranking need to be returned.
     */
    psTribeNeed* CalculateNeed(NPC * npc);

    /**
     * Set the named need to the most needed need.
     */
    void MaxNeed(const csString& needName);

    /**
     * @return the tribe for the need set.
     */
    psTribe * GetTribe() const { return tribe; };

    /**
     * Add a new need to the need set.
     */
    void AddNeed(psTribeNeed * newNeed);

    /**
     * Find a Need by name
     */
    psTribeNeed* Find(const csString& needName) const;

    /**
     * Return iterator for needs
     */
    NeedIterator GetIterator(){ return needs.GetIterator(); };

private:
    psTribe               *tribe;
    csArray<psTribeNeed*>  needs;
};

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

class psTribeNeedGeneric : public psTribeNeed
{
public:
    psTribeNeedGeneric(const csString& name, const csString& perception,
                       float needStartValue, float needGrowthValue )
        :psTribeNeed(psTribe::GENERIC, name, perception, needStartValue, needGrowthValue)
    {
    }

    virtual ~psTribeNeedGeneric()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        current_need += needGrowthValue;
    }
    
private:

};

// ---------------------------------------------------------------------------------

class psTribeNeedResourceArea : public psTribeNeed
{
public:
    
    psTribeNeedResourceArea(const csString& name, const csString& perception,
                            float needStartValue, float needGrowthValue,
                            psTribeNeed * explore)
        :psTribeNeed(psTribe::RESOURCE_AREA,name,perception,needStartValue,needGrowthValue),
         explore(explore)
    {
    }

    virtual ~psTribeNeedResourceArea()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        if (!GetTribe()->CanGrow())
        {
            current_need += 10*needGrowthValue; // Make sure tribe can grow at all time
        }
        else
        {
            current_need += needGrowthValue;
        }
    }

    virtual psTribeNeed* GetNeed()
    {
        if (GetTribe()->FindMemory(GetTribe()->GetNeededResourceAreaType()))
        {
            return this;
        }
        else
        {
            return explore->GetNeed();
        }
    }
private:
    psTribeNeed * explore;

};


// ---------------------------------------------------------------------------------


/** TribeNeedReproduce govern the need for reproduction
 *
 *  Will increase in need as long as the tribe hasn't reached the maximum size.
 *  If the tribe hasn't the resources needed to grow it will call the dependend
 *  get_resource_need.
 */

class psTribeNeedReproduce : public psTribeNeed
{
public:
    
    psTribeNeedReproduce(const csString& name, const csString& perception,
                         float needStartValue, float needGrowthValue,
                         psTribeNeed * get_resource_need)
        :psTribeNeed(psTribe::REPRODUCE,name,perception,needStartValue,needGrowthValue),
         get_resource_need(get_resource_need)
    {
    }

    virtual ~psTribeNeedReproduce()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        if (GetTribe()->ShouldGrow())
        {
            current_need += needGrowthValue;
        } else
        {
            current_need = 0.0;
        }
    }

    virtual psTribeNeed* GetNeed()
    {
        if (GetTribe()->CanGrow())
        {
            return this;
        }
        else
        {
            return get_resource_need->GetNeed();
        }
    }

 private:
    psTribeNeed * get_resource_need;
};

#endif
