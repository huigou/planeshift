# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'spells'
#

CREATE TABLE spells (
  id int(8) unsigned NOT NULL auto_increment,
  name varchar(30) DEFAULT '0' ,
  way_id int(8) unsigned DEFAULT '0' ,
  realm tinyint(3) unsigned DEFAULT '0' ,
  caster_effect varchar(255) DEFAULT '0' ,
  target_effect varchar(255) DEFAULT '0' ,
  image_name varchar(100) DEFAULT '0' ,
  spell_description text ,
  offensive tinyint(3) DEFAULT '0' ,
  progression_event varchar(255) ,
  saved_progression_event varchar(255) ,
  saving_throw varchar(32) DEFAULT '0' ,
  saving_throw_value int(4) DEFAULT '-1' ,
  max_power int(4) DEFAULT '1' ,
  target_type int(4) DEFAULT '32' ,
  cstr_npc_spell_category int(10) DEFAULT '0' ,
  npc_spell_power float(10,3) DEFAULT '0.000' ,
  PRIMARY KEY (id),
  UNIQUE name (name)
);


#
# Dumping data for table 'spells'
#

INSERT INTO spells VALUES("1","Summon Missile","1","1","casting","test_","","A wooden arrow is summoned and thrown at the target dealing 6*P damages.","1","cast Summon Missile","","0","-1","2","288","163","1.000");
INSERT INTO spells VALUES("2","Life Infusion","1","1","casting","clear","","By means of this spell the wizard is able to instill pure energy in a creature. The energy, which has healing effects, is less powerful but similar to the energy of the Great Crystal. It can be cast on the wizard or on another character in touch range.","0","cast Life Infusion","","0","-1","5","24","164","1.000");
INSERT INTO spells VALUES("3","Gust Of Wind","1","1","casting","puff","","AoE Example. A nasty smelling breeze passes by you.","0","cast Gust of Wind","","0","-1","20","32","165","1.500");
INSERT INTO spells VALUES("4","Defensive Wind","1","1","casting","puff","","Buff Example. A calming effect washes over you.","0","cast Defensive Wind","","0","-1","50","24","166","1.500");
INSERT INTO spells VALUES("5","Summon Creature","5","1","casting","clear","","The caster summons a creature as it's familiar.","0","cast Summon Creature","","0","-1","50","24","166","1.500");
INSERT INTO spells VALUES("6","Fire Warts","3","1","casting","fire","","DoT Example. This spell will cause burning sores appear all over your enemies body, searing their flesh.","0","cast Fire Warts","","0","-1","20","288","165","1.500");
INSERT INTO spells VALUES("7","Gem of Clarity","1","1","casting","clear","","Block Example. The caster will feel very clear minded.","0","GemOfClarity","","0","-1","20","8","165","1.500");
INSERT INTO spells VALUES("8","Recharge","1","1","casting","clear","","Test the recharge operation.","0","Recharge","","0","-1","20","8","165","1.500");
