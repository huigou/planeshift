/*
 * msgmanager.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"            // Chat Message definitions
#include "util/psdatabase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"
#include "gem.h"
#include "clients.h"
#include "globals.h"


bool MessageManager::Verify(MsgEntry *pMsg,unsigned int flags,Client*& client)
{
    client = NULL;

    gemObject *obj = NULL;
    gemActor  *actor;
    gemNPC    *npc;

    if (flags == NO_VALIDATION)
        return true; // Nothing to validate

    client = psserver->GetConnections()->FindAny(pMsg->clientnum);
    if (!client)
    {
        Warning2(LOG_NET, "MessageManager got unknown client %d!", pMsg->clientnum);
        return false;
    }
    
    if (flags & REQUIRE_READY_CLIENT)
    {
        // Infer the client MUST be ready to have sent this message.
        // NOTE: The sole purpose is to prevent the ready flag from being abused.
        //       This may still trigger ordinarily due to message reordering.
        client->SetReady(true);
    }
    if (flags & (REQUIRE_TARGET|REQUIRE_TARGETNPC|REQUIRE_TARGETACTOR))
    {
        // Check to see if this client has an npc targeted
        obj = client->GetTargetObject();
        if (!obj)
        {
            psserver->SendSystemError(pMsg->clientnum, "You do not have a target selected.");
            return false;
        }
    }
    if (flags & REQUIRE_TARGETACTOR)
    {
        actor = (obj) ? obj->GetActorPtr() : NULL;
        if (!actor)
        {
            psserver->SendSystemError(pMsg->clientnum, "You do not have a player or NPC selected.");
            return false;
        }
    }
    if (flags & REQUIRE_TARGETNPC)
    {
        npc = (obj) ? obj->GetNPCPtr() : NULL;
        if (!npc)
        {
            psserver->SendSystemError(pMsg->clientnum, "You do not have an NPC selected.");
            return false;
        }
    }
    if (flags & REQUIRE_ALIVE)
    {
        if (!client->IsAlive())
        {
            psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot do that now.");
            return false;
        }
    }
    return true;
}

Client * MessageManager::FindPlayerClient(const char *name)
{
    if (!name || strlen(name)==0)
    {
        return NULL;
    }
 
    csString playername = name;
    playername = NormalizeCharacterName(playername);
 
    Client * player = psserver->GetConnections()->Find(playername);
    return player;
}

