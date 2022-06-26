--
ALTER TABLE `online_reward`
CHANGE COLUMN `ItemID` `Items` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL AFTER `Seconds`,
CHANGE COLUMN `FactionID` `Reputations` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL AFTER `Items`;

UPDATE `online_reward` SET `Items` = CONCAT(`Items`, ':', `ItemCount`);
UPDATE `online_reward` SET `Reputations` = CONCAT(`Reputations`, ':', `ReputationCount`);
UPDATE `online_reward` SET `Reputations` = NULL WHERE `Reputations` = '0:0';

ALTER TABLE `online_reward`
DROP COLUMN `ItemCount`,
DROP COLUMN `ReputationCount`;
