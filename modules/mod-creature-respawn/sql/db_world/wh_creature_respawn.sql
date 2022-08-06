-- ----------------------------
-- Table structure for wh_creature_respawn
-- ----------------------------
DROP TABLE IF EXISTS `wh_creature_respawn`;
CREATE TABLE `wh_creature_respawn`  (
  `SpawnID` int(11) NOT NULL,
  `Enable` tinyint(4) NOT NULL DEFAULT 1,
  `KillAnounceEnable` tinyint(4) NOT NULL DEFAULT 1,
  `RespawnAnounceEnable` tinyint(4) NOT NULL DEFAULT 1,
  `Comment` text CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`SpawnID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of wh_creature_respawn
-- ----------------------------
INSERT INTO `wh_creature_respawn` VALUES (54984, 1, 1, 1, 'Магистр');
