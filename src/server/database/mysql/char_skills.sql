# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'character_skills'
#

DROP TABLE IF EXISTS `character_skills`;
CREATE TABLE character_skills (
  character_id int(10) unsigned NOT NULL DEFAULT '0' ,      # Key into characters table
  skill_id int(10) unsigned NOT NULL DEFAULT '0' ,          # Key into skills table
  skill_Z int(10) unsigned NOT NULL DEFAULT '0' ,           # Practice Level Of skill
  skill_Y int(10) unsigned NOT NULL DEFAULT '0' ,           # Knowledge Level of skills
  skill_Rank int(10) unsigned NOT NULL DEFAULT '0' ,        # Current Skill Rank
  PRIMARY KEY (character_id,skill_id)
);


#
# Dumping data for table 'character_skills'
#
INSERT INTO character_skills VALUES("3","0","4","4","3");
INSERT INTO character_skills VALUES("2","0","0","0","2");
INSERT INTO character_skills VALUES("2","11","0","0","3");
INSERT INTO character_skills VALUES("2","37","0","0","2");
INSERT INTO character_skills VALUES("2","15","0","0","60");
INSERT INTO character_skills VALUES("2","38","0","0","5");
INSERT INTO character_skills VALUES("2","39","0","0","5");
INSERT INTO character_skills VALUES("2","40","0","0","20");
INSERT INTO character_skills VALUES("19","51","0","0","60");
INSERT INTO character_skills VALUES("19","50","0","0","60");
INSERT INTO character_skills VALUES("19","49","0","0","60");
INSERT INTO character_skills VALUES("19","48","0","0","60");
INSERT INTO character_skills VALUES("19","47","0","0","60");
INSERT INTO character_skills VALUES("19","46","0","0","60");

# Fighter skills on axe
INSERT INTO character_skills VALUES("6","2","0","0","2");
INSERT INTO character_skills VALUES("7","2","0","0","2");

# Tribe skills on mining
INSERT INTO character_skills VALUES("20","37","200","200","200");

