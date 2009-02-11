/*
 * msghandler.h by Matze Braun <MatzeBraun@gmx.de>
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
#ifndef __MSGHANDLER_H__
#define __MSGHANDLER_H__

#include <csutil/parray.h>
#include <csutil/refcount.h>
#include <csutil/threading/thread.h>

#include "net/message.h"
#include "net/netbase.h"

// forward decls
struct iNetSubscriber;
class NetBase;
class Client;

#define  MAX_MESSAGE_TYPES 255   // MSGTYPE enumeration in messages.h has less types than this

/// Functor base class for direct callbacks on Subscriptions
class MsgtypeCallback
{
public:
    virtual ~MsgtypeCallback() { };
    virtual void operator()(MsgEntry *message, Client *client) = 0; // call using operator
    virtual void Call(MsgEntry *message, Client *client)       = 0; // call using function
};

template <class Manager>
class NetMessageCallback : public MsgtypeCallback
{
 private:
      void (Manager::*funcptr)(MsgEntry *message, Client *client);   // pointer to member function
      Manager *thisPtr;                                              // pointer to object

   public:

      // constructor - takes pointer to an object and pointer to a member and stores
      // them in two private variables
      NetMessageCallback(Manager *myObject, void(Manager::*fpt)(MsgEntry *message, Client *client))
      { 
          thisPtr = myObject;
          funcptr = fpt;
      }

      virtual ~NetMessageCallback() { };

      // override operator "()"
      virtual void operator()(MsgEntry *message, Client *client)
      {
          (*thisPtr.*funcptr)(message,client);
      }

      // override function "Call"
      virtual void Call(MsgEntry *message, Client *client)
      {
          (*thisPtr.*funcptr)(message,client);
      }
};



/// This little struct tracks who is interested in what.
struct Subscription
{
    /// type of the messages this listener listens to
    msgtype type;

    /// Flags for central testing
    uint32_t flags;

    /// pointer to the subscriber
    iNetSubscriber *subscriber;

    /// pointer to functor class callback as alternative to iNetSubscriber
    MsgtypeCallback *callback;
};


/// This class handles the incoming network packets
class MsgHandler : public csRefCount
{
public:
    MsgHandler();
    virtual ~MsgHandler();

    /** Initializes the Handler */
    bool Initialize(NetBase *nb, int queuelen = 500);

    /** Any subclass of iNetSubscriber can subscribe to incoming network messages
     * with this function
     */
    virtual bool Subscribe(iNetSubscriber *subscriber, msgtype type, uint32_t flags = 0x01/*REQUIRE_READY_CLIENT*/);
    virtual bool Subscribe(iNetSubscriber *subscriber, MsgtypeCallback *callback, msgtype type, uint32_t flags = 0x01/*REQUIRE_READY_CLIENT*/);

    /// Remove subscriber from list
    virtual bool Unsubscribe(iNetSubscriber *subscriber, msgtype type);

    /// Distribute message to all subscribers
    void Publish(MsgEntry *msg);

    /// import the broadcasttype
    typedef NetBase::broadcasttype broadcasttype;

    /// Allows subscribers to respond with new messages
    virtual void SendMessage(MsgEntry *msg)
    { netbase->SendMessage(msg); }

    /// Send messages to many clients with one func call
    virtual void Broadcast(MsgEntry* msg, broadcasttype scope = NetBase::BC_EVERYONEBUTSELF, int guildID=-1)
    { netbase->Broadcast(msg, scope, guildID); }

    /// Send messages to all listed client nums.
    virtual void Multicast(MsgEntry* msg, csArray<PublishDestination>& multi, int except, float range)
    { netbase->Multicast(msg, multi, except, range); }

    /// Detects multiple subscriptions on the same object
    bool IsSubscribed (Subscription* p );

    void AddToLocalQueue(MsgEntry *me) { netbase->QueueMessage(me); }

    csTicks GetPing() { return netbase->GetPing(); }

    /// Flush the connected output queue.
    bool Flush() { return netbase->Flush(queue); }

protected:
    NetBase                       *netbase;
    MsgQueue                      *queue;

    /** 
     * Each message type now has an array of subscribers so we can publish 
     * to them directly instead of searching the entire list of all subscribers.
     */
    csPDelArray<Subscription>      subscribers[MAX_MESSAGE_TYPES];
    CS::Threading::RecursiveMutex  mutex;
};

#endif

