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

#include "LootItemStorage.h"
#include "ObjectMgr.h"
#include "PreparedStatement.h"
#include <time.h>

LootItemStorage::LootItemStorage()
{
}

LootItemStorage::~LootItemStorage()
{
}

LootItemStorage* LootItemStorage::instance()
{
    static LootItemStorage instance;
    return &instance;
}

void LootItemStorage::LoadStorageFromDB()
{
    uint32 oldMSTime = getMSTime();
    lootItemStore.clear();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEMCONTAINER_ITEMS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        LOG_INFO("server", ">>  Loaded 0 stored items!");
        LOG_INFO("server", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        StoredLootItemList& itemList = lootItemStore[ObjectGuid::Create<HighGuid::Item>(fields[0].GetUInt32())];
        itemList.push_back(StoredLootItem(fields[1].GetUInt32(), fields[2].GetUInt32(), fields[3].GetInt32(), fields[4].GetUInt32()));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server", ">> Loaded %d stored items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server", " ");
}

void LootItemStorage::RemoveEntryFromDB(ObjectGuid containerGUID, uint32 itemid, uint32 count)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEMCONTAINER_SINGLE_ITEM);
    stmt->setUInt32(0, containerGUID.GetCounter());
    stmt->setUInt32(1, itemid);
    stmt->setUInt32(2, count);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void LootItemStorage::AddNewStoredLoot(Loot* loot, Player* /*player*/)
{
    if (lootItemStore.find(loot->containerGUID) != lootItemStore.end())
    {
        LOG_INFO("misc", "LootItemStorage::AddNewStoredLoot (A1) - %s!", loot->containerGUID.ToString().c_str());
        return;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = nullptr;

    StoredLootItemList& itemList = lootItemStore[loot->containerGUID];

    // Gold at first
    if (loot->gold)
    {
        itemList.push_back(StoredLootItem(0, loot->gold, 0, 0));

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEMCONTAINER_SINGLE_ITEM);
        stmt->setUInt32(0, loot->containerGUID.GetCounter());
        stmt->setUInt32(1, 0);
        stmt->setUInt32(2, loot->gold);
        stmt->setInt32(3, 0);
        stmt->setUInt32(4, 0);
        trans->Append(stmt);
    }

    // And normal items
    if (!loot->isLooted())
        for (LootItemList::const_iterator li = loot->items.begin(); li != loot->items.end(); li++)
        {
            // Even if an item is not available for a specific player, it doesn't mean that
            // we are not able to trade this container to another player that is able to loot that item
            // if we don't save it then the item will be lost at player re-login.
            //if (!li->AllowedForPlayer(player))
            //    continue;

            const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(li->itemid);
            if (!itemTemplate || itemTemplate->IsCurrencyToken())
                continue;

            itemList.push_back(StoredLootItem(li->itemid, li->count, li->randomPropertyId, li->randomSuffix));

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ITEMCONTAINER_SINGLE_ITEM);
            stmt->setUInt32(0, loot->containerGUID.GetCounter());
            stmt->setUInt32(1, li->itemid);
            stmt->setUInt32(2, li->count);
            stmt->setInt32 (3, li->randomPropertyId);
            stmt->setUInt32(4, li->randomSuffix);
            trans->Append(stmt);
        }

    CharacterDatabase.CommitTransaction(trans);
}

bool LootItemStorage::LoadStoredLoot(Item* item)
{
    Loot* loot = &item->loot;
    LootItemContainer::iterator itr = lootItemStore.find(loot->containerGUID);
    if (itr == lootItemStore.end())
        return false;

    StoredLootItemList& itemList = itr->second;
    for (StoredLootItemList::iterator it2 = itemList.begin(); it2 != itemList.end(); ++it2)
    {
        if (it2->itemid == 0)
        {
            loot->gold = it2->count;
            continue;
        }

        LootItem li;
        li.itemid = it2->itemid;
        li.count = it2->count;
        li.follow_loot_rules = false;
        li.freeforall = false;
        li.is_blocked = false;
        li.is_counted = false;
        li.is_underthreshold = false;
        li.is_looted = false;
        li.needs_quest = false;
        li.randomPropertyId = it2->randomPropertyId;
        li.randomSuffix = it2->randomSuffix;
        li.rollWinnerGUID = ObjectGuid::Empty;

        loot->items.push_back(li);
        loot->unlootedCount++;
    }

    // Mark the item if it has loot so it won't be generated again on open
    item->m_lootGenerated = true;
    return true;
}

void LootItemStorage::RemoveStoredLootItem(ObjectGuid containerGUID, uint32 itemid, uint32 count, Loot* loot)
{
    LootItemContainer::iterator itr = lootItemStore.find(containerGUID);
    if (itr == lootItemStore.end())
        return;

    StoredLootItemList& itemList = itr->second;
    for (StoredLootItemList::iterator it2 = itemList.begin(); it2 != itemList.end(); ++it2)
        if (it2->itemid == itemid && it2->count == count)
        {
            RemoveEntryFromDB(containerGUID, itemid, count);
            itemList.erase(it2);
            break;
        }

    // loot with empty itemList but unlootedCount > 0
    // must be deleted manually by the player or traded
    if (!loot->unlootedCount)
        lootItemStore.erase(itr);
}

void LootItemStorage::RemoveStoredLootMoney(ObjectGuid containerGUID, Loot* loot)
{
    LootItemContainer::iterator itr = lootItemStore.find(containerGUID);
    if (itr == lootItemStore.end())
        return;

    StoredLootItemList& itemList = itr->second;
    for (StoredLootItemList::iterator it2 = itemList.begin(); it2 != itemList.end(); ++it2)
        if (it2->itemid == 0)
        {
            RemoveEntryFromDB(containerGUID, 0, it2->count);
            itemList.erase(it2);
            break;
        }

    // loot with empty itemList but unlootedCount > 0
    // must be deleted manually by the player or traded
    if (!loot->unlootedCount)
        lootItemStore.erase(itr);
}

void LootItemStorage::RemoveStoredLoot(ObjectGuid containerGUID)
{
    lootItemStore.erase(containerGUID);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEMCONTAINER_CONTAINER);
    stmt->setUInt32(0, containerGUID.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}
