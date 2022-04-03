/*
 Navicat Premium Data Transfer

 Source Server         : #MariaDB_root
 Source Server Type    : MariaDB
 Source Server Version : 100604
 Source Host           : localhost:3306
 Source Schema         : wh_node

 Target Server Type    : MariaDB
 Target Server Version : 100604
 File Encoding         : 65001

 Date: 21/03/2022 08:17:01
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for node_info
-- ----------------------------
DROP TABLE IF EXISTS `node_info`;
CREATE TABLE `node_info`  (
  `ID` int(11) NOT NULL,
  `Type` tinyint(1) NULL DEFAULT NULL,
  `IP` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '127.0.0.1',
  `Port` smallint(5) NULL DEFAULT NULL,
  `Name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`ID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of node_info
-- ----------------------------
INSERT INTO `node_info` VALUES (0, 1, '127.0.0.1', 8085, 'Master');
INSERT INTO `node_info` VALUES (1, 2, '127.0.0.1', 8086, 'PvP');

SET FOREIGN_KEY_CHECKS = 1;
