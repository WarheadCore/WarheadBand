-- DB update 2023_05_10_00 -> 2023_05_10_01
--
DELETE FROM `spell_custom_attr` WHERE `spell_id` IN (33783, 39364);
INSERT INTO `spell_custom_attr` (`spell_id`, `attributes`) VALUES
(33783, 4194304),
(39364, 4194304);