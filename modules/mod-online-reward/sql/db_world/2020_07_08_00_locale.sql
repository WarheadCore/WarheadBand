-- Add locales
DELETE FROM `string_module`
WHERE `Entry` IN ('OR_LOCALE_SUBJECT', 'OR_LOCALE_TEXT', 'OR_LOCALE_MESSAGE');
INSERT INTO `string_module` (`Entry`, `Locale`, `Text`) VALUES

-- enUS
('OR_LOCALE_SUBJECT', 'enUS', 'Reward for online {}'),
('OR_LOCALE_TEXT', 'enUS', 'Hi, {}!\nYou been playing on our server for over {}. Please accept a gift from us.'),
('OR_LOCALE_MESSAGE', 'enUS', 'You were rewarded for online ({}). You can get the award at the post office.'),

-- ruRU
('OR_LOCALE_SUBJECT', 'ruRU', 'Награда за онлайн {}'),
('OR_LOCALE_TEXT', 'ruRU', 'Привет, {}!\nВы играете на нашем сервере уже более {}. Пожалуйста примите подарок'),
('OR_LOCALE_MESSAGE', 'ruRU', 'Вы были вознаграждены за онлайн ({}). Получить награду можно на почте.');
