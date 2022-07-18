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

 Date: 18/07/2022 08:37:46
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for wh_stats_control
-- ----------------------------
DROP TABLE IF EXISTS `wh_stats_control`;
CREATE TABLE `wh_stats_control`  (
  `StatControlType` enum('AttackPowerFromAgility','DoodgeFromAgility','ArmorFromAgility','CritFromAgility','CritFromIntellect','ManaFromIntellect','BlockFromStrenght','DamageFromAttackPower') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL,
  `ClassMask` int(11) NOT NULL DEFAULT 0,
  `Value` float NOT NULL DEFAULT 1,
  `IsEnable` tinyint(1) NOT NULL DEFAULT 1,
  `Comment` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`StatControlType`, `ClassMask`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of wh_stats_control
-- ----------------------------
INSERT INTO `wh_stats_control` VALUES ('AttackPowerFromAgility', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('DoodgeFromAgility', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('ArmorFromAgility', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('CritFromAgility', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('CritFromIntellect', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('ManaFromIntellect', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('BlockFromStrenght', 0, 1, 1, NULL);
INSERT INTO `wh_stats_control` VALUES ('DamageFromAttackPower', 0, 1, 1, NULL);

SET FOREIGN_KEY_CHECKS = 1;
