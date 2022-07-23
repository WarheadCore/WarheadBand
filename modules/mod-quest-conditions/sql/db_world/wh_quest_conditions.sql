/*
 Navicat Premium Data Transfer

 Source Server         : #MariaDB_Local
 Source Server Type    : MariaDB
 Source Server Version : 100901
 Source Host           : localhost:3306
 Source Schema         : wh_world

 Target Server Type    : MariaDB
 Target Server Version : 100901
 File Encoding         : 65001

 Date: 20/07/2022 14:00:30
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for wh_quest_conditions
-- ----------------------------
DROP TABLE IF EXISTS `wh_quest_conditions`;
CREATE TABLE `wh_quest_conditions`  (
  `QuestID` int(11) NOT NULL DEFAULT 0,
  `UseSpellID` int(11) NOT NULL DEFAULT 0,
  `UseSpellCount` int(11) NOT NULL DEFAULT 0,
  `WinBGType` int(11) NOT NULL DEFAULT 0,
  `WinBGCount` int(11) NOT NULL DEFAULT 0,
  `WinArenaType` int(11) NOT NULL DEFAULT 0,
  `WinArenaCount` int(11) NOT NULL DEFAULT 0,
  `CompleteAchievementID` int(11) NOT NULL DEFAULT 0,
  `CompleteQuestID` int(11) NOT NULL DEFAULT 0,
  `EquipItemID` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`QuestID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
