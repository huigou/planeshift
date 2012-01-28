/*
 * serversongmngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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


psEndSongEvent::psEndSongEvent(gemActor* actor, int songLength)
    : psGameEvent(0, songLength, "psEndSongEvent")
{
    charActor = actor;
}

psEndSongEvent::~psEndSongEvent()
{
    // obviously charActor must not be deleted :P
}

bool psEndSongEvent::CheckTrigger()
{
    return charActor->GetMode() == PSCHARACTER_MODE_PLAY;
}

void psEndSongEvent::Trigger()
{
    ServerSongManager::GetSingleton().OnStopSong(charActor, true);
}


//----------------------------------------------------------------------------


ServerSongManager::ServerSongManager()
{
    isProcessedSongEnded = false;

    Subscribe(&ServerSongManager::HandlePlaySongMessage, MSGTYPE_MUSICAL_SHEET, REQUIRE_READY_CLIENT);
    Subscribe(&ServerSongManager::HandleStopSongMessage, MSGTYPE_STOP_SONG, REQUIRE_READY_CLIENT);

    psserver->GetMathScriptEngine()->CheckAndUpdateScript(calcSongPar, "Calculate Song Parameters");
    psserver->GetMathScriptEngine()->CheckAndUpdateScript(calcSongExp, "Calculate Song Experience");

    CS_ASSERT_MSG("Could not load mathscript 'Calculate Song Parameters'", calcSongPar.IsValid());
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Song Experience'", calcSongExp.IsValid());
}

ServerSongManager::~ServerSongManager()
{
    // deleting scores' ranks
    scoreRanks.DeleteAll();

    // unsubscribing from message notifications
    Unsubscribe(MSGTYPE_MUSICAL_SHEET);
    Unsubscribe(MSGTYPE_STOP_SONG);
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

    if(musicMsg.valid && musicMsg.play)
    {
        psItem *item = client->GetCharacterData()->Inventory().FindItemID(musicMsg.itemID);

        // playing
        if(item != 0)
        {
            int canPlay;
            int scoreRank;
            uint32 actorEID;
            float durationFactor;
            float playerMinDuration;
            psItem* instrItem;
            const char* instrName;
            ScoreStatistics scoreStats;

            MathEnvironment mathEnv;
            csArray<PublishDestination> proxList;

            psCharacter* charData = client->GetCharacterData();
            gemActor* charActor = charData->GetActor();

            // checking if the client's player is already playing something
            if(charActor->GetMode() == PSCHARACTER_MODE_PLAY)
            {
                return;
            }

            // checking if the score is valid
            csRef<iDocumentSystem> docSys = csQueryRegistry<iDocumentSystem>(psserver->GetObjectReg());;
            csRef<iDocument> scoreDoc = docSys->CreateDocument();
            scoreDoc->Parse(musicMsg.musicalSheet, true);
            if(!psMusic::GetStatistics(scoreDoc, scoreStats))
            {
                // sending an error message
                psStopSongMessage stopMsg(client->GetClientNum(), 0, true, psStopSongMessage::ILLEGAL_SCORE);
                stopMsg.SendMessage();
                return;
            }

            // getting equipped instrument
            instrItem = GetEquippedInstrument(charData);
            if(instrItem == 0)
            {
                // sending an error message
                psStopSongMessage stopMsg(client->GetClientNum(), 0, true, psStopSongMessage::NO_INSTRUMENT);
                stopMsg.SendMessage();
                return;
            }

            instrName = instrItem->GetName();
            actorEID = charActor->GetEID().Unbox();

            // calculating song parameters
            
            if(!psserver->GetMathScriptEngine()->CheckAndUpdateScript(calcSongPar, "Calculate Song Parameters"))
            {
                canPlay = -1; // the song won't be played
            }
            else
            {
                // input variables
                mathEnv.Define("Player", client->GetActor());
                mathEnv.Define("Instrument", instrItem);
                mathEnv.Define("Fifths", scoreStats.fifths);
                mathEnv.Define("BeatType", scoreStats.beatType);
                mathEnv.Define("AverageDuration", scoreStats.averageDuration);
                mathEnv.Define("AveragePolyphony", scoreStats.averagePolyphony);
                mathEnv.Define("MinimumDuration", scoreStats.minimumDuration);
                mathEnv.Define("MaximumPolyphony", scoreStats.maximumPolyphony);

                // script evaluation
                calcSongPar->Evaluate(&mathEnv);

                // output variables
                canPlay = mathEnv.Lookup("CanPlay")->GetValue();
                scoreRank = mathEnv.Lookup("ScoreRank")->GetValue();
                durationFactor = mathEnv.Lookup("DurationFactor")->GetValue();
                playerMinDuration = mathEnv.Lookup("PlayerMinimumDuration")->GetValue();
            }

            if(canPlay >= 0)
            {
                // sending message to requester, it's useless to send the musical sheet again
                psPlaySongMessage sendedPlayMsg(client->GetClientNum(), actorEID, true, playerMinDuration, instrName, 0, "");
                sendedPlayMsg.SendMessage();

                // if the score is empty or has only rests there's no need to inform other clients
                if(scoreStats.averageDuration != 0.0)
                {
                    // preparing compressed musical sheet
                    csString compressedScore;
                    psMusic::ZCompressSong(musicMsg.musicalSheet, compressedScore);

                    // sending play messages to proximity list
                    proxList = charActor->GetProxList()->GetClients();
                    for(size_t i = 0; i < proxList.GetSize(); i++)
                    {
                        if(client->GetClientNum() == proxList[i].client)
                        {
                            continue;
                        }
                        psPlaySongMessage sendedPlayMsg(proxList[i].client, actorEID, false, playerMinDuration, instrName, compressedScore.Length(), compressedScore);
                        sendedPlayMsg.SendMessage();
                    }
                }

                // updating character mode and item status
                charActor->SetMode(PSCHARACTER_MODE_PLAY);
                instrItem->SetInUse(true);

                // keeping track of the song's data
                psEndSongEvent* event = new psEndSongEvent(charActor, scoreStats.totalLength * durationFactor);
                psserver->GetEventManager()->Push(event);
                scoreRanks.Put(actorEID, scoreRank);
            }
            else
            {
                // sending an error message
                psStopSongMessage stopMsg(client->GetClientNum(), 0, true, psStopSongMessage::LOW_SKILL);
                stopMsg.SendMessage();
                return;
            }
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
        OnStopSong(client->GetActor(), false);
    }
}

void ServerSongManager::OnStopSong(gemActor* charActor, bool isEnded)
{
    // checking that the client's player is actually playing
    if(charActor->GetMode() != PSCHARACTER_MODE_PLAY)
    {
        return;
    }

    isProcessedSongEnded = isEnded;

    // updating character mode, this will call StopSong
    charActor->SetMode(PSCHARACTER_MODE_PEACE);
}

void ServerSongManager::StopSong(gemActor* charActor, bool skillRanking)
{
    int scoreRank;
    psItem* instrItem;
    csTicks bonusTime = 0;
    uint32 actorEID = charActor->GetEID().Unbox();
    psCharacter* charData = charActor->GetCharacterData();
    int charClientID = charActor->GetProxList()->GetClientID();

    // forwarding the message to the whole proximity list
    if(!isProcessedSongEnded) // no need to send it
    {
        csArray<PublishDestination> proxList = charActor->GetProxList()->GetClients();

        for(size_t i = 0; i < proxList.GetSize(); i++)
        {
            if(charClientID != proxList[i].client)
            {
                psStopSongMessage sendedStopMsg(proxList[i].client, actorEID, false, psStopSongMessage::NO_SONG_ERROR);
                sendedStopMsg.SendMessage();
            }
        }
    }
    else // client needs to be notified if sounds are disabled
    {
        // resetting flag
        isProcessedSongEnded = false;

        // notifying the client
        psStopSongMessage sendedStopMsg(charClientID, actorEID, true, psStopSongMessage::NO_SONG_ERROR);
        sendedStopMsg.SendMessage();
    }

    // handling skill ranking
    instrItem = GetEquippedInstrument(charData);
    scoreRank = scoreRanks.Get(actorEID, -10000);  // score rank should always be positive

    if(instrItem != 0 && scoreRank != -10000)
    {
        // unlocking instrument
        instrItem->SetInUse(false);

        //check and update scripts
        psserver->GetMathScriptEngine()->CheckAndUpdateScript(calcSongPar, "Calculate Song Parameters");
        psserver->GetMathScriptEngine()->CheckAndUpdateScript(calcSongExp, "Calculate Song Experience");

        if(skillRanking && calcSongPar.IsValid() && calcSongExp.IsValid())
        {
            MathEnvironment mathEnv;
            int practicePoints;
            float modifier;
            PSSKILL instrSkill;

            // input variables
            mathEnv.Define("Player", charActor);
            mathEnv.Define("Instrument", instrItem);
            mathEnv.Define("SongTime", charData->GetPlayingTime() / 1000);
            mathEnv.Define("ScoreRank", scoreRank);

            // scripts evaluation
            calcSongExp->Evaluate(&mathEnv);

            // output variables
            // practicePoints is always truncated (not rounded) since the remaining
            // time is taken into account by bonusTime
            practicePoints = mathEnv.Lookup("PracticePoints")->GetValue();
            modifier = mathEnv.Lookup("Modifier")->GetValue();
            instrSkill = (PSSKILL)(mathEnv.Lookup("InstrSkill")->GetRoundValue());
            bonusTime = mathEnv.Lookup("TimeLeft")->GetRoundValue() * 1000; // in milliseconds

            charData->CalculateAddExperience(instrSkill, practicePoints, modifier);
        }
    }

    // resetting song's information
    charData->EndSong(bonusTime);
    scoreRanks.DeleteAll(actorEID);
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

