-- Add DB table
DROP TABLE IF EXISTS `wh_vip_levels_config`;
CREATE TABLE IF NOT EXISTS `wh_vip_levels_config` (
  `Level` int(20) NOT NULL AUTO_INCREMENT,
  `MountSpell` int(11) NOT NULL DEFAULT 0,
  `LearnSpells` TEXT DEFAULT '',
  `CanUseUnbindCommand` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`Level`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4;

INSERT INTO `wh_vip_levels_config` (`Level`, `MountSpell`, `LearnSpells`, `CanUseUnbindCommand`) VALUES
(1, 0, '', 0),
(2, 0, '', 1),
(3, 0, '', 1);
