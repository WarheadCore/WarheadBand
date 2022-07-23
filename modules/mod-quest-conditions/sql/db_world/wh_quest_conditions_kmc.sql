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

 Date: 21/07/2022 16:26:14
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for wh_quest_conditions_kmc
-- ----------------------------
DROP TABLE IF EXISTS `wh_quest_conditions_kmc`;
CREATE TABLE `wh_quest_conditions_kmc`  (
  `NpcEntry` int(11) NOT NULL DEFAULT 0,
  `QuestConditionType` tinyint(1) NOT NULL DEFAULT 0,
  `Value` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`NpcEntry`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of wh_quest_conditions_kmc
-- ----------------------------
INSERT INTO `wh_quest_conditions_kmc` VALUES (190010, 4, 15);

SET FOREIGN_KEY_CHECKS = 1;
