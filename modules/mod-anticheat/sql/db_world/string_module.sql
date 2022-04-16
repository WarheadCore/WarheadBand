SET @r := '|r '; -- Reset color with whitespace
SET @r1 := '|r'; -- Reset color without whitespace
SET @red := '|cFFFF0000';
SET @extraColor := '|cff7bbef7';

-- En
SET @newLineEn := CONCAT(@red, '[Anticheat]:', @r, @extraColor, 'Player', @r, '{} ', @extraColor);

-- Ru
SET @newLineRu := CONCAT(@red, '[Античит]:', @r, @extraColor, 'Игрок', @r, '{} ', @extraColor);

-- ----------------------------
-- Records of string_module
-- ----------------------------

DELETE FROM `string_module`
WHERE `Entry` IN ('ANTICHEAT_LOCALE_ALERT', 'ANTICHEAT_LOCALE_TELEPORT', 'ANTICHEAT_LOCALE_IGNORE_CONTROL');

INSERT INTO `string_module` VALUES
-- enUS
('ANTICHEAT_LOCALE_ALERT', 'enUS', CONCAT(@newLineEn, 'possible cheater!', @r1)),
('ANTICHEAT_LOCALE_TELEPORT', 'enUS', CONCAT(@newLineEn, 'possible teleport hack detected!', @r1)),
('ANTICHEAT_LOCALE_IGNORE_CONTROL', 'enUS', CONCAT(@newLineEn, 'possible ignore control hack detected!', @r1)),

-- ruRU
('ANTICHEAT_LOCALE_ALERT', 'ruRU', CONCAT(@newLineRu, 'использует читы!', @r1)),
('ANTICHEAT_LOCALE_TELEPORT', 'ruRU', CONCAT(@newLineRu, 'использует читы на телепорт!', @r1)),
('ANTICHEAT_LOCALE_IGNORE_CONTROL', 'ruRU', CONCAT(@newLineRu, 'использует читы на игнор контроля!', @r1));
