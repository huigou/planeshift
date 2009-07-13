# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'player_spells'
#

DROP TABLE IF EXISTS `player_spells`;
CREATE TABLE player_spells (
  player_id int(10) unsigned NOT NULL DEFAULT '0' ,
  spell_id int(8) unsigned NOT NULL DEFAULT '0' ,
  spell_slot tinyint(3) unsigned DEFAULT '0' ,
  PRIMARY KEY (player_id,spell_id)
);


#
# Dumping data for table 'player_spells'
#

#INSERT INTO player_spells VALUES("2","1","0");
#INSERT INTO player_spells VALUES("2","2","1");
#INSERT INTO player_spells VALUES("1","1","0");
#INSERT INTO player_spells VALUES("1","2","1");
#INSERT INTO player_spells VALUES("1","3","2");
#INSERT INTO player_spells VALUES("1","4","3");
#INSERT INTO player_spells VALUES("1","5","4");
