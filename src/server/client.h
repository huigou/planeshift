/*
 * client.h - Author: Keith Fulton
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
 */

#ifndef __CLIENT_H__
#define __CLIENT_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "net/netbase.h"
#include "util/psconst.h"
#include "util/waypoint.h"

//=============================================================================
// Local Includes
//=============================================================================


class ClientConnectionSet;
class psCharacter;
class psItem;
class gemObject;
class gemActor;
class gemNPC;
class psPath;

class FloodBuffRow
{
public:
    FloodBuffRow(uint8_t chtType, csString txt, csString rcpt, unsigned int newticks);
    FloodBuffRow() : ticks(0) {}
    uint8_t chatType;
    csString text;
    csString recipient;
    unsigned int ticks;
};

enum TARGET_TYPES
{
    TARGET_NONE     = 0x01, /* Also Area */
    TARGET_ITEM     = 0x04,
    TARGET_SELF     = 0x08,
    TARGET_FRIEND   = 0x10,
    TARGET_FOE      = 0x20,
    TARGET_DEAD     = 0x40,
};

enum CheatFlags // will have more of these as Paladin gets better
{
    NO_CHEAT   = 0x00,
    MOVE_CHEAT = 0x01
};

struct OrderedMessageChannel;

/**
* This class collects data of a netclient. While the socket data like
* ip adress and port is managed inside NetManager and not stored here
* this is the object that will be saved in the ObjectManager and will
* get an Object ID
*/
class Client : protected NetBase::Connection, public CS::Utility::WeakReferenced
{
public:
    /**
    * Please call constructor with the connection object produced by
    * handleUnknownClient
    */
    Client();


    ~Client();

    bool Initialize(LPSOCKADDR_IN addr, uint32_t clientnum);
    bool Disconnect();

    /**
     * Called from server side to set the allowedToDisconnect flag.
     * This flagg will be read by the network thread to
     * see if the client could be disconnected.
     */
    void SetAllowedToDisconnect(bool allowed);

    /// Permit the player to disconnect? Players cannot quit while in combat (includes spell casting).
    /// Also causes the client to be set as a zombie indicating that the server knows the connection has been broken.
    bool AllowDisconnect();

    /// Check if a zombie is allowed to disconnect. Called from the network thread
    /// so no access to server internal data should be made.
    bool ZombieAllowDisconnect();
    
    /// SetMute is the function that toggles the muted flag
    void SetMute(bool flag) { mute = flag; }
    bool IsMute() { return mute; }

    void SetName(const char* n) { name = n; }
    const char* GetName() { return name; }

    // Additional Entity information
    void SetActor(gemActor* myactor);
    gemActor* GetActor() const { return actor; }
    psCharacter *GetCharacterData();

    // Get / Set Familiar information;
    void SetFamiliar(gemActor *familiar);
    gemActor* GetFamiliar();

    // Get / Set Pet information;
    void AddPet(gemActor *pet);
    void RemovePet( size_t index );
    gemActor *GetPet( size_t index );
    size_t GetNumPets();

    /// Return if other is one of my pets.
    bool IsMyPet(gemActor * other) const;
        
    /// Returns whether the client's character is alive.
    bool IsAlive() const;

    /// Check whether the client is frozen.
    bool IsFrozen() const { return isFrozen; }
    void SetFrozen(bool frozen) { isFrozen = frozen; }

    /// Check if distance between client and target is within range.
    bool ValidateDistanceToTarget(float range);

    // Target information
    void SetTargetObject(gemObject *object, bool updateClientGUI=false);
    gemObject* GetTargetObject() const { return target; }

    // Targeted mesh information - i.e. for adding action locations
    void SetMesh(csString nextMesh) { mesh = nextMesh; }
    csString GetMesh() const { return mesh; }

    /** Get the current selected target player.
     * @return -1 if no target selected or target not a player
     */
    int GetTargetClientID();

    /** Get the current selected object id.
    * @return -1 if no target selected
    */
    //    int GetTargetObjectID();

    uint32_t GetClientNum() const { return clientnum; }

    /// The account number for this client.
    AccountID GetAccountID() { return accountID; }
    void SetAccountID(AccountID id) { accountID = id; }

    /// The player number for this client.
    PID GetPID() { return playerID; }
    void SetPID(PID id) { playerID = id; }

    int GetExchangeID() { return exchangeID; }
    void SetExchangeID(int ID) { exchangeID = ID; }

    /// The object number for this client.
    //    int GetObjectID() { return objectID; }
    //    void SetObjectID(int ID) { objectID = ID; }

    /// The security level of this player
    int GetSecurityLevel() const { return securityLevel; }
    void SetSecurityLevel(int level) { securityLevel=level; }
    
    bool IsGM() const;

    /// The guild id value if player is member of guild.
    int GetGuildID();
    //    bool IsGuildSecret() { return guild_is_secret; }

    void SetReady(bool rdy) { ready = rdy; }
    bool IsReady() { return ready; }

    /** Checks if out client can attack given target
     *  If not, it sends him some informative text message
     */
    bool IsAllowedToAttack(gemObject* target, bool inform = true);

    /** Returns the type of the target, from TARGET_TYPES.
     *
     * @param target The target to check.
     * @return The type of the target.
     */
    int GetTargetType(gemObject* target);

    /** Builds a list of target type names associated with a target type
     *  bitmap (formed by OR-ing TARGET_TYPES).
     *
     * @param targetType A target type bitmap.
     * @param targetDesc [CHANGES] Gets filled in with a comma-separated list
     *                   of target names.
     */
    void GetTargetTypeName(int32_t targetType, csString& targetDesc) const;

    //    void SetExchange(csRef<ExchangeManager> exch);

    /*
    typedef enum
    { NOT_TRADING, SELLING, BUYING} TradingStatus;

    void SetTradingStatus( TradingStatus trading, int merchantID)
    { tradingStatus = trading; this->merchantID = merchantID; }

    TradingStatus GetTradingStatus() { return tradingStatus; }
    int GetMerchantID() { return merchantID; }
    */
    
    /**
     * A zombie client is a client that is prevented from disconnecting because
     * of combat, spellcasting, or defeted.
     */
    bool IsZombie() { return zombie; }
    
    /// Allow distinguishing superclients from regular player clients
    bool IsSuperClient() { return superclient; }
    void SetSuperClient(bool flag) { superclient = flag; }

    long GetIPAddress(char *addr)
    {
        unsigned int a1,a2,a3,a4;
#ifdef WIN32
        unsigned long a = this->addr.sin_addr.S_un.S_addr;
#else
        unsigned long a = this->addr.sin_addr.s_addr;
#endif

        if (addr)
        {
            a1 = a&0x000000FF;
            a2 = (a&0x0000FF00)>>8;
            a3 = (a&0x00FF0000)>>16;
            a4 = (a&0xFF000000)>>24;
            sprintf(addr,"%d.%d.%d.%d",a1,a2,a3,a4);
        }
        return a;
    }

    csString GetIPRange(int octets=3)
    {
        char ipaddr[20] = {0};
        GetIPAddress(ipaddr);
        return GetIPRange(ipaddr,octets);
    }

    static csString GetIPRange(const char* ipaddr, int octets=3)
    {
        csString range(ipaddr);
        for (size_t i=0; i<range.Length(); i++)
        {
            if (range[i] == '.')
                --octets;
    
            if (!octets)
            {
                range[i+1] = '\0';
                break;
            }
        }
        return range;
    }

    NetBase::Connection* GetConnection() const { return (NetBase::Connection*)this; }

    //    bool SetTradingStopped(bool stopped);

    //    bool ReadyToExchange();

    const SOCKADDR_IN& GetAddress() const { return addr; }

    csRef<NetPacketQueueRefCount> outqueue;
    
    unsigned int GetAccountTotalOnlineTime();

    void AddDuelClient(int clientnum);
    void RemoveDuelClient(Client *client);
    void ClearAllDuelClients();
    int GetDuelClientCount();
    int GetDuelClient(int id);
    bool IsDuelClient(int clientnum);
    void AnnounceToDuelClients(gemActor *attacker, const char *event);

    /// Warn or mute for chat spamming
    void FloodControl(uint8_t chatType, const csString & newMessage, const csString & recipient);

    void SetAdvisorPoints(int p) { advisorPoints = p; }
    void IncrementAdvisorPoints(int n=1) { advisorPoints += n; }
    int GetAdvisorPoints() { return advisorPoints; }

    /// Set this client's advisor status
    void SetAdvisor(bool advisor) { isAdvisor = advisor; }
    bool GetAdvisor() { return isAdvisor; }
    void SetAdvisorBan(bool ban);
    bool IsAdvisorBanned();

    /// For cheat detection
    csTicks accumulatedLag;

    // Invite flood control
    csTicks GetLastInviteTime() { return lastInviteTime; }
    void SetLastInviteTime(csTicks time) { lastInviteTime = time; }
    bool GetLastInviteResult() { return lastInviteResult; }
    void SetLastInviteResult(bool result) { lastInviteResult = result; }
    bool HasBeenWarned() { return hasBeenWarned; }
    void SetWarned() { hasBeenWarned = true; }
    bool HasBeenPenalized() { return hasBeenPenalized; }
    void SetPenalized(bool value) { hasBeenPenalized = value; }
    int GetSpamPoints() { return spamPoints; }
    void SetSpamPoints(int points) { spamPoints = points; }  // For setting on account load
    void IncrementSpamPoints() { if (spamPoints<4) spamPoints++; }
    void DecrementSpamPoints() { if (spamPoints>0) spamPoints--; }
    
    /// Online edit of paths
    void WaypointSetPath(csString& path, int index) { waypointPathName = path; waypointPathIndex = index;}
    csString& WaypointGetPathName(){ return waypointPathName; }
    int WaypointGetPathIndex() { return waypointPathIndex; }
    int WaypointGetNewPathIndex() { return waypointPathIndex++; }

    uint32_t PathGetEffectID();
    uint32_t WaypointGetEffectID();

    psPath * PathGetPath() { return pathPath; }
    void PathSetPath(psPath * path) { pathPath = path; }
    
    void PathSetIsDisplaying( bool displaying ) { pathIsDisplaying = displaying; }
    bool PathIsDisplaying() { return pathIsDisplaying; }

    void WaypointSetIsDisplaying( bool displaying ) { waypointIsDisplaying = displaying; }
    bool WaypointIsDisplaying() { return waypointIsDisplaying; }

    
    /// Online edit of location
    uint32_t LocationGetEffectID();    
    void LocationSetIsDisplaying( bool displaying ) { locationIsDisplaying = displaying; }
    bool LocationIsDisplaying() { return locationIsDisplaying; }

    /// Give a warning to the client silently. Capped at 10000.
    void CountDetectedCheat() {if (detectedCheatCount < 10000) detectedCheatCount++; }
    int GetDetectedCheatCount() { return detectedCheatCount;}
    /// Set the next move as not a cheat, or clear it.
    void SetCheatMask(CheatFlags mask,bool flag);
    bool GetCheatMask(CheatFlags mask) { return (cheatMask & mask) != 0;  }

	/// Get the channel to use for an ordered message
	OrderedMessageChannel * GetOrderedMessageChannel(msgtype mtype);
	
protected:

    /**
     * A zombie client is a client where the player has disconnected, but
     * still active due to not finished combat, spellcasting, or defeted.
     */
    bool zombie;
    csTicks zombietimeout;

    /**
     * Server set this flag when the player isn't casting spells,
     * fighting or doing anythinge else that should prevent the player
     * from disconnecting.
     */
    bool allowedToDisconnect;
    
	/// Currently active id of Exchange object managed by Exchange Manager, or 0 if no exchange is active.
    int exchangeID;

	/// Pointer to the gemActor object played by this client once a character is logged in and in-game.
    gemActor *actor;

    csArray<PID> pets;
    csWeakRef<gemObject> target;
    csString mesh;
    bool ready;

	/// This collection stores queues of ordered messages while we're waiting for an out of order one to arrive.
	csHash<OrderedMessageChannel*> orderedMessages;
    
    bool isAdvisor;         ///< Store if this client is acting as an advisor.

    /// mute flag
    bool mute;

    AccountID accountID;
    PID playerID;
    int  securityLevel;
    bool superclient;
    csArray<gemNPC *> listeningNpc;
    csString name;

    csArray<int> duel_clients;

    // Flood control
    static const int floodWarn = 3;                     ///< Warn client after 3 repeated messages
    static const int floodMax  = 5;                     ///< Mute client after 5 repeated messages
    static const unsigned int floodForgiveTime = 10000; ///< How long to wait before forgiving a repeated message
    FloodBuffRow floodHistory[floodMax];
    int nextFloodHistoryIndex;

    int spamPoints;
    int advisorPoints;

    void SaveAccountData();

    // Invite flood
    csTicks lastInviteTime;
    bool lastInviteResult;
    bool hasBeenWarned;
    bool hasBeenPenalized;

    // Path edit global vars for client
    csString waypointPathName;
    int waypointPathIndex;
    uint32_t waypointEffectID;
    bool waypointIsDisplaying;
    uint32_t pathEffectID;
    psPath *pathPath;
    bool pathIsDisplaying;
    
    // Location edit global vars for client
    uint32_t locationEffectID;
    bool locationIsDisplaying;
    
private:    
    bool isFrozen;  ///< Whether the client is frozen or not.

    /// Potential number of exploits automatically detected.
    int detectedCheatCount;

    /// This flag set to true after a teleport, to keep Paladin from reporting a cheat
    int cheatMask;
};

#endif
