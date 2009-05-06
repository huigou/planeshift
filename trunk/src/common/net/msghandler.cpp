/*
 * msghandler.cpp
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
#include <psconfig.h>

#include "net/message.h"
#include "net/messages.h"
#include "net/netbase.h"
#include "net/msghandler.h"
#include "csutil/scopedlock.h"

#include "net/subscriber.h"
#include "util/psconst.h"

class Client;

MsgHandler::MsgHandler()
{
    netbase = NULL;
    queue   = NULL;
}

MsgHandler::~MsgHandler()
{   
    if (queue)
        delete queue;
}

bool MsgHandler::Initialize(NetBase* nb, int queuelen)
{
    if (!nb) return false;
    netbase = nb;

    queue = new MsgQueue(queuelen);
    if (!netbase->AddMsgQueue(queue))
        return false;

    return true;
}

void MsgHandler::Publish(MsgEntry* me)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    netbase->LogMessages('R',me);

    bool handled = false;

    int mtype = me->GetType();

    for ( size_t x = 0; x < subscribers[mtype].GetSize(); x++ )
    {
        Client *client;
        me->Reset();
		// Copy the reference so we can modify it in the loop
		MsgEntry *message = me;
        if (subscribers[mtype][x]->subscriber->Verify(message,subscribers[mtype][x]->flags,client))
        {
            if (subscribers[mtype][x]->callback)
	            subscribers[mtype][x]->callback->Call(message,client);
		    else
			    subscribers[mtype][x]->subscriber->HandleMessage(message,client);
        }
        handled = true;
    }

    if (!handled)
    {
        Debug4(LOG_ANY,me->clientnum,"Unhandled message received 0x%04X(%d) from %d",
               me->GetType(), me->GetType(), me->clientnum);
    }
}

bool MsgHandler::Subscribe(iNetSubscriber *subscriber, msgtype type,uint32_t flags)
{
    Subscription* p = new Subscription;

    p->subscriber = subscriber;
    p->callback   = NULL; 
    p->type       = type;
    p->flags      = flags;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    if ( IsSubscribed(p) )
        delete p;
    else
        subscribers[type].Push(p);

    return true;
}

bool MsgHandler::Subscribe(iNetSubscriber *subscriber, MsgtypeCallback *callback, msgtype type,uint32_t flags)
{
    Subscription* p = new Subscription;

    p->subscriber = subscriber;
    p->callback   = callback;
    p->type       = type;
    p->flags      = flags;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    if ( IsSubscribed(p) )
    {
        if (p->callback)
            delete p->callback;
        delete p;
    }
    else
        subscribers[type].Push(p);

    return true;
}

bool MsgHandler::Unsubscribe(iNetSubscriber *subscriber, msgtype type)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    for ( size_t x = 0; x < subscribers[type].GetSize(); x++ )
    {
        if (subscribers[type][x]->subscriber == subscriber)
        {
            delete subscribers[type][x]->callback; // delete functor if present
            subscribers[type].DeleteIndex(x);
            return true;
        }        
    }

    return false;
}


bool MsgHandler::IsSubscribed( Subscription* sub )
{
    for ( size_t x = 0; x < subscribers[sub->type].GetSize(); x++ )
    {
        if (subscribers[sub->type][x]->subscriber == sub->subscriber)
        {
            return true;
        }        
    }

    return false;
}
