# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_waypoint_aliases'
#

DROP TABLE IF EXISTS `sc_waypoint_aliases`;
CREATE TABLE `sc_waypoint_aliases` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `wp_id` int(8) unsigned NOT NULL default '0',
  `alias` varchar(30) NOT NULL DEFAULT '' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table 'sc_waypoint_lists'
#
INSERT INTO `sc_waypoint_aliases` VALUES (1, 20, 'smith_work');
