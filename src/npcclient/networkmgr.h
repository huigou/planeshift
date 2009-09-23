/*
 * networkmgr.h
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
#ifndef __NETWORKMGR_H__
#define __NETWORKMGR_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "net/cmdbase.h"            // Subscriber class
#include "util/serverconsole.h"     // iCommandCatcher

class MsgHandler;
class EventManager;
class psLinearMovement;

//=============================================================================
// Local Includes
//=============================================================================
class psNPCCommandsMessage;
class psNetConnection;
class psGameEvent;
class NPC;
class gemNPCActor;
class gemNPCObject;


class NetworkManager : public psClientNetSubscriber 
{
protected:
    MsgHandler *msghandler;
    psNetConnection *connection;
    bool ready;
    bool connected;
    csStringHashReversible*    msgstrings;  /// server-supplied list of strings to use in net msgs

    // Command Message Queue  
    psNPCCommandsMessage *outbound;
    csHash<NPC*,PID>      cmd_dr_outbound; /// Entities queued for sending of DR. 
    int                   cmd_count;       /// Number of command messages queued

    void RequestAllObjects();
    
    bool ReceiveMapList(MsgEntry *msg);
    bool ReceiveNPCList(MsgEntry *msg);

    void HandlePositionUpdates(MsgEntry *msg);
    void HandlePersistMessage(MsgEntry *msg);
    void HandlePerceptions(MsgEntry *msg);
    void HandleDisconnect(MsgEntry *msg);
    void HandleTimeUpdate(MsgEntry *msg);
    void HandleNewNpc(MsgEntry *msg);
    // void RemoveEntity(psCelPersistMessage& pmsg);

    void PrepareCommandMessage();
    
    void HandleRaceList( MsgEntry* me );
    void HandleAllEntities(MsgEntry *message);
    void HandleActor( MsgEntry* me );
    void HandleItem( MsgEntry* me );
    void HandleObjectRemoval( MsgEntry* me );

    csString host,password,user;
    int port;

public:

    NetworkManager(MsgHandler *mh,psNetConnection* conn, iEngine* engine);
    virtual ~NetworkManager();

    virtual void HandleMessage(MsgEntry *pMsg);

    void Authenticate(csString& host,int port,csString& user,csString& pass);
    bool IsReady() { return ready; }
    void Disconnect();
    void QueueDRData(gemNPCActor *entity,psLinearMovement *linmove,uint8_t counter);
    void QueueDRData(NPC * npc );
    /// Call to remove queued dr updates when entities are removed/deleted.
    void DequeueDRData(NPC * npc );
    void QueueAttackCommand(gemNPCActor *attacker, gemNPCActor *target);
    void QueueSpawnCommand(gemNPCActor *mother, gemNPCActor *father);
    void QueueTalkCommand(gemNPCActor *speaker, const char* text);
    void QueueVisibilityCommand(gemNPCActor *entity, bool status);
    void QueuePickupCommand(gemNPCActor *entity, gemNPCObject *item, int count);
    void QueueEquipCommand(gemNPCActor *entity, csString item, csString slot, int count);
    void QueueDequipCommand(gemNPCActor *entity, csString slot);
    void QueueDigCommand(gemNPCActor *entity, csString resource);
    void QueueTransferCommand(gemNPCActor *entity, csString item, int count, csString target);
    void QueueDropCommand(gemNPCActor *entity, csString slot);
    void QueueResurrectCommand(csVector3 where, float rot, csString sector, PID character_id);
    void QueueSequenceCommand(csString name, int cmd, int count);
    void QueueImperviousCommand(gemNPCActor * entity, bool impervious);
    void SendAllCommands(bool final = false);

    void SendConsoleCommand(const char *cmd);

    csStringHashReversible * GetMsgStrings() { return msgstrings; }
    const char * GetCommonString(uint32_t id);

    void ReAuthenticate();
    void ReConnect();

    bool reconnect;
private:
    iEngine* engine;
};

class psNPCReconnect : public psGameEvent
{
protected:
    NetworkManager *networkMgr;
    bool authent;

public:
    psNPCReconnect(int offsetticks, NetworkManager *mgr, bool authent);
    virtual void Trigger();  // Abstract event processing function
};

#endif

