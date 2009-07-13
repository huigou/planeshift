#
# Table structure for mini game boards
#

DROP TABLE IF EXISTS `gameboards`;
CREATE TABLE gameboards
(
  name VARCHAR(30) NOT NULL,
  numColumns INTEGER UNSIGNED NOT NULL,
  numRows INTEGER UNSIGNED NOT NULL,
  layout TEXT NOT NULL,
  pieces TEXT NOT NULL,
  numPlayers INTEGER DEFAULT 2 NOT NULL,
  gameboardOptions varchar(100) DEFAULT 'White,Checked' NOT NULL,
  gameRules TEXT,
  endgames TEXT,
  PRIMARY KEY (`name`)
);

#
# data for gameboards
#
# Notes for gameboards.gameRules
# <GameRules>
#   <Rules PlayerTurns = 'Ordered'/'Relaxed'*/'StrictOrdered'
#          MoveType = 'PlaceOrMovePiece'*/'MoveOnly'/'PlaceOnly'
#          MoveablePieces = 'Own'/'Any'*
#          MoveTo = 'Vacancy'/'Anywhere'* />
# </GameRules>
# * Default
# All Rules are optional.
#
# Notes for gameboards.endgames
# <MGEndGame>
#  <EndGame Coords="relative"/"absolute" [SourceTile="T"] [Winner="T"]>
#   <Coord Col="0-15" Row="0-15" Tile="T" />
#  </EndGame>
# </MGEndGame>
# where T : A = any valid piece / W = any white piece / B = any black piece
#           E = empty tile / F = follows source piece
# Each <EndGame> has 1 or more <Coord> spec.
# Each <MGEndGame> has 1 or more <EndGame>.
# Endgames can be left blank.
#
INSERT INTO `gameboards` VALUES ('Test Game', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 2, 'White,Checked', '', '');
INSERT INTO `gameboards` VALUES ('Test Game 2', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 1, 'White,Checked', '', '');
INSERT INTO `gameboards` VALUES ('Tic Tac Toe', 3, 3, '000000000', '12', 2, 'White,Plain', '<GameRules><Rules PlayerTurns=\"StrictOrdered\" MoveType=\"PlaceOnly\" MoveTo=\"Vacancy\" /></GameRules>', '<MGEndGame><EndGame Coords=\"relative\" SourceTile=\"A\"><Coord Col=\"1\" Row=\"0\" Tile=\"F\" /><Coord Col=\"2\" Row=\"0\" Tile=\"F\" /></EndGame><EndGame Coords=\"relative\" SourceTile=\"A\"><Coord Col=\"0\" Row=\"1\" Tile=\"F\" /><Coord Col=\"0\" Row=\"2\" Tile=\"F\" /></EndGame><EndGame Coords=\"relative\" SourceTile=\"A\"><Coord Col=\"1\" Row=\"1\" Tile=\"F\" /><Coord Col=\"2\" Row=\"2\" Tile=\"F\" /></EndGame><EndGame Coords=\"relative\" SourceTile=\"A\"><Coord Col=\"-1\" Row=\"1\" Tile=\"F\" /><Coord Col=\"-2\" Row=\"2\" Tile=\"F\" /></EndGame></MGEndGame>');
INSERT INTO `gameboards` VALUES ('Weather game', 3, 3, '000000000', '1', 1, 'Black,Plain', '', '<MGEndGame><EndGame Coords=\"absolute\"><Coord Col=\"0\" Row=\"0\" Tile=\"A\" /><Coord Col=\"1\" Row=\"0\" Tile=\"A\" /><Coord Col=\"2\" Row=\"0\" Tile=\"A\" /><Coord Col=\"0\" Row=\"1\" Tile=\"A\" /><Coord Col=\"0\" Row=\"2\" Tile=\"A\" /></EndGame></MGEndGame>');

