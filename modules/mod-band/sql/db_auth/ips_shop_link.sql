-- ----------------------------
-- Table structure for ips_shop_link
-- ----------------------------
DROP TABLE IF EXISTS `ips_shop_link`;
CREATE TABLE `ips_shop_link`  (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `RealmName` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `Nickname` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `ItemID` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `ItemQuantity` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `Flag` int(2) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci COMMENT = 'Покупки из магазина' ROW_FORMAT = Dynamic;
