#
# Table structure for table tribe_needs
#

DROP TABLE IF EXISTS `tribe_needs`;
CREATE TABLE `tribe_needs` 
(
  `tribe_id` int(10) NOT NULL,
  `need_id` int(10) NOT NULL,
  `type` varchar(30) NOT NULL default '',
  `name` varchar(30) NOT NULL default '',
  `perception` varchar(30) NOT NULL default '',
  `depend` varchar(30) NOT NULL default '',
  `need_start_value` float(10,2) NOT NULL default '0.00',
  `need_growth_value` float(10,2) NOT NULL default '0.00',
  PRIMARY KEY  (`tribe_id`,`need_id`)
);

#
# Dumping data for table tribe_needs
#

#
# Nothing has a start value to ensure that this is the need that all members starts with.
# Without a perception this will result in idle behaviour from the tribe brain as long as this
# is the highest needed behaviour.
INSERT INTO `tribe_needs` VALUES (1,1,'GENERIC'      ,'Nothing'  ,''               ,''       ,'0.50','0.00');
#
# Resurrect is and idle need that will be activated for every not alive NPC.
INSERT INTO `tribe_needs` VALUES (1,2,'GENERIC'      ,'Resurrect','tribe:resurrect',''       ,'0.00','0.00');
#
# Explore has a growth value higher than most so members will spend a lot of time exploring.
INSERT INTO `tribe_needs` VALUES (1,3,'GENERIC'      ,'Explore'  ,'tribe:explore'  ,''       ,'0.00','0.20');
#
# Dig needs a mine and if that can not be found it force the depened need Explore to be used.
INSERT INTO `tribe_needs` VALUES (1,4,'RESOURCE_AREA','Dig'      ,'tribe:dig'      ,'Explore','0.00','0.10');
#
# Reproduce need some welth in order to work. Dig will gather that resource to the tribe.
INSERT INTO `tribe_needs` VALUES (1,5,'REPRODUCE'    ,'Reproduce','tribe:reproduce','Dig'    ,'0.00','1.00');
#
# Two gemeric tribe needs. To show that any perceptions could be triggered at any rate.
INSERT INTO `tribe_needs` VALUES (1,6,'GENERIC'      ,'Walk1'    ,'tribe:path1'    ,''       ,'0.00','0.11');
INSERT INTO `tribe_needs` VALUES (1,7,'GENERIC'      ,'Walk2'    ,'tribe:path2'    ,''       ,'0.00','0.09');


#
# Nothing has a start value to ensure that this is the need that all members starts with.
# Without a perception this will result in idle behaviour from the tribe brain as long as this
# is the highest needed behaviour.
INSERT INTO `tribe_needs` VALUES (2,1,'GENERIC'      ,'Nothing'  ,''               ,''       ,'0.50','0.00');
#
# Resurrect is and idle need that will be activated for every not alive NPC.
INSERT INTO `tribe_needs` VALUES (2,2,'GENERIC'      ,'Resurrect','tribe:resurrect',''       ,'0.00','0.00');
#
# Explore has a growth value higher than most so members will spend a lot of time exploring.
INSERT INTO `tribe_needs` VALUES (2,3,'GENERIC'      ,'Explore'  ,'tribe:explore'  ,''       ,'0.00','0.20');
#
# Hunt needs a hunting_ground and if that can not be found it force the depened need Explore to be used.
INSERT INTO `tribe_needs` VALUES (2,4,'RESOURCE_AREA','Hunt'     ,'tribe:hunt'      ,'Explore','0.00','0.10');
#
# Reproduce need some welth in order to work. Hunt will gather that resource to the tribe.
INSERT INTO `tribe_needs` VALUES (2,5,'REPRODUCE'    ,'Reproduce','tribe:reproduce','Hunt'    ,'0.00','1.00');
#
# Once in a while this tribe will relocate to a new place.
INSERT INTO `tribe_needs` VALUES (2,6,'GENERIC'      ,'Move'     ,'tribe:move'     ,''        ,'0.00','0.05');

