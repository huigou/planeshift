
CREATE TABLE IF NOT EXISTS `character_factions` (
  `character_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'The PID of the character this faction is assigned to',
  `faction_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'The id of the faction the points are for',
  `value` int(10) NOT NULL DEFAULT '0' COMMENT 'The amount of points in this faction',
  PRIMARY KEY (`character_id`,`faction_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 ROW_FORMAT=FIXED COMMENT='Stores the faction of the characters';

