-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('NPA_MESSAGE');

INSERT INTO `string_module` (`Entry`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('NPA_MESSAGE', 'ruRU', '|cffFFFFFF[|cff2897FFНовый игрок|cffFFFFFF]: Поприветствуйте нового игрока, {}'),
            -- enUS
            ('NPA_MESSAGE', 'enUS', '|cffFFFFFF[|cff2897FFNew player|cffFFFFFF]: Welcome a new player, {}');
