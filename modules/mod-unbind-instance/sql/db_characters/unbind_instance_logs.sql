-- ----------------------------
-- Table structure for unbind_instance_logs
-- ----------------------------

DROP TABLE IF EXISTS `unbind_instance_logs`;
CREATE TABLE `unbind_instance_logs` (
    `ID` int(11) NOT NULL AUTO_INCREMENT COMMENT 'Инкремент',
    `Info` varchar(255) DEFAULT NULL,
    `PlayerGuid` int(11) NOT NULL,
    `MapID` int(10) NOT NULL,
    `Difficulty` int(5) NOT NULL,
    `ItemID` int(10) NOT NULL,
    PRIMARY KEY (`ID`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
