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
#include "net/npcmessages.h"
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
    
    /** Checks if the npc command message could overrun if the neededSize is tried to be added.
     *  Automatically sends the message in case it could overrun.
     * 	@param neededSize The size of data we are going to attempt to add to the npc commands message.
     */
    void CheckCommandsOverrun(size_t neededSize);

 private:

    /** Send a DR Data command to server.
     *
     *  This command is private to the NetworkManager since it should
     *  only be called as a result of first calling QueueDRData that will
     *  put the npc in a queue for DR updates.
     *
     * @param npc             The npc that sit/stand
     * @param target          The current target of then npc
     * @param sit             True if sitting, false if standing
     */
    void QueueDRDataCommand(gemNPCActor *entity,psLinearMovement *linmove,uint8_t counter);

 public:
    /** Queue the NPC for an DR Update to the server.
     *
     *  Can be called multiple times. If in queue already nothing will
     *  be added.
     */
    void QueueDRData(NPC * npc );
    
    /// Call to remove queued dr updates when entities are removed/deleted.
    void DequeueDRData(NPC * npc );
    
    void QueueAttackCommand(gemNPCActor *attacker, gemNPCActor *target);
    
    /** Send a sit command to server
     *
     * @param npc             The npc that sit/stand
     * @param target          The current target of then npc
     * @param sit             True if sitting, false if standing
     */
    void QueueSitCommand(gemNPCActor *npc, gemNPCObject *target, bool sit);

    /** Send a spawn command to server
     *
     * @param mother          The mother of the spawn
     * @param father          If the father is known point to the father, use mother if unknow.
     * @param tribeMemberType If part of a tribe set to the need set index to be used for this new entity.
     */
    void QueueSpawnCommand(gemNPCActor *mother, gemNPCActor *father, uint32_t tribeMemberType);
    
    /** Queue a talk command to the server
     *
     * The speaker will send a talk event say/action to target or nearby targets.
     *
     */
    void QueueTalkCommand(gemNPCActor *speaker, gemNPCActor* target, psNPCCommandsMessage::PerceptionTalkType talkType, bool publicTalk, const char* text);
    void QueueVisibilityCommand(gemNPCActor *entity, bool status);
    void QueuePickupCommand(gemNPCActor *entity, gemNPCObject *item, int count);

    /** Send an emote command to server
     *
     * @param npc             The npc that has an emotion
     * @param target          The current target of then npc
     * @param cmd             The emotion
     */
    void QueueEmoteCommand(gemNPCActor *npc, gemNPCObject *target, const csString& cmd);

    void QueueEquipCommand(gemNPCActor *entity, csString item, csString slot, int count);
    void QueueDequipCommand(gemNPCActor *entity, csString slot);
    void QueueDigCommand(gemNPCActor *entity, csString resource);
    void QueueTransferCommand(gemNPCActor *entity, csString item, int count, csString target);
    void QueueDropCommand(gemNPCActor *entity, csString slot);
    void QueueResurrectCommand(csVector3 where, float rot, iSector* sector, PID character_id);
    void QueueSequenceCommand(csString name, int cmd, int count);
    void QueueImperviousCommand(gemNPCActor * entity, bool impervious);
    void QueueInfoReplyCommand(uint32_t clientNum,const char* reply);
    void SendAllCommands(bool final = false);

    void SendConsoleCommand(const char *cmd);

    csStringHashReversible * GetMsgStrings();
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

