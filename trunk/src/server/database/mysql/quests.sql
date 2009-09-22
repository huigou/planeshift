# HeidiSQL Dump 
#
# --------------------------------------------------------
# Host:                 127.0.0.1
# Database:             planeshift
# Server version:       5.0.67-community-nt
# Server OS:            Win32
# Target-Compatibility: MySQL 5.0
# max_allowed_packet:   1048576
# HeidiSQL version:     3.2 Revision: 1129
# --------------------------------------------------------

/*!40100 SET CHARACTER SET latin1*/;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0*/;


DROP TABLE IF EXISTS `quests`;

#
# Table structure for table 'quests'
#

CREATE TABLE `quests` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(40) NOT NULL DEFAULT '',
  `task` varchar(250) NOT NULL DEFAULT '',
  `cstr_icon` varchar(200) NOT NULL DEFAULT '',
  `flags` tinyint(3) unsigned DEFAULT '0',
  `master_quest_id` int(10) unsigned DEFAULT NULL,
  `minor_step_number` tinyint(3) unsigned DEFAULT '0',
  `player_lockout_time` int(10) NOT NULL DEFAULT '0',
  `quest_lockout_time` int(10) NOT NULL DEFAULT '0',
  `category` varchar(30) NOT NULL DEFAULT '',
  `prerequisite` varchar(250) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `indx_quests_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=211 /*!40100 DEFAULT CHARSET=utf8*/;



#
# Dumping data for table 'quests'
#

LOCK TABLES `quests` WRITE;
/*!40000 ALTER TABLE `quests` DISABLE KEYS*/;
INSERT INTO `quests` VALUES (1,'Rescue the Princess','Princess Leia is being held by the Empire on the Death Star.  You must rescue her before she is compelled to give up the wherabouts of the rebel base!',NULL,0,0,0,0,0,'Newbie','');
INSERT INTO `quests` VALUES (2,'Sandwich Quest','Bring the merchant a freakin\' sandwich!',NULL,0,0,0,10,0,'Newbie','');
INSERT INTO `quests` VALUES (3,'Falchion Quest','Bring the merchant a steel falchion',NULL,0,0,0,120,60,'Newbie','<pre><skill name=\"Sword\" min=\"10\" max=\"20\"/></pre>');
INSERT INTO `quests` VALUES (10,'Male Enki Alina Quest','Find Smith and ask him if he loves his daughter.',NULL,0,0,0,240,60,'Newbie','<pre><completed quest=\"Male Enki Gold\"/></pre>');
INSERT INTO `quests` VALUES (11,'Male Enki Gold','Mine gold ore for the MaleEnki npc.',NULL,0,0,0,240,60,'Newbie','');
INSERT INTO `quests` VALUES (12,'Male Enki Trusted Transport','Transport the glyph to the merchant.',NULL,0,0,0,300,60,'Newbie','<pre><or><completed quest=\"Male Enki Gold\"/><completed quest=\"Male Enki Alina Quest\"/></or></pre>');
INSERT INTO `quests` VALUES (13,'Male Enki Lost Found','Get Key',NULL,0,0,0,300,60,'Newbie','');
INSERT INTO `quests` VALUES (14,'Flying a Kite','Up up and a way on a windy day!',NULL,0,0,0,1,1,'Newbie','<pre><activemagic name=\"+DefensiveWind\"/></pre>');
INSERT INTO `quests` VALUES (15,'Reverse Word Lookup','Tests checking each word in a phrase as as trigger',NULL,0,0,0,1,1,'Newbie','');
INSERT INTO `quests` VALUES (16,'Multiple item test','Tests giving multiple items and money at the same time',NULL,0,0,0,1,1,'Newbie','');
INSERT INTO `quests` VALUES (101,'QuestMaster1 Quest 1','Test Quest for the Quest Test Case: Quest sub step control',NULL,0,0,0,0,0,'Test1','');
INSERT INTO `quests` VALUES (102,'QuestMaster1 Quest 2','Test Quest for the Quest Test Case: Quest lockouts(player lockout 30 sec.)',NULL,0,0,0,30,0,'Test1','');
INSERT INTO `quests` VALUES (103,'QuestMaster1 Quest 3','Test Quest for the Quest Test Case: Quest lockouts(quest lockout 30 sec.)',NULL,0,0,0,0,30,'Test1','');
INSERT INTO `quests` VALUES (104,'QuestMaster1 Quest 4','Test Quest for the Quest Test Case: Give items',NULL,0,0,0,0,30,'Test1','');
INSERT INTO `quests` VALUES (201,'QuestMaster2 Quest 1','Test Quest for the Quest Test Case: Quest prerequisite',NULL,0,0,0,0,0,'Test2','');
INSERT INTO `quests` VALUES (202,'QuestMaster2 Quest 2','Test Quest for the Quest Test Case: Quest prerequisite',NULL,0,0,0,0,0,'Test2','<pre><and><completed quest=\"QuestMaster2 Quest 1\"/><not><completed quest=\"QuestMaster2 Quest 3\"/></not></and></pre>');
INSERT INTO `quests` VALUES (203,'QuestMaster2 Quest 3','Test Quest for the Quest Test Case: Quest prerequisite',NULL,0,0,0,0,0,'Test2','<pre><or><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/></or></pre>');
INSERT INTO `quests` VALUES (204,'QuestMaster2 Quest 4','Test Quest for the Quest Test Case: Quest prerequisite',NULL,0,0,0,0,0,'Test2','<pre><require min=\"2\"><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/><completed quest=\"QuestMaster2 Quest 3\"/></require></pre>');
INSERT INTO `quests` VALUES (205,'QuestMaster2 Quest 5','Test Quest for the Quest Test Case: Quest prerequisite',NULL,0,0,0,0,0,'Test2','<pre><completed category=\"Test2\" min=\"3\" max=\"4\"/></pre>');
INSERT INTO `quests` VALUES (206,'QuestMaster2 Quest 6','Test Quest for the Quest Test Case: Quest lockouts',NULL,0,0,0,-1,0,'Test3','');
INSERT INTO `quests` VALUES (207,'Acquire Lapar','Test Quest for Audio with Smith',NULL,0,0,0,-1,0,'TestAudio','');
INSERT INTO `quests` VALUES (208,'One Thousand Year Sammich','Long complex demo quest',NULL,0,0,0,0,0,'Test4','');
INSERT INTO `quests` VALUES (209,'Puzzle Quest','Answer the merchant\'s question with BLUE to complete this quest.',NULL,0,0,0,0,0,'Newbie','');
INSERT INTO `quests` VALUES (210,'Fortune Quest','Give the merchant 5 tria and he will tell your fortune.',NULL,0,0,0,0,0,'Newbie','');
/*!40000 ALTER TABLE `quests` ENABLE KEYS*/;
UNLOCK TABLES;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS*/;
