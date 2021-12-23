-- ----------------------------
-- Table structure for online_reward
-- ----------------------------
DROP TABLE IF EXISTS `online_reward`;
CREATE TABLE `online_reward` (
  `RewardPlayedTime` INT NOT NULL DEFAULT 0,
  `ItemID` INT NOT NULL DEFAULT 1,
  `Count` INT NOT NULL DEFAULT 1,
  PRIMARY KEY (`RewardPlayedTime`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 ROW_FORMAT=COMPACT;

-- ----------------------------
-- Table structure for online_reward_history
-- ----------------------------
DROP TABLE IF EXISTS  `online_reward_history`;
CREATE TABLE `online_reward_history` (
  `PlayerGuid` INT NOT NULL DEFAULT 0,
  `RewardedPerOnline` longtext DEFAULT NULL,
  `RewardedPerHour` INT DEFAULT 0,
  PRIMARY KEY (`PlayerGuid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 ROW_FORMAT=COMPACT;
