ALTER TABLE `online_reward`
ADD COLUMN `IsPerOnline` tinyint(1) NOT NULL DEFAULT 0 FIRST,
CHANGE COLUMN `RewardPlayedTime` `Seconds` int(20) NOT NULL DEFAULT 0 AFTER `IsPerOnline`,
CHANGE COLUMN `Count` `ItemCount` int(11) NOT NULL DEFAULT 1 AFTER `ItemID`,
ADD COLUMN `FactionID` int(11) NOT NULL DEFAULT 0 AFTER `ItemCount`,
ADD COLUMN `ReputationCount` int(11) NOT NULL DEFAULT 0 AFTER `FactionID`,
DROP PRIMARY KEY,
ADD PRIMARY KEY (`IsPerOnline`, `Seconds`) USING BTREE,
CHARACTER SET = utf8mb4, COLLATE = utf8mb4_general_ci;

ALTER TABLE `online_reward_history`
CHANGE COLUMN `RewardedPerHour` `RewardedPerTime` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL AFTER `RewardedPerOnline`,
CHARACTER SET = utf8mb4, COLLATE = utf8mb4_general_ci;

SET @ConfigPerTime := '3600'; -- Time from old config option 'OR.PerTime.Time'
SET @PerTimeDelimer := ':';
SET @PerTimeValue := CONCAT(@ConfigPerTime, @PerTimeDelimer);
UPDATE `online_reward_history` SET `RewardedPerTime` = CONCAT('3600', ':', `RewardedPerTime`);

/*
-- If you use other value (not 3600)

-- Start query
SET @ConfigPerTime := '3600'; -- Time from old config option 'OR.PerTime.Time'
SET @PerTimeDelimer := ':';
SET @PerTimeValue := CONCAT(@ConfigPerTime, @PerTimeDelimer);
SET @OldConfigPerTime := '36000'; -- Change this value for you config!
SET @OldPerTimeValue := CONCAT(@OldConfigPerTime, @PerTimeDelimer);
UPDATE `online_reward_history` SET `RewardedPerTime` = REPLACE(`RewardedPerTime`, @PerTimeValue, @OldPerTimeValue) WHERE INSTR(`RewardedPerTime`, @PerTimeValue) > 0;
-- End query

-- Add data from old config options `OR.PerTime`
INSERT INTO `online_reward` (`IsPerOnline`, `Seconds`, `ItemID`, `ItemCount`) VALUES (0, 3600, 62701, 1);
*/