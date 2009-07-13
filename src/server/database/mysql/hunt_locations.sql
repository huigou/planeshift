# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'hunt_locations'
#

DROP TABLE IF EXISTS `hunt_locations`;
CREATE TABLE `hunt_locations` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) default '0.00',
  `itemid` int(10) NOT NULL default '0',
  `interval` int(11) NOT NULL default '0',
  `max_random` int(11) NOT NULL default '0',
  `sector` varchar(30) NOT NULL default '',
  `amount` int(10) unsigned NOT NULL default '1',
  `range` double(10,2) unsigned NOT NULL default '0.00',
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table 'hunt_locations'
#
INSERT INTO `hunt_locations` VALUES (1, '-16.60', '-0.01', '-171.90', 91, 4000, 0, 'NPCroom', 1, '0.00');




