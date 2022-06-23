-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('UI_COMMAND_FORBIDEN_RESET', 'UI_COMMAND_UNBOUND', 'UI_COMMAND_UNBINDING');

INSERT INTO `string_module`
        VALUES
        -- enUS
        ('UI_COMMAND_FORBIDEN_RESET', 'enUS', 'Instance {} ({}) forbidden to unbind'),
        ('UI_COMMAND_UNBINDING', 'enUS', 'Unbinding map: {} ({}), inst: {}, perm: {}, diff: {}, canReset: {}, TTR: {}{}'),
        ('UI_COMMAND_UNBOUND', 'enUS', 'Instances unbound: {}'),
        -- ruRU
        ('UI_COMMAND_FORBIDEN_RESET', 'ruRU', 'Подземелье {} ({}) в списке запрета для сброса сохранения.'),
        ('UI_COMMAND_UNBINDING', 'ruRU', 'Сброшена карта: {} ({}), подземелье: {}, постоянный: {}, сложность: {}, можно сбросить: {}, сброс общего времени: {}{}'),
        ('UI_COMMAND_UNBOUND', 'ruRU', 'Подземелья сброшены: {}');