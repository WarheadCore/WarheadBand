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

#ifndef _ARENA_REWARD_H_
#define _ARENA_REWARD_H_

#include "SharedDefines.h"
#include <memory>
#include <string>

class Battleground;
class Player;

struct WorldLocation;

class ArenaReward
{
public:
    static ArenaReward* instance();

    void LoadConfig(bool reload);
    void Init();

    void SendRewardArena(Battleground* bg, TeamId winnerTeamId);

private:
    bool IsPossibleFarm(Battleground* bg, TeamId winnerTeamId);
    bool CheckIP(Battleground* bg, TeamId winnerTeamId);
    bool CheckEqipment(Battleground* bg, TeamId winnerTeamId);
    bool CheckHealth(Battleground* bg, TeamId winnerTeamId);

    void ParalysePlayer(Player* player);

    bool _isEnable{ false };

    // Rating
    bool _isRewardRatingEnable{ false };
    uint32 _rewardRatingItemID{ 0 };
    uint32 _rewardRatingItemCountWinner{ 0 };
    uint32 _rewardRatingItemCountLoser{ 0 };

    // Skirmish
    bool _isRewardSkirmishEnable{ false };
    uint32 _rewardSkirmishItemID{ 0 };
    uint32 _rewardSkirmishItemCountWinner{ 0 };
    uint32 _rewardSkirmishItemCountLoser{ 0 };

    // AntiFarm
    bool _isAntiFarmEnable{ false };
    bool _isAntiFarmCheckIpEnable{ false };
    bool _isAntiFarmCheckEquipmentEnable{ false };
    bool _isAntiFarmCheckHealthEnable{ false };
    bool _isAntiFarmTeleportEnable{ false };
    std::unique_ptr<WorldLocation> _antiFarmTeleportLocation;
    bool _isAntiFarmSpellApplyEnable{ false };
    bool _isAntiFarmBanEnable{ false };
    std::string _antiFarmBanDuration;
};

#define sAR ArenaReward::instance()

#endif /* _ARENA_REWARD_H_ */
