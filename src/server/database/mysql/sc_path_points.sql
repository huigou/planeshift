
--
-- Table structure for table `sc_path_points`
--

DROP TABLE IF EXISTS `sc_path_points`;
CREATE TABLE `sc_path_points` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `path_id` int(8) unsigned NOT NULL default '0',
  `prev_point` int(8) unsigned NOT NULL default '0',
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) NOT NULL default '0.00',
  `loc_sector_id` int(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `sc_path_points`
--
INSERT INTO `sc_path_points` VALUES (7,23,0,-26.79,0.00,-183.13,3);
INSERT INTO `sc_path_points` VALUES (8,23,7,-26.64,0.00,-174.72,3);
INSERT INTO `sc_path_points` VALUES (9,23,8,-24.98,0.00,-165.16,3);
INSERT INTO `sc_path_points` VALUES (10,23,9,-16.96,0.00,-164.90,3);
INSERT INTO `sc_path_points` VALUES (11,23,10,-13.49,0.00,-177.05,3);
INSERT INTO `sc_path_points` VALUES (12,23,11,-8.90,0.00,-180.01,3);
INSERT INTO `sc_path_points` VALUES (13,23,12,-8.26,0.38,-180.22,3);
INSERT INTO `sc_path_points` VALUES (14,23,13,-1.61,4.36,-181.57,3);
INSERT INTO `sc_path_points` VALUES (15,23,14,-1.49,4.43,-186.58,3);
INSERT INTO `sc_path_points` VALUES (16,23,15,-8.36,0.31,-188.72,3);
INSERT INTO `sc_path_points` VALUES (17,23,16,-9.39,0.00,-188.86,3);
INSERT INTO `sc_path_points` VALUES (18,23,17,-13.48,0.00,-191.37,3);
INSERT INTO `sc_path_points` VALUES (19,23,18,-25.41,0.00,-189.27,3);
INSERT INTO `sc_path_points` VALUES (20,1,0,-32.25,0.00,-185.13,3);
INSERT INTO `sc_path_points` VALUES (21,1,20,-40.69,0.00,-184.96,3);
INSERT INTO `sc_path_points` VALUES (22,15,0,-54.28,0.00,-166.22,3);
INSERT INTO `sc_path_points` VALUES (23,15,22,-52.76,0.00,-174.08,3);
INSERT INTO `sc_path_points` VALUES (24,15,23,-48.38,0.00,-182.01,3);
INSERT INTO `sc_path_points` VALUES (25,16,0,-44.69,0.00,-187.11,3);
INSERT INTO `sc_path_points` VALUES (26,16,25,-44.68,0.00,-191.76,3);
INSERT INTO `sc_path_points` VALUES (27,16,26,-41.40,0.00,-196.44,3);
INSERT INTO `sc_path_points` VALUES (28,16,27,-39.72,0.00,-200.92,3);
INSERT INTO `sc_path_points` VALUES (29,16,28,-42.21,0.00,-205.76,3);
