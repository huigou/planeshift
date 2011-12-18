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

INSERT INTO `tribe_recipes` VALUES (1, 'Explore', 'tribesman(any,1);', 'select(any,1);explore();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (2, 'Dig Resource', 'memory(BUFFER);tribesman(miner,1);', 'select(any,1);locateResource(BUFFER);mine();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (3, 'Gather Resource', 'memory(BUFFER);tribesman(gatherer,1);', 'select(any,1);locateResource(BUFFER);gather();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (4, 'mate', 'resource(REPRODUCTION_RESOURCE,REPRODUCTION_COST);tribesman(any,1);', 'select(any,1);mate();alterResource(REPRODUCTION_RESOURCE, -REPRODUCTION_COST);', 0 , 0);

# Tribal Recipes

INSERT INTO `tribe_recipes` VALUES (5, 'tribe_mining', ' ', 'brain(civilised);aggressivity(pacifist);growth(conservatory);unity(organised);loadRecipe(do nothing);loadRecipe(evolve,distributed);loadRecipe(Miners Tribe Spots);', 1, 1);

# Spot Recipe
INSERT INTO `tribe_recipes` VALUES (7, 'Miners Tribe Spots', '', 'reserveSpot(-77,0,-189,campfire);reserveSpot(-90,0,-190,door);reserveSpot(-77,0,-208,campfire);reserveSpot(-80,0,-205,torch);reserveSpot(-70,0,-205,torch);reserveSpot(-70,0,-212,torch);reserveSpot(-80,0,-212,torch);reserveSpot(-49,0,-203,door);reserveSpot(-52,0,-174,campfire);reserveSpot(-67,0,-147,door);reserveSpot(-47,0,-140,campfire);', 0, 0);

# Upkeep
INSERT INTO `tribe_recipes` VALUES (8, 'upkeep', 'resource(REPRODUCTION_RESOURCE,10);', 'alterResource(REPRODUCTION_RESOURCE,-10);', 1, 1);

# Buildings
INSERT INTO `tribe_recipes` VALUES (9, 'campfire', 'tribesman(any,1);resource(Coal,10);', 'select(any,1);locateBuildingSpot(campfire);goWork(100);wait(100);addBuilding(campfire);alterResource(Coal,10);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (10, 'door', 'tribesman(any,1);resource(Tin Ore,15);', 'select(any,1);locateBuildingSpot(door);goWork(150);wait(150);addBuilding(door);alterResource(Tin Ore,15);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (11, 'torch', 'tribesman(any,1);resource(Coal Ore,5);resource(Tin Ore,5);', 'select(any,1);locateBuildingSpot(torch);goWork(50);wait(50);addBuilding(torch);alterResource(Tin Ore,5);alterResource(Coal Ore,5);', 0, 0);

# Targets ~ Missions
INSERT INTO `tribe_recipes` VALUES (12, 'do nothing', '', 'wait(100);', 1,1);
INSERT INTO `tribe_recipes` VALUES (13, 'evolve', 'tribesman(number,50);resource(Coal,150);resource(Gold Ore,200);resource(Tin Ore,150);resource(Silver Ore,50);resource(Platinum Ore,50);item(campfire,4);item(torch,4);item(door,2);', 'wait(1000);', 0, 0);
