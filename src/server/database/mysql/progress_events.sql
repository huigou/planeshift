# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'progression_events'
#

CREATE TABLE progression_events (
  name varchar(40) NOT NULL DEFAULT '' ,
  event_script text NOT NULL,
  PRIMARY KEY (name)
);


#
# Dumping data for table 'progression_events'
#

INSERT INTO progression_events VALUES("DamageHP","<evt><hp aim=\"target\" value=\"-Param0\" save=\"Result\" /><msg aim=\"target\" text=\"You lost $Param0 hitpoints.\"/><msg aim=\"actor\" text=\"You hit $Target for $Param0 hitpoints.\"/></evt>");
INSERT INTO progression_events VALUES("GainHP","<evt><hp aim=\"target\" value=\"Param0\" /><msg aim=\"actor\" text=\"You gained $Param0 hitpoints\"/><msg text=\"Gained $Param0 hitpoints\"/></evt>");
INSERT INTO progression_events VALUES("give_exp","<evt><exp value=\"Param0\" type=\"allocate_last\" /></evt>");
INSERT INTO progression_events VALUES("kill","<evt><exp value=\"Target:KillExp\" /></evt>");
INSERT INTO progression_events VALUES("killed orc","<evt><faction name=\"orcs\" value=\"1\" /></evt>");
INSERT INTO progression_events VALUES("killed conservative","<evt><faction name=\"liberals\" value=\"2\" /></evt>");
INSERT INTO progression_events VALUES("cast Life Infusion","<evt><hp adjust=\"add\" aim=\"target\" value=\"10*PowerLevel\"/> <msg aim=\"actor\" text=\"You transfer healing energy by your touch.\"/> <msg aim=\"target\" text=\"Your wounds were healed a bit.\"/> </evt>");
INSERT INTO progression_events VALUES("cast Summon Missile","<evt><hp adjust=\"add\" aim=\"target\" value=\"-6*PowerLevel\"/> <msg aim=\"actor\" text=\"The missile hits your enemy.\"/> <msg aim=\"target\" text=\"You are hit with a missile!\"/> </evt>");
INSERT INTO progression_events VALUES("cast Summon Missile Save","<evt><msg text=\"Saved against Summon Missile\" /></evt>");
INSERT INTO progression_events VALUES("cast Defensive Wind","<evt><block category=\"+DefensiveWind\" delay=\"30000*PowerLevel\" /><defense adjust=\"mul\" aim=\"actor\" value=\"1+0.05*PowerLevel\" delay=\"30000*PowerLevel\" undomsg=\"The defensive wind stops blowing.\"/> <msg aim=\"actor\" text=\"You summon wind around your body that defends you from attacks.\"/> </evt>");
INSERT INTO progression_events VALUES("cast Gust of Wind","<evt><hp aim=\"target\" value=\"-10.0\" /><msg aim=\"actor\" text=\"You hit $Target with a strong gust of wind.\"/></evt>");
INSERT INTO progression_events VALUES("cast Fire Warts","<evt><block category=\"-FireWarts\" /><hpRate aim=\"target\" value=\"-1.0\" /><hpRate aim=\"actor\" value=\"0.5\" /></evt>");
INSERT INTO progression_events VALUES("effect_energy_arrow","<evt><hp aim=\"target\" value=\"-5\" save=\"spelldamage\"/><msg text=\"You deal $spelldamage damage.\"/></evt>");
INSERT INTO progression_events VALUES("caster_flame_shot","<evt><msg text=\"You attempt to cast Flame Shot.\" /></evt>");
INSERT INTO progression_events VALUES("effect_flame_shot","<evt><hp aim=\"target\" value=\"-15+rnd(5)\"  save=\"spelldamage\"/><msg text=\"Your spell did $spelldamage damage.\"/></evt>");
INSERT INTO progression_events VALUES("caster_flame_spiral","<evt><msg text=\"You attempt to cast Flame Spiral.\" /></evt>");
INSERT INTO progression_events VALUES("effect_flame_spiral","<evt><hp aim=\"target\" value=\"rnd(25)\" /><msg text=\"You do a random amount of damage.\"/></evt>");
INSERT INTO progression_events VALUES("caster_flame_fountain","<evt><msg text=\"You deal 25-35 HP damage\" /><script delay=\"3000\"><hp value=\"5\"/><msg text=\"You feel fresh\"/></script></evt>");
INSERT INTO progression_events VALUES("effect_flame_fountain","<evt><hp aim=\"target\" value=\"-25-rnd(10)\" /><script delay=\"3000\"><hp value=\"-5\"/></script></evt>");

# Char creation events - god
INSERT INTO progression_events VALUES ('charcreate_2','<evt><skill name="crystal way" buff="no" attribute="adjust" value="2"/><int attribute="adjust" base="yes" value="3"/><animalaffinity attribute="adjust" name="daylight" value="2"/></evt>');

# Char creation events - life
INSERT INTO progression_events VALUES ('charcreate_3','<evt><str attribute="adjust" base="yes" value="3"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_6','<evt><wil attribute="adjust" base="yes" value="4"/><cha attribute="adjust" base="yes" value="-4"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_7','<evt><wil attribute="adjust" base="yes" value="-1"/><cha attribute="adjust" base="yes" value="1"/></evt>');

# Char creation events - parents
INSERT INTO progression_events VALUES ('charcreate_9','<evt><agi attribute="adjust" base="yes" value="4*ParentStatus"/><skill name="alchemy" buff="no" attribute="adjust" value="4*ParentStatus"/><animalaffinity attribute="adjust" name="acid" value="8"/></evt>');

# Char creation events - zodiacs
INSERT INTO progression_events VALUES ('charcreate_215','<evt><wil attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_216','<evt><agi attribute="adjust" base="yes" value="20"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_217','<evt><str attribute="adjust" base="yes" value="-12"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_218','<evt><end attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_219','<evt><int attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_220','<evt><cha attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_221','<evt><str attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_222','<evt><end attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_223','<evt><int attribute="adjust" base="yes" value="10"/></evt>');
INSERT INTO progression_events VALUES ('charcreate_224','<evt><cha attribute="adjust" base="yes" value="10"/></evt>');



INSERT INTO progression_events VALUES("drink_potion","<evt><hp attribute=\"adjust\" base=\"no\" value=\"10\" /><msg text=\"You drink an healing potion and feel better.\"/></evt>");
INSERT INTO progression_events VALUES("drink_poison","<evt><hp attribute=\"adjust\" base=\"no\" value=\"-20\" /><msg text=\"You drink some poison and feel bad.\"/></evt>");
INSERT INTO progression_events VALUES("BuffPotionWILL","<evt><block category=\"+PotionWILL\" delay=\"120000\" /><wil value=\"5\" delay=\"120000\" aim=\"actor\" attribute=\"adjust\" base=\"no\" undomsg=\"The potion wears off.\" /></evt>");
INSERT INTO progression_events VALUES("equip_mind","<evt><msg text=\"You now have something in mind.\"/></evt>");
INSERT INTO progression_events VALUES("de_equip_mind","<evt><msg text=\"Your mind is now clear.\"/></evt>");
INSERT INTO progression_events VALUES("equip_ring_hp","<evt><hp attribute=\"adjust\" base=\"yes\" value=\"10\" /><msg text=\"You hear the dark lord calling for you.\"/></evt>");
INSERT INTO progression_events VALUES("equip_ring_mana","<evt><mana attribute=\"adjust\" base=\"yes\" value=\"10\" /><msg text=\"You feel magic flow through you.\"/></evt>");
INSERT INTO progression_events VALUES("equip_ring_strength","<evt><str attribute=\"adjust\" base=\"no\" value=\"10\" /><msg text=\"You feel stronger.\"/></evt>");
INSERT INTO progression_events VALUES("equip_ring_agility","<evt><agi attribute=\"adjust\" base=\"no\" value=\"10\" /><msg text=\"You feel faster.\"/></evt>");

INSERT INTO progression_events VALUES("equip_ring_faction","<evt><faction name=\"orcs\" value=\"100\"/></evt>");
INSERT INTO progression_events VALUES("equip_sword_buffed","<evt><str attribute=\"adjust\" base=\"no\" value=\"30\" /><skill name=\"Sword\" value=\"3\" /></evt>");
INSERT INTO progression_events VALUES("equip_sword_negativity","<evt><str attribute=\"adjust\" base=\"no\" value=\"-50\" /><skill name=\"Sword\" value=\"-5\" /></evt>");
INSERT INTO progression_events VALUES("drink_speed_potion","<evt><move duration=\"10\" type=\"mod\" x=\"2.0\" y=\"2.0\" z=\"2.0\" /><msg text=\"You drink an speed potion and feel very fast.\"/></evt>");
INSERT INTO progression_events VALUES("drink_strange_potion","<evt><move duration=\"10\" type=\"mod\" x=\"-0.8\" z=\"-1.1\" yrot=\"5.0\" /><move type=\"push\" y=\"6.0\" yrot=\"10.0\" /><msg text=\"You feel very dizzy\"/></evt>");
INSERT INTO progression_events VALUES('failed_fly', '<evt><hp attribute="adjust" base="no" value="-45" /><msg text="You failed horribly to fly and injured yourself badly"/></evt>');
INSERT INTO progression_events VALUES('create_familiar','<evt><charge charges=\"1\"><createfamiliar /><msg text="Your new familiar appears nearby."/></charge></evt>');
INSERT INTO progression_events VALUES('kick_you','<evt><hp aim=\"actor\" value=\"20\" save=\"Result\" /><msg aim=\"target\" text=\"You lost $Param0 hitpoints\"/><msg aim=\"actor\" text=\"Target kicked for $Param0 hitpoints\"/></evt>');
INSERT INTO progression_events VALUES('fire_damage','<evt><hp aim=\"actor\" value=\"-6\" /><msg text=\"You touch the ingots and your hand is burned!\"/></evt>');
INSERT INTO progression_events VALUES('healing_tree','<evt><hp aim=\"actor\" value=\"6\" /><msg text=\"Walking near the tree you feel refreshed!\"/></evt>');
INSERT INTO progression_events VALUES('morph_ulbernaut','<evt><morph mesh=\"ulbernaut\" duration=\"60\" /><msg text=\"You turn into a lumbering giant!\"/></evt>');
INSERT INTO progression_events VALUES('stronglegs','<evt><set attrib=\"nofalldamage\" duration=\"120\" /><set attrib=\"nevertired\" duration=\"120\" /><msg text=\"Your legs feel more powerful!\"/></evt>');
INSERT INTO progression_events VALUES('rain','<evt><weather type=\"rain\" sector=\"this\" duration=\"60\" fade=\"10\" density=\"8000\" /></evt>');
INSERT INTO progression_events VALUES("explosion","<evt><msg aim=\"actor\" text=\"The bottle explodes and the blast hits everyone in within 10m for 1000HP damage each.\"/><area type=\"actor\" range=\"10\"><hp aim=\"target\" value=\"-1000\" /><msg aim=\"target\" text=\"You lost 1000 hitpoints!\"/></area></evt>");
INSERT INTO progression_events VALUES('GemOfClarity','<evt><block category=\"+Crystal Magic Boost\" delay=\"Duration\" /> <skill name=\"Crystal Way\" aim=\"target\" value=\"200\" delay=\"Duration\" /><msg aim=\"target\" text=\"You feel clear headed.\"/></evt>');
INSERT INTO progression_events VALUES('ResearchSpellSuccess','<evt><msg text=\"You have discovered a new spell!\" /></evt>');
INSERT INTO progression_events VALUES('ResearchSpellFailure','<evt><msg text=\"You fail to discover a new spell.\" /></evt>');
INSERT INTO progression_events VALUES('rescue_check','<evt><quest funct=\"complete\" aim=\"actor\" prerequisite=\"Rescue the Princess\" /><msg aim=\"actor\" text=\"You are trying to access restricted area.\"/></evt>');
INSERT INTO progression_events VALUES('get_next_entrance','<evt><action funct=\"activate\" sector=\"room\" stat=\"Small Key\" /></evt>');
INSERT INTO progression_events VALUES("flame_damage","<evt><hp aim=\"actor\" value=\"-5\" /><msg aim=\"actor\" text=\"You lost 5 hitpoints.\"/></evt>");
INSERT INTO progression_events VALUES('teleport_spawn','<evt><teleport funct=\"spawn\" /><msg aim=\"actor\" text=\"Good Luck!\"/></evt>');
INSERT INTO progression_events VALUES('drop_marker','<evt><effect funct=\"attached\" effect=\"shadow\"  /><msg aim=\"actor\" text=\"Spot Marked!\"/></evt>');
INSERT INTO progression_events VALUES('death_penalty','<evt><block category="-Death Realm Curse" delay="100000"/><agi adjust="pct" aim="actor" base="no" value="-50" delay="100000"/><end adjust="pct" aim="actor" base="no" value="-50" delay="100000"/><str adjust="pct" aim="actor" base="no" value="-50" delay="100000"/><cha adjust="pct" aim="actor" base="no" value="-50" delay="100000"/><int adjust="pct" aim="actor" base="no" value="-50" delay="100000"/><wil adjust="pct" aim="actor" base="no" value="-50" delay="100000" undomsg="You feel the curse of death lift."/><msg aim="actor" text="You feel the curse of the death realm upon you."/></evt>');
INSERT INTO progression_events VALUES('Recharge','<evt><recharge charges="1" error="Nothing to recharge" success="Item where recharged" /></evt>');
INSERT INTO progression_events VALUES("CraftTest","<evt><craft pattern=\"wayoutbread\" /></evt>");
INSERT INTO progression_events VALUES("CraftTest2","<evt><craft pattern=\"flame weapon\" /></evt>");

INSERT INTO progression_events VALUES('PATH_Street Warrior','<evt><str adjust=\"add\" value=\"0.35*CharPoints\" base=\"yes\" /><end adjust=\"add\" value=\"0.25*CharPoints\" base=\"yes\" /><agi adjust=\"add\" value=\"0.25*CharPoints\" base=\"yes\" /><int adjust=\"add\" value=\"0.05*CharPoints\" base=\"yes\" /><wil adjust=\"add\"  value=\"0.05*CharPoints\" base=\"yes\" /><cha adjust=\"add\" value=\"0.05*CharPoints\" base=\"yes\" /><skill name=\"Mace & Hammer\" adjust=\"set\" value=\"1\" buff=\"no\" /><skill name=\"Medium Armor\" adjust=\"set\" value=\"1\" buff=\"no\" /><skill name=\"Pickpockets\" adjust=\"set\" value=\"1\" buff=\"no\" /><skill name=\"Find Traps\" adjust=\"set\" value=\"1\" buff=\"no\" /><skill name=\"Body Development\" adjust=\"set\" value=\"2\" buff=\"no\" /></evt>');

INSERT INTO progression_events VALUES('minigame_win', '<evt><item aim=\"actor\" name=\"trias\" location=\"wallet\" count=\"50\" /><exp type=\"allocate_last\" value=\"100\" /><msg aim=\"actor\" text=\"You won 50 tria\" /></evt>');
