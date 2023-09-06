-- DB update 2022_12_11_00 -> 2022_12_11_01
--

-- add item_dbc table
DROP TABLE IF EXISTS `item_dbc`;
CREATE TABLE `item_dbc` ( `ID` INT NOT NULL DEFAULT '0', `ClassID` INT NOT NULL DEFAULT '0', `SubclassID` INT NOT NULL DEFAULT '0', `Sound_Override_Subclassid` INT NOT NULL DEFAULT '0', `Material` INT NOT NULL DEFAULT '0', `DisplayInfoID` INT NOT NULL DEFAULT '0', `InventoryType` INT NOT NULL DEFAULT '0', `SheatheType` INT NOT NULL DEFAULT '0', PRIMARY KEY (`ID`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Dumping structure for table acore_world.namesreserved_dbc
DROP TABLE IF EXISTS `namesreserved_dbc`;
CREATE TABLE IF NOT EXISTS `namesreserved_dbc` (
  `ID` int unsigned NOT NULL,
  `Pattern` tinytext COLLATE utf8mb4_unicode_ci NOT NULL,
  `LanguagueID` tinyint NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `namesprofanity_dbc`;
CREATE TABLE IF NOT EXISTS `namesprofanity_dbc` (
  `ID` int unsigned NOT NULL,
  `Pattern` tinytext COLLATE utf8mb4_unicode_ci NOT NULL,
  `LanguagueID` tinyint NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;