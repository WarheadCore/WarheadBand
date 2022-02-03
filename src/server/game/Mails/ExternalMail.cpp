/*
* This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ExternalMail.h"
#include "CharacterCache.h"
#include "GameConfig.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "TaskScheduler.h"

namespace
{
    bool _isSystemEnable = false;
    TaskScheduler scheduler;
}

bool ExMail::AddItems(uint32 itemID, uint32 itemCount)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
    if (!itemTemplate)
    {
        LOG_ERROR("mail.external", "> External Mail: Предмета под номером {} не существует. ID ({})", itemID, ID);
        return false;
    }

    if (itemCount < 1 || (itemTemplate->MaxCount > 0 && itemCount > static_cast<uint32>(itemTemplate->MaxCount)))
    {
        LOG_ERROR("mail.external", "> External Mail: Некорректное количество ({}) для предмета ({}). ID ({})", itemCount, itemID, ID);
        return false;
    }

    while (itemCount > itemTemplate->GetMaxStackSize())
    {
        if (Items.size() <= MAX_MAIL_ITEMS)
        {
            Items.emplace_back(itemID, itemTemplate->GetMaxStackSize());
            itemCount -= itemTemplate->GetMaxStackSize();
        }
        else
        {
            OverCountItems.emplace_back(Items);
            Items.clear();
        }
    }

    Items.emplace_back(itemID, itemCount);
    OverCountItems.emplace_back(Items);

    return true;
}

ExternalMail* ExternalMail::instance()
{
    static ExternalMail instance;
    return &instance;
}

void ExternalMail::Update(uint32 diff)
{
    if (!_isSystemEnable)
        return;

    scheduler.Update(diff);
    _queryProcessor.ProcessReadyCallbacks();
}

void ExternalMail::LoadSystem()
{
    _isSystemEnable = CONF_GET_BOOL("ExternalMail.Enable");

    if (!_isSystemEnable)
    {
        LOG_INFO("server.loading", ">> External mail disabled");
        return;
    }

    scheduler.CancelAll();
    scheduler.Schedule(10s, [this](TaskContext context)
    {
        SendMails();

        context.Repeat();
    });

    LOG_INFO("server.loading", ">> External mail loaded");
}

void ExternalMail::AddMail(std::string_view charName, std::string_view thanksSubject, std::string_view thanksText, uint32 itemID, uint32 itemCount, uint32 creatureEntry)
{
    // Add mail item
    CharacterDatabase.Execute("INSERT INTO `mail_external` (PlayerName, Subject, ItemID, ItemCount, Message, CreatureEntry) VALUES ('{}', '{}', {}, {}, '{}', {})",
        charName, thanksSubject, itemID, itemCount, thanksText, creatureEntry);
}

void ExternalMail::SendMails()
{
    LOG_TRACE("mail.external", "> External Mail: GetMailsFromDB");

    _queryProcessor.AddCallback(
        CharacterDatabase.AsyncQuery("SELECT ID, PlayerName, Subject, Message, Money, ItemID, ItemCount, CreatureEntry FROM mail_external ORDER BY id ASC").
        WithCallback(std::bind(&ExternalMail::SendMailsAsync, this, std::placeholders::_1)));
}

void ExternalMail::SendMailsAsync(QueryResult result)
{
    LOG_TRACE("mail.external", "> External Mail: GetMailsAsync");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();
        std::string PlayerName = fields[1].Get<std::string>();
        std::string Subject = fields[2].Get<std::string>();
        std::string Body = fields[3].Get<std::string>();
        uint32 Money = fields[4].Get<uint32>();
        uint32 ItemID = fields[5].Get<uint32>();
        uint32 ItemCount = fields[6].Get<uint32>();
        uint32 CreatureEntry = fields[7].Get<uint32>();

        if (!normalizePlayerName(PlayerName))
        {
            LOG_ERROR("mail.external", "> External Mail: Неверное имя персонажа ({})", PlayerName);
            continue;
        }

        auto playerGuid = sCharacterCache->GetCharacterGuidByName(PlayerName);
        if (playerGuid.IsEmpty())
            continue;

        // Проверка
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(ItemID);
        if (!itemTemplate)
        {
            LOG_ERROR("mail.external", "> External Mail: Предмета под номером {} не существует. Пропуск", ItemID);
            continue;
        }

        auto const* creature = sObjectMgr->GetCreatureTemplate(CreatureEntry);
        if (!creature)
        {
            LOG_ERROR("mail.external", "> External Mail: НПС под номером {} не существует. Пропуск", CreatureEntry);
            continue;
        }

        ExMail _data;
        _data.ID = ID;
        _data.PlayerGuid = playerGuid;
        _data.Subject = Subject;
        _data.Body = Body;
        _data.Money = Money;
        _data.CreatureEntry = CreatureEntry;

        if (!_data.AddItems(ItemID, ItemCount))
            continue;

        _store.emplace(_data.ID, _data);

    } while (result->NextRow());

    // Check mails
    if (_store.empty())
        return;

    LOG_TRACE("mail.external", "> External Mail: Send mails");

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    for (auto const& [lowGuid, exMail] : _store)
    {
        for (auto const& items : exMail.OverCountItems)
        {
            Player* receiver = ObjectAccessor::FindPlayer(exMail.PlayerGuid);
            MailDraft* mail = new MailDraft(exMail.Subject, exMail.Body);

            if (exMail.Money)
                mail->AddMoney(exMail.Money);

            for (auto const& [itemID, itemCount] : items)
            {
                if (Item* mailItem = Item::CreateItem(itemID, itemCount))
                {
                    mailItem->SaveToDB(trans);
                    mail->AddItem(mailItem);
                }
            }

            mail->SendMailTo(trans, receiver ? receiver : MailReceiver(exMail.PlayerGuid.GetCounter()), MailSender(MAIL_CREATURE, exMail.CreatureEntry, MAIL_STATIONERY_DEFAULT), MAIL_CHECK_MASK_RETURNED);
            delete mail;
        }

        trans->Append("DELETE FROM mail_external WHERE id = {}", exMail.ID);
    }

    CharacterDatabase.CommitTransaction(trans);

    LOG_DEBUG("mail.external", "> External Mail: Отправлено ({}) писем", static_cast<uint32>(_store.size()));
    LOG_DEBUG("mail.external", "");

    // Clear for next time
    _store.clear();
}
