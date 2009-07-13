# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'character_advantages'
#

DROP TABLE IF EXISTS `character_advantages`;
CREATE TABLE character_advantages (
  character_id int(10) unsigned NOT NULL DEFAULT '0' ,
  advantage_id int(10) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (character_id,advantage_id)
);


#
# Dumping data for table 'character_advantages'
#

