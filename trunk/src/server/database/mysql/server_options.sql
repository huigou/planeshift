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
# Table structure for table 'server_options'
#

CREATE TABLE /*!32312 IF NOT EXISTS*/ "server_options" (
  "option_name" varchar(50) NOT NULL default '',
  "option_value" varchar(90) NOT NULL default '',
  PRIMARY KEY  ("option_name")
) /*!40100 DEFAULT CHARSET=latin1*/;



#
# Dumping data for table 'server_options'
#

LOCK TABLES "server_options" WRITE;
/*!40000 ALTER TABLE "server_options" DISABLE KEYS;*/
REPLACE INTO "server_options" ("option_name", "option_value") VALUES
	('db_version','1208');
REPLACE INTO "server_options" ("option_name", "option_value") VALUES
	('game_date','100-1-4');
REPLACE INTO "server_options" ("option_name", "option_value") VALUES
	('game_time','6:00');
REPLACE INTO "server_options" ("option_name", "option_value") VALUES
	('standard_motd','This is the message of the day from server_options table.');
/*!40000 ALTER TABLE "server_options" ENABLE KEYS;*/
UNLOCK TABLES;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE;*/
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;*/
