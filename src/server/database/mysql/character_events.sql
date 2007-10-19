#
# Table to register players to GM events
#
CREATE TABLE `character_events` (
  `player_id` int(10) NOT NULL default '0',
  `event_id` int(10) NOT NULL default '0',
  PRIMARY KEY  (`player_id`,`event_id`)
);
