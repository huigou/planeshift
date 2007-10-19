-- MySQL dump 10.11
--
-- Host: localhost    Database: planeshift
-- ------------------------------------------------------
-- Server version	5.0.37-community-nt

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `race_info`
--

DROP TABLE IF EXISTS `race_info`;
CREATE TABLE `race_info` (
  `id` int(10) NOT NULL default '0',
  `name` varchar(35) NOT NULL default '',
  `cstr_id_mesh` int(10) NOT NULL default '0',
  `sex` char(1) NOT NULL default 'M',
  `start_x` float(10,2) default '0.00',
  `start_y` float(10,2) default '0.00',
  `start_z` float(10,2) default '0.00',
  `size_x` float unsigned default '0',
  `size_y` float unsigned default '0',
  `size_z` float unsigned default '0',
  `start_sector_id` int(10) default NULL,
  `start_yrot` float(10,2) default '0.00',
  `cstr_id_base_texture` int(10) unsigned NOT NULL default '0',
  `initial_cp` int(10) default '0',
  `start_str` int(5) NOT NULL default '50',
  `start_end` int(5) NOT NULL default '50',
  `start_agi` int(5) NOT NULL default '50',
  `start_int` int(5) NOT NULL default '50',
  `start_will` int(5) NOT NULL default '50',
  `start_cha` int(5) unsigned NOT NULL default '50',
  `base_physical_regen_still` float NOT NULL default '0',
  `base_physical_regen_walk` float NOT NULL default '0',
  `base_mental_regen_still` float NOT NULL default '0',
  `base_mental_regen_walk` float NOT NULL default '0',
  `armor_id` int(10) unsigned default '0',
  `helm` varchar(20) default '',
  `race` int(5) unsigned NOT NULL,
  PRIMARY KEY  USING BTREE (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `race_info`
--

LOCK TABLES `race_info` WRITE;
/*!40000 ALTER TABLE `race_info` DISABLE KEYS */;
INSERT INTO `race_info` VALUES (0,'StoneBreaker',125,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',0),(1,'Enkidukai',127,'M',2.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',1),(2,'Ynnwn',103,'M',4.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',2),(3,'Ylian',102,'M',-2.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',3),(4,'Xacha',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',4),(5,'Nolthrir',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',5),(6,'Dermorian',108,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',6),(7,'Hammerwielder',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',7),(8,'Diaboli',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',8),(9,'Kran',105,'N',-4.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',9),(10,'Lemur',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',10),(11,'Klyros',125,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'ylianm',11),(12,'Enkidukai',128,'F',-4.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',1),(13,'StoneBreaker',126,'F',-2.00,-2.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'ylianm',0),(14,'Ynnwn',130,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',2),(15,'Ylian',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',3),(16,'Xacha',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',4),(17,'Nolthrir',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',5),(18,'Dermorian',125,'F',0.00,-4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',6),(19,'Hammerwielder',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',7),(20,'Diaboli',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',8),(21,'Lemur',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',10),(22,'Klyros',125,'F',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',11),(23,'Rogue',187,'M',0.00,0.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',12),(24,'Clacker',117,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',13),(25,'Rat',118,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',14),(26,'Grendol',119,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',15),(27,'Gobble',120,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',16),(28,'Consumer',121,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',17),(29,'Trepor',122,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',18),(30,'Ulbernaut',123,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',19),(31,'Tefusang',124,'M',-20.00,4.00,-150.00,0,0,0,3,0.00,19,100,50,50,50,50,50,50,10,10,10,10,0,'',20);
/*!40000 ALTER TABLE `race_info` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2007-08-30 20:23:26
