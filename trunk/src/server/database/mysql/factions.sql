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
  faction_weight float(6,3) NOT NULL DEFAULT '1.000' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'factions'
#

INSERT INTO factions VALUES("1","orcs","1.000");
INSERT INTO factions VALUES("2","merchants","1.000");
INSERT INTO factions VALUES("3","liberals","2.000");
INSERT INTO factions VALUES("4","conservatives","2.000");
