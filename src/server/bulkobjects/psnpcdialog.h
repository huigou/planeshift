/*
 * psnpcdialog.h
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
 * 
 */
#ifndef __PS_IMP_NPC_DIALOG__
#define __PS_IMP_NPC_DIALOG__

//CS Includes
#include "cstypes.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "csutil/scf.h"
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include "util/prb.h"

//psincludes
#include <util/psstring.h>
#include "dictionary.h"
#include "../client.h"
#include "../iserver/idal.h"

class gemNPC;
struct iObjectRegistry;
class  NPCDialogDict;
class  NpcResponse;
class  GemActor;

struct KnowledgeArea
{
    csString area;
    int      priority;

    bool operator==(KnowledgeArea& other)
    {
        return (priority==other.priority && area == other.area);
    };
    bool operator<(KnowledgeArea& other)
    {
        if (priority<other.priority)
        {
            return true;
        }
        else if (priority>other.priority)
        {
            return false;
        }
        // Priority is equal so check area.
        if (strcmp(area,other.area)<0)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
};

/** Sentence written by the user represented
    as sequence of known terms */
class NpcTriggerSentence
{
protected:
    csArray<NpcTerm*> terms;  // Not PArray because these ptrs are shared
    csString str; /// String version built from array

public:
    NpcTriggerSentence()  {};
    void AddToSentence(NpcTerm *next_word);
    size_t TermLength() { return terms.GetSize(); }
    const csString& GetString();
    void operator=(NpcTriggerSentence& other) { terms = other.terms; str=other.str; }
    NpcTerm*& Term(size_t i) { return terms[i]; }
    const char* GeneralizeTerm(NPCDialogDict *dict,size_t which, size_t depth);
};


/**
 * This class right now holds a simple circular MRU list
 * of responses, so the npc can tell if he is getting
 * the same question over and over.
 */
class DialogHistory
{
protected:
    struct DialogHistoryEntry
    {
        int playerID; /// Who was the response said to
        int responseID;  /// What response was said
        csTicks when;    /// Timestamp of response
    };
    csArray<DialogHistoryEntry> history;
    int counter;  /// Current location in circular buffer
public:
    DialogHistory() { counter=0; }

    void AddToHistory(int playerID, int responseID, csTicks when);
    bool EverSaid(int playerID, int responseID, csTicks& howLongAgo, int& times);
};
#define MAX_HISTORY_LEN 100

/**
 * This class exists per NPC, and holds all dialog triggers, responses
 * and scripts for this particular NPC by holding references to his/her
 * Knowledge Areas.
 */
class psNPCDialog
{
protected:
    gemNPC *self;
    csArray<KnowledgeArea*> knowareas;
    
    psString antecedent_her,
             antecedent_him,
             antecedent_it,
             antecedent_them;
    iDataConnection *db;
    csRandomGen *randomgen;
    gemActor *currentplayer;
    Client *currentClient;
    DialogHistory dialogHistory;


    //void FindTriggerWords(const char *text,csString& trigger);
    NpcResponse* FindResponse(csString& trigger,const char *text);
    bool CheckPronouns(psString& text);
    void UpdateAntecedents(NpcResponse *resp);
    void AddBadText(const char *text,const char *trigger);

protected:
    void CleanPunctuation(psString& str);
    
    /** Recognizes phrases in the text and translates them to synonyms,
        Ignores any unrecognized phrases. Returns array of recognized phrases. */
    void FilterKnownTerms(const psString & text, NpcTriggerSentence &trigger, Client *client);
    /**
     * Find a trigger by trying every combinations of generalization for each
     * term.
     */
    NpcResponse *FindOrGeneralizeTrigger(Client *client,NpcTriggerSentence& trigger,
                                         csArray<int>& gen_terms, int word);


    NpcResponse * ErrorResponse(const psString & text, const char *trigger);
    NpcResponse * RepeatedResponse(const psString& text, NpcResponse *resp, csTicks when, int times);
    
public:
    psNPCDialog(gemNPC *npc);
    virtual ~psNPCDialog();

    bool Initialize(iDataConnection *db,int NPCID);
    bool Initialize( iDataConnection* db );
    bool LoadKnowledgeAreas(int NPCID);

    NpcResponse *Respond(const char * text,Client *client);

    NpcResponse* FindXMLResponse(Client *client, csString trigger);

    bool AddWord(const char *word);
    bool AddSynonym(const char *word,const char *synonym);
    bool AddKnowledgeArea(const char *area);
    bool AddResponse(const char *area,const char *words,const char *response,const char *minfaction);
    bool AssignNPCArea(const char *npcname,const char *areaname);

    void SubstituteKeywords(Client * player, csString& response) const;


    /// Add a new trigger to the cached list.
    //    void AddNewTrigger( int databaseID );

    /// Add a new response to the cached list. 
    //    void AddNewResponse ( int databaseID );

    void DumpDialog();
};

#endif 


