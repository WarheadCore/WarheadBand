-- Дамп структуры для таблица wh_characters.vip_rates
DROP TABLE IF EXISTS `vip_rates`;
CREATE TABLE IF NOT EXISTS `vip_rates` (
  `VipLevel` tinyint(4) NOT NULL,
  `RateXp` float NOT NULL DEFAULT 1,
  `RateHonor` float NOT NULL DEFAULT 1,
  `RateArenaPoint` float NOT NULL DEFAULT 1,
  `RateReputation` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`VipLevel`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Дамп структуры для таблица wh_characters.vip_unbind
DROP TABLE IF EXISTS `vip_unbind`;
CREATE TABLE IF NOT EXISTS `vip_unbind` (
  `PlayerGuid` bigint(20) NOT NULL,
  `UnbindTime` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`PlayerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
