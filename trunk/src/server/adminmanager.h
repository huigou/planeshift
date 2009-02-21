/*
 * adminmanager.h
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
#ifndef __ADMINMANAGER_H__
#define __ADMINMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"            // Chat Message definitions
#include "util/psconst.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class
#include "gmeventmanager.h"

///This is used for petitions to choose if it's a player or a gm asking
///to do a certain operation
#define PETITION_GM 0xFFFFFFFF

class psDatabase;
class psSpawnManager;
class psServer;
class psDatabase;
class EventManager;
class ClientConnectionSet;
class EntityManager;
class psSpawnManager;
class psAdminGameEvent;
class psNPCDialog;
class psItem;
struct Result;
class iResultSet;
class Client;
class psPathNetwork;
class psPath;
class Waypoint;

/// List of GM levels and their security level.
enum GM_LEVEL
{
    GM_DEVELOPER = 30,
    GM_LEVEL_9 = 29,
    GM_LEVEL_8 = 28,
    GM_LEVEL_7 = 27,
    GM_LEVEL_6 = 26,
    GM_LEVEL_5 = 25,
    GM_LEVEL_4 = 24,
    GM_LEVEL_3 = 23,
    GM_LEVEL_2 = 22,
    GM_LEVEL_1 = 21,
    GM_LEVEL_0 = 20,
    GM_TESTER  = 10
};

/** Admin manager that handles GM commands and general game control.
 */
class AdminManager : public MessageManager
{
public:
    AdminManager();
    virtual ~AdminManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client);

    /** This is called when a player does /admin.
      * This builds up the list of commands that are available to the player
      * at their current GM rank.  If commands need to be placed in different
      * GM levels then this function needs to be updated for that.
      */
    void Admin(int clientnum, Client *client);

    /** This sets the player as a Role Play Master and hooks them into the
      * admin commands they should have for their level.
      *
      */

    void AdminCreateNewNPC(csString& data);

    void AwardExperienceToTarget(int gmClientnum, Client* target, csString recipient, int ppAward);
    void AdjustFactionStandingOfTarget(int gmClientnum, Client* target, csString factionName, int standingDelta);

    /** Get sector and coordinates of starting point of a map. Returns success. */
    bool GetStartOfMap(Client *client, const csString & map, iSector * & targetSector,  csVector3 & targetPoint);

protected:

    bool IsReseting(const csString& command);

    struct AdminCmdData
    {
        csString player, target, command, subCmd, commandMod;
        csString action, setting, attribute, attribute2, skill;
        csString map, sector, direction;
        csString text, petition, reason;
        csString newName, newLastName;
        csString item, mesh;
        csString description, script;
        csString wp1, wp2;
        csString gmeventName, gmeventDesc;
        csString zombie, requestor;
        csString type,name; ///< Used by: /location

        int value, interval, random;
        int rainDrops, density, fade;
        unsigned int mins, hours, days;
        float amt, x, y, z, rot;
        bool uniqueName, uniqueFirstName,help,insert;
        float radius, range;
        unsigned short stackCount;
        InstanceID instance;
        bool instanceValid;
        RangeSpecifier rangeSpecifier;

        bool DecodeAdminCmdMessage(MsgEntry *pMsg, psAdminCmdMessage& msg, Client *client);
    };

    void CommandArea          (MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, int range);
    void HandleAdminCmdMessage(MsgEntry *pMsg, Client *client);
    void HandlePetitionMessage(MsgEntry *pMsg, Client *client);
    void HandleGMGuiMessage   (MsgEntry *pMsg, Client *client);
    void SpawnItemInv         ( MsgEntry* me, Client *client);
    void SpawnItemInv         ( MsgEntry* me, psGMSpawnItem& msg, Client *client);

    /** @brief Handles a request to reload a quest from the database.
     *  @param msg The text name is in the msg.text field.
     *  @param client The client we will send error codes back to.
     */
    void HandleLoadQuest(psAdminCmdMessage& msg, AdminCmdData& data, Client* client);

    /** @brief Get the list of characters in the same account of the provided one.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param targetobject A pointer to the targetted object for direct access
     *  @param duplicateActor If it the provided target is ambiguous this will be true
     *  @param client The GM client the command came from.
     */
    void GetSiblingChars(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, gemObject *targetobject, bool duplicateActor, Client *client);

    void GetInfo(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data,Client *client, gemObject* target);
    void CreateNPC(MsgEntry *me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor* basis);

    /** @brief Kill an npc in order to make it respawn, It can be used also to reload the data of the npc from the database
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param targetobject A pointer to the targetted object for direct access
     *  @param client The GM client the command came from.
     */
    void KillNPC(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, gemObject* targetobject, Client *client);
    void CreateItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void CreateMoney(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void ModifyKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void ChangeLock(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeUnlockable(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeSecurity(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, bool masterkey);
    void CopyKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, psItem* key);
    void AddRemoveLock(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client,psItem* key);

    /** @brief Runs a progression script on the targetted client
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     *  @param object The object we are going to run the progression script
     */
    void RunScript(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object);

    void CreateHuntLocation(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /** @brief Changes various parameters associated to the item
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     *  @param object The item of which we are going to edit parameters
     */
    void ModifyItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, gemObject* object);

    /** Move an object by a certain amount along one direction (up, down, left, right, forward, backward)
        or allow to turn an object around the y axis*/ 
    void Slide(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject *target);

    void Teleport(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject *subject);

    /** Get sector and coordinates of target of teleportation described by 'msg'.
        Return success */
    bool GetTargetOfTeleport(Client *client, psAdminCmdMessage& msg, AdminCmdData& data, iSector * & targetSector,  csVector3 & targetPoint, float &yRot, gemObject *subject, InstanceID &instance);

    /** Handles movement of objects for teleport and slide. */
    bool MoveObject(Client *client, gemObject *target, csVector3& pos, float yrot, iSector* sector, InstanceID instance);

    /** This function sends a warning message from a GM to a player, and displays it in
     *  big, red, un-ignorable text on the screen and in the chat window.
     */
    void WarnMessage(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /** This function kicks a player off the server as long as the calling client
     *  has sufficient privlidges.
     */
    void KickPlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    ///This function will mute a player until logoff
    void MutePlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    ///This function will unmute a player
    void UnmutePlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /// Kills by doing a large amount of damage to target.
    void Death( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor* target);

    /// Impersonate a character
    void Impersonate(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Set various GM/debug abilities (invisible, invincible, etc.)
    void SetAttrib(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Set the label color for char
    void SetLabelColor(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor * subject);

    /// Divorce char1 and char2, if they're married.
    void Divorce(MsgEntry* me, AdminCmdData& data);

    /** @brief Get the marriage info of a player.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param duplicateActor If it the provided target is ambiguous this will be true
     *  @param client The client of the targeted player
     */
    void ViewMarriage(MsgEntry* me, AdminCmdData& data, bool duplicateActor, Client *player);

    /// Add new Path point to DB
    int PathPointCreate(int pathID, int prevPointId, csVector3& pos, csString& sectorName);

    /// Lookup path information close to a point
    void FindPath(csVector3 & pos, iSector * sector, float radius,
                  Waypoint** wp, float *rangeWP,
                  psPath ** path, float *rangePath, int *indexPath, float *fraction,
                  psPath ** pointPath, float *rangePoint, int *indexPoint);

    /// Handle online path editing.
    void HandlePath( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /// Add new location point to DB
    int LocationCreate(int typeID, csVector3& pos, csString& sectorName, csString& name);

    /// Handle online path editing.
    void HandleLocation( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /// Handle action location entrances
    void HandleActionLocation(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Handles a user submitting a petition
    void HandleAddPetition(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data,Client *client);

    /// Handles broadcasting the petition list dirty signal
    void BroadcastDirtyPetitions(int clientNum, bool includeSelf=false);

    /// Handles queries sent by the client to the server for information or actions
    void ListPetitions(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);
    void CancelPetition(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);
    void ChangePetition(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);

    /// Handles queries send by a GM to the server for dealing with petitions
    void GMListPetitions(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);
    void GMHandlePetition(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);

    void SendGMPlayerList(MsgEntry* me, psGMGuiMessage& msg, Client *client);

    /** @brief Changes the name of the player to the specified one.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param targetobject A pointer to the targetted object for direct access
     *  @param duplicateActor If it the provided target is ambiguous this will be true
     *  @param client The GM client the command came from.
     */
    void ChangeName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, gemObject *targetobject, bool duplicateActor, Client *client);

    /// Handles a change to set the NPC's default spawn location.
    void UpdateRespawn(AdminCmdData& data, Client* client, gemActor* target);

    /// Controlls the rain / thunder and other weather
    void Weather(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Rain(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Snow(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Thunder(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Fog(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /** @brief Deletes a character from the database.
      *
      * Should be used with caution.
      * This function will also send out reasons why a delete failed. Possible
      * reasons are not found or the requester is not the same account as the
      * one to delete.  Also if the character is a guild leader they must resign
      * first and assign a new leader.
      *
      * @param me The incoming message from the GM
      * @param msg The cracked command message.
      * @param client The GM client the command came from.
      */
    void DeleteCharacter( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client );

    void BanName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void UnBanName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    void BanClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void UnbanClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void BanAdvisor(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void UnbanAdvisor(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void SendSpawnTypes (MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void SendSpawnItems (MsgEntry* me, Client *client);
    bool GetAccount(csString useroracc,Result& resultre);
    void RenameGuild( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client);
    void ChangeGuildLeader( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client);

    /** Awards experience to a player, by a GM */
    void AwardExperience(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client* target);

    /** Transfers an item from one client to another */
    void TransferItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* source, Client* target);

    /** Checks the presence of items */
    void CheckItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* target);

    /** Freezes a client, preventing it from doing anything. */
    void FreezeClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client* target);

    /** Thaws a client, reversing a freeze command. */
    void ThawClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client* target);

    void Inspect(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, gemActor* target);

    /// Changes the skill of the target
    void SetSkill(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client *target);

    /// Temporarily changes the mesh for a player
    void Morph(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client *targetclient);

    /// Temporarily changes the security level for a player
    void TempSecurityLevel(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /// Gets the given account number's security level from the DB
    int GetTrueSecurityLevel(AccountID accountID);

    /// Handle GM Event command
    void HandleGMEvent(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /// Handle request to view bad text from the targeted NPC.
    void HandleBadText(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /// Manipulate quests from characters
    void HandleCompleteQuest(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *subject);

    /// Change quality of items
    void HandleSetQuality(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /// Decide whether an item is stackable or not, bypassing the flag for this type of item
    void ItemStackable(MsgEntry* me, AdminCmdData& data, Client *client, gemObject* target );

    /// Set trait of a char
    void HandleSetTrait(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /// Change name of items
    void HandleSetItemName(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /// Handle reloads from DB
    void HandleReload(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /** @brief List warnings given to account
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     *  @param target The client we are inspecting for warnings
     */
    void HandleListWarnings(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target );

    /** @brief Shows what the target used in command will afflict in the game
     *  @param msg The cracked command message.
     *  @param targetobject A pointer to the targetted object for direct access
     *  @param client The GM client the command came from.
     */
    void CheckTarget(psAdminCmdMessage& msg, AdminCmdData& data, gemObject* targetobject , Client *client);

    /** @brief Allows to disable/enable quests temporarily (server cache) or definitely (database)
     *
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     */
    void DisableQuest(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /** @brief Allows to set an experience given to who kills the issing player
     *
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     */
    void SetKillExp(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /** @brief Allows to change faction points of players.
     *
     *  @param me The incoming message from the GM.
     *  @param msg The cracked command message.
     *  @param client The GM client the command came from.
     *  @param client The target client which will have his faction points changed.
     */
    void AssignFaction(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /** @brief Adds a petition under the passed user's name to the 'petitions' table in the database.
     *
     *  Will automatically add the date and time of the petition's submission
     *  in the appropriate table columns
     *  @param playerName: Is the name of the player who is submitting the petition.
     *  @param petition: The player's request.
     *  @return Returns either success or failure.
     */
    bool AddPetition(PID playerID, const char* petition);

    /** @brief Returns a list of all the petitions for the specified player
     *  @param playerID: Is the ID of the player who is requesting the list.
     *                   if the ID is -1, that means a GM is requesting a complete listing
     *  @param gmID: Is the id of the GM who is requesting petitions, ignored if playerID != -1
     *  @param gmLevel: Is the security level of the GM who is requesting petitions, ignored if playerID != -1
     *  @return Returns a iResultSet which contains the set of all matching petitions for the user
     */
    iResultSet *GetPetitions(PID playerID, PID gmID = PETITION_GM, int gmLevel = -1);

    /** @brief Cancels the specified petition if the player was its creator
     *  @param playerID: Is the ID of the player who is requesting the change.
     *                   if ID is -1, that means a GM is cancelling someone's petition
     *  @param petitionID: The petition id
     *  @return Returns either success or failure.
     */
    bool CancelPetition(PID playerID, int petitionID);

    /** @brief Changes the description of the specified petition if the player was its creator
     *  @param playerID: Is the ID of the player who is requesting the change.
     *                   if ID is -1, that means a GM is changing someone's petition
     *  @param petitionID: The petition id
     *  @return Returns either success or failure.
     */
    bool ChangePetition(PID playerID, int petitionID, const char* petition);

    /** @brief Closes the specified petition (GM only)
     *  @param gmID: Is the ID of the GM who is requesting the close.
     *  @param petitionID: The petition id
     *  @param desc: the closing description
     *  @return Returns either success or failure.
     */
    bool ClosePetition(PID gmID, int petitionID, const char* desc);

    /** @brief Assignes the specified GM to the specified petition
     *  @param gmID: Is the ID of the GM who is requesting the assignment.
     *  @param petitionID: The petition id
     *  @return Returns either success or failure.
     */
    bool AssignPetition(PID gmID, int petitionID);

    /** @brief Deassignes the specified GM to the specified petition
     *  @param gmID: Is the ID of the GM who is requesting the deassignment.
     *  @param petitionID: The petition id
     *  @return Returns either success or failure.
     */
    bool DeassignPetition(PID gmID, int petitionID);

    /** @brief Escalates the level of the specified petition.
     *
     *  Changes the assigned_gm to -1, and the status to 'Open'
     *  @param gmID: Is the ID of the GM who is requesting the escalation.
     *  @param gmLevel: The security level of the gm
     *  @param petitionID: The petition id
     *  @return Returns either success or failure.
     */
    bool EscalatePetition(PID gmID, int gmLevel, int petitionID);
    bool DescalatePetition(PID gmID, int gmLevel, int petitionID);

    /** @brief logs all gm commands
     *  @param gmID: the ID of the GM
     *  @param playerID: the ID of the player
     *  @param cmd: the command the GM executed
     *  @return Returns either success or failure.
     */
    bool LogGMCommand(PID gmID, PID playerID, const char* cmd);

    /** @brief Returns the last error generated by SQL
     *
     *  @return Returns a string that describes the last sql error.
     *  @see iConnection::GetLastError()
     */
    const char* GetLastSQLError ();

    csString lasterror;

    ClientConnectionSet* clients;

    void SendGMAttribs(Client* client);

    //! Holds a dummy dialog.
    /*! We may need this later on when NPC's are inserted.  This also
     * insures that the dicitonary will always exist.  There where some
     * problems with the dictionary getting deleted just after the
     * initial npc was added. This prevents that
     */
    psNPCDialog *npcdlg;

    /** Holds the entire PathNetwork for editing of paths.
     */
    psPathNetwork * pathNetwork;
};

#endif
