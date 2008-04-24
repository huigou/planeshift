--
-- Table structure for table `wc_accessrules`
--

DROP TABLE IF EXISTS `wc_accessrules`;
CREATE TABLE `wc_accessrules` (
  `id` int(10) NOT NULL auto_increment,
  `security_level` tinyint(3) NOT NULL,
  `objecttype` varchar(50) NOT NULL,
  `access` tinyint(3) default NULL,
  UNIQUE KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `wc_accessrules`
--


/*!40000 ALTER TABLE `wc_accessrules` DISABLE KEYS */;
LOCK TABLES `wc_accessrules` WRITE;
INSERT INTO `wc_accessrules` VALUES (1,31,'quests',4),(3,31,'items',4),(4,31,'als',4),(5,31,'npcs',4),(6,30,'quests',1),(7,30,'items',1),(8,30,'als',1),(9,30,'npcs',1);
UNLOCK TABLES;
