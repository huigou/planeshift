/*
 * psguildinfo.h
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



#ifndef __PSGUILDINFO_H__
#define __PSGUILDINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/refarr.h>
#include <csutil/refcount.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

#include "rpgrules/psmoney.h"

//=============================================================================
// Local Includes
//=============================================================================


class psCharacter;
class iResultRow;

// There are no restrictions on operations done by level 9 member

enum GUILD_PRIVILEGE
{
    RIGHTS_VIEW_CHAT         = 1,            // User can view guild chat (command /guild)

    RIGHTS_CHAT              = 2,            // User can send guild chat messages (command /guild)

    RIGHTS_INVITE            = 4,            // User can invite players to guild

    RIGHTS_REMOVE            = 8,            // User can remove player of lower level from guild

    RIGHTS_PROMOTE           = 16,           // User can promote/demote players of lower level to another lower level
                                             //      User at level 9 (leader) can promote other player to level 9,
                                             //      which in turn demotes the leader to level 8.
                                             //      Leader cannot just demote himself.
                                             //      Leader cannot leave guild.
                                             //      This means that each guild has _just_ one level 9 person.

    RIGHTS_EDIT_LEVEL         = 32,           // User can change privileges of lower guild levels

    RIGHTS_EDIT_POINTS        = 64,           // User can change guild points of players of lower level

    RIGHTS_EDIT_GUILD         = 128,          // User can change guild name, guild secrecy, guild web page, can disband guild,
                                              // max guild points

    RIGHTS_EDIT_PUBLIC        = 256,          // User can edit public notes of player of lower level,
                                              //     but the notes are visible to all guild members

    RIGHTS_EDIT_PRIVATE       = 512,          // User can view and edit private notes of player of lower level,
                                              //     others can neither view nor edit them

    RIGHTS_VIEW_CHAT_ALLIANCE = 1024,         // User can view alliance chat (command /alliance /a)

    RIGHTS_CHAT_ALLIANCE      = 2048,         // User can send alliace chat messages (command /alliace /a)

    RIGHTS_USE_BANK           = 4096         // User can use guild bank
};

/** Defines a level inside a guild.
    The level has a name and different flags for the privileges assigned to the level.
*/
struct psGuildLevel
{
    csString title;             ///< Name of the level.
    int         level;          ///< The Rank of the level.
    int         privileges;     ///< Bit field for the privileges.

    ///checks if the request right is possessed by this guild level
    bool HasRights(GUILD_PRIVILEGE rights)
    { return (level==MAX_GUILD_LEVEL)  ||  (privileges & rights); }
};

//------------------------------------------------------------------------------

/** Defines a guild member in a guild.
*/
class psGuildMember
{
public:
    PID char_id;                ///< The character ID of the person.
    csString name;              ///< The name of the member
    psCharacter  *actor;        ///< Pointer to the character data of the person.
    psGuildLevel *guildlevel;   ///< Members current level.
    int guild_points;           ///< Their points in the guild.
    csString public_notes;      ///< The public notes the member has.
    csString private_notes;     ///< Private Guild notes for the player.
    csString last_login;        ///< The last login time for that user.
    int privileges;             ///< Bit field for additional privileges.
    int removedPrivileges;      ///< Bit field for privileges removed from this member.

    ///checks if the request right is possessed by this guild member
    bool HasRights(GUILD_PRIVILEGE rights)
    { return (guildlevel->HasRights(rights) && !(removedPrivileges & rights)) || (privileges & rights); }
};

//------------------------------------------------------------------------------

/** Holds data for a guild.
 */
class psGuildInfo : public csRefCount
{
public:
    int id;                     ///< UID of the guild.
    csString name;              ///< Name of the guild.
    PID founder;                ///< Character id for the founder of the guild.
    int karma_points;           ///< Guild's current karma points.
    csString web_page;          ///< URL for the guild.


protected:
    csString motd;              ///< The guild's Message of the day.
    bool secret;                ///< Flag if the guild is secret or not.

private:
    psMoney bankMoney;          ///< Money stored in the guild bank account.
    int max_guild_points;       ///< Maximum guild points obtainable in the guild.

public:
    int alliance;
    csArray<psGuildMember*> members;
    csArray<psGuildLevel*>  levels;
    csArray<int> guild_war_with_id;

    psGuildInfo();
    psGuildInfo(csString name, PID founder);
    ~psGuildInfo();

    bool Load(unsigned int id);
    bool Load(iResultRow& row);

    bool InsertNew(PID leader_char_id);
    bool RemoveGuild();

    psGuildLevel *Connect(psCharacter *player);        ///< Find existing and set actor
    void Disconnect(psCharacter *player);        ///< Find actor, remove and check for delete if last one

    psGuildMember *FindMember(const char *name);
    psGuildMember *FindMember(PID char_id);
    psGuildMember *FindLeader();
    psGuildLevel *FindLevel(int level);

    bool MeetsMinimumRequirements();

    bool AddNewMember(psCharacter *player, int level=1);
    bool RemoveMember(psGuildMember *target);

    bool RenameLevel(int level, const char *levelname);
    bool SetPrivilege(int level, GUILD_PRIVILEGE privilege, bool on);

    /** Allows to change the member priviledges (add/remove them). It will update the data accordly
     *  in the permissions/denied permissions entries
     *
     *  @param member: a pointer to the structure of the member we are changing privileges
     *  @param privilege: the privilige to add or remove
     *  @param on: if true we add the privilege, if false we remove it from the member
     *  @return bool: true if successfull
     */
    bool SetMemberPrivilege(psGuildMember *member, GUILD_PRIVILEGE privilege, bool on);
    bool UpdateMemberLevel(psGuildMember *target,int level);

    void AdjustMoney(psMoney money, bool);
    psMoney& GetBankMoney() { return bankMoney; }
    void SaveBankMoney();

    csString& GetName(){ return name; }

    /** Allows to change max amount of guild points for this guild which can be set
     *
     *  @param points: the amount of points to set
     *  @return bool: true if successfull and not out of scope to the global limit
     */
    bool SetMaxMemberPoints(int points);

    /** Gets the max member points allowed for this guild
     *
     *  @return int: the maximum amount of points which can be set for the members of this guild
     */
    int GetMaxMemberPoints() { return max_guild_points; }

    bool SetMemberPoints(psGuildMember * member, int points);
    bool SetMemberNotes(psGuildMember * member, const csString & notes, bool isPublic);

    bool SetName(csString guildName);
    bool SetWebPage(const csString & web_page);
    bool SetSecret(bool secret);
    bool IsSecret() { return secret; }

    ///Gets the MOTD string
    const char * GetMOTD();
    ///Sets the MOTD string
    bool SetMOTD(const char* str);

    void AddGuildWar(psGuildInfo *other);
    bool IsGuildWarActive(psGuildInfo *other);
    void RemoveGuildWar(psGuildInfo *other);

    int GetAllianceID() { return alliance; }
    int GetID() { return id; }

};

//-----------------------------------------------------------------------------

/** A guild alliance between 2 guilds.
 */
class psGuildAlliance : public csRefCount
{
public:
    psGuildAlliance();
    psGuildAlliance(const csString & name);

    /** INSERTs alliance data in database */
    bool InsertNew();

    /** Removes alliance data from database and sets psGuildInfo::alliance of members to 0 */
    bool RemoveAlliance();

    /** Loads alliance data from database */
    bool Load(int id);

    /** Adds new member to alliance (saves to database) */
    bool AddNewMember(psGuildInfo * member);

    /** Checks if 'member' is in our alliance */
    bool CheckMembership(psGuildInfo * member);

    /** Removes a member from alliance (saves to database) */
    bool RemoveMember(psGuildInfo * member);

    size_t GetMemberCount();

    /** Returns member with index 'memberNum' */
    psGuildInfo * GetMember(int memberNum);

    int GetID() { return id; }
    csString GetName() { return name; }

    psGuildInfo * GetLeader() { return leader; }
    bool SetLeader(psGuildInfo * newLeader);

    static csString lastError;    ///< When a psGuildAlliance method fails (returns false), this contains description of the problem

protected:
    int id;
    csString name;
    csRef<psGuildInfo> leader;
    csRefArray<psGuildInfo> members;
};

#endif

