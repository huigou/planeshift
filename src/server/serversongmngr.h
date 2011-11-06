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

#ifndef SERVER_SONG_MANAGER_H
#define SERVER_SONG_MANAGER_H


//====================================================================================
// Local Includes
//====================================================================================
#include "msgmanager.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class psItem;
class MathScript;
class psCharacter;


/**
 * At the moment this class just handles play and stop messages for musical instruments.
 * In the future it will also take care of the instruments relating skills management.
 */
class ServerSongManager: public MessageManager<ServerSongManager>
{
public:
    ServerSongManager();
    bool Initialize();

    /**
     * Handles a play song request.
     * @param me the message from the client.
     * @param client the sender.
     */
    void HandlePlaySongMessage(MsgEntry* me, Client* client);

    /**
     * Handles a stop song request.
     * @param me the message from the client.
     * @param client the sender.
     */
    void HandleStopSongMessage(MsgEntry* me, Client* client);

    /**
     * Takes care of the skill ranking and of sending the stop song message to the player's
     * proximity list.
     *
     * @param charActor the player that has just stopped to play his instrument.
     * @note This function is called by a gemActor when it exits from the PLAY mode thus
     * StopSong does not take care of updating the player's status.
     */
    void StopSong(gemActor* charActor);

private:
    bool isEnded;                           ///< Flag used to keep track of the last stop request. Remember to reset each time.

    MathScript* calcSongPar;
    MathScript* calcSongExp;
    unsigned int instrumentsCategory;       ///< Keeps the instruments' category from server_options table.

    /**
     * Gets the instrument currently equipped by the player.
     * @param charData the player's data.
     * @return the pointer to the instrument or null if no instrument is equipped.
     */
    psItem* GetEquippedInstrument(psCharacter* charData) const;
};

#endif // SERVER_SONG_MANAGER_H

