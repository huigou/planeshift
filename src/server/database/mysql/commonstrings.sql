# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'common_strings'
#

DROP TABLE IF EXISTS `common_strings`;
CREATE TABLE common_strings (
  id int(8) unsigned NOT NULL auto_increment,
  string varchar(60) NOT NULL DEFAULT 'undefined' ,
  PRIMARY KEY (id),
  KEY id_2 (id),
  KEY string_2 (string),
  UNIQUE id (id),
  UNIQUE string (string)
);


#
# Dumping data for table 'common_strings'
#

INSERT INTO common_strings VALUES("1","NPCroom");
INSERT INTO common_strings VALUES("2","hydlaa_plaza");
INSERT INTO common_strings VALUES("3","laanx_inside");
INSERT INTO common_strings VALUES("4","jayose_inside");
INSERT INTO common_strings VALUES("5","tavern_de_kadel");
INSERT INTO common_strings VALUES("6","laanx_dungeon");
INSERT INTO common_strings VALUES("7","laanx_entrance");
INSERT INTO common_strings VALUES("8","laanx_main");
INSERT INTO common_strings VALUES("9","laanx_well");
INSERT INTO common_strings VALUES("10","windowless_tower_dungeon");
INSERT INTO common_strings VALUES("11","windowless_tower_exit");
INSERT INTO common_strings VALUES("12","windowless_tower_top");
INSERT INTO common_strings VALUES("13","windowless_tower");
INSERT INTO common_strings VALUES("14","room");
INSERT INTO common_strings VALUES("15","one");
INSERT INTO common_strings VALUES("16","two");
INSERT INTO common_strings VALUES("17","three");
INSERT INTO common_strings VALUES("18","four");
INSERT INTO common_strings VALUES("19","1.dds");
INSERT INTO common_strings VALUES("20","2.dds");
INSERT INTO common_strings VALUES("21","3.dds");
INSERT INTO common_strings VALUES("22","4.dds");
INSERT INTO common_strings VALUES("23","hair");
INSERT INTO common_strings VALUES("24","chin");
INSERT INTO common_strings VALUES("25","clothes");
INSERT INTO common_strings VALUES("26","eyes");
INSERT INTO common_strings VALUES("28","weapons#doubleaxe01a");
INSERT INTO common_strings VALUES("29","/planeshift/weapons/doubleaxe01a_icon.dds");
INSERT INTO common_strings VALUES("30","weapons#doubleaxe02a");
INSERT INTO common_strings VALUES("31","/planeshift/weapons/doubleaxe02a_icon.dds");
INSERT INTO common_strings VALUES("32","weapons#broadsword01a");
INSERT INTO common_strings VALUES("33","/planeshift/weapons/broadsword01a_icon.dds");
INSERT INTO common_strings VALUES("34","weapons#claymore01a");
INSERT INTO common_strings VALUES("35","/planeshift/weapons/claymore01a_icon.dds");
INSERT INTO common_strings VALUES("36","weapons#falchion01a");
INSERT INTO common_strings VALUES("37","/planeshift/weapons/falchion01a_icon.dds");
INSERT INTO common_strings VALUES("38","weapons#falchion01b");
INSERT INTO common_strings VALUES("39","/planeshift/weapons/falchion01b_icon.dds");
INSERT INTO common_strings VALUES("40","weapons#falchion01c");
INSERT INTO common_strings VALUES("41","/planeshift/weapons/falchion01c_icon.dds");
INSERT INTO common_strings VALUES("42","weapons#jug01a");
INSERT INTO common_strings VALUES("43","/planeshift/weapons/jug01a_icon.dds");
INSERT INTO common_strings VALUES("44","weapons#normalaxe01a");
INSERT INTO common_strings VALUES("45","/planeshift/weapons/normalaxe01a_icon.dds");
INSERT INTO common_strings VALUES("46","weapons#sabre01a");
INSERT INTO common_strings VALUES("47","/planeshift/weapons/sabre01a_icon.dds");
INSERT INTO common_strings VALUES("48","weapons#galkard01a");
INSERT INTO common_strings VALUES("49","/planeshift/weapons/galkard01a_icon.dds");
INSERT INTO common_strings VALUES("50","glyph_arrow_icon.dds");
INSERT INTO common_strings VALUES("51","glyph_door_icon.dds");
INSERT INTO common_strings VALUES("52","glyph_energy_icon.dds");
INSERT INTO common_strings VALUES("53","glyph_faith_icon.dds");
INSERT INTO common_strings VALUES("54","glyph_light_icon.dds");
INSERT INTO common_strings VALUES("55","glyph_sight_icon.dds");
INSERT INTO common_strings VALUES("56","glyph_air_icon.dds");
INSERT INTO common_strings VALUES("57","glyph_bond_icon.dds");
INSERT INTO common_strings VALUES("58","glyph_dome_icon.dds");
INSERT INTO common_strings VALUES("59","glyph_fly_icon.dds");
INSERT INTO common_strings VALUES("60","glyph_gas_icon.dds");
INSERT INTO common_strings VALUES("61","glyph_humanoid_icon.dds");
INSERT INTO common_strings VALUES("62","glyph_mind_icon.dds");
INSERT INTO common_strings VALUES("63","glyph_sleep_icon.dds");
INSERT INTO common_strings VALUES("64","glyph_sound_icon.dds");
INSERT INTO common_strings VALUES("65","glyph_chaos_icon.dds");
INSERT INTO common_strings VALUES("66","glyph_fire_icon.dds");
INSERT INTO common_strings VALUES("67","glyph_meteor_icon.dds");
INSERT INTO common_strings VALUES("68","glyph_might_icon.dds");
INSERT INTO common_strings VALUES("69","glyph_vortex_icon.dds");
INSERT INTO common_strings VALUES("70","glyph_weapon_icon.dds");
INSERT INTO common_strings VALUES("71","glyph_blindness_icon.dds");
INSERT INTO common_strings VALUES("72","glyph_deamon_icon.dds");
INSERT INTO common_strings VALUES("73","glyph_darkness_icon.dds");
INSERT INTO common_strings VALUES("74","glyph_death_icon.dds");
INSERT INTO common_strings VALUES("75","glyph_entropy_icon.dds");
INSERT INTO common_strings VALUES("76","glyph_illness_icon.dds");
INSERT INTO common_strings VALUES("77","glyph_negative_icon.dds");
INSERT INTO common_strings VALUES("78","glyph_weakness_icon.dds");
INSERT INTO common_strings VALUES("79","glyph_animal_icon.dds");
INSERT INTO common_strings VALUES("80","glyph_armor_icon.dds");
INSERT INTO common_strings VALUES("81","glyph_creature_icon.dds");
INSERT INTO common_strings VALUES("82","glyph_earth_icon.dds");
INSERT INTO common_strings VALUES("83","glyph_rock_icon.dds");
INSERT INTO common_strings VALUES("84","glyph_summon_icon.dds");
INSERT INTO common_strings VALUES("85","glyph_tree_icon.dds");
INSERT INTO common_strings VALUES("86","glyph_wall_icon.dds");
INSERT INTO common_strings VALUES("87","glyph_weight_icon.dds");
INSERT INTO common_strings VALUES("88","glyph_cold_icon.dds");
INSERT INTO common_strings VALUES("89","glyph_divination_icon.dds");
INSERT INTO common_strings VALUES("90","glyph_key_icon.dds");
INSERT INTO common_strings VALUES("91","glyph_object_icon.dds");
INSERT INTO common_strings VALUES("92","glyph_poison_icon.dds");
INSERT INTO common_strings VALUES("93","glyph_purify_icon.dds");
INSERT INTO common_strings VALUES("94","glyph_sphere_icon.dds");
INSERT INTO common_strings VALUES("95","glyph_water_icon.dds");
INSERT INTO common_strings VALUES("96","crystal_way#glyph_crystal_01a");
INSERT INTO common_strings VALUES("97","azure_way#glyph_azure_01a");
INSERT INTO common_strings VALUES("98","red_way#glyph_red_01a");
INSERT INTO common_strings VALUES("99","dark_way#glyph_dark_01a");
INSERT INTO common_strings VALUES("100","brown_way#glyph_brown_01a");
INSERT INTO common_strings VALUES("101","blue_way#glyph_blue_01a");
INSERT INTO common_strings VALUES("102","ylianm");
INSERT INTO common_strings VALUES("103","ynnwnm");
INSERT INTO common_strings VALUES("104","menki");
INSERT INTO common_strings VALUES("105","kran");
INSERT INTO common_strings VALUES("106","fenki");
INSERT INTO common_strings VALUES("107","fstoneb");
INSERT INTO common_strings VALUES("108","dermm");
INSERT INTO common_strings VALUES("109","attack");
INSERT INTO common_strings VALUES("110","greet");
INSERT INTO common_strings VALUES("111","items#chest");
INSERT INTO common_strings VALUES("112","/planeshift/items/chest.dds");
INSERT INTO common_strings VALUES("113","weapons#longsword01a");
INSERT INTO common_strings VALUES("114","/planeshift/weapons/longsword01a_icon.dds");
INSERT INTO common_strings VALUES("115","money#circle_01a_01");
INSERT INTO common_strings VALUES("116","/planeshift/money/circle01a_icon.dds");
INSERT INTO common_strings VALUES("117","clacker");
INSERT INTO common_strings VALUES("118","rat");
INSERT INTO common_strings VALUES("119","grendol");
INSERT INTO common_strings VALUES("120","gobble");
INSERT INTO common_strings VALUES("121","consumer");
INSERT INTO common_strings VALUES("122","trepor");
INSERT INTO common_strings VALUES("123","ulbernaut");
INSERT INTO common_strings VALUES("124","tefusang");
INSERT INTO common_strings VALUES("125","stonebm");
INSERT INTO common_strings VALUES("126","stonebf");
INSERT INTO common_strings VALUES("127","enkim");
INSERT INTO common_strings VALUES("128","enkif");
INSERT INTO common_strings VALUES("130","fynnwn");
INSERT INTO common_strings VALUES("131","potions#potion01a");
INSERT INTO common_strings VALUES("132","potions#potion01b");
INSERT INTO common_strings VALUES("133","potions#potion01c");
INSERT INTO common_strings VALUES("134","potions#smallpotion01a");
INSERT INTO common_strings VALUES("135","potions#smallpotion01b");
INSERT INTO common_strings VALUES("136","potions#smallpotion01c");
INSERT INTO common_strings VALUES("137","potions#smallpotion01d");
INSERT INTO common_strings VALUES("138","potions#potionbig01a");
INSERT INTO common_strings VALUES("139","potions#potionbig01b");
INSERT INTO common_strings VALUES("140","potions#potionbig01c");
INSERT INTO common_strings VALUES("141","money#tria_01a_01");
INSERT INTO common_strings VALUES("142","money#octa_01a_01");
INSERT INTO common_strings VALUES("143","money#hexa_01a_01");
# Duplicate
# INSERT INTO common_strings VALUES("144","money#circle_01a_01");
INSERT INTO common_strings VALUES("145","money#tria_01a_pile_01_01");
INSERT INTO common_strings VALUES("146","money#octa_01a_pile_01_01");
INSERT INTO common_strings VALUES("147","money#hexa_01a_pile_01_01");
INSERT INTO common_strings VALUES("148","money#circle01a_pile_01");
INSERT INTO common_strings VALUES("149","books#bookclosed01");
INSERT INTO common_strings VALUES("150","books#scroll01");
INSERT INTO common_strings VALUES("151","/planeshift/potions/smallpotion01a_icon.dds");
INSERT INTO common_strings VALUES("152","/planeshift/potions/smallpotion01b_icon.dds");
INSERT INTO common_strings VALUES("153","/planeshift/potions/smallpotion01c_icon.dds");
INSERT INTO common_strings VALUES("154","/planeshift/potions/smallpotion01d_icon.dds");
INSERT INTO common_strings VALUES("155","/planeshift/potions/potion01a_icon.dds");
INSERT INTO common_strings VALUES("156","/planeshift/potions/potion01b_icon.dds");
INSERT INTO common_strings VALUES("157","/planeshift/potions/potion01c_icon.dds");
INSERT INTO common_strings VALUES("158","/planeshift/potions/potionbig01a_icon.dds");
INSERT INTO common_strings VALUES("159","/planeshift/potions/potionbig01b_icon.dds");
INSERT INTO common_strings VALUES("160","/planeshift/potions/potionbig01c_icon.dds");
INSERT INTO common_strings VALUES("161","food#genericfood01");
INSERT INTO common_strings VALUES("162","/planeshift/food/genericfood01_icon.dds");
INSERT INTO common_strings VALUES("163","direct damage");
INSERT INTO common_strings VALUES("164","direct heal");
INSERT INTO common_strings VALUES("165","aoe damage");
INSERT INTO common_strings VALUES("166","aoe heal");
INSERT INTO common_strings VALUES("167","buff");
INSERT INTO common_strings VALUES("168","debuff");
INSERT INTO common_strings VALUES("169","stand");
INSERT INTO common_strings VALUES("170","idle_var");
INSERT INTO common_strings VALUES("171","walk");
INSERT INTO common_strings VALUES("172","run");
INSERT INTO common_strings VALUES("173","combat stand");
INSERT INTO common_strings VALUES("174","hit");
INSERT INTO common_strings VALUES("175","death");
INSERT INTO common_strings VALUES("176","cast");
INSERT INTO common_strings VALUES("177","sit");
INSERT INTO common_strings VALUES("178","sit idle");
INSERT INTO common_strings VALUES("179","stand up");
INSERT INTO common_strings VALUES("180","Head");
INSERT INTO common_strings VALUES("181","$F_head");
INSERT INTO common_strings VALUES("182", "/planeshift/models/$F/$F_head.dds");
INSERT INTO common_strings VALUES("183","$F_head_2");
INSERT INTO common_strings VALUES("184", "/planeshift/models/$F/$F_head_2.dds");
INSERT INTO common_strings VALUES("187","rogue");
INSERT INTO common_strings VALUES("2701","Torso");
INSERT INTO common_strings VALUES("2702","Arm");
INSERT INTO common_strings VALUES("2703","Foot");
INSERT INTO common_strings VALUES("2704","Hand");
INSERT INTO common_strings VALUES("2705","Legs");
INSERT INTO common_strings VALUES("2706","$F_torso_leather");
INSERT INTO common_strings VALUES("2707","$F_arm_leather");
INSERT INTO common_strings VALUES("2708","$F_foot_leather");
INSERT INTO common_strings VALUES("2709","$F_hand_leather");
INSERT INTO common_strings VALUES("2710","$F_legs_leather");
INSERT INTO common_strings VALUES("211","$F_torso_chainmail");
INSERT INTO common_strings VALUES("212","$F_arm_chainmail");
INSERT INTO common_strings VALUES("213","$F_foot_chainmail");
INSERT INTO common_strings VALUES("214","$F_hand_chainmail");
INSERT INTO common_strings VALUES("215","$F_legs_chainmail");
INSERT INTO common_strings VALUES("216","$F_torso_plate");
INSERT INTO common_strings VALUES("217","$F_arm_plate");
INSERT INTO common_strings VALUES("218","$F_foot_plate");
INSERT INTO common_strings VALUES("219","$F_hand_plate");
INSERT INTO common_strings VALUES("220","$F_legs_plate");
INSERT INTO common_strings VALUES("221","$P_Plate");
INSERT INTO common_strings VALUES("222","items#torch");
INSERT INTO common_strings VALUES("223","/planeshift/items/torch.png");
INSERT INTO common_strings VALUES("224","items#campfire01a");
INSERT INTO common_strings VALUES("225","/planeshift/items/campfire01a.dds");

INSERT INTO common_strings VALUES("1162","weapons#round_shield_01a");
INSERT INTO common_strings VALUES("1163","/planeshift/weapons/roundshield01a_icon.dds");
INSERT INTO common_strings VALUES("9999","nullmesh");
