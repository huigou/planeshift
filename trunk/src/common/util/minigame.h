/*
* minigame.h - Author: Enar Vaikene
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


#ifndef __MINIGAME_H
#define __MINIGAME_H

/// Globals for minigames
namespace psMiniGame
{
/**
 * Game board limits
 */
#define GAMEBOARD_MIN_COLS 1
#define GAMEBOARD_MAX_COLS 16
#define GAMEBOARD_MIN_ROWS 1
#define GAMEBOARD_MAX_ROWS 16

    /// Minigame options
    enum GameOptions
    {
        ManagedGame         = 0x01, ///< A game managed by the server.
        BlackPieces         = 0x02, ///< Player with black pieces.
        ReadOnly            = 0x04, ///< The game is read-only (for watchers).
        PersonalGame        = 0x08, ///< The game is personal & private
        BlackSquare         = 0x10, ///< Top left/all squares Black. Else white.
        PlainSquares        = 0x20, ///< Board squares all plain. Else checked.
        DisallowedMove      = 0x40, ///< Last move disallowed
        ObserveEndGame      = 0x80  ///< observe endgame play
    };

    /// Minigame tile state values
    enum TileStates
    {
        EmptyTile          = 0,     ///< An empty game tile.

        White1             = 1,     ///< White regular game piece
        White2,
        White3,
        White4,
        White5,
        White6,
        White7,

        Black1              = 8,    ///< Black regular game piece.
        Black2,
        Black3,
        Black4,
        Black5,
        Black6,
        Black7,

        DisabledTile        = 15    ///< Disable game tile.
    };

};

#endif
