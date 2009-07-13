# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_waypoints'
#

DROP TABLE IF EXISTS `sc_waypoints`;
CREATE TABLE `sc_waypoints` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL DEFAULT '' ,
  `wp_group` varchar(30) NOT NULL DEFAULT '' ,
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) NOT NULL default '0.00',
  `radius` int(10) NOT NULL default '0',
  `flags` varchar(100) DEFAULT '' ,
  `loc_sector_id` int(8) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY  (`id`),
  UNIQUE name (name)
);

#
# Dumping data for table 'waypoints'
#
INSERT INTO `sc_waypoints` VALUES (1, 'p1', '', '-45.00', '0.0', '-159.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (2, 'p2', '', '-49.90', '0.0', '-152.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (3, 'p3', '', '-57.30', '0.0', '-142.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (4, 'p4', '', '-66.50', '0.0', '-149.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (5, 'p5', '', '-61.00', '0.0', '-158.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (6, 'p6', '', '-54.00', '0.0', '-163.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (7, 'p7', '', '-75.00', '0.0', '-190.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (8, 'p8', '', '-75.00', '0.0', '-170.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (9, 'p9', '', '-80.00', '0.0', '-220.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (10, 'p10', '', '-45.00', '0.0', '-210.00', 10, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (11, 'p11', '', '-45.00', '0.0', '-135.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (12, 'p12', '', '30.00', '0.0', '-215.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (13, 'tower_1', '', '-45.00', '0.0', '-185.00', 5, '', 3);
INSERT INTO `sc_waypoints` VALUES (14, 'tower_2', '', '-30.00', '0.0', '-185.00', 1, '', 3);
INSERT INTO `sc_waypoints` VALUES (15, 'corr_ent1', '', '-58.00', '0.0', '-127.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (16, 'corr_1', '', '-54.00', '0.0', '-93.00', 2, 'ALLOW_RETURN', 4);
INSERT INTO `sc_waypoints` VALUES (17, 'corr_2', '', '-22.00', '0.0', '-88.00', 2, '', 5);
INSERT INTO `sc_waypoints` VALUES (18, 'p13', '', '-20.00', '0.0', '-55.00', 5, '', 6);
INSERT INTO `sc_waypoints` VALUES (19, 'smith_1', '', '-57.56', '0.0', '-156.72', 0, '', 3);
INSERT INTO `sc_waypoints` VALUES (20, 'smith_2', '', '-53.56', '0.0', '-155.46', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (21, 'ug_entrance_1', '', '-11.00', '0.0', '-153.00', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (22, 'ug_entrance_2', '', '-11.00', '0.0', '-170.00', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (23, 'ug_1', '', '0.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (24, 'ug_2', '', '0.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (25, 'ug_3', '', '15.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (26, 'ug_4', '', '15.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (27, 'ug_5', '', '30.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (28, 'ug_6', '', '30.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (29, 'MoveMaster5A_home', '', '-56.0', '0.0', '-147.5', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (30, 'MoveMaster5A_work', '', '-61.5', '0.0', '-217.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (31, 'MoveMaster5B_home', '', '-40.5', '0.0', '-192.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (32, 'MoveMaster5B_work', '', '-40.0', '0.0', '-170.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (33, 'MoveMaster5C_home', '',   '4.5', '0.0', '-239.5', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (34, 'MoveMaster5C_work', '', '-23.0', '0.0', '-125.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (35, 'p35', '', '-52.0', '0.0', '-146.5', 1, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (36, 'tower_watch1', 'tower_watch', '-30.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (37, 'tower_watch2', 'tower_watch', '-32.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (38, 'tower_watch3', 'tower_watch', '-34.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (39, 'tower_watch4', 'tower_watch', '-36.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (40, 'tower_watch5', 'tower_watch', '-38.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (41, 'tower_watch6', 'tower_watch', '-40.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
