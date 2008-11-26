# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'quests'
#

CREATE TABLE quests (
  id int(10) unsigned NOT NULL auto_increment,
  name varchar(40) NOT NULL DEFAULT '' ,
  task varchar(250) NOT NULL DEFAULT '' ,
  cstr_id_icon int(10) unsigned DEFAULT '0' ,
  flags tinyint(3) unsigned DEFAULT '0' ,
  master_quest_id int(10) unsigned ,
  minor_step_number tinyint(3) unsigned DEFAULT '0' ,
  player_lockout_time int(10) NOT NULL DEFAULT '0' ,
  quest_lockout_time int(10) NOT NULL DEFAULT '0' ,
  category varchar(30) NOT NULL DEFAULT '' ,
  prerequisite varchar(250) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'quests'
#

INSERT INTO quests VALUES("1","Rescue the Princess","Princess Leia is being held by the Empire on the Death Star.  You must rescue her before she is compelled to give up the wherabouts of the rebel base!","0","0","0","0","0","0","Newbie","");
INSERT INTO quests VALUES("2","Sandwich Quest","Bring the merchant a freakin' sandwich!","0","0","0","0","10","0","Newbie","");
INSERT INTO quests VALUES("3","Falchion Quest","Bring the merchant a steel falchion","0","0","0","0","120","60","Newbie","<pre><skill name=\"Sword\" min=\"10\" max=\"20\"/></pre>");
INSERT INTO quests VALUES("10","Male Enki Alina Quest","Find Smith and ask him if he loves the daugther.","0","0","0","0","240","60","Newbie","<pre><completed quest=\"Male Enki Gold\"/></pre>");
INSERT INTO quests VALUES("11","Male Enki Gold","Mine gold ore for the MaleEnki npc.","0","0","0","0","240","60","Newbie","");
INSERT INTO quests VALUES("12","Male Enki Trusted Transport","Transport the glyph to the merchant.","0","0","0","0","300","60","Newbie","<pre><or><completed quest=\"Male Enki Gold\"/><completed quest=\"Male Enki Alina Quest\"/></or></pre>");
INSERT INTO quests VALUES("13","Male Enki Lost Found","Get Key","0","0","0","0","300","60","Newbie","");
INSERT INTO quests VALUES("14","Flying a Kite","Up up and a way on a windy day!","0","0","0","0","1","1","Newbie",'<pre><activemagic name="+DefensiveWind"/></pre>');
INSERT INTO quests VALUES("15","Reverse Word Lookup", "Tests checking each word in a phrase as as trigger", 0, 0, 0, 0, 1, 1, "Newbie", "");
INSERT INTO quests VALUES("16","Multiple item test", "Tests giving multiple items and money at the same time", 0, 0, 0, 0, 1, 1, "Newbie", "");
INSERT INTO quests VALUES("207","JuiceBerry Fruit quest","Test Quest for Audio with Smith","0","0","0","0","-1","0","TestAudio","");

#
# Quest Test Case quests
#
# QuestMaster1 Quests
#
INSERT INTO quests VALUES("101","QuestMaster1 Quest 1","Test Quest for the Quest Test Case: Quest sub step control","0","0","0","0","0","0","Test1","");
INSERT INTO quests VALUES("102","QuestMaster1 Quest 2","Test Quest for the Quest Test Case: Quest lockouts(player lockout 30 sec.)","0","0","0","0","30","0","Test1","");
INSERT INTO quests VALUES("103","QuestMaster1 Quest 3","Test Quest for the Quest Test Case: Quest lockouts(quest lockout 30 sec.)","0","0","0","0","0","30","Test1","");
INSERT INTO quests VALUES("104","QuestMaster1 Quest 4","Test Quest for the Quest Test Case: Give items","0","0","0","0","0","30","Test1","");
#
# QuestMaster2 Quests
#
INSERT INTO quests VALUES("201","QuestMaster2 Quest 1","Test Quest for the Quest Test Case: Quest prerequisite","0","0","0","0","0","0","Test2","");
INSERT INTO quests VALUES("202","QuestMaster2 Quest 2","Test Quest for the Quest Test Case: Quest prerequisite","0","0","0","0","0","0","Test2","<pre><and><completed quest=\"QuestMaster2 Quest 1\"/><not><completed quest=\"QuestMaster2 Quest 3\"/></not></and></pre>");
INSERT INTO quests VALUES("203","QuestMaster2 Quest 3","Test Quest for the Quest Test Case: Quest prerequisite","0","0","0","0","0","0","Test2","<pre><or><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/></or></pre>");
INSERT INTO quests VALUES("204","QuestMaster2 Quest 4","Test Quest for the Quest Test Case: Quest prerequisite","0","0","0","0","0","0","Test2","<pre><require min=\"2\"><completed quest=\"QuestMaster2 Quest 1\"/><completed quest=\"QuestMaster2 Quest 2\"/><completed quest=\"QuestMaster2 Quest 3\"/></require></pre>");
INSERT INTO quests VALUES("205","QuestMaster2 Quest 5","Test Quest for the Quest Test Case: Quest prerequisite","0","0","0","0","0","0","Test2","<pre><completed category=\"Test2\" min=\"3\" max=\"4\"/></pre>");
INSERT INTO quests VALUES("206","QuestMaster2 Quest 6","Test Quest for the Quest Test Case: Quest lockouts","0","0","0","0","-1","0","Test3","");
