#
# Table structure for mini game boards
#

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
  PRIMARY KEY (`name`)
);

#
# data for gameboards
#
# Notes for gameboards.gameRules
# <GameRules>
#   <Rules PlayerTurns = 'Strict'/'Relaxed'*
#          MoveType = 'PlaceOrMovePiece'*/'MoveOnly'/'PlaceOnly'
#          MoveablePieces = 'Own'/'Any'*
#          MoveTo = 'Vacancy'/'Anywhere'* />
# </GameRules>
# * Default
# All Rules are optional.
#
INSERT INTO `gameboards` VALUES ('Test Game', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 2, 'White,Checked', '');
INSERT INTO `gameboards` VALUES ('Test Game 2', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 1, 'White,Checked', '');
INSERT INTO `gameboards` VALUES ('Tic Tac Toe', 3, 3, '000000000', '12', 2, 'White,Plain', '<GameRules><Rules PlayerTurns=\"Ordered\" MoveType=\"PlaceOnly\" MoveTo=\"Vacancy\" /></GameRules>');
