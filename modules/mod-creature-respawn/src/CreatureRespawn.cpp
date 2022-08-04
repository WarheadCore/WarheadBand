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

#include "CreatureRespawn.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "ObjectMgr.h"
#include "MapMgr.h"
#include "StopWatch.h"
#include "ScriptedGossip.h"
#include "GameLocale.h"
#include "Player.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "ModuleLocale.h"
#include "StringConvert.h"
#include "Vip.h"
#include <ranges>

CreatureRespawnMgr* CreatureRespawnMgr::instance()
{
    static CreatureRespawnMgr instance;
    return &instance;
}

void CreatureRespawnMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("CreatureRespawn.Enable");
    if (!_isEnable)
        return;

    _vipDiscountLevel1 = MOD_CONF_GET_UINT("CreatureRespawn.VipDiscount.Level1");
    _vipDiscountLevel2 = MOD_CONF_GET_UINT("CreatureRespawn.VipDiscount.Level2");
    _vipDiscountLevel3 = MOD_CONF_GET_UINT("CreatureRespawn.VipDiscount.Level3");
}

void CreatureRespawnMgr::Initialize()
{
    if (!_isEnable)
        return;

    LoadDataFromDB();
}

void CreatureRespawnMgr::LoadDataFromDB()
{
    if (!_isEnable)
        return;

    LoadCreatureListFromDB();
    LoadCreatureItemsFromDB();
}

void CreatureRespawnMgr::LoadCreatureListFromDB()
{
    if (!_isEnable)
        return;

    LOG_INFO("module", "Loading creatures for respawn...");

    StopWatch sw;
    _creatureList.clear();

    auto result = WorldDatabase.Query("SELECT `SpawnID`, `Enable`, `KillAnounceEnable`, `RespawnAnounceEnable` FROM `wh_creature_respawn`");
    if (!result)
    {
        LOG_ERROR("module", "> Not found data from `wh_creature_respawn`. Disable module");
        LOG_INFO("module", "");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [spawnID, isEnable, killAnounceEnable, respawnAnounceEnable] = result->FetchTuple<uint32, bool, bool, bool>();
        if (!isEnable)
            continue;

        if (!FindCreature(spawnID))
            continue;

        _creatureList.emplace(spawnID, CreatureRespawnInfo{ spawnID, {}, killAnounceEnable, respawnAnounceEnable });
    } while (result->NextRow());

    if (_creatureList.empty())
    {
        LOG_ERROR("module", "> Not found enabled data from `wh_creature_respawn`. Disable module");
        _isEnable = false;
        return;
    }

    LOG_INFO("module", ">> Loaded {} creatures for respawn in {}", _creatureList.size(), sw);
    LOG_INFO("module", "");
}

void CreatureRespawnMgr::LoadCreatureItemsFromDB()
{
    if (!_isEnable)
        return;

    LOG_INFO("module", "Loading creature respawn items...");

    StopWatch sw;

    for (auto& [spawnId, info] : _creatureList)
        info.Items.clear();

    auto result = WorldDatabase.Query("SELECT `SpawnID`, `ItemID`, `ItemCount`, `DecreaseSeconds` FROM `wh_creature_respawn_items`");
    if (!result)
    {
        LOG_ERROR("module", "> Not found data from `wh_creature_respawn_items`");
        LOG_INFO("module", "");
        return;
    }

    auto GetDiscont = [](uint32 value, uint32 discont)
    {
        uint32 total = value - value * discont / 100;

        if (total <= 0)
            total = 1;

        return total;
    };

    std::size_t count{ 0 };

    do
    {
        auto const& [spawnID, itemID, itemCount, seconds] = result->FetchTuple<uint32, uint32, uint32, Seconds>();

        if (!sObjectMgr->GetItemTemplate(itemID))
        {
            LOG_ERROR("module", "> CreatureRespawnMgr: Not found item with id: {}", itemID);
            continue;
        }

        auto info = GetInfo(spawnID);
        if (!info)
        {
            LOG_ERROR("module", "> CreatureRespawnMgr: Not found info for creature with spawn id: {}", spawnID);
            continue;
        }

        std::array<uint32, 4> itemCounts
        {
            itemCount,
            GetDiscont(itemCount, _vipDiscountLevel1),
            GetDiscont(itemCount, _vipDiscountLevel2),
            GetDiscont(itemCount, _vipDiscountLevel3)
        };

        info->Items.emplace_back(itemID, std::move(itemCounts), seconds);
        count++;
    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} creature respawn items in {}", count, sw);
    LOG_INFO("module", "");

    for (auto& [spawnId, info] : _creatureList)
        std::ranges::sort(info.Items, {}, &CreatureRespawnInfoItems::DecreaseSeconds);
}

Creature* CreatureRespawnMgr::FindCreature(uint32 spawnGuid)
{
    // #1 - check cache
    auto const& itr = _creatureCache.find(spawnGuid);
    if (itr != _creatureCache.end())
        return itr->second;

    // #2 - not found in cache, try find default
    auto const& creatureData = sObjectMgr->GetCreatureData(spawnGuid);
    if (!creatureData)
    {
        LOG_ERROR("module", "> CreatureRespawnMgr: Not found creature data with spawn id: {}", spawnGuid);
        return nullptr;
    }

    Map* map = sMapMgr->FindBaseMap(creatureData->mapid);
    if (!map)
    {
        LOG_ERROR("module", "> CreatureRespawnMgr: Not found map in creature data with spawn id: {}. Map id: {}", spawnGuid, creatureData->mapid);
        return nullptr;
    }

    map->LoadGrid(creatureData->posX, creatureData->posY);

    auto const creBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnGuid);
    if (creBounds.first == creBounds.second)
    {
        LOG_ERROR("module", "> CreatureRespawnMgr: Not found creatures in map with spawn id: {}. Map id: {}", spawnGuid, creatureData->mapid);
        return nullptr;
    }

    _creatureCache.emplace(spawnGuid, creBounds.first->second);
    return creBounds.first->second;
}

CreatureRespawnInfo* CreatureRespawnMgr::GetInfo(uint32 spawnGuid)
{
    return Warhead::Containers::MapGetValuePtr(_creatureList, spawnGuid);
}

// Hooks
void CreatureRespawnMgr::OnGossipHello(Player* player, Creature* creature)
{
    if (!_isEnable)
        return;

    ClearGossipMenuFor(player);

    int8 localeIndex = player->GetSession()->GetSessionDbLocaleIndex();
    auto currentTime = GameTime::GetGameTime().count();

    std::size_t count{ 0 };

    for (auto const& [spawnId, __] : _creatureList)
    {
        // Support only 32 creature
        if (count >= GOSSIP_MAX_MENU_ITEMS)
            break;

        auto const& creatureData = sObjectMgr->GetCreatureData(spawnId);
        auto creatureName = sGameLocale->GetCreatureNamelocale(creatureData->id1, localeIndex);
        std::string curRespawnDelayStr{ "<unknown>" };
        uint32 action{ 0 };

        auto const& creature = FindCreature(spawnId);
        if (creature)
        {
            if (creature->IsAlive())
                curRespawnDelayStr = "Alive";
            else
            {
                int64 curRespawnDelay = creature->GetRespawnTimeEx() - currentTime;
                if (curRespawnDelay < 0)
                    curRespawnDelay = 0;

                curRespawnDelayStr = Warhead::Time::ToTimeString(Seconds(curRespawnDelay));
                action = spawnId;
            }
        }

        AddGossipItemFor(player, GOSSIP_ICON_DOT, Warhead::StringFormat("{} | {}", creatureName, curRespawnDelayStr), GOSSIP_SENDER_MAIN, action);
        count++;
    }

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

void CreatureRespawnMgr::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (!_isEnable)
        return;

    ClearGossipMenuFor(player);

    int8 localeIndex = player->GetSession()->GetSessionDbLocaleIndex();
    uint32 itemCountIndex{ 0 };

    if (sVip->IsVip(player))
    {
        auto vipLevel = sVip->GetLevel(player);
        itemCountIndex += vipLevel;
    }

    auto spawnID = action;
    if (!spawnID)
    {
        OnGossipHello(player, creature);
        return;
    }

    AddGossipItemFor(player, GOSSIP_ICON_DOT, GetCreatureName(spawnID, localeIndex), GOSSIP_SENDER_MAIN, 0);
    AddGossipItemFor(player, GOSSIP_ICON_DOT, "---", GOSSIP_SENDER_MAIN, 0);

    auto info = GetInfo(spawnID);
    if (info && !info->Items.empty())
    {
        for (auto const& [itemID, itemCounts, seconds] : info->Items)
        {
            std::string itemLink = sGameLocale->GetItemLink(itemID, localeIndex);
            std::string timeString = Warhead::Time::ToTimeString(seconds);
            AddGossipItemFor(player, GOSSIP_ICON_DOT, Warhead::StringFormat("{}. {} - {}", timeString, itemCounts.at(itemCountIndex), itemLink), spawnID, itemID, "", 0, true);
        }
    }

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

void CreatureRespawnMgr::OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, std::string_view code)
{
    CloseGossipMenuFor(player);

    auto count = Warhead::StringTo<int32>(code);
    if (!count)
        return;

    auto spawnID = sender;
    auto enterCount = *count;

    auto toReapawnCreature = FindCreature(spawnID);
    if (!toReapawnCreature)
        return;

    auto info = GetInfo(spawnID);
    if (!info || info->Items.empty())
        return;

    auto itemID = action;
    int8 localeIndex = player->GetSession()->GetSessionDbLocaleIndex();
    uint32 itemCount{ 0 };
    Seconds seconds{ 0 };
    uint32 itemCountIndex{ 0 };

    if (sVip->IsVip(player))
    {
        auto vipLevel = sVip->GetLevel(player);
        itemCountIndex += vipLevel;
    }

    for (auto const& [_itemID, itemCounts, _seconds] : info->Items)
    {
        if (itemID == _itemID)
        {
            itemCount = itemCounts.at(itemCountIndex);
            seconds = _seconds;
            break;
        }
    }

    if (!itemCount || seconds == 0s)
        return;

    auto totalCount{ itemCount * enterCount };
    std::string itemLink{ sGameLocale->GetItemLink(itemID, localeIndex) };
    auto creatureName{ GetCreatureName(spawnID, localeIndex) };

    if (!player->HasItemCount(itemID, totalCount))
    {
        sModuleLocale->SendPlayerMessage(player, "CR_LOCALE_NOT_ENOUGH", itemLink, totalCount - player->GetItemCount(itemID));
        return;
    }

    auto currentTime = GameTime::GetGameTime();
    auto curRespawnDelay = Seconds(toReapawnCreature->GetRespawnTimeEx()) - currentTime;

    if (toReapawnCreature->IsAlive() || curRespawnDelay == 0s)
    {
        sModuleLocale->SendPlayerMessage(player, "CR_LOCALE_NPC_IS_ALIVE", creatureName);
        return;
    }

    auto totalDescreaseSeconds{ seconds * enterCount };

    if (totalDescreaseSeconds > curRespawnDelay && enterCount > 1)
    {
        auto decreaseSecondsPrew{ seconds * (enterCount - 1) };
        if (decreaseSecondsPrew > curRespawnDelay)
        {
            sModuleLocale->SendPlayerMessage(player, "CR_LOCALE_MORE_TIME", Warhead::Time::ToTimeString(totalDescreaseSeconds), Warhead::Time::ToTimeString(curRespawnDelay));
            return;
        }
    }

    player->DestroyItemCount(itemID, totalCount, true);
    DecreaseRespawnTimeForCreature(toReapawnCreature, totalDescreaseSeconds);
    sModuleLocale->SendPlayerMessage(player, "CR_LOCALE_YOU_DECREASED_TIME", creatureName, Warhead::Time::ToTimeString(totalDescreaseSeconds));
}

void CreatureRespawnMgr::OnCreatureRespawn(Creature* creature)
{
    if (!_isEnable)
        return;

    auto spawnID = creature->GetSpawnId();
    if (!spawnID)
        return;

    auto info = GetInfo(spawnID);
    if (!info)
        return;

    if (!info->IsRespawnAnnounceEnable)
        return;

    sModuleLocale->SendGlobalMessageFmt(false, [creature](uint8 localeIndex)
    {
        auto creatureName = sGameLocale->GetCreatureNamelocale(creature->GetEntry(), localeIndex);
        return sModuleLocale->GetLocaleMessage("CR_LOCALE_NPC_RESPAWN", localeIndex, creatureName);
    });
}

void CreatureRespawnMgr::OnCreatureKill(Player* player, Creature* creature)
{
    if (!_isEnable)
        return;

    auto spawnID = creature->GetSpawnId();
    if (!spawnID)
        return;

    auto info = GetInfo(spawnID);
    if (!info)
        return;

    if (!info->IsKillAnnounceEnable)
        return;

    sModuleLocale->SendGlobalMessageFmt(false, [player, creature](uint8 localeIndex)
    {
        auto creatureName = sGameLocale->GetCreatureNamelocale(creature->GetEntry(), localeIndex);
        return sModuleLocale->GetLocaleMessage("CR_LOCALE_NPC_KILL", localeIndex, player->GetName(), creatureName);
    });
}

std::string CreatureRespawnMgr::GetCreatureName(uint32 spawnID, uint8 localeIndex)
{
    auto const& creatureData = sObjectMgr->GetCreatureData(spawnID);
    return sGameLocale->GetCreatureNamelocale(creatureData->id1, localeIndex);
}

void CreatureRespawnMgr::DecreaseRespawnTimeForCreature(Creature* creature, Seconds seconds)
{
    if (!creature || seconds == 0s)
        return;

    auto currentTime = GameTime::GetGameTime();
    auto curRespawnDelay = Seconds(creature->GetRespawnTimeEx()) - currentTime;

    if (curRespawnDelay <= 10s || curRespawnDelay <= seconds)
    {
        creature->Respawn();
        return;
    }

    auto newTime = curRespawnDelay - seconds;
    creature->SetRespawnTime(newTime.count());
}
