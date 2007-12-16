/*
* minigamemanager.h - Author: Enar Vaikene
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __MINIGAMEMANAGER_H
#define __MINIGAMEMANAGER_H

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"           // Message definitions

//=============================================================================
// Local Includes
//=============================================================================
#include "deleteobjcallback.h"
#include "msgmanager.h"             // Parent class

class Client;
class psMiniGameManager;
class gemActionLocation;
class gemObject;

/**
 * Wrapper class for the game board layout buffer.
 */
class psMiniGameBoard
{
public:

    psMiniGameBoard();

    ~psMiniGameBoard();

    /// Sets up the game board layout.
    void Setup(int8_t newCols, int8_t newRows, uint8_t *newLayout, uint8_t newNUmPieces, uint8_t *newPieces);

    /// Returns the number of columns.
    int8_t GetCols() const { return cols; }
    
    /// Returns the number of rows.
    int8_t GetRows() const { return rows; }

    /// Returns the packed game board layout.
    uint8_t *GetLayout() const { return layout; };

    /// Returns the number of available pieces.
    uint8_t GetNumPieces() const { return numPieces; }

    /// Returns the package list of available pieces.
    uint8_t *GetPieces() const { return pieces; }

    /// Gets the tile state from the specified column and row.
    uint8_t Get(int8_t col, int8_t row) const;

    /// Sets the tile state at the specified column and row.
    void Set(int8_t col, int8_t row, uint8_t state);

protected:

    /// The current game board layout with tiles and pieces.
    uint8_t *layout;
        
    /// The number of columns.
    int8_t cols;

    /// The number of rows.
    int8_t rows;

    /// The number of available pieces.
    uint8_t numPieces;

    /// The package list of available gane pieces.
    uint8_t *pieces;

};

/**
 * Implements one minigame session.
 *
 * Game sessions are bound to a game board (action location) and identified
 * by a unique name. The name of the game board is defined in the action_locations table (name field).
 *
 * The game itself is defined in the response string. The string is expected to have the
 * following format:
 * <Examine>
 *  <GameBoard Cols='number of columns' Rows='number of rows' Layout='board layout' Pieces='list of pieces' />
 *  <Description>Description as seen by players</Description>
 * </Examine>
 *
 * The Layout attribute defines the layout of the game board and optionally also preset game
 * pieces on it.
 *
 * The Pieces attribute defines the available game pieces. Clients can use only the pieces that are
 * defined in the Pieces attribute.
 *
 * In the future we will add more attributes like for game options and name of a plugin or
 * script for managed games.

 * Every session can have 0, 1, 2 or more players attached to it. The first player
 * gets white game pieces and the second player black game pieces. All other players
 * can just watch the game.
 */
class psMiniGameSession : public iDeleteObjectCallback
{
public:

    psMiniGameSession(psMiniGameManager *mng, gemActionLocation *obj, const char *name);

    ~psMiniGameSession();

    /// Returns the game session ID
    const uint32_t GetID() const { return id; }

    /// Returns the session name.
    const csString& GetName() const { return name; }

    /// Returns the game options.
    uint16_t GetOptions() const { return options; }

    /**
     * Loads the game.
     * @param[in] responseString The string with minigame options and board layout.
     * @return Returns true if the game was loaded and false if not.
     *
     * The Load() function loads the game from the given action location response string.
     */
    bool Load(csString &responseString);

    /// Restarts the game.
    void Restart();

    /// Adds a player to the session.
    void AddPlayer(Client *client);

    /// Removes a player from the session.
    void RemovePlayer(Client *client);

    /// Returns true if it is valid for this player to update the game board.
    bool IsValidToUpdate(Client *client) const;

    /// Updates the game board. NB! Call IsValidToUpdate() first to verify that updating is valid.
    void Update(Client *client, psMGUpdateMessage &msg);

    /**
     * Sends the current game board to the given player.
     * @param[in] clientID The ID of the client.
     * @param[in] modOptions Modifier for game options.
     */
    void Send(uint32_t clientID, uint32_t modOptions);

    /// Broadcast the current game board layout to all the players/watchers.
    void Broadcast();
    
    /// Handles disconnected players.
    virtual void DeleteObjectCallback(iDeleteNotificationObject * object);

    /// Checks for idle players and players too far away
    void Idle();

protected:

    /// Game manager.
    psMiniGameManager *manager;

    /// The game session ID (equals to the action location ID)
    uint32_t id;

    /// The game session name.
    csString name;

    /// Action location object
    csWeakRef<gemObject> actionObject;

    /// Game options
    uint16_t options;

    /// The player with white game pieces.
    uint32_t whitePlayerID;

    /// Idle counter for the player with white pieces.
    int whiteIdleCounter;

    /// The player with black game pieces.
    uint32_t blackPlayerID;

    /// Idle counter for the player with black pieces.
    int blackIdleCounter;

    /// Watchers.
    csArray<uint32_t> watchers;

    /// Current message counter for versionin.
    uint8_t currentCounter;

    /// The current game board.
    psMiniGameBoard gameBoard;
};


/**
 * This manager handles minigame sessions.
 */
class psMiniGameManager : public MessageManager
{

public:

    psMiniGameManager();

    ~psMiniGameManager();

    virtual void HandleMessage(MsgEntry *me, Client *client);
    
    psMiniGameSession *GetSessionByID(uint32_t id);

    /// Idle function to check for idle players and players too far away.
    void Idle();

protected:

    void HandleStartGameRequest(Client *client);

    void HandleStopGameRequest(Client *client);

    void HandleGameUpdate(Client *client, psMGUpdateMessage &msg);

    /// Game sessions.
    csPDelArray<psMiniGameSession> sessions;

    /// Maps players to game sessions for quicker access.
    csHash<psMiniGameSession *, uint32_t> playerSessions;
};


#endif
