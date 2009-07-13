# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'alliances'
#

DROP TABLE IF EXISTS `alliances`;
CREATE TABLE alliances (
  id int(11) NOT NULL auto_increment,
  name varchar(25) NOT NULL,
  leading_guild int(11) NOT NULL,

  PRIMARY KEY (id),
  UNIQUE id (id),
  UNIQUE name (name)
);


#
# Data for table 'alliances'
#

INSERT INTO alliances (name, leading_guild) VALUES ('Late Nighters', 1);
