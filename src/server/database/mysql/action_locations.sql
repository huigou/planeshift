CREATE TABLE action_locations
(
  id INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  master_id INTEGER UNSIGNED,
  name VARCHAR(45) NOT NULL,
  sectorname VARCHAR(30) NOT NULL,
  meshname VARCHAR(250) NOT NULL,
  polygon INTEGER UNSIGNED, 
  pos_x FLOAT,
  pos_y FLOAT,
  pos_z FLOAT,
  radius FLOAT,
  triggertype VARCHAR(10),
  responsetype VARCHAR(10),
  response TEXT,
  active_ind  char(1) NOT NULL default 'Y',
  PRIMARY KEY(`id`)
);

#Entrance with lock
INSERT INTO `action_locations` VALUES (1,0,'The Blacksmith Shop','NPCroom','_s_blacksmith',0,-56,0,-148,10,'SELECT','EXAMINE','<Examine><Entrance Type=\'ActionID\' LockID=\'95\' X=\'-58\' Y=\'0\' Z=\'-115\' Rot=\'280\' Sector=\'NPCroom1\' /><Return X=\'-56\' Y=\'0\' Z=\'-148\' Rot=\'130\' Sector=\'NPCroom\' /><Description>A lovely residence</Description></Examine>','Y');

#Entrance with quest check
INSERT INTO `action_locations` VALUES (2,0,'A door to a restricted place','NPCroom','door1', 0,-42,0,-158,10,'SELECT','EXAMINE','<Examine><Entrance Type=\'Prime\' Script=\'rescue_check\'  X=\'-58\' Y=\'0\' Z=\'-115\' Rot=\'280\' Sector=\'NPCroom1\' /><Description>A door to a restricted place</Description></Examine>','Y');

INSERT INTO `action_locations` VALUES (3,0,'IngotsOfFire','NPCroom','smith_ingots',0,0,0,0,10,'PROXIMITY','SCRIPT','flame_damage','Y');
INSERT INTO `action_locations` VALUES (4,0,'HealingTree','NPCroom','_g_treetop02big_npc02',0,0,0,0,4,'PROXIMITY','SCRIPT','healing_tree','Y');
INSERT INTO `action_locations` VALUES (5,0,'woodbox','NPCroom','smith_woodbox',0,0,0,0,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'80\'/>\n<Description>A Box full of books</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (6,0,'Smith Forge','NPCroom','smith_furnace',0,0,0,0,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'115\'/>\n<Description>A smith forge, ready to heat materials</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (7,0,'Anvil','NPCroom','smith_anvil',0,0,0,0,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'116\'/>\n<Description>An anvil used to forge items</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (8,0,'Test Game','NPCroom','Box10',0,4.2,0.99,-216.94,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Test Game\' /><Description>A minigame board for testing.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (10,0,'ownedbox','NPCroom','smith_woodplank',0,0,0,0,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'82\'/>\n<Description>A Box owned by the merchant</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (11,0,'Tic Tac Toe','NPCroom','Box09',0,-1.69,0.89,-217.21,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Tic Tac Toe\' EndGame=\'Yes\' Script=\'minigame_win\' /><Description>Tic Tac Toe Board Game with $$$ prizes $$$.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (12,0,'Rainmaker game','NPCroom','Box08',0,-9.00,0.79,-217.21,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Weather game\' EndGame=\'Yes\' Script=\'rain\' /><Description>Magic puzzle that makes it rain. Make a simple r shape with the game pieces.</Description></Examine>','Y');

