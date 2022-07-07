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

#include "Reforge.h"
#include "Log.h"
#include "Player.h"
#include "Item.h"
#include "GameLocale.h"
#include "StopWatch.h"

void ReforgeItem::Reset()
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(_id);
    if (!itemTemplate)
        return;

    for (size_t i = 0; i < MAX_ITEM_PROTO_STATS; i++)
    {
        _itemStats[i].ItemStatType = itemTemplate->ItemStat[i].ItemStatType;
        _itemStats[i].ItemStatValue = itemTemplate->ItemStat[i].ItemStatValue;
    }

    _itemStatsCount = itemTemplate->StatsCount;
}

bool ReforgeItem::AddStat(uint32 statType, int32 value)
{
    for (auto& [itemStatType, itemStatValue] : _itemStats)
    {
        if (itemStatType == statType)
        {
            itemStatValue += value;
            return true;
        }
    }

    if (_itemStatsCount == MAX_ITEM_PROTO_STATS)
        return false;

    _itemStats[_itemStatsCount].ItemStatType = statType;
    _itemStats[_itemStatsCount].ItemStatValue = value;
    _itemStatsCount++;
}

bool ReforgeItem::SetStat(uint32 statType, int32 value)
{
    for (auto& [itemStatType, itemStatValue] : _itemStats)
    {
        if (itemStatType == statType)
        {
            itemStatValue = value;
            return true;
        }
    }

    if (_itemStatsCount == MAX_ITEM_PROTO_STATS)
        return false;

    _itemStats[_itemStatsCount].ItemStatType = statType;
    _itemStats[_itemStatsCount].ItemStatValue = value;
    _itemStatsCount++;
}

int32 ReforgeItem::GetValueForStat(uint32 statType) const
{
    for (auto const& [itemStatType, itemStatValue] : _itemStats)
        if (itemStatType == statType)
            return itemStatValue;

    LOG_ERROR("module", "> {}: Not found stat type {}", __FUNCTION__, statType);
    return 0;
}

void ReforgeItem::SaveToDB()
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(_id);
    if (!itemTemplate)
        return;

    auto trans = CharacterDatabase.BeginTransaction();

    trans->Append("DELETE FROM `wh_items_reforge` WHERE `Guid` = {}", _guid.GetRawValue());

    for (size_t i = 0; i < MAX_ITEM_PROTO_STATS; i++)
    {
        auto const& statType = _itemStats[i].ItemStatType;
        auto const& statValue = _itemStats[i].ItemStatValue;

        if (statType == itemTemplate->ItemStat[i].ItemStatType && statValue == itemTemplate->ItemStat[i].ItemStatValue)
            continue;

        trans->Append("INSERT INTO `wh_items_reforge` (`Guid`, `Entry`, `StatType`, `StatValue`) VALUES ({}, {}, {}, {})",
            _guid.GetRawValue(), _id, statType, statValue);
    }

    CharacterDatabase.CommitTransaction(trans);
}

void ReforgeMgr::OnBeforeApplyItemBonuses(Player* player, Item* item, uint32& statsCount)
{
    if (!player || !item || statsCount >= MAX_ITEM_PROTO_STATS)
        return;

    auto const& reforgeItem = GetReforgeItem(item->GetGUID());
    if (!reforgeItem)
        return;

    if (reforgeItem->GetStatsCount() == statsCount)
        return;

    statsCount = reforgeItem->GetStatsCount();
}

void ReforgeMgr::OnApplyItemBonuses(Player* player, Item* item, uint32 statIndex, uint32& statType, int32& value)
{
    if (!player || !item)
        return;

    auto const& reforgeItem = GetReforgeItem(item->GetGUID());
    if (!reforgeItem)
        return;

    auto itemStats = reforgeItem->GetItemsStatsForIndex(statIndex);
    if (!itemStats || !itemStats->ItemStatType)
        return;

    statType = itemStats->ItemStatType;
    value = itemStats->ItemStatValue;
}

bool ReforgeMgr::ChangeItem(Player* player, Item* item, uint32 statType, int32 value)
{
    if (item->IsEquipped())
        player->_ApplyItemMods(item, item->GetSlot(), false);

    auto reforgeItem = GetReforgeItem(item->GetGUID());
    if (!reforgeItem)
    {
        CreateBaseReforgeItem(item);
        reforgeItem = GetReforgeItem(item->GetGUID());

        if (!reforgeItem)
            return false;
    }

    if (!reforgeItem->AddStat(statType, value))
        return false;

    reforgeItem->SaveToDB();

    if (item->IsEquipped())
        player->_ApplyItemMods(item, item->GetSlot(), true);

    UpdateItemForPlayer(player, item);
    return true;
}

void ReforgeMgr::UpdateItemForPlayer(Player* player, Item* item)
{
    auto const& reforgeItem = GetReforgeItem(item->GetGUID());
    if (!reforgeItem)
        return;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item->GetEntry());
    if (!itemTemplate)
        return;

    std::string Name = itemTemplate->Name1;
    std::string Description = itemTemplate->Description;

    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    if (loc_idx >= 0)
    {
        if (ItemLocale const* il = sGameLocale->GetItemLocale(itemTemplate->ItemId))
        {
            GameLocale::GetLocaleString(il->Name, loc_idx, Name);
            GameLocale::GetLocaleString(il->Description, loc_idx, Description);
        }
    }

    // guess size
    WorldPacket queryData(SMSG_ITEM_QUERY_SINGLE_RESPONSE, 600);
    queryData << itemTemplate->ItemId;
    queryData << itemTemplate->Class;
    queryData << itemTemplate->SubClass;
    queryData << itemTemplate->SoundOverrideSubclass;
    queryData << Name;
    queryData << uint8(0x00);                                //pProto->Name2; // blizz not send name there, just uint8(0x00); <-- \0 = empty string = empty name...
    queryData << uint8(0x00);                                //pProto->Name3; // blizz not send name there, just uint8(0x00);
    queryData << uint8(0x00);                                //pProto->Name4; // blizz not send name there, just uint8(0x00);
    queryData << itemTemplate->DisplayInfoID;
    queryData << itemTemplate->Quality;
    queryData << itemTemplate->Flags;
    queryData << itemTemplate->Flags2;
    queryData << itemTemplate->BuyPrice;
    queryData << itemTemplate->SellPrice;
    queryData << itemTemplate->InventoryType;
    queryData << itemTemplate->AllowableClass;
    queryData << itemTemplate->AllowableRace;
    queryData << itemTemplate->ItemLevel;
    queryData << itemTemplate->RequiredLevel;
    queryData << itemTemplate->RequiredSkill;
    queryData << itemTemplate->RequiredSkillRank;
    queryData << itemTemplate->RequiredSpell;
    queryData << itemTemplate->RequiredHonorRank;
    queryData << itemTemplate->RequiredCityRank;
    queryData << itemTemplate->RequiredReputationFaction;
    queryData << itemTemplate->RequiredReputationRank;
    queryData << int32(itemTemplate->MaxCount);
    queryData << int32(itemTemplate->Stackable);
    queryData << itemTemplate->ContainerSlots;
    queryData << reforgeItem->GetStatsCount(); // item stats count

    for (auto const& [statType, statValue] : *reforgeItem->GetStats())
    {
        if (!statType)
            continue;

        //LOG_INFO("module", "> {}: Item {}. {}/{}", __FUNCTION__, item->GetGUID().ToString(), statType, statValue);

        queryData << statType;
        queryData << statValue;
    }

    queryData << itemTemplate->ScalingStatDistribution;            // scaling stats distribution
    queryData << itemTemplate->ScalingStatValue;                   // some kind of flags used to determine stat values column

    for (int i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
    {
        queryData << itemTemplate->Damage[i].DamageMin;
        queryData << itemTemplate->Damage[i].DamageMax;
        queryData << itemTemplate->Damage[i].DamageType;
    }

    // resistances (7)
    queryData << itemTemplate->Armor;
    queryData << itemTemplate->HolyRes;
    queryData << itemTemplate->FireRes;
    queryData << itemTemplate->NatureRes;
    queryData << itemTemplate->FrostRes;
    queryData << itemTemplate->ShadowRes;
    queryData << itemTemplate->ArcaneRes;

    queryData << itemTemplate->Delay;
    queryData << itemTemplate->AmmoType;
    queryData << itemTemplate->RangedModRange;

    for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
    {
        // send DBC data for cooldowns in same way as it used in Spell::SendSpellCooldown
        // use `item_template` or if not set then only use spell cooldowns
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(itemTemplate->Spells[s].SpellId);
        if (spell)
        {
            bool db_data = itemTemplate->Spells[s].SpellCooldown >= 0 || itemTemplate->Spells[s].SpellCategoryCooldown >= 0;

            queryData << itemTemplate->Spells[s].SpellId;
            queryData << itemTemplate->Spells[s].SpellTrigger;
            queryData << int32(itemTemplate->Spells[s].SpellCharges);

            if (db_data)
            {
                queryData << uint32(itemTemplate->Spells[s].SpellCooldown);
                queryData << uint32(itemTemplate->Spells[s].SpellCategory);
                queryData << uint32(itemTemplate->Spells[s].SpellCategoryCooldown);
            }
            else
            {
                queryData << uint32(spell->RecoveryTime);
                queryData << uint32(spell->GetCategory());
                queryData << uint32(spell->CategoryRecoveryTime);
            }
        }
        else
        {
            queryData << uint32(0);
            queryData << uint32(0);
            queryData << uint32(0);
            queryData << uint32(-1);
            queryData << uint32(0);
            queryData << uint32(-1);
        }
    }
    queryData << itemTemplate->Bonding;
    queryData << Description;
    queryData << itemTemplate->PageText;
    queryData << itemTemplate->LanguageID;
    queryData << itemTemplate->PageMaterial;
    queryData << itemTemplate->StartQuest;
    queryData << itemTemplate->LockID;
    queryData << int32(itemTemplate->Material);
    queryData << itemTemplate->Sheath;
    queryData << itemTemplate->RandomProperty;
    queryData << itemTemplate->RandomSuffix;
    queryData << itemTemplate->Block;
    queryData << itemTemplate->ItemSet;
    queryData << itemTemplate->MaxDurability;
    queryData << itemTemplate->Area;
    queryData << itemTemplate->Map;                                // Added in 1.12.x & 2.0.1 client branch
    queryData << itemTemplate->BagFamily;
    queryData << itemTemplate->TotemCategory;

    for (int s = 0; s < MAX_ITEM_PROTO_SOCKETS; ++s)
    {
        queryData << itemTemplate->Socket[s].Color;
        queryData << itemTemplate->Socket[s].Content;
    }

    queryData << itemTemplate->socketBonus;
    queryData << itemTemplate->GemProperties;
    queryData << itemTemplate->RequiredDisenchantSkill;
    queryData << itemTemplate->ArmorDamageModifier;
    queryData << itemTemplate->Duration;                           // added in 2.4.2.8209, duration (seconds)
    queryData << itemTemplate->ItemLimitCategory;                  // WotLK, ItemLimitCategory
    queryData << itemTemplate->HolidayId;                          // Holiday.dbc?

    player->SendDirectMessage(&queryData);
}

ReforgeItem* ReforgeMgr::GetReforgeItem(ObjectGuid rawGuid)
{
    return Warhead::Containers::MapGetValuePtr(_store, rawGuid.GetRawValue());
}

void ReforgeMgr::CreateBaseReforgeItem(Item* item)
{
    CreateBaseReforgeItem(item->GetGUID(), item->GetEntry());
}

void ReforgeMgr::CreateBaseReforgeItem(ObjectGuid itemGuid, uint32 id)
{
    if (GetReforgeItem(itemGuid))
    {
        LOG_FATAL("module", "> Reforge item is exist. {}", itemGuid.ToString());
        return;
    }

    _store.emplace(itemGuid.GetRawValue(), ReforgeItem{itemGuid, id});
}

void ReforgeMgr::Initialize()
{
    StopWatch sw;
    _store.clear();

    LOG_INFO("server.loading", "Loading reforge items...");

    auto result = CharacterDatabase.Query("SELECT `Guid`, `Entry`. `StatType`, `StatValue` FROM `wh_items_reforge`");
    if (!result)
    {
        LOG_INFO("sql.sql", ">> Loaded 0 reforge items. DB table `wh_items_reforge` is empty.");
        LOG_INFO("server.loading", "");
        return;
    }

    ReforgeItem* reforgeItem{ nullptr };

    do
    {
        auto const& [itemGuid, entry, statType, statValue] = result->FetchTuple<uint64, uint32, uint32, int32>();

        ObjectGuid guid{ itemGuid };

        if (reforgeItem && reforgeItem->GetGUID() != guid)
            reforgeItem = nullptr;

        if (!reforgeItem)
            reforgeItem = GetReforgeItem(guid);

        if (!reforgeItem)
            CreateBaseReforgeItem(guid, entry);

        if (!reforgeItem)
        {
            LOG_FATAL("server.loading", "> Can't find reforge item. {}. Entry {}", guid.ToString(), entry);
            continue;
        }

        reforgeItem->SetStat(statType, statValue);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} reforge items in {}", _store.size(), sw);
    LOG_INFO("server.loading", "");
}
