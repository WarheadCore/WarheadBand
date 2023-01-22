/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "MailMgr.h"
#include "AuctionHouseMgr.h"
#include "BattlegroundMgr.h"
#include "CalendarMgr.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

GameMail::~GameMail()
{
    if (_items.empty())
        return;

    for (auto const& [itemLowGuid, item] : _items)
        delete item;
}

void GameMail::BuildSender(MailMessageType messageType, uint32 guidLowOrEntry, MailStationery stationery /*= MAIL_STATIONERY_DEFAULT*/)
{
    MessageType = messageType;
    Sender = guidLowOrEntry;
    Stationery = stationery;
}

void GameMail::BuildSender(Object* sender, MailStationery stationery /*= MAIL_STATIONERY_DEFAULT*/)
{
    switch (sender->GetTypeId())
    {
        case TYPEID_UNIT:
            MessageType = MAIL_CREATURE;
            Sender = sender->GetEntry();
            break;
        case TYPEID_GAMEOBJECT:
            MessageType = MAIL_GAMEOBJECT;
            Sender = sender->GetEntry();
            break;
        case TYPEID_PLAYER:
            MessageType = MAIL_NORMAL;
            Sender = sender->GetGUID().GetCounter();
            break;
        default:
//            MessageType = MAIL_NORMAL;
//            Sender = 0; // will show mail from not existed player
            LOG_ERROR("mail", "MailSender::MailSender - Mail have unexpected sender typeid ({})", sender->GetTypeId());
            break;
    }
}

void GameMail::BuildSender(CalendarEvent* sender)
{
    MessageType = MAIL_CALENDAR;
    Sender = sender->GetEventId();
}

void GameMail::BuildSender(AuctionEntry* sender)
{
    MessageType = MAIL_AUCTION;
    Sender = sender->GetHouseId();
    Stationery = MAIL_STATIONERY_AUCTION;
}

void GameMail::BuildSender(Player* sender)
{
    Stationery = sender->IsGameMaster() ? MAIL_STATIONERY_GM : MAIL_STATIONERY_DEFAULT;
    Sender = sender->GetGUID().GetCounter();
}

void GameMail::BuildSender(uint32 senderEntry)
{
    MessageType = MAIL_CREATURE;
    Sender = senderEntry;
}

void GameMail::BuildExpireTime(Seconds delay /*= 0s*/, bool withItems /*= false*/)
{
    DeliverTime = GameTime::GetGameTime() + delay;

    Seconds expireDelay;

    // auction mail without any items and money
    if (MessageType == MAIL_AUCTION && withItems && !Money)
        expireDelay = Seconds{ CONF_GET_UINT("MailDeliveryDelay") };
        // mail from battlemaster (rewardmarks) should last only one day
    else if (MessageType == MAIL_CREATURE && sBattlegroundMgr->GetBattleMasterBG(Sender) != BATTLEGROUND_TYPE_NONE)
        expireDelay = 1_days;
        // default case: expire time if COD 3 days, if no COD 30 days (or 90 days if sender is a game master)
    else if (COD)
        expireDelay = 3_days;
    else
    {
        Player* playerSender = ObjectAccessor::FindPlayerByLowGUID(Sender);
        expireDelay = playerSender && playerSender->GetSession()->GetSecurity() ? 90_days : 30_days;
    }

    ExpireTime = DeliverTime + expireDelay;
}

void GameMail::SaveMailToDB(CharacterDatabaseTransaction trans)
{
    bool hasItems = _items.empty();

    // Add to DB
    uint8 index = 0;
    CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_MAIL);
    stmt->SetData(  index, MessageID);
    stmt->SetData(++index, uint8(MessageType));
    stmt->SetData(++index, int8(Stationery));
    stmt->SetData(++index, MailTemplateId);
    stmt->SetData(++index, Sender);
    stmt->SetData(++index, Receiver);
    stmt->SetData(++index, Subject);
    stmt->SetData(++index, Body);
    stmt->SetData(++index, hasItems);
    stmt->SetData(++index, ExpireTime);
    stmt->SetData(++index, DeliverTime);
    stmt->SetData(++index, Money.GetCopper());
    stmt->SetData(++index, COD.GetCopper());
    stmt->SetData(++index, uint8(Checked));
    trans->Append(stmt);

    if (!hasItems)
        return;

    for (auto const& [itemGuid, item] : _items)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_MAIL_ITEM);
        stmt->SetData(0, MessageID);
        stmt->SetData(1, itemGuid);
        stmt->SetData(2, Receiver);
        trans->Append(stmt);
    }
}

void GameMail::AddMailItem(Item* item)
{
    auto lowGuid{ item->GetGUID().GetCounter() };
    ASSERT(!_items.contains(lowGuid));
    _items.emplace(lowGuid, item);
}

bool GameMail::DeleteMailItem(ObjectGuid::LowType lowGuid, bool includeDB /*= false*/)
{
    auto item{ Warhead::Containers::MapGetValuePtr(_items, lowGuid) };
    if (!item)
        return false;

    if (includeDB)
    {
        CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
        stmt->SetArguments(lowGuid);
        CharacterDatabase.Execute(stmt);
    }

    delete item;
    return _items.erase(lowGuid) > 0;
}

MailMgr* MailMgr::instance()
{
    static MailMgr instance;
    return &instance;
}

void MailMgr::Update(Milliseconds diff)
{
//    scheduler.Update(diff);
}

std::shared_ptr<GameMail> MailMgr::CreateGameMail()
{
    return std::make_shared<GameMail>();
}

void MailMgr::AddMailItem(uint32 mailID, ObjectGuid::LowType lowGuid, uint32 itemID)
{
    auto const& itr = _mailItems.find(mailID);
    if (itr == _mailItems.end())
    {
        _mailItems.emplace(mailID, MailItems{ { lowGuid, itemID } });
        return;
    }

    itr->second.emplace(lowGuid, itemID);
}

void MailMgr::AddMailItems(std::shared_ptr<GameMail> mail)
{
    if (!mail || !mail->MessageID || mail->GetItems().empty())
        return;

    auto const& itr = _mailItems.find(mail->MessageID);
    if (itr == _mailItems.end())
    {
        MailItems items;

        for (auto const& [itemLowGuid, item] : mail->GetItems())
            items.emplace(itemLowGuid, item->GetEntry());

        _mailItems.emplace(mail->MessageID, std::move(items));
        return;
    }

    for (auto const& [itemLowGuid, item] : mail->GetItems())
        itr->second.emplace(itemLowGuid, item->GetEntry());
}

void MailMgr::DeleteMailItem(uint32 mailID, ObjectGuid::LowType lowGuid)
{
    auto items{ Warhead::Containers::MapGetValuePtr(_mailItems, mailID) };
    if (!items)
        return;

    items->erase(lowGuid);
}

void MailMgr::AddMailForPlayer(Player* player, std::shared_ptr<GameMail> mail)
{
    if (!player || !mail)
        return;

    auto playerGuid{ player->GetGUID() };

    auto const& itr = _mails.find(playerGuid);
    if (itr == _mails.end())
    {
        _mails.emplace(playerGuid, std::vector{ mail });
        return;
    }

    itr->second.emplace_back(mail);
}

void MailMgr::SendMail(CharacterDatabaseTransaction trans, std::shared_ptr<GameMail> mail)
{
    mail->MessageID = sObjectMgr->GenerateMailID();

    auto receiverGuid{ ObjectGuid(HighGuid::Player, mail->Receiver) };
    auto receiverGuidLow = receiverGuid.GetCounter();
    auto playerReceiver = ObjectAccessor::FindPlayer(receiverGuid); // can be nullptr

    PrepareItemsForPlayer(playerReceiver, mail, trans);

    // Add to DB
    mail->SaveMailToDB(trans);

    // Update players cache
    sCharacterCache->IncreaseCharacterMailCount(receiverGuid);

    // For online receiver update in game mail status and data
    if (playerReceiver)
    {
        playerReceiver->AddNewMailDeliverTime(mail->DeliverTime.count());

        AddMailItems(mail);
        AddMailForPlayer(playerReceiver, mail);

        pReceiver->AddMail(m);                           // to insert new mail to beginning of maillist

        if (!m_items.empty())
        {
            for (MailItemMap::iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
            {
                pReceiver->AddMItem(mailItemIter->second);
            }
        }
    }
    else if (!m_items.empty())
    {
        deleteIncludedItems(CharacterDatabaseTransaction(nullptr));
    }
}

void MailMgr::PrepareItemsForPlayer(Player* player, std::shared_ptr<GameMail> mail, CharacterDatabaseTransaction trans)
{
    if (!player || !mail->MailTemplateId || !mail->IsCanPrepareItems)
        return;

    mail->IsCanPrepareItems = false;

    // The mail sent after turning in the quest The Wrath of Neptulon contains 100g
    // Only quest in the game up to BFA which sends raw gold through mail. So would just be overkill to introduce a new column in the database.
    if (mail->MailTemplateId == 123)
        mail->Money = 100_gold;

    Loot mailLoot;

    // can be empty
    mailLoot.FillLoot(mail->MailTemplateId, LootTemplates_Mail, player, true, true);

    uint32 max_slot = mailLoot.GetMaxSlotInLootFor(player);
    for (uint32 i = 0; mail->GetItems().size() < MAX_MAIL_ITEMS && i < max_slot; ++i)
    {
        if (LootItem* lootitem = mailLoot.LootItemInSlot(i, player))
        {
            if (Item* item = Item::CreateItem(lootitem->itemid, lootitem->count, player))
            {
                item->SaveToDB(trans);                           // save for prevent lost at next mail load, if send fail then item will deleted
                mail->AddMailItem(item);
            }
        }
    }
}
