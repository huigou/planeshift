/*
 * questmanager.h
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
#ifndef __QUESTMANAGER_H__
#define __QUESTMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psquest.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class


class Client;
class psItemStats;

struct QuestRewardOffer
{
    uint32_t clientID;
    csArray<psItemStats*> items;
};

class gemNPC;
class NpcDialogMenu;
class NpcResponse;
class WordArray;
class psString;

/**
 * This class handles quest management for the player,
 * tracking who has what quests assigned, etc.
 */
class QuestManager : public MessageManager
{
protected:
    csArray<QuestRewardOffer*>  offers;

    csString lastError;     ///< Last error message to send to client on loadquest.
            
    /**
     * Load all scripts from db
     */
    bool LoadQuestScripts();
    int  ParseQuestScript(int id,const char *script);
    void CutOutParenthesis(csString &response, csString &within,char start_char,char end_char);

    bool GetResponseText(csString& block,csString& response,csString& file_path,
                         csString& him, csString& her, csString& it, csString& them);
    bool BuildTriggerList(csString& block,csStringArray& list);

	bool BuildMenu(const csString& block,const csStringArray& list, psQuest *quest, NpcDialogMenu *menu);

    int GetNPCFromBlock(WordArray words,csString& current_npc);
    bool ParseItemList(const csString & input, csString & parsedItemList);
    bool ParseItem(const char *text, psStringArray & xmlItems, psMoney & money);

    NpcResponse *AddResponse(csString& current_npc,const char *response_text,
                             int& last_response_id, psQuest * quest,
                             csString& him, csString& her, csString& it, csString& them, csString& file_path);
    bool         AddTrigger(csString& current_npc,const char *trigger,
                            int prior_response_id,int trig_response, psQuest* quest, const psString& postfix);
	void		 MergeTriggerMenus(NpcDialogMenu *pending_menu, const csString& current_npc);

    void GetNextScriptLine(psString& scr, csString& block, size_t& start, int& line_number);
    bool PrependPrerequisites(csString &substep_requireop, 
                              csString &response_requireop,
                              bool quest_assigned_already,
                              NpcResponse *last_response,
                              psQuest *mainQuest);

    bool HandlePlayerAction(csString& block, 
                            size_t& which_trigger,
                            csString& current_npc,
                            csStringArray& pending_triggers);

    bool HandleScriptCommand(csString& block,
                             csString& response_requireop,
                             csString& substep_requireop,
                             NpcResponse *last_response,
                             psQuest *mainQuest,
                             bool& quest_assigned_already,
                             psQuest *quest);

    void HandleQuestInfo(MsgEntry *pMsg,Client *client);
    void HandleQuestReward(MsgEntry *pMsg,Client *client);

public:

    QuestManager();
    virtual ~QuestManager();

    bool Initialize();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client) { }

    void Assign(psQuest *quest, Client *who, gemNPC *assigner,csTicks timeDelay=0);
    bool Complete(psQuest *quest, Client *who);

    void OfferRewardsToPlayer(Client *who, csArray<psItemStats*> &offer,csTicks& timeDelay);
    bool GiveRewardToPlayer(Client *who, psItemStats* item);
    bool LoadQuestScript(int id);
    const char* LastError() { return lastError.GetData(); }
};


#endif

