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

#include "BgOrArenaReward.h"
#include "Battleground.h"
#include "Player.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "StopWatch.h"
#include "ObjectMgr.h"

BOARMgr* BOARMgr::instance()
{
    static BOARMgr instance;
    return &instance;
}

void BOARMgr::Initialize()
{
    if (!_isEnable)
        return;

    LoadDBData();
}

void BOARMgr::LoadConig()
{
    _isEnable = MOD_CONF_GET_BOOL("BOAR.Enable");

    // For other config options
    /*if (!_isEnable)
        return;*/
}

void BOARMgr::LoadDBData()
{
    LOG_INFO("server.loading", "Loading battleground/arena rewards...");

    StopWatch sw;
    _store.clear();

    QueryResult result = CharacterDatabase.Query("SELECT `BgTypeID`, `WinnerItemID`, `WinnerItemCount`, `LoserItemID`, `LoserItemCount` FROM battlegound_arena_rewards");
    if (!result)
    {
        LOG_FATAL("sql.sql", ">> Loaded 0 rewards. DB table `battlegound_arena_rewards` is empty.");
        LOG_WARN("server.loading", "> Disable this module");
        _isEnable = false;
        return;
    }

    uint32 count{ 0 };

    do
    {
        auto const& [bgTypeId, winnerItemID, winnerItemCount, loserItemID, loserItemCount] = result->FetchTuple<uint8, uint32, uint32, uint32, uint32>();

        if (!CreateReward(static_cast<BattlegroundTypeId>(bgTypeId), winnerItemID, winnerItemCount, loserItemID, loserItemCount))
        {
            LOG_WARN("server.loading", "> Disable this module");
            _isEnable = false;
            return;
        }

        count++;

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} rewars in {}", count, sw);
    LOG_INFO("server.loading", "");
}

void BOARMgr::RewardPlayer(Player* player, Battleground* bg, TeamId winnerTeamId)
{
    if (!_isEnable || !player || !bg)
        return;

    auto bgTypeID{ bg->GetBgTypeID(bg->IsRandom()) };

    auto const& reward = GetReward(bgTypeID);
    if (!reward || reward->empty())
        return;

    for (auto const& [winnerItemID, winnerItemCount, loserItemID, loserItemCount] : *reward)
    {
        auto itemID{ player->GetBgTeamId() == winnerTeamId ? winnerItemID : loserItemID };
        auto itemCount{ player->GetBgTeamId() == winnerTeamId ? winnerItemCount : loserItemCount };

        player->AddItem(itemID, itemCount);
    }
}

BOARRewardItemsVector* BOARMgr::GetReward(BattlegroundTypeId bgType)
{
    auto const& itr = _store.find(static_cast<uint8>(bgType));
    if (itr == _store.end())
        return nullptr;

    return &itr->second;
}

bool BOARMgr::CreateReward(BattlegroundTypeId bgType, uint32 winnerItemID, uint32 winnerItemCount, uint32 loserItemID, uint32 loserItemCount)
{
    auto CheckItem = [](std::initializer_list<uint32> items)
    {
        for (auto const& itemID : items)
        {
            auto const& itemTemplate = sObjectMgr->GetItemTemplate(itemID);
            if (!itemTemplate)
            {
                LOG_ERROR("module.boar", "> Try create reward with invalid item id {}", itemID);
                return false;
            }
        }

        return true;
    };

    if (!CheckItem({ winnerItemID, loserItemID }))
        return false;

    BOARRewardItemsVector place{ { { winnerItemID, winnerItemCount, loserItemID, loserItemCount } } };

    auto const& reward = GetReward(bgType);
    if (!reward)
    {
        _store.emplace(static_cast<uint8>(bgType), std::vector{ std::move(place) });
        return true;
    }

    if (reward->empty())
    {
        LOG_ERROR("module.boar", "> Try create reward with empty 'std::vector<BOARReward>'");
        return false;
    }

    reward->emplace_back(winnerItemID, winnerItemCount, loserItemID, loserItemCount);
    return true;
}
