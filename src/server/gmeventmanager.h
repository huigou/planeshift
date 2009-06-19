/** gmeventmanager.h
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

#ifndef __GMEVENTMANAGER_H__
#define __GMEVENTMANAGER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/csstring.h>
#include <csutil/array.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

#define MAX_PLAYERS_PER_EVENT 100
#define MAX_EVENT_NAME_LENGTH 40
#define MAX_REGISTER_RANGE    100.0
#define EVAL_LOCKOUT_TIME 24*60*60*1000 ///< how much time the player should be allowed to evalute an event (24 hours here)

#define UNDEFINED_GMID        0

#define SUPPORT_GM_LEVEL      GM_LEVEL_4

class psItemStats;

enum GMEventStatus
{
    EMPTY = 0,                 ///< no GM event
    RUNNING,                   ///< GM event is running
    COMPLETED                  ///< GM event is complete
};

#define NO_RANGE -1.0

enum RangeSpecifier
{
    ALL,
    IN_RANGE,
    INDIVIDUAL
};

/** GameMaster Events manager class.
 */
class GMEventManager : public MessageManager
{
public:
    GMEventManager();
    ~GMEventManager();

    bool Initialise(void);

    /** @brief GM attempts to add new event.
     *
     *  @param client: client pointer.
     *  @param eventName: event name.
     *  @param eventDescription: event description.
     *  @return bool: true = success, false = failed.
     */
    bool AddNewGMEvent (Client* client, csString eventName, csString eventDescription);

    /** @brief GM registers player into his/her event.
     *
     *  @param client: client pointer.
     *  @param target: registeree player client.
     *  @return bool: true = success, false = failed.
     */
    bool RegisterPlayerInGMEvent (Client* client, Client* target);

    /** @brief GM registers all players in range.
     *
     *  @param client: client pointer.
     *  @param range: required range.
     *  @return bool: true = success, false = failed.
     */
    bool RegisterPlayersInRangeInGMEvent (Client* client, float range);

    /** @brief A player completes an event.
     *
     *  @param client: client pointer.
     *  @param gmID: Game Master player ID.
     *  @param byTheControllerGM: true if it is the controlling GM
     *  @return bool: true = success, false = failed.
     */
    bool CompleteGMEvent (Client* client, PID gmID, bool byTheControllerGM = true);

    /** @brief A player completes an event.
     *
     *  @param client: client pointer.
     *  @param eventName: Name of the event.
     *  @return bool: true = success, false = failed.
     */
    bool CompleteGMEvent (Client* client, csString eventName);

    /** @brief Sends a list of all events to client.
     *
     *  @param client: client pointer.
     *  @return bool: true = success, false = failed.
     */
    bool ListGMEvents (Client* client);

    /** @brief A player is removed from a running event.
     *
     *  A player can be excused from finishing an event.
     *
     *  @param client: client pointer.
     *  @param target: registeree player client to be removed.
     *  @return bool: true = success, false = failed.
     */
    bool RemovePlayerFromGMEvent (Client* client, Client* target);

    /** @brief Reward players who complete an event
     *
     *  Event *must* be live at the time of reward).
     *
     *  @param client: client pointer.
     *  @param rewardRecipient: who will receive the reward.
     *  @param range: required range of winners (NO_RANGE = all participants).
     *  @param target: specific individual winner.
     *  @param stackCount: number of items to reward.
     *  @param itemName: name of the reward item.
     *  @return bool: true = success, false = failed.
     */
    bool RewardPlayersInGMEvent (Client* client,
                                 RangeSpecifier rewardRecipient,
                                 float range,
                                 Client *target,
                                 short stackCount,
                                 csString itemName);

    /** @brief Returns all events for a player.
     *
     *  @param playerID: the player identity.
     *  @param completedEvents: array of event ids of completed events player.
     *  @param runningEventAsGM: running event id as GM.
     *  @param completedEventsAsGM: array of ids of completed events as GM.
     *  @return int: running event id.
     */
    int GetAllGMEventsForPlayer (PID playerID,
                                 csArray<int>& completedEvents,
                                 int& runningEventAsGM,
                                 csArray<int>& completedEventsAsGM);

    /** @brief Returns event details for a particular event.
     *
     *  @param id: event id.
     *  @param name: name of event.
     *  @param description: description of event.
     *  @return GMEventStatus: status of event.
     */
     GMEventStatus GetGMEventDetailsByID (int id,
                                          csString& name,
                                          csString& description);

     virtual void HandleMessage(MsgEntry *me, Client *client) { };
     virtual void HandleGMEventCommand(MsgEntry *me, Client *client);

     /** @brief Removes a player from any GM event they maybe involved with (eg player being deleted)
      *
      *  @param playerID: id of player being removed
      *  @return bool: true = success
      */
     bool RemovePlayerFromGMEvents(PID playerID);

    /** @brief GM attempts to assume control of an event, after originator has absconded.
     *
     *  @param client: client pointer.
     *  @param eventName: event name.
     *  @return bool: true = success, false = failed.
     */
    bool AssumeControlOfGMEvent(Client* client, csString eventName);

    /** @brief GM discards an event of theirs by name; participants are removed, and it is wiped from the DB.
     *
     *  @param client: client pointer.
     *  @param eventName: event name.
     *  @return bool: true = success, false = failed.
     */
    bool EraseGMEvent(Client* client, csString eventName);

private:

    int nextEventID;

    struct PlayerData
    {
        PID PlayerID; ///< The playerID of this player who partecipated an event
        bool CanEvaluate; ///< Stores if this player is allowed to evaluate this event (more precisely if he didn't already)
    };

    struct GMEvent
    {
        int id; ///< The id of the event, corresponds to db ids
        GMEventStatus status; ///< The status of the event
        PID gmID; ///< The id of the gm running this event
        csTicks EndTime; ///< Stores the end time of the event. It's zero till the event is completed and after serve restart.
        csString eventName; ///< The name of the event
        csString eventDescription; ///< The description of the event
        csArray<PlayerData> Player; ///< An array with all the players who parteciped in the event and additional data about them.
    };
    csArray<GMEvent*> gmEvents;    ///< cache of GM events

    /** @brief find a GM event by its id.
     *
     *  @param id: id of event.
     *  @return GMEvent*: ptr to GM event structure.
     */
    GMEvent* GetGMEventByID(int id);

    /** @brief find a particular GM's event from them all.
     *
     *  @param gmID: player id of the GM.
     *  @param status: event of status looked for.
     *  @param startIndex: start index into array.
     *  @return GMEvent*: ptr to GM event structure.
     */
    GMEvent* GetGMEventByGM(PID gmID, GMEventStatus status, int& startIndex);

    /** @brief find a particular GM's event from them all.
     *
     *  @param eventName: Name of the event.
     *  @param status: event of status looked for.
     *  @param startIndex: start index into array.
     *  @return GMEvent*: ptr to GM event structure.
     */
    GMEvent* GetGMEventByName(csString eventName, GMEventStatus status, int& startIndex);

    /** @brief Find any event that a player may be/was registered to, returns index.
     *
     *  @param playerID: the player index.
     *  @param status: event of status looked for.
     *  @param startIndex: start index into array.
     *  @return GMEvent*: ptr to GM event structure.
     */
    GMEvent* GetGMEventByPlayer(PID playerID, GMEventStatus status, int& startIndex);

    /** @brief Return the index position of the particular player in an event
     *
     *  @param PlayerID: the player index.
     *  @param GMEvent*: ptr to GM event structure.
     *  @return size_t: the index position of the player
     */
    size_t GetPlayerFromEvent(PID& PlayerID, GMEvent *Event);

    /** @brief Checks if a Player is allowed to evaluate an event
     *  @note This is the function to change in order to change the rules
     *        which allow to evaluate an event.
     *
     *  @param PlayerID: the player index.
     *  @param GMEvent*: ptr to GM event structure.
     *  @return bool: true if the player is allowed to evaluate an event
     */
    bool CheckEvalStatus(PID PlayerID, GMEvent *Event);

    /** @brief Set if a player is allowed to evaluate an event
     *
     *  @param PlayerID: the player index.
     *  @param GMEvent*: ptr to GM event structure.
     *  @param NewStatus: true if the player is to be allowed to evaluate 
     *                    an event, else false
     */
    void SetEvalStatus(PID PlayerID, GMEvent *Event, bool NewStatus);

    /** @brief Reward player in event.
     *
     *  @param clientnum: GM client number.
     *  @param target: client pointer to recipient.
     *  @param stackCount: stack count # items in reward.
     *  @param basestats: base stats of reward item.
     */
    void RewardPlayer(int clientnum, Client* target, short stackCount, psItemStats* basestats);
    enum RewardType
    {
        REWARD_ITEM,
        REWARD_EXPERIENCE,
        REWARD_FACTION_POINTS
    };

    /** Get next free event id number.
     */
    int GetNextEventID(void);

    /** @brief Player discards an event
     *
     *  @param client: the player client
     *  @param eventID: id of the event
     */
    void DiscardGMEvent(Client* client, int eventID);

    /** @brief Remove player references to event.
     *
     *  @param gmEvent: the GMEvent*.
     *  @param client: the Client* to be removed.
     *  @param playerID: the player id to be removed.
     *  @return bool: true/false success.
     */
    bool RemovePlayerRefFromGMEvent(GMEvent* gmEvent, Client* client, PID playerID);

    /** @brief GM discards an event of theirs, by GMEvent*; participants are removed, and it is wiped from the DB.
     *
     *  @param client: client pointer.
     *  @param gmEvent: the event.
     *  @return bool: true = success, false = failed.
     */
    bool EraseGMEvent(Client* client, GMEvent* gmEvent);

    /** @brief Saves the comments from a player to the database
     *
     *  @param client: client pointer.
     *  @param gmEvent: the event.
     *  @return bool: true = success, false = failed.
     */
    void WriteGMEventEvaluation(Client* client, GMEvent* Event, csString XmlStr);
};

#endif

