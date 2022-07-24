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

#include "ArenaReward.h"
#include "ArenaTeamMgr.h"
#include "BanMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "GameLocale.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include <tuple>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 SPELL_PARALYSE = 41083;
    constexpr uint32 SPELL_EXILE = 35182;
}

/*static*/ ArenaReward* ArenaReward::instance()
{
    static ArenaReward instance;
    return &instance;
}

void ArenaReward::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("ArenaReward.Enable");
    if (!_isEnable)
        return;

    // Rating
    _isRewardRatingEnable = MOD_CONF_GET_BOOL("ArenaReward.Reward.Rating.Enable");
    _rewardRatingItemID = MOD_CONF_GET_UINT("ArenaReward.Reward.Rating.ItemID");
    _rewardRatingItemCountWinner = MOD_CONF_GET_UINT("ArenaReward.Reward.Rating.ItemCount.WinnerTeam");
    _rewardRatingItemCountLoser = MOD_CONF_GET_UINT("ArenaReward.Reward.Rating.ItemCount.LoserTeam");

    // Skirmish
    _isRewardSkirmishEnable = MOD_CONF_GET_BOOL("ArenaReward.Reward.Skirmish.Enable");
    _rewardSkirmishItemID = MOD_CONF_GET_UINT("ArenaReward.Reward.Skirmish.ItemID");
    _rewardSkirmishItemCountWinner = MOD_CONF_GET_UINT("ArenaReward.Reward.Skirmish.ItemCount.WinnerTeam");
    _rewardSkirmishItemCountLoser = MOD_CONF_GET_UINT("ArenaReward.Reward.Skirmish.ItemCount.LoserTeam");

    // AntiFarm
    _isAntiFarmEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Enable");
    _isAntiFarmCheckIpEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Check.IP.Enable");
    _isAntiFarmCheckEquipmentEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Check.Equipment.Enable");
    _isAntiFarmCheckHealthEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Check.Health.Enable");
    _isAntiFarmTeleportEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Teleport.Enable");
    _isAntiFarmSpellApplyEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.SpellApply.Enable");
    _isAntiFarmBanEnable = MOD_CONF_GET_BOOL("ArenaReward.AntiFarm.Ban.Enable");
    _antiFarmBanDuration = MOD_CONF_GET_STR("ArenaReward.AntiFarm.Ban.Duration");
}

void ArenaReward::Init()
{
    if (!_isEnable || !_isAntiFarmTeleportEnable)
        return;

    std::string const& locationInfo = MOD_CONF_GET_STR("ArenaReward.AntiFarm.Teleport.Location");

    std::vector<std::string_view> tokens = Warhead::Tokenize(locationInfo, ' ', false);
    ASSERT(tokens.size() == 5);

    uint32 mapID = *Warhead::StringTo<uint32>(tokens[0]);
    float posX = *Warhead::StringTo<float>(tokens[1]);
    float posY = *Warhead::StringTo<float>(tokens[2]);
    float posZ = *Warhead::StringTo<float>(tokens[3]);
    float orientation = *Warhead::StringTo<float>(tokens[4]);

    _antiFarmTeleportLocation = std::make_unique<WorldLocation>(mapID, posX, posY, posZ, orientation);
}

void ArenaReward::SendRewardArena(Battleground* bg, TeamId winnerTeamId)
{
    // Not reward on end bg
    if (bg->isBattleground())
        return;

    uint32 rewardItemID = 0;
    uint32 rewardItemCountWinner = 0;
    uint32 rewardItemCountLoser = 0;
    bool isRated = bg->isRated();

    auto SendItems = [&](uint32 itemID, uint32 itemCountWinner, uint32 itemCountLoser)
    {
        if (isRated && _isRewardRatingEnable)
        {
            auto winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_HORDE : winnerTeamId));
            auto loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_ALLIANCE : bg->GetOtherTeamId(winnerTeamId)));

            if (!winnerArenaTeam || !loserArenaTeam)
                return;

            for (auto const& member : winnerArenaTeam->GetMembers())
            {
                Player* player = ObjectAccessor::FindPlayer(member.Guid);
                if (!player)
                    continue;

                player->AddItem(itemID, itemCountWinner);
                ChatHandler(player->GetSession()).PSendSysMessage("|cFFFF0000#|r |cff6C8CD5Вы выйграли матч и получили|r {} x{}", sGameLocale->GetItemLink(itemID), itemCountWinner);
            }

            for (auto const& member : loserArenaTeam->GetMembers())
            {
                Player* player = ObjectAccessor::FindPlayer(member.Guid);
                if (!player)
                    continue;

                player->AddItem(itemID, itemCountLoser);
                ChatHandler(player->GetSession()).PSendSysMessage("|cFFFF0000#|r |cff6C8CD5Вы проиграли матч и получили|r {} x{}", sGameLocale->GetItemLink(itemID), itemCountLoser);
            }
        }
        else if (_isRewardSkirmishEnable) // !isRated
        {
            for (auto const& playerMap : bg->GetPlayers())
            {
                Player* player = playerMap.second;
                if (!player)
                    continue;

                uint32 count = itemCountLoser;
                bool isWinner = player->GetBgTeamId() == winnerTeamId;

                if (isWinner)
                    count = itemCountWinner;

                player->AddItem(itemID, count);
                ChatHandler(player->GetSession()).PSendSysMessage("|cFFFF0000#|r |cff6C8CD5Вы {} матч и получили|r {} x{}", isWinner ? "выйграли" : "проиграли", sGameLocale->GetItemLink(itemID), count);
            }
        }
    };

    if (isRated)
    {
        rewardItemID = _rewardRatingItemID;
        rewardItemCountWinner = _rewardRatingItemCountWinner;
        rewardItemCountLoser = _rewardRatingItemCountLoser;
    }
    else // !isRated
    {
        rewardItemID = _rewardSkirmishItemID;
        rewardItemCountWinner = _rewardSkirmishItemCountWinner;
        rewardItemCountLoser = _rewardSkirmishItemCountLoser;
    }

    if (!IsPossibleFarm(bg, winnerTeamId))
        SendItems(rewardItemID, rewardItemCountWinner, rewardItemCountLoser);
}

bool ArenaReward::IsPossibleFarm(Battleground* bg, TeamId winnerTeamId)
{
    if (!_isAntiFarmEnable)
        return false;

    bool isFarmer = false;

    if (CheckIP(bg, winnerTeamId))
        isFarmer = true;

    if (CheckEqipment(bg, winnerTeamId))
        isFarmer = true;

    if (CheckHealth(bg, winnerTeamId))
        isFarmer = true;

    bool isRated = bg->isRated();

    if (isRated)
    {
        auto winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_HORDE : winnerTeamId));
        auto loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_ALLIANCE : bg->GetOtherTeamId(winnerTeamId)));

        if (!winnerArenaTeam || !loserArenaTeam)
            return false;

        for (auto const& member : winnerArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            ParalysePlayer(player);
        }

        for (auto const& member : loserArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            ParalysePlayer(player);
        }
    }
    else // !isRated
    {
        for (auto const& [playerGuid, player] : bg->GetPlayers())
        {
            if (!player)
                continue;

            ParalysePlayer(player);
        }
    }

    return isFarmer;
}

bool ArenaReward::CheckIP(Battleground* bg, TeamId winnerTeamId)
{
    if (!_isAntiFarmCheckIpEnable)
        return false;

    bool isRated = bg->isRated();

    std::vector<std::tuple<Player*, std::string, bool>> playerStore;
    std::vector<std::string> ipStore;
    std::vector<std::string> toBanIP;

    if (isRated)
    {
        auto winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_HORDE : winnerTeamId));
        auto loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_ALLIANCE : bg->GetOtherTeamId(winnerTeamId)));

        if (!winnerArenaTeam || !loserArenaTeam)
            return false;

        for (auto const& member : winnerArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            playerStore.emplace_back(std::make_tuple(player, player->GetSession()->GetRemoteAddress(), true));
        }

        for (auto const& member : loserArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            playerStore.emplace_back(std::make_tuple(player, player->GetSession()->GetRemoteAddress(), false));
        }
    }
    else // !isRated
    {
        for (auto const& playerMap : bg->GetPlayers())
        {
            Player* player = playerMap.second;
            if (!player)
                continue;

            if (player->GetBgTeamId() == winnerTeamId)
                playerStore.emplace_back(std::make_tuple(player, player->GetSession()->GetRemoteAddress(), true));
            else
                playerStore.emplace_back(std::make_tuple(player, player->GetSession()->GetRemoteAddress(), false));
        }
    }

    auto IsSameIPInDifferentTeams = [&](std::string const & targetIP)
    {
        uint8 countWin = 0;
        uint8 countLose = 0;

        for (auto const& [player, ip, isWinnerTeam] : playerStore)
        {
            if (targetIP != ip)
                continue;

            isWinnerTeam ? countWin++ : countLose++;
        }

        return countWin >= 1 && countLose >= 1;
    };

    for (auto const& [player, ip, isWinner] : playerStore)
        ipStore.emplace_back(ip);

    for (auto const& ip : ipStore)
        if (IsSameIPInDifferentTeams(ip))
            toBanIP.emplace_back(ip);

    bool isIPFound = false;
    std::vector<Player*> _toParalisePlayers;

    for (auto const& [player, ip, isWinnerTeam] : playerStore)
    {
        auto find = std::find(toBanIP.begin(), toBanIP.end(), ip);

        if (find != toBanIP.end())
        {
            _toParalisePlayers.emplace_back(player);
            isIPFound = true;
        }
    }

    if (!_toParalisePlayers.empty())
    {
        std::string playersName;

        for (auto const& player : _toParalisePlayers)
            playersName += player->GetName() + ", ";

        // Delete last (, )
        if (!playersName.empty())
            playersName.erase(playersName.end() - 2, playersName.end());

        LOG_WARN("module.ar", "> AR: Players ({}) used same ip for farming arena", playersName);
    }

    return isIPFound;
}

bool ArenaReward::CheckEqipment(Battleground* bg, TeamId winnerTeamId)
{
    if (!_isAntiFarmCheckEquipmentEnable)
        return false;

    bool isRated = bg->isRated();

    auto IsFullEquipment = [](Player * player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (slot == EQUIPMENT_SLOT_OFFHAND || slot == EQUIPMENT_SLOT_RANGED || slot == EQUIPMENT_SLOT_TABARD || slot == EQUIPMENT_SLOT_BODY)
                continue;

            if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                LOG_WARN("module.ar", "> AR: Player ({}) is no full equipment! Slot ({}) is empty", player->GetName(), slot);
                return false;
            }
        }

        return true;
    };

    if (isRated)
    {
        auto winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_HORDE : winnerTeamId));
        auto loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_ALLIANCE : bg->GetOtherTeamId(winnerTeamId)));

        if (!winnerArenaTeam || !loserArenaTeam)
            return false;

        for (auto const& member : winnerArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            if (!IsFullEquipment(player))
                return true;
        }

        for (auto const& member : loserArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            if (!IsFullEquipment(player))
                return true;
        }
    }
    else // !isRated
    {
        for (auto const& playerMap : bg->GetPlayers())
        {
            Player* player = playerMap.second;
            if (!player)
                continue;

            if (!IsFullEquipment(player))
                return true;
        }
    }

    return false;
}

bool ArenaReward::CheckHealth(Battleground* bg, TeamId winnerTeamId)
{
    if (!_isAntiFarmCheckHealthEnable)
        return false;

    bool isRated = bg->isRated();

    auto IsLowHealth = [](Player * player)
    {
        if (player->GetMaxHealth() <= 10000)
        {
            LOG_WARN("module.ar", "> AR: Player ({}) have ({}) health", player->GetName(), player->GetMaxHealth());
            return true;
        }

        return false;
    };

    if (isRated)
    {
        auto winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_HORDE : winnerTeamId));
        auto loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(bg->GetArenaTeamIdForTeam(winnerTeamId == TEAM_NEUTRAL ? TEAM_ALLIANCE : bg->GetOtherTeamId(winnerTeamId)));

        if (!winnerArenaTeam || !loserArenaTeam)
            return false;

        for (auto const& member : winnerArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            if (!IsLowHealth(player))
                return true;
        }

        for (auto const& member : loserArenaTeam->GetMembers())
        {
            Player* player = ObjectAccessor::FindPlayer(member.Guid);
            if (!player)
                continue;

            if (!IsLowHealth(player))
                return true;
        }
    }
    else // !isRated
    {
        for (auto const& playerMap : bg->GetPlayers())
        {
            Player* player = playerMap.second;
            if (!player)
                continue;

            if (!IsLowHealth(player))
                return true;
        }
    }

    return false;
}

void ArenaReward::ParalysePlayer(Player* player)
{
    if (_isAntiFarmTeleportEnable && _antiFarmTeleportLocation)
        player->TeleportTo(*_antiFarmTeleportLocation);

    if (_isAntiFarmSpellApplyEnable)
    {
        player->AddAura(SPELL_PARALYSE, player);
        player->AddAura(SPELL_EXILE, player);

        LOG_DEBUG("module.ar", "> Player ({}) paralyse!", player->GetName());
    }

    if (_isAntiFarmBanEnable)
        sBan->BanCharacter(player->GetName(), _antiFarmBanDuration, "Arena farming", "Winfidonarleyan");
}
