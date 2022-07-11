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

#include "UnbindInstance.h"
#include "Chat.h"
#include "Creature.h"
#include "GameLocale.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "MapMgr.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "StopWatch.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"

namespace
{
    std::string const GetMapName(uint32 mapID, uint8 localeIndex)
    {
        MapEntry const* map = sMapStore.LookupEntry(mapID);
        if (!map)
            return "Неизвестно";

        return map->name[localeIndex];
    }

    std::string const GetDiffName(Difficulty diff, bool isRaid = true)
    {
        if (isRaid)
        {
            switch (diff)
            {
                case RAID_DIFFICULTY_10MAN_NORMAL:
                    return "10 об.";
                case RAID_DIFFICULTY_25MAN_NORMAL:
                    return "25 об.";
                case RAID_DIFFICULTY_10MAN_HEROIC:
                    return "10 гер.";
                case RAID_DIFFICULTY_25MAN_HEROIC:
                    return "25 гер.";
                default:
                    break;
            }
        }
        else // if dungeon
        {
            switch (diff)
            {
                case DUNGEON_DIFFICULTY_NORMAL:
                    return "5 об.";
                case DUNGEON_DIFFICULTY_HEROIC:
                    return "5 гер.";
                default:
                    break;
            }
        }

        return "###";
    }

    bool IsExistBindDifficulty(Player* player, Difficulty diff, bool isRaid = true)
    {
        uint8 count = 0;

        auto const& boundMap = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), diff);
        if (boundMap.empty())
            return false;

        for (auto const& playerBind : boundMap)
        {
            if (playerBind.second.save->GetMapEntry()->IsRaid() != isRaid)
                continue;

            count++;
        }

        return count ? true : false;
    }

    uint8 GetCountInstanceBind(Player* player, Difficulty diff, bool isRaid = true)
    {
        auto const& boundMap = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), diff);
        if (boundMap.empty())
            return 0;

        uint8 count = 0;

        for (auto const& playerBind : boundMap)
        {
            if (playerBind.second.save->GetMapEntry()->IsRaid() != isRaid)
                continue;

            count++;
        }

        return count;
    }

    bool IsExistBind(Player* player)
    {
        if (IsExistBindDifficulty(player, DUNGEON_DIFFICULTY_HEROIC, false))
            return true;

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            if (IsExistBindDifficulty(player, Difficulty(i)))
                return true;

        return false;
    }
}

/*static*/ UnbindInstance* UnbindInstance::instance()
{
    static UnbindInstance instance;
    return &instance;
}

void UnbindInstance::Init()
{
    if (!_isEnable)
        return;

    LoadCostData();
    LoadDisabledMaps();
}

void UnbindInstance::LoadConfig()
{
    _isEnable = MOD_CONF_GET_BOOL("UnbindInsance.Enable");
    _isEnableCommand = MOD_CONF_GET_BOOL("UI.Command.Enable");
}

void UnbindInstance::LoadCostData()
{
    StopWatch sw;

    _costStore.clear();

    if (!_isEnable)
        return;

    LOG_INFO("module.unbind", "Загрузка вариантов стоимости сброса кд...");

    QueryResult result = CharacterDatabase.Query("SELECT "
                         "`ItemID`,"                 // 0
                         "`CountForDungeonHeroic`,"  // 1
                         "`CountForRaid10Normal`,"   // 2
                         "`CountForRaid25Normal`,"   // 3
                         "`CountForRaid10Heroic`, "  // 4
                         "`CountForRaid25Heroic` "   // 5
                         "FROM `unbind_instance_cost`");

    if (!result)
    {
        LOG_WARN("module.unbind", "> Загружено 0 вариантов стоимости. Таблица `unbind_instance_cost` пустая.");
        LOG_INFO("module.unbind", "");
        return;
    }

    auto IsCorrectItemId = [](uint32 itemID) -> bool
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
        if (!itemTemplate)
        {
            LOG_ERROR("module.unbind", "> UI: ItemID ({}) is not exist!", itemID);
            return false;
        }

        return true;
    };

    auto IsCorrectCount = [](std::initializer_list<uint32> listCount) -> bool
    {
        for (auto const& count : listCount)
            return count ? true : false;

        return false;
    };

    do
    {
        Field* fields = result->Fetch();

        UnbindCost _UIData;
        _UIData.ItemID                  = fields[0].Get<uint32>();
        _UIData.CountForDungeonHeroic   = fields[1].Get<uint32>();
        _UIData.CountForRaid10Normal    = fields[2].Get<uint32>();
        _UIData.CountForRaid25Normal    = fields[3].Get<uint32>();
        _UIData.CountForRaid10Heroic    = fields[4].Get<uint32>();
        _UIData.CountForRaid25Heroic    = fields[5].Get<uint32>();

        // Check item
        if (!IsCorrectItemId(_UIData.ItemID))
            continue;

        if (!IsCorrectCount({ _UIData.CountForRaid10Normal, _UIData.CountForRaid25Normal, _UIData.CountForRaid10Heroic, _UIData.CountForRaid25Heroic }))
            continue;

        _costStore.emplace(_UIData.ItemID, _UIData);
    } while (result->NextRow());

    LOG_INFO("module.unbind", ">> Загружено {} вариантов за {}", _costStore.size(), sw);
    LOG_INFO("module.unbind", "");
}

void UnbindInstance::LoadDisabledMaps()
{
    StopWatch sw;

    _disabledIds.clear();

    if (!_isEnable)
        return;

    LOG_INFO("module.unbind", "Loading disabled maps for unbind instance command...");

    QueryResult result = CharacterDatabase.Query("SELECT `MapID` FROM `unbind_instance_disabled_maps`");

    if (!result)
    {
        LOG_INFO("module.unbind", "> Loaded 0 disabled maps. Table `unbind_instance_cost` is empty.");
        LOG_INFO("module.unbind", "");
        return;
    }

    do
    {
        auto const& [mapID] = result->FetchTuple<uint32>();

        if (!sMapStore.LookupEntry(mapID))
        {
            LOG_ERROR("module.unbind", "> UI::LoadDisabledMaps: Incorrect map {}. Skip", mapID);
            continue;
        }

        _disabledIds.emplace_back(mapID);
    } while (result->NextRow());

    LOG_INFO("module.unbind", ">> Loaded {} disabled maps in {}", _disabledIds.size(), sw);
    LOG_INFO("module.unbind", "");
}

void UnbindInstance::SendGossipHello(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);

    if (!IsExistBind(player))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "-- Вы не имеете кд ни на что", GOSSIP_SENDER_MAIN, 0);

    if (::IsExistBindDifficulty(player, DUNGEON_DIFFICULTY_HEROIC, false))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("[{}] Героические подземелья", ::GetCountInstanceBind(player, DUNGEON_DIFFICULTY_HEROIC, false)), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DUNGEON_HEROIC);

    if (::IsExistBindDifficulty(player, RAID_DIFFICULTY_10MAN_NORMAL))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("[{}] Рейды 10 об.", ::GetCountInstanceBind(player, RAID_DIFFICULTY_10MAN_NORMAL)), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_RAID_10_NORMAL);

    if (::IsExistBindDifficulty(player, RAID_DIFFICULTY_25MAN_NORMAL))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("[{}] Рейды 25 об.", ::GetCountInstanceBind(player, RAID_DIFFICULTY_25MAN_NORMAL)), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_RAID_25_NORMAL);

    if (::IsExistBindDifficulty(player, RAID_DIFFICULTY_10MAN_HEROIC))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("[{}] Рейды 10 гер.", ::GetCountInstanceBind(player, RAID_DIFFICULTY_10MAN_HEROIC)), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_RAID_10_HEROIC);

    if (::IsExistBindDifficulty(player, RAID_DIFFICULTY_25MAN_HEROIC))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("[{}] Рейды 25 гер.", ::GetCountInstanceBind(player, RAID_DIFFICULTY_25MAN_HEROIC)), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_RAID_25_HEROIC);

    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
}

void UnbindInstance::SendBindList(Player* player, Creature* creature, uint8 diff, bool isRaid /*= true*/)
{
    ClearGossipMenuFor(player);

    auto _diff = static_cast<Difficulty>(diff);

    auto const& boundMap = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), _diff);
    if (boundMap.empty())
    {
        SendGossipHello(player, creature);
        return;
    }

    uint8 index = 1;

    for (auto const& playerBind : boundMap)
    {
        auto const save = playerBind.second.save;
        bool _isRaidMapSave = save->GetMapEntry()->IsRaid();

        if (_isRaidMapSave != isRaid)
            continue;

        std::string const mapName = ::GetMapName(save->GetMapId(), static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex()));
        std::string const diffName = ::GetDiffName(_diff, _isRaidMapSave);
        std::string const gossipInfo = Warhead::StringFormat("{}. {} ({})", index, mapName, diffName);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, gossipInfo, diff + GOSSIP_SENDER_DIFFICULTY, save->GetMapId());
        index++;
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<< Назад", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_MAIN_MENU);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature);
}

void UnbindInstance::BindInfo(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    ClearGossipMenuFor(player);

    uint32 mapID = action;

    auto diff = static_cast<Difficulty>(sender - GOSSIP_SENDER_DIFFICULTY);

    auto const save = sInstanceSaveMgr->PlayerGetInstanceSave(player->GetGUID(), mapID, diff);
    if (!save)
    {
        SendGossipHello(player, creature);
        return;
    }

    bool isRaidDiff = save->GetMapEntry()->IsRaid();
    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());
    std::string const mapName = ::GetMapName(mapID, localeIndex);
    std::string const diffName = ::GetDiffName(diff, isRaidDiff);

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("-- Информация о сохранении"), sender, action);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("- {} ({})", mapName, diffName), sender, action);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "--", sender, action);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("# Варианты сброса сохранения"), sender, action);

    auto GetCostCount = [&](uint32 itemID) -> uint32
    {
        auto const& itr = _costStore.find(itemID);
        if (itr == _costStore.end())
            return 0;

        auto const& uiCostsStore = itr->second;

        if (isRaidDiff)
        {
            switch (diff)
            {
                case RAID_DIFFICULTY_10MAN_NORMAL:
                    return uiCostsStore.CountForRaid10Normal;
                case RAID_DIFFICULTY_25MAN_NORMAL:
                    return uiCostsStore.CountForRaid25Normal;
                case RAID_DIFFICULTY_10MAN_HEROIC:
                    return uiCostsStore.CountForRaid10Heroic;
                case RAID_DIFFICULTY_25MAN_HEROIC:
                    return uiCostsStore.CountForRaid25Heroic;
                default:
                    LOG_FATAL("module.unbind", "> UI: Incorrect diff for raid ({})", sender - GOSSIP_SENDER_DIFFICULTY);
                    break;
            }
        }
        else // if dungeon
        {
            switch (diff)
            {
                case DUNGEON_DIFFICULTY_HEROIC:
                    return uiCostsStore.CountForDungeonHeroic;
                default:
                    LOG_FATAL("module.unbind", "> UI: Incorrect diff for dungeon ({})", sender - GOSSIP_SENDER_DIFFICULTY);
                    break;
            }
        }

        return 0;
    };

    for (auto const& uiCost : _costStore)
    {
        uint32 itemID = uiCost.first;
        std::string itemlink = sGameLocale->GetItemLink(itemID, localeIndex);
        uint32 itemCount = GetCostCount(itemID);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Warhead::StringFormat("{}. {} ({}) шт.", itemID, itemlink, itemCount), sender, action);
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "-- Удалить сохранение (выбрать предмет)", sender, action, "Введите номер предмета", 0, true);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<< В главное меню", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_MAIN_MENU);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature);
}

void UnbindInstance::Unbind(Player* player, Creature* creature, uint32 sender, uint32 action, uint32 itemID)
{
    uint32 mapID = action;

    auto diff = static_cast<Difficulty>(sender - GOSSIP_SENDER_DIFFICULTY);

    auto const save = sInstanceSaveMgr->PlayerGetInstanceSave(player->GetGUID(), mapID, diff);
    if (!save)
    {
        SendGossipHello(player, creature);
        return;
    }

    bool isRaidDiff = save->GetMapEntry()->IsRaid();

    auto GetCostCount = [&]() -> uint32
    {
        auto const& itr = _costStore.find(itemID);
        if (itr == _costStore.end())
            return 0;

        auto const& uiCostsStore = itr->second;

        if (isRaidDiff)
        {
            switch (diff)
            {
                case RAID_DIFFICULTY_10MAN_NORMAL:
                    return uiCostsStore.CountForRaid10Normal;
                case RAID_DIFFICULTY_25MAN_NORMAL:
                    return uiCostsStore.CountForRaid25Normal;
                case RAID_DIFFICULTY_10MAN_HEROIC:
                    return uiCostsStore.CountForRaid10Heroic;
                case RAID_DIFFICULTY_25MAN_HEROIC:
                    return uiCostsStore.CountForRaid25Heroic;
                default:
                    LOG_FATAL("module.unbind", "> UI: Incorrect diff for raid ({})", sender - GOSSIP_SENDER_DIFFICULTY);
                    break;
            }
        }
        else // if dungeon
        {
            switch (diff)
            {
                case DUNGEON_DIFFICULTY_HEROIC:
                    return uiCostsStore.CountForDungeonHeroic;
                default:
                    LOG_FATAL("module.unbind", "> UI: Incorrect diff for dungeon ({})", sender - GOSSIP_SENDER_DIFFICULTY);
                    break;
            }
        }

        return 0;
    };

    ChatHandler handler(player->GetSession());
    uint32 itemCount = GetCostCount();
    uint32 hasItemCount = player->GetItemCount(itemID);
    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());
    std::string const& itemLink = sGameLocale->GetItemLink(itemID, localeIndex);
    std::string const& mapName = ::GetMapName(mapID, localeIndex);
    std::string const& diffName = ::GetDiffName(diff, isRaidDiff);

    if (!itemCount)
    {
        handler.PSendSysMessage("|cFFFF0000#|cff6C8CD5 Вы не можете удалить сохранение за|r {}", itemLink);
        BindInfo(player, creature, sender, action);
        return;
    }

    if (player->GetMapId() == mapID)
    {
        handler.PSendSysMessage("|cFFFF0000#|cff6C8CD5 Вы не можете удалить сохранение находясь здесь|r {} ({})", mapName, diffName);
        BindInfo(player, creature, sender, action);
        return;
    }

    if (!player->HasItemCount(itemID, itemCount))
    {
        handler.PSendSysMessage("|cFFFF0000#|cff6C8CD5 Вам не хватает|r {} х{}", itemLink, itemCount - hasItemCount);
        BindInfo(player, creature, sender, action);
        return;
    }

    player->DestroyItemCount(itemID, itemCount, true);
    handler.PSendSysMessage("|cFFFF0000#|cff6C8CD5 Удалено сохранение:|r {} ({})", mapName, diffName);
    sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), mapID, diff, true, player);
    SaveLogUnbind(player, mapID, sender - GOSSIP_SENDER_DIFFICULTY, itemID, isRaidDiff);

    SendGossipHello(player, creature);
}

void UnbindInstance::SaveLogUnbind(Player* player, uint32 mapID, uint8 diff, uint32 itemID, bool isRaid /*= true*/)
{
    std::string const& itemName = sGameLocale->GetItemNameLocale(itemID, 8);
    std::string const& mapName = ::GetMapName(mapID, 8);
    std::string const& diffName = ::GetDiffName(static_cast<Difficulty>(diff), isRaid);

    std::string const& info = Warhead::StringFormat("{} ({}). {}. {}", mapName, diffName, player->GetName(), itemName);

    CharacterDatabase.Execute("INSERT INTO `unbind_instance_logs`(`Info`, `PlayerGuid`, `MapID`, `Difficulty`, `ItemID`) VALUES ('{}', {}, {}, {}, {})",
                               info, player->GetGUID().GetCounter(), mapID, diff, itemID);
}

// .ui command
bool UnbindInstance::IsDisabledMap(uint32 mapID)
{
    auto const& itr = std::find(std::begin(_disabledIds), std::end(_disabledIds), mapID);
    return itr != std::end(_disabledIds);
}
