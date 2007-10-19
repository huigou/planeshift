/*
* psnpcdialog.cpp
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
#include <psconfig.h>
#include <ctype.h>
//#include "psstdint.h"

//CS includes
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include <iutil/objreg.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/evdefs.h>
#include <iutil/virtclk.h>

#include <imesh/sprite3d.h>

#include <csutil/databuf.h>
#include <csutil/plugmgr.h>
#include <iengine/movable.h>
#include <iengine/engine.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

//CEL includes
// #include <physicallayer/pl.h>
#include <physicallayer/entity.h>
#include <propclass/mesh.h>
//#include <behaviourlayer/behave.h>


// PS Includes
#include "util/strutil.h"
#include "util/psstring.h"
#include "util/serverconsole.h"

#include "psnpcdialog.h"
#include "util/log.h"
#include "../iserver/idal.h"

#include "dictionary.h"
#include "../psserver.h"
#include "util/psdatabase.h"
#include "../gem.h"
#include "../globals.h"
#include "../playergroup.h"
#include "psraceinfo.h"

//----------------------------------------------------------------------------

void NpcTriggerSentence::AddToSentence(NpcTerm *next_word)
{ 
    assert(next_word!=NULL); 
    terms.Push(next_word); 
    str="";
}


const csString& NpcTriggerSentence::GetString()
{
    if (str.Length())
        return str;

    for (size_t i=0; i<terms.GetSize(); i++)
    {
        str.Append(terms[i]->term);
        if (i<terms.GetSize()-1)
            str.Append(' ');
    }
    return str;
}

const char* NpcTriggerSentence::GeneralizeTerm(NPCDialogDict *dict,size_t which, size_t depth)
{
    if (which >= terms.GetSize())
        return false;
    
    str = "";

    return terms[which]->GetInterleavedHypernym(depth);
}




//----------------------------------------------------------------------------



psNPCDialog::psNPCDialog(gemNPC *npc)
{
    db = NULL;
    randomgen = NULL;
    self = npc;
}

psNPCDialog::~psNPCDialog()
{
}

bool psNPCDialog::Initialize( iDataConnection *db )
{
    this->db = db;

    if (!dict)
    {
        dict = new NPCDialogDict;

        if (!dict->Initialize(db))
        {
            delete dict;
            dict=NULL;
            return false;
        }
    }
    return true;
}


bool psNPCDialog::Initialize(iDataConnection *db,int NPCID)
{
    randomgen = psserver->rng;
    this->db = db;
    // Initialize base dictionary
    if (!dict)
    {
        dict = new NPCDialogDict;

        if (!dict->Initialize(db))
        {
            delete dict;
            dict=NULL;
            return false;
        }
    }
    else
        dict->IncRef();

    return LoadKnowledgeAreas(NPCID);
}

bool psNPCDialog::LoadKnowledgeAreas(int NPCID)
{
    Result result(db->Select("select area,priority"
        "  from npc_knowledge_areas"
        " where player_id=%d order by priority ASC",NPCID));

    if (!result.IsValid() )
    {
        Error1("Cannot load knowledge areas into dictionary from database.");
        return false;
    }

    for (unsigned int i=0; i<result.Count(); i++)
    {
        KnowledgeArea *newarea = new KnowledgeArea;

        // Downcase KA area before inserting into tree
        csString area = result[i]["area"];
        newarea->area = area.Downcase();

        newarea->priority = result[i].GetInt("priority");

        knowareas.Push(newarea);
    }
    return true;
}

void psNPCDialog::DumpDialog()
{
    CPrintf(CON_CMDOUTPUT,"Pri Knowledge area\n");
    for ( size_t z = 0; z < knowareas.GetSize(); z++ )
    {
        KnowledgeArea *ka = knowareas[z];    
        CPrintf(CON_CMDOUTPUT,"%3d %s\n",ka->priority,ka->area.GetDataSafe());
    }        
}

bool psNPCDialog::CheckPronouns(psString& text)
{
    int wordnum=1;
    psString word("temp");

    while (word.Length())
    {
        word = GetWordNumber(text,wordnum++);

        if (word == "him" || word=="he")
        {
            if (!antecedent_him.IsEmpty())
                text.ReplaceSubString(word,antecedent_him);
        }

        if (word == "her" || word=="she")
        {
            if (!antecedent_her.IsEmpty())
                text.ReplaceSubString(word,antecedent_her);
        }

        if (word == "them" || word=="they")
        {
            if (!antecedent_them.IsEmpty())
                text.ReplaceSubString(word,antecedent_them);
        }

        if (word == "it")
        {
            if (!antecedent_them.IsEmpty())
                text.ReplaceSubString(word,antecedent_them);
        }
    }
    return true;
}

void psNPCDialog::CleanPunctuation(psString& str)
{
    for (unsigned int i=0; i<str.Length(); i++)
    {
        if (ispunct(str.GetAt(i)) && str.GetAt(i)!='\'')
        {
            str.DeleteAt(i);
            i--;
        }
    }

    // we also use this function to remove the NPC's own name from the dialog
    psString wordInName,npc_name;
    wordInName = "bla";
    npc_name = self->GetEntity()->GetName();
    int num = 1;
    while (wordInName.Length())
    {
        wordInName = GetWordNumber(npc_name,num);
        if (!wordInName.Length())
            break;
        int pos = (int)str.FindSubString(wordInName,0,XML_CASE_INSENSITIVE);
        if (pos != -1)
        {
            str.DeleteAt(pos,wordInName.Length());
        }
        num++;
    }
}

void psNPCDialog::FilterKnownTerms(const psString & text, NpcTriggerSentence &trigger, Client *client)
{
    const size_t MAX_SENTENCE_LENGTH = 4;
    NpcTerm* term;

    WordArray words(text);
    size_t numWordsInPhrase = words.GetCount();
    size_t firstWord=0;
    csString candidate;
    
    if (!dict)   // Pointless to try if no dictionary loaded.
        return;
    
    Debug2(LOG_NPC, client->GetClientNum(),"Recognizing phrases in '%s'", text.GetData());
    
    while (firstWord<words.GetCount() && trigger.TermLength()<MAX_SENTENCE_LENGTH)
    {
        candidate = words.GetWords(firstWord, firstWord+numWordsInPhrase);
        term  = dict->FindTermOrSynonym(candidate);
        if (term)
        {
            trigger.AddToSentence(term);
            firstWord += numWordsInPhrase;
            numWordsInPhrase = words.GetCount() - firstWord;
        }
        else if (numWordsInPhrase > 1)
            numWordsInPhrase--;
        else
        {
            firstWord ++;
            numWordsInPhrase = (int)words.GetCount() - firstWord;
        }
    }
    
    Debug2(LOG_NPC, client->GetClientNum(),"Phrases recognized: '%s'", trigger.GetString().GetData());
}

void psNPCDialog::AddBadText(const char *text, const char *trigger)
{
    csString escText,escTrigger;
    csString escName;
    csString escSelfName;
    
    db->Escape( escText, text );
    db->Escape( escName, currentClient->GetName() );
    db->Escape( escSelfName, self->GetEntity()->GetName() );
    db->Escape( escTrigger, trigger);
    int ret = db->Command("insert into npc_bad_text "
                          "(badtext,triggertext,player,npc,occurred) "
                          "values ('%s','%s','%s','%s',Now() )",
                          escText.GetData(),
                          escTrigger.GetData(),
                          escName.GetData(),
                          escSelfName.GetData());
    if ((uint)ret == QUERY_FAILED)
    {
        Error2("Inserting npc_bad_text failed: %s",db->GetLastError() );
    }

    self->AddBadText(text,trigger);  // Save bad text in RAM cache so Settings can get at it easily in-game.
}


NpcResponse *psNPCDialog::FindResponse(csString& trigger,const char *text)
{
    KnowledgeArea *area;
    NpcResponse *resp = NULL;
    csString trigger_error;

    //May be it is safe not to check for characterdata (now needed for GetLastRespons())
    if (currentClient->GetCharacterData() == NULL)
    {
        Error1("NpcResponse *psNPCDialog::FindResponse(csString& trigger,const char *text) called with "
            "currentClient->GetCharacterData() returning NULL.");
        return NULL;
    }

    if (trigger.GetData() == NULL) 
        return NULL;

    trigger.Downcase();

    trigger_error = trigger;
    trigger_error.Append(" error");

    for (size_t z = 0; z < knowareas.GetSize(); z++)
    {
        area = knowareas[z];
        Debug4(LOG_NPC, currentClient->GetClientNum(),"NPC checking %s for trigger %s , with lastResponseID %d...",
                    (const char *)area->area,(const char *)trigger,currentClient->GetCharacterData()->GetLastResponse());

        //first try with last responses of all assigned quests
        for (size_t q = 0; q < currentClient->GetCharacterData()->GetNumAssignedQuests(); q++)
        {
            resp = dict->FindResponse(self, area->area,trigger,0,
                currentClient->GetCharacterData()->GetAssignedQuestLastResponse(q),currentClient); 
            if (resp)
                break;
        }
        if (!resp) //else, try old way with general last response
        {
            resp = dict->FindResponse(self, area->area,trigger,0,currentClient->GetCharacterData()->GetLastResponse(),currentClient);
        }
        if (!resp) // If no response found, try search for error trigger
        {
            //first try with last responses of all assigned quests
            for (size_t q = 0; q < currentClient->GetCharacterData()->GetNumAssignedQuests(); q++)
            {
                resp = dict->FindResponse(self, area->area,trigger_error,0,
                    currentClient->GetCharacterData()->GetAssignedQuestLastResponse(q),currentClient); 
                if (resp)
                    break;
            }
            if (!resp) //else, try old way with general last response
            {
                resp = dict->FindResponse(self, area->area,trigger_error,0,currentClient->GetCharacterData()->GetLastResponse(),currentClient);
            }
            if (!resp) // If no response found, try search without last response
            {
                if (currentClient->GetCharacterData()->GetLastResponse() == -1)
                {
                    // No point testing without last response
                    // if last response where no last response.
                    continue;
                }

                resp = dict->FindResponse(self, area->area,trigger,0,-1,currentClient);
                if (!resp) // If no response found, try search for error trigger without last response
                {
                    resp = dict->FindResponse(self, area->area,trigger_error,0,-1,currentClient);
                    if (resp)
                    {
                        // Force setting of type Error if error trigger found
                        resp->type = NpcResponse::ERROR_RESPONSE;
                    }
                }
            }
            else
            {
                // Force setting of type Error if error trigger found
                resp->type = NpcResponse::ERROR_RESPONSE;
            }
        }
        
        if (resp)
        {
            Debug3(LOG_NPC, currentClient->GetClientNum(),"Found response %d: %s",resp->id,resp->GetResponse());
            resp->triggerText = trigger;
            UpdateAntecedents(resp);
            break;
        }
    }
    return resp;
}

void psNPCDialog::SubstituteKeywords(Client * player, csString& resp) const
{
    psString dollarsign("$"),response(resp);
    int where = response.FindSubString(dollarsign);

    while (where!=-1)
    {
        psString word2,word;
        response.GetWord(where+1,word2,psString::NO_PUNCT);

        word = "$";
        word.Append(word2);  // include $sign in subst.

        if (strcmp(word.GetData(),"$playername")==0)
        {
            if (!response.ReplaceSubString(word,player->GetName()))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if (strcmp(word.GetData(),"$playerrace")==0)
        {            
            if (!response.ReplaceSubString(word, player->GetCharacterData()->raceinfo->name))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if (strcmp(word.GetData(),"$sir")==0)
        {
            const char* sir;
            if ( player->GetCharacterData()->raceinfo->gender == PSCHARACTER_GENDER_FEMALE )
                sir = "Madam";
            else if (player->GetCharacterData()->raceinfo->gender == PSCHARACTER_GENDER_MALE)
                sir = "Sir";
            else
                sir = "Gemma";
                
            if (!response.ReplaceSubString(word,sir))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        where = response.FindSubString(dollarsign,where+1);
    }
    resp = response;
}

NpcResponse *psNPCDialog::FindOrGeneralizeTrigger(Client *client,NpcTriggerSentence& trigger,
                                                  csArray<int>& gen_terms, int word)
{
    NpcResponse *resp;
    csStringArray generalized;

    bool hit;

    // Perform breadth-first search on generalisations

    // Copy all terms into stringarray
    for(size_t i=0;i<trigger.TermLength();i++)
        generalized.Push(trigger.Term(i)->term);
    
    // We do at least one search with no generalisations
    size_t depth = 0;

    do
    {
        hit = false;

        for(size_t i=0;i<gen_terms.GetSize();i++)
        {
            const char* hypernym = trigger.Term(gen_terms[i])->GetInterleavedHypernym(depth);
            if(hypernym)
            {
                generalized.Put(gen_terms[i],hypernym);
                hit = true;
            }

            csString generalized_copy;

            // Merge string
            for(size_t i=0;i<generalized.GetSize();i++)
            {
                generalized_copy.Append(generalized[i]);
            }
     
			printf("Searching for generalized trigger: %s'\n", generalized_copy.GetDataSafe());

            dict->CheckForTriggerGroup(generalized_copy);  // substitute master trigger if this is child trigger in group

            resp = FindResponse( generalized_copy, trigger.GetString());
            if (resp)
            {
                Debug2(LOG_NPC, client->GetClientNum(),"Found response to: '%s'", generalized_copy.GetData());

                //         // Removed till we find a better way to manage repeated responses
                //         // At the moment are annoying since you cannot restart the conversation from
                //         // a certain point
                //         int times;
                //         csTicks when;
                //         if (dialogHistory.EverSaid(client->GetPlayerID(), resp->id, when, times))
                //         {
                //             return RepeatedResponse(trigger.GetString(), resp, when, times);
                //         }
                //         else
                //             dialogHistory.AddToHistory(client->GetPlayerID(), resp->id, csGetTicks() );


                return resp; // Found what we are looking for
            }
        }
        depth++;
    } while (hit == true);

    return NULL;
}

NpcResponse *psNPCDialog::Respond(const char * text,Client *client)
{
    NpcResponse *resp;
    NpcTriggerSentence trigger,generalized;
    psString pstext(text);
    
    currentplayer = client->GetActor();
    currentClient = client;

    // Removes everything except alphanumeric character and spaces
    // Removes the NPC name
    CleanPunctuation(pstext);

    // Replace him/he,her/she,them/they,it with the stored
    // antecedent
    if (!CheckPronouns(pstext))
    {
        Debug2(LOG_NPC, currentClient->GetClientNum(),"Failed pronouns check for \"%s\"",pstext.GetDataSafe());
        return ErrorResponse(pstext,"(none)");
    }

    // Replace custom known terms to get standard terms
    // eg. hello is replaced with greetings
    FilterKnownTerms(pstext, trigger, client);

    if (trigger.TermLength() == 0)
    {
        Debug1(LOG_NPC, currentClient->GetClientNum(),"Failed filter known terms check");
        return ErrorResponse(pstext,"(no known words)");
    }

    csString copy;
    copy = trigger.GetString();
    dict->CheckForTriggerGroup(copy);  // substitute master trigger if this is child trigger in group

	// This is the generic version that does not use WordNet
	resp = FindResponse(copy, trigger.GetString());
	
    // Try each word (in reverse order) of the trigger by itself before giving up
    if (!resp)
    {
        WordArray words(trigger.GetString());
        for (size_t i=words.GetCount(); i>0; i--)
        {
            csString word(words.Get(i-1));
            Debug2(LOG_NPC, currentClient->GetClientNum(), "psNPCDialog::Respond: Trying word: '%s'\n", word.GetDataSafe());
    
            resp = FindResponse(word, text);
            if (resp)
            {
                resp->triggerText = trigger.GetString();
                break;
            }
        }
    }

    if (resp)
    {
        Debug2(LOG_NPC, currentClient->GetClientNum(),"Found response to: '%s'", copy.GetData());

        //         // Removed till we find a better way to manage repeated responses
        //         // At the moment are annoying since you cannot restart the conversation from
        //         // a certain point
        //         int times;
        //         csTicks when;
        //         if (dialogHistory.EverSaid(client->GetPlayerID(), resp->id, when, times))
        //         {
        //             return RepeatedResponse(trigger.GetString(), resp, when, times);
        //         }
        //         else
        //             dialogHistory.AddToHistory(client->GetPlayerID(), resp->id, csGetTicks() );

        //May be it is safe not to check for characterdata (now needed for GetLastRespons())
        if (currentClient->GetCharacterData() == NULL)
        {
            Error1("NpcResponse *psNPCDialog::Respond(const char * text,Client *client) called with "
                "currentClient->GetCharacterData() returning NULL.");
        }
        else
        {
            Debug3(LOG_NPC, currentClient->GetClientNum(),"Setting last response %d: %s",resp->id,resp->GetResponse());
            currentClient->GetCharacterData()->SetLastResponse(resp->id);
            Debug4(LOG_NPC, currentClient->GetClientNum(),"Setting last response for quest %d, %d: %s",
               resp->quest,resp->id,resp->GetResponse());
            currentClient->GetCharacterData()->SetAssignedQuestLastResponse(resp->quest,resp->id);
        }
    }
    else
    {
        Debug1(LOG_NPC, currentClient->GetClientNum(),"No response found");
        return ErrorResponse(pstext,trigger.GetString() );
    }
}

NpcResponse *psNPCDialog::FindXMLResponse(Client *client, csString trigger)
{
    if(!client) return NULL;
    currentplayer = client->GetActor();
    currentClient = client;

    return FindResponse(trigger, trigger.GetDataSafe());
}

NpcResponse *psNPCDialog::RepeatedResponse(const psString& text, NpcResponse *resp, csTicks when, int times)
{
    const char *time_description;
    if (when < 30000)  // 30 seconds
        time_description = "just now";
    else if (when < 300000) // 5 minutes
        time_description = "recently";
    else
        time_description = "already";

    csString key;
    key.Format("repeat %s %d",time_description,times);
    resp = FindResponse(key,text);
    if (!resp)
    {
        key.Format("repeat %s",time_description);
        resp = FindResponse(key,text);
        if (!resp)
        {
            key = "repeat";
            resp = FindResponse(key,text);
        }
    }
    return resp;
}

NpcResponse *psNPCDialog::ErrorResponse(const psString & text, const char *trigger)
{
    AddBadText(text,trigger);

    psString error("error");
    NpcResponse *resp = FindResponse(error,text);

    // save the trigger that didn't work for possible display to devs
    if (resp)
        resp->triggerText = trigger;

    return resp;
}

void psNPCDialog::UpdateAntecedents(NpcResponse *resp)
{
    // later this will need to be kept on a per player basis
    // but for now, its just one set

    if (resp->her.Length())
        antecedent_her = resp->her;

    if (resp->him.Length())
        antecedent_him = resp->him;

    if (resp->it.Length())
        antecedent_it = resp->it;

    if (resp->them.Length())
        antecedent_them = resp->them;

}

bool psNPCDialog::AddWord(const char *)
{
    return false;
}

bool psNPCDialog::AddSynonym(const char *,const char *)
{
    return false;
}

bool psNPCDialog::AddKnowledgeArea(const char *)
{
    return false;
}

bool psNPCDialog::AddResponse(const char *area,const char *words,const char *response,const char *minfaction)
{
    (void) area;
    (void) words;
    (void) response;
    (void) minfaction;
    return false;
}

bool psNPCDialog::AssignNPCArea(const char *npcname,const char *areaname)
{
    (void) npcname;
    (void) areaname;
    return false;
}


// void psNPCDialog::AddNewTrigger( int databaseID )
// {
//     if ( !dict )
//         return;
//     else
//         dict->AddTrigger( db, databaseID );
// }

// void psNPCDialog::AddNewResponse ( int databaseID )
// {
//     if ( !dict )
//         return;
//     else
//         dict->AddResponse( db, databaseID );
// }



void DialogHistory::AddToHistory(int playerID, int responseID, csTicks when)
{
    DialogHistoryEntry entry;
    entry.playerID  = playerID;
    entry.responseID= responseID;
    entry.when      = when;

    if (history.GetSize() < MAX_HISTORY_LEN)
        history.Push(entry);
    else
    {
        history.Put(counter,entry);
        counter++;
        if (counter == MAX_HISTORY_LEN)
            counter=0;
    }
}

bool DialogHistory::EverSaid(int playerID, int responseID, csTicks& howLongAgo, int& times)
{
    times      = 0;
    howLongAgo = 0;

    for (size_t i=0; i<history.GetSize(); i++)
    {
        if (history[i].playerID   == playerID &&
            history[i].responseID == responseID)
        {
            times++;
            if (history[i].when > howLongAgo)
                howLongAgo = history[i].when;
        }
    }
    if (howLongAgo)
        howLongAgo = csGetTicks() - howLongAgo;  // need the delta here

    return (times > 0);
}


