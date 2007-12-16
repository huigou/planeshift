/*
 * NetManager.cpp by Matze Braun <MatzeBraun@gmx.de>
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
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>
#include <physicallayer/entity.h>
#include <propclass/mesh.h>
#include <physicallayer/entity.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/pserror.h"
#include "util/sleep.h"
#include "util/serverconsole.h"

#include "net/message.h"
#include "net/messages.h"
#include "net/netpacket.h"

#include "bulkobjects/psaccountinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "netmanager.h"
#include "client.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "cachemanager.h"
#include "globals.h"


// Check every 3 seconds for linkdead clients
#define LINKCHECK    3000
// Check for resending every 200 ms
#define RESENDCHECK    200
// Redisplay network server stats every 60 seconds
#define STATDISPLAYCHECK 60000

NetManager::NetManager()
    : NetBase (1000),stop_network(false)
{
    port=0;
}

NetManager::~NetManager()
{
    stop_network = true;
    thread->Wait ();
    thread = NULL;
}

bool NetManager::Initialize(int client_firstmsg, int npcclient_firstmsg, int timeout)
{
    if (!NetBase::Init(false))
        return false;

    NetManager::port = port;
    NetManager::client_firstmsg = client_firstmsg;
    NetManager::npcclient_firstmsg = npcclient_firstmsg;
    NetManager::timeout = timeout;

    if (!clients.Initialize())
        return false;

    thread.AttachNew( new CS::Threading::Thread(this, true) );
    if (!thread->IsRunning())
        return false;

    SetMsgStrings(CacheManager::GetSingleton().GetMsgStrings());

    return true;
}

bool NetManager::HandleUnknownClient (LPSOCKADDR_IN addr, MsgEntry* me)
{
    psMessageBytes* msg = me->bytes;

    // The first msg from client must be "firstmsg", "alt_first_msg" or "PING"
    if (msg->type!=client_firstmsg && msg->type!=npcclient_firstmsg && msg->type!=MSGTYPE_PING)
        return false;

    if ( msg->type == MSGTYPE_PING )
    {   
        psPingMsg ping(me);

        if (!(ping.flags & PINGFLAG_REQUESTFLAGS) && !psserver->IsReady())
            return false;

        int flags = 0;
        if (psserver->IsReady()) flags |= PINGFLAG_READY;
        if (psserver->HasBeenReady()) flags |= PINGFLAG_HASBEENREADY;
        if (psserver->IsFull(clients.Count(),NULL)) flags |= PINGFLAG_SERVERFULL;

        // Create the reply to the ping
        psPingMsg pong(0,ping.id,flags);
        pong.msg->msgid = GetRandomID();

        psNetPacketEntry *pkt = new
            psNetPacketEntry(pong.msg->priority, 0,
            pong.msg->msgid, 0, (uint32_t) pong.msg->bytes->GetTotalSize(),
            (uint16_t) pong.msg->bytes->GetTotalSize(), pong.msg->bytes);

        SendFinalPacket(pkt,addr);

        delete pkt;

        return false;
    }        

    // Don't accept any new clients when not ready, npcclients is accepted 
    if(msg->type==client_firstmsg && !psserver->IsReady())
        return false;

    // Create and add the client object to client list
    Client* client = clients.Add(addr);
    if (!client)
        return false;

    // This is for the accept message that will be sent back
    me->clientnum = client->GetClientNum();

    return true;
}

void NetManager::CheckResendPkts()
{
    BinaryRBIterator<psNetPacketEntry> loop(&awaitingack);
    psNetPacketEntry *pkt;
    csArray<psNetPacketEntry *> pkts;

    csTicks currenttime = csGetTicks();

    for (pkt = loop.First(); pkt; pkt = ++loop)
    {
        if (pkt->timestamp + PKTMAXLATENCY < currenttime)
            pkts.Push(pkt);
    }
    for (size_t i = 0; i < pkts.GetSize(); i++)
    {
#ifdef PACKETDEBUG
        Debug2(LOG_NET,"Resending nonacked HIGH packet (ID %d).\n", pkt->packet->pktid);
#endif
        pkt = pkts.Get(i);
        pkt->timestamp = currenttime;   // update stamp on packet

        // re-add to send queue
        Client *client = clients.FindAny(pkt->clientnum);
        if (!client)
        {
            awaitingack.Delete(pkt);
            continue;
        }

        /*  The proper way to send a message is to add it to the queue, and then add the queue to the senders.
        *  If you do it the other way around the net thread may remove the queue from the senders before you add the packet.
        *   Yes - this has actually happened!
        */

        /* We store the result here to return as the result of this function to maintain historic functionality.
        *  In actuality a false response does not actually mean no data was added to the queue, just that
        *  not all of the data could be added.
        */
        if (!client->outqueue->Add(pkt))
            Error1("Queue full. Could not add packet.\n");

        /**
        * The senders list is a list of busy queues.  The SendOut() function
        * in NetBase clears this list each time through.  This saves having
        * to check every single connection each time through.
        */

        //client->outqueue->IncRef();
        if (!senders.Add (client->outqueue))
            Error1("Senderlist Full!");

        //printf("pkt=%p, pkt->packet=%p\n",pkt,pkt->packet);
        // take out of awaiting ack pool.
        // This does NOT delete the pkt mem block itself.
        if (awaitingack.Delete(pkt) == 0 )
        {
#ifdef PACKETDEBUG
            Debug2(LOG_NET,"No packet in ack queue :%d\n", pkt->packet->pktid);
#endif
        }

    }
}

NetManager::Connection *NetManager::GetConnByIP (LPSOCKADDR_IN addr)
{
    Client* client = clients.Find(addr);
    
    if (!client)
        return NULL;
    
    return client->GetConnection();
}

NetManager::Connection *NetManager::GetConnByNum (uint32_t clientnum)
{
    Client* client = clients.FindAny(clientnum);

    if (!client)
        return NULL;

    // Debug check to see if we have a concurrency problem
    CS_ASSERT(clients.FindAny(clientnum) != NULL);
    
    return client->GetConnection();
}

bool NetManager::SendMessage(MsgEntry* me)
{
    bool sendresult;
    Client *client = clients.FindAny(me->clientnum);
    if (!client)
        return false;

    /*  The proper way to send a message is to add it to the queue, and then add the queue to the senders.
     *  If you do it the other way around the net thread may remove the queue from the senders before you add the packet.
     *   Yes - this has actually happened!
     */

    /* We store the result here to return as the result of this function to maintain historic functionality.
     *  In actuality a false response does not actually mean no data was added to the queue, just that
     *  not all of the data could be added.
     */
    sendresult=NetBase::SendMessage(me,client->outqueue);

    /**
     * The senders list is a list of busy queues.  The SendOut() function
     * in NetBase clears this list each time through.  This saves having
     * to check every single connection each time through.
     */

    /*  The senders queue does not hold a reference itself, so we have to manually add one before pushing
     *  this queue on.  The queue is decref'd in the network thread when it's taken out of the senders queue.
     */
    //client->outqueue->IncRef();
    if (!senders.Add (client->outqueue))
    {
        // The add failed, remove the extra reference.
        //client->outqueue->DecRef();
        Error1("Senderlist Full!");
    }

    return sendresult;
}

void NetManager::Run ()
{
    csTicks currentticks    = csGetTicks();
    csTicks lastlinkcheck   = currentticks;
    csTicks lastresendcheck = currentticks;
    csTicks laststatdisplay = currentticks;
    
    // Maximum time to spend in ProcessNetwork
    csTicks maxTime = MIN(MIN(LINKCHECK, RESENDCHECK), STATDISPLAYCHECK);
    csTicks timeout;

    long    lasttotaltransferin=0;
    long    lasttotaltransferout=0;

    long    lasttotalcountin=0;
    long    lasttotalcountout=0;

    float   kbpsout = 0;
    float   kbpsin = 0;

    float   kbpsOutMax = 0;
    float   kbpsInMax  = 0;

    size_t clientCountMax = 0;
    
    stop_network = false;
    while ( !stop_network )
    {
        if (!IsReady())
        {
            psSleep(100);
            continue;
        }

        timeout = csGetTicks() + maxTime;
        ProcessNetwork (timeout);

        currentticks = csGetTicks();
       
        // Check for link dead clients.  
        if (currentticks - lastlinkcheck > LINKCHECK)
        {
            lastlinkcheck = currentticks;
            CheckLinkDead();
        }

        // Check to resend packages that have not been ACK'd yet
        if (currentticks - lastresendcheck > RESENDCHECK)
        {
            lastresendcheck = currentticks;
            CheckResendPkts();
            CheckFragmentTimeouts();
        }

        // Display Network statistics
        if (currentticks - laststatdisplay > STATDISPLAYCHECK)
        {
            kbpsin = (float)(totaltransferin - lasttotaltransferin) / (float)STATDISPLAYCHECK;
            if ( kbpsin > kbpsInMax )
                kbpsInMax = kbpsin;
            lasttotaltransferin = totaltransferin;

            kbpsout = (float)(totaltransferout - lasttotaltransferout) / (float)STATDISPLAYCHECK;
            if ( kbpsout > kbpsOutMax )
                kbpsOutMax = kbpsout;
 
            lasttotaltransferout = totaltransferout;

            laststatdisplay = currentticks;
       
            if ( clients.Count() > clientCountMax )
               clientCountMax = clients.Count();

            if (pslog::disp_flag[LOG_LOAD])
            {
                CPrintf(CON_DEBUG, "Currently %d (Max: %d) clients using %1.2fKbps (Max: %1.2fKbps) outbound, %1.2fkbps (Max: %1.2fKbps) inbound...\n",
                        clients.Count(),clientCountMax, kbpsout, kbpsOutMax, kbpsin, kbpsInMax);
                CPrintf(CON_DEBUG, "Packets inbound %ld , outbound %ld...\n",totalcountin-lasttotalcountin,totalcountout-lasttotalcountout);
            }
        
            lasttotalcountout = totalcountout;
            lasttotalcountin = totalcountin;
        }
    }
    printf("Network thread stopped!\n");
}

void NetManager::Broadcast(MsgEntry *me, int scope, int guildID)
{
    switch (scope)
    {
        case NetBase::BC_EVERYONE:
        case NetBase::BC_EVERYONEBUTSELF:
        {
            // send the message to each client (except perhaps the client that originated it)
            uint32_t originalclient = me->clientnum;

            Client* p;

            // Copy message to send out to everyone
            MsgEntry *newmsg = new MsgEntry(me);
            newmsg->msgid = GetRandomID();

            // Message is copied again into packet sections, so we can reuse same one.
            ClientIterator i(clients);

            for (p = i.First(); p; p = i.Next())
            {    
                if (scope==NetBase::BC_EVERYONEBUTSELF
                    && p->GetClientNum() == originalclient)
                    continue;

                // send to superclient only the messages he needs
                if (p->IsSuperClient()) {
                  // time of the day is needed
                  if (me->GetType()!=MSGTYPE_WEATHER)
                    continue;
                }

                // Only clients that finished connecting get broadcastet
                // stuff                                                    
                if (!p->IsReady())
                    continue;

                newmsg->clientnum = p->GetClientNum();
                SendMessage (newmsg);
            }

            CHECK_FINAL_DECREF(newmsg,"BroadcastMsg");
            newmsg->DecRef();
            break;
        }
        // TODO: NetBase::BC_GROUP
        case NetBase::BC_GUILD:
        {
            CS_ASSERT_MSG("NetBase::BC_GUILD broadcast must specify guild ID", guildID != -1 );

            /** 
             * Send the message to each client with the same guildID 
             */
            Client* p;

            // Copy message to send out to everyone
            MsgEntry *newmsg = new MsgEntry(me);
            newmsg->msgid = (uintptr_t) newmsg;
            
            // Message is copied again into packet sections, so we can reuse same one.
            ClientIterator i(clients);

            for (p = i.First(); p; p = i.Next())
            {    
                if (p->GetGuildID() == guildID)
                {
                    newmsg->clientnum = p->GetClientNum();
                    SendMessage (newmsg);
                }
            }

        CHECK_FINAL_DECREF(newmsg,"GuildMsg");
            newmsg->DecRef();
            break;
        }
    case NetBase::BC_FINALPACKET:
    {
        MsgEntry* newmsg = new MsgEntry(me);
        newmsg->msgid = GetRandomID();

        LogMessages('F',newmsg);

        // XXX: This is hacky, but we need to send the message to the client
        // here and now! Because in the next moment he'll be deleted
        psNetPacketEntry* pkt = 
        new psNetPacketEntry(me->priority, newmsg->clientnum,
            newmsg->msgid, 0, (uint32_t) newmsg->bytes->GetTotalSize(),
            (uint16_t) newmsg->bytes->GetTotalSize(), newmsg->bytes);
        // this will also delete the pkt
        SendFinalPacket(pkt);
        delete pkt;

        CHECK_FINAL_DECREF(newmsg,"FinalPacket");
        newmsg->DecRef();
        break;
    }
    default:
        psprintf("\nIllegal Broadcast scope %d!\n",scope);
        return;
    }
}

Client *NetManager::GetClient(int cnum)
{
    return clients.Find(cnum);
}

Client *NetManager::GetAnyClient(int cnum)
{
    return clients.FindAny(cnum);
}


void NetManager::Multicast (MsgEntry* me, const csArray<PublishDestination>& multi, int except, float range)
{
    for (size_t i=0; i<multi.GetSize(); i++)
    {       
         if (multi[i].client==except)  // skip the exception client to avoid circularity
            continue;

        Client *c = clients.Find(multi[i].client);
        if (c && c->IsReady() )
        {
            if (range == 0 || multi[i].dist < range)
            {
                me->clientnum = multi[i].client;
                SendMessage(me);      // This copies the mem block, so we can reuse.
            }
        }
    }
}


void NetManager::CheckLinkDead()
{
    csTicks currenttime = csGetTicks();

    ClientIterator i(clients);

    for (Client* pClient = i.First(); pClient; pClient = i.Next() )
    {
        // Shortcut here so zombies may immediately disconnect
        if(pClient->zombie && pClient->AllowDisconnect())
        {
            /* This simulates receipt of this message from the client
            ** without any network access, so that disconnection logic
            ** is all in one place.
            */
            psDisconnectMessage discon(pClient->GetClientNum(), pClient->GetActor() && pClient->GetActor()->GetEntity() ? 
                pClient->GetActor()->GetEntity()->GetID() : 0,"You should not see this.");
            if (discon.valid)
            {
                Connection* connection = pClient->GetConnection();
                HandleCompletedMessage(discon.msg, connection, NULL,NULL);
            }
            else
            {
                Bug2("Failed to create valid psDisconnectMessage for client id %u.\n",pClient->GetClientNum());
            }
        }
        else if (pClient->GetConnection()->lastRecvPacketTime+timeout < currenttime)
        {
            if (pClient->GetConnection()->heartbeat < 10)
            {
                psHeartBeatMsg ping(pClient->GetClientNum());
                Broadcast(ping.msg, NetBase::BC_FINALPACKET);
                pClient->GetConnection()->heartbeat++;
            }
            else
            {
                if(!pClient->AllowDisconnect())
                    continue;

                char ipaddr[20];
                pClient->GetIPAddress(ipaddr);

                csString status;
                status.Format("%s, %u, Client (%s) went linkdead.", ipaddr, pClient->GetClientNum(), pClient->GetName());
                psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

                /* This simulates receipt of this message from the client
                ** without any network access, so that disconnection logic
                ** is all in one place.
                */
                psDisconnectMessage discon(pClient->GetClientNum(), pClient->GetActor() && pClient->GetActor()->GetEntity() ? 
                    pClient->GetActor()->GetEntity()->GetID() : 0,"You are linkdead.");
                if (discon.valid)
                {
                    Connection* connection = pClient->GetConnection();
                    HandleCompletedMessage(discon.msg, connection, NULL,NULL);
                }
                else
                {
                    Bug2("Failed to create valid psDisconnectMessage for client id %u.\n",pClient->GetClientNum());
                }
            }
        }
    }
}

