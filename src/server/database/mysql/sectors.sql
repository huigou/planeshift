# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sectors'
#

CREATE TABLE sectors (
  id smallint(3) unsigned NOT NULL auto_increment,
  name varchar(30) NOT NULL DEFAULT '' ,
  rain_enabled char(1) NOT NULL DEFAULT 'N' ,
  rain_min_gap int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_max_gap int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_min_duration int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_max_duration int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_min_drops int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_max_drops int(10) unsigned DEFAULT '0' ,
  rain_min_fade_in int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_max_fade_in int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_min_fade_out int(10) unsigned NOT NULL DEFAULT '0' ,
  rain_max_fade_out int(10) unsigned NOT NULL DEFAULT '0' ,
  lightning_min_gap int(10) unsigned DEFAULT '0' ,
  lightning_max_gap int(10) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (id),
  UNIQUE name (name)
);


#
# Dumping data for table 'sectors'
#

INSERT INTO sectors VALUES("1","room","N","0","0","0","0","0","0","0","0","0","0","0","0");
INSERT INTO sectors VALUES("2","temple","N","0","0","0","0","0","0","0","0","0","0","0","0");
INSERT INTO sectors VALUES("3","NPCroom","N","15000","15000","10000","10000","8000","8000","5000","5000","5000","5000","4000","4000");
INSERT INTO sectors VALUES("4","NPCroom1","N","15000","15000","10000","10000","8000","8000","5000","5000","5000","5000","4000","4000");
INSERT INTO sectors VALUES("5","NPCroom2","N","15000","15000","10000","10000","8000","8000","5000","5000","5000","5000","4000","4000");
INSERT INTO sectors VALUES("6","NPCroom3","N","15000","15000","10000","10000","8000","8000","5000","5000","5000","5000","4000","4000");
INSERT INTO sectors VALUES("7","NPCroomwarp","N","15000","15000","10000","10000","8000","8000","5000","5000","5000","5000","4000","4000");