DELETE FROM `string_module`
WHERE `Entry` IN (
    -- Slots
    'TRANSMOG_LOCALE_HEAD',
    'TRANSMOG_LOCALE_SHOULDERS',
    'TRANSMOG_LOCALE_SHIRT',
    'TRANSMOG_LOCALE_CHEST',
    'TRANSMOG_LOCALE_WAIST',
    'TRANSMOG_LOCALE_LEGS',
    'TRANSMOG_LOCALE_FEET',
    'TRANSMOG_LOCALE_WRISTS',
    'TRANSMOG_LOCALE_HANDS',
    'TRANSMOG_LOCALE_BACK',
    'TRANSMOG_LOCALE_MAIN_HAND',
    'TRANSMOG_LOCALE_OFF_HAND',
    'TRANSMOG_LOCALE_RANGED',
    'TRANSMOG_LOCALE_TABARD',

    -- Gossips
    'TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_WORKS',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_MANAGE_SETS',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_SINGLE_TRANSMOG',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_UPDATE_MENU',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_BACK',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG_Q',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_SINGLE_TRANSMOG_Q',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_SAVE_SET',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_ITEM_BIND_Q',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_SET_WORKS',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_SET_USE',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE_Q',
    'TRANSMOG_LOCALE_GOSSIP_ITEM_SET_INSERT_NAME',

    -- Strings
    'TRANSMOG_LOCALE_TRANSMOG_OK',
    'TRANSMOG_LOCALE_TRANSMOG_INVALID_SLOT',
    'TRANSMOG_LOCALE_TRANSMOG_INVALID_SRC_ENTRY',
    'TRANSMOG_LOCALE_TRANSMOG_MISSING_SRC_ITEM',
    'TRANSMOG_LOCALE_TRANSMOG_MISSING_DEST_ITEM',
    'TRANSMOG_LOCALE_TRANSMOG_INVALID_ITEMS',
    'TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_MONEY',
    'TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_TOKENS',
    'TRANSMOG_LOCALE_UNTRANSMOG_OK',
    'TRANSMOG_LOCALE_UNTRANSMOG_NO_TRANSMOGS',
    'TRANSMOG_LOCALE_PRESET_ERR_INVALID_NAME');

INSERT INTO `string_module` (`ModuleName`, `ID`, `Locale`, `Text`)
        VALUES
            -- Slot name enUS
            ('TRANSMOG_LOCALE_HEAD', 'enUS', 'Head'), ('TRANSMOG_LOCALE_SHOULDERS', 'enUS', 'Shoulders'), ('TRANSMOG_LOCALE_SHIRT', 'enUS', 'Shirt'), ('TRANSMOG_LOCALE_CHEST', 'enUS', 'Chest'), ('TRANSMOG_LOCALE_WAIST', 'enUS', 'Waist'), ('TRANSMOG_LOCALE_LEGS', 'enUS', 'Legs'), ('TRANSMOG_LOCALE_FEET', 'enUS', 'Feet'), ('TRANSMOG_LOCALE_WRISTS', 'enUS', 'Wrists'), ('TRANSMOG_LOCALE_HANDS', 'enUS', 'Hands'), ('TRANSMOG_LOCALE_BACK', 'enUS', 'Back'), ('TRANSMOG_LOCALE_MAIN_HAND', 'enUS', 'Main hand'), ('TRANSMOG_LOCALE_OFF_HAND', 'enUS', 'Off hand'), ('TRANSMOG_LOCALE_RANGED', 'enUS', 'Ranged'), ('TRANSMOG_LOCALE_TABARD', 'enUS', 'Tabard'),
            -- Slot name ruRU
            ('TRANSMOG_LOCALE_HEAD', 'ruRU', 'Голова'), ('TRANSMOG_LOCALE_SHOULDERS', 'ruRU', 'Плечи'), ('TRANSMOG_LOCALE_SHIRT', 'ruRU', 'Рубашка'), ('TRANSMOG_LOCALE_CHEST', 'ruRU', 'Грудь'), ('TRANSMOG_LOCALE_WAIST', 'ruRU', 'Пояс'), ('TRANSMOG_LOCALE_LEGS', 'ruRU', 'Ноги'), ('TRANSMOG_LOCALE_FEET', 'ruRU', 'Ступни'), ('TRANSMOG_LOCALE_WRISTS', 'ruRU', 'Запястье'), ('TRANSMOG_LOCALE_HANDS', 'ruRU', 'Руки'), ('TRANSMOG_LOCALE_BACK', 'ruRU', 'Спина'), ('TRANSMOG_LOCALE_MAIN_HAND', 'ruRU', 'Правая рука'), ('TRANSMOG_LOCALE_OFF_HAND', 'ruRU', 'Левая рука'), ('TRANSMOG_LOCALE_RANGED', 'ruRU', 'Дальний бой'), ('TRANSMOG_LOCALE_TABARD', 'ruRU', 'Гербовая накидка'),
            -- enUS Gossips
            ('TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_WORKS', 'enUS', '|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|tHow transmogrification works'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_MANAGE_SETS', 'enUS', '|TInterface/RAIDFRAME/UI-RAIDFRAME-MAINASSIST:30:30:-18:0|tManage sets'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG', 'enUS', '|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|tRemove all transmogrifications'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG', 'enUS', '|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|tRemove transmogrification'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_UPDATE_MENU', 'enUS', '|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|tUpdate menu'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_BACK', 'enUS', '|TInterface/ICONS/Ability_Spy:30:30:-18:0|tBack..'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG_Q', 'enUS', 'Remove transmogrifications from all equipped items?'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_SINGLE_TRANSMOG_Q', 'enUS', 'Remove transmogrification from the slot?'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SAVE_SET', 'enUS', '|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:30:30:-18:0|tSave set'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_ITEM_BIND_Q', 'enUS', 'Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\nDo you wish to continue?\n\n'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_SET_WORKS', 'enUS', '|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|tHow sets work'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_USE', 'enUS', '|TInterface/ICONS/INV_Misc_Statue_02:30:30:-18:0|tUse set'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE', 'enUS', '|TInterface/PaperDollInfoFrame/UI-GearManager-LeaveItem-Opaque:30:30:-18:0|tDelete set'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE_Q', 'enUS', 'Are you sure you want to delete '), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_INSERT_NAME', 'enUS', 'Insert set name'),
            -- ruRU Gossips
            ('TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_WORKS', 'ruRU', '|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|tКак это работает'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_MANAGE_SETS', 'ruRU', '|TInterface/RAIDFRAME/UI-RAIDFRAME-MAINASSIST:30:30:-18:0|tМеню комплектов'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG', 'ruRU', '|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|tУдалить все трансмогрификации'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG', 'ruRU', '|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|tУдалить трасмогрификацию'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_UPDATE_MENU', 'ruRU', '|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|tОбновить меню'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_BACK', 'ruRU', '|TInterface/ICONS/Ability_Spy:30:30:-18:0|tНазад..'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_ALL_TRANSMOG_Q', 'ruRU', 'Удалить все трансмогрификации со всех одетых предметов?'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_REMOVE_SINGLE_TRANSMOG_Q', 'ruRU', 'Удалить трансмогрификацию с этого слота?'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SAVE_SET', 'ruRU', '|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:30:30:-18:0|tСохранить комплект'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_ITEM_BIND_Q', 'ruRU', 'Использование этого предмета для трансмогрификации сделать предмет персональным.\nВы хотите продолжить?\n\n.'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_HOW_SET_WORKS', 'ruRU', '|TInterface/ICONS/INV_Misc_Book_11:30:30:-18:0|tКак работают сеты'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_USE', 'ruRU', '|TInterface/ICONS/INV_Misc_Statue_02:30:30:-18:0|tИспользовать комплект'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE', 'ruRU', '|TInterface/PaperDollInfoFrame/UI-GearManager-LeaveItem-Opaque:30:30:-18:0|tУдалить комплект'), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_DELETE_Q', 'ruRU', 'Вы действительно хотите удалить '), ('TRANSMOG_LOCALE_GOSSIP_ITEM_SET_INSERT_NAME', 'ruRU', 'Введите имя комплекта'),
            -- Strings ruRU
            ('TRANSMOG_LOCALE_TRANSMOG_OK', 'ruRU', 'Предмет изменён'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_SLOT', 'ruRU', 'Пустой слот'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_SRC_ENTRY', 'ruRU', 'Неверно выбранный предмет'), ('TRANSMOG_LOCALE_TRANSMOG_MISSING_SRC_ITEM', 'ruRU', 'Изначальный предмет не найден'), ('TRANSMOG_LOCALE_TRANSMOG_MISSING_DEST_ITEM', 'ruRU', 'Выбранный предмет не найден'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_ITEMS', 'ruRU', 'Выбранный предмет неверный'), ('TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_MONEY', 'ruRU', 'Вам не хватает золота'), ('TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_TOKENS', 'ruRU', 'Вам не хватает токенов'), ('TRANSMOG_LOCALE_UNTRANSMOG_OK', 'ruRU', 'Трансмогрификация удалена'), ('TRANSMOG_LOCALE_UNTRANSMOG_NO_TRANSMOGS', 'ruRU', 'Трансмогрификации не найдены'), ('TRANSMOG_LOCALE_PRESET_ERR_INVALID_NAME', 'ruRU', 'Введёное имя некорректно'),
            -- Strings enUS
            ('TRANSMOG_LOCALE_TRANSMOG_OK', 'enUS', 'Item transmogrified'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_SLOT', 'enUS', 'Equipment slot is empty'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_SRC_ENTRY', 'enUS', 'Invalid source item selected'), ('TRANSMOG_LOCALE_TRANSMOG_MISSING_SRC_ITEM', 'enUS', 'Source item does not exist'), ('TRANSMOG_LOCALE_TRANSMOG_MISSING_DEST_ITEM', 'enUS', 'Destination item does not exist'), ('TRANSMOG_LOCALE_TRANSMOG_INVALID_ITEMS', 'enUS', 'Selected items are invalid'), ('TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_MONEY', 'enUS', 'Not enough money'), ('TRANSMOG_LOCALE_TRANSMOG_NOT_ENOUGH_TOKENS', 'enUS', 'You don\'t have enough tokens'), ('TRANSMOG_LOCALE_UNTRANSMOG_OK', 'enUS', 'Transmogrifications removed'), ('TRANSMOG_LOCALE_UNTRANSMOG_NO_TRANSMOGS', 'enUS', 'There are no transmogrifications'), ('TRANSMOG_LOCALE_PRESET_ERR_INVALID_NAME', 'enUS', 'Invalid name inserted');
