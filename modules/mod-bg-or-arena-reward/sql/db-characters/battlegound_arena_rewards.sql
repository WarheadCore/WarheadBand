/*
 Navicat Premium Data Transfer

 Source Server         : MariaDB_Local
 Source Server Type    : MariaDB
 Source Server Version : 100901
 Source Host           : localhost:3306
 Source Schema         : wh_characters

 Target Server Type    : MariaDB
 Target Server Version : 100901
 File Encoding         : 65001

 Date: 06/06/2022 02:48:06
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for battlegound_arena_rewards
-- ----------------------------
DROP TABLE IF EXISTS `battlegound_arena_rewards`;
CREATE TABLE `battlegound_arena_rewards`  (
  `BgTypeID` int(11) NOT NULL DEFAULT 0,
  `WinnerItemID` int(11) NOT NULL DEFAULT 1,
  `WinnerItemCount` int(11) NOT NULL DEFAULT 1,
  `LoserItemID` int(11) NOT NULL DEFAULT 1,
  `LoserItemCount` int(11) NOT NULL DEFAULT 1,
  PRIMARY KEY (`BgTypeID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of battlegound_arena_rewards
-- ----------------------------
INSERT INTO `battlegound_arena_rewards` VALUES (2, 37711, 3, 37711, 1);
INSERT INTO `battlegound_arena_rewards` VALUES (30, 37711, 5, 37711, 1);

SET FOREIGN_KEY_CHECKS = 1;
