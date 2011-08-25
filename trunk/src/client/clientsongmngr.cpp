/*
 * clientsongmngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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
#include "clientsongmngr.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iengine/mesh.h>
#include <iengine/movable.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <util/music.h>
#include <gui/pawsmusicwindow.h>
#include <paws/pawsmanager.h>
#include <net/clientmsghandler.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "globals.h"
#include "pscelclient.h"


ClientSongManager::ClientSongManager()
{
    mainSongID = NO_SONG;

    // Subscribing
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_PLAY_SONG);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_STOP_SONG);
}

ClientSongManager::~ClientSongManager()
{
    // Unsubscribing
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_PLAY_SONG);
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_STOP_SONG);
}

void ClientSongManager::PlayMainPlayerSong(uint32_t itemID, const csString &musicalSheet)
{
    if(mainSongID != NO_SONG)
    {
        return;
    }

    sheet = musicalSheet;
    mainSongID = PENDING;

    psMusicalSheetMessage musicalSheetMessage(0, itemID, true, true, "", sheet);
    musicalSheetMessage.SendMessage();
}

void ClientSongManager::StopMainPlayerSong(bool isEnded)
{
    if(mainSongID == NO_SONG)
    {
        return;
    }

    // sending message
    psStopSongMessage stopMessage(0, 0, false, isEnded);
    stopMessage.SendMessage();

    // stopping song
    StopSong(mainSongID);
    mainSongID = NO_SONG;
    sheet.Empty();

    // updating listeners
    TriggerListeners();
}

void ClientSongManager::Update()
{
    iSoundManager* sndMngr = psengine->GetSoundManager();

    // checking main player song
    if(mainSongID != NO_SONG && mainSongID != PENDING)
    {
        if(!sndMngr->IsSoundValid(mainSongID))
        {
            StopMainPlayerSong(true);
        }
    }

    // handling ended songs
    uint songID;
    csHash<uint, uint32>::GlobalIterator songIter(songMap.GetIterator());
    while(songIter.HasNext())
    {
        songID = songIter.Next();

        if(!sndMngr->IsSoundValid(songID))
        {
            StopSong(songID);
        }
    }
}

void ClientSongManager::Subscribe(iSongManagerListener* listener)
{
    listeners.PushSmart(listener);
}

void ClientSongManager::Unsubscribe(iSongManagerListener* listener)
{
    listeners.Delete(listener);
}

void ClientSongManager::HandleMessage(MsgEntry* message)
{
    uint8_t msgType = message->GetType();

    // Playing
    if(msgType == MSGTYPE_PLAY_SONG)
    {
        uint songHandleID;
        csVector3 playerPos;

        psPlaySongMessage playMsg(message);

        // getting player's position
        playerPos = psengine->GetCelClient()->FindObject(playMsg.songID)->GetMesh()->GetMovable()->GetFullPosition();

        // playing
        if(playMsg.toPlayer)
        {
            songHandleID = PlaySong(sheet, playMsg.instrName, playMsg.errorRate, playerPos);
        }
        else
        {
            // decompressing score
            csString uncompressedScore;
            Music::ZDecompressSong(playMsg.musicalScore, uncompressedScore);

            songHandleID = PlaySong(uncompressedScore, playMsg.instrName, playMsg.errorRate, playerPos);
        }

        if(songHandleID == 0)   // instrument not defined or illegal score
        {
            // stopping song, informing server and player
            if(playMsg.toPlayer)
            {
                // sending message
                psStopSongMessage stopMessage(0, 0, false, false);
                stopMessage.SendMessage();

                // updating listeners
                TriggerListeners();

                psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("You cannot play this song!"));
                msg.FireEvent();
            }

            return;
        }

        // saving song ID
        if(playMsg.toPlayer)
        {
            mainSongID = songHandleID;
        }
        else
        {
            songMap.Put(playMsg.songID, songHandleID);
        }
    }

    // Stopping
    else if(msgType == MSGTYPE_STOP_SONG)
    {
        psStopSongMessage stopMsg(message);

        if(songMap.Contains(stopMsg.songID))
        {
            StopSong(songMap.Get(stopMsg.songID, 0));
            songMap.DeleteAll(stopMsg.songID);
        }
        else if(mainSongID == PENDING && stopMsg.toPlayer) // no instrument equipped
        {
            psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("You do not have an equipped musical instrument to play."));
            msg.FireEvent();

            mainSongID = NO_SONG;

            // updating listeners
            TriggerListeners();
        }
    }
}

uint ClientSongManager::PlaySong(const char* musicalSheet, const char* instrName, float errorRate, csVector3 playerPos)
{
    csRef<iDocument> sheetDoc;
    csRef<iDocumentSystem> docSys;
    iSoundManager* sndMngr = psengine->GetSoundManager();
    iSoundControl* sndCtrl = sndMngr->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL);

    // creating document
    docSys = csQueryRegistry<iDocumentSystem>(psengine->GetObjectRegistry());
    sheetDoc = docSys->CreateDocument();
    sheetDoc->Parse(musicalSheet, true);

    return sndMngr->PlaySong(sheetDoc, instrName, errorRate, sndCtrl, playerPos, 0);
}

void ClientSongManager::StopSong(uint songID)
{
    psengine->GetSoundManager()->StopSound(songID);
}

void ClientSongManager::TriggerListeners()
{
    for(size_t i = 0; i < listeners.GetSize(); i++)
    {
        listeners[i]->OnMainPlayerSongStop();
    }
}
