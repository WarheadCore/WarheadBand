--
DELETE FROM `acore_string` WHERE `entry` IN (713, 726);
INSERT INTO `acore_string`(`entry`, `content_default`) VALUES
(713, 'Queue status for %s (skirmish %s) (Lvl: %u to %u)\nQueued: %u (Need at least %u more)'),
(726, '|cffff0000[Arena Queue]:|r %s (skirmish %s) -- [%u-%u] [%u/%u]|r');
