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

 Date: 21/03/2022 08:17:10
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for updates_include
-- ----------------------------
DROP TABLE IF EXISTS `updates_include`;
CREATE TABLE `updates_include`  (
  `path` varchar(200) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED','CUSTOM') CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`) USING BTREE
) ENGINE = MyISAM CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci COMMENT = 'List of directories where we want to include sql updates.' ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of updates_include
-- ----------------------------
INSERT INTO `updates_include` VALUES ('$/data/sql/updates/db_node', 'RELEASED');
INSERT INTO `updates_include` VALUES ('$/data/sql/custom/db_node', 'CUSTOM');
INSERT INTO `updates_include` VALUES ('$/data/sql/archive/db_node', 'ARCHIVED');

SET FOREIGN_KEY_CHECKS = 1;
