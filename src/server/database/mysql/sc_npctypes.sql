CREATE TABLE  `planeshift`.`sc_npctypes` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'An unique id for this row',
  `name` varchar(200) NOT NULL COMMENT 'The name of this npctype',
  `parents` varchar(200) NOT NULL COMMENT 'The parents of this npctype for inheritance',
  `ang_vel` float DEFAULT '999' COMMENT 'Angular speed of t he npctype',
  `vel` varchar(200) DEFAULT '999' COMMENT 'Speed of the npctype',
  `collision` varchar(200) DEFAULT NULL COMMENT 'Perception when colliding',
  `out_of_bounds` varchar(200) DEFAULT NULL COMMENT 'Perception when out of bounds',
  `in_bounds` varchar(200) DEFAULT NULL COMMENT 'Perception when in bounds',
  `falling` varchar(200) DEFAULT NULL COMMENT 'Perception when falling',
  `script` text NOT NULL COMMENT 'The script of this npctype',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Stores the list of npc types';

