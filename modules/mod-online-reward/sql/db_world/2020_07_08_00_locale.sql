-- Add locales
DELETE FROM `string_module`
WHERE `Entry` IN ('OR_LOCALE_SUBJECT', 'OR_LOCALE_TEXT',
'OR_LOCALE_MESSAGE_MAIL', 'OR_LOCALE_MESSAGE_IN_GAME', 'OR_LOCALE_NOT_ENOUGH_BAG');
INSERT INTO `string_module` (`Entry`, `Locale`, `Text`) VALUES

-- enUS
('OR_LOCALE_SUBJECT', 'enUS', 'Reward for online {}'),
('OR_LOCALE_TEXT', 'enUS', 'Hi, {}!\nYou been playing on our server for over {}. Please accept a gift from us.'),
('OR_LOCALE_MESSAGE_MAIL', 'enUS', 'You were rewarded for online ({}). You can get the award at the post office.'),
('OR_LOCALE_MESSAGE_IN_GAME', 'enUS', 'You were rewarded for online ({}).'),
('OR_LOCALE_NOT_ENOUGH_BAG', 'enUS', 'Not enough room in the bag. Send wia mail'),

-- ruRU
('OR_LOCALE_SUBJECT', 'ruRU', 'Награда за онлайн {}'),
('OR_LOCALE_TEXT', 'ruRU', 'Привет, {}!\nВы играете на нашем сервере уже более {}. Пожалуйста примите подарок'),
('OR_LOCALE_MESSAGE_MAIL', 'ruRU', 'Вы были вознаграждены за онлайн ({}). Получить награду можно на почте.'),
('OR_LOCALE_MESSAGE_IN_GAME', 'ruRU', 'Вы были вознаграждены за онлайн ({}).'),
('OR_LOCALE_NOT_ENOUGH_BAG', 'ruRU', 'У вас недостаточно места в сумке, награда будет ждать вас на почте');
