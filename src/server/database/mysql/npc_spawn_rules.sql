-- MySQL Administrator dump 1.4
--
-- ------------------------------------------------------
-- Server version	5.0.37-community-nt


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;


--
-- Definition of table `npc_spawn_rules`
--

DROP TABLE IF EXISTS `npc_spawn_rules`;
CREATE TABLE `npc_spawn_rules` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `min_spawn_time` int(10) unsigned NOT NULL default '0',
  `max_spawn_time` int(10) unsigned NOT NULL default '0',
  `substitute_spawn_odds` float(6,4) default '0.0000',
  `substitute_player` int(10) unsigned default '0',
  `fixed_spawn_x` float(10,2) default NULL,
  `fixed_spawn_y` float(10,2) default NULL,
  `fixed_spawn_z` float(10,2) default NULL,
  `fixed_spawn_rot` float(6,4) default '0.0000',
  `fixed_spawn_sector` varchar(40) default 'room',
  `fixed_spawn_instance` int(10) unsigned NOT NULL default '0',
  `loot_category_id` int(10) unsigned NOT NULL default '0',
  `dead_remain_time` int(10) unsigned NOT NULL default '0',
  `name` varchar(40) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1000 DEFAULT CHARSET=latin1;

--
-- Dumping data for table `npc_spawn_rules`
--

/*!40000 ALTER TABLE `npc_spawn_rules` DISABLE KEYS */;
INSERT INTO `npc_spawn_rules` (`id`,`min_spawn_time`,`max_spawn_time`,`substitute_spawn_odds`,`substitute_player`,`fixed_spawn_x`,`fixed_spawn_y`,`fixed_spawn_z`,`fixed_spawn_rot`,`fixed_spawn_sector`,`fixed_spawn_instance`,`loot_category_id`,`dead_remain_time`,`name`) VALUES 
 (1,10000,20000,0.0000,0,0.00,0.00,0.00,0.0000,'startlocation',0,0,30000,'Respawn at Orig Location'),
 (100,10000,20000,0.0000,0,10.00,0.00,-110.00,0.0000,'NPCroom',0,0,30000,'Anywhere in npcroom forest-quick'),
 (999,25000,60000,0.0000,0,-4.50,-1.10,-4.00,4.5500,'hydlaa_plaza',0,0,0,'Test Hydlaa Plaza');
/*!40000 ALTER TABLE `npc_spawn_rules` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
