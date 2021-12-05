-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('LEVEL_REWARD_LOCALE_SUBJECT', 'LEVEL_REWARD_LOCALE_TEXT', 'LEVEL_REWARD_LOCALE_MESSAGE');

INSERT INTO `string_module` (`Entry`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('LEVEL_REWARD_LOCALE_SUBJECT', 'ruRU', 'Награда за повышение уровня до {}'), ('LEVEL_REWARD_LOCALE_TEXT', 'ruRU', 'Вы повысили свой уровень до {} и получаете награду!'), ('LEVEL_REWARD_LOCALE_MESSAGE', 'ruRU', '|cffff0000#|r |cff7bbef7Вы повысили свой уровень до|r {} |cff7bbef7и получаете награду!|r'),
            -- enUS
            ('LEVEL_REWARD_LOCALE_SUBJECT', 'enUS', 'Reward for level up to {}'), ('LEVEL_REWARD_LOCALE_TEXT', 'enUS', 'You increased level to {} and get a reward!'), ('LEVEL_REWARD_LOCALE_MESSAGE', 'enUS', '|cffff0000#|r |cff7bbef7You increased level to|r {} |cff7bbef7and get a reward!|r');
