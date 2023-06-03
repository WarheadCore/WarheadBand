-- New vip rates
DROP TABLE IF EXISTS `wh_vip_rates`;
CREATE TABLE IF NOT EXISTS `wh_vip_rates` (
  `VipLevel` tinyint(4) NOT NULL,
  `RateXp` float NOT NULL DEFAULT 1,
  `RateHonor` float NOT NULL DEFAULT 1,
  `RateArenaPoint` float NOT NULL DEFAULT 1,
  `RateReputation` float NOT NULL DEFAULT 1,
  `RateProfession` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`VipLevel`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;