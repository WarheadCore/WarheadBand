-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('PIAL_LOCALE_MSG_PREFIX', 'PIAL_LOCALE_MSG_HI', 'PIAL_LOCALE_MSG_GM_LEVEL', 'PIAL_LOCALE_MSG_IP', 'PIAL_LOCALE_MSG_ONLINE', 'PIAL_LOCALE_MSG_UPTIME');

INSERT INTO `string_module` (`Entry`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('PIAL_LOCALE_MSG_PREFIX', 'ruRU', '|cffff0000#|r --'),
            ('PIAL_LOCALE_MSG_HI', 'ruRU', '|cffff0000# |cff00ff00Привет,|r {}!'),
            ('PIAL_LOCALE_MSG_GM_LEVEL', 'ruRU', '|cffff0000# |cff00ff00Ваш уровень аккаунта:|r {}'),
            ('PIAL_LOCALE_MSG_IP', 'ruRU', '|cffff0000# |cff00ff00Ваш айпи:|r {}'),
            ('PIAL_LOCALE_MSG_ONLINE', 'ruRU', '|cffff0000# |cff00ff00Сейчас|r {} |cff00ff00игроков онлайн|r |cff00ff00(максимум:|r {}|cff00ff00)|r'),
            ('PIAL_LOCALE_MSG_UPTIME', 'ruRU', '|cffff0000# |cff00ff00Время работы мира:|r {}'),
            -- enUS
            ('PIAL_LOCALE_MSG_PREFIX', 'enUS', '|cffff0000#|r --'),
            ('PIAL_LOCALE_MSG_HI', 'enUS', '|cffff0000# |cff00ff00Hi,|r {}'),
            ('PIAL_LOCALE_MSG_GM_LEVEL', 'enUS', '|cffff0000# |cff00ff00You account level:|r {}'),
            ('PIAL_LOCALE_MSG_IP', 'enUS', '|cffff0000# |cff00ff00You IP:|r {}'),
            ('PIAL_LOCALE_MSG_ONLINE', 'enUS', '|cffff0000# |cff00ff00Now|r {} |cff00ff00players online|r |cff00ff00(max:|r {}|cff00ff00)|r'),
            ('PIAL_LOCALE_MSG_UPTIME', 'enUS', '|cffff0000# |cff00ff00Server uptime:|r {}');
