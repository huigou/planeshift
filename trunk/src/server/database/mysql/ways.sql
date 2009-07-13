# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'ways'
#

DROP TABLE IF EXISTS `ways`;
CREATE TABLE ways (
  id int(8) unsigned NOT NULL auto_increment,
  name varchar(20) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'ways'
#

INSERT INTO ways VALUES("1","Crystal");
INSERT INTO ways VALUES("2","Azure");
INSERT INTO ways VALUES("3","Red");
INSERT INTO ways VALUES("4","Dark");
INSERT INTO ways VALUES("5","Brown");
INSERT INTO ways VALUES("6","Blue");
