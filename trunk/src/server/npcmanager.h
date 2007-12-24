/*
* npcmanager.h by Keith Fulton <keith@paqrat.com>
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

#ifndef __NPCMANAGER_H_
#define __NPCMANAGER_H_
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/netbase.h"                // PublishVector class
#include "net/npcmessages.h"

#include "bulkobjects/psskills.h"    

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class


class Client;
class psDatabase;
class psNPCCommandsMessage;
class ClientConnectionSet;
class EntityManager;
class EventManager;
class gemObject;
class gemActor;
class gemNPC;
class PetOwnerSession;

class NPCManager : public MessageManager
{
public:
    NPCManager(ClientConnectionSet *pCCS,
        psDatabase *db,
        EventManager *evtmgr);

    virtual ~NPCManager();

    /// Handle incoming messages from the superclients.
    virtual void HandleMessage(MsgEntry *pMsg,Client *client);

    /// Send a list of managed NPCs to a newly connecting superclient.
    void SendNPCList(Client *client);

    /// Remove a disconnecting superclient from the active list.
    void Disconnect(Client *client);

    /// Communicate a entity added to connected superclients.
    void AddEntity(gemObject *obj);

    /// Communicate a entity going away to connected superclients.
    void RemoveEntity(MsgEntry *me);

    /// Build a message with all changed world positions for superclients to get.
    void UpdateWorldPositions();

    /// Let the superclient know that a player has said something to one of its npcs.
    void QueueTalkPerception(gemActor *speaker,gemNPC *target);

    /// Let the superclient know that a player has attacked one of its npcs.
    void QueueAttackPerception(gemActor *attacker,gemNPC *target);

    /// Let the superclient know that a player has taken HP from one of its npcs.
    void QueueDamagePerception(gemActor *attacker,gemNPC *target,float dmg);

    /// Let the superclient know that one of its npcs has died.
    void QueueDeathPerception(gemObject *who);

    /// Let the superclient know that a spell has been cast.
    void QueueSpellPerception(gemActor *caster, gemObject *target,const char *spell_cat, uint32_t spell_category, float severity);

    /// Let the superclient know that one enemy is close.
    void QueueEnemyPerception(psNPCCommandsMessage::PerceptionType type, gemActor *npc, gemActor *player, float relative_faction);

    /// Let the superclient know that one of its npcs has been commanded to stay.
    void QueueOwnerCmdStayPerception(gemActor *owner, gemNPC *pet);

    /// Let the superclient know that one of its npcs has been commanded to follow.
    void QueueOwnerCmdFollowPerception(gemActor *owner, gemNPC *pet);

    /// Let the superclient know that one of its npcs has been commanded to attack.
    void QueueOwnerCmdAttackPerception(gemActor *owner, gemNPC *pet);

    /// Let the superclient know that one of its npcs has been commanded to stop attacking.
    void QueueOwnerCmdStopAttackPerception(gemActor *owner, gemNPC *pet);

    /// Let the superclient know that one of its npcs has a change in inventory.
    void QueueInventoryPerception(gemActor *owner, psItem * itemdata, bool inserted);

    /// Let the superclient know that one of the actors flags has changed.
    void QueueFlagPerception(gemActor *owner);

    /// Let the superclient know that a response has commanded a npc.
    void QueueNPCCmdPerception(gemActor *owner, const csString& cmd);

    /// Let the superclient know that a transfer has happend.
    void QueueTransferPerception(gemActor *owner, psItem * itemdata, csString target);
    
    /// Send all queued commands and perceptions to active superclients and reset the queues.
    void SendAllCommands();

    /// Get the vector of active superclients, used in Multicast().
    csArray<PublishDestination>& GetSuperClients() { return superclients; }

    /// Send a newly spawned npc to a superclient to manage it.
    void NewNPCNotify(int player_id,int master_id,int owner_id);

    /// Tell a superclient to control an existing npc.
    void ControlNPC( gemNPC* npc );

    /// Tell a superclient the npc is ownered by another
    void SetNPCOwner(gemNPC *npc,int owner_id);

    /// Add Session for pets
    PetOwnerSession *CreatePetOwnerSession( gemActor *, psCharacter * );

    /// Remove Session for pets
    void RemovePetOwnerSession( PetOwnerSession *session );
    
    /// Updates time in game for a pet
    void UpdatePetTime();
    
protected:

    /// Handle a login message from a superclient.
    void HandleAuthentRequest(MsgEntry *me);

    /// Handle a network msg with a list of npc directives.
    void HandleCommandList(MsgEntry *me);

    /// Catch an internal server event for damage so a perception can be sent about it.
    void HandleDamageEvent(MsgEntry *me);

    /// Catch an internal server event for death so a perception can be sent about it.
    void HandleDeathEvent(MsgEntry *me);

    /// Send the list of maps for the superclient to load on startup.
    void SendMapList(Client *client);

    /// Send the list of races for the superclient to load on startup.
    void SendRaces(Client *client);

    /// Handle network message with pet directives
    void HandlePetCommand( MsgEntry *me );

    /// Handle network message with console commands from npcclient
    void HandleConsoleCommand(MsgEntry *me);

    /// Handle network message with pet skills
    void HandlePetSkill( MsgEntry * me );
    void SendPetSkillList( Client * client, bool forceOpen = true, PSSKILL focus = PSSKILL_NONE );

    /// Create an empty command list message, waiting for items to be queued in it.
    void PrepareMessage();

    /// List of active superclients.
    csArray<PublishDestination> superclients;

    psDatabase*  database;
    EventManager *eventmanager;
    ClientConnectionSet* clients;
    psNPCCommandsMessage *outbound;
    int cmd_count;

    BinaryRBTree<PetOwnerSession> OwnerPetList;
};

#endif
