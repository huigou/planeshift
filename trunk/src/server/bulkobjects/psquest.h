/*
 * psquest.h
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

#ifndef __PSQUEST_H__
#define __PSQUEST_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <idal.h>

//=============================================================================
// Local Includes
//=============================================================================

using namespace CS;


#define QUEST_OPT_SAVEONCOMPLETE 0x01

/// The quest is disabled and won't be loaded by the server, used for the flags column
#define PSQUEST_DISABLED_QUEST 0x00000001


class psQuestPrereqOp;
class psCharacter;
class NpcResponse;
class NpcTrigger;
class NpcDialogMenu;
class psQuest;

/**
 * Utility function to parse prerequisite scripts.
 *
 * @param prerequisite The variable that will hold the parsed prerequisite
 * @param self         Pointer to the quest if used to load for a quest
 * @param script       The prerequisite to parse \<pre\>...\</pre\>.
 * @return True if successfully parsed.
 */
bool LoadPrerequisiteXML(csRef<psQuestPrereqOp> &prerequisite, psQuest* self, csString script);

/**
 * This class holds the master list of all quests available in the game.
 */
class psQuest : public CS::Utility::WeakReferenced
{
public:
    /**
     * default constructor
     * 
     * @param id questID - either from the database, or dynamically created
     * @param name is a unique string
     */
    psQuest(int id = 0, const char* name = "");
    virtual ~psQuest();

    /**
     * loads the quest information from a supplied result set
     * @param row result set to store in this object
     */
    bool Load(iResultRow &row);
    /**
     * parses the prerequisite string and caches the result
     */
    bool PostLoad();
    
    /**
     * The id can be defined by the database or dynamically created.
     * 
     * Dynamic creation takes place in CacheManager::AddDynamicQuest
     * @return the quest's id
     */
    int GetID() const
    {
        return id;
    }
    /**
     * @return the quest's name
     */
    const char* GetName() const
    {
        return name;
    }
    const char* GetImage() const
    {
        return image;
    }
    const char* GetTask() const
    {
        return task;
    }

    /**
     * Gets if the task (quest description/note) contains some text.
     *
     * @return TRUE if the task has some text, FALSE otherwise.
     */
    bool hasTaskText()
    {
        return task.Length() > 0;
    }
    void SetTask(csString mytask)
    {
        task = mytask;
    }
    psQuest* GetParentQuest() const
    {
        return parent_quest;
    }
    void SetParentQuest(psQuest* parent)
    {
        parent_quest=parent;
    }
    int GetStep() const
    {
        return step_id;
    }
    bool HasInfinitePlayerLockout() const
    {
        return infinitePlayerLockout;
    }
    unsigned int GetPlayerLockoutTime() const
    {
        return player_lockout_time;
    }
    unsigned int GetQuestLockoutTime() const
    {
        return quest_lockout_time;
    }
    unsigned int GetQuestLastActivatedTime() const
    {
        return quest_last_activated;
    }
    void SetQuestLastActivatedTime(unsigned int when)
    {
        quest_last_activated=when;
    }
    // csString QuestToXML() const;
    bool AddPrerequisite(csString prerequisitescript);
    bool AddPrerequisite(csRef<psQuestPrereqOp> op);
    
    /**
     * Adds a set of pointers to a trigger and a response.
     * 
     * This is needed for deallocation at desctruction time.
     * @param trigger created by the quests questscript
     * @param response created by the quests questscript
     */
    void AddTriggerResponse(NpcTrigger* trigger, NpcResponse* response);
    /**
     * Adds a pointer to a menu.
     * 
     * This is needed for deallocation at desctruction time.
     * @param menu created by the quests questscript
     */
    //void AddMenu(NpcDialogMenu* menu);
    
    //csArray<NpcDialogMenu*> &GetMenuList();
    
    /**
     * Register a quest as a subquest of this quest.
     * 
     * Subquests are normally generated when parsing quest_scripts.
     * @param id of the subquest to register.
     */
    void AddSubQuest(int id)
    {
        subquests.Push(id);
    }

    /**
     * Returns an ordered list of the subquests of this quest (so it's steps).
     *
     * @return A reference to an array containing the id of each subquests.
     */
    csArray<int> &GetSubQuests()
    {
        return subquests;
    }

    /**
     * Return the prerequisite for this quest.
     *
     * @return The prerequisite for this quest.
     */
    csRef<psQuestPrereqOp>& GetPrerequisite()
    {
        return prerequisite;
    }
    csString GetPrerequisiteStr();
    const csString &GetCategory() const
    {
        return category;
    }

    /**
     * Check if the quest is active (and also it's parents).
     *
     * A quest to be active must be active itself and, if so, also it's parents (most probably earlier steps)
     * must be active themselves so check back to them if this quest is active else return as not active directly.
     *
     * @return The active status of the quest.
     */
    bool Active()
    {
        return active ? (parent_quest ? parent_quest->Active() : active) : active;
    }

    /**
     * Sets activation status of the quest.
     */
    void Active(bool state)
    {
        active = state;
    }

protected:
    int id;             ///< quest id - either as stored in the database or as assigned by CacheManager::AddDynamicQuest
    csString name;      ///< unique quest name
    csString task;      
    csString image;
    int flags;
    psQuest* parent_quest;      ///< parent quest of this quest (or NULL if there is none)
    int step_id;                ///< natoka: never used, though i suppose it was ment for the substep number
    csRef<psQuestPrereqOp> prerequisite;
    csString category;
    csString prerequisiteStr;
    bool infinitePlayerLockout;

    unsigned int player_lockout_time;
    unsigned int quest_lockout_time;
    unsigned int quest_last_activated;

    struct TriggerResponse
    {
        NpcTrigger* trigger;
        int responseID;
    };

    // this stuff is needed for cleanup when destroying the object
    csArray<TriggerResponse> triggerPairs; ///< list of trigger-response pairs added for the quest
    csArray<int> subquests;             ///< list of IDs of the subquests of this quest

    bool active;
};

#endif
