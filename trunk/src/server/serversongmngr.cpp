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
#include "bulkobjects/psmerchantinfo.h"


ServerSongManager::ServerSongManager()
{
    isEnded = false;

    Subscribe(&ServerSongManager::HandlePlaySongMessage, MSGTYPE_MUSICAL_SHEET, REQUIRE_READY_CLIENT);
    Subscribe(&ServerSongManager::HandleStopSongMessage, MSGTYPE_STOP_SONG, REQUIRE_READY_CLIENT);

    calcSongPar  = psserver->GetMathScriptEngine()->FindScript("Calculate Song Parameters");
    calcSongExp  = psserver->GetMathScriptEngine()->FindScript("Calculate Song Experience");

    CS_ASSERT_MSG("Could not load mathscript 'Calculate Song Parameters'", calcSongPar);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Song Experience'", calcSongExp);
}

bool ServerSongManager::Initialize()
{
    csString instrCatStr;

    if(psserver->GetServerOption("instruments_category", instrCatStr))
    {
        instrumentsCategory = atoi(instrCatStr.GetData());
        return true;
    }
    else
    {
        return false;
    }
}

void ServerSongManager::HandlePlaySongMessage(MsgEntry* me, Client* client)
{
    psMusicalSheetMessage musicMsg(me);

    if (musicMsg.valid && musicMsg.play)
    {
        psItem *item = client->GetCharacterData()->Inventory().FindItemID(musicMsg.itemID);

        // playing
        if(item != 0)
        {
            float errorRate;
            psItem* instrItem;
            const char* instrName;

            MathEnvironment mathEnv;
            csArray<PublishDestination> proxList;

            psCharacter* charData = client->GetCharacterData();
            gemActor* charActor = charData->GetActor();

            // checking if the client's player is already playing something
            if(charActor->GetMode() == PSCHARACTER_MODE_PLAY)
            {
                return;
            }

            // getting equipped instrument
            instrItem = GetEquippedInstrument(charData);
            if(instrItem == 0)
            {
                // sending an error message
                psStopSongMessage stopMsg(client->GetClientNum(), 0, true, false);
                stopMsg.SendMessage();
                return;
            }

            instrName = instrItem->GetName();

            // calculating song parameters
            if(calcSongPar == 0)
            {
                errorRate = 0;
            }
            else
            {
                mathEnv.Define("Player", client->GetActor());
                mathEnv.Define("Instrument", instrItem);
                calcSongPar->Evaluate(&mathEnv);

                errorRate = mathEnv.Lookup("ErrorRate")->GetValue();
            }

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

            // updating character mode
            charActor->SetMode(PSCHARACTER_MODE_PLAY);
        }
        else
        {
            Error3("Item %u not found in musical sheet message from client %s.", musicMsg.itemID, client->GetName());
        }
    }
}

void ServerSongManager::HandleStopSongMessage(MsgEntry* me, Client* client)
{
    psStopSongMessage receivedStopMsg(me);

    if(receivedStopMsg.valid)
    {
        gemActor* charActor = client->GetActor();

        // checking that the client's player is actually playing
        if(charActor->GetMode() != PSCHARACTER_MODE_PLAY)
        {
            return;
        }

        // setting isEnded flag for the next StopSong call
        isEnded = receivedStopMsg.isEnded;

        // updating character mode, this will call StopSong
        charActor->SetMode(PSCHARACTER_MODE_PEACE);

        // resetting isEnded flag
        isEnded = false;
    }
}

void ServerSongManager::StopSong(gemActor* charActor)
{
    psItem* instrItem;
    psCharacter* charData = charActor->GetCharacterData();

    // forwarding the message to the whole proximity list
    // if the song has ended there's no need to send it
    if(!isEnded)
    {
        int charClientID = charActor->GetProxList()->GetClientID();
        csArray<PublishDestination> proxList = charActor->GetProxList()->GetClients();

        for(size_t i = 0; i < proxList.GetSize(); i++)
        {
            if(charClientID == proxList[i].client)
            {
                psStopSongMessage sendedStopMsg(proxList[i].client, charActor->GetEID().Unbox(), true, false);
                sendedStopMsg.SendMessage();
            }
            else
            {
                psStopSongMessage sendedStopMsg(proxList[i].client, charActor->GetEID().Unbox(), false, false);
                sendedStopMsg.SendMessage();
            }
        }
    }

    // handling skill ranking
    instrItem = GetEquippedInstrument(charData);
    if(instrItem != 0 && calcSongPar != 0 && calcSongExp != 0)
    {
        MathEnvironment mathEnv;
        int practicePoints;
        float modifier;
        PSSKILL instrSkill;

        mathEnv.Define("Player", charActor);
        mathEnv.Define("Instrument", instrItem);
        mathEnv.Define("SongTime", charData->GetPlayingTime() / 1000);
        mathEnv.Define("AverageDuration", 1);   // TODO to be implemented
        mathEnv.Define("AveragePolyphony", 1);  // TODO to be implemented

        calcSongPar->Evaluate(&mathEnv);
        calcSongExp->Evaluate(&mathEnv);

        practicePoints = mathEnv.Lookup("PracticePoints")->GetRoundValue();
        modifier = mathEnv.Lookup("Modifier")->GetValue();
        instrSkill = (PSSKILL)(mathEnv.Lookup("InstrSkill")->GetRoundValue());

        charData->CalculateAddExperience(instrSkill, practicePoints, modifier);
    }
}

psItem* ServerSongManager::GetEquippedInstrument(psCharacter* charData) const
{
    psItem* instrItem;

    instrItem = charData->Inventory().GetItem(0, PSCHARACTER_SLOT_RIGHTHAND);
    if(instrItem == 0 || instrItem->GetCategory()->id != instrumentsCategory)
    {
        instrItem = charData->Inventory().GetItem(0, PSCHARACTER_SLOT_LEFTHAND);
        if(instrItem != 0 && instrItem->GetCategory()->id != instrumentsCategory)
        {
            instrItem = 0;
        }
    }

    return instrItem;
}

