-- ----------------------------
-- Table structure for `attack_types`
-- ----------------------------
DROP TABLE IF EXISTS `attack_types`;
CREATE TABLE `attack_types` (
  `id` INT NOT NULL AUTO_INCREMENT COMMENT 'holds the attack type unique id number' ,
  `name` VARCHAR(40) NOT NULL COMMENT 'is the attack type name, each name must be unique.' ,
  `weaponID` INT NOT NULL COMMENT 'item_stats.id if a specific weapon is required for the special attack. Otherwise, weaponTypeID should be filled in.', 
  `weaponType` VARCHAR(100) DEFAULT NULL COMMENT 'More than one required weapon type may be listed (see weapon_types.id). Separate each weapon type ID with a space.',
  `onehand` BOOLEAN NOT NULL COMMENT 'attack designed for a 1 hand weapon (true) or 2 handed (false)',
  `stat` int(11) NOT NULL COMMENT 'The skill related to this type',
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name`)
) COMMENT='Holds attack types to be used in the updated combat system';

INSERT INTO `attack_types` VALUES (1, 'Assassination', 0, '1 2', 1, 1);
INSERT INTO `attack_types` VALUES (2, 'Archery', 0, '7 8', 1, 6);
INSERT INTO `attack_types` VALUES (3, 'Barbaric', 0, '1 10 5 4 3', 0, 3);
INSERT INTO `attack_types` VALUES (4, 'Long Bow Special', 409, NULL, 1, 6);
