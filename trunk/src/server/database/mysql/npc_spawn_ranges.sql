# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_spawn_ranges'
#

CREATE TABLE npc_spawn_ranges (
  id int(10) unsigned NOT NULL auto_increment,
  npc_spawn_rule_id int(10) unsigned NOT NULL DEFAULT '0' ,
  x1 float(10,2) NOT NULL DEFAULT '0.00' ,
  y1 float(10,2) NOT NULL DEFAULT '0.00' ,
  z1 float(10,2) NOT NULL DEFAULT '0.00' ,
  x2 float(10,2) NOT NULL DEFAULT '0.00' ,
  y2 float(10,2) NOT NULL DEFAULT '0.00' ,
  z2 float(10,2) NOT NULL DEFAULT '0.00' ,
  sector_id int(10) NOT NULL DEFAULT '0' ,
  range_type_code char(1) NOT NULL DEFAULT 'A' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'npc_spawn_ranges'
#

INSERT INTO npc_spawn_ranges VALUES("1","999","-20.00","0.00","-160.00","0.00","0.00","-180.00","3","A");
INSERT INTO npc_spawn_ranges VALUES("2","100","-20.00","0.00","-160.00","0.00","0.00","-180.00","3","L");
