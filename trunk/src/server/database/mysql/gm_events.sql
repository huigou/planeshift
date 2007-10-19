#
# Table for GM Events
#
CREATE TABLE `gm_events` (
  `id` int(10) NOT NULL default '0',
  `name` varchar(40) NOT NULL default '',
  `description` blob NOT NULL,
  `status` int NOT NULL default '0',
  `gm_id` int(10) NOT NULL default '0',
  PRIMARY KEY  (`id`)
);
