#
# Table structure for table loot_modifiers_restrains
#

CREATE TABLE `planeshift`.`loot_modifiers_restrains` (
  `loot_modifier_id` INTEGER  NOT NULL COMMENT 'The id of the loot modifier rule',
  `item_id` INTEGER  NOT NULL COMMENT 'The id of the item included in the loot modifier rule',
  PRIMARY KEY (`loot_modifier_id`, `item_id`)
)
ENGINE = MyISAM
COMMENT = 'Allows to define some restrain to the loot_modifiers';

