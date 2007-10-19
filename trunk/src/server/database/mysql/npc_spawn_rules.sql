# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_spawn_rules'
#

CREATE TABLE npc_spawn_rules (
  id int(10) unsigned NOT NULL auto_increment,
  min_spawn_time int(10) unsigned NOT NULL DEFAULT '0' ,
  max_spawn_time int(10) unsigned NOT NULL DEFAULT '0' ,
  substitute_spawn_odds float(6,4) DEFAULT '0.0000' ,
  substitute_player int(10) unsigned DEFAULT '0' ,
  fixed_spawn_x float(10,2) ,
  fixed_spawn_y float(10,2) ,
  fixed_spawn_z float(10,2) ,
  fixed_spawn_rot float(6,4) DEFAULT '0.0000' ,
  fixed_spawn_sector varchar(40) DEFAULT 'room' ,
  loot_category_id int(10) unsigned NOT NULL DEFAULT '0' ,
  dead_remain_time int(10) unsigned NOT NULL DEFAULT '0' ,
  name varchar(40) ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'npc_spawn_rules'
#

INSERT INTO npc_spawn_rules VALUES("1","10000","20000","0.0000","0","0.00","0.00","0.00","0.0000","startlocation","0","30000","Respawn at Orig Location");
INSERT INTO npc_spawn_rules VALUES("100","10000","20000","0.0000","0","10.00","0.00","-110.00","0.0000","NPCroom","0","30000","Anywhere in npcroom forest-quick");
INSERT INTO npc_spawn_rules VALUES("999","25000","60000","0.0000","0","-4.50","-1.10","-4.00","4.5500","hydlaa_plaza","0","0","Test Hydlaa Plaza");
