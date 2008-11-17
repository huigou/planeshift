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
 * Game board limits
 */
#define GAMEBOARD_MIN_PLAYERS 1
#define GAMEBOARD_MAX_PLAYERS 2
#define GAMEBOARD_DEFAULT_PLAYERS 2

/**
 * Following enums define values to represent simple game rules
 */
enum Rule_PlayerTurn
{
    RELAXED,
    ORDERED
};
enum Rule_MovePieceType
{
    PLACE_OR_MOVE,
    PLACE_ONLY,
    MOVE_ONLY
};
enum Rule_MoveablePieces
{
    ANY_PIECE,
    OWN_PIECES_ONLY,
};
enum Rule_MovePiecesTo
{
    ANYWHERE,
    VACANCY_ONLY
};

/**
 * Game board definition class.
 */
class psMiniGameBoardDef
{
    friend class psMiniGameBoard;

public:

    psMiniGameBoardDef(const int8_t defCols, const int8_t defRows,
		       const char *defLayout, const char *defPieces,
		       const int8_t defPlayers, const int16_t options);

    ~psMiniGameBoardDef();

    /// Returns the number of columns.
    int8_t GetCols() const { return cols; }
    
    /// Returns the number of rows.
    int8_t GetRows() const { return rows; }

    /// returns layout size
    int GetLayoutSize() const { return layoutSize; };

    /// pack string layout into binary array
    void PackLayoutString(const char *layoutStr, uint8_t *packedLayout);

    /// returns gameboard layout options
    uint16_t GetGameboardOptions(void) { return gameboardOptions; };

    /// decipher simple game rules from XML
    bool DetermineGameRules(csString rulesXMLstr, csString name);

private:
    /// layout size
    int layoutSize;

    /// The initial game board layout with tiles and pieces.
    uint8_t *layout;
        
    /// The number of columns.
    int8_t cols;

    /// The number of rows.
    int8_t rows;

    /// The number of available pieces.
    uint8_t numPieces;

    /// The package list of available game pieces.
    uint8_t *pieces;

    int8_t numPlayers;

    /// gameboard options flags
    int16_t gameboardOptions;

    /// simple game rule variables
    Rule_PlayerTurn playerTurnRule;
    Rule_MovePieceType movePieceTypeRule;
    Rule_MoveablePieces moveablePiecesRule;
    Rule_MovePiecesTo movePiecesToRule;
};

/**
 * Wrapper class for the game board layout buffer.
 */
class psMiniGameBoard
{
public:

    psMiniGameBoard();

    ~psMiniGameBoard();

    /// Sets up the game board layout.
    void Setup(psMiniGameBoardDef *newGameDef, uint8_t *preparedLayout);

    /// Returns the number of columns.
    int8_t GetCols() const { return gameBoardDef->cols; }
    
    /// Returns the number of rows.
    int8_t GetRows() const { return gameBoardDef->rows; }

    /// Returns the packed game board layout.
    uint8_t *GetLayout() const { return layout; };

    /// Returns the number of available pieces.
    uint8_t GetNumPieces() const { return gameBoardDef->numPieces; }

    /// Returns the package list of available pieces.
    uint8_t *GetPieces() const { return gameBoardDef->pieces; }

    /// Gets the tile state from the specified column and row.
    uint8_t Get(int8_t col, int8_t row) const;

    /// Sets the tile state at the specified column and row.
    void Set(int8_t col, int8_t row, uint8_t state);

    /// returns number of players
    int8_t GetNumPlayers(void) { return gameBoardDef->numPlayers; };

    /// return Game rules
    Rule_PlayerTurn GetPlayerTurnRule(void) { return gameBoardDef->playerTurnRule; };
    Rule_MovePieceType GetMovePieceTypeRule(void) { return gameBoardDef->movePieceTypeRule; };
    Rule_MoveablePieces GetMoveablePiecesRule(void) { return gameBoardDef->moveablePiecesRule; };
    Rule_MovePiecesTo GetMovePiecesToRule (void) { return gameBoardDef->movePiecesToRule; };

protected:

    /// The current game board layout with tiles and pieces.
    uint8_t *layout;
        
    /// game board definition
    psMiniGameBoardDef *gameBoardDef;
};

/**
 * Implements one minigame session.
 *
 * Game sessions are bound to a game board (action location) and identified
 * by a unique name. The name of the game board is defined in the action_locations table (name field).
 *
 * The game itself is defined in the gameboards db table. 
 * The response string specifies the name to the record in gameboards
 * and an optional prepared layout & is expected to have the
 * following format:
 * <Examine>
 *  <GameBoard Name='gameboard name' [Layout='board layout'] [Session=personal|public] />
 *  <Description>Description as seen by players</Description>
 * </Examine>
 *
 * The Layout attribute defines the layout of the game board and optionally also preset game
 * pieces on it. Optional - to override the default.
 *
 * The Session attribute allows a game to be personal (restricted to one-player games)
 * whereby only the player sees the game, no other players or watchers. One such
 * minigame at one action_location can spawn a session per player who plays their own
 * private game. Set to public is for the traditional board game, and is the default if
 * this option is omitted.
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

    /// Checks if game is actually active at all
    bool GameSessionActive(void);

    void SetSessionReset(void) { toReset = true; }

    bool GetSessionReset(void) { return toReset; }

    bool IsSessionPublic(void);

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

    /// if game session marked for reset
    bool toReset;

    /// next player to move: 0=dont care, 1=player 1 (white), 2=player 2 (black), etc
    int nextPlayerToMove;

private:
    /// resend board layouts as was, e.g. correcting an illegal move
    void ResendBoardLayout(uint32_t clientnum);

    /// Game rule checking function
    bool GameMovePassesRules(uint32_t movingClient,
                             int8_t col1, int8_t row1, int8_t state1,
                             int8_t col2=-1, int8_t row2=-1, int8_t state2=-1); 
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

    /// Initialise Mini Game manager
    bool Initialise();

    /// Find & return game board definition
    psMiniGameBoardDef *FindGameDef(csString gameName);

    /// reset all sessions, prob due to reloading action locations
    void ResetAllGameSessions(void);

protected:

    void HandleStartGameRequest(Client *client);

    void HandleStopGameRequest(Client *client);

    void RemovePlayerFromSessions(psMiniGameSession *session, Client *client, uint32_t clientID);

    void HandleGameUpdate(Client *client, psMGUpdateMessage &msg);

    void ResetGameSession(psMiniGameSession *sessionToReset);

    int16_t ParseGameboardOptions(psString optionsStr);

    /// Game sessions.
    csPDelArray<psMiniGameSession> sessions;

    /// Maps players to game sessions for quicker access.
    csHash<psMiniGameSession *, uint32_t> playerSessions;

    /// Minigame board definitions by name
    csHash<psMiniGameBoardDef *, csString> gameBoardDef;
};


#endif
