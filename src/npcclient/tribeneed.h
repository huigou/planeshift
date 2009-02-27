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


/**
 * Represent the base class for all tribe need types.
 */
class psTribeNeed 
{
public:
    psTribeNeedSet     *parentSet;    ///< Point to the need set that this need is part of
    psTribe::TribeNeed  needType;     ///< Set by each need type to one of the tribe needs
    float               current_need; ///< Represent current need. Will be used to sort each need.
    csString            name;         ///< Name of need used for debug prints.

    /**
     * Construct a basic need with the given needType and needName for debuging.
     */
    psTribeNeed(psTribe::TribeNeed needType, csString needName)
        :needType(needType),current_need(0.0),name(needName)
    {
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
        current_need = 0;
    }

    /**
     * By default the current_need is returned. To be overloaded by
     * specefic needs if there are gate conditions for returning the need.
     */
    virtual float GetNeed(NPC * npc)
    {
        return current_need;
    }

    /**
     * Get the need that is needed if this need is the most needed.
     * To be overloded if a need is selected and that need depend on some
     * other need before it can be used.
     */
    virtual psTribe::TribeNeed GetNeedType()
    {
        return needType;
    }
};

/**
 * Hold a collection of needs and calculate the highest need for the tribe.
 */
class psTribeNeedSet
{
public:

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
    psTribe::TribeNeed CalculateNeed(NPC * npc);

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
    void AddNeed(psTribeNeed * newNeed)
    {
        newNeed->SetParent(this);
        newNeed->ResetNeed();
        needs.Push(newNeed);
    }

private:
    psTribe               *tribe;
    csArray<psTribeNeed*>  needs;
};

// ---------------------------------------------------------------------------------

class psTribeNeedExplore : public psTribeNeed
{
public:
    psTribeNeedExplore()
        :psTribeNeed(psTribe::EXPLORE,"Explore")
    {
    }

    virtual ~psTribeNeedExplore()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        current_need += 0.2F;
    }

};

// ---------------------------------------------------------------------------------

class psTribeNeedWalk : public psTribeNeed
{
public:
    psTribeNeedWalk()
        :psTribeNeed(psTribe::WALK,"Walk")
    {
    }

    virtual ~psTribeNeedWalk()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        current_need += 0.1F;
    }

};

// ---------------------------------------------------------------------------------

class psTribeNeedDig : public psTribeNeed
{
public:
    psTribeNeedExplore * explore;
    
    psTribeNeedDig(psTribeNeedExplore * explore)
        :psTribeNeed(psTribe::DIG,"Dig"), explore(explore)
    {
    }

    virtual ~psTribeNeedDig()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        if (!GetTribe()->CanGrow())
        {
            current_need += 1.0f; // Make sure tribe can grow at all time
        }
        else
        {
            current_need += 0.1f;
        }
    }

    virtual psTribe::TribeNeed GetNeedType()
    {
        if (GetTribe()->FindMemory("mine"))
        {
            return needType;
        }
        else
        {
            return explore->GetNeedType();
        }
    }
};


// ---------------------------------------------------------------------------------

class psTribeNeedReproduce : public psTribeNeed
{
public:
    psTribeNeedDig * dig;
    
    psTribeNeedReproduce(psTribeNeedDig * dig)
        :psTribeNeed(psTribe::REPRODUCE,"Reproduce"),dig(dig)
    {
    }

    virtual ~psTribeNeedReproduce()
    {
    }

    virtual void UpdateNeed(NPC * npc)
    {
        if (GetTribe()->ShouldGrow())
        {
            current_need += 1.0;
        } else
        {
            current_need = 0.0;
        }
    }

    virtual psTribe::TribeNeed GetNeedType()
    {
        if (GetTribe()->CanGrow())
        {
            return needType;
        }
        else
        {
            return dig->GetNeedType();
        }
    }
};

// ---------------------------------------------------------------------------------

class psTribeNeedNothing : public psTribeNeed
{
public:
    psTribeNeedNothing()
        :psTribeNeed(psTribe::NOTHING,"Nothing")
    {
    }

    virtual ~psTribeNeedNothing()
    {
    }

    virtual void ResetNeed()
    {
        current_need = 0.5;  // Constant need to do nothing, 
                             // just to make sure its about everyone 
                             // without any need at all.
    }
};




#endif
