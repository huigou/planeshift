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
-- Table structure for table `natural_resources`
--

DROP TABLE IF EXISTS `natural_resources`;
CREATE TABLE `natural_resources` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `loc_sector_id` int(10) unsigned NOT NULL default '0',
  `loc_x` float(10,2) default '0.00',
  `loc_y` float(10,2) default '0.00',
  `loc_z` float(10,2) default '0.00',
  `radius` float(10,2) default '0.00',
  `visible_radius` float(10,2) default '0.00',
  `probability` float(10,6) default '0.000000',
  `skill` smallint(4) unsigned default '0',
  `skill_level` int(3) unsigned default '0',
  `item_cat_id` int(8) unsigned default '0',
  `item_quality` float(10,6) default '0.000000',
  `animation` varchar(30) default NULL,
  `anim_duration_seconds` int(10) unsigned default '0',
  `item_id_reward` int(10) unsigned default '0',
  `reward_nickname` varchar(30) default '0',
  `action` varchar(45) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=8 DEFAULT CHARSET=latin1;

--
-- Dumping data for table `natural_resources`
--

LOCK TABLES `natural_resources` WRITE;
/*!40000 ALTER TABLE `natural_resources` DISABLE KEYS */;
INSERT INTO `natural_resources` VALUES (1,3,-50.00,0.00,-150.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,85,'gold','dig'),(2,3,-50.00,0.00,-150.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,102,'iron','dig'),(3,3,-50.00,0.00,-150.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,103,'coal','dig'),(4,3,-50.00,0.00,-150.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,115,'clay','dig'),(5,3,-50.00,0.00,-150.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,116,'sand','dig'),(6,3,-80.00,0.00,-145.00,40.00,50.00,0.200000,37,50,6,0.500000,'greet',10,85,'gold','dig');
/*!40000 ALTER TABLE `natural_resources` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2007-09-27 10:47:41
