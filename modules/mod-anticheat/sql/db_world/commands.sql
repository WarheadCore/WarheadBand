DELETE FROM `command` WHERE `name` LIKE 'anticheat%';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('anticheat player', 2, 'Syntax: .anticheat player [$charactername]\nDisplay anticheat statistics of current session of player.'),
('anticheat delete', 3, 'Syntax: .anticheat delete [$charactername]\nDeletes anticheat statistics of current session of player.'),
('anticheat jail', 2, 'Syntax: .anticheat jail [$charactername]\nJails and restricts player and teleports GM cmd user to jail with no restrictions'),
('anticheat parole', 3, 'Syntax: .anticheat parole [$charactername]\nDeletes anticheat statistics, removes jail restrictions, and sends to faction capital of player.');

DELETE FROM `commands_help_locale` WHERE `Command` LIKE 'anticheat%';
INSERT INTO `commands_help_locale` (`Command`, `Locale`, `Content`) VALUES
('anticheat player', 'ruRU', 'Синтаксис: .anticheat player [$имяигрока]\nПоказать статистику для игрока'),
('anticheat delete', 'ruRU', 'Синтаксис: .anticheat delete [$имяигрока]\nУдалить статистику для игрока'),
('anticheat jail', 'ruRU', 'Синтаксис: .anticheat jail [$имяигрока]\n Поместить иигрока в тюрьму и ограничить его'),
('anticheat parole', 'ruRU', 'Синтаксис: .anticheat parole [$имяигрока]\nУдалить статистику для игрока, переместить его в столицу фракции и убрать все ограничения');
