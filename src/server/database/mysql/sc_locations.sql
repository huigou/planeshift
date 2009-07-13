# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_locations'
#

DROP TABLE IF EXISTS `sc_locations`;
CREATE TABLE `sc_locations` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `type_id` int(8) unsigned NOT NULL DEFAULT '0',
  `id_prev_loc_in_region` int(8) NOT NULL DEFAULT '0',
  `name` varchar(30) NOT NULL DEFAULT '' ,
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) NOT NULL default '0.00',
  `radius` int(10) NOT NULL default '0',
  `angle` double(10,2) NOT NULL default '0.0',
  `flags` varchar(30) DEFAULT '' ,
  `loc_sector_id` int(8) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table 'locations'
#
INSERT INTO `sc_locations` VALUES (1,  1, -1, 'Field 1','-80.00', '0.0', '-220.00', 10, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (2,  1, -1, 'Field 2','-50.00', '0.0', '-200.00', 2, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (3,  1, -1, 'Field 3','-40.00', '0.0', '-130.00', 5, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (4,  1, -1, 'Field 4','30.00', '0.0', '-200.00', 10, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (5,  2, -1, 'Noth mine','-30.00', '0.0', '-140.00', 20, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (6,  2, -1, 'South mine','-45.00', '0.0', '-240.00', 30, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (7,  2, -1, 'Main mine','-80.00', '0.0', '-145.00', 15, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (8,  3, -1, 'Main market','-30.00', '0.0', '-160.00', 2, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (9,  5, 13, 'Wander region 1','33.5', '0.0', '-213.57', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (10, 5,  9, 'Wander region 1','27.18', '0.0', '-263.67', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (11, 5, 10, 'Wander region 1','-22.18', '0.0', '-256.67', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (12, 5, 11, 'Wander region 1','-27.89', '0.0', '-209.85', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (13, 5, 12, 'Wander region 1','1.12', '0.0', '-191.12', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (14, 1, -1, 'Field 5','-5.00', '0.0', '-40.00', 30, 0.0,'', 6);
INSERT INTO `sc_locations` VALUES (15, 4, 18, 'PVP 1','20.0', '0.0', '-250.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (16, 4, 15, 'PVP 1','20.0', '0.0', '-270.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (17, 4, 16, 'PVP 1','40.0', '0.0', '-270.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (18, 4, 17, 'PVP 1','40.0', '0.0', '-250.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (19, 6, -1, 'Mover pos 1','-35.00', '0.0', '-195.00', 0, 180.0,'', 3);
INSERT INTO `sc_locations` VALUES (20, 6, -1, 'Beast pos 1','-30.00', '0.0', '-190.00', 0, 90.0,'', 3);
INSERT INTO `sc_locations` VALUES (21, 7, 24, 'NPCRoom FR 1 p1','0', '0.0', '-130.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (22, 7, 21, 'NPCRoom FR 1 p2','20.0', '0.0', '-130.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (23, 7, 22, 'NPCRoom FR 1 p3','20.0', '0.0', '-150.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (24, 7, 23, 'NPCRoom FR 1 p4','0.0', '0.0', '-150.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (25, 8, -1, 'MoveMaster5A','-56.0', '0.0', '-147.5', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (26, 9, -1, 'MoveMaster5A','-61.5', '0.0', '-217.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (27, 8, -1, 'MoveMaster5B','-40.5', '0.0', '-192.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (28, 9, -1, 'MoveMaster5B','-40.0', '0.0', '-170.0', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (29, 8, -1, 'MoveMaster5C','4.5', '0.0', '-239.5', 0, 0.0,'', 3);
INSERT INTO `sc_locations` VALUES (30, 9, -1, 'MoveMaster5C','-23.0', '0.0', '-125.0', 0, 0.0,'', 3);

