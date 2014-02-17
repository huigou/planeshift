-- ----------------------------
-- Table structure for `attacks`
-- ----------------------------
DROP TABLE IF EXISTS `attacks`;
CREATE TABLE `attacks` (
  `id` INT NOT NULL AUTO_INCREMENT COMMENT ' holds the attacks unique id number' ,
  `name` VARCHAR(40) NOT NULL DEFAULT 'default' COMMENT 'is the attacks name, each name must be unique.' ,
  `image_name` VARCHAR(200) NULL DEFAULT '' COMMENT 'the icon image',
  `attack_anim` VARCHAR(40) COMMENT 'The visual effect of the attack',
  `attack_description` text COMMENT 'a short description of the attack',
  `speed` double COMMENT 'the attack speed value',
  `successchance` VARCHAR(40) COMMENT 'a math script for the chance of success should always equal 1-100 and can use weapon skill and a random number',
  `attackType` VARCHAR(40) NOT NULL COMMENT 'the type of attack it is,  found in the attack_types table',
  `form` VARCHAR(40) NOT NULL COMMENT 'the form of attack, currently limited to range and melee, can/will be expanded',
  `range` text COMMENT 'Mostly for range attacks, melee is confined based on weapon range',
  `aoe_radius` text COMMENT 'This is the radius, can be a formula or a solid number, of the aoe attack',
  `aoe_angle` text COMMENT 'The angle in front of the player the aoe attack will affect, 0 for no aoe, 1-360 for aoe, can also be a formula',
  `outcome` text NOT NULL COMMENT 'The effect, in a progression script, of the attack',
  `requirements` VARCHAR(250) NULL DEFAULT '<pre></pre>' COMMENT 'is a xml script of requirements, these requirements will be checked before a character can use the attack. schema to be added soon, but will be very flexable.' ,
  PRIMARY KEY (`id`) ,	
  UNIQUE INDEX `name_UNIQUE` (`name`)
) COMMENT='Holds attacks';

INSERT INTO `attacks` (`id`, `name`, `image_name`, `speed`, `successchance`, `attack_anim`, `attack_description`, `attackType`, `form`, `range`, `aoe_radius`, `aoe_angle`, `outcome`, `requirements`) VALUES (1, 'Hammer Smash', '/planeshift/materials/club01a_icon.dds', 5.0, '100', NULL, 'an attack that causes a hammer to smash the ground and damage all nearby targets', 'Barbaric', 'melee', NULL, '20', '360', 'cast Hammer Smash', '');
INSERT INTO `attacks` (`id`, `name`, `image_name`, `speed`, `successchance`, `attack_anim`, `attack_description`, `attackType`, `form`, `range`, `aoe_radius`, `aoe_angle`, `outcome`, `requirements`) VALUES (2, 'Hyper Shot', '/planeshift/materials/bow_higher01a_icon.dds', 5.0, '100', NULL, 'A very powerful bow attack', 'Archery', 'range', '60', '0', '0', 'cast Hyper Shot', '<pre> <skill name=\"range\" min=\"50\" max=\"4000\" /> </pre>');
INSERT INTO `attacks` (`id`, `name`, `image_name`, `speed`, `successchance`, `attack_anim`, `attack_description`, `attackType`, `form`, `range`, `aoe_radius`, `aoe_angle`, `outcome`, `requirements`) VALUES (3, 'Divine Shot', '/planeshift/materials/bow_higher01a_icon.dds', 5.0, '100', NULL, 'A long bow specific shot that takes the power of the gods and pushes it into an arrow', 'Long Bow Special', 'range', '100', '50', '90', 'cast LB Shot', '');
