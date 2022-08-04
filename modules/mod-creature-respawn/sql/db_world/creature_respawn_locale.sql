-------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('CR_LOCALE_NPC_RESPAWN', 'CR_LOCALE_NOT_ENOUGH', 'CR_LOCALE_NPC_IS_ALIVE', 'CR_LOCALE_NPC_KILL');

INSERT INTO `string_module`
        VALUES
        ('CR_LOCALE_NPC_RESPAWN', 'enUS', '|cFFFF0000[CR]:|r {} |cff6C8CD5respawned'),
        ('CR_LOCALE_NOT_ENOUGH', 'enUS', '|cFFFF0000[CR]:|r|cff6C8CD5 You dont have enough|r {} х{}'),
        ('CR_LOCALE_NPC_IS_ALIVE', 'enUS', '|cFFFF0000#|r {} |cff6C8CD5is alive'),
        ('CR_LOCALE_NPC_KILL', 'enUS', '|cFFFF0000[CR]: |cff7bbef7{}|r and his group killed |cffff0000{}|r'),

        ('CR_LOCALE_NPC_RESPAWN', 'ruRU', '|cFFFF0000[CR]:|r {} |cff6C8CD5возродился'),
        ('CR_LOCALE_NOT_ENOUGH', 'ruRU', '|cFFFF0000[CR]:|r|cff6C8CD5 Вам не хватает|r {} х{}'),
        ('CR_LOCALE_NPC_IS_ALIVE', 'ruRU', '|cFFFF0000[CR]:|r {} |cff6C8CD5жив'),
        ('CR_LOCALE_NPC_KILL', 'ruRU', '|cFFFF0000[CR]: |cff7bbef7{}|r и его группа убили |cffff0000{}|r');
