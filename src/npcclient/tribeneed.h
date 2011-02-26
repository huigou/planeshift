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
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>

//=============================================================================
// Local Includes
//=============================================================================

class TribeNeedSet;
class Tribe;
class NPC;

/**
 * Represent the base class for all tribe need types.
 */
class TribeNeed 
{
public:
    /**
     * Represent each type of NPC Needs
     */
    enum TribeNeedType
    {
        DEATH_RATE,
        GENERIC,
        REPRODUCE,
        RESOURCE_AREA,
        RESOURCE_RATE,
        TIME_OF_DAY
    };

    static const char *TribeNeedTypeName[];    
    
    TribeNeedSet*           parentSet;    ///< Point to the need set that this need is part of
    TribeNeedType           needType;     ///< Set by each need type to one of the tribe needs
    float                   current_need; ///< Represent current need. Will be used to sort each need.
    csString                needName;     ///< Name of need

    /**
     * Construct a basic need with the given needType and needName for debuging.
     *
     */
    TribeNeed(TribeNeedType needType, csString name, csString perception,
              float needStartValue, float needGrowthValue)
        :parentSet(NULL),needType(needType),current_need(0.0),needName(name), perception(perception),
         needStartValue(needStartValue), needGrowthValue(needGrowthValue)
    {
        ResetNeed();
    }

    virtual ~TribeNeed() {};

    /**
     * @return The tribe this need is a part of
     */
    Tribe * GetTribe() const;

    /**
     * Set the parent need. Called when a need is added to a set.
     */
    void SetParent(TribeNeedSet * parent)
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
    virtual TribeNeedType GetNeedType()
    {
        return needType;
    }

    /**
     * Get the need that is needed if this need is the most needed.
     * To be overloded if a need is selected and that need depend on some
     * other need before it can be used.
     */
    virtual const TribeNeed* GetNeed() const
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
class TribeNeedSet
{
public:

    /**
     * Iterator type
     */
    typedef csPDelArray<TribeNeed>::Iterator NeedIterator;
    typedef csPDelArray<TribeNeed>::ConstIterator ConstNeedIterator;
    
    /**
     * Construct a need set for the given tribe.
     */
    TribeNeedSet(Tribe * tribe)
        :tribe(tribe)
    {
    }

    /** Destructor
     */
    virtual ~TribeNeedSet()
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
    TribeNeed* CalculateNeed(NPC * npc);

    /**
     * Set the named need to the most needed need.
     */
    void MaxNeed(const csString& needName);

    /**
     * @return the tribe for the need set.
     */
    Tribe * GetTribe() const { return tribe; };

    /**
     * Add a new need to the need set.
     */
    void AddNeed(TribeNeed * newNeed);

    /**
     * Find a Need by name
     */
    TribeNeed* Find(const csString& needName) const;

    /**
     * Return iterator for needs
     */
    NeedIterator GetIterator(){ return needs.GetIterator(); };

    /**
     * Return const iterator for needs
     */
    ConstNeedIterator GetIterator() const{ return needs.GetIterator(); };

private:
    Tribe*                    tribe;
    csPDelArray<TribeNeed>    needs;
};

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

class TribeNeedGeneric : public TribeNeed
{
public:
    TribeNeedGeneric(const csString& name, const csString& perception,
                     float needStartValue, float needGrowthValue )
        :TribeNeed(GENERIC, name, perception, needStartValue, needGrowthValue)
    {
    }

    virtual ~TribeNeedGeneric()
    {
    }

    virtual void UpdateNeed(NPC * npc);
private:

};

// ---------------------------------------------------------------------------------

class TribeNeedResourceArea : public TribeNeed
{
public:
    
    TribeNeedResourceArea(const csString& name, const csString& perception,
                          float needStartValue, float needGrowthValue,
                          TribeNeed * explore)
        :TribeNeed(RESOURCE_AREA,name,perception,needStartValue,needGrowthValue),
         explore(explore)
    {
    }

    virtual ~TribeNeedResourceArea()
    {
    }

    virtual void UpdateNeed(NPC * npc);

    virtual const TribeNeed* GetNeed() const;
private:
    TribeNeed * explore;

};

// ---------------------------------------------------------------------------------


/** TribeNeedReproduce govern the need for reproduction
 *
 *  Will increase in need as long as the tribe hasn't reached the maximum size.
 *  If the tribe hasn't the resources needed to grow it will call the dependend
 *  get_resource_need.
 */

class TribeNeedReproduce : public TribeNeed
{
public:
    
    TribeNeedReproduce(const csString& name, const csString& perception,
                       float needStartValue, float needGrowthValue,
                       TribeNeed * getResourceNeed)
        :TribeNeed(REPRODUCE,name,perception,needStartValue,needGrowthValue),
         getResourceNeed(getResourceNeed)
    {
    }

    virtual ~TribeNeedReproduce()
    {
    }

    virtual void UpdateNeed(NPC * npc);

    virtual const TribeNeed* GetNeed() const;

 private:
    TribeNeed * getResourceNeed;
};

// ---------------------------------------------------------------------------------

/** TribeNeedResourceRate respond to the resource rate of the tribe.
 *
 *  Will start to grow need when the resource rate limit is reached.
 */
class TribeNeedResourceRate : public TribeNeed
{
public:
    
    TribeNeedResourceRate(const csString& name, const csString& perception,
                          float needStartValue, float needGrowthValue,
                          TribeNeed * dependendNeed,float limit)
        :TribeNeed(RESOURCE_RATE,name,perception,needStartValue,needGrowthValue),
         dependendNeed(dependendNeed),limit(limit)
    {
        // No dependedNeed so make the GetNeed function return this need.
        if (dependendNeed == NULL)
        {
            dependendNeed = this;
        }
    }

    virtual ~TribeNeedResourceRate()
    {
    }

    virtual void UpdateNeed(NPC * npc);

    virtual const TribeNeed* GetNeed() const;
    
private:
    TribeNeed* dependendNeed;
    float      limit;
};

// ---------------------------------------------------------------------------------

/** TribeNeedDeathRate respond to the death rate of the tribe.
 *
 *  Will start to grow need when the death rate limit is reached.
 */
class TribeNeedDeathRate : public TribeNeed
{
public:
    
    TribeNeedDeathRate(const csString& name, const csString& perception,
                       float needStartValue, float needGrowthValue,
                       TribeNeed * dependendNeed, float limit)
        :TribeNeed(DEATH_RATE,name,perception,needStartValue,needGrowthValue),
         dependendNeed(dependendNeed), limit(limit)
    {
        // No dependedNeed so make the GetNeed function return this need.
        if (dependendNeed == NULL)
        {
            dependendNeed = this;
        }
    }

    virtual ~TribeNeedDeathRate()
    {
    }

    virtual void UpdateNeed(NPC * npc);

    virtual const TribeNeed* GetNeed() const;
    
private:
    TribeNeed* dependendNeed;
    float      limit;
};

// ---------------------------------------------------------------------------------

/** TribeNeedTimeOfDay respond to the time of day.
 *
 *  Will start to grow need when the time of day is reached.
 */
class TribeNeedTimeOfDay : public TribeNeed
{
public:
    
    TribeNeedTimeOfDay(const csString& name, const csString& perception,
                       float needStartValue, float needGrowthValue,
                       int startHour, int endHour)
        :TribeNeed(TIME_OF_DAY,name,perception,needStartValue,needGrowthValue),
         startHour(startHour), endHour(endHour)
    {
        // Make sure the endHour is greather than the start hour
        if (endHour < startHour) endHour += 24;
    }

    virtual ~TribeNeedTimeOfDay()
    {
    }

    virtual void UpdateNeed(NPC * npc);

    virtual const TribeNeed* GetNeed() const;
    
private:
    int        startHour;
    int        endHour;
};

// ---------------------------------------------------------------------------------

#endif
