DROP TABLE IF EXISTS `wh_title_stats`;
CREATE TABLE IF NOT EXISTS `wh_title_stats` (
  `ID` INT unsigned NOT NULL DEFAULT '0',
  `StatType1` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue1` int NOT NULL DEFAULT '0',
  `StatType2` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue2` int NOT NULL DEFAULT '0',
  `StatType3` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue3` int NOT NULL DEFAULT '0',
  `StatType4` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue4` int NOT NULL DEFAULT '0',
  `StatType5` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue5` int NOT NULL DEFAULT '0',
  `StatType6` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue6` int NOT NULL DEFAULT '0',
  `StatType7` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue7` int NOT NULL DEFAULT '0',
  `StatType8` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue8` int NOT NULL DEFAULT '0',
  `StatType9` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue9` int NOT NULL DEFAULT '0',
  `StatType10` tinyint unsigned NOT NULL DEFAULT '0',
  `StatValue10` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;


INSERT INTO wh_title_stats (ID, StatType1, StatValue1, StatType2, StatValue2, StatType3, StatValue3) VALUES
(98, 3, 10000, 36, 2000, 7, 50000);
