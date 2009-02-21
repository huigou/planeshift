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
#include "util/strutil.h"

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

csArray<csString> MessageManager::DecodeCommandArea(Client *client, csString target)
{
    csArray<csString> result;

    if (!psserver->CheckAccess(client, "command area"))
        return result;

    csArray<csString> splitTarget = psSplit(target, ':');
    size_t splitSize = splitTarget.GetSize();

    if (splitSize < 3 || splitSize > 4)
    {
        psserver->SendSystemError(client->GetClientNum(),
                "Try /$CMD area:item:range[:name]");
        return result;
    }

    csString itemName = splitTarget[1];
    csString* nameFilter = splitSize > 3 ? &splitTarget[3] : 0;

    const int range = atoi(splitTarget[2].GetData());
    if (range <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(),
                "You must specify a positive integer for the area: range.");
        return result;
    }

    bool allNames = true;
    if (nameFilter && (*nameFilter!="all"))
        allNames = false;

    int mode;
    if (itemName == "players")
        mode = 0;
    else if (itemName == "actors")
        mode = 1;
    else if (itemName == "items")
        mode = 2;
    else if (itemName == "npcs")
        mode = 3;
    else if (itemName == "entities")
        mode = 4;
    else
    {
        psserver->SendSystemError(client->GetClientNum(),
                "item must be players|actors|items|npcs|entities");
        return result;
    }

    gemActor* self = client->GetActor();
    if (!self)
    {
        psserver->SendSystemError(client->GetClientNum(), "You do not exist...");
        return result;
    }

    csVector3 pos;
    iSector* sector;
    self->GetPosition(pos, sector);

    GEMSupervisor* gem = GEMSupervisor::GetSingletonPtr();
    if (!gem)
        return result;

    csArray<gemObject*> nearlist = gem->FindNearbyEntities(sector, pos,
            range);
    size_t count = nearlist.GetSize();
    csArray<csString *> results;

    for (size_t i=0; i<count; i++)
    {
        gemObject *nearobj = nearlist[i];
        if (!nearobj)
            continue;

        if (nearobj->GetInstance() != self->GetInstance())
            continue;

        if (!allNames)
        {
            csString nearobjName = nearobj->GetName();
            if (!nearobjName.StartsWith(*nameFilter->GetData(), true))
                continue;
        }

        csString newTarget;

        switch (mode)
        {
        case 0: // Target players
        {
            if (nearobj->GetClientID())
            {
                newTarget.Format("pid:%d", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 1: // Target actors
        {
            if (nearobj->GetPID().IsValid())
            {
                newTarget.Format("pid:%d", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 2: // Target items
        {
            if (nearobj->GetItem())
            {
                newTarget.Format("eid:%u", nearobj->GetEID().Unbox());
                break;
            }
            else
                continue;
        }
        case 3: // Target NPCs
        {
            if (nearobj->GetNPCPtr())
            {
                newTarget.Format("pid:%u", nearobj->GetPID().Unbox());
                break;
            }
            else
                continue;
        }
        case 4: // Target everything
        {
            newTarget.Format("eid:%u", nearobj->GetEID().Unbox());
            break;
        }
        }

        // Run this once for every target in range  (each one will be verified and logged seperately)
        result.Push(newTarget);
    }
    if (result.IsEmpty())
    {
        psserver->SendSystemError(client->GetClientNum(),
                "Nothing of specified type in range.");
    }
    return result;
}

gemObject* MessageManager::FindObjectByString(const csString& str, gemActor * me) const
{
    gemObject* found = NULL;
    GEMSupervisor *gem = GEMSupervisor::GetSingletonPtr();
    if (!gem)
    {
        return NULL;
    }

    if ( str.StartsWith("pid:",true) ) // Find by player ID
    {
        csString pid_str = str.Slice(4);
        PID pid = PID(strtoul(pid_str.GetDataSafe(), NULL, 10));
        if (pid.IsValid())
            found = gem->FindPlayerEntity(pid);
    }
    else if ( str.StartsWith("eid:",true) ) // Find by entity ID
    {
        csString eid_str = str.Slice(4);
        EID eid = EID(strtoul(eid_str.GetDataSafe(), NULL, 10));
        if (eid.IsValid())
            found = gem->FindObject(eid);
    }
    else if ( str.StartsWith("itemid:",true) ) // Find by item UID
    {
        csString itemid_str = str.Slice(7);
        uint32 itemID = strtoul(itemid_str.GetDataSafe(), NULL, 10);
        if (itemID != 0)
            found = gem->FindItemEntity(itemID);
    }
    else if ( me != NULL && str.CompareNoCase("me") ) // Return me
    {
        found = me;
    }
    else // Try finding an entity by name
    {
        found = gem->FindObject(str);
    }

    return found;
}
