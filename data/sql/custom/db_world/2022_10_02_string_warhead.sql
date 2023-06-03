DROP TABLE IF EXISTS `string_warhead`;
CREATE TABLE `string_warhead` (
  `Entry` varchar(100) NOT NULL,
  `Locale` enum('enUS','koKR','frFR','deDE','zhCN','zhTW','esES','esMX','ruRU') NOT NULL,
  `Content` TEXT DEFAULT NULL,
  PRIMARY KEY (`Entry`,`Locale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO string_warhead (`Entry`, `Locale`, `Content`) VALUES
('Alive', 'enUS', 'Alive'),
('Alive', 'ruRu', 'Живой');