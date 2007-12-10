/*
 * progressionmanager.h
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
#ifndef __PROGRESSIONMANAGER_H__
#define __PROGRESSIONMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/hash.h>
#include <iutil/document.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/prb.h"                   // Red Black tree template class
#include "bulkobjects/psskills.h"       // Red Black tree template class

//=============================================================================
// Application Includes
//=============================================================================
#include "msgmanager.h"                 // Subscriber class

class psServer;
class ProgressionOperation;
class psGUISkillMessage;
class MathScriptVar;
class MathScript;

struct ProgressionEvent;
struct Faction;


class ProgressionManager : public MessageManager
{
public:

    ProgressionManager(ClientConnectionSet *ccs);

    virtual ~ProgressionManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client);
    void HandleSkill(Client * client, psGUISkillMessage& msg);
    
    /** Send the skill list to the client. 
      * @param client The client that the message is for.
      * @param forceOpen  If true it will force open the skills screen on the client.
      */
    void SendSkillList(Client * client, bool forceOpen, PSSKILL focus = PSSKILL_NONE, bool isTraining = false);

    void StartTraining(Client * client, psCharacter * trainer);
    
    float ProcessEvent(ProgressionEvent *event, gemActor * actor = NULL, gemObject *target = NULL, bool inverse = false);
    float ProcessEvent(const char *event, gemActor * actor = NULL, gemObject *target = NULL, bool inverse = false);
    float ProcessScript(const char *script, gemActor * actor = NULL, gemObject *target = NULL);

    bool AddScript(const char *name, const char *script);
    void QueueUndoScript(const char *script, int delay, gemActor * actor = NULL, gemObject *target = NULL, int persistentID = 0);

    Faction *FindFaction(const char *name);
    Faction *FindFaction(int id);

    csHash< csString, csString> &GetAffinityCategories() { return affinitycategories; }

    // Internal utility functions for the progression system
    void QueueEvent(psGameEvent *event);
    void SendMessage(MsgEntry *me);
    void Broadcast(MsgEntry *me);

    void ChangeScript( csString& script, int param, const char* change );
    ProgressionEvent *FindEvent(char const *name);
    
    /**
     * Create a ProgressionEvent script from a script input.
     *
     * @param name Template for the new name. A random number XXXX will be appended
     *             until a uniq name is found that dosn't exist in the store.
     */
    ProgressionEvent *CreateEvent(const char *name, const char *script);
     
protected:
    
    void Initialize();
    void HandleDeathEvent(MsgEntry *me);

    csHash<csString, csString> affinitycategories;

    BinaryRBTree<ProgressionEvent> events;

    ClientConnectionSet    *clients;
};


//-----------------------------------------------------------------------------

class ProgressionDelay : public psGameEvent
{
private:
    ProgressionEvent * progEvent;
    unsigned int client;

public:
    ProgressionDelay(ProgressionEvent * progEvent, csTicks delay, unsigned int clientnum);
    virtual ~ProgressionDelay();
    void Trigger(); 
};

//-----------------------------------------------------------------------------

struct ProgressionEvent
{
    // saved parameters given to Run() so that we can use callbacks for delayed events
    gemActor * runParamActor;
    gemObject * runParamTarget;
    bool runParamInverse;

    csString name;
    csArray<ProgressionOperation*> sequence;
    csArray<MathScriptVar*>        variables;

    MathScript * triggerDelay;
    MathScriptVar * triggerDelayVar;
    ProgressionDelay * progDelay;

    ProgressionEvent();
    virtual ~ProgressionEvent();

    bool LoadScript(iDocument *doc);
    bool LoadScript(iDocumentNode *topNode);
    virtual csString ToString(bool topLevel) const;
    float ForceRun();
    float Run(gemActor *actor, gemObject *target, bool inverse = false);
    void LoadVariables(MathScript *script);
    void CopyVariables(MathScript *from);
    void AddVariable(MathScriptVar *pv);
    MathScriptVar *FindVariable(const char *name);
    MathScriptVar *FindOrCreateVariable(const char *name);
    void SetValue( const char* name, double val );
    bool operator==(ProgressionEvent& other) const
    { return name == other.name; }
    bool operator<(ProgressionEvent& other) const
    { return strcmp(name,other.name)<0; }
    
    csString Dump() const;
};

#endif

