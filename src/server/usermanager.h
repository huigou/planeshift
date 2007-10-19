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

///////////////////////////////////////////////////////////////////////////////
//  PLANESHIFT INCLUDES
///////////////////////////////////////////////////////////////////////////////
#include "net/messages.h"           // Message definitions
#include "net/msghandler.h"         // Network access
#include "client.h"                 // Client, and ClientConnectionSet classes
#include "msgmanager.h"             // Parent class
#include "util/psdatabase.h"        // Database
#include "bulkobjects/pscharacter.h" // Stances

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
///////////////////////////////////////////////////////////////////////////////
class EntityManager;
class psCombatManager;
class psServer;
class AdminManager;
class ClientConnectionSet;
class psDatabase;
class EventManager;
class psCombatManager;
class PendingDuelInvite;
class AdviceManager;
class StatSet;
/** Used to manage incomming user commands from a client. Most commands are in
 * the format of /command param1 param2 ... paramN
 */
class UserManager : public MessageManager
{
public:

    UserManager(ClientConnectionSet *pCCS);
    virtual ~UserManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client);

    /** Send a notification to all clients on a person buddy list if they log on/off.
     * This does a database hit.
     *
     * @param client The client that has logged in/out.
     * @param loggedon True if the player has logged on. False if logged off.
     */
    void NotifyBuddies(Client * client, bool loggedon);

    enum
    {
        LOGGED_OFF = 0,
        LOGGED_ON  = 1
    };

     /** Send a buddy list to a player.
      * Sends a list of all the players that are currently on a player's buddy list.
      *
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      * @param filter True if show only buddies online. Else show all buddies.
      */
    void BuddyList(Client *client,int clientnum,bool filter);

    void UserStatRegeneration();
    void Ready();

    /** This is called by the Pending Invite if the duel is accepted.
      *
      * @param invite This is the invitemanager structure used to invoke the invitation.
      */
    void AcceptDuel(PendingDuelInvite *invite);

    /** Sends detail information about 'charData' to 'client'.
      * If 'full' is true, it contains info about HP and basic stats like Strength.
      */
    void SendCharacterDescription(Client * client, psCharacter * charData, bool full, bool simple, const csString & requestor);

    void Attack(Stance stance, Client *client,int clientnum);

    /** Handles a /loot command from a player to loot something.
      * Sends the lootable item to the client and splits any loot money across
      * a group ( if present ).
      *
      * @param client The client where the loot command came from.
      */
    void HandleLoot(Client *client);

	// Load emotes from xml.
	bool LoadEmotes(const char *xmlfile, iVFS *vfs);

protected:
    /** Send a list of the players that are online to a client.
      * Sends the name/guild/rank of all players in the world.
      *
      * @param msg The incomming user command message (unused)
      * @param client The client that request the /who (unused)
      * @param clientnum The client id number of the requesting client.
      */
    void Who(psUserCmdMessage& msg, Client* client, int clientnum);

    /** Converts a string to lowercase.
      *
      * @param str The string.
      */
    void StrToLowerCase(csString& str);

    /** Adds a person to a player's buddy list.
      * This does a database hit to add to the buddy table.

      * @param msg The incomming user command message.
      * @param client The client that request the /buddy.
      * @param clientnum The client id number of the requesting client.
      */
    void Buddy(psUserCmdMessage& msg,Client *client,int clientnum);

    /** Removes a person to a player's buddy list.
      * This does a database hit to remove from the buddy table.

      * @param msg The incomming user command message.
      * @param client The client that request the /notbuddy.
      * @param clientnum The client id number of the requesting client.
      */
    void NotBuddy(psUserCmdMessage& msg,Client *client,int clientnum);


   
    enum
    {
        ALL_PLAYERS=0,
        PLAYER_BUDDIES=1
    };


    /** Calculates a dice roll from a player based on number of die and sides.

      * @param msg The incomming user command message.
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      */
    void RollDice(psUserCmdMessage& msg,Client *client,int clientnum);


    /** Sends the player their current position and sector.

      * @param msg The incomming user command message.
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      */
    void ReportPosition(psUserCmdMessage& msg,Client *client,int clientnum);


    /** Moves a player back to the default start point for their race.

      * @param msg The incomming user command message.
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      */
    void MoveToSpawnPos(psUserCmdMessage& msg,Client *client,int clientnum);

    /** Moves a player back to his last valid position.

      * @param msg The incomming user command message.
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      */
    void HandleUnstick(psUserCmdMessage& msg,Client *client,int clientnum);

    /** Helper function to log a stuck character's details.
     *
     * @param The client that requested to be unstuck.
     */
    void LogStuck(Client* client);

    /** Helper function to make the character stop attacking.
     *
     * @param client The client that needs to stop attacking.
     */
    void StopAllCombat(Client* client);

    /** Command to start attacking something. Starts the combat manager
        working.

      * @param msg The incomming user command message.
      * @param client The client that request the command..
      * @param clientnum The client id number of the requesting client.
      */
    void HandleAttack(psUserCmdMessage& msg,Client *client,int clientnum);

    /** Command to challenge someone to a duel.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command..
      */
    void ChallengeToDuel(psUserCmdMessage& msg,Client *client);

    /** Command to surrender to someone in a duel.
      *
      * @param msg The incoming user command message.
      * @param client The client that request the command..
      */
    void YieldDuel(Client *client);


    /** Sends a client a list of their current assigned quests.
     *
     * @param client The requesting client.
     */
    void HandleQuests(Client *client);

    /** Sends a client a list of their current assigned event.
     * @param client: the requesting client.
     */
    void HandleGMEvents(Client* client);

    /** Give a tip from the database to the client
      *
      * @param id The id of the client who wants the tip
      */
    void GiveTip(int id);

    /** Sends the MOTD  to the client
      *
      * @param id The id of the client
      */
    void GiveMOTD(int id);


    /** Handles a player command to start training with targeted entity.
      *
      * @param client The client that issued the command.
      */
    void HandleTraining(Client *client);

    /** Handles a player command to start banking with the targeted entity.
      * @param client The client that issued the command.
      */
    void HandleBanking(Client *client, csString accountType);

    /** Handles a player request to 'use' the targeted item.
      *
      * @param client The client that issued the command.
      * @param on Toggle for start/stop using.
      */
    void HandleUse(Client *client, bool on);

    /** Handles an /Assist command comming from the client.
     * @param msg The incomming command message
     * @param client A pointer to the client struct.
     * @param clientnum The id number of this client.
     */
    void Assist( psUserCmdMessage& msg, Client* client, int clientnum );

    /** Teleport player to a location.
     *
     *  @param client The player to move
     *  @param x,y,z  Location to move to
     *  @param w      Instance to move to
     *  @param rot    The rotation to use.
     *  @param sector The sector name to move to.
     */
    void Teleport( Client *client, float x, float y, float z, int instance, float rot, const char* sectorname );

    void HandleMOTDRequest(MsgEntry *me,Client *client);
    void HandleUserCommand(MsgEntry *me,Client *client);
    void HandleCharDescUpdate(MsgEntry *me,Client *client);
    void HandleCharDetailsRequest(MsgEntry *me,Client *client);
    void HandleTargetEvent(MsgEntry *me);
    void HandleEntranceMessage( MsgEntry* me, Client *client );

    void SwitchAttackTarget(Client *targeter, Client *targeted);

	/** Process an emote command.
	 *
	 *  @param general   The phrase to broadcast if no target is selected.
	 *  @param specific  The phrase to broadcast if a target is selected.
	 *  @param animation The animation for the emote. If there isn't one pass "noanim".
	 *  @param range     The range of the broadcast.
	 *  @param target    The target, if there is one.
	 */
	void Emote(csString general, csString specific, csString animation, MsgEntry *me, Client *client);

	/** Check to see if command is an emote.
     *
     *  @param command  The command in question.
     *  @param execute  Execute the emote or not.
     */
	bool CheckForEmote(csString command, bool execute, MsgEntry *me, Client *client);

	// Struct to hold our emote data.
    struct EMOTE {
    csString command;
    csString general;
    csString specific;
    csString anim;
    };

	csArray<EMOTE> emoteList;

    ClientConnectionSet     *clients;
//    psDatabase              *database;
//    csRandomGen             *randomgen;
//    EventManager            *eventmanager;
//    psCombatManager         *combatmanager;
//    psServer                *server;
//    AdminManager            *adminmanager;
    csTicks                  nextUserStatRegeneration;
};

#endif
