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

#ifndef _ONLINE_REWARD_H_
#define _ONLINE_REWARD_H_

#include "Common.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "Duration.h"
#include <unordered_map>
#include <vector>

class OnlineReward
{
public:
    static OnlineReward* instance();

    void InitSystem();
    void AddRewardHistory(ObjectGuid::LowType lowGuid);
    void DeleteRewardHistory(ObjectGuid::LowType lowGuid);
    void Update(uint32 diff);

private:
    void LoadRewards();
    void RewardPlayers();
    bool IsExistHistory(ObjectGuid::LowType lowGuid);
    void RewardPlayersPerOnline(Player* player);
    void RewardPlayersPerTime(Player* player);
    void SaveRewardDB();

    std::vector<Seconds>* GetHistoryPerOnline(ObjectGuid::LowType lowGuid);
    Seconds GetHistoryPerTime(ObjectGuid::LowType lowGuid);

    void SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, Seconds secondsOnine, bool isPerOnline = true);
    void SaveDataForDB(ObjectGuid::LowType lowGuid, Seconds seconds, bool isPerOnline = true);

private:
    QueryCallbackProcessor _queryProcessor;
};

#define sOL OnlineReward::instance()

#endif /* _ONLINE_REWARD_H_ */
