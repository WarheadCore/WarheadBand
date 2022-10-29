--
-- Table structure for table `ddos_protection`
--

DROP TABLE IF EXISTS `ddos_protection`;
CREATE TABLE `ddos_protection` (
  `IP` varchar(50) NOT NULL,
  `BanDate` int(11) NOT NULL DEFAULT 0,
  `UnBanDate` int(11) NOT NULL DEFAULT 0,
  `Reason` varchar(255) DEFAULT 'No reason',
  `IsActive` tinyint(4) NOT NULL DEFAULT 0,
  PRIMARY KEY (`IP`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
