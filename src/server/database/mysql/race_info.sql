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
-- Definition of table `race_info`
--

DROP TABLE IF EXISTS `race_info`;
CREATE TABLE `race_info` (
  `id` int(10) NOT NULL default '0',
  `name` varchar(35) NOT NULL default '',
  `cstr_id_mesh` int(10) NOT NULL default '0',
  `sex` char(1) NOT NULL default 'M',
  `size_x` float unsigned default '0',
  `size_y` float unsigned default '0',
  `size_z` float unsigned default '0',
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
);

--
-- Dumping data for table `race_info`
--

/*!40000 ALTER TABLE `race_info` DISABLE KEYS */;
INSERT INTO `race_info` (`id`,`name`,`cstr_id_mesh`,`sex`,`size_x`,`size_y`,`size_z`,`cstr_id_base_texture`,`initial_cp`,`start_str`,`start_end`,`start_agi`,`start_int`,`start_will`,`start_cha`,`base_physical_regen_still`,`base_physical_regen_walk`,`base_mental_regen_still`,`base_mental_regen_walk`,`armor_id`,`helm`,`race`) VALUES 
 (0,'StoneBreaker',125,'M',0.8,1.2,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',0),
 (1,'Enkidukai',127,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',1),
 (2,'Ynnwn',103,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',2),
 (3,'Ylian',102,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',3),
 (4,'Xacha',125,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',4),
 (5,'Nolthrir',125,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',5),
 (6,'Dermorian',108,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.2,0.8,10,10,0,'',6),
 (7,'Hammerwielder',125,'M',0.8,1.2,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',7),
 (8,'Diaboli',125,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',8),
 (9,'Kran',105,'N',0.8,1.4,0.6,19,100,50,50,50,50,50,50,2.0,1.5,10,10,0,'',9);
INSERT INTO `race_info` (`id`,`name`,`cstr_id_mesh`,`sex`,`size_x`,`size_y`,`size_z`,`cstr_id_base_texture`,`initial_cp`,`start_str`,`start_end`,`start_agi`,`start_int`,`start_will`,`start_cha`,`base_physical_regen_still`,`base_physical_regen_walk`,`base_mental_regen_still`,`base_mental_regen_walk`,`armor_id`,`helm`,`race`) VALUES 
 (10,'Lemur',125,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',10),
 (11,'Klyros',125,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'ylianm',11),
 (12,'Enkidukai',128,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',1),
 (13,'StoneBreaker',126,'F',0.8,1.2,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'ylianm',0),
 (14,'Ynnwn',130,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',2),
 (15,'Ylian',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',3),
 (16,'Xacha',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',4),
 (17,'Nolthrir',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',5),
 (18,'Dermorian',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.2,0.8,10,10,0,'',6),
 (19,'Hammerwielder',125,'F',0.8,1.2,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',7);
INSERT INTO `race_info` (`id`,`name`,`cstr_id_mesh`,`sex`,`size_x`,`size_y`,`size_z`,`cstr_id_base_texture`,`initial_cp`,`start_str`,`start_end`,`start_agi`,`start_int`,`start_will`,`start_cha`,`base_physical_regen_still`,`base_physical_regen_walk`,`base_mental_regen_still`,`base_mental_regen_walk`,`armor_id`,`helm`,`race`) VALUES 
 (20,'Diaboli',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',8),
 (21,'Lemur',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',10),
 (22,'Klyros',125,'F',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',11),
 (23,'Rogue',187,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',12),
 (24,'Clacker',117,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',13),
 (25,'Rat',118,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',14),
 (26,'Grendol',119,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',15),
 (27,'Gobble',120,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',16),
 (28,'Consumer',121,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',17),
 (29,'Trepor',122,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',18);
INSERT INTO `race_info` (`id`,`name`,`cstr_id_mesh`,`sex`,`size_x`,`size_y`,`size_z`,`cstr_id_base_texture`,`initial_cp`,`start_str`,`start_end`,`start_agi`,`start_int`,`start_will`,`start_cha`,`base_physical_regen_still`,`base_physical_regen_walk`,`base_mental_regen_still`,`base_mental_regen_walk`,`armor_id`,`helm`,`race`) VALUES 
 (30,'Ulbernaut',123,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,1.5,1.0,10,10,0,'',19),
 (31,'Tefusang',124,'M',0.8,1.4,0.6,19,100,50,50,50,50,50,50,2.0,1.5,10,10,0,'',20);
/*!40000 ALTER TABLE `race_info` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
