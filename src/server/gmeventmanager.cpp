/** gmeventmanager.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Manages Game Master events for players.
 */

#include <psconfig.h>
#include "globals.h"
#include "gmeventmanager.h"
#include "util/psdatabase.h"
#include "util/log.h"
#include "clients.h"
#include "gem.h"
#include "net/messages.h"
#include "util/eventmanager.h"
#include "cachemanager.h"
#include "entitymanager.h"

GMEventManager::GMEventManager()
{
    nextEventID = 0;

    // initialise gmEvents
    gmEvents.DeleteAll();
    
    psserver->GetEventManager()->Subscribe(this, MSGTYPE_GMEVENT_INFO, REQUIRE_READY_CLIENT);
}

GMEventManager::~GMEventManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GMEVENT_INFO);

    for (size_t e = 0; e < gmEvents.GetSize(); e++)
    {
        gmEvents[e]->playerID.DeleteAll();
        delete gmEvents[e];
    }
    gmEvents.DeleteAll();
}

bool GMEventManager::Initialise(void)
{
    GMEvent* ongoingGMEvent;

    // load any existing gm events from database
    Result events(db->Select("SELECT * from gm_events order by id"));
    if (events.IsValid())
    {
        for (unsigned long e=0; e<events.Count(); e++)
        {
            ongoingGMEvent = new GMEvent;
            ongoingGMEvent->id = events[e].GetInt("id");
            ongoingGMEvent->status = static_cast<GMEventStatus>(events[e].GetInt("status"));
            ongoingGMEvent->gmID = events[e].GetInt("gm_id");
            ongoingGMEvent->eventName = csString(events[e]["name"]);
            ongoingGMEvent->eventDescription = csString(events[e]["description"]);
            gmEvents.Push(ongoingGMEvent);

            // setup next available id
            if (ongoingGMEvent->id >= nextEventID)
                nextEventID = ongoingGMEvent->id + 1;
        }

        // load registered players from database
        Result registeredPlayers(db->Select("SELECT * from character_events order by event_id"));
        if (registeredPlayers.IsValid())
        {
            int eventPlayerID;
            int eventID;
            for (unsigned long rp=0; rp<registeredPlayers.Count(); rp++)
            {
                eventID = registeredPlayers[rp].GetInt("event_id");
                eventPlayerID = registeredPlayers[rp].GetInt("player_id");
                if ((ongoingGMEvent = GetGMEventByID(eventID)) != NULL)
                    ongoingGMEvent->playerID.Push(eventPlayerID);
                else
                {
                    Error1("GMEventManager: gm_events / character_events table mismatch.");
                    return false;        // ermm.. somethings gone wrong with the DB!!!
                }
            }
        }
        else
        {
            Error1("GMEventManager: character_events table is not valid.");
            return false;
        }

        return true;
    }

    Error1("GMEventManager: gm_events table is not valid.");
    return false;
}

/// GameMaster creates a new event for players.
bool GMEventManager::AddNewGMEvent (Client* client, csString eventName, csString eventDescription)
{
    int newEventID, zero=0;
    int gmID = client->GetPlayerID();
    int clientnum = client->GetClientNum();
    GMEvent* gmEvent;
    
    // if this GM already has an active event, he/she cant start another
    if ((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) != NULL)
    {
        psserver->SendSystemInfo(clientnum, 
                                 "You are already running the \'%s\' event.",
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    if (eventName.Length() > MAX_EVENT_NAME_LENGTH)
    {
        eventName.Truncate(MAX_EVENT_NAME_LENGTH);
        psserver->SendSystemInfo(client->GetClientNum(), 
                                 "Event name truncated to \'%s\'.",
                                 eventName.GetDataSafe());
    }

    
    // GM can register his/her event
    newEventID = GetNextEventID();

    // remove undesirable characters from the name & description
    csString escEventName, escEventDescription;
    db->Escape(escEventName, eventName.GetDataSafe());
    db->Escape(escEventDescription, eventDescription.GetDataSafe());

    // first in the database
    db->Command("INSERT INTO gm_events(id, gm_id, name, description, status) VALUES (%i, %i, '%s', '%s', %i)",
        newEventID,
        gmID,
       	escEventName.GetDataSafe(),
       	escEventDescription.GetDataSafe(),
        RUNNING);

    // and in the local cache.
    gmEvent = new GMEvent;
    gmEvent->id = newEventID;
    gmEvent->gmID = gmID;
    gmEvent->eventName = escEventName;
    gmEvent->eventDescription = escEventDescription;
    gmEvent->status = RUNNING;
    gmEvents.Push(gmEvent);

    // keep psCharacter upto date
    client->GetActor()->GetCharacterData()->AssignGMEvent(gmEvent->id,true);

    psserver->SendSystemInfo(client->GetClientNum(), 
                             "You have initiated a new event, \'%s\'.",
                             eventName.GetDataSafe());

    return true;
}

/// GM registers player into his/her event
bool GMEventManager::RegisterPlayerInGMEvent (int clientnum, int gmID, Client* target)
{
    int playerID, zero=0;
    GMEvent *gmEvent;
    
    // make sure GM is running an event, the player is valid and available. 
    if ((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }
    if (!target)
    {
        psserver->SendSystemInfo(clientnum, "Invalid target for registering in event.");
        return false;
    }
    if ((playerID = target->GetPlayerID()) == gmID)
    {
        psserver->SendSystemInfo(clientnum, "You cannot register yourself in your own event.");
        return false;
    }
    zero = 0;
    if (GetGMEventByPlayer(playerID, RUNNING, zero) != NULL)
    {
        psserver->SendSystemInfo(clientnum, "%s is already registered in an event.", target->GetName());
        return false;
    }
    if (gmEvent->playerID.GetSize() == MAX_PLAYERS_PER_EVENT)
    {
        psserver->SendSystemInfo(clientnum,
                                 "There are already %d players in the \'%s\' event.",
                                 MAX_PLAYERS_PER_EVENT,
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    // store the registration in db
    db->Command("INSERT INTO character_events(event_id, player_id) VALUES (%i, %i)",
        gmEvent->id,
        playerID);
    gmEvent->playerID.Push(playerID);

    // keep psCharacter up to date
    target->GetActor()->GetCharacterData()->AssignGMEvent(gmEvent->id,false);

    // player is available to join the valid GM event
    psserver->SendSystemInfo(clientnum, "%s is registered in the \'%s\' event.",
                             target->GetName(),
                             gmEvent->eventName.GetDataSafe());
    psserver->SendSystemInfo(target->GetClientNum(), "You are registered in the \'%s\' event.",
                             gmEvent->eventName.GetDataSafe());
    return true;
}

/// Register all players within range (upto 100m)
bool GMEventManager::RegisterPlayersInRangeInGMEvent (Client* client, float range)
{
    int clientnum = client->GetClientNum(), zero = 0;

    // make sure GM is running an event
    if (GetGMEventByGM(client->GetPlayerID(), RUNNING, zero) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }
    // check range is within max permissable
    if (range <= 0.0 || range > MAX_REGISTER_RANGE)
    {
       psserver->SendSystemInfo(clientnum,
                                "Range should be greater than 0m upto %.2fm to register participants.",
                                MAX_REGISTER_RANGE);
       return false;
    }

    csArray<PublishDestination>& participants = client->GetActor()->GetMulticastClients();

    // search for players in range & register them
    bool regResult;
    for (size_t i = 0; i < participants.GetSize(); i++)
    {
        Client *target = psserver->GetConnections()->Find(participants[i].client);
        if (target && target != client && target->IsReady() && participants[i].dist <= range)
        {
            regResult = RegisterPlayerInGMEvent (clientnum, client->GetPlayerID(), target);
        }
    }

    return true;
}

bool GMEventManager::ListGMEvents (Client* client)
{
    psserver->SendSystemInfo(client->GetClientNum(), "Event list");
    psserver->SendSystemInfo(client->GetClientNum(), "--------------------");
    for (unsigned int i = 0 ; i < gmEvents.GetSize() ; i++)
    {
        if (gmEvents[i]->status == RUNNING)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s", gmEvents[i]->eventName.GetData());
        }            
    }
    return true;
}

bool GMEventManager::CompleteGMEvent (Client* client, csString eventName)
{
    int zero = 0;
    GMEvent* theEvent = GetGMEventByName(eventName, RUNNING, zero);
    if (!theEvent)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Event %s wasn't found.", eventName.GetData());
        return false;
    }
    return CompleteGMEvent(client, theEvent->gmID);
}

/// GM completes their event
bool GMEventManager::CompleteGMEvent (Client* client, int gmID)
{
    int zero = 0;

    // if this GM does not have an active event, he/she can't end it.
    GMEvent* theEvent;
    int clientnum = client->GetClientNum();
    
    if ((theEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }

    // inform players
    ClientConnectionSet* clientConnections = psserver->GetConnections();
    Client* target;
    for (size_t p = 0; p < theEvent->playerID.GetSize(); p++)
    {
        if ((target = clientConnections->FindPlayer(theEvent->playerID[p])))
        {
            // psCharacter
            target->GetActor()->GetCharacterData()->CompleteGMEvent(false);

            psserver->SendSystemInfo(target->GetClientNum(),
                                     "Event '%s' complete.",
                                     theEvent->eventName.GetDataSafe());
        }
    }

    // GMs psCharacter    
    client->GetActor()->GetCharacterData()->CompleteGMEvent(true);

    // flag the event complete
    db->Command("UPDATE gm_events SET status = %d WHERE id = %d", COMPLETED, theEvent->id);
    theEvent->status = COMPLETED;
    
    psserver->SendSystemInfo(clientnum, "Event '%s' complete.", theEvent->eventName.GetDataSafe());

    return true;
}

/// GM removes player from incomplete event
bool GMEventManager::RemovePlayerFromGMEvent (int clientnum, int gmID, Client* target)
{
    int playerID, zero=0;
    GMEvent* gmEvent;
    GMEvent* playerEvent;
    
    // make sure GM is running an event, the player is valid and registered.
    if ((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }
    if (!target)
    {
        psserver->SendSystemInfo(clientnum, "Invalid target - cannot remove.");
        return false;
    }
    playerID = target->GetPlayerID();
    zero = 0;
    playerEvent = GetGMEventByPlayer(playerID, RUNNING, zero);
    if (!playerEvent || gmEvent->id != playerEvent->id)
    {
        psserver->SendSystemInfo(clientnum,
                                 "%s is not registered in your \'%s\' event.",
                                 target->GetName(),
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    // all tests pass, drop the player
    db->Command("DELETE FROM character_events WHERE player_id = %d AND event_id = %d",
                playerID, gmEvent->id);
    gmEvent->playerID.Delete(playerID);
    
    // keep psCharacter up to date
    target->GetActor()->GetCharacterData()->RemoveGMEvent();

    psserver->SendSystemInfo(clientnum, "%s has been removed from the \'%s\' event.",
                             target->GetName(),
                             gmEvent->eventName.GetDataSafe());
    psserver->SendSystemInfo(target->GetClientNum(), "You have been removed from the \'%s\' event.",
                             gmEvent->eventName.GetDataSafe());

    return true;
}

/// Reward player(s). Create reward item(s) and put in player's inventory if possible.
bool GMEventManager::RewardPlayersInGMEvent (Client* client,
                                             float range,
                                             unsigned short stackCount,
                                             csString itemName)
{
    GMEvent* gmEvent;
    int clientnum = client->GetClientNum(), zero = 0;

    // make sure GM is running an event
    if ((gmEvent = GetGMEventByGM(client->GetPlayerID(), RUNNING, zero)) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }
    // check range is within max permissable
    if (range != NO_RANGE && (range <= 0.0 || range > MAX_REGISTER_RANGE))
    {
       psserver->SendSystemInfo(clientnum,
                                "Range should be greater than 0m upto %.2fm to reward participants.",
                                MAX_REGISTER_RANGE);
       return false;
    }

    // retrieve base stats item
    psItemStats *basestats = CacheManager::GetSingleton().GetBasicItemStatsByName(
                                                                itemName.GetDataSafe());
    if (basestats == NULL)
    {
        psserver->SendSystemInfo(clientnum, "Reward \'%s\' not recognised.", itemName.GetDataSafe());
        Error2("'%s' was not found as a valid base item.", itemName.GetDataSafe());
        return false;
    }

    // reward ALL players
    ClientConnectionSet* clientConnections = psserver->GetConnections();
    Client* target;
    gemActor* clientActor = client->GetActor();
    for (size_t p = 0; p < gmEvent->playerID.GetSize(); p++)
    {
        if ((target = clientConnections->FindPlayer(gmEvent->playerID[p])))
        {
            if (range == NO_RANGE || clientActor->RangeTo(target->GetActor()) <= range)
            {
                if (RewardPlayer(target, stackCount, basestats))
                   psserver->SendSystemInfo(clientnum, "%s has been rewarded.", target->GetName());
                else
                   psserver->SendSystemInfo(clientnum, "%s has not been rewarded.", target->GetName());
            }
        }
    }

    return true;
}
 
/// return all events, running & complete, for a specified player
int GMEventManager::GetAllGMEventsForPlayer (int playerID,
                                             csArray<int>& completedEvents,
                                             int& runningEventAsGM,
                                             csArray<int>& completedEventsAsGM)
{
    int runningEvent = -1, id, index=0;
    GMEvent* gmEvent;
    
    completedEvents.DeleteAll();
    completedEventsAsGM.DeleteAll();

    if ((gmEvent = GetGMEventByGM(playerID, RUNNING, index)))
    {
        runningEventAsGM = gmEvent->id;
    }
    else
    {
        runningEventAsGM = -1;
    }

    index = 0;
    do
    {
        gmEvent = GetGMEventByGM(playerID, COMPLETED, index);
        if (gmEvent)
        {
            id = gmEvent->id;
            completedEventsAsGM.Push(id);
        }
    }
    while (gmEvent);
   
    index = 0; 
    gmEvent = GetGMEventByPlayer(playerID, RUNNING, index);
    if (gmEvent)
        runningEvent = gmEvent->id;

    index = 0;
    do
    {
        gmEvent = GetGMEventByPlayer(playerID, COMPLETED, index);
        if (gmEvent)
        {
            id = gmEvent->id;
            completedEvents.Push(id);
        }
    }
    while (gmEvent);

    return (runningEvent);
}

/// handle message from client
void GMEventManager::HandleMessage(MsgEntry* me, Client* client)
{
    // request from client for the events description
    if (me->GetType() == MSGTYPE_GMEVENT_INFO)
    {
        //int gmID;
        psGMEventInfoMessage msg(me);

        if (msg.command == psGMEventInfoMessage::CMD_QUERY)
        {
            GMEvent* theEvent;    
            if ((theEvent = GetGMEventByID(msg.id)) == NULL)
            {
                Error3("Client %s requested unavailable GM Event %d", client->GetName(), msg.id);
                return;
            }

            csString eventDesc(theEvent->eventDescription);

            if (theEvent->status != EMPTY)
            {
                ClientConnectionSet* clientConnections = psserver->GetConnections();
                Client* target;

                // if this client is the GM, list the participants too
                if (client->GetPlayerID() == theEvent->gmID)
                {		    
                    eventDesc.AppendFmt(". Participants: %zu. Online: ", theEvent->playerID.GetSize());
                    csArray<unsigned int>::Iterator iter = theEvent->playerID.GetIterator();
                    while (iter.HasNext())
                    {
                        if ((target = clientConnections->FindPlayer(iter.Next())))
                        {
                            eventDesc.AppendFmt("%s, ", target->GetName());
                        }
                    }
                }
                else // and name the running GM
                {
                    if ((target = clientConnections->FindPlayer(theEvent->gmID)))
                    {
                        eventDesc.AppendFmt(" (%s)", target->GetName());
                    }		    
                }
		
                psGMEventInfoMessage response(me->clientnum, 
                                              psGMEventInfoMessage::CMD_INFO,
                                              msg.id,
                                              theEvent->eventName.GetDataSafe(),
                                              eventDesc.GetDataSafe());
                response.SendMessage();
            }
            else
            {
                Error3("Client %s requested unavailable GM Event %d", client->GetName(), msg.id);
            }
        }
    }
}
     
/// remove players complete references to GM events they were involved with
bool GMEventManager::RemovePlayerFromGMEvents(int playerID)
{
    int runningEventIDAsGM;
    int runningEventID, gmEventID;
    csArray<int> completedEventIDsAsGM;
    csArray<int> completedEventIDs;
    GMEvent* gmEvent;
    bool eventsFound = false;

    runningEventID = GetAllGMEventsForPlayer(playerID,
                                             completedEventIDs,
                                             runningEventIDAsGM,
                                             completedEventIDsAsGM);

    // remove if partaking in an ongoing event
    if (runningEventID >= 0)
    {
        gmEvent = GetGMEventByID(runningEventID);
        if (gmEvent)
        {
            gmEvent->playerID.Delete(playerID);
            eventsFound = true;
        }
        else
            Error3("Cannot remove player %d from GM Event %d.", playerID, runningEventID);
    }
    // remove ref's to old completed events
    csArray<int>::Iterator evIter = completedEventIDs.GetIterator();
    while(evIter.HasNext())
    {
        gmEventID = evIter.Next();
        gmEvent = GetGMEventByID(gmEventID);
        if (gmEvent)
        {
            gmEvent->playerID.Delete(playerID);
            eventsFound = true;
         }
         else
            Error3("Cannot remove player %d from GM Event %d.", playerID, runningEventID);
    }
    // ...and from the DB too
    if (eventsFound)
        db->Command("DELETE FROM character_events WHERE player_id = %d", playerID);
 
    // if this is a GM whats running an event, remove players from it first
    // before removing GM themself
    if (runningEventIDAsGM >= 0)
    {
        gmEvent = GetGMEventByID(runningEventIDAsGM);
        if (gmEvent)
        {
            gmEvents.Delete(gmEvent);
            db->Command("DELETE FROM character_events WHERE event_id = %d", runningEventIDAsGM);
            db->Command("DELETE FROM gm_events WHERE id = %d", runningEventIDAsGM);
            delete gmEvent;
        }
        else
            Error3("Cannot remove GM Event %d after loss of GM %d.", runningEventID, playerID);
    }
    evIter = completedEventIDsAsGM.GetIterator();
    while(evIter.HasNext())
    {
        gmEventID = evIter.Next();
        gmEvent = GetGMEventByID(gmEventID);
        if (gmEvent)
        {
            gmEvents.Delete(gmEvent);
            db->Command("DELETE FROM character_events WHERE event_id = %d", gmEventID);
            db->Command("DELETE FROM gm_events WHERE id = %d", gmEventID);
            delete gmEvent;
        }
        else
            Error3("Cannot remove GM Event %d after loss of GM %d.", gmEventID, playerID);
     }

    return true;
}

/// returns details of an event
GMEventStatus GMEventManager::GetGMEventDetailsByID (int id, 
                                                     csString& name,
                                                     csString& description)
{
    GMEvent* gmEvent = GetGMEventByID(id);

    if (gmEvent)
    {
        name = gmEvent->eventName;
        description = gmEvent->eventDescription;
        return gmEvent->status;
    }

    // ooops - some kinda cockup
    name = "?";
    description = "?";
    return EMPTY;
}

/// returns the specified GM Event
GMEventManager::GMEvent* GMEventManager::GetGMEventByID(int id)
{
    for (size_t e = 0; e < gmEvents.GetSize(); e++)
    {
        if (gmEvents[e]->id == id)
            return gmEvents[e];
    }

    return NULL;
}

/// returns a RUNNING event for a GM, or NULL
GMEventManager::GMEvent* GMEventManager::GetGMEventByGM(unsigned int gmID, GMEventStatus status, int& startIndex)
{
    for (size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if (gmEvents[e]->gmID == gmID && gmEvents[e]->status == status)
            return gmEvents[e];
    }

    return NULL;
}

/// returns a RUNNING event by name or NULL
GMEventManager::GMEvent* GMEventManager::GetGMEventByName(csString eventName, GMEventStatus status, int& startIndex)
{
    for (size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if (gmEvents[e]->eventName == eventName && gmEvents[e]->status == status)
            return gmEvents[e];
    }

    return NULL;
}

/// get the index into the gmEvents array for the next event of a specified
/// status for a particular player. Note startIndex will be modified upon
/// return.
GMEventManager::GMEvent* GMEventManager::GetGMEventByPlayer(unsigned int playerID, GMEventStatus status, int& startIndex)
{    
    for (size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if (gmEvents[e]->status == status)
        {
            csArray<unsigned int>::Iterator iter = gmEvents[e]->playerID.GetIterator();
            while (iter.HasNext())
            {
                if (iter.Next() == playerID)
                {
                    return gmEvents[e];
                }
            }
        }
    }

    return NULL;
}

/// reward an individual player.
bool GMEventManager::RewardPlayer(Client* target, unsigned short stackCount, psItemStats* basestats)
{
    // generate the prize item
    psItem* newitem = basestats->InstantiateBasicItem(true);
    if (newitem == NULL)
    {
        Error1("Could not instantiate from base item.");
        return false;
    }
    newitem->SetItemQuality(basestats->GetQuality());
    newitem->SetStackCount(stackCount);

    // spawn the item into the recipients inventory
    newitem->SetLoaded();  // Item is fully created
    if (target->GetActor()->GetCharacterData()->Inventory().Add(newitem))
    {
        // inform recipient of their prize
        psserver->SendSystemInfo(target->GetClientNum(), 
                                 "You have been rewarded for participating in this GM event.");

        return true;
    }

    // failed to stash item, so remove it
    CacheManager::GetSingleton().RemoveInstance(newitem);

    return false;
}

/// allocates the next GM event id
int GMEventManager::GetNextEventID(void)
{
    // TODO this is just too simple
    return nextEventID++;
}

