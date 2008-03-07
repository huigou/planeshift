/*
* introductionmanager.cpp
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <csutil/set.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"

#include "util/psdatabase.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "introductionmanager.h"
#include "chatmanager.h"
#include "client.h"
#include "clients.h"
#include "gem.h"
#include "globals.h"
#include "psserver.h"
#include "globals.h"

IntroductionManager::IntroductionManager() : introMap(100, 10, 500)
{
    psserver->GetEventManager()->Subscribe(this,MSGTYPE_INTRODUCTION,REQUIRE_READY_CLIENT);
}

IntroductionManager::~IntroductionManager()
{
    csHash< csSet<unsigned int>* >::GlobalIterator iter = introMap.GetIterator();
    while(iter.HasNext())
    {
        csSet<unsigned int>* currSet = iter.Next();
        delete currSet;
    }
}

bool IntroductionManager::LoadCharIntroductions(unsigned int charid)
{
    if (introMap.Contains(charid))
        return true;

    Result r = db->Select("select * from introductions where charid=%d", charid);
    if (r.IsValid())
    {
        csSet<unsigned int> *newSet = new csSet<unsigned int>(100, 10, 500);
        introMap.Put(charid, newSet);
        for (unsigned long i = 0 ; i < r.Count() ; i++)
        {
            unsigned int charintroid = r[i].GetUInt32("introcharid");
            newSet->Add(charintroid);  
        }
        return true;
    }

    return false;
}
bool IntroductionManager::UnloadCharIntroductions(unsigned int charid)
{
    if (!introMap.Contains(charid))
        return true;

    delete introMap.Get(charid, NULL);

    introMap.DeleteAll(charid);

    return true;
}

bool IntroductionManager::Introduce(unsigned int charid, unsigned int targetcharid)
{
    if (IsIntroduced(charid, targetcharid))
        return false;

    csSet<unsigned int> *targetSet = introMap.Get(charid, NULL);
    if (targetSet)
    {
        targetSet->Add(targetcharid);
        db->CommandPump("insert into introductions values(%d, %d)", charid, targetcharid);
    }

    return true;
}

bool IntroductionManager::UnIntroduce(unsigned int charid, unsigned int targetcharid)
{
    if (!IsIntroduced(charid, targetcharid))
        return false;

    csSet<unsigned int> *targetSet = introMap.Get(charid, NULL);
    if (targetSet)
    {
        targetSet->Delete(targetcharid);
        db->CommandPump("delete from introductions where charid=%d and introcharid=%d", charid, targetcharid);
    }

    return true;
}

// IsIntroduced(A,B) iff A knows B
bool IntroductionManager::IsIntroduced(unsigned int charid, unsigned int targetcharid)
{
    csSet<unsigned int> *targetSet = introMap.Get(charid, NULL);
    if (!targetSet)
        return false;

    return targetSet->Contains(targetcharid);
}

void IntroductionManager::HandleMessage(MsgEntry *pMsg, Client *client)
{
    psCharIntroduction msg(pMsg);
    if (!msg.valid)
        return;

    gemActor *target = client->GetTargetObject() ? client->GetTargetObject()->GetActorPtr() : NULL;

    // If player targeted another character...introduce only to him/her
    if (target)
    {
        Introduce(target->GetCharacterData()->GetCharacterID(), client->GetCharacterData()->GetCharacterID());
        
        psserver->SendSystemOK(client->GetClientNum(), "You have successfully introduced yourself.");
        psserver->SendSystemOK(client->GetTargetClientID(), "%s was introduced to you", client->GetName());
        
        client->GetActor()->Send(client->GetTargetClientID(), false, false);
    }
    else // introduce to everyone in /say range (except self)
    {
        csArray<PublishDestination>& dest = client->GetActor()->GetMulticastClients();
        for (size_t i = 0; i < dest.GetSize(); i++)
        {
            if (dest[i].dist < CHAT_SAY_RANGE && client->GetClientNum() != (uint32_t) dest[i].client)
            {
                gemObject *obj = (gemObject*) dest[i].object;
                gemActor *destActor = obj->GetActorPtr();
                if (destActor)
                {
                    Introduce(destActor->GetCharacterData()->GetCharacterID(), client->GetCharacterData()->GetCharacterID());
                    psserver->SendSystemOK(dest[i].client, "%s was introduced to you", client->GetName());
                    client->GetActor()->Send(dest[i].client, false, false);
                }
            }
        }
        //psserver->SendSystemOK(client->GetClientNum(), "You introduced yourself to everyone around you.");
    }
}

