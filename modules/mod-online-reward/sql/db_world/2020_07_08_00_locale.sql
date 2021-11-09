-- Add locales
DELETE FROM `string_module`
WHERE `Entry` IN ('OR_LOCALE_SUBJECT', 'OR_LOCALE_TEXT', 'OR_LOCALE_MESSAGE');
INSERT INTO `string_module` (`ModuleName`, `ID`, `Locale`, `Text`) VALUES

-- enUS
('OR_LOCALE_SUBJECT', 'enUS', 'Reward for online %s'),
('OR_LOCALE_TEXT', 'enUS', 'Hi, %s!\nYou been playing on our server for over %s. Please accept a gift from us.'),
('OR_LOCALE_MESSAGE', 'enUS', 'You were rewarded for online (%s). You can get the award at the post office.'),

-- ruRU
('OR_LOCALE_SUBJECT', 'ruRU', 'Награда за онлайн %s'),
('OR_LOCALE_TEXT', 'ruRU', 'Привет, %s!\nВы играете на нашем сервере уже более %s. Пожалуйста примите подарок'),
('OR_LOCALE_MESSAGE', 'ruRU', 'Вы были вознаграждены за онлайн (%s). Получить награду можно на почте.');
