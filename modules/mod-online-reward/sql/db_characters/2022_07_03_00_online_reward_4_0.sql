--
DROP TABLE IF EXISTS `wh_online_rewards`;
CREATE TABLE `wh_online_rewards` (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `IsPerOnline` tinyint(1) NOT NULL,
  `Seconds` int(20) NOT NULL DEFAULT 0,
  `Items` longtext CHARACTER SET utf8mb4 DEFAULT NULL,
  `Reputations` longtext CHARACTER SET utf8mb4 DEFAULT NULL,
  PRIMARY KEY (`ID`,`IsPerOnline`,`Seconds`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb4 ROW_FORMAT=COMPACT;

DROP TABLE IF EXISTS `wh_online_rewards_history`;
CREATE TABLE `wh_online_rewards_history` (
  `PlayerGuid` int(20) NOT NULL DEFAULT 0,
  `RewardID` int(11) NOT NULL DEFAULT 0,
  `RewardedSeconds` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`PlayerGuid`, `RewardID`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 ROW_FORMAT=COMPACT;

-- Add from old table
INSERT INTO `wh_online_rewards` (`IsPerOnline`, `Seconds`, `Items`, `Reputations`) SELECT * FROM `online_reward`;

-- Delete old table
DROP TABLE IF EXISTS `online_reward`;

-- Add column for played time
ALTER TABLE `online_reward_history`
ADD COLUMN `RewardedSeconds` int(11) NOT NULL DEFAULT 0 AFTER `RewardedPerTime`;

-- Set played time
UPDATE `characters` AS `ch`, `online_reward_history` AS `orh` SET `orh`.`RewardedSeconds` = `ch`.`totaltime` WHERE `ch`.`guid` = `orh`.`PlayerGuid`;
