-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('BOSS_AN_LOCALE_SEND_TEXT_RAID', 'BOSS_AN_LOCALE_SEND_TEXT_WORLD', 'BOSS_AN_LOCALE_10_MAN_NORMAL', 'BOSS_AN_LOCALE_10_MAN_HEROIC', 'BOSS_AN_LOCALE_25_MAN_NORMAL', 'BOSS_AN_LOCALE_25_MAN_HEROIC');

INSERT INTO `string_module` (`ModuleName`, `ID`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('BOSS_AN_LOCALE_SEND_TEXT_RAID', 'ruRU', '|cffff0000# |cff7bbef7{}|r и его группа убили босса |cffff0000{}|r в режиме |cffff0000{}|r'),
            ('BOSS_AN_LOCALE_SEND_TEXT_WORLD', 'ruRU', '|cffff0000# |cff7bbef7{}|r и его группа убили мирового босса |cffff0000{}|r'),
            ('BOSS_AN_LOCALE_10_MAN_NORMAL', 'ruRU', '10 об.'),
            ('BOSS_AN_LOCALE_10_MAN_HEROIC', 'ruRU', '10 гер.'),
            ('BOSS_AN_LOCALE_25_MAN_NORMAL', 'ruRU', '25 об.'),
            ('BOSS_AN_LOCALE_25_MAN_HEROIC', 'ruRU', '25 гер.'),
            -- enUS
            ('BOSS_AN_LOCALE_SEND_TEXT_RAID', 'enUS', '|cffff0000# |cff7bbef7{}|r and his group killed boss |cffff0000{}|r in difficulty |cffff0000{}|r'),
            ('BOSS_AN_LOCALE_SEND_TEXT_WORLD', 'enUS', '|cffff0000# |cff7bbef7{}|r and his group killed world boss |cffff0000{}|r'),
            ('BOSS_AN_LOCALE_10_MAN_NORMAL', 'enUS', '10 def.'),
            ('BOSS_AN_LOCALE_10_MAN_HEROIC', 'enUS', '10 her.'),
            ('BOSS_AN_LOCALE_25_MAN_NORMAL', 'enUS', '25 def.'),
            ('BOSS_AN_LOCALE_25_MAN_HEROIC', 'enUS', '25 her.');
