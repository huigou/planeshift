#
# Table structure for table tribe_recipes
#

DROP TABLE IF EXISTS `tribe_recipes`;
CREATE TABLE `tribe_recipes`
(
  id int(10) NOT NULL auto_increment,
  name varchar(30) NOT NULL default '',
  requirements varchar(1000) NOT NULL default '',
  algorithm varchar(1000) NOT NULL default '',
  persistent int(1) NOT NULL default '0',
  uniqueness int(1) NOT NULL default '0',
  PRIMARY KEY (`id`)
);

#
# Dumping test data for recipe table
#

#
# Basic Recipes ~ Should not be changed. Data in these recipes is hardcoded in
# the sources. 
#

INSERT INTO `tribe_recipes` VALUES (1, 'Do Nothing', '', 'wait(100);', 1,1);

INSERT INTO `tribe_recipes` VALUES (10, 'Miner Explore', 'tribesman(Miner,1);', 'select(Miner,1);explore();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (11, 'Hunter Explore', 'tribesman(Hunter,1);', 'select(Hunter,1);explore();', 0, 0);

INSERT INTO `tribe_recipes` VALUES (20, 'Dig Resource', 'memory(BUFFER,mine,Miner Explore);tribesman(Miner,1);', 'select(Miner,1);locateResource(BUFFER,Miner Explore);mine();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (21, 'Hunt Resource', 'memory(BUFFER,hunting_ground,Hunter Explore);tribesman(Hunter,1);', 'select(Hunter,1);locateResource(BUFFER,Hnter Explore);gather();', 0, 0);


INSERT INTO `tribe_recipes` VALUES (30, 'Miner Mate', 'resource(REPRODUCTION_RESOURCE,REPRODUCTION_COST,Dig Resource);tribesman(any,1);', 'select(any,1);mate();alterResource(REPRODUCTION_RESOURCE, -REPRODUCTION_COST);', 0 , 0);
INSERT INTO `tribe_recipes` VALUES (31, 'Hunter Mate', 'resource(REPRODUCTION_RESOURCE,REPRODUCTION_COST,Hunt Resource);tribesman(any,1);', 'select(any,1);mate();alterResource(REPRODUCTION_RESOURCE, -REPRODUCTION_COST);', 0 , 0);

# Spot Recipe
INSERT INTO `tribe_recipes` VALUES (40, 'Miners Tribe Spots', '', 'reserveSpot(-77,0,-189,Campfire);reserveSpot(-90,0,-190,Tent);reserveSpot(-77,0,-208,Tent);reserveSpot(-80,0,-205,Tent);', 0, 0);

INSERT INTO `tribe_recipes` VALUES (41, 'Hunting Tribe Spots', '', 'reserveSpot(-70,0,-205,Campfire);reserveSpot(-70,0,-212,Campfire);reserveSpot(-80,0,-212,Tent);reserveSpot(-49,0,-203,Tent);reserveSpot(-52,0,-174,Tent);reserveSpot(-67,0,-147,Tent);reserveSpot(-47,0,-140,Tent);', 0, 0);


# Upkeep
INSERT INTO `tribe_recipes` VALUES (50, 'Mining Upkeep', 'resource(REPRODUCTION_RESOURCE,10,Dig Resource);', 'alterResource(REPRODUCTION_RESOURCE,-10);', 1, 1);
INSERT INTO `tribe_recipes` VALUES (51, 'Hunting Upkeep', 'resource(REPRODUCTION_RESOURCE,10,Hunt Resource);', 'alterResource(REPRODUCTION_RESOURCE,-10);', 1, 1);

# 
INSERT INTO `tribe_recipes` VALUES (60, 'Mining Campfire', 'tribesman(Miner,1);resource(Coal,10,Dig Resource);', 'select(Miner,1);locateBuildingSpot(Campfire);goWork(100);wait(100);addBuilding(Campfire);alterResource(Coal,10);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (61, 'Mining Tent', 'tribesman(Miner,1);resource(Skin,15,Buy Skin);', 'select(Miner,1);locateBuildingSpot(Tent);goWork(150);wait(150);addBuilding(Small Tent);alterResource(Skin,15);', 0, 0);

INSERT INTO `tribe_recipes` VALUES (70, 'Hunter Campfire', 'tribesman(Hunter,1);resource(Coal,10,Buy Coal);', 'select(Miner,1);locateBuildingSpot(Campfire);goWork(100);wait(100);addBuilding(Campfire);alterResource(Coal,10);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (71, 'Hunter Tent', 'tribesman(Hunter,1);resource(Skin,15,Hunt Resource);', 'select(Miner,1);locateBuildingSpot(Tent);goWork(150);wait(150);addBuilding(Small Tent);alterResource(Skin,15);', 0, 0);


# Targets ~ Missions
INSERT INTO `tribe_recipes` VALUES (90, 'Evolve Mining Tribe', 'tribesman(number,16,Miner Mate);resource(Coal,150,Dig Resource);resource(Gold Ore,200,Dig Resource);item(Campfire,2);item(Tent,8);', 'wait(1000);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (91, 'Evolve Hunting Tribe', 'tribesman(number,4,Hunter Mate);resource(Skin,150,Hunt Resource);resource(Meat,200,Hunt Resource);item(Campfire,1);item(Tent,2);', 'wait(1000);', 0, 0);

# Tribal Recipes

INSERT INTO `tribe_recipes` VALUES (100, 'Mining Tribe', ' ', 'brain(civilised);aggressivity(peaceful);growth(conservatory);unity(organised);loadRecipe(Do Nothing);loadRecipe(Evolve Mining Tribe,distributed);loadRecipe(Miners Tribe Spots);', 1, 1);
INSERT INTO `tribe_recipes` VALUES (101, 'Hunting Tribe', ' ', 'brain(civilised);aggressivity(neutral);growth(conservatory);unity(organised);loadRecipe(Do Nothing);loadRecipe(Evolve Hunting Tribe,distributed);loadRecipe(Miners Tribe Spots);', 1, 1);
