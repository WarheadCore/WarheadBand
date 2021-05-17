--
DELETE FROM `acore_string` WHERE `entry` IN (713, 726);
INSERT INTO `acore_string`(`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES
(713, 'Queue status for %s (Lvl: %u to %u)\nQueued: %u (Need at least %u more)', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
(726, '|cffff0000[Arena Queue Announcer]:|r %s -- [%u-%u] [%u/%u]|r', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
