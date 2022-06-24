-- ----------------------------
-- Table structure for unbind_instance_disabled_maps
-- ----------------------------

DROP TABLE IF EXISTS `unbind_instance_disabled_maps`;
CREATE TABLE `unbind_instance_disabled_maps` (
  `MapID` int(11) NOT NULL,
  `Comment` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`MapID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
