-- Дамп структуры для таблица account_premium
DROP TABLE IF EXISTS `account_premium`;
CREATE TABLE IF NOT EXISTS `account_premium` (
  `Index` bigint(20) NOT NULL AUTO_INCREMENT,
  `AccountID` int(11) NOT NULL DEFAULT 0,
  `StartTime` datetime NOT NULL DEFAULT current_timestamp(),
  `EndTime` datetime NOT NULL DEFAULT current_timestamp(),
  `Level` int(11) NOT NULL DEFAULT 1,
  `IsActive` tinyint(4) NOT NULL DEFAULT 1,
  PRIMARY KEY (`Index`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4;
