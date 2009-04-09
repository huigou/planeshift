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
  `id` int(10) unsigned NOT NULL auto_increment,
  `name` varchar(40) NOT NULL default '',
  `task` varchar(250) NOT NULL default '',
  `cstr_id_icon` int(10) unsigned default '0',
  `flags` tinyint(3) unsigned default '0',
  `master_quest_id` int(10) unsigned default NULL,
  `minor_step_number` tinyint(3) unsigned default '0',
  `player_lockout_time` int(10) NOT NULL default '0',
  `quest_lockout_time` int(10) NOT NULL default '0',
  `category` varchar(30) NOT NULL default '',
  `prerequisite` varchar(250) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `indx_quests_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=211 /*!40100 DEFAULT CHARSET=utf8*/;



#
# Dumping data for table 'quests'
#

LOCK TABLES `quests` WRITE;
/*!40000 ALTER TABLE `quests` DISABLE KEYS*/;
INSERT INTO `quests` (`id`, `name`, `task`, `cstr_id_icon`, `flags`, `master_quest_id`, `minor_step_number`, `player_lockout_time`, `quest_lockout_time`, `category`, `prerequisite`) VALUES
	('1','Rescue the Princess','Princess Leia is being held by the Empire on the Death Star.  You must rescue her before she is compelled to give up the wherabouts of the rebel base!','0',0,'0',0,0,0,'Newbie',''),
	('2','Sandwich Quest','Bring the merchant a freakin\' sandwich!','0',0,'0',0,10,0,'Newbie',''),
	('3','Falchion Quest','Bring the merchant a steel falchion','0',0,'0',0,120,60,'Newbie','<pre><skill name=\"Sword\" min=\"10\" max=\"20\"/></pre>'),
	('10','Male Enki Alina Quest','Find Smith and ask him if he loves his daughter.','0',0,'0',0,240,60,'Newbie','<pre><completed quest=\"Male Enki Gold\"/></pre>'),
	('11','Male Enki Gold','Mine gold ore for the MaleEnki npc.','0',0,'0',0,240,60,'Newbie',''),
	('12','Male Enki Trusted Transport','Transport the glyph to the merchant.','0',0,'0',0,300,60,'Newbie','<pre><or><completed quest=\"Male Enki Gold\"/><completed quest=\"Male Enki Alina Quest\"/></or></pre>'),
	('13','Male Enki Lost Found','Get Key','0',0,'0',0,300,60,'Newbie',''),
	('14','Flying a Kite','Up up and a way on a windy day!','0',0,'0',0,1,1,'Newbie','<pre><activemagic name=\"+DefensiveWind\"/></pre>'),
	('15','Reverse Word Lookup','Tests checking each word in a phrase as as trigger','0',0,'0',0,1,1,'Newbie',''),
	('16','Multiple item test','Tests giving multiple items and money at the same time','0',0,'0',0,1,1,'Newbie',''),
	('101','QuestMaster1 Quest 1','Test Quest for the Quest Test Case: Quest sub step control','0',0,'0',0,0,0,'Test1',''),
	('102','QuestMaster1 Quest 2','Test Quest for the Quest Test Case: Quest lockouts(player lockout 30 sec.)','0',0,'0',0,30,0,'Test1',''),
	('103','QuestMaster1 Quest 3','Test Quest for the Quest Test Case: Quest lockouts(quest lockout 30 sec.)','0',0,'0',0,0,30,'Test1',''),
	('104','QuestMaster1 Quest 4','Test Quest for the Quest Test Case: Give items','0',0,'0',0,0,30,'Test1',''),
	('201','QuestMaster2 Quest 1','Test Quest for the Quest Test Case: Quest prerequisite','0',0,'0',0,0,0,'Test2',''),
	('202','QuestMaster2 Quest 2','Test Quest for the Quest Test Case: Quest prerequisite','0',0,'0',0,0,0,'Test2','<pre><and><completed quest=\"QuestMaster2 Quest 1\"/><not><completed quest=\"QuestMaster2 Quest 3\"/></not></and></pre>'),
	('203','QuestMaster2 Quest 3','Test Quest for the Quest Test Case: Quest prerequisite','0',0,'0',0,0,0,'Test2','<pre><or><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/></or></pre>'),
	('204','QuestMaster2 Quest 4','Test Quest for the Quest Test Case: Quest prerequisite','0',0,'0',0,0,0,'Test2','<pre><require min=\"2\"><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/><completed quest=\"QuestMaster2 Quest 3\"/></require></pre>'),
	('205','QuestMaster2 Quest 5','Test Quest for the Quest Test Case: Quest prerequisite','0',0,'0',0,0,0,'Test2','<pre><completed category=\"Test2\" min=\"3\" max=\"4\"/></pre>'),
	('206','QuestMaster2 Quest 6','Test Quest for the Quest Test Case: Quest lockouts','0',0,'0',0,-1,0,'Test3',''),
	('207','Acquire Lapar','Test Quest for Audio with Smith','0',0,'0',0,-1,0,'TestAudio',''),
	('208','One Thousand Year Sammich','Long complex demo quest','0',0,'0',0,0,0,'Test4',''),
	('209','Puzzle Quest','Answer the merchant\'s question with BLUE to complete this quest.','0',0,'0',0,0,0,'Newbie',''),
	('210','Fortune Quest','Give the merchant 5 tria and he will tell your fortune.','0',0,'0',0,0,0,'Newbie','');
/*!40000 ALTER TABLE `quests` ENABLE KEYS*/;
UNLOCK TABLES;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS*/;
