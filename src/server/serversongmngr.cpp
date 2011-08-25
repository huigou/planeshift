/*
 * serversongmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include "serversongmngr.h"

//====================================================================================
// Project Includes
//====================================================================================
#include <net/messages.h>
#include <util/music.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "globals.h"
#include "client.h"
#include "gem.h"
#include "psproxlist.h"

#include "bulkobjects/pscharacter.h"


ServerSongManager::ServerSongManager()
{
    Subscribe(&ServerSongManager::HandlePlaySong, MSGTYPE_MUSICAL_SHEET, REQUIRE_READY_CLIENT);
    Subscribe(&ServerSongManager::HandleStopSong, MSGTYPE_STOP_SONG, REQUIRE_READY_CLIENT);
}

void ServerSongManager::HandlePlaySong(MsgEntry* me, Client* client)
{
    psMusicalSheetMessage musicMsg(me);

    if (musicMsg.valid && musicMsg.play)
    {
        psItem *item = client->GetCharacterData()->Inventory().FindItemID(musicMsg.itemID);

        // playing
        if(item != 0)
        {
            float errorRate;
            const char* instrName;
            psItem* instrItem;
            csArray<PublishDestination> proxList;

            psCharacter* charData = client->GetCharacterData();
            gemActor* charActor = charData->GetActor();

            // checking if the client's player is already playing something
            if(charData->IsPlaying())
            {
                return;
            }

            // getting equipped instrument
            instrItem = charData->Inventory().GetItem(0, PSCHARACTER_SLOT_RIGHTHAND);
            if(instrItem == 0 /* || !instrItem->GetIsInstrument() */)     // TODO uncomment when the instruments are added and implemented
            {
                instrItem = charData->Inventory().GetItem(0, PSCHARACTER_SLOT_LEFTHAND);
                if(instrItem == 0 /* || !instrItem->GetIsInstrument() */)     // TODO uncomment when the instruments are added and implemented
                {
                    // sending an error message
                    psStopSongMessage stopMsg(client->GetClientNum(), 0, true, false);
                    stopMsg.SendMessage();
                    return;
                }
            }

            instrName = instrItem->GetName();

            // getting error rate
            // TODO the commented one is just an example of implementation
            /*
            MathEnvironment env;
            MathScript* errorRateScript;
            PSSKILL instrumentSkill;

            instrumentSkill = instrItem->GetInstrumentSkill()
            errorRateScript  = psserver->GetMathScriptEngine()->FindScript("Compute Song Error");
            env.Define("InstrumentSkill", charData->Skills().GetSkillRank(instrumentSkill).Current());
            errorRateScript->Evaluate(&env);
            errorRate = env.Lookup("ErrorRate")->GetValue();
            */
            errorRate = 0;

            // TODO here it should be determined if the actions is a success/failure and manage the player's skill

            // sending message to requester, it's useless to send the musical sheet again
            psPlaySongMessage sendedPlayMsg(client->GetClientNum(), charActor->GetEID().Unbox(), true, errorRate, instrName, 0, "");
            sendedPlayMsg.SendMessage();

            // preparing compressed musical sheet
            csString compressedScore;
            Music::ZCompressSong(musicMsg.musicalSheet, compressedScore);

            // sending play messages to proximity list
            proxList = charActor->GetProxList()->GetClients();
            for(size_t i = 0; i < proxList.GetSize(); i++)
            {
                if(client->GetClientNum() == proxList[i].client)
                {
                    continue;
                }
                psPlaySongMessage sendedPlayMsg(proxList[i].client, charActor->GetEID().Unbox(), false, errorRate, instrName, compressedScore.Length(), compressedScore);
                sendedPlayMsg.SendMessage();
            }

            // updating character status
            charData->SetPlaying(true);
        }
        else
        {
            Error3("Item %u not found in musical sheet message from client %s.", musicMsg.itemID, client->GetName());
        }
    }
}

void ServerSongManager::HandleStopSong(MsgEntry* me, Client* client)
{
    psStopSongMessage receivedStopMsg(me);

    if(receivedStopMsg.valid)
    {
        gemActor* charActor;
        psCharacter* charData;
        csArray<PublishDestination> proxList;

        charData = client->GetCharacterData();
        charActor = charData->GetActor();

        // checking that the client's player is actually playing
        if(charData->IsPlaying() == false)
        {
            return;
        }

        // forwarding the message to the whole proximity list
        // if the song has ended there's no need to send it
        if(!receivedStopMsg.isEnded)
        {
            proxList = charActor->GetProxList()->GetClients();
            for(size_t i = 0; i < proxList.GetSize(); i++)
            {
                if(client->GetClientNum() == proxList[i].client)
                {
                    continue;
                }
                psStopSongMessage sendedStopMsg(proxList[i].client, charActor->GetEID().Unbox(), false, false);
                sendedStopMsg.SendMessage();
            }
        }

        // updating character status
        charData->SetPlaying(false);
    }
}
