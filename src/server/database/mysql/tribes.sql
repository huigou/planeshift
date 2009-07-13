#
# Table structure for table tribes
#

#
# Use max_size = -1 for unlimited tribe size.
#

DROP TABLE IF EXISTS `tribes`;
CREATE TABLE `tribes` 
(
  id int(10) NOT NULL auto_increment,
  name varchar(30) NOT NULL default '',
  home_sector_id int(10) unsigned NOT NULL default '0',
  home_x float(10,2) DEFAULT '0.00' ,
  home_y float(10,2) DEFAULT '0.00' ,
  home_z float(10,2) DEFAULT '0.00' ,
  home_radius float(10,2) DEFAULT '0.00' ,
  max_size int(10) signed DEFAULT '0',
  wealth_resource_name varchar(30) NOT NULL default '',
  wealth_resource_nick varchar(30) NOT NULL default '',
  wealth_resource_area varchar(30) NOT NULL default '',
  reproduction_cost int(10) signed DEFAULT '0',
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table characters
#

INSERT INTO `tribes` VALUES (1,'Test tribe',3,-80,0,-180,10,15,'Gold Ore','gold','mine',5);
