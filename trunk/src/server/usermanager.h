/*
 * usermanager.h - Author: Keith Fulton
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

/**
 * This implements the user command handler which handles who's online messages,
 * buddy lists, and so forth.
 */

#ifndef __USERMANAGER_H__
#define __USERMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
//#include "net/messages.h"           // Message definitions
//#include "net/msghandler.h"         // Network access
//
//#include "util/psdatabase.h"        // Database

//#include "bulkobjects/pscharacter.h" // Stances

//=============================================================================
// Local Includes
//=============================================================================
//#include "client.h"                 // Client, and ClientConnectionSet classes
#include "msgmanager.h"             // Parent class

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
///////////////////////////////////////////////////////////////////////////////
class EntityManager;
class CombatManager;
class psServer;
class AdminManager;
class ClientConnectionSet;
class psDatabase;
class EventManager;
class CombatManager;
class PendingDuelInvite;
class AdviceManager;
class StatSet;

/** Used to manage incoming user commands from a client. Most commands are in
 * the format of /command param1 param2 ... paramN
 */
class UserManager : public MessageManager
{
public:

    UserManager(ClientConnectionSet *pCCS);
    virtual ~UserManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client) { }

    /** @brief Send a notification to all clients on a person buddy list if they log on/off.
     *
     *  This does a database hit.
     *
     *  @param client The client that has logged in/out.
     *  @param loggedon True if the player has logged on. False if logged off.
     */
    void NotifyBuddies(Client * client, bool loggedon);

    /** @brief Send a notification to all clients on a person guild if they log on/off.
     *
     *  @param client The client that has logged in/out.
     *  @param loggedon True if the player has logged on. False if logged off.
     */
    void NotifyGuildBuddies(Client * client, bool loggedon);

    enum
    {
        LOGGED_OFF = 0,
        LOGGED_ON  = 1
    };

     /** @brief Send a buddy list to a player.
      *
      *  Sends a list of all the players that are currently on a player's buddy list.
      *
      *  @param client The client that request the command..
      *  @param clientnum The client id number of the requesting client.
      *  @param filter True if show only buddies online. Else show all buddies.
      */
    void BuddyList(Client *client,int clientnum,bool filter);

    void UserStatRegeneration();
    void Ready();

    /** @brief This is called by the Pending Invite if the duel is accepted.
      *
      * @param invite This is the invitemanager structure used to invoke the invitation.
      */
    void AcceptDuel(PendingDuelInvite *invite);

    /** @brief Sends detail information about 'charData' to 'client'.
      *
      * If 'full' is true, it contains info about HP and basic stats like Strength.
      */
    void SendCharacterDescription(Client *client, gemActor *actor, bool full, bool simple, const csString & requestor);

    void Attack(Stance stance, Client *client);

    void SendPlayerMoney(Client *client);

    
    /** @brief Handles a /loot command from a player to loot something.
      *
      * @param msg The incoming user command message
      * @param client The client that request the /who
      */
    void HandleLoot(psUserCmdMessage& msg,Client *client);
    
    /** @brief Attempt to loot the target
      *
      * Sends the lootable item to the client and splits any loot money across
      * a group ( if present ).
      *
      * @param msg The incoming user command message
      * @param client The client where the loot command came from.
      */
    void Loot(Client *client);

    /// Load emotes from xml.
    bool LoadEmotes(const char *xmlfile, iVFS *vfs);
    
     /** @brief Process an emote command.
     *
     *  @param general   The phrase to broadcast if no target is selected.
     *  @param specific  The phrase to broadcast if a target is selected.
     *  @param animation The animation for the emote. If there isn't one pass "noanim".
     */
    void Emote(csString general, csString specific, csString animation, Client *client);

protected:
    /** @brief Send a list of the players that are online to a client.
      *
      * Sends the name/guild/rank of all players in the world.
      *
      * @param msg The incoming user command message
      * @param client The client that request the /who
      */
    void Who(psUserCmdMessage& msg, Client* client);


    /** @brief Formats output of a player and adds it to a message.
      *
      * @param client The client that is to be formated
      * @param guildId The guildId of the caller
      * @param message The target csString object
      * @param filter What clients
      * @param check If a check against filter should be done
      * @param count COunter to be increased in case of a successfull match
      */
    bool WhoProcessClient(Client *curr, int guildId, csString* message, csString filter, bool check, unsigned * count);

    /** @brief Converts a string to lowercase.
      *
      * @param str The string.
      */
    void StrToLowerCase(csString& str);

    /** @brief Adds/removes a person to a player's buddy list.
      *
      * This does a database hit to add/remove to the buddy table.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the /buddy.
      */
    void Buddy(psUserCmdMessage& msg,Client *client);

    enum
    {
        ALL_PLAYERS=0,
        PLAYER_BUDDIES=1
    };


    /** @brief Calculates a dice roll from a player based on number of die and sides.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command.
      */
    void RollDice(psUserCmdMessage& msg,Client *client);


    /** @brief Sends the player their current position and sector.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command.
      */
    void ReportPosition(psUserCmdMessage& msg,Client *client);


    /* @brief Moves a player back to the default start point for their race.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command..
      *
    void MoveToSpawnPos(psUserCmdMessage& msg,Client *client,int clientnum); */

    /** @brief Moves a player back to his last valid position.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command.
      */
    void HandleUnstick(psUserCmdMessage& msg,Client *client);

    /** @brief Helper function to log a stuck character's details.
     *
     *  @param The client that requested to be unstuck.
     */
    void LogStuck(Client* client);

    /** @brief Helper function to make the character stop attacking.
     *
     *  @param client The client that needs to stop attacking.
     */
    void StopAllCombat(Client* client);

    /** @brief Command to start attacking something. Starts the combat manager
      * working.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command.
      */
    void HandleAttack(psUserCmdMessage& msg,Client *client);

    /** @brief Command to stop attacking your target
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command.
      */
    void HandleStopAttack(psUserCmdMessage& msg,Client *client);


    /** @brief Command to challenge someone to a duel.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command..
      */
    void ChallengeToDuel(psUserCmdMessage& msg,Client *client);

    /** @brief Command to surrender to someone in a duel.
      *
      * @param client The client that request the command..
      */
    void YieldDuel(Client *client);
    
    /** @brief Handle the yield command
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command..
      */
    void HandleYield(psUserCmdMessage& msg,Client *client);

    /// Handle the /starttrading command
    void StartTrading(psUserCmdMessage& msg,Client *client);

    /// Handle the /stoptrading command
    void StopTrading(psUserCmdMessage& msg,Client *client);

    /** @brief Command to marry/divorce someone
     * 
     * @param msg The incoming user command message.
     * @param client The client that request the command..
     */
    void HandleMarriage(psUserCmdMessage& msg,Client *client);

    /** @brief Sends a client a list of their current assigned quests.
     *
     *  @param client The requesting client.
     */
    void HandleQuests(Client *client);

    /** @brief Sends a client a list of their current assigned quests and events
     *
     *  @param msg The incoming user command message.
     *  @param client The requesting client.
     */
    void HandleQuestsCommand(psUserCmdMessage& msg,Client *client);

    /** @brief Sends a client a list of their current assigned event.
     *  @param client: the requesting client.
     */
    void HandleGMEvents(Client* client);

    /** @brief Give a tip from the database to the client
     *
     * @param msg The incoming user command message
     * @param the client who sit
     */
    void GiveTip(psUserCmdMessage& msg, Client *client);

    /** @brief Sends the MOTD  to the client
      *
      * @param msg The incoming user command message
      * @param the client who sit
      */
    void GiveMOTD(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to show the popup dialog menu of the currently targeted NPC, if any.
      *
      * @param msg The incoming user command message
      * @param client The client that issued the command.
      */
    void ShowNpcMenu(psUserCmdMessage& msg, Client *client);

    /**
     * @brief Handles a player command to sit down
     * 
     * @param msg The incoming user command message
     * @param the client who sit
     */
    void HandleSit(psUserCmdMessage& msg, Client *client);

    /** @brief Handles the /admin command
      * 
      * @param msg The incoming user command message
      * @param the client who sit
      */
    void HandleAdminCommand(psUserCmdMessage& msg, Client *client);

    /**
     * @brief Handles a player command to stand up
     * 
     * @param msg The incoming user command message
     * @param the client who stand
     */
    void HandleStand(psUserCmdMessage& msg, Client *client);
    
    /** @brief Handles a player command to die
     * 
     *  @param msg The incoming user command message
     *  @param the client who stand
     */
    void HandleDie(psUserCmdMessage& msg, Client *client);
    
    /** @brief Handles a player command to start training with targeted entity.
      *
      * @param the user command message
      * @param client The client that issued the command.
      */
    void HandleTraining(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to start banking with the targeted entity.
     * 
     * @param msg The incoming user command message
     * @param client The client that issued the command.
     */
    void HandleBanking(psUserCmdMessage& msg, Client *client);

    /// Handle the /pickup command, send it to Pickup()
    void HandlePickup(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to pickup an item.
      *
      * @param client The client that issued the command.
      * @param target description of the item to be picked up.
      */
    void Pickup(Client *client, csString target);

    /** @brief Handles a player command to mount.
	  *	
      * @param msg the user command message
	  * @param client The client that issued the command.
      */	
    void HandleMount(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to unmount.
	  *	
      * @param msg the user command message
	  * @param client The client that issued the command.
      */	
    void HandleUnmount(psUserCmdMessage& msg, Client *client);

    /// Handle the /guard command, send it to Guard()
    void HandleGuard(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to guard/unguard an item.
      *
      * If the action parameter is empty, the guarding status
      * of the item will be toggled.
      * 
      * @param client The client that issued the command.
      * @param object pointer to the item to be guarded/unguarded.
      * @param action value can be "on" or "off"
      */
    void Guard(Client *client, gemObject *object, csString action);

    /// Handle the /rotate command, send it to Rotate()
    void HandleRotate(psUserCmdMessage& msg, Client *client);

    /** @brief Handles a player command to rotate an item.
      *
      * @param client The client that issued the command.
      * @param target pointer to the item to be rotated.
      * @param action rotation data
      */
    void Rotate(Client *client, gemObject* target, csString action);

    /** @brief Handles a player request to 'use' the targeted item.
      *
      * @param client The client that issued the command.
      * @param on Toggle for start/stop using.
      */
    void HandleUse(Client *client, bool on);

    /** @brief Handles an /Assist command comming from the client.
     * 
     *  @param msg The incoming command message
     *  @param client A pointer to the client struct.
     */
    void Assist( psUserCmdMessage& msg, Client* client);

    /** @brief Sends to client the MOTD and tip, and his guild's MOTD if needed
     * 
     * @param me The incoming message
     * @param client the client to send the message to
     */
    void HandleMOTDRequest(MsgEntry *me,Client *client);
    
    /// Take a user command from a client, and send it to be handled by a function
    void HandleUserCommand(MsgEntry *me,Client *client);
    
    /// Handle a message to update a character's description
    void HandleCharDescUpdate(MsgEntry *me,Client *client);
    
    /// Handle a request to send a character description
    void HandleCharDetailsRequest(MsgEntry *me,Client *client);
    void HandleTargetEvent(MsgEntry *me,Client *client);
    void HandleEntranceMessage( MsgEntry* me, Client *client );

    void SwitchAttackTarget(Client *targeter, Client *targeted);

    /** @brief Check to see if command is an emote.
     *
     *  @param command  The command in question.
     *  @param execute  Execute the emote or not.
     */
    bool CheckForEmote(csString command, bool execute, Client *client);

    /// Struct to hold our emote data.
    struct EMOTE {
    csString command;
    csString general;
    csString specific;
    csString anim;
    };

    csHash<EMOTE *, csString> emoteHash;

    ClientConnectionSet     *clients;
//    psDatabase              *database;
//    csRandomGen             *randomgen;
//    EventManager            *eventmanager;
//    CombatManager         *combatmanager;
//    psServer                *server;
//    AdminManager            *adminmanager;

    /// pointer to member function typedef, improves readability
    typedef void (UserManager::*userCmdPointer)(psUserCmdMessage& msg, Client *client);
    /// Hash of the user commands, the key is the command name
    csHash<userCmdPointer, csString> userCommandHash;
};

#endif
