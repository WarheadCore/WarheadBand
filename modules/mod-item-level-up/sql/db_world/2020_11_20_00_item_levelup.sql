-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('ILU_LOCALE_MAX_LEVEL', 'ILU_LOCALE_GET_LEVEL');

INSERT INTO `string_module` (`Entry`, `Locale`, `Text`)
        VALUES
            -- ruRU
            ('ILU_LOCALE_MAX_LEVEL', 'ruRU', '|cffff0000# |cff00ff00Вы уже имеете максимально возможный уровень в игре:|r %u'), ('ILU_LOCALE_GET_LEVEL', 'ruRU', '|cffff0000# |cff00ff00Вы получили максимальный уровень в игре:|r %u'),
            -- enUS
            ('ILU_LOCALE_MAX_LEVEL', 'enUS', '|cffff0000# |cff00ff00You already have the highest possible level in the game:|r %s'), ('ILU_LOCALE_GET_LEVEL', 'enUS', '|cffff0000# |cff00ff00You got the maximum level in game:|r %s');

-- Add item
SET @ITEM_ID := 113874;

DELETE FROM `item_template`
WHERE `entry` = @ITEM_ID;

INSERT INTO `item_template`
        VALUES (@ITEM_ID, 0, 0, - 1, 'Level-Up Token', 46787, 6, 0, 0, 1, 0, 0, 0, - 1, - 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1000, 0, 0, 18282, 0, 0, 0, 8000, 0, - 1, 0, 0, 0, 0, - 1, 0, - 1, 0, 0, 0, 0, - 1, 0, - 1, 0, 0, 0, 0, - 1, 0, - 1, 0, 0, 0, 0, - 1, 0, - 1, 0, '|cff00FF00Use: Instantly levels your character to the next level.|r', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, - 1, 0, 0, 0, 0, 'ItemLevelUp_Item', 0, 0, 0, 0, 0, - 4);
