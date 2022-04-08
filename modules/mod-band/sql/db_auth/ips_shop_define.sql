-- ----------------------------
-- Table structure for ips_shop_define
-- ----------------------------
DROP TABLE IF EXISTS `ips_shop_define`;
CREATE TABLE `ips_shop_define`  (
  `ID` int(10) NOT NULL DEFAULT 1,
  `Type` int(10) NOT NULL DEFAULT 0,
  `Value` int(10) NOT NULL,
  PRIMARY KEY (`ID`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of ips_shop_define
-- ----------------------------
INSERT INTO `ips_shop_define` VALUES (9, 0, 37711);
INSERT INTO `ips_shop_define` VALUES (10, 1, 0);
