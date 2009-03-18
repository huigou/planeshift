/*
 * dictionary.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PS_DICTIONARY_
#define PS_DICTIONARY_

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/refcount.h>
#include <csutil/parray.h>
#include <csutil/hash.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/prb.h"

#include "rpgrules/psmoney.h"

#include "../tools/wordnet/wn.h"

#include "../server/iserver/idal.h"      // Database Abstraction Layer Interface

//=============================================================================
// Local Includes
//=============================================================================
#include "psquestprereqops.h"

#define MAX_RESP 5

class NpcTerm;
class NpcTriggerGroupEntry;
class NpcTrigger;
class NpcResponse;
class NpcDialogMenu;
class psSkillInfo;
class gemNPC;
class gemActor;
class Client;
class psQuest;
class psCharacter;

struct iDocumentNode;
struct Faction;

class NPCDialogDict : public csRefCount
{
protected:
    BinaryRBTree<NpcTerm>              phrases;
    BinaryRBTree<NpcTriggerGroupEntry> trigger_groups;
    csHash<NpcTriggerGroupEntry*>      trigger_groups_by_id;
    BinaryRBTree<NpcTrigger>           triggers;
    csHash<NpcTrigger>                 trigger_by_id;
    BinaryRBTree<NpcResponse>          responses;
    BinaryRBTree<csString>             disallowed_words;
	csHash<csString,csString>		   knowledgeAreas;  /// Collection of all referenced knowledge areas

	/// This is a storage area for popup menus parsed during quest loading, which is done before NPCs are spawned.
	csHash<NpcDialogMenu*,csString>    initial_popup_menus;
    
	int dynamic_id;

    bool LoadSynonyms(iDataConnection *db);
    bool LoadTriggerGroups(iDataConnection *db);
    bool LoadTriggers(iDataConnection *db);
    bool LoadResponses(iDataConnection *db);
    bool LoadDisallowedWords(iDataConnection *db);
    
    /** All unknown words from 'trigger' that are not disallowed are added to 'phrases'.
        All disallowed words from 'trigger' are removed from 'trigger' */
    void AddWords(csString& trigger);

    /** 
     * Adds a new record to 'terms' 
     *
     * @return A new term or and existing term if allready added.
     */
    NpcTerm* AddTerm(const char *term);

    int AddTriggerGroupEntry(int id,const char *txt, int equivID);
    void ParseMultiTrigger(NpcTrigger *parsetrig);

public:
    NPCDialogDict();
    virtual ~NPCDialogDict();

    bool Initialize(iDataConnection *db);
        
	bool FindKnowledgeArea(const csString& name);

    /** Returns record of 'term' (or NULL if unknown) */
    NpcTerm * FindTerm(const char *term);
    
    /** Returns synonym of 'term' (or NULL if unknown). If 'term' is known but has no synonym, then 'term' itself is returned */
    NpcTerm * FindTermOrSynonym(const csString & term);
    
    NpcResponse *FindResponse(gemNPC * npc,
                              const char *area,
                              const char *trigger,
                              int faction,
                              int priorresponse,
                              Client *client);

    /** Lookup the response with the given ID from responses */
    NpcResponse * FindResponse(int responseID); 

    /** Loads a trigger from database (its id=databaseID) */
    bool AddTrigger( iDataConnection* db, int databaseID, int responseID );

    /** substitute master trigger if this is child trigger in group. return true if substituted */
    bool CheckForTriggerGroup(csString& trigger);
    
    /** Loads a response from database (its id=databaseID) */
    void AddResponse( iDataConnection* db, int databaseID );

    /** Adds a response dynamically not from the database. Supplies new_id if 0. */
    NpcResponse *AddResponse(const char *response_text,
                             const char *pronoun_him,
                             const char *pronoun_her,
                             const char *pronoun_it,
                             const char *pronoun_them,
                             const char *npc_name,
                             int &new_id,
                             psQuest * quest,
							 const char *audio_path);

    NpcTrigger *AddTrigger(const char *k_area,
                    const char *mytrigger,
                    int prior_response, 
                    int trigger_response);

    void DeleteTriggerResponse(NpcTrigger * trigger, int responseId);

	/// Find a stored initial trigger menu with the specified NPC name
	NpcDialogMenu *FindMenu(const char *name);

	/// Store an initial trigger menu with the specified NPC name
	void AddMenu(const char *name, NpcDialogMenu *menu);

    /**
     * Dump the entire dictionary
     */
    void Print(const char *area);
};

/** 
 * A phrase recognized by the dialog system. Each term can hold a pointer to a
 * synonym e.g. hello = greetings or a more general term e.g. "apple" --> "fruit". 
 */
class NpcTerm
{
    SynsetPtr hypernymSynNet;

    csArray<const char*> hypernyms;

    void BuildHypernymList();

public:

    /** 
     * The recognized phrase/term
     */
    csString term;

    /**
     * Pointer to a synonym term.
     *
     * If nonzero, phrase is translated to 'synonym' 
     *
     * Phrases that have 'synonym' should have no 'moreGeneral' term,
     * because 'moreGeneral' of their 'synonym' is used instead of it
     */
    NpcTerm* synonym;      


    /**
     * Constructor for the term
     */
    NpcTerm(const char* term)
    {
        synonym     = NULL;
        hypernymSynNet = NULL;
        this->term = term;
        this->term.Downcase();
    }
    ~NpcTerm()
    {
        if(hypernymSynNet)
            free_syns(hypernymSynNet);
    }

    const char* GetInterleavedHypernym(size_t which);   

    bool IsNoun();

    /**
     * Compare if two terms are equal
     */
    bool operator==(NpcTerm& other) const
    {
        return term==other.term;
    };

    /**
     * Compare if one term is less than the other.
     */
    bool operator<(NpcTerm& other) const
    {
        return (strcmp(term,other.term)<0);
    };

    /**
     * Dump the term to stdout.
     */
    void Print();
};

class NpcTriggerGroupEntry
{
public:
    int id;
    csString text;
    NpcTriggerGroupEntry *parent;

    NpcTriggerGroupEntry(int i,const char *txt,NpcTriggerGroupEntry *myParent=0)
    {
        id = i;
        text = txt;
        parent = myParent;
    }
    bool operator<(NpcTriggerGroupEntry& other) const
    {
        return text < other.text;
    }
    bool operator==(NpcTriggerGroupEntry& other) const
    {
        return text == other.text;
    }
};

class NpcTrigger
{
public:
    int      id;
    csString area;
    csString trigger;
    int      priorresponseID;
    csArray<int> responseIDlist;
    //    int      questID;

    /// Load the trigger from a database
    bool Load(iResultRow& row);

    /// Return true if there are one available response for this trigger
    bool HaveAvailableResponses(Client * client, gemNPC * npc, NPCDialogDict * dict, csArray<int> *availableResponseList = NULL );

    /// Return one of the members of responseIDlist array randomly
    int  GetRandomResponse( const csArray<int> &availableResponseList);

    /// Compare two triggers. Used when searching for triggers.
    bool operator==(NpcTrigger& other) const;

    /// Compare two triggers. Used when searching for triggers.
    bool operator<(NpcTrigger& other) const;
};


/**
 * Possible actions scriptable in the quest engine all
 * inherit from this class.
 */
class ResponseOperation
{
protected:
    csString name;
public:
    virtual ~ResponseOperation() {};
    virtual bool Load(iDocumentNode *node) = 0;
    virtual csString GetResponseScript() = 0;
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay) = 0;
    const char *GetName() { return name; }
};

/**
 * Holds the trigger menu, if it exists, for a given location in a dialog
 */
class NpcDialogMenu
{
protected:
	struct DialogTrigger
	{
		unsigned int triggerID;
		csString menuText;
		csString trigger;
        psQuest *quest;
        csRef<psQuestPrereqOp> prerequisite;
	};

	unsigned int counter; // ID counter

public:

	csArray<DialogTrigger> triggers;

	NpcDialogMenu();

	void AddTrigger( const csString &menuText, const csString &trigger, psQuest *quest, psQuestPrereqOp *script=NULL );
	void Add( NpcDialogMenu *add);
	void ShowMenu( Client *client );
    void SetPrerequisiteScript(psQuestPrereqOp *script);
};

/**
 * This class holds several possible responses and an
 * action script for the npc to run whenever an 
 * appropriate trigger is triggered.
 */
class NpcResponse
{
 public:
    int      id;                 /// xref from trigger response
    csString response[MAX_RESP]; /// possible alternative answers for this response
    csString him,her,it,them;    /// record antecedents for next question pronouns
    csString triggerText;        /// This is the text that triggered the response.
	csString voiceAudioPath;	 /// Optional vfs path to audio file to stream on demand to the client
    int type;                    /// record the type of response
    psQuest * quest;             /// Quest that this respons is part of
    int active_quest;            /// which one should be run.  this is actually set by check quest avail op
    csTicks timeDelay;           /// This tracks the current time delay for chat msgs in the responses, so a single script can have a sequence of things that take a while
    csPDelArray<ResponseOperation> script;  /// list of ops in script to execute when triggered
    csRef<psQuestPrereqOp> prerequisite; /// prerequisite for this Response to be available
	NpcDialogMenu *menu;		/// List of possible player trigger replies for this response, for display to the player.

    enum
    {
        VALID_RESPONSE,
        ERROR_RESPONSE
    };

    NpcResponse();
    virtual ~NpcResponse() {}
    
    bool Load(iResultRow& row);

    void SetActiveQuest(int max);
    int  GetActiveQuest() { return active_quest; }

    const char *GetResponse();

	/// Returns the VFS path to the audio file which represents this response on the server, or NULL if one was not specified.
	const char *GetVoiceFile() { return voiceAudioPath; }

    /// Check for SayResponseOp with public flag set, which tells chat whether it is public or private.
    bool HasPublicResponse();
    bool ParseResponseScript(const char *xmlstr,bool insertBeginning=false);
    bool ExecuteScript(Client *client, gemNPC* target);
    csString GetResponseScript();

    // This is used so that the popup menu and the subsequent response can share the same filtering criteria
    psQuestPrereqOp *GetPrerequisiteScript() { return prerequisite; }

    /**
     * Pars and append the xml based prerequisite script to the
     * prerequisite for this Response.
     *
     */
    bool ParsePrerequisiteScript(const char *xmlstr,bool insertBeginning=false);

    /**
     * Add a prerequisite to the prerequisite for this response.
     *
     * @param op The prerequisite op to add
     * @insertBeginning Insert at beginning or at end (Default at end).
     * @return True if successfully added.
     */
    bool AddPrerequisite(csRef<psQuestPrereqOp> op, bool insertBeginning = false);
    
    /**
     * Check if the prerequisite for this response is valid
     *
     * @param character The Character trying to get the response
     * @return True if prerequisite are all ok
     */
    bool CheckPrerequisite(psCharacter * character);

    bool operator==(NpcResponse& other) const
    {
        return id==other.id;
    };

    bool operator<(NpcResponse& other) const
    {
        return id<other.id;
    };
};

/**
 * This script operation chooses randomly between the responses
 * specified in the response columns and causes the server to 
 * say one of them to the player.
 */
class SayResponseOp : public ResponseOperation
{
public:
    /// Indicates whether the response dialog should be said publicly or /tell in private
    bool saypublic;
    /// Optional specific string to say, used by quest_scripts
    csString *sayWhat;

    SayResponseOp(bool is_public) { sayWhat=NULL; saypublic = is_public; name = "respond";}
    virtual ~SayResponseOp() { if (sayWhat) delete sayWhat; }
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation makes an npc do an action, like greet or bow,
 * as part of his response to a player event.
 */
class ActionResponseOp : public ResponseOperation
{
protected:
    bool actionMy,actionNarrate;
    csString anim;
    csString *actWhat;
public:
    ActionResponseOp(bool my,bool narrate) { actionMy = my; actionNarrate=narrate; name = "action"; }
    virtual ~ActionResponseOp() { if (actWhat) delete actWhat; }
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation send a perception/command to the npc client.
 * This command could picked up by a response in the beavior for a npc.
 * E.g. sending the command "test_cmd" could be received with the:
 * <react event="npccmd:test_cmd" ... />
 */
class NPCCmdResponseOp : public ResponseOperation
{
protected:
    csString cmd;
public:
    NPCCmdResponseOp() { name = "npccmd"; }
    virtual ~NPCCmdResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

class psQuest; 

/**
 * This script operation checks to make sure a named quest
 * has been completed by a player, and stops the script if not,
 * issuing the specified error_msg dialog.
 */
class VerifyQuestCompletedResponseOp : public ResponseOperation
{
protected:
    psQuest *quest;        /// ptrs to cachemanager entries for quests. must have at least one.
    csString error_msg;    /// string with npc declining to play ball if quest is not completed

public:
    VerifyQuestCompletedResponseOp() { name="verifyquestcompleted"; quest=NULL; }
    virtual ~VerifyQuestCompletedResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation checks to make sure a named quest
 * has been assigned to a player, and stops the script if not,
 * issuing the specified error_msg dialog.
 */
class VerifyQuestAssignedResponseOp : public ResponseOperation
{
protected:
    psQuest *quest;        /// ptrs to cachemanager entries for quests. must have at least one.
    csString error_msg;    /// string with npc declining to proceed because player does not have quest

public:
    VerifyQuestAssignedResponseOp() { name="verifyquestassigned"; quest=NULL; }
    VerifyQuestAssignedResponseOp(int quest);
    virtual ~VerifyQuestAssignedResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/*
class FireEventResponseOp : public ResponseOperation
{
protected:
    csString event;
public:
    FireEventResponseOp() { name="fireevent"; }
    virtual ~FireEventResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);    
};
*/

/**
 * This script operation checks to make sure a named quest
 * has not been assigned to a player, and stops the script if not,
 * issuing the specified error_msg dialog.
 */
class VerifyQuestNotAssignedResponseOp : public ResponseOperation
{
protected:
    psQuest *quest;        /// ptrs to cachemanager entries for quests. must have at least one.
    csString error_msg;    /// string with npc declining to proceed because player does not have quest

public:
    VerifyQuestNotAssignedResponseOp() { name="verifyquestnotassigned"; quest=NULL; }
    VerifyQuestNotAssignedResponseOp(int quest);
    virtual ~VerifyQuestNotAssignedResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation makes an npc assign one out of a list of quest to a player,
 * as part of his response to a player event.
 */
class AssignQuestResponseOp : public ResponseOperation
{
protected:
    psQuest *quest[MAX_RESP];  /// ptrs to cachemanager entries for quests. must have at least one.
    csString timeout_msg;  /// string with npc declinign to give quest
    int num_quests;        /// how many of the quests are valid.

public:
    AssignQuestResponseOp() { name="assign"; quest[0]=NULL; }
    virtual ~AssignQuestResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
    const char *GetTimeoutMsg() { return timeout_msg; }
    psQuest *GetQuest(int n) { return quest[n]; }
    int      GetMaxQuests() { return num_quests; }
};

/**
 * This script operation is a pre operation to the AssignQuestResponseOp
 * that is inserted at the start of the ResponseOp script list and it will
 * select the quest that will be assigned in the AssingQuestResponseOp
 * later. Any SayRespondOp will select the corresponding response to.
 */
class AssignQuestSelectOp : public ResponseOperation
{
protected:
    AssignQuestResponseOp *quest_op;

public:
    AssignQuestSelectOp(AssignQuestResponseOp *questop) { name = "questselect"; quest_op = questop; }
    virtual ~AssignQuestSelectOp() {};
    virtual bool Load(iDocumentNode *node) { return true; }
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};


/**
 * This script operation is a pre operation to the AssingQuestResponseOp
 * that is inserted at the start of the ResponseOp script list and it will
 * check if the selected response is available and if not send a give response
 * back to the player and terminate the response script.
 */
class CheckQuestTimeoutOp : public ResponseOperation
{
protected:
    AssignQuestResponseOp *quest_op;

public:
    CheckQuestTimeoutOp(AssignQuestResponseOp *questop) { name = "checkquesttimeout"; quest_op = questop; }
    virtual ~CheckQuestTimeoutOp() {};
    virtual bool Load(iDocumentNode *node) { return true; }
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation makes an npc complete a quest for a player,
 * as part of his response to a player event.
 */
class CompleteQuestResponseOp : public ResponseOperation
{
protected:
    psQuest *quest;
    csString error_msg;

public:
    CompleteQuestResponseOp() { name="complete"; quest=NULL; }
    virtual ~CompleteQuestResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

class psItemStats;

/**
 * This script operation makes an npc give an item to a player,
 * as part of his response to a player event.
 */
class GiveItemResponseOp : public ResponseOperation
{
protected:
    psItemStats *itemstat;
    int count;

public:
    GiveItemResponseOp() { name="give"; itemstat=NULL; count=1; }
    virtual ~GiveItemResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};


/**
 * This script operation adjust a given faction.
 */
class FactionResponseOp : public ResponseOperation
{
protected:
    Faction * faction;
    int value;

public:
    FactionResponseOp() { name="faction"; }
    virtual ~FactionResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation invokes the progression manager to run a script,
 * as part of his response to a player event.
 */
class RunScriptResponseOp : public ResponseOperation
{
protected:
    csString scriptname;
    float p0, p1, p2;

public:
    RunScriptResponseOp() { name="run"; p0=p1=p2=0; }
    virtual ~RunScriptResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation invokes the progression manager to start training
 * as part of his response to a player event.
 */
class TrainResponseOp : public ResponseOperation
{
protected:
    csString scriptname;
    psSkillInfo* skill;

public:
    TrainResponseOp() { name="train"; }
    virtual ~TrainResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation makes an npc do an action, like greet or bow,
 * as part of his response to a player event.
 */
class GuildAwardResponseOp : public ResponseOperation
{
protected:
    int karma;
public:
    GuildAwardResponseOp() { name = "guild_award"; }
    virtual ~GuildAwardResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};


/**
 * This script operation makes an npc offer a list of possible
 * rewards that the player can chose from (upon quest completion).
 */
class OfferRewardResponseOp : public ResponseOperation
{
protected:
    csArray<psItemStats*>  offer;
public:
    OfferRewardResponseOp() { name = "offer"; }
    virtual ~OfferRewardResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation makes an npc give money
 * to the player.
 */
class MoneyResponseOp : public ResponseOperation
{
protected:
     psMoney money;

public:
    MoneyResponseOp() { name = "money"; }
    virtual ~MoneyResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation introduces an npc
 */
class IntroduceResponseOp : public ResponseOperation
{
protected:
    csString targetName;
public:
    IntroduceResponseOp() { name = "introduce"; }
    virtual ~IntroduceResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

/**
 * This script operation executes an admin command
 */
class DoAdminCommandResponseOp : public ResponseOperation
{
protected:
    csString origCommandString, modifiedCommandString;
public:
    DoAdminCommandResponseOp() { name = "doadmincmd"; }
    virtual ~DoAdminCommandResponseOp() {};
    virtual bool Load(iDocumentNode *node);
    virtual csString GetResponseScript();    
    virtual bool Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay);
};

#endif
