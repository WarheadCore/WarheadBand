-- -------------------------
-- Records of string_module
-- -------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('CR_LOCALE_NPC_RESPAWN',
    'CR_LOCALE_NOT_ENOUGH',
    'CR_LOCALE_NPC_IS_ALIVE',
    'CR_LOCALE_NPC_KILL',
    'CR_LOCALE_MORE_TIME',
    'CR_LOCALE_YOU_DECREASED_TIME');

INSERT INTO `string_module`
    VALUES
    ('CR_LOCALE_NPC_RESPAWN', 'enUS', '|cFFFF0000[CR]:|r {} |cff6C8CD5respawned'),
    ('CR_LOCALE_NOT_ENOUGH', 'enUS', '|cFFFF0000[CR]:|r|cff6C8CD5 You dont have enough|r {} х{}'),
    ('CR_LOCALE_NPC_IS_ALIVE', 'enUS', '|cFFFF0000#|r {} |cff6C8CD5is alive'),
    ('CR_LOCALE_NPC_KILL', 'enUS', '|cFFFF0000[CR]: |cff7bbef7{}|r and his group killed |cffff0000{}|r'),
    ('CR_LOCALE_MORE_TIME', 'enUS', '|cFFFF0000[CR]: |cff7bbef7Entered time|r {} |cff7bbef7much more than it needs to be|r {}'),
    ('CR_LOCALE_YOU_DECREASED_TIME', 'enUS', '|cFFFF0000[CR]:|cff7bbef7 You decreased respawn time for |cFFFF0000{}|r |cff7bbef7by|r {}'),

    ('CR_LOCALE_NPC_RESPAWN', 'ruRU', '|cFFFF0000[CR]:|r {} |cff6C8CD5возродился'),
    ('CR_LOCALE_NOT_ENOUGH', 'ruRU', '|cFFFF0000[CR]:|r|cff6C8CD5 Вам не хватает|r {} х{}'),
    ('CR_LOCALE_NPC_IS_ALIVE', 'ruRU', '|cFFFF0000[CR]:|r {} |cff6C8CD5жив'),
    ('CR_LOCALE_NPC_KILL', 'ruRU', '|cFFFF0000[CR]: |cff7bbef7{}|r и его группа убили |cffff0000{}|r'),
    ('CR_LOCALE_MORE_TIME', 'ruRU', '|cFFFF0000[CR]: |cff7bbef7Введёное время|r ({}) |cff7bbef7намного больше нужного|r {}'),
    ('CR_LOCALE_YOU_DECREASED_TIME', 'ruRU', '|cFFFF0000[CR]:|cff7bbef7 Вы уменьшили время возрождения для |cFFFF0000{}|r |cff7bbef7на|r {}');
