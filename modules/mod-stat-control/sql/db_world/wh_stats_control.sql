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

 Date: 16/07/2022 10:42:16
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for wh_stats_control
-- ----------------------------
DROP TABLE IF EXISTS `wh_stats_control`;
CREATE TABLE `wh_stats_control`  (
  `StatControlType` enum('AttackPowerFromAgility','DoodgeFromAgility','ArmorFromAgility','CritFromAgility','CritFromIntellect','ManaFromIntellect','BlockFromStrenght') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `Value` float NULL DEFAULT 1,
  PRIMARY KEY (`StatControlType`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of wh_stats_control
-- ----------------------------
INSERT INTO `wh_stats_control` VALUES ('AttackPowerFromAgility', 1);
INSERT INTO `wh_stats_control` VALUES ('DoodgeFromAgility', 1);
INSERT INTO `wh_stats_control` VALUES ('ArmorFromAgility', 1);
INSERT INTO `wh_stats_control` VALUES ('CritFromAgility', 1);
INSERT INTO `wh_stats_control` VALUES ('CritFromIntellect', 1);
INSERT INTO `wh_stats_control` VALUES ('ManaFromIntellect', 1);
INSERT INTO `wh_stats_control` VALUES ('BlockFromStrenght', 1);

SET FOREIGN_KEY_CHECKS = 1;
