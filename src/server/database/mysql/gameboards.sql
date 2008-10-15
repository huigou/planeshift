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
  PRIMARY KEY (`name`)
);

#
# data for gameboards
#

INSERT INTO `gameboards` VALUES ('Test Game', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 2);
INSERT INTO `gameboards` VALUES ('Test Game 2', 6, 6, 'FF00FFF0000F000000000000F0000FFF00FF', '123456789ABCDE', 1);
INSERT INTO `gameboards` VALUES ('Tic Tac Toe', 3, 3, '000000000', '12', 2);
