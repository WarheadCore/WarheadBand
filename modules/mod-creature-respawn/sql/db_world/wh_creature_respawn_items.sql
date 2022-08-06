-- ----------------------------
-- Table structure for wh_creature_respawn_items
-- ----------------------------
DROP TABLE IF EXISTS `wh_creature_respawn_items`;
CREATE TABLE `wh_creature_respawn_items`  (
  `SpawnID` int(11) NOT NULL,
  `ItemID` int(11) NOT NULL DEFAULT 37711,
  `ItemCount` int(11) NOT NULL DEFAULT 1,
  `DecreaseSeconds` int(11) NOT NULL DEFAULT 60,
  PRIMARY KEY (`SpawnID`, `ItemID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of wh_creature_respawn_items
-- ----------------------------
INSERT INTO `wh_creature_respawn_items` VALUES (54984, 37711, 1, 60);
INSERT INTO `wh_creature_respawn_items` VALUES (54984, 49426, 100, 600);
