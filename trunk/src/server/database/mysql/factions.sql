# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'factions'
#

CREATE TABLE factions (
  id int(8) unsigned NOT NULL auto_increment,
  faction_name varchar(40) NOT NULL DEFAULT '' ,
  `faction_description` text,
  faction_character BLOB NOT NULL,
  faction_weight float(6,3) NOT NULL DEFAULT '1.000' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'factions'
#

INSERT INTO factions VALUES("1","orcs","faction orcs will garantie you are hated by every one except orcs","","1.000");
INSERT INTO factions VALUES("2","merchants","faction merchants will offer you a nice deal from time to time","","1.000");
INSERT INTO factions VALUES("3","liberals","faction liberals will make you popular by all those who value freedom and such stuff","","2.000");
INSERT INTO factions VALUES("4","conservatives","faction conservatives are for those who value tradition and steady life style","","2.000");
