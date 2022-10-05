-- ----------------------------
-- Table structure for node_info
-- ----------------------------
DROP TABLE IF EXISTS `node_info`;
CREATE TABLE `node_info`  (
  `ID` int(11) NOT NULL,
  `Type` tinyint(1) NULL DEFAULT NULL,
  `IP` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT '127.0.0.1',
  `WorldPort` smallint(5) NULL DEFAULT NULL,
  `NodePort` smallint(5) NULL DEFAULT NULL,
  `Name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`ID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of node_info
-- ----------------------------
INSERT INTO `node_info` VALUES (1, 0, '127.0.0.1', 8085, 8095, 'Realm');
INSERT INTO `node_info` VALUES (2, 1, '127.0.0.1', 8086, 8096, 'Master');
INSERT INTO `node_info` VALUES (3, 2, '127.0.0.1', 8087, 8097, 'Pvp');
