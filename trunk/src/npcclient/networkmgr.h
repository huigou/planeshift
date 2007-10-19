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

#include <csutil/csstring.h>

#include "engine/netpersist.h"
#include "net/cmdbase.h"   // Subscriber class
#include "util/serverconsole.h"  // iCommandCatcher
class  MsgHandler;
class  EventManager;
class  psNPCCommandsMessage;
struct iPcLinearMovement;
class psNetConnection;
class psGameEvent;
class NPC;

class NetworkManager : public psClientNetSubscriber 
{
protected:
    MsgHandler *msghandler;
    psNetConnection *connection;
    bool ready;
    bool connected;
    csStringHash*    msgstrings;  /// server-supplied list of strings to use in net msgs

    // Command Message Queue  
    psNPCCommandsMessage *outbound;
    csHash<NPC*>          cmd_dr_outbound; /// Entities queued for sending of DR. 
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
    void HandleNPCSetOwner( MsgEntry *msg);
    // void RemoveEntity(psCelPersistMessage& pmsg);

    void PrepareCommandMessage();
    
    void HandleRaceList( MsgEntry* me );
    void HandleActor( MsgEntry* me );
    void HandleItem( MsgEntry* me );
    void HandleObjectRemoval( MsgEntry* me );

    csString host,password,user;
    int port;

public:

    NetworkManager(MsgHandler *mh,psNetConnection* conn);
    virtual ~NetworkManager();

    virtual void HandleMessage(MsgEntry *pMsg);

    void Authenticate(csString& host,int port,csString& user,csString& pass);
    bool IsReady() { return ready; }
    void Disconnect();
    void QueueDRData(iCelEntity *entity,iPcLinearMovement *linmove,uint8_t counter);
    void QueueDRData(NPC * npc );
    void QueueAttackCommand(iCelEntity *attacker, iCelEntity *target);
    void QueueSpawnCommand(iCelEntity *mother, iCelEntity *father);
    void QueueTalkCommand(iCelEntity *speaker, const char* text);
    void QueueVisibilityCommand(iCelEntity *entity, bool status);
    void QueuePickupCommand(iCelEntity *entity, iCelEntity *item, int count);
    void QueueEquipCommand(iCelEntity *entity, csString item, csString slot, int count);
    void QueueDequipCommand(iCelEntity *entity, csString slot);
    void QueueDigCommand(iCelEntity *entity, csString resource);
    void QueueTransferCommand(iCelEntity *entity, csString item, int count, csString target);
    void QueueDropCommand(iCelEntity *entity, csString slot);
    void QueueResurrectCommand(csVector3 where, float rot, csString sector, int character_id);
    void QueueSequenceCommand(csString name, int cmd, int count);
    void SendAllCommands(bool final = false);

    void SendConsoleCommand(const char *cmd);

    csStringHash * GetMsgStrings() { return msgstrings; }
    const char * GetCommonString(uint32_t id);

    void ReAuthenticate();
    void ReConnect();

    bool reconnect;
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

