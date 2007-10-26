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

#include <csutil/ref.h>

#include "net/messages.h"            // Chat Message definitions
#include "net/msghandler.h"         // Subscriber definition
#include "client.h"                 // Client, and ClientConnectionSet classes
#include "msgmanager.h"             // Parent class
#include "bulkobjects/psitem.h"
#include "gmeventmanager.h"

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
struct Result;
class iResultSet;


// List of GM levels and their security level.
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
    void Admin(int playerID, int clientnum,Client *client);
    
    /** This sets the player as a Role Play Master and hooks them into the 
      * admin commands they should have for their level.
      *       
      */      
    
    void AdminCreateNewNPC(csString& data);

    gemObject* FindObjectByString(const csString& str);

protected:

    bool Valid( int level, const char* command, int clientnum );
    bool IsReseting(const csString& command);

    
    typedef struct AdminCmdData
    {
        csString player, target, command, subCmd, commandMod;
        csString action, setting, attribute, attribute2, skill;
        csString map, sector, direction;
        csString text, petition, reason;
        csString newName, newLastName;
        csString item, mesh;
        csString description;
        csString wp1, wp2;
        csString gmeventName, gmeventDesc;
        csString zombie, requestor;
        csString type,name; // Used by: /location
        
        int value, interval, random;
        int rainDrops, density, fade;
        unsigned int mins, hours, days;
        float amt, x, y, z, rot;
        bool uniqueName;
        float radius, range;
        unsigned short stackCount;
        int instance;
        RangeSpecifier rangeSpecifier;

        bool DecodeAdminCmdMessage(MsgEntry *pMsg, psAdminCmdMessage& msg, Client *client);        
    };
    
    void CommandArea(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, int range);
    void HandleAdminCmdMessage(MsgEntry *pMsg, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void HandlePetitionMessage(MsgEntry *pMsg, psPetitionRequestMessage& msg, Client *client);
    void HandleGMGuiMessage(MsgEntry *pMsg, psGMGuiMessage& msg, Client *client);
    
    /** Handles a request to reload a quest from the database.
     *  @param msg The text name is in the msg.text field.
     *  @param client The client we will send error codes back to.
     */
    void HandleLoadQuest(psAdminCmdMessage& msg, AdminCmdData& data, Client* client);

    void GetSiblingChars(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data,Client *client);
    void GetInfo(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data,Client *client, gemObject* target);
    void CreateNPC(MsgEntry *me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor* basis);
    int  CopyNPCFromDatabase(int master_id, float x, float y, float z, float angle, const csString & sector, int instance);
    void KillNPC(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void CreateItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    bool CreateItem(const char * name, double xPos, double yPos, double zPos, float angle, const char * sector, int instance,int stackCount, int random, int value);
    void ModifyKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void ChangeLock(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeUnlockable(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeSecurity(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void MakeKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, bool masterkey);
    void CopyKey(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, psItem* key);
    void AddRemoveLock(MsgEntry *me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client,psItem* key);

    void CreateHuntLocation(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void ModifyHuntLocation(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, gemObject* object);

    void Slide(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject *target);

    void Teleport(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject *subject);
    /** Get sector and coordinates of target of teleportation described by 'msg'.
        Return success */
    bool GetTargetOfTeleport(Client *client, psAdminCmdMessage& msg, AdminCmdData& data, iSector * & targetSector,  csVector3 & targetPoint, float &yRot, gemObject *subject, int &instance);

    /** Get sector and coordinates of starting point of a map. Returns success. */
    bool GetStartOfMap(Client *client, const csString & map, iSector * & targetSector,  csVector3 & targetPoint);

    /** Handles movement of objects for teleport and slide. */
    bool MoveObject(Client *client, gemObject *target, csVector3& pos, float yrot, iSector* sector, int instance);

    /** This function sends a warning message from a GM to a player, and displays it in
     *  big, red, un-ignorable text on the screen and in the chat window.
     */
    void WarnMessage(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /** This function kicks a player off the server as long as the calling client
     *  has sufficient privlidges.
     */
    void KickPlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    //This function will mute a player until logoff
    void MutePlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);
    //This function will unmute a player
    void UnmutePlayer(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);
    
    /// Kills by doing a large amount of damage to target.
    void Death( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor* target);

    /// Impersonate a character
    void Impersonate(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Set various GM/debug abilities (invisible, invincible, etc.)
    void SetAttrib(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Set the label color for char
    void SetLabelColor(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemActor * subject);

    //Divorce char1 and char2, if char2 hasn't been online in the latest 2 months, or is deleted.
    void Divorce( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /// Add new wp to DB
    int WaypointCreate(csString& name, csVector3& pos, csString& sectorName, float radius, csString& flags);
    
    /// Handle online waypoint editing.
    void HandleWaypoint( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client );

    /// Add new Path point to DB
    int PathPointCreate(int pathID, int prevPointId, csVector3& pos, csString& sectorName);
    
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

    /// Handles queries send by a GM to the server for dealing with petitions
    void GMListPetitions(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);
    void GMHandlePetition(MsgEntry* me, psPetitionRequestMessage& msg, Client *client);
    
    void SendGMPlayerList(MsgEntry* me, psGMGuiMessage& msg, Client *client);

    
    void ChangeName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /// Handles a change to set the NPC's default spawn location.
    void UpdateRespawn(Client* client, gemActor* target);

    /// Controlls the rain / thunder and other weather
    void Weather(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Rain(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Snow(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Thunder(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void Fog(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    /** Deletes a character from the database.  Should be used with caution.
      * This function will also send out reasons why a delete failed. Possible
      * reasons are not found or the requester is not the same account as the 
      * one to delete.  Also if the character is a guild leader they must resign 
      * first and assign a new leader. 
      *
      * @param me The incomming message from the GM
      * @param mesg The cracked command message.
      * @param client The GM client the command came from.
      */
    void DeleteCharacter( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client );
    
    void BanName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void UnBanName(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);

    void BanClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void UnbanClient(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void SendSpawnTypes (MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client);
    void SendSpawnItems (MsgEntry* me, psGMSpawnItems& msg, Client *client);
    void SpawnItemInv( MsgEntry* me, psGMSpawnItem& msg, Client *client);
    bool GetAccount(csString useroracc,Result& resultre);
    void RenameGuild( MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client);
    
    /** Awards experience to a player, by a GM */
    void AwardExperience(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* client, Client* target);

    /** Transfers an item from one client to another */
    void TransferItem(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client* source, Client* target);

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
    int GetTrueSecurityLevel(int accountID);

    /// Handle GM Event command
    void HandleGMEvent(MsgEntry* me, psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *target);

    /// Handle request to view bad text from the targeted NPC.
    void HandleBadText(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

    /// Manipulate quests from characters
    void HandleCompleteQuest(MsgEntry* me,psAdminCmdMessage& msg, AdminCmdData& data, Client *client, Client *subject);

    /// Change quality of items
    void HandleSetQuality(psAdminCmdMessage& msg, AdminCmdData& data, Client *client, gemObject* object );

protected:

    /** Adds a petition under the passed user's name to the 'petitions' table in the database
     * Will automatically add the date and time of the petition's submission
     * in the appropriate table columns
     * @param playerName: Is the name of the player who is submitting the petition.
     * @param petition: The player's request.
     * @return Returns either success or failure.
     */
    bool AddPetition(int playerID, const char* petition);

    /** Returns a list of all the petitions for the specified player
     * @param playerID: Is the ID of the player who is requesting the list.
     *            if the ID is -1, that means a GM is requesting a complete listing
     * @param gmID: Is the id of the GM who is requesting petitions, ignored if playerID != -1
     * @param gmLevel: Is the security level of the GM who is requesting petitions, ignored if playerID != -1
     * @return Returns a iResultSet which contains the set of all matching petitions for the user
     */
    iResultSet *GetPetitions(int playerID, int gmID = -1, int gmLevel = -1);

    /** Cancels the specified petition if the player was its creator
     * @param playerID: Is the ID of the player who is requesting the change.
     *            if ID is -1, that means a GM is cancelling someone's petition
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool CancelPetition(int playerID, int petitionID);

    /** Closes the specified petition (GM only)
     * @param gmID: Is the ID of the GM who is requesting the close.
     * @param petitionID: The petition id
     * @param desc: the closing description
     * @return Returns either success or failure.
     */
    bool ClosePetition(int gmID, int petitionID, const char* desc);

    /** Assignes the specified GM to the specified petition
     * @param gmID: Is the ID of the GM who is requesting the assignment.
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool AssignPetition(int gmID, int petitionID);

    /** Escalates the level of the specified petition, changes
     * the assigned_gm to -1, and the status to 'Open'
     * @param gmID: Is the ID of the GM who is requesting the escalation.
     * @param gmLevel: The security level of the gm
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool EscalatePetition(int gmID, int gmLevel, int petitionID);
    bool DescalatePetition(int gmID, int gmLevel, int petitionID);
    
    /** logs all gm commands
     * @param gmID: the ID of the GM
     * @param playerID: the ID of the player
     * @param cmd: the command the GM executed
     * @return Returns either success or failure.
     */
    bool LogGMCommand(int gmID, int playerID, const char* cmd);

    /** Returns the last error generated by SQL
     * 
     * @return Returns a string that describes the last sql error.
     * @see iConnection::GetLastError()
     */
    const char* GetLastSQLError ();
    
    csString lasterror;

    ClientConnectionSet* clients;
   
//    csRef<iCelEntity> entity;

    //! Holds a dummy dialog.
    /*! We may need this later on when NPC's are inserted.  This also
     * insures that the dicitonary will always exist.  There where some 
     * problems with the dictionary getting deleted just after the 
     * initial npc was added. This prevents that
     */
    psNPCDialog *npcdlg;
};

#endif

