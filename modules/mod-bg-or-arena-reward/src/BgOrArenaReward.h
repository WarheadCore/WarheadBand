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

#ifndef _BG_OR_ARENA_REWARD_H_
#define _BG_OR_ARENA_REWARD_H_

#include "Define.h"
#include "Battleground.h"
#include <unordered_map>
#include <vector>
#include <tuple>

enum class BOARRewardType : uint8
{
    BG,
    Arena,

    Max
};

using BOARRewardItems = std::tuple<uint32/*wi*/, uint32/*wic*/, uint32/*li*/, uint32/*lic*/>;
using BOARRewardItemsVector = std::vector<BOARRewardItems>;

class Player;

class BOARMgr
{
    BOARMgr() = default;
    ~BOARMgr() = default;

    BOARMgr(BOARMgr const&) = delete;
    BOARMgr(BOARMgr&&) = delete;
    BOARMgr& operator=(BOARMgr const&) = delete;
    BOARMgr& operator=(BOARMgr&&) = delete;

public:
    static BOARMgr* instance();

    inline bool IsEnable() { return _isEnable; }

    void Initialize();
    void LoadConig();
    void LoadDBData();

    void RewardPlayer(Player* player, Battleground* bg, TeamId winnerTeamId);

private:
    BOARRewardItemsVector* GetReward(BattlegroundTypeId bgType);
    bool CreateReward(BattlegroundTypeId bgType, uint32 winnerItemID, uint32 winnerItemCount, uint32 loserItemID, uint32 loserItemCount);

    bool _isEnable{ false };

    std::unordered_map<uint8, BOARRewardItemsVector> _store;
};

#define sBOARMgr BOARMgr::instance()

#endif // _BG_OR_ARENA_REWARD_H_
