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

using namespace CS;

#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>
#include "../iserver/idal.h"

#define QUEST_OPT_SAVEONCOMPLETE 0x01


class psQuestPrereqOp;
class psCharacter;
class NpcTrigger;
class psQuest;

/**
 * Utility function to parse prerequisite scripts.
 *
 * @param prerequisite The variable that will hold the parsed prerequisite
 * @param self    Pointer to the quest if used to load for a quest
 * @param scripts The prerequisite to parse <pre>...</pre>.
 * @return True if successfully parsed.
 */
bool LoadPrerequisiteXML(psQuestPrereqOp*&prerequisite, psQuest* self, csString script);

/**
 * This class holds the master list of all quests available in the game.
 */
class psQuest : public CS::Utility::WeakReferenced
{
 public:
    psQuest();
    virtual ~psQuest();

    bool Load(iResultRow& row);
    bool PostLoad();
    void Init(int id, const char *name);

    int GetID() const { return id; }    
    const char *GetName() const { return name; }
    const char *GetImage() const { return image; }
    const char *GetTask() const { return task; }
    void SetTask(csString mytask) { task = mytask; }
    psQuest *GetParentQuest() const { return parent_quest; }
    void SetParentQuest(psQuest *parent) { parent_quest=parent; }
    int GetStep() const { return step_id; }
    bool HasInfinitePlayerLockout() const { return infinitePlayerLockout; }
    unsigned int GetPlayerLockoutTime() const { return player_lockout_time; }
    unsigned int GetQuestLockoutTime() const { return quest_lockout_time; }
    unsigned int GetQuestLastActivatedTime() const { return quest_last_activated; }
    void SetQuestLastActivatedTime(unsigned int when) { quest_last_activated=when; }
    // csString QuestToXML() const;
    bool AddPrerequisite(csString prerequisitescript);
    bool AddPrerequisite(psQuestPrereqOp * op);
    void AddTriggerResponse(NpcTrigger * trigger, int responseID);
    void AddSubQuest(int id) { subquests.Push(id); }
    
    /**
     * Return the prerequisite for this quest.
     * @return The prerequisite for this quest.
     */
    psQuestPrereqOp *GetPrerequisite() { return prerequisite; }
    csString GetPrerequisiteStr();
    const csString& GetCategory() const { return category; }
    
    bool Active() { return active; }
    void Active(bool state) { active = state; }
    
 protected:
    int id;
    csString name;
    csString task;
    const char *image;
    int flags;
    psQuest *parent_quest;
    int step_id;
    psQuestPrereqOp * prerequisite;
    csString category;
    csString prerequisiteStr;
    bool infinitePlayerLockout;
    
    unsigned int player_lockout_time;
    unsigned int quest_lockout_time;
    unsigned int quest_last_activated;

    struct TriggerResponse
    {
        NpcTrigger * trigger;
        int responseID;
    };

    csArray<TriggerResponse> triggerPairs;
    csArray<int> subquests;
    
    bool active;
};

#endif
