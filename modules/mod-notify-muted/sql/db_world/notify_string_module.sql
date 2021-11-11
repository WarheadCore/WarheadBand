-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('NM_LOCALE_MESSAGE');

INSERT INTO `string_module` (`Entry`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('NM_LOCALE_MESSAGE', 'ruRU', '|cffff0000# |cff00ff00Игрок|r %s |cff00ff00не сможет говорить ещё:|r %s'),
            -- enUS
            ('NM_LOCALE_MESSAGE', 'enUS', '|cffff0000# |cff00ff00Playe|r %s |cff00ff00will not be able to speak yet:|r %s');
