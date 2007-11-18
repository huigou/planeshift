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

#include <csutil/hash.h>
#include <csutil/set.h>

#include "client.h"
#include "clients.h"
#include "gem.h"
#include "globals.h"
#include "psserver.h"
#include "bulkobjects/pscharacter.h"
#include "globals.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"
#include "introductionmanager.h"

IntroductionManager::IntroductionManager() : introMap(10000, 10000, 2000000)
{
    Result r = db->Select("select * from introductions");
    if (r.IsValid())
    {
        for (unsigned long i = 0 ; i < r.Count() ; i++)
        {
            unsigned int charid = r[i].GetUInt32("charid");
            unsigned int charintroid = r[i].GetUInt32("introcharid");
            if (!introMap.Contains(charid))
                introMap.Put(charid, *(new csSet<unsigned int>(10000, 10000, 2000000)));
            introMap[charid]->Add(charintroid);  
        }
    }

    psserver->GetEventManager()->Subscribe(this,MSGTYPE_INTRODUCTION,REQUIRE_READY_CLIENT);
}

bool IntroductionManager::Introduce(unsigned int charid, unsigned int targetcharid)
{
    if (IsIntroduced(charid, targetcharid))
        return false;

    introMap[charid]->Add(targetcharid);

    db->CommandPump("insert into introductions values(%d, %d)", charid, targetcharid);

    return true;
}

bool IntroductionManager::UnIntroduce(unsigned int charid, unsigned int targetcharid)
{
    if (!IsIntroduced(charid, targetcharid))
        return false;

    introMap[charid]->Delete(targetcharid);

    db->CommandPump("delete from introductions where charid=%d and introcharid=%d", charid, targetcharid);

    return true;
}

bool IntroductionManager::IsIntroduced(unsigned int charid, unsigned int targetcharid)
{
    return introMap[charid]->Contains(targetcharid);
}

void IntroductionManager::HandleMessage(MsgEntry *pMsg,Client *client)
{
    psCharIntroduction msg(pMsg);
    if (!msg.valid)
        return;
    if (!(client->GetTargetObject()) || !(client->GetTargetObject()->GetActorPtr()))
    {
        psserver->SendSystemError(client->GetClientNum(), "You must target another player");
        return;
    }

    Introduce(client->GetTargetObject()->GetActorPtr()->GetCharacterData()->GetCharacterID(), 
                client->GetCharacterData()->GetCharacterID());
    
    psserver->SendSystemOK(client->GetClientNum(), "You have successfully introduced yourself");

    client->GetActor()->Send(client->GetTargetClientID(), false, false);
}