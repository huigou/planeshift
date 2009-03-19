
CREATE TABLE loot_modifiers (
  id INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  modifier_type VARCHAR(20) NULL,
  name VARCHAR(255) NULL,
  effect TEXT NULL,
  probability FLOAT NULL,
  stat_req_modifier TEXT NULL,
  cost_modifier FLOAT NULL,
  mesh VARCHAR(255) NULL,
  not_usable_with VARCHAR(255) NULL,
  equip_script text NULL,
  PRIMARY KEY(id)
);

-- (id,modifier_type,name,effect,probability,stat_req_modifier,cost_modifier,mesh,not_usable_with) 
-- INSERT INTO `loot_modifiers` ( modifier_type, name, effect, probability, stat_req_modifier, cost_modifier, mesh, not_usable_with ) VALUES 

-- Prefixes
INSERT INTO loot_modifiers VALUES (1,'prefix','Platinum','<ModiferEffect operation="mul" name="item.attack_value" value="-0.10" />',39,'<StatReq name="CHA" value="100"/><StatReq name="STR" value="70"/>',0.6,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (2,'prefix','Steel','<ModiferEffect operation="mul" name="item.attack_value" value="-0.20" />',73,'<StatReq name="CHA" value="100"/><StatReq name="STR" value="70"/>',0.4,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (3,'prefix','Plastic','<ModiferEffect operation="mul" name="item.damage" value="0.90" />',94,'<StatReq name="INT" value="100"/><StatReq name="STR" value="70"/>',0.7,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (4,'prefix','Fabric','<ModiferEffect operation="add" name="item.damage" value="5" />',99,'<StatReq name="STR" value="100"/>',1.2,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (5,'prefix','Plutonium','<ModiferEffect operation="add" name="item.damage" value="4" />',124,'<StatReq name="STR" value="200"/>',1.2,'','','<str value="10"/><hp-max value="10"/>');

-- Adjectives
INSERT INTO loot_modifiers VALUES (6,'adjective','Torn','<ModiferEffect operation="mul" name="item.damage" value="0.30" />',4,'<StatReq name="STR" value="200"/>',0.2,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (7,'adjective','Spotted','<ModiferEffect operation="mul" name="item.damage" value="0.40" />',8,'<StatReq name="STR" value="200"/>',0.3,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (8,'adjective','Xeroxed','<ModiferEffect operation="mul" name="item.damage" value="0.50" />',12,'<StatReq name="STR" value="200"/>',0.4,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (9,'adjective','Yellowed','<ModiferEffect operation="mul" name="item.damage" value="0.60" />',15,'<StatReq name="STR" value="200"/>',0.4,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (10,'adjective','Brown','<ModiferEffect operation="mul" name="item.damage" value="0.70" />',19,'<StatReq name="STR" value="200"/>',0.5,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (11,'adjective','Scratched','<ModiferEffect operation="mul" name="item.damage" value="0.80" />',24,'<StatReq name="INT" value="200"/>',0.6,'','','<str value="10"/><hp-max value="10"/>');

-- Suffixes
INSERT INTO loot_modifiers VALUES (12,'suffix','of Purity','<ModiferEffect operation="mul" name="item.damage" value="1.30" />',10,'<StatReq name="STR" value="130" /><StatReq name="END" value="130" />',1.7,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (13,'suffix','of Light','<ModiferEffect operation="mul" name="item.damage_value" value="1.50" />',20,'<StatReq name="STR" value="140" /><StatReq name="AGI" value="140" />',2,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (14,'suffix','of Strength','<ModiferEffect operation="mul" name="item.damage" value="1.20" /><ModiferEffect operation="mul" name="item.attack_value" value="1.50" /><ModiferEffect operation="mul" name="item.damage_value" value="1.50" />',24,'<StatReq name="STR" value="150" /><StatReq name="AGI" value="150" />',4,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (15,'suffix','of Wind','<ModiferEffect operation="mul" name="item.attack_value" value="1.10" /><ModiferEffect operation="add" name="item.damage.cold" value="10" />',30,'<StatReq name="STR" value="130" /><StatReq name="AGI" value="130" />',3,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (16,'suffix','of Electricity','<ModiferEffect operation="mul" name="item.attack_value" value="1.70" />',33,'<StatReq name="AGI" value="140" />',3,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (17,'suffix','of Faith','<ModiferEffect operation="mul" name="item.damage" value="1.80" /><ModiferEffect operation="mul" name="item.attack_value" value="1.60" />',35,'<StatReq name="STR" value="160" /><StatReq name="END" value="160" />',4,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (18,'suffix','of Steel','<ModiferEffect operation="mul" name="item.damage" value="1.80" />',45,'<StatReq name="END" value="150" />',2,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (19,'suffix','of Corn','<ModiferEffect operation="mul" name="item.attack_value" value="1.50" /><ModiferEffect operation="ADD" name="wielder.hitpoints" value="3" />',50,'<StatReq name="INT" value="130" />',3,'','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (20,'suffix','of Incense','<ModiferEffect operation="mul" name="target.hitpoints" value=".9" />',52,'<StatReq name="WIL" value="130" />',6,'','','<str value="10"/><hp-max value="10"/>');

