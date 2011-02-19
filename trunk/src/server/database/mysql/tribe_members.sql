#
# Table structure for table tribe_members
#

DROP TABLE IF EXISTS `tribe_members`;
CREATE TABLE `tribe_members` 
(
  `tribe_id` int(10) NOT NULL,
  `member_id` int(10) unsigned NOT NULL,
  `member_type` int(10) NOT NULL default '0',
  PRIMARY KEY  (`tribe_id`,`member_id`)
);

#
# Dumping data for table characters
#

INSERT INTO `tribe_members` VALUES (1,20,0);
INSERT INTO `tribe_members` VALUES (2,62,0);

