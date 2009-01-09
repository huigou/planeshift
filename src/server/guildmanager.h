/*
 * guildmanager.h
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


#ifndef __GUILDMANAGER_H__
#define __GUILDMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psguildinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class

class ClientConnectionSet;
class ChatManager;
class PendingGuildInvite;
class PendingGuildWarInvite;


/** Information about client that asked us to tell him when some guild data change */
class GuildNotifySubscription
{
public:
    GuildNotifySubscription(int guild, int clientnum, bool onlineOnly)
    {
        this->guild       = guild;
        this->clientnum   = clientnum;
        this->onlineOnly  = onlineOnly;
    }
    int guild;            ///< Guild ID
    int clientnum;        ///< Client Id
    bool onlineOnly;      ///< Should we send members that are online only, or all members ?
};


class GuildManager : public MessageManager
{
friend class PendingAllianceInvite;
public:

    GuildManager(ClientConnectionSet *pCCS, ChatManager *chat);
    virtual ~GuildManager();

    virtual void HandleMessage(MsgEntry *pMsg,Client *client) { };

    void HandleJoinGuild(PendingGuildInvite *invite);
    void AcceptWar(PendingGuildWarInvite *invite);

    void ResendGuildData(int id);

    /// After the grace period is up, disband guild if requirements not met.
    void RequirementsDeadline(int guild_id);

    /// Ensure guild has at least the minimum members, and set timer to disband if not.
    void CheckMinimumRequirements(psGuildInfo *guild, gemActor *notify);


protected:
    int  GetClientLevel(Client *client);
    bool CheckAllianceOperation(Client * client, bool checkLeaderGuild, psGuildInfo * & guild, psGuildAlliance * & alliance);

    void HandleCmdMessage(MsgEntry *me,Client *client);
    void HandleGUIMessage(MsgEntry *me,Client *client);
    void HandleMOTDSet(MsgEntry *me,Client *client);
    void HandleSubscribeGuildData(Client *client,iDocumentNode * root);
    void UnsubscribeGuildData(Client *client);
    void HandleSetOnline(Client *client,iDocumentNode * root);
    void HandleSetGuildNotifications(Client *client,iDocumentNode * root);
    void HandleSetLevelRight(Client *client,iDocumentNode * root);
    void HandleRemoveMember(Client *client,iDocumentNode * root);
    void HandleSetMemberLevel(Client *client,iDocumentNode * root);
    void HandleSetMemberPoints(Client *client,iDocumentNode * root);
    void HandleSetMemberNotes(Client *client,iDocumentNode * root, bool isPublic);

    /** Checks if client has right 'priv' */
    bool CheckClientRights(Client * client, GUILD_PRIVILEGE priv);

    /** Checks if client has right 'priv'. If not, it sends him psSystemMessage with text 'denialMsg' */
    bool CheckClientRights(Client * client, GUILD_PRIVILEGE priv, const char * denialMsg);

    void SendGuildData(Client *client);
    void SendLevelData(Client *client);
    void SendMemberData(Client *client, bool onlineOnly);
    void SendAllianceData(Client *client);

    csString MakeAllianceMemberXML(psGuildInfo * member, bool allianceLeader);

    void CreateGuild(psGuildCmdMessage& msg,Client *client);

    /// This function actually removes the guild
    void EndGuild(psGuildInfo *guild,int clientnum);

    /// This handles the command from the player to end the guild, validates and calls the other EndGuild.
    void EndGuild(psGuildCmdMessage& msg,Client *client);

    void ChangeGuildName(psGuildCmdMessage& msg,Client *client);
    bool FilterGuildName(const char* name);
    void Invite(psGuildCmdMessage& msg,Client *client);
    void Remove(psGuildCmdMessage& msg,Client *client);
    void Rename(psGuildCmdMessage& msg,Client *client);
    void Promote(psGuildCmdMessage& msg,Client *client);
    void ListMembers(psGuildCmdMessage& msg,Client *client);
    void Secret(psGuildCmdMessage &msg, Client *client);
    void Web(psGuildCmdMessage &msg, Client *client);
    void MOTD(psGuildCmdMessage &msg, Client *client);
    void GuildWar(psGuildCmdMessage &msg, Client *client);
    void GuildYield(psGuildCmdMessage &msg, Client *client);

    void NewAlliance(psGuildCmdMessage &msg, Client *client);
    void AllianceInvite(psGuildCmdMessage &msg, Client *client);
    void AllianceRemove(psGuildCmdMessage &msg, Client *client);
    void AllianceLeave(psGuildCmdMessage &msg, Client *client);
    void AllianceLeader(psGuildCmdMessage &msg, Client *client);
    void EndAlliance(psGuildCmdMessage &msg, Client *client);
    void RemoveMemberFromAlliance(Client * client, psGuildInfo * guild, psGuildAlliance * alliance,
                                  psGuildInfo * removedGuild);

    bool AddPlayerToGuild(int guild,const char *guildname,Client *client,int level);
    GuildNotifySubscription * FindNotifySubscr(Client * client);

    /** @brief Sends changed guild data to notification subscribers
      *
      * Value of 'msg' says which kind of data and it can be:
      *       psGUIGuildMessage::GUILD_DATA
      *       psGUIGuildMessage::LEVEL_DATA
      *       psGUIGuildMessage::MEMBER_DATA
      *       psGUIGuildMessage::ALLIANCE_DATA
      */
    void SendNotifications(int guild, int msg);

    /** Calls SendNotifications() with type psGUIGuildMessage::ALLIANCE_DATA for all alliance members */
    void SendAllianceNotifications(psGuildAlliance * alliance);

    /** Sends psGUIGuildMessage::ALLIANCE_DATA messages saying "you are not in any alliance"
      * to all notification subscribers from given alliance.
      * This is used when an alliance is being disbanded or when one of its members is removed. */
    void SendNoAllianceNotifications(psGuildAlliance * alliance);
    void SendNoAllianceNotifications(psGuildInfo * guild);

    void SendGuildPoints(psGuildCmdMessage& msg,Client *client);

    void UnsubscribeWholeGuild(psGuildInfo * guild);

    bool IsLeader(Client * client);

    ChatManager* chatserver;
    ClientConnectionSet* clients;
    csArray<GuildNotifySubscription*> notifySubscr;

    csRef<iDocumentSystem>  xml;
};

#endif

