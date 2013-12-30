
--
-- Table structure for table 'npc_hired_npcs'
--

DROP TABLE IF EXISTS `npc_hired_npcs`;
CREATE TABLE `npc_hired_npcs` (
  `owner_id` int(8) unsigned NOT NULL default '0',
  `hired_npc_id` int(8) unsigned NOT NULL default '0',
  `guild` char(1) DEFAULT 'N',
  PRIMARY KEY (`owner_id`,`hired_npc_id`)
);

#
# Dumping data for table 'npc_hired_npcs'
#
