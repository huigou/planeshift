/*
 * clients.cpp - Author: Keith Fulton
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

#include "psserver.h"
#include "playergroup.h"
#include "globals.h"
#include "clients.h"
#include "util/psconst.h"
#include "util/log.h"
#include "netmanager.h"

//static int compareClientsByName(Client * const &, Client * const &);

ClientConnectionSet::ClientConnectionSet()
{
}

ClientConnectionSet::~ClientConnectionSet()
{
    hash.DeleteAll();
    tree.Clear();
}

bool ClientConnectionSet::Initialize()
{
    return true;
}

Client *ClientConnectionSet::Add(LPSOCKADDR_IN addr)
{
    int newclientnum;
    
    // Get a random uniq client number
    Client* testclient;
    do 
    {
        newclientnum= psserver->rng->Get(0x8fffff); //make clientnum random
        testclient = FindAny(newclientnum);
    } while (testclient != NULL);

    // Have uniq client number, create the new client
    Client* client = new Client();
    if (!client->Initialize(addr, newclientnum))
    {
        Bug1("Client Init failed?!?\n");
        delete client;
        return NULL;
    }

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    if (tree.Insert(client, true) != NULL)
        CS_ASSERT(false);
    hash.Put(client->GetClientNum(), client);
    return client;
}

/*
static int compareClientsByName(Client * const &a, Client * const &b)
{
    return strcmp(a->GetName(), b->GetName());
}
*/

void ClientConnectionSet::Delete(Client *client)
{
	CS::Threading::RecursiveMutexScopedLock lock (mutex);
    
    uint32_t clientid = client->GetClientNum();
    if (tree.Delete(client) == 0)
        Bug2("Couldn't delete client %d, it was never added!", clientid);
    hash.DeleteAll(clientid);
}

size_t ClientConnectionSet::Count() const
{
    return tree.Count();
}

Client *ClientConnectionSet::FindAny(uint32_t clientnum)
{
    if (clientnum==0)
        return NULL;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    return hash.Get(clientnum, 0);
}

Client *ClientConnectionSet::Find(uint32_t clientnum)
{
    if (clientnum==0)
        return NULL;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    Client* temp = hash.Get(clientnum, 0);

    if (temp && temp->IsReady())
        return temp;
    else
        return NULL;
}

Client *ClientConnectionSet::Find(const char* name)
{
    if (!name)
    {
        Error1("name == 0!");
        return NULL;
    }

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    BinaryRBIterator<Client> loop(&tree);
    Client *p = NULL;
    for (p = loop.First(); p; p = ++loop)
    {
        if (!p->GetName())
            continue;

        if (!strcasecmp(p->GetName(), name))
            break;
    }

    if (p && p->IsReady())
        return p;
    else
        return NULL;
}

Client *ClientConnectionSet::FindPlayer(unsigned int playerID)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    BinaryRBIterator<Client> loop(&tree);
    Client *p;

    for (p = loop.First(); p; p = ++loop)
    {
        if (p->GetPlayerID() == playerID)
            break;
    }

    return p;
}

Client *ClientConnectionSet::FindAccount(int accountID)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    BinaryRBIterator<Client> loop(&tree);
    Client *p;

    for (p = loop.First(); p; p = ++loop)
    {
        if (p->GetAccountID() == accountID)
            break;
    }

    return p;
}

Client *ClientConnectionSet::Find(LPSOCKADDR_IN addr)
{
    Client temp(addr);
   
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    return tree.Find(&temp);
}

/**
 * Any client who has an entity targeted is potentially preventing
 * that entity from being removed from the world.  For example, a group
 * of players will often all have the same mob targeted when the mob is
 * killed.  This function removes the reference from all clients.
 * This function does no networking, so the client with this target will
 * not be updated automatically by this function, but it is assumed that
 * the client will be notified of the removed entity by the same function
 * that calls this one.
 */
void ClientConnectionSet::ClearAllTargets(gemObject *obj)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    BinaryRBIterator<Client> loop(&tree);
    Client *p;

    for (p = loop.First(); p; p = ++loop)
    {
        if (p->GetTargetObject() == obj)
        {
            p->SetTargetObject(NULL);  // This fixes DecRef problems also.
        }
    }
}

ClientIterator::ClientIterator (ClientConnectionSet& clients)
    : BinaryRBIterator<Client> (&clients.tree), mutex(clients.mutex)
{
    mutex.Lock();
}

ClientIterator::~ClientIterator ()
{
    mutex.Unlock();
}
