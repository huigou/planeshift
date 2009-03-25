/*
 * Table structure for table 'spells'
 */

CREATE TABLE spells (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL,
  `way_id` int(8) unsigned DEFAULT '0',
  `realm` tinyint(3) unsigned DEFAULT '0',
  `casting_effect` varchar(255),
  `image_name` varchar(100),
  `spell_description` text,
  `offensive` boolean DEFAULT true,
  `max_power` int(4) DEFAULT '1',
  `target_type` int(4) DEFAULT 0x20,
  `exclude_target` boolean DEFAULT false,
  `cast_duration` text NOT NULL, /* (power, wayskill, relatedstat) -> seconds */
  `range` text,                  /* (power, wayskill, relatedstat) -> meters  */
  `aoe_radius` text,             /* (power, wayskill, relatedstat) -> meters  */
  `aoe_angle` text,              /* (power, wayskill, relatedstat) -> radians */
  `outcome` text NOT NULL,       /* (power, caster, target) -> side effects   */
  `cstr_npc_spell_category` int(10) DEFAULT '0',
  `npc_spell_power` float(10,3) DEFAULT '0.000',
  PRIMARY KEY (id),
  UNIQUE name (name)
);


/*
 * Sample/fake spells
 */

INSERT INTO spells VALUES(1,"Summon Missile",1,1,"casting","","You summon a wooden arrow and magically shoot it at your target.",true,1,0x20,false,"5000-(WaySkill+RelatedStat)/Power","40*Power","0","0","cast Summon Missile",163,"1.000");
INSERT INTO spells VALUES(2,"Life Infusion",1,1,"casting","","By means of this spell the wizard is able to instill pure energy in a creature. The energy, which has healing effects, is less powerful but similar to the energy of the Great Crystal. It can be cast on the wizard or on another character in touch range.",false,1,0x18,false,"1000-(WaySkill+RelatedStat)/Power","3.0","0","0","cast Life Infusion","164","1.000");
INSERT INTO spells VALUES(3,"Swiftness",2,1,"casting","","This spell increases the agility of the caster for a time.",false,1,0x18,false,"1000-(WaySkill+RelatedStat)/Power","3.0","0","0","cast Swiftness","164","1.000");
INSERT INTO spells VALUES(4,"Dispel",1,1,"casting","","This spell cancels various benificial status effects on your target.",false,1,0x38,false,"2000-(WaySkill+RelatedStat)/Power","3+Power","0","0","cast Dispel",164,"1.000");
INSERT INTO spells VALUES(5,"Flame Spire",3,1,"casting","","This spells creates a spire of flames all around the caster.",false,1,0x8,false,"2000-(WaySkill+RelatedStat)/Power","3.0","0","0","cast Flame Spire",167,0.000);
INSERT INTO spells VALUES(6,"Magetank",3,1,"casting","","Everybody wants to play one...",false,1,0x18,false,"2000-(WaySkill+RelatedStat)/Power","3.0","0","0","cast Magetank",167,0.000);
INSERT INTO spells VALUES(7,"Drain",4,1,"casting","","This spell drains the life of your opponent, over time, and gives some of it back to you.",true,1,0x20,false,"2000-(WaySkill+RelatedStat)/Power","15.0","0","0","cast Drain",163,1.000);
