# from 1086 to 1088

ALTER TABLE traits ADD shader varchar(32);
ALTER TABLE character_quests DROP notes;

# from 1088 to 1090

ALTER TABLE accounts ADD (spam_points tinyint(1) default '0');
ALTER TABLE accounts MODIFY COLUMN spam_points tinyint(1) default '0' AFTER banned_until;

ALTER TABLE characters ADD (animal_affinity text);
ALTER TABLE characters ADD (familiar_id int(10) NOT NULL DEFAULT '0');

DELETE FROM movement_base where name='Fly';
UPDATE movement_base SET family=4 where id=4 and id=5;

#ALTER TABLE skills ADD (category varchar(255) NOT NULL default 'VARIOUS');
source skills.sql

UPDATE server_options set option_value=1090 where option_name='db_version';

#from 1090 to 1091

ALTER TABLE characters ADD (last_login datetime default NULL);
ALTER TABLE characters MODIFY COLUMN last_login datetime default NULL AFTER guild_private_notes;

UPDATE server_options SET option_value=1091 where option_name='db_version';

#from 1091 to 1092
ALTER TABLE characters ADD ( `character_type` int(10) unsigned default '0' );
ALTER TABLE characters MODIFY COLUMN character_type int(10) unsigned default '0' AFTER racegender_id;
ALTER TABLE characters ADD ( `owner_id` int(10) unsigned NOT NULL default '0' );

UPDATE server_options SET option_value=1092 where option_name='db_version';

#from 1092 to 1093

DROP TABLE movement_base;
DROP TABLE movement_require;
DROP TABLE movement_variant;
DROP TABLE movement_extras;
SOURCE movement_extras.sql;
SOURCE movement_require.sql;
SOURCE movement_variant.sql;
SOURCE movement_base.sql;

UPDATE server_options SET option_value=1093 where option_name='db_version';

ALTER TABLE tips MODIFY COLUMN tip varchar(64);
ALTER TABLE server_options MODIFY COLUMN option_value varchar(90);
ALTER TABLE guilds MODIFY COLUMN motd char(90);


#from 1093 to 1096

INSERT INTO item_categories VALUES (21,'Coins');

ALTER TABLE character_quests CHANGE completiondate remaininglockout int(10) default '0';
ALTER TABLE npc_triggers DROP COLUMN quest_id;
ALTER TABLE quests ADD COLUMN category varchar(30) NOT NULL DEFAULT '' AFTER quest_lockout_time;
ALTER TABLE quests ADD COLUMN prerequisite varchar(250) NOT NULL DEFAULT '' AFTER category;
ALTER TABLE quest_scripts ADD COLUMN quest_id int(10) unsigned NOT NULL DEFAULT '0' AFTER id;

UPDATE quests, quest_scripts  SET quests.category=quest_scripts.category where quests.id=quest_scripts.id;
UPDATE quest_scripts SET quest_id=id;

ALTER TABLE quest_scripts DROP COLUMN category;
ALTER TABLE quest_scripts DROP COLUMN name;
ALTER TABLE accounts MODIFY COLUMN security_level tinyint(3);

UPDATE server_options SET option_value=1096 where option_name='db_version';

#from 1096 to 1097
SOURCE command_access.sql;

#from 1097 to 1098

SOURCE bans.sql;

ALTER TABLE accounts DROP COLUMN banned_until;
ALTER TABLE accounts ADD COLUMN last_login_hostname varchar(255) default NULL AFTER last_login_ip;

UPDATE server_options SET option_value=1098 where option_name='db_version';

#from 1098 to 1099

ALTER TABLE item_stats MODIFY COLUMN id bigint(20);
ALTER TABLE hunt_locations MODIFY COLUMN itemid bigint(20);
ALTER TABLE natural_resources MODIFY COLUMN item_id_reward bigint(20);
ALTER TABLE spell_glyphs MODIFY COLUMN item_id bigint(20);
ALTER TABLE loot_rule_details MODIFY COLUMN item_stat_id bigint(20);

DROP TABLE command_group_assignment;
DROP TABLE command_groups;
SOURCE command_access.sql;

UPDATE server_options SET option_value=1099 where option_name='db_version';

#from 1099 to 1100

ALTER TABLE accounts ADD COLUMN advisor_points int(10) NOT NULL default '0' AFTER spam_points;
ALTER TABLE characters DROP COLUMN advisor_points;

UPDATE server_options SET option_value=1100 where option_name='db_version';

# Does not require a db version dump.
drop table sys_modules;
drop table sys_privileges;
drop table sys_privileges_list;
drop table sys_programs;

#from 1100 to 1101

DROP TABLE command_group_assignment;
DROP TABLE command_groups;
SOURCE command_access.sql;

UPDATE server_options SET option_value=1101 where option_name='db_version';

#from 1101 to 1102

ALTER TABLE item_stats ADD COLUMN cstr_id_part_mesh int(10) NOT NULL default '0' AFTER cstr_id_part;
UPDATE server_options SET option_value=1102 where option_name='db_version';

#from 1102 to 1103

DROP TABLE command_group_assignment;
DROP TABLE command_groups;
SOURCE command_access.sql;

UPDATE characters SET mod_hitpoints = base_hitpoints_max WHERE (character_type = 1 OR character_type = 2) AND base_hitpoints_max > 0.0;

UPDATE server_options SET option_value=1103 where option_name='db_version';

#from 1103 to 1104

ALTER TABLE sectors ADD COLUMN rain_min_fade_in int(10) unsigned NOT NULL DEFAULT '0' AFTER rain_max_drops;
ALTER TABLE sectors ADD COLUMN rain_max_fade_in int(10) unsigned NOT NULL DEFAULT '0' AFTER rain_min_fade_in;
ALTER TABLE sectors ADD COLUMN rain_min_fade_out int(10) unsigned NOT NULL DEFAULT '0' AFTER rain_max_fade_in;
ALTER TABLE sectors ADD COLUMN rain_max_fade_out int(10) unsigned NOT NULL DEFAULT '0' AFTER rain_min_fade_out;

UPDATE server_options SET option_value=1104 where option_name='db_version';

#from 1104 to 1105

ALTER TABLE characters
  ADD COLUMN help_event_flags int(11) unsigned NULL DEFAULT 0 COMMENT 'Bit field of which instruction events have played for him.';
ALTER TABLE `tips`
  CHANGE COLUMN `id` `id` int(10) unsigned NULL auto_increment;
ALTER TABLE `tips`
  CHANGE COLUMN `tip` `tip` varchar(255) NOT NULL DEFAULT '';


INSERT INTO `tips` VALUES (1001,'Welcome to the world of Yliakum, a huge underground cave with many strange and wonderful sights.  Use your arrow keys to walk around and explore.');
INSERT INTO `tips` VALUES (1002,'You can move using the arrow keys, or strafe using WASD keys.  Holding Shift makes you run.  Movement takes your stamina, which means you get tired faster running than walking.  Next, find the merchant named Thorian and right-click him.');
INSERT INTO `tips` VALUES (1003,'When you right-click another player or npc in the game, that is called \"targeting.\"  You always have to target what you are focused on so that PlaneShift knows your intent when attacking, chatting or inspecting.');
INSERT INTO `tips` VALUES (1004,'To talk to any NPC, target him/her and talk normally in the chat window.  Say \'hello\' to the NPC now and see if he responds in your chat window.');
INSERT INTO `tips` VALUES (1005,'Congratulations, you now have a quest assigned.  To help find rats, try targeting the sign behind Thorian to get directions.');


UPDATE server_options SET option_value=1105 where option_name='db_version';

#from 1105 to 1106

ALTER TABLE sectors ADD COLUMN rain_enabled char(1) NOT NULL DEFAULT 'N' AFTER name;
UPDATE sectors SET rain_enabled="Y" where rain_max_duration != 0;

UPDATE server_options SET option_value=1106 where option_name='db_version';

#from 1106 to 1107

DROP TABLE command_group_assignment;
DROP TABLE command_groups;
SOURCE command_access.sql;

#from 1107 to 1108
ALTER TABLE characters DROP COLUMN familiar_id;
SOURCE char_create_affinity.sql;

UPDATE server_options SET option_value=1108 where option_name='db_version';

#from 1108 to 1109

ALTER TABLE characters CHANGE COLUMN `experience_points` `experience_points` int(10) unsigned default '0';
ALTER TABLE hunt_locations CHANGE COLUMN `itemid` `itemid` int(10) NOT NULL default '0';
ALTER TABLE item_instances CHANGE COLUMN id id int(10) unsigned NOT NULL auto_increment;
ALTER TABLE item_instances CHANGE COLUMN parent_item_id parent_item_id int(10) unsigned default '0';
ALTER TABLE item_stats CHANGE COLUMN `id` `id` int(10) unsigned NOT NULL auto_increment;
ALTER TABLE loot_rule_details CHANGE COLUMN item_stat_id item_stat_id int(10) unsigned NOT NULL DEFAULT '0';
ALTER TABLE natural_resources CHANGE COLUMN item_id_reward item_id_reward int(10) unsigned DEFAULT '0';
ALTER TABLE npc_spawn_rules CHANGE COLUMN min_spawn_time min_spawn_time int(10) unsigned NOT NULL DEFAULT '0';
ALTER TABLE npc_spawn_rules CHANGE COLUMN max_spawn_time max_spawn_time int(10) unsigned NOT NULL DEFAULT '0';
ALTER TABLE spell_glyphs CHANGE COLUMN item_id item_id int(10) unsigned NOT NULL DEFAULT '0';
ALTER TABLE trade_autocontainers CHANGE COLUMN item_instance_id item_instance_id int(10) unsigned DEFAULT '0';
ALTER TABLE trade_autocontainers CHANGE COLUMN garbage_item_id garbage_item_id int(10) unsigned DEFAULT '0';
ALTER TABLE trade_combinations CHANGE COLUMN result_id result_id int(10) unsigned NOT NULL;
ALTER TABLE trade_combinations CHANGE COLUMN item_id item_id int(10) unsigned NOT NULL;
ALTER TABLE trade_patterns CHANGE COLUMN designitem_id designitem_id int(10) unsigned;
ALTER TABLE trade_transformations CHANGE COLUMN result_id result_id int(10) unsigned NOT NULL;
ALTER TABLE trade_transformations CHANGE COLUMN item_id item_id int(10) unsigned NOT NULL;
ALTER TABLE trade_transformations CHANGE COLUMN workitem_id workitem_id int(10) unsigned NOT NULL;
ALTER TABLE trade_transformations CHANGE COLUMN equipment_id equipment_id int(10) unsigned;
ALTER TABLE trade_transformations CHANGE COLUMN garbage_id garbage_id int(10) unsigned;

UPDATE server_options SET option_value=1109 where option_name='db_version';

#from 1109 to 1110

### Create character_relationships table
CREATE TABLE `character_relationships`
(
    character_id int(11) NOT NULL default '0'           COMMENT 'character id from the characters table' ,
    related_id int(11) NOT NULL default '0'             COMMENT 'character id of the related character' ,
    relationship_type varchar(15) NOT NULL default ''   COMMENT 'one of three values currently `familiar`, `buddy`, or `spouse` ' ,
    spousename  varchar(30) default NULL                COMMENT 'used for marriages, historical data in case of deletion' ,

    PRIMARY KEY (`character_id`, `relationship_type`, `related_id` ),
    KEY ( `character_id` ),
    KEY (`related_id`, `relationship_type`)
);

### Copy Buddies
INSERT INTO character_relationships ( character_id, related_id, relationship_type )
SELECT player_id, player_buddy, 'buddy' FROM buddy_list;
DROP TABLE buddy_list;

### Copy Marriage Data
ALTER TABLE characters ADD COLUMN old_lastname VARCHAR(30) NOT NULL DEFAULT '' AFTER lastname;
UPDATE characters c, character_marriage_details cmd SET c.old_lastname = cmd.old_lastname
    WHERE cmd.character_id = c.id;

INSERT INTO character_relationships ( character_id, related_id, relationship_type, spousename )
    SELECT character_id, spouse.id, 'spouse', CONCAT( spouse.name, ' ', spouse.lastname )
    FROM character_marriage_details cmd, characters spouse
    WHERE STRCMP( cmd.spousename , CONCAT( spouse.name, ' ', spouse.lastname ) ) = 0;

DROP TABLE character_marriage_details;

### Copy Pet Data
INSERT INTO character_relationships ( character_id, related_id, relationship_type )
    SELECT owner_id, id, 'familiar'
    FROM characters
    WHERE owner_id <> 0;

ALTER TABLE characters DROP COLUMN owner_id;


UPDATE server_options SET option_value=1110 where option_name='db_version';

#from 1110 to 1111

source tribes.sql
source tribe_members.sql

UPDATE server_options SET option_value=1111 where option_name='db_version';

### 1111 to 1112 - Keith

#add item quality tracking columns
ALTER TABLE `item_stats` CHANGE COLUMN `item_quality` `item_max_quality` int(4) NULL DEFAULT 100;

ALTER TABLE `item_instances` DROP COLUMN `item_decay`;
ALTER TABLE `item_instances` ADD COLUMN `decay_resistance` float(3,2) NOT NULL DEFAULT 0.0 AFTER `loc_yrot`;

ALTER TABLE `item_stats` DROP COLUMN `decay_resistance`;
ALTER TABLE `item_stats` ADD COLUMN `decay_rate` float(3,2) NOT NULL DEFAULT 0.1 AFTER `flags`;

# Set all current items to defaults
update item_stats set decay_rate=0.1;
update item_stats set item_max_quality=50;
update item_instances set decay_resistance=0;
update item_instances set item_quality=50;

UPDATE server_options SET option_value=1112 where option_name='db_version';

### 1112 to 1113 - Luca

ALTER TABLE tips CHANGE COLUMN tip tip text NOT NULL default '';
UPDATE server_options SET option_value=1113 where option_name='db_version';

### 1113 to 1114 - Keith

ALTER TABLE `item_categories` ADD COLUMN `item_stat_id_repair_tool` int(11) NULL;
ALTER TABLE `item_categories` ADD COLUMN `is_repair_tool_consumed` char(1) NULL DEFAULT 'Y';
ALTER TABLE `item_categories` ADD COLUMN `skill_id_repair` int(11) NULL;
UPDATE server_options SET option_value=1114 where option_name='db_version';


### 1114 to 1115 - Keith
ALTER TABLE `npc_bad_text` ADD COLUMN `triggertext` varchar(255) NULL AFTER `badtext`;
UPDATE `server_options` SET `option_value`='1115' WHERE `option_name`='db_version';


### 1115 to 1116 - Keith
ALTER TABLE item_stats ADD COLUMN `illum_definition` text NULL COMMENT 'This is used for map/sketch xml';
UPDATE `server_options` SET `option_value`='1116' WHERE `option_name`='db_version';

### 1116 to 1117 - Dave
ALTER TABLE accounts DROP COLUMN last_login_hostname;
ALTER TABLE bans DROP COLUMN host_mask;
ALTER TABLE bans ADD COLUMN ip_range CHAR(15) default NULL AFTER account;
UPDATE `server_options` SET `option_value`='1117' WHERE `option_name`='db_version';

### 1117 to 1118 - Dave
DROP TABLE command_group_assignment;
DROP TABLE command_groups;
SOURCE command_access.sql;
UPDATE `server_options` SET `option_value`='1118' WHERE `option_name`='db_version';

### 1118 to 1119 - Dave
DROP TABLE movement_base;
DROP TABLE movement_extras;
DROP TABLE movement_require;
DROP TABLE movement_variant;
SOURCE movement.sql;
UPDATE `server_options` SET `option_value`='1119' WHERE `option_name`='db_version';


### 1119 to 1120 - Acraig,  adding render_effect column to trades
ALTER TABLE trade_transformations ADD COLUMN render_effect CHAR(32) default NULL AFTER animation;
UPDATE `server_options` SET `option_value`='1120' WHERE `option_name`='db_version';

### 1120 to 1121 - Michael - Added familiar types to complete familiar affinity
CREATE TABLE `familiar_types`
(
    `id` INT UNSIGNED NOT NULL ,
    `name` VARCHAR(30) NOT NULL,
    `type` VARCHAR(30) NOT NULL,
    `lifecycle` VARCHAR(30) NOT NULL,
    `attacktool` VARCHAR(30) NOT NULL,
    `attacktype` VARCHAR(30) NOT NULL,
    `magicalaffinity` VARCHAR(30) NOT NULL,
    `vision` INT NULL,   /* 1 = enhanced, 0 = normal, -1 = limited */
    `speed` INT NULL,    /* 1 = enhanced, 0 = normal, -1 = limited */
    `hearing` INT NULL,  /* 1 = enhanced, 0 = normal, -1 = limited */
    PRIMARY KEY (`id`)
) ENGINE=MyISAM ;
UPDATE `server_options` SET `option_value`='1121' WHERE `option_name`='db_version';

### 1121 to 1122 - Luca - changed natural res to use id of sector
alter table natural_resources drop column sector;
alter table natural_resources ADD COLUMN loc_sector_id int(10) unsigned NOT NULL default '0' AFTER id;
UPDATE `server_options` SET `option_value`='1122' WHERE `option_name`='db_version';

### 1122 to 1123 - Magodra - Added table for waypoints used in super clients (NPC Client).
source sc_waypoints.sql;
source sc_waypoint_links.sql;
UPDATE `server_options` SET `option_value`='1123' WHERE `option_name`='db_version';

### 1123 to 1124 - Magodra - Added tribe home
DROP table tribes; # Since tribes is not deployed yet, assumes that there are now data.
source tribes.sql;
UPDATE `server_options` SET `option_value`='1124' WHERE `option_name`='db_version';

### 1124 to 1125 - Luca - added armor_id on the race definition to manage natural armor
alter table race_info add column `armor_id` int(10) default '0';
UPDATE `server_options` SET `option_value`='1125' WHERE `option_name`='db_version';

### 1125 to 1126 - acraig - Added helm column to races to have a helm group
ALTER TABLE race_info ADD  COLUMN `helm` varchar(16) default '';
UPDATE race_info SET helm='ylianm' WHERE race_id=13 OR race_id=11;
UPDATE server_options SET option_value=1126 where option_name='db_version';

### 1126 to 1127 - Magodra - Added location table
source sc_location_type.sql;
source sc_locations.sql;
UPDATE `server_options` SET `option_value`='1127' WHERE `option_name`='db_version';

alter table item_instances add column `crafted_quality` float(5,2) default '-1';
ALTER TABLE item_instances MODIFY COLUMN crafted_quality float(5,2) default '-1' AFTER item_quality;
UPDATE `server_options` SET `option_value`='1128' WHERE `option_name`='db_version';

### 1128 to 1129 - Magodra - Added location table
ALTER TABLE tribes add column max_size int(10) signed DEFAULT '0' AFTER home_radius;
ALTER TABLE tribes add column wealth_resource_name varchar(30) NOT NULL default '' AFTER max_size;
ALTER TABLE tribes add column wealth_resource_area varchar(30) NOT NULL default '' AFTER wealth_resource_name;
ALTER TABLE tribes add column reproduction_cost int(10) signed DEFAULT '0' AFTER wealth_resource_area;
UPDATE `server_options` SET `option_value`='1129' WHERE `option_name`='db_version';


### 1129 to 1130 - TomT - Added TradeProcess table and split up TradeTransforms
DROP TABLE trade_transformations;
CREATE TABLE trade_transformations (
  id int(10) unsigned NOT NULL auto_increment,
  pattern_id int(10) unsigned NOT NULL,                         # pattern for transformation
  process_id int(10) unsigned NOT NULL,                         # process for transformation
  result_id int(10) unsigned NOT NULL,                      # resulting item
  result_qty int(8) unsigned NOT NULL ,                         # resulting item quantity
  item_id int(10) unsigned NOT NULL,                            # item to be transformed
  item_qty int(8) unsigned NOT NULL ,                           # required quantity for transformation
  trans_points int(8) unsigned NOT NULL DEFAULT '0' ,           # ammount of time to complete transformation
  penilty_pct float(10,6) NOT NULL DEFAULT '1.000000' ,     # percent of quality for resulting item
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);
CREATE TABLE trade_processes (
  process_id int(10) unsigned NOT NULL auto_increment,
  animation varchar(30) ,                                       # transformation animation
  render_effect CHAR(32) default NULL,                          # transformation render effect
  workitem_id int(10) unsigned NOT NULL,                        # target item to complete transformation
  equipment_id int(10) unsigned  ,                              # required equipted item
  constraints varchar(64) NOT NULL DEFAULT '',                  # constraints that apply to transformation
  garbage_id int(10) unsigned ,                                 # garbage item for flubbed transformations
  garbage_qty int(8) unsigned ,                                 # garbage quantity for flubbed transformations
  primary_skill_id int(10) ,                                    # primary skill for transformation
  primary_min_skill int(8) unsigned ,                           # minimum primary skill level required to perform transformation, 0 is no minimum
  primary_max_skill int(8) unsigned ,                           # maximum primary skill level at which practice and quality are effected, 0 is no maximum
  primary_practice_points int(4) ,                              # number of practice primary skill points gained for performing transformation
  primary_quality_factor int(3) unsigned DEFAULT '0' ,          # percentage of the primary skill range that applies to quality
  secondary_skill_id int(10) unsigned ,                         # secondary skill foriegn key
  secondary_min_skill int(8) unsigned ,                         # minimum secondary skill level required to perform transformation, 0 is no minimum
  secondary_max_skill int(8) unsigned ,                         # maximum secondary skill level at which practice and quality are effected, 0 is no maximum
  secondary_practice_points int(4) ,                            # number of practice secondary skill points gained for performing transformation
  secondary_quality_factor int(3) unsigned ,                    # percentage of the secondary skill range that applies to quality
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (process_id)
);
UPDATE `server_options` SET `option_value`='1130' WHERE `option_name`='db_version';

### 1129 to 1130 - TomT - Added name column to TradeProcess table
###   No need to alter table because data has not been loaded yet
DROP TABLE trade_processes;
CREATE TABLE trade_processes (
  process_id int(10) unsigned NOT NULL auto_increment,
  name varchar(40) default NULL,                # process name
  animation varchar(30) ,                                       # transformation animation
  render_effect CHAR(32) default NULL,                          # transformation render effect
  workitem_id int(10) unsigned NOT NULL,                        # target item to complete transformation
  equipment_id int(10) unsigned  ,                              # required equipted item
  constraints varchar(64) NOT NULL DEFAULT '',                  # constraints that apply to transformation
  garbage_id int(10) unsigned ,                                 # garbage item for flubbed transformations
  garbage_qty int(8) unsigned ,                                 # garbage quantity for flubbed transformations
  primary_skill_id int(10) ,                                    # primary skill for transformation
  primary_min_skill int(8) unsigned ,                           # minimum primary skill level required to perform transformation, 0 is no minimum
  primary_max_skill int(8) unsigned ,                           # maximum primary skill level at which practice and quality are effected, 0 is no maximum
  primary_practice_points int(4) ,                              # number of practice primary skill points gained for performing transformation
  primary_quality_factor int(3) unsigned DEFAULT '0' ,          # percentage of the primary skill range that applies to quality
  secondary_skill_id int(10) unsigned ,                         # secondary skill foriegn key
  secondary_min_skill int(8) unsigned ,                         # minimum secondary skill level required to perform transformation, 0 is no minimum
  secondary_max_skill int(8) unsigned ,                         # maximum secondary skill level at which practice and quality are effected, 0 is no maximum
  secondary_practice_points int(4) ,                            # number of practice secondary skill points gained for performing transformation
  secondary_quality_factor int(3) unsigned ,                    # percentage of the secondary skill range that applies to quality
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (process_id)
);
UPDATE `server_options` SET `option_value`='1131' WHERE `option_name`='db_version';

### 1131 to 1132 - Magodra - Added angle
ALTER TABLE sc_waypoints change `flags` `flags` varchar(30) DEFAULT '';
ALTER TABLE sc_waypoint_links change `flags` `flags` varchar(30) DEFAULT '';
ALTER TABLE sc_locations change `flags` `flags` varchar(30) DEFAULT '';
UPDATE `server_options` SET `option_value`='1132' WHERE `option_name`='db_version';
#
# At npc client run: importnpcdefs /this/data/npcdefs.xml
#
### 1132 to 1133 - Borrillis - moved spells.casting_duration to MathScripts
ALTER TABLE spells DROP COLUMN casting_duration;
UPDATE `server_options` SET `option_value`='1133' WHERE `option_name`='db_version';

### 1133 to 1134 - Talad - increase name field size
ALTER TABLE item_stats modify `name` varchar(60) DEFAULT NULL;
UPDATE `server_options` SET `option_value`='1134' WHERE `option_name`='db_version';




### 1134 to 1135 - Keith Fulton - Add instances to characters and items.

ALTER TABLE `characters`
  ADD COLUMN `loc_instance` int(11) NULL DEFAULT 0 AFTER `bank_money_trias`;
ALTER TABLE `item_instances`
  ADD COLUMN `loc_instance` int(11) NULL DEFAULT 0 AFTER `guild_mark_id`;
UPDATE `server_options` SET `option_value`='1135' WHERE `option_name`='db_version';

### 1135 to 1136 - Keith Fulton - Add instances to characters and items.

ALTER TABLE item_stats
  CHANGE COLUMN `illum_definition` `sketch_definition` text NULL COMMENT 'This is used for map/sketch xml';
UPDATE `server_options` SET `option_value`='1136' WHERE `option_name`='db_version';

### 1136 to 1137 - Dave Bentham - Add GM-Events tables.

DROP TABLE character_events;
DROP TABLE gm_events;
SOURCE gm_events.sql;
SOURCE character_events.sql;
INSERT INTO command_group_assignment VALUES( "/event", 30 );
INSERT INTO command_group_assignment VALUES( "/event", 26 );
INSERT INTO command_group_assignment VALUES( "/event", 25 );
INSERT INTO command_group_assignment VALUES( "/event", 24 );
UPDATE `server_options` SET `option_value`='1137' WHERE `option_name`='db_version';

### 1137 to 1138 - Tom Towey - Add active indicator to action locations.
ALTER TABLE `action_locations`
  ADD COLUMN `active_ind`  char(1) NOT NULL default 'Y' AFTER `response`;
UPDATE `action_locations` SET `active_ind`='Y';
UPDATE `server_options` SET `option_value`='1138' WHERE `option_name`='db_version';

### 1138 to 1139 - Anders Reggestad - Infinite player lockout on quests
UPDATE quests SET player_lockout_time=-1 WHERE player_lockout_time=2147483647;
UPDATE quests SET player_lockout_time=-1 WHERE player_lockout_time=2147483;

UPDATE `server_options` SET `option_value`='1139' WHERE `option_name`='db_version';

### 1139 to 1141 - Anders Reggestad - Trigger/Response update
ALTER TABLE `npc_responses`
  ADD COLUMN `prerequisite` blob AFTER `script`;
ALTER TABLE `npc_responses`
  ADD COLUMN `trigger_id` int(10) unsigned default '0' AFTER `id`;
UPDATE npc_responses r, npc_triggers t SET r.trigger_id=t.id WHERE t.response_id=r.id;
#Check if you will delete something
SELECT id FROM npc_responses where trigger_id=0;

# Delete any respose that do not have a trigger
DELETE FROM npc_responses where trigger_id=0;
# Should check if any trigger point to multiple responses. Did not figure
# out the sql for this. Though to display a table with count of dublicates run
# the following query and check everywhere count is diffrent from 1.
SELECT response_id,count(response_id) AS val from npc_triggers group by response_id;
# Remove not used colums
ALTER TABLE npc_triggers DROP COLUMN response_id;
ALTER TABLE npc_triggers DROP COLUMN min_attitude_required;
ALTER TABLE npc_triggers DROP COLUMN max_attitude_required;

# Keith - limitations for sketches
SOURCE character_limitations.sql;

UPDATE `server_options` SET `option_value`='1141' WHERE `option_name`='db_version';

### 1141 to 1142 - Anders Reggestad - Trigger reserved word in new databaes
ALTER TABLE npc_triggers CHANGE COLUMN trigger trigger_text varchar(255) default NULL;

UPDATE `server_options` SET `option_value`='1142' WHERE `option_name`='db_version';


#
# Needs to be done at server at some point. Include in next update - Anders Reggestad
#
INSERT INTO command_group_assignment VALUES( "/waypoint", 30 );
INSERT INTO command_group_assignment VALUES( "/action", 30 );


# Item inventory updates -- THIS IS NOT COMPLETE YET AND WILL NOT PRESERVE DATA YET
ALTER TABLE `item_instances`
  CHANGE COLUMN `location_in_parent` `location_in_parent` smallint(4) NULL
  DEFAULT 0 COMMENT 'Slot number in inventory, container or -1 in world.';

### 1142 to 1144 - Daniel Fryer - Books keeping text in new unique_content table
SOURCE unique_content.sql;

#First get the content from the descriptions (where they used to be stored..)
INSERT INTO unique_content (content) SELECT description FROM item_stats WHERE flags like "%readable%";

#Then update the item_instances so that their content key is pointing at the same text that was just copied from their description
UPDATE item_instances inst SET item_stats_id_unique = (SELECT id from unique_content WHERE content = (select description from item_stats where item_stats.id = inst.item_stats_id_standard)) WHERE (SELECT flags FROM item_stats WHERE item_stats.id = inst.item_stats_id_standard) like "%readable%";

UPDATE `server_options` SET `option_value`='1143' WHERE `option_name`='db_version';


### 1144 to 1145 - Kenny Graunke - Simplify weapon and armor in item_stats

# First remove unused columns...
ALTER TABLE item_stats DROP COLUMN weapon_dmg_force;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_fire;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_ice;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_air;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_poison;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_disease;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_holy;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_unholy;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_force;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_fire;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_ice;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_air;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_poison;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_disease;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_holy;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_unholy;
ALTER TABLE item_stats DROP COLUMN armor_prot_force;
ALTER TABLE item_stats DROP COLUMN armor_prot_fire;
ALTER TABLE item_stats DROP COLUMN armor_prot_ice;
ALTER TABLE item_stats DROP COLUMN armor_prot_air;
ALTER TABLE item_stats DROP COLUMN armor_prot_poison;
ALTER TABLE item_stats DROP COLUMN armor_prot_disease;
ALTER TABLE item_stats DROP COLUMN armor_prot_holy;
ALTER TABLE item_stats DROP COLUMN armor_prot_unholy;

# Merge data from weapon_dmg_X and armor_prot_X into single dmg_X column
# Create new columns...
ALTER TABLE item_stats ADD COLUMN `dmg_slash` float(10,2) default '0.00' AFTER `item_bonus_3_max`;
ALTER TABLE item_stats ADD COLUMN `dmg_blunt` float(10,2) default '0.00' AFTER `dmg_slash`;
ALTER TABLE item_stats ADD COLUMN `dmg_pierce` float(10,2) default '0.00' AFTER `dmg_blunt`;
# Merge the two data sources...
UPDATE `item_stats` SET `dmg_slash`=IF(weapon_dmg_slash > armor_prot_slash, weapon_dmg_slash, armor_prot_slash);
UPDATE `item_stats` SET `dmg_blunt`=IF(weapon_dmg_blunt > armor_prot_blunt, weapon_dmg_blunt, armor_prot_blunt);
UPDATE `item_stats` SET `dmg_pierce`=IF(weapon_dmg_pierce > armor_prot_pierce, weapon_dmg_pierce, armor_prot_pierce);
# Delete old columns...
ALTER TABLE item_stats DROP COLUMN weapon_dmg_slash;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_blunt;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pierce;
ALTER TABLE item_stats DROP COLUMN armor_prot_slash;
ALTER TABLE item_stats DROP COLUMN armor_prot_blunt;
ALTER TABLE item_stats DROP COLUMN armor_prot_pierce;

UPDATE `server_options` SET `option_value`='1145' WHERE `option_name`='db_version';

### 1145 to 1146 - Kenny Graunke - Remove more unusued columns
# According to Talad, these all had 0 in them and thus were essentially unused.
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_slash;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_blunt;
ALTER TABLE item_stats DROP COLUMN weapon_dmg_pct_pierce;

UPDATE `server_options` SET `option_value`='1146' WHERE `option_name`='db_version';

### 1146 to 1147 - Anders Reggestad - Added tribe memory.
source sc_tribe_memories.sql;
UPDATE `server_options` SET `option_value`='1147' WHERE `option_name`='db_version';

### 1147 to 1148 - Anders Reggestad - Added tribe memory.
source sc_tribe_resources.sql;
ALTER TABLE tribes ADD COLUMN wealth_resource_nick varchar(30) NOT NULL default '' AFTER wealth_resource_name;
update tribes set wealth_resource_nick="iron" where wealth_resource_name="Iron Ore";
update tribes set wealth_resource_nick="gold" where wealth_resource_name="Gold Ore";
UPDATE `server_options` SET `option_value`='1148' WHERE `option_name`='db_version';

# inventory migration

# slot 16 is the first bulk slot, equipped slots start from 0
update item_instances set location_in_parent=location_in_parent+16 where location='I' and char_id_owner is not null;

#update equipped items ( no update needed for containers )
update item_instances set location_in_parent=0 where location='E' and equipped_slot='righthand' and char_id_owner is not null;
update item_instances set location_in_parent=1 where location='E' and equipped_slot='lefthand' and char_id_owner is not null;
update item_instances set location_in_parent=3 where location='E' and equipped_slot='leftfinger' and char_id_owner is not null;
update item_instances set location_in_parent=4 where location='E' and equipped_slot='rightfinger' and char_id_owner is not null;
update item_instances set location_in_parent=5 where location='E' and equipped_slot='head' and char_id_owner is not null;
update item_instances set location_in_parent=6 where location='E' and equipped_slot='neck' and char_id_owner is not null;
update item_instances set location_in_parent=7 where location='E' and equipped_slot='back' and char_id_owner is not null;
update item_instances set location_in_parent=8 where location='E' and equipped_slot='arms' and char_id_owner is not null;
update item_instances set location_in_parent=9 where location='E' and equipped_slot='gloves' and char_id_owner is not null;
update item_instances set location_in_parent=10 where location='E' and equipped_slot='boots' and char_id_owner is not null;
update item_instances set location_in_parent=11 where location='E' and equipped_slot='legs' and char_id_owner is not null;
update item_instances set location_in_parent=12 where location='E' and equipped_slot='belt' and char_id_owner is not null;
update item_instances set location_in_parent=13 where location='E' and equipped_slot='bracers' and char_id_owner is not null;
update item_instances set location_in_parent=14 where location='E' and equipped_slot='torso' and char_id_owner is not null;
update item_instances set location_in_parent=15 where location='E' and equipped_slot='mind' and char_id_owner is not null;

#update containers owned by players, which have location_in_parent 16-based instead of 0-based
delete from temp;
insert into temp
(select i.id from item_instances i, item_stats s where i.item_stats_id_standard=s.id and char_id_owner!=0 and container_max_size>0 and item_stats_id_standard<300);

update item_instances set location_in_parent=
location_in_parent+16 where parent_item_id IN (
select id from temp);

alter table item_instances drop column location;
alter table item_instances drop column equipped_slot;

### 1148 to 1149 - Luca Pancallo - Added gender, birth to accounts.

ALTER TABLE accounts ADD COLUMN gender varchar(1) default 'N' AFTER country;
ALTER TABLE accounts ADD COLUMN birth int(4) default '0' AFTER gender;

### 1149 to 1150 - Kenny Graunke - Added char_id_guardian to item_instances.
ALTER TABLE item_instances ADD COLUMN char_id_guardian int(10) unsigned default '0' AFTER char_id_owner;
UPDATE `server_options` SET `option_value`='1150' WHERE `option_name`='db_version';

# Fix for traits
alter table character_traits modify trait_id int(10) unsigned NOT NULL DEFAULT '0';

### 1150 - Andrew Dai - Added creation timestamp to characters table.

ALTER TABLE characters
  ADD COLUMN creation_time timestamp default current_timestamp;
#reset newly added column to 0
update characters set creation_time=0;

### 1151 - Dave Bentham - "creative content" method of holding book & sketch data
# There is set of data conversion scripts (mysql & awk) to convert item_stats table
# in situ, if recreating item_stats from source is not desired. It is available
# from Acissej and Talad.
ALTER TABLE `item_stats`
  CHANGE COLUMN `sketch_definition` `creative_definition` text NULL COMMENT 'This is used for creative things xml, such as sketches/maps, books';
# otherwise, recreate item_stats from scratch:
DROP TABLE item_stats;
SOURCE item_stats.sql;

# DB bump
UPDATE `server_options` SET `option_value`='1151' WHERE `option_name`='db_version';

### 1152 - Kenny Graunke - remove duel points
ALTER TABLE characters DROP COLUMN duel_points;
UPDATE server_options SET option_value='1152' WHERE option_name='db_version';

### 1153 - Sasha Levin - Remove hardcoded race info
ALTER TABLE `race_info` CHANGE COLUMN `race_id` `id` INT(10) NOT NULL DEFAULT 0,
 ADD COLUMN `race` INT(5) UNSIGNED NOT NULL AFTER `helm`,
 DROP PRIMARY KEY,
 ADD PRIMARY KEY  USING BTREE(`id`);

#### 1154 - Michael Cummings - Moved RPGRules.xml into math_scripts DB table
SOURCE math_scripts.sql

#### 1155 - Michael Gist - Added stances table.
SOURCE stances.sql
UPDATE `server_options` SET `option_value`='1155' WHERE `option_name`='db_version';

#### 1156 - Kenny Graunke - Separate equip and consume actions
ALTER TABLE item_stats ADD COLUMN prg_evt_consume text AFTER prg_evt_unequip;
UPDATE item_stats SET prg_evt_consume=prg_evt_equip WHERE flags LIKE '%CONSUMABLE%';
UPDATE item_stats SET prg_evt_equip='' WHERE flags LIKE '%CONSUMABLE%';
UPDATE `server_options` SET `option_value`='1156' WHERE `option_name`='db_version';


### 1157 - Michael Gist - Added banker flag to characters table. Added bank money values for guilds.
ALTER TABLE characters ADD COLUMN banker tinyint(1) unsigned NOT NULL default '0';
ALTER TABLE guilds ADD COLUMN bank_money_circles int(10) unsigned NOT NULL default '0';
ALTER TABLE guilds ADD COLUMN bank_money_octas int(10) unsigned NOT NULL default '0';
ALTER TABLE guilds ADD COLUMN bank_money_hexas int(10) unsigned NOT NULL default '0';
ALTER TABLE guilds ADD COLUMN bank_money_trias int(10) unsigned NOT NULL default '0';
UPDATE `server_options` SET `option_value`='1157' WHERE `option_name`='db_version';

### 1158 - Dave Bentham - item_stats.name now unique key.
ALTER TABLE `item_stats` ADD UNIQUE KEY (`name`);
UPDATE `server_options` SET `option_value`='1158' WHERE `option_name`='db_version';

### 1159 - Michael Gist - Added money events table.
SOURCE money_events.sql
UPDATE `server_options` SET `option_value`='1159' WHERE `option_name`='db_version';

#### 1160 - Anders Reggestad - Modified waypoint system to use predefined paths
ALTER TABLE sc_waypoint_links ADD COLUMN name VARCHAR(30) NOT NULL DEFAULT '' AFTER id;
ALTER TABLE sc_waypoint_links ADD COLUMN type VARCHAR(30) NOT NULL DEFAULT '' AFTER name;
INSERT INTO command_group_assignment VALUES( "/path", 30 );
INSERT INTO command_group_assignment VALUES( "/location", 30 );
UPDATE sc_waypoint_links SET type='LINEAR';
SOURCE sc_path_points.sql
UPDATE `server_options` SET `option_value`='1160' WHERE `option_name`='db_version';

### 1161 - Sasha Levin - Added required action info in natural resources
ALTER TABLE `natural_resources` ADD COLUMN `action` VARCHAR(45) NOT NULL AFTER `reward_nickname`;
UPDATE `server_options` SET `option_value`='1161' WHERE `option_name`='db_version';

#### 1162 - Anders Reggestad - Add order of points in paths.
ALTER TABLE sc_path_points ADD COLUMN  `prev_point` int(8) unsigned NOT NULL default '0' AFTER path_id;
update sc_path_points set prev_point=id-1; # Start points has to be set to 0.
UPDATE `server_options` SET `option_value`='1162' WHERE `option_name`='db_version';

#### 1163 - Thomas Towey - Removed autoincr on id column and added subprocess number column
ALTER TABLE trade_processes MODIFY COLUMN process_id int(10) unsigned NOT NULL,
  ADD COLUMN subprocess_number int(4) unsigned NOT NULL default '0' AFTER process_id,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (process_id,subprocess_number);
UPDATE `server_options` SET `option_value`='1163' WHERE `option_name`='db_version';

#### 1164 - Michael Gist - Update money events for more auto-tax stuff.
ALTER TABLE money_events ADD COLUMN prg_evt_paid text;
ALTER TABLE money_events ADD COLUMN prg_evt_npaid text;
ALTER TABLE money_events ADD COLUMN prg_evt_fnpaid text;
ALTER TABLE money_events ADD COLUMN latePayment bool;
ALTER TABLE money_events ADD COLUMN lateBy int(3) NOT NULL;
ALTER TABLE money_events ADD COLUMN instance int(11) default '0';
UPDATE `server_options` SET `option_value`='1164' WHERE `option_name`='db_version';

#### Add weather command.
INSERT INTO command_group_assignment VALUES( "/weather", 30 );
INSERT INTO command_group_assignment VALUES( "/weather", 26 );
INSERT INTO command_group_assignment VALUES( "/weather", 25 );

#### Add command to alter the label colour.
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 30 );
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 26 );
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 25 );
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 24 );
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 23 );
INSERT INTO command_group_assignment VALUES( "/setlabelcolor", 22 );

#### 1165 - Sasha Levin - Fix by Lanarel to quests
ALTER TABLE `character_quests` MODIFY COLUMN `remaininglockout` INT(10) UNSIGNED DEFAULT 0,
 ADD COLUMN `last_response` INT(10) DEFAULT '-1' AFTER `remaininglockout`;

#### 1166 - Sasha Levin - Added multiple spawn points support
CREATE TABLE `race_spawns` (
  `raceid` INTEGER,
  `x` FLOAT,
  `y` FLOAT,
  `z` FLOAT,
  `yrot` FLOAT,
  `sector_id` INTEGER,
  PRIMARY KEY (`raceid`, `x`, `y`, `z`, `yrot`, `sector_id`)
)

# moves start location data from old table to new table
INSERT INTO race_spawns (select id,start_x,start_y,start_z,start_yrot,start_sector_id from race_info);

ALTER TABLE `race_info` DROP COLUMN `start_x`,
 DROP COLUMN `start_y`,
 DROP COLUMN `start_z`,
 DROP COLUMN `start_sector_id`,
 DROP COLUMN `start_yrot`;

#### 1167 - Sasha Levin - Added custom item name support
ALTER TABLE `item_instances` ADD COLUMN `item_name` VARCHAR(100) DEFAULT '' AFTER `openable_locks`,
 ADD COLUMN `item_description` VARCHAR(100) DEFAULT '' AFTER `item_name`;

#### 1168 - Sasha Levin - Add introduction manager
CREATE TABLE `introductions` (
  `charid` int(10) unsigned NOT NULL,
  `introcharid` int(10) unsigned NOT NULL,
  PRIMARY KEY  (`charid`,`introcharid`)
) TYPE=MyISAM;

#### 1169 - Sasha Levin - Add support for collideable sectors.
ALTER TABLE `sectors` ADD COLUMN `collide_objects` BOOLEAN NOT NULL DEFAULT 0 AFTER `lightning_max_gap`;

UPDATE `server_options` SET `option_value`='1169' WHERE `option_name`='db_version';

#### 1170 - Sasha Levin - Add logging of warnings given to accounts.
CREATE TABLE `warnings` (
  `accountid` INTEGER UNSIGNED NOT NULL,
  `warningGM` VARCHAR(45) NOT NULL,
  `timeOfWarn` DATETIME NOT NULL,
  `warnMessage` TEXT NOT NULL,
  PRIMARY KEY (`accountid`, `warningGM`, `timeOfWarn`)
);

#### 1171 - Anders Reggestad - Added disabled flag for NPCs, default to No.
ALTER TABLE `sc_npc_definitions` ADD COLUMN disabled char(1) DEFAULT 'N';
UPDATE `server_options` SET `option_value`='1171' WHERE `option_name`='db_version';

#### 1172 - Anders Reggestad - Extedned flags part of waypoints.
ALTER TABLE sc_waypoints MODIFY COLUMN `flags` varchar(100) DEFAULT '';
UPDATE `server_options` SET `option_value`='1172' WHERE `option_name`='db_version';

#### 1173 - Anders Reggestad - Added aliases for waypoints.
source sc_waypoint_aliases.sql;
delete from command_group_assignment where command_name="/waypoint";
UPDATE `server_options` SET `option_value`='1173' WHERE `option_name`='db_version';

#### 1174 - Sasha Levin - Added 'ruling god' to each sector
ALTER TABLE `sectors` ADD COLUMN `god_name` VARCHAR(45) NOT NULL DEFAULT 'Laanx' AFTER `collide_objects`;

#### 1175 - Anders Reggestad - Added uniq name to waypoints and type field to allow groups of waypoints with same type.
alter table sc_waypoints add unique name (name);
alter table sc_waypoints add column `wp_group` varchar(30) NOT NULL DEFAULT '' after name;
UPDATE `server_options` SET `option_value`='1175' WHERE `option_name`='db_version';

#### 1176 - Anders Reggestad - Added charges to items
ALTER TABLE item_stats ADD COLUMN `max_charges` int(3) NOT NULL default '-1' AFTER creative_definition;
ALTER TABLE item_instances ADD COLUMN `charges` int(3) NOT NULL default '0' AFTER item_description;
UPDATE `server_options` SET `option_value`='1176' WHERE `option_name`='db_version';

#### 1177 - Sasha Levin - Added support for ranged weapons
ALTER TABLE item_stats ADD COLUMN `weapon_range` FLOAT UNSIGNED NOT NULL DEFAULT 2 AFTER `max_charges`;
UPDATE `server_options` SET `option_value`='1177' WHERE `option_name`='db_version';

#### 1178 - Anders Reggestad - Make sure NPCs based on master npc id point to the real master.
# Does not change the DB but is needed with the correspoinding code change.
UPDATE `server_options` SET `option_value`='1178' WHERE `option_name`='db_version';
# Used to move npc_master_id from a child of a npc_master to the master itself. To be applyed
# multiple times until number of changed records are null.
update characters c1, characters c2, characters c3 set c3.npc_master_id=c1.id where  (c1.npc_master_id=c1.id || c1.npc_master_id=0) && c2.npc_master_id=c1.id && c2.id != c2.npc_master_id && c3.npc_master_id=c2.id;
# List the NPCs that does not point to a true master.
# select c1.id,c1.name,c1.npc_master_id,c2.id,c2.name,c2.npc_master_id,c3.id,c3.name,c3.npc_master_id from characters c1, characters c2, characters c3 where (c1.npc_master_id=c1.id || c1.npc_master_id=0) && c2.npc_master_id=c1.id && c2.id != c2.npc_master_id && c3.npc_master_id=c2.id;

#### 1179 - Sasha Levin
ALTER TABLE item_stats MODIFY COLUMN `item_type_id_ammo` VARCHAR(30) DEFAULT '';

INSERT INTO math_scripts VALUES( "CalculateMaxPetRange", "MaxRange = 10 + Skill*10;" );
INSERT INTO math_scripts VALUES( "CalculatePetReact"," React = if(1+Skill>=Level,1,0);");

UPDATE `server_options` SET `option_value`='1179' WHERE `option_name`='db_version';

#### 1180 - Sasha Levin - Added instance support to npc respawn rules
ALTER TABLE npc_spawn_rules ADD COLUMN fixed_spawn_instance INTEGER UNSIGNED NOT NULL DEFAULT 0 AFTER `fixed_spawn_sector`;
UPDATE `server_options` SET `option_value`='1180' WHERE `option_name`='db_version';

#### 1181 - Sasha Levin - Added persistency to guild wars.
CREATE TABLE guild_wars (
  `guild_a` INTEGER UNSIGNED NOT NULL,
  `guild_b` INTEGER UNSIGNED NOT NULL,
  PRIMARY KEY (`guild_a`, `guild_b`)
)
ENGINE = MyISAM;
source guild_wars.sql;
UPDATE `server_options` SET `option_value`='1181' WHERE `option_name`='db_version';

#### 1182 - Roland Schulz - switch loc_instance in item_instances to unsigned
update item_instances set loc_instance=0x7fffffff where loc_instance=-1;
ALTER TABLE item_instances MODIFY COLUMN `loc_instance` int(11) unsigned default '0';
update item_instances set loc_instance=0xffffffff where loc_instance=0x7fffffff;

alter table item_instances modify   `item_description` varchar(255) default '';

UPDATE `server_options` SET `option_value`='1182' WHERE `option_name`='db_version';

#### 1183 - Thomas Towey - added identify skill and minimum skill level for item categories
ALTER TABLE item_categories ADD COLUMN identify_skill_id int(10);
ALTER TABLE item_categories ADD COLUMN identify_min_skill int(8) unsigned;
UPDATE `server_options` SET `option_value`='1183' WHERE `option_name`='db_version';

#### 1184 - Frank Barton - Added wc_accessrules table in prep for updated WC
CREATE TABLE `wc_accessrules` (`id` int(10) NOT NULL auto_increment, `security_level` tinyint(3) NOT NULL, `objecttype` varchar(50) NOT NULL, `access` tinyint(3) default NULL, UNIQUE KEY `id` (`id`));
UPDATE `server_options` SET `option_value`='1184' WHERE `option_name`='db_version';

#### 1185 - Frank Barton - Added wc_cmdlog table in prep for updated WC
CREATE TABLE `wc_cmdlog` (`id` int(10) NOT NULL auto_increment, `username` varchar(100) NOT NULL, `query` text , `date` datetime , UNIQUE KEY `id` (`id`));
UPDATE `server_options` SET `option_value`='1185' WHERE `option_name`='db_version';

#### 1186 - Steven Patrick - Adding range and amount for /crystal
ALTER TABLE `hunt_locations` ADD COLUMN `amount` INTEGER UNSIGNED NOT NULL DEFAULT 1 AFTER `sector`,
 ADD COLUMN `range` DOUBLE(10,2) UNSIGNED NOT NULL DEFAULT '0.00' AFTER `amount`;
UPDATE `server_options` SET `option_value`='1186' WHERE `option_name`='db_version';

#### 1187 - Frank Barton - changing stat_type to appropriate value for random loot an unique items
UPDATE `item_stats` SET `stat_type`='R' WHERE `id`>'10000';
UPDATE `item_stats` SET `stat_type`='U' WHERE `stat_type`='R' AND `flags` LIKE '%PERSONALISE%';
UPDATE `server_options` SET `option_value`='1187' WHERE `option_name`='db_version';

#### 1188 - Steven Patrick - Adding advisor_ban field
ALTER TABLE `accounts` ADD COLUMN `advisor_ban` tinyint(1) DEFAULT 0 AFTER `realname`;
UPDATE `server_options` SET `option_value`='1188' WHERE `option_name`='db_version';

#### 1189 - Steven Patrick - Expanding length of guild MOTD, FS#1572
ALTER TABLE `guilds` MODIFY COLUMN `motd` CHAR(200);
UPDATE `server_options` SET `option_value`='1189' WHERE `option_name`='db_version';

#### 1190 - Zwenze - Adding description to factions table, FS#1699
ALTER TABLE `factions` ADD COLUMN `faction_description` text AFTER `faction_name`;
UPDATE `server_options` SET `option_value`='1190' WHERE `option_name`='db_version';

#### 1191 - Kayden - bugfix for bogus default value for stat requirements
ALTER TABLE `item_stats` ALTER COLUMN `requirement_3_name` SET DEFAULT NULL;
UPDATE `item_stats` SET `requirement_3_name`=NULL where `requirement_3_name` = '0';
UPDATE `server_options` SET `option_value`='1191' WHERE `option_name`='db_version';

#### 1192 - Paldorin - adding flag for forceall option of changename and adding checkitem command
DELETE FROM command_group_assignment where command_name='/checkitem';
INSERT INTO command_group_assignment VALUES( "/checkitem", 30 );
INSERT INTO command_group_assignment VALUES( "/checkitem", 26 );
INSERT INTO command_group_assignment VALUES( "/checkitem", 25 );
INSERT INTO command_group_assignment VALUES( "/checkitem", 24 );
INSERT INTO command_group_assignment VALUES( "/checkitem", 23 );
INSERT INTO command_group_assignment VALUES( "/checkitem", 22 );
INSERT INTO command_group_assignment VALUES( "changenameall", 30 );
INSERT INTO command_group_assignment VALUES( "changenameall", 25 );
INSERT INTO command_group_assignment VALUES( "changenameall", 24 );
INSERT INTO command_group_assignment VALUES( "changenameall", 23 );
UPDATE `server_options` SET `option_value`='1192' WHERE `option_name`='db_version';

#### 1193 - Steven Patrick - Adding /changeguildleader GM command
INSERT INTO command_group_assignment VALUES( "/changeguildleader", 30 );
INSERT INTO command_group_assignment VALUES( "/changeguildleader", 25 );
INSERT INTO command_group_assignment VALUES( "/changeguildleader", 24 );
INSERT INTO command_group_assignment VALUES( "/changeguildleader", 23 );
INSERT INTO command_group_assignment VALUES( "/changeguildleader", 22 );
UPDATE `server_options` SET `option_value`='1193' WHERE `option_name`='db_version';

#### 1194 - Steven Patrick - Adding /changeguildleader GM command
INSERT INTO command_group_assignment VALUES( "command area", 23 );
INSERT INTO command_group_assignment VALUES( "command area", 22 );
UPDATE `server_options` SET `option_value`='1194' WHERE `option_name`='db_version';

#### 1195 - Stefano Angeleri - adding /target_name command and notifications on join/part of guild members from game

INSERT INTO command_group_assignment VALUES( "/target_name", 30 );
INSERT INTO command_group_assignment VALUES( "/target_name", 25 );
INSERT INTO command_group_assignment VALUES( "/target_name", 24 );
INSERT INTO command_group_assignment VALUES( "/target_name", 23 );
INSERT INTO command_group_assignment VALUES( "/target_name", 22 );
INSERT INTO command_group_assignment VALUES( "/target_name", 21 );
ALTER TABLE `characters` ADD `guild_notifications` TINYINT( 1 ) UNSIGNED NOT NULL DEFAULT '0' AFTER `guild_private_notes` ;
UPDATE `server_options` SET `option_value`='1195' WHERE `option_name`='db_version';

#### 1196 - Stefano Angeleri - allowing /morph me to gm3+
INSERT INTO command_group_assignment VALUES( "morph others", 30 );
INSERT INTO command_group_assignment VALUES( "morph others", 25 );
INSERT INTO command_group_assignment VALUES( "morph others", 24 );
INSERT INTO command_group_assignment VALUES( "/morph", 23 );
UPDATE `server_options` SET `option_value`='1196' WHERE `option_name`='db_version';

#### 1197 - Tristan Cragnolini - Adding /unstackable GM command
INSERT INTO command_group_assignment VALUES( "/unstackable", 30 );
INSERT INTO command_group_assignment VALUES( "/unstackable", 25 );
INSERT INTO command_group_assignment VALUES( "/unstackable", 24 );
UPDATE `server_options` SET `option_value`='1197' WHERE `option_name`='db_version';

#### 1198 - Tristan Cragnolini - changing /unstackable to /setstackable
DELETE FROM command_group_assignment WHERE command_name = '/unstackable';
INSERT INTO command_group_assignment VALUES( "/setstackable", 30 );
INSERT INTO command_group_assignment VALUES( "/setstackable", 25 );
INSERT INTO command_group_assignment VALUES( "/setstackable", 24 );
DELETE FROM command_group_assignment WHERE command_name="/unstackable";
UPDATE `server_options` SET `option_value`='1198' WHERE `option_name`='db_version';

#### 1199 - Dave Bentham - Mini game boards definition table
CREATE TABLE gameboards
(
  name VARCHAR(30) NOT NULL,
  numColumns INTEGER UNSIGNED NOT NULL,
  numRows INTEGER UNSIGNED NOT NULL,
  layout TEXT NOT NULL,
  pieces TEXT NOT NULL,
  numPlayers INTEGER DEFAULT 2,
  PRIMARY KEY (`name`)
);
UPDATE `server_options` SET `option_value`='1199' WHERE `option_name`='db_version';

### 1200 - Jochen 'peeg' Woidich - add support for sectors where dropped items do not get deleted after some time
ALTER TABLE `sectors` ADD `non_transient_objects` TINYINT NOT NULL DEFAULT '0' AFTER `collide_objects` ;
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'guildlaw';
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'guildsimple';
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'NPCRoom';
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'NPCRoom1';
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'NPCRoom2';
UPDATE sectors SET non_transient_objects = 1 WHERE name = 'NPCRoom3';
UPDATE `server_options` SET `option_value`='1200' WHERE `option_name`='db_version';

#### 1201 - Stefano Angeleri - added a /disablequest command to disable/enable quests
INSERT INTO command_group_assignment VALUES( "save quest disable", 30 );
INSERT INTO command_group_assignment VALUES( "/disablequest", 30 );
INSERT INTO command_group_assignment VALUES( "/disablequest", 25 );
UPDATE `server_options` SET `option_value`='1201' WHERE `option_name`='db_version';

#### 1202 - Dave Bentham - Mini game board options
ALTER TABLE `gameboards` ADD COLUMN `gameboardOptions` varchar(100) NOT NULL DEFAULT 'White,Checked' AFTER `numPlayers`;
UPDATE `server_options` SET `option_value`='1202' WHERE `option_name`='db_version';

#### 1203 - Keith Fulton - Voiced NPC Dialog Support
ALTER TABLE `npc_responses` ADD `audio_path` VARCHAR(100) NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand.';
UPDATE `server_options` SET `option_value`='1203' WHERE `option_name`='db_version';

#### 1204 - Stefano Angeleri - removed underscore from admin commands and removed /show_gm which is a duplicate of /show gm
UPDATE command_group_assignment SET command_name = '/targetname' WHERE command_name = '/target_name';
UPDATE command_group_assignment SET command_name = '/banadvisor' WHERE command_name = '/ban_advisor';
UPDATE command_group_assignment SET command_name = '/unbanadvisor' WHERE command_name = '/unban_advisor';
DELETE FROM command_group_assignment WHERE command_name="/show_gm";
UPDATE `server_options` SET `option_value`='1204' WHERE `option_name`='db_version';

#1205 - Stefano Angeleri - Added a required change in order to make last kougaro patch work correctly
UPDATE item_stats SET valid_slots = REPLACE(valid_slots, 'HEAD', 'HELM');
UPDATE `server_options` SET `option_value`='1205' WHERE `option_name`='db_version';

#### 1206 - Dave Bentham - added rules string to gameboards table
ALTER TABLE `gameboards` ADD COLUMN `gameRules` TEXT AFTER `gameboardOptions`;
UPDATE `server_options` SET `option_value`='1206' WHERE `option_name`='db_version';

#### 1207 - Steven Patrick - Changed /updaterespawn to developers only
DELETE FROM command_group_assignment WHERE command_name = '/updaterespawn';
INSERT INTO command_group_assignment VALUES( "/updaterespawn", 30 );
UPDATE `server_options` SET `option_value`='1207' WHERE `option_name`='db_version';

#### 1208 - Keith Fulton - New required item stats for new quest
UPDATE `server_options` SET `option_value`='1208' WHERE `option_name`='db_version';

#### 1209 - Dave Bentham - added endgames string to gameboards table
ALTER TABLE `gameboards` ADD COLUMN `endgames` TEXT AFTER `gameRules`;
UPDATE `server_options` SET `option_value`='1209' WHERE `option_name`='db_version';

#### 1210 - Stefano Angeleri - Added a new column to store char creation data
ALTER TABLE `characters` ADD COLUMN `description_ooc` TEXT  DEFAULT NULL AFTER `description`;
ALTER TABLE `characters` ADD COLUMN `creation_info` TEXT  DEFAULT NULL AFTER `description_ooc`;
UPDATE `server_options` SET `option_value`='1210' WHERE `option_name`='db_version';

#### 1211 - Keith Fulton - added last npc tracking to quest assignements
ALTER TABLE `character_quests` ADD `last_response_npc_id` INT(10) UNSIGNED NULL COMMENT 'This field stores the npc PID who gave the last response. Required for menu filtering.';
UPDATE `server_options` SET `option_value`='1211' WHERE `option_name`='db_version';

#### Also 1211 - Stefano Angeleri - Added /setkillexp
INSERT INTO command_group_assignment VALUES( "/setkillexp", 30 );
INSERT INTO command_group_assignment VALUES( "/setkillexp", 25 );
INSERT INTO command_group_assignment VALUES( "/setkillexp", 24 );
INSERT INTO command_group_assignment VALUES( "/setkillexp", 23 );

#### Lowered killnpc required access level to gm2
INSERT INTO command_group_assignment VALUES( "/killnpc", 24 );
INSERT INTO command_group_assignment VALUES( "/killnpc", 23 );
INSERT INTO command_group_assignment VALUES( "/killnpc", 22 );

#### Added /assignfaction for gm4+
INSERT INTO command_group_assignment VALUES( "/assignfaction", 30 );
INSERT INTO command_group_assignment VALUES( "/assignfaction", 25 );
INSERT INTO command_group_assignment VALUES( "/assignfaction", 24 );

#### 1212 - Keith Fulton - Renamed column name to be more accurate and to be required
ALTER TABLE `npc_spawn_ranges` CHANGE `cstr_id_spawn_sector` `sector_id` int(10) NOT NULL DEFAULT '0';
UPDATE `server_options` SET `option_value`='1212' WHERE `option_name`='db_version';

#### 1213 - Stefano Angeleri - Added the ability to make char creation data dynamic with factions
ALTER TABLE factions ADD COLUMN faction_character BLOB  NOT NULL AFTER faction_description;
ALTER TABLE characters ADD COLUMN description_life TEXT  DEFAULT NULL AFTER creation_info;

#### Kougaro - Added item rotation around the x and z axis

INSERT INTO command_group_assignment VALUES( "rotate all", 30 );
INSERT INTO command_group_assignment VALUES( "rotate all", 25 );
INSERT INTO command_group_assignment VALUES( "rotate all", 24 );
INSERT INTO command_group_assignment VALUES( "rotate all", 23 );
INSERT INTO command_group_assignment VALUES( "rotate all", 22 );

ALTER TABLE item_instances ADD COLUMN `loc_xrot` float(14,6) default '0' AFTER `loc_z`;
ALTER TABLE item_instances ADD COLUMN `loc_zrot` float(14,6) default '0' AFTER `loc_yrot`;

UPDATE `server_options` SET `option_value`='1213' WHERE `option_name`='db_version';

#### 1214 - Kenny Graunke - Rewrote the magic system
# Manual conversion is required!
# 1. All your progression scripts are bunk.  This includes item equip scripts.
# 2. Action locations with <Entrance Type='Prime' Script=...> need to specify an inline MathExpression
#    rather than the name of a progression script.
UPDATE characters SET progression_script = '';
ALTER TABLE item_stats DROP COLUMN prg_evt_unequip;
ALTER TABLE item_stats CHANGE prg_evt_equip   equip_script   text;
ALTER TABLE item_stats CHANGE prg_evt_consume consume_script text;

-- You'll also need to remove spells from the math_script table, and add the individual
-- pieces to new columns in the spells table.
ALTER TABLE spells CHANGE caster_effect casting_effect varchar(255),
                   DROP target_effect,
                   CHANGE image_name image_name varchar(100),
                   CHANGE offensive offensive boolean DEFAULT true,
                   DROP saved_progression_event,
                   DROP saving_throw,
                   DROP saving_throw_value,
                   ADD exclude_target boolean DEFAULT false AFTER target_type,
                   ADD cast_duration text NOT NULL AFTER exclude_target,
                   ADD `range` text AFTER cast_duration,
                   ADD aoe_radius text AFTER range,
                   ADD aoe_angle text AFTER aoe_radius,
                   CHANGE progression_event outcome text NOT NULL AFTER aoe_angle;

UPDATE `server_options` SET `option_value`='1214' WHERE `option_name`='db_version';

#### 1215 - Kenny Graunke - Make the loot generator work with the new scripting system
ALTER TABLE loot_modifiers DROP COLUMN prg_evt_unequip;
ALTER TABLE loot_modifiers CHANGE prg_evt_equip equip_script text NULL AFTER not_usable_with;

UPDATE `server_options` SET `option_value`='1215' WHERE `option_name`='db_version';

#### 1216 - Kenny Graunke - Remove TARGET_NPC and TARGET_PVP from spell target types.
UPDATE `spells` SET target_type = target_type & ~0x102;

UPDATE `server_options` SET `option_value`='1216' WHERE `option_name`='db_version';

#### 1217 - Kenny Graunke - Update syntax of RunScriptResponseOp

-- You'll need to manually change responses from:
-- <response><run scr="...script name..." param0="..."/></response>
-- to something like:
-- <response><run script="...script name..." with="Param0 = 42.0; Foo = 'foo';"/></response>

UPDATE `server_options` SET `option_value`='1217' WHERE `option_name`='db_version';

#### added a new math script Calculate Repair Rank, removing so the hardcoded script

INSERT INTO math_scripts VALUES( "Calculate Repair Rank","Result = if(Object:SalePrice > 300,Object:SalePrice/150,0);");

-- remember to add this line at the end of Calculate Repair Time,
-- or the one of your choice which does the same
-- Result = if(Result < 20, 20, Result);

INSERT INTO math_scripts VALUES( "Calculate Repair Quality",
"
ResultQ = if(Object:Quality+RepairAmount > Object:MaxQuality, Object:MaxQuality, Object:Quality+RepairAmount);
ResultMaxQ = Object:MaxQuality-(ResultQ-Object:Quality)*0.2;
ResultMaxQ = if(ResultMaxQ < 0, 0, ResultMaxQ);
ResultQ = if(ResultQ > ResultMaxQ, ResultMaxQ, ResultQ);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Experience",
"
ResultPractice = 1;
ResultModifier = RepairAmount;
");

INSERT INTO math_scripts VALUES( "Calculate Mining Experience",
"
ResultPractice = if(Success, 1, 0);
ResultModifier = if(Success, 25, 2);
");

INSERT INTO math_scripts VALUES( "Calculate Skill Experience",
"
Exp = PracticePoints*Modifier;
");

INSERT INTO math_scripts VALUES( "Calculate Trasformation Experience",
"
Exp = if(StartQuality < CurrentQuality, 2*(CurrentQuality-StartQuality), 0);
");

ALTER TABLE `trade_processes` MODIFY COLUMN `secondary_skill_id` INTEGER;

#1218 - Stefano Angeleri - added a statue column in character table to determine statue characters

ALTER TABLE `characters` ADD COLUMN `statue` TINYINT(1) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Determines if the character is a statue so will be put in STATUE mode' AFTER `banker`;
UPDATE `server_options` SET `option_value`='1218' WHERE `option_name`='db_version';

#1219 - Stefano Angeleri - Added a sector specific say range patch by aiken

ALTER TABLE `sectors` ADD COLUMN `say_range` FLOAT(5,2) unsigned NOT NULL DEFAULT '0.0' COMMENT 'Determines the range of say in the specific sector. Set 0.0 to use default.' AFTER `non_transient_objects`;
UPDATE `server_options` SET `option_value`='1219' WHERE `option_name`='db_version';

#1219 - Stefano Angeleri - Added a setkillexp others access rule and allowed setkillexp to have a target

INSERT INTO command_group_assignment VALUES( "setkillexp others", 30 );

#1219 - Stefano Angeleri - Added a setattrib others access rule and allowed setattrib to have a target
INSERT INTO command_group_assignment VALUES( "setattrib others", 30 );

#1219 - Stefano Angeleri - /serverquit command
INSERT INTO command_group_assignment VALUES( "/serverquit", 30 );

#1220 - Steven Patrick - Added account_id column to track accounts in gm_command_log
ALTER TABLE `gm_command_log` ADD COLUMN `account_id` INT(10) UNSIGNED NOT NULL DEFAULT 0 AFTER `id`;
UPDATE `server_options` SET `option_value`='1220' WHERE `option_name`='db_version';

#1221 - Stefano Angeleri - Added vote and comment in the character_events table
ALTER TABLE `character_events` ADD COLUMN `vote` TINYINT(2)  DEFAULT NULL COMMENT 'The vote expressed by the player on the specific event (range 1-10)' AFTER `event_id`,
ADD COLUMN `comments` TEXT  DEFAULT NULL COMMENT 'A comment left by the player on the event' AFTER `vote`;
UPDATE `server_options` SET `option_value`='1221' WHERE `option_name`='db_version';

#1222 - Stefano Angeleri - Added bracer support
ALTER TABLE `race_info` ADD COLUMN `bracer` VARCHAR(20)  COMMENT 'Stores a bracer group allowing to use the same bracer mesh for more than one race, just like for the helm column' default '' AFTER `helm`;
UPDATE `server_options` SET `option_value`='1222' WHERE `option_name`='db_version';

#1223 - Steven Patrick - Added optional IP ban
ALTER TABLE `bans` ADD COLUMN `ban_ip` BOOLEAN NOT NULL DEFAULT 0 COMMENT 'Account banned by IP as well' AFTER `reason`;
INSERT INTO command_group_assignment VALUES( "long bans", 30 );
INSERT INTO command_group_assignment VALUES( "long bans", 25 );
INSERT INTO command_group_assignment VALUES( "long bans", 24 );
INSERT INTO command_group_assignment VALUES( "/ban", 23 );
INSERT INTO command_group_assignment VALUES( "/ban", 22 );
INSERT INTO command_group_assignment VALUES( "/unban", 23 );
INSERT INTO command_group_assignment VALUES( "/unban", 22 );
UPDATE `server_options` SET `option_value`='1223' WHERE `option_name`='db_version';

#1224 - Stefano Angeleri - added belt/cloak support
ALTER TABLE `race_info` ADD COLUMN `belt` VARCHAR(20)  COMMENT 'Stores a belt group allowing to use the same belt mesh for more than one race, just like for the helm column' default '' AFTER `bracer`;
ALTER TABLE `race_info` ADD COLUMN `cloak` VARCHAR(20)  COMMENT 'Stores a cloak group allowing to use the same cloak mesh for more than one race, just like for the helm column' default '' AFTER `belt`;
UPDATE `server_options` SET `option_value`='1224' WHERE `option_name`='db_version';

#1224 - Stefano Angeleri - Added separate audio paths per response
ALTER TABLE `npc_responses` CHANGE COLUMN `audio_path` `audio_path1` VARCHAR(100) DEFAULT NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response1.',
 ADD COLUMN `audio_path2` VARCHAR(100)  DEFAULT NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response2.' AFTER `audio_path1`,
 ADD COLUMN `audio_path3` VARCHAR(100)  DEFAULT NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response3.' AFTER `audio_path2`,
 ADD COLUMN `audio_path4` VARCHAR(100)  DEFAULT NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response4.' AFTER `audio_path3`,
 ADD COLUMN `audio_path5` VARCHAR(100)  DEFAULT NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response5.' AFTER `audio_path4`;

#1225 - Stefano Angeleri - Added max guild points and per character permissions
ALTER TABLE `guilds` ADD COLUMN `max_guild_points` INT(10)  NOT NULL DEFAULT 100 COMMENT 'Stores the maximum amount of gp allowed for assignment in this guild' AFTER `secret_ind`;
ALTER TABLE `characters` ADD COLUMN `guild_additional_privileges` SMALLINT(3) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Contains a bitfield with the additional priviledges assigned to this char (additional to the guild level it\'s in)' AFTER `guild_level`,
 ADD COLUMN `guild_denied_privileges` SMALLINT(3) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Contains a bitfield with the removed priviledges from this char (removed from the guild level it\'s in)' AFTER `guild_additional_privileges`;
UPDATE `server_options` SET `option_value`='1225' WHERE `option_name`='db_version';

#1225 - Stefano Angeleri - Deputize allowed to gm2

INSERT INTO command_group_assignment VALUES( "/deputize", 23 );
INSERT INTO command_group_assignment VALUES( "/deputize", 22 );

#1225 - Stefano Angeleri - Allow death to have a requestor (which can be anything)

INSERT INTO command_group_assignment VALUES( "requested death", 30 );

#1226 - Stefano Angeleri - Added selectable max slots for a container

ALTER TABLE `item_stats` ADD COLUMN `container_max_slots` INT(5) UNSIGNED NOT NULL DEFAULT 0 COMMENT "The amount of slots which will be available in this item to store items inside if it's a container" AFTER `container_max_size`;
UPDATE item_stats set container_max_slots=16 where flags like "%CONTAINER%";
UPDATE `server_options` SET `option_value`='1226' WHERE `option_name`='db_version';

#1227 - Tristan Cragnolini - Added per-race movement modifiers table
CREATE TABLE `movement_race_mods` (
  `id` int(11) NOT NULL,
  `name` varchar(100) NOT NULL default 'stonebm'     COMMENT 'name of the race',
  `move_mod` float NOT NULL default '1.0'            COMMENT 'motion modifier' ,
  PRIMARY KEY  (`id`)
);
UPDATE `server_options` SET `option_value`='1227' WHERE `option_name`='db_version';

#1228 - Tristan Cragnolini - the speed modifier is now stored in race_info
DROP TABLE movement_race_mods;
ALTER TABLE `race_info` ADD COLUMN `speed_modifier` float NOT NULL default '1.0'   COMMENT "Used as a multiplier of the velocity" AFTER `race`;
UPDATE `server_options` SET `option_value`='1228' WHERE `option_name`='db_version';

#1229 - Kenny Graunke - remove character_advantages relation table
DROP TABLE character_advantages;
UPDATE `server_options` SET `option_value`='1229' WHERE `option_name`='db_version';

#1230 - Mike Gist - Remove common_strings, update field names and types.
ALTER TABLE `item_animations` CHANGE COLUMN `cstr_id_animation` `cstr_animation` VARCHAR(200) DEFAULT '';
UPDATE `item_animations` set cstr_animation=(select string from common_strings where id=cstr_animation);
UPDATE `item_animations` set cstr_animation='' where cstr_animation is null;
ALTER TABLE `item_animations` MODIFY COLUMN `cstr_animation` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `item_stats` CHANGE COLUMN `cstr_id_gfx_mesh` `cstr_gfx_mesh` VARCHAR(200) DEFAULT '';
UPDATE `item_stats` set cstr_gfx_mesh=(select string from common_strings where id=cstr_gfx_mesh);
UPDATE `item_stats` set cstr_gfx_mesh='' where cstr_gfx_mesh is null;
ALTER TABLE `item_stats` MODIFY COLUMN `cstr_gfx_mesh` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `item_stats` CHANGE COLUMN `cstr_id_gfx_icon` `cstr_gfx_icon` VARCHAR(200) DEFAULT '';
UPDATE `item_stats` set cstr_gfx_icon=(select string from common_strings where id=cstr_gfx_icon);
UPDATE `item_stats` set cstr_gfx_icon='' where cstr_gfx_icon is null;
ALTER TABLE `item_stats` MODIFY COLUMN `cstr_gfx_icon` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `item_stats` CHANGE COLUMN `cstr_id_gfx_texture` `cstr_gfx_texture` VARCHAR(200) DEFAULT '';
UPDATE `item_stats` set cstr_gfx_texture=(select string from common_strings where id=cstr_gfx_texture);
UPDATE `item_stats` set cstr_gfx_texture='' where cstr_gfx_texture is null;
ALTER TABLE `item_stats` MODIFY COLUMN `cstr_gfx_texture` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `item_stats` CHANGE COLUMN `cstr_id_part` `cstr_part` VARCHAR(200) DEFAULT '';
UPDATE `item_stats` set cstr_part=(select string from common_strings where id=cstr_part);
UPDATE `item_stats` set cstr_part='' where cstr_part is null;
ALTER TABLE `item_stats` MODIFY COLUMN `cstr_part` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `item_stats` CHANGE COLUMN `cstr_id_part_mesh` `cstr_part_mesh` VARCHAR(200) DEFAULT '';
UPDATE `item_stats` set cstr_part_mesh=(select string from common_strings where id=cstr_part_mesh);
UPDATE `item_stats` set cstr_part_mesh='' where cstr_part_mesh is null;
ALTER TABLE `item_stats` MODIFY COLUMN `cstr_part_mesh` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `quests` CHANGE COLUMN `cstr_id_icon` `cstr_icon` VARCHAR(200) DEFAULT '';
UPDATE `quests` set cstr_icon=(select string from common_strings where id=cstr_icon);
UPDATE `quests` set cstr_icon='' where cstr_icon is null;
ALTER TABLE `quests` MODIFY COLUMN `cstr_icon` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `race_info` CHANGE COLUMN `cstr_id_mesh` `cstr_mesh` VARCHAR(200) DEFAULT '';
UPDATE `race_info` set cstr_mesh=(select string from common_strings where id=cstr_mesh);
UPDATE `race_info` set cstr_mesh='' where cstr_mesh is null;
ALTER TABLE `race_info` MODIFY COLUMN `cstr_mesh` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `race_info` CHANGE COLUMN `cstr_id_base_texture` `cstr_base_texture` VARCHAR(200) DEFAULT '';
UPDATE `race_info` set cstr_base_texture=(select string from common_strings where id=cstr_base_texture);
UPDATE `race_info` set cstr_base_texture='' where cstr_base_texture is null;
ALTER TABLE `race_info` MODIFY COLUMN `cstr_base_texture` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `spells` MODIFY COLUMN `cstr_npc_spell_category` VARCHAR(200) DEFAULT '';
UPDATE `spells` set cstr_npc_spell_category=(select string from common_strings where id=cstr_npc_spell_category);
UPDATE `spells` set cstr_npc_spell_category='' where cstr_npc_spell_category is null;
ALTER TABLE `spells` MODIFY COLUMN `cstr_npc_spell_category` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `traits` CHANGE COLUMN `cstr_id_mesh` `cstr_mesh` VARCHAR(200) DEFAULT '';
UPDATE `traits` set cstr_mesh=(select string from common_strings where id=cstr_mesh);
UPDATE `traits` set cstr_mesh='' where cstr_mesh is null;
ALTER TABLE `traits` MODIFY COLUMN `cstr_mesh` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `traits` CHANGE COLUMN `cstr_id_material` `cstr_material` VARCHAR(200) DEFAULT '';
UPDATE `traits` set cstr_material=(select string from common_strings where id=cstr_material);
UPDATE `traits` set cstr_material='' where cstr_material is null;
ALTER TABLE `traits` MODIFY COLUMN `cstr_material` VARCHAR(200) NOT NULL DEFAULT '';
ALTER TABLE `traits` CHANGE COLUMN `cstr_id_texture` `cstr_texture` VARCHAR(200) DEFAULT '';
UPDATE `traits` set cstr_texture=(select string from common_strings where id=cstr_texture);
UPDATE `traits` set cstr_texture='' where cstr_texture is null;
ALTER TABLE `traits` MODIFY COLUMN `cstr_texture` VARCHAR(200) NOT NULL DEFAULT '';
DROP TABLE common_strings;
UPDATE `server_options` SET `option_value`='1230' WHERE `option_name`='db_version';

#1230 - Stefano Angeler made penilty penalty
ALTER TABLE `trade_transformations` CHANGE COLUMN `penilty_pct` `penalty_pct` FLOAT(10,6)  NOT NULL DEFAULT '1.000000';

#1231 - Andrew Dai added os and graphics card info
ALTER TABLE `accounts` ADD COLUMN `operating_system` VARCHAR(32) NULL default ''   COMMENT "The last operating system used with this account" AFTER `advisor_ban`;
ALTER TABLE `accounts` ADD COLUMN `graphics_card` VARCHAR(64) NULL default ''   COMMENT "The last graphics card used with this account" AFTER `operating_system`;
UPDATE `server_options` SET `option_value`='1231' WHERE `option_name`='db_version';

#1232 - Andrew Dai added graphics driver version
ALTER TABLE `accounts` ADD COLUMN `graphics_version` VARCHAR(32) NULL default ''   COMMENT "The graphics card driver version used with this account" AFTER `graphics_card`;
UPDATE `server_options` SET `option_value`='1232' WHERE `option_name`='db_version';

#1232 - Stefano Angeleri this is better as text than a blob
ALTER TABLE `factions` MODIFY COLUMN `faction_character` TEXT  NOT NULL;

#1233 - Stefano Angeleri added the anim to be used by the mounter when mounting something
ALTER TABLE `race_info` ADD COLUMN `cstr_mounter_animation` VARCHAR(200)  NOT NULL default '' COMMENT 'Defines the animation the mounter of this race will use when mounting it' AFTER `cloak`;
ALTER TABLE `race_info` MODIFY COLUMN `bracer` VARCHAR(20)  DEFAULT '' COMMENT 'Stores a bracer group allowing to use the same bracer mesh for more than one race, just like for the helm column';
ALTER TABLE `item_categories` ADD COLUMN `repair_difficulty_pct` INT(3) UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Difficulty amount when repairing this item category (passed to the repair scripts)' AFTER `skill_id_repair`;
UPDATE `server_options` SET `option_value`='1233' WHERE `option_name`='db_version';

#1234 - Kenneth Graunke removed exclude_target from spells (scripts can now check via <if t="Target = OrigTarget">)
ALTER TABLE `spells` DROP COLUMN `exclude_target`;
UPDATE `server_options` SET `option_value`='1234' WHERE `option_name`='db_version';

#1234 - Steven Patrick added hiding from buddy lists for gms/devs
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 30 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 25 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 24 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 23 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 22 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 21 );
INSERT INTO command_group_assignment VALUES( "default buddylisthide", 10 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 30 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 25 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 24 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 23 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 22 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 21 );
INSERT INTO command_group_assignment VALUES( "/buddyhide", 10 );


#1235 - Talad - changed wc_statistics table
ALTER TABLE `wc_statistics` ADD COLUMN `param1` TEXT AFTER `query`;
ALTER TABLE `wc_statistics` DROP COLUMN `query`;


# Insert your upgrade before this line. Remember when you set a new db_version
# to update the server_options.sql file and update psserver.cpp as well.
# This to ensure that everything is working if you use the create_all.sql to
# create a new fresh DB.



