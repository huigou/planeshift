# HeidiSQL Dump 
#
# --------------------------------------------------------
# Host:                 127.0.0.1
# Database:             planeshift
# Server version:       5.0.67-community-nt
# Server OS:            Win32
# Target-Compatibility: Standard ANSI SQL
# HeidiSQL version:     3.2 Revision: 1129
# --------------------------------------------------------

/*!40100 SET CHARACTER SET latin1;*/
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ANSI';*/
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;*/


#
# Database structure for database 'planeshift'
#

CREATE DATABASE /*!32312 IF NOT EXISTS*/ "planeshift" /*!40100 DEFAULT CHARACTER SET utf8 */;

USE "planeshift";


#
# Table structure for table 'npc_responses'
#

CREATE TABLE /*!32312 IF NOT EXISTS*/ "npc_responses" (
  "id" int(10) unsigned NOT NULL auto_increment,
  "trigger_id" int(10) unsigned default '0',
  "response1" text,
  "response2" text,
  "response3" text,
  "response4" text,
  "response5" text,
  "pronoun_him" varchar(30) default '',
  "pronoun_her" varchar(30) default '',
  "pronoun_it" varchar(30) default '',
  "pronoun_them" varchar(30) default '',
  "script" blob,
  "prerequisite" blob,
  "quest_id" int(10) unsigned default NULL,
  "audio_path" varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand.',
  PRIMARY KEY  ("id")
) AUTO_INCREMENT=45 /*!40100 DEFAULT CHARSET=utf8*/;



#
# Dumping data for table 'npc_responses'
#

LOCK TABLES "npc_responses" WRITE;
/*!40000 ALTER TABLE "npc_responses" DISABLE KEYS;*/
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('1','1','Hello $playername.','Hello friend.','Hi.  What can I do for you?','Go away!','Whatever dude...','0','0','0','0','<response><respondpublic/><action anim="greet"/></response>','','0','/planeshift/data/voice/merchant/hello_n.wav');
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('2','2','I''m fine and you?','Terrible... what a day...','Comme ci, comme ca... et toi?','Va bene, e tu?','','0','0','0','0','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('3','3','Later.','May the Crystal shine brightly on you.','Goodbye $playername.','','','0','0','0','0','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('11','4','I''m just a simple peasant.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('12','5','I''m the great creator of many weapons.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('15','6','I will train you','','','','','','','','','<response><respond/><train skill="Sword"/></response>','<pre><faction name="orcs" value="0"/></pre>','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('16','6','No way I train you','','','','','','','','','','<pre><not><faction name="orcs" value="0"/></not></pre>','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('17','9','Thanks, friend.','What a lovely gesture.','','','','','','','','<response><respond/><action anim="greet"/></response>','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('18','10','Umm, didn''t you just say that?','I feel like I''m repeating myself here','Don''t be annoying.','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('19','11','Oh weird, it''s like deja vu!','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('20','12','I have already responded to that and I will not do so again, $sir!','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('21','13','I''m not giving you anything.  What have you given me?','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('22','14','Ok, I will give you some exp.','','','','','','','','','<response><respond/><run scr="give_exp" param0="200" /></response>','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('23','15','Hi $sir!, I will say thank you for 11 trias.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('24','16','You are speaking strangely, please rephrase.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('25','17','this is my error response.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('26','19','I QuestMaster1 test Quest sub step control[Type ''step2'', verify error response. Type ''step1'', verify quest started and follow instructions], and Quest lockouts[Type ''quest2'', veriyf quest started and follow instructions.]','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('27','20','I QuestMaster2 test Quest prerequisites[Type ''quest2'', verify error response. Type ''quest1'', verify quest assigned and follow instructions]','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('28','21','I have the following fruits: apple, orange','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('29','22','You asked for apple','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('30','23','You asked for orange','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('31','24','A greeting from DictMaster2','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('32','25','I have the following fruits: pear, plum','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('33','26','You asked for pear','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('34','27','You asked for plum','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('35','29','How rude! You asked for about me, without saying hello.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('36','28','You asked for about me','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('37','20','Welcome to npcroom, $playername.  Do you need help?',NULL,NULL,NULL,NULL,'','','','',NULL,'','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('38','31','Be thee friend or foe, stranger?',NULL,NULL,NULL,NULL,'','','','','','','0','/data/voice/merchant/betheefriend.spx');
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('39','32','Hark!  Who goes there?',NULL,NULL,NULL,NULL,'','','','','','','0','/data/voice/merchant/hark.spx');
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('40','33','Ok, I will winch send the "winch up" command to the beast.','','','','','','','','','<response><respond/><npccmd cmd="winch_up" /></response>','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('41','34','Ok, I will winch send the "winch down" command to the beast.','','','','','','','','','<response><respond/><npccmd cmd="winch_down" /></response>','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('42','8','I will train you','','','','','','','','','<response><respond/><train skill="Sword"/></response>','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('43','18','this is my error response.','','','','','','','','','','','0',NULL);
REPLACE INTO "npc_responses" ("id", "trigger_id", "response1", "response2", "response3", "response4", "response5", "pronoun_him", "pronoun_her", "pronoun_it", "pronoun_them", "script", "prerequisite", "quest_id", "audio_path") VALUES
	('44','35','You just asked a question that I do not know the answer to.','','','','','','','','','','','0',NULL);
/*!40000 ALTER TABLE "npc_responses" ENABLE KEYS;*/
UNLOCK TABLES;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE;*/
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;*/
