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

#include "Define.h"
#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include "TaskScheduler.h"
#include <unordered_map>
#include <vector>

class Player;

class OnlineRewardMgr
{
    using RewardPerOnline = std::vector<Seconds>;
    using RewardsPair = std::pair<uint32/*item id*/, uint32/*item count*/>;
    using RewardHistoryPair = std::pair<RewardPerOnline, Seconds>;

    OnlineRewardMgr() = default;
    ~OnlineRewardMgr() = default;

    OnlineRewardMgr(OnlineRewardMgr const&) = delete;
    OnlineRewardMgr(OnlineRewardMgr&&) = delete;
    OnlineRewardMgr& operator= (OnlineRewardMgr const&) = delete;
    OnlineRewardMgr& operator= (OnlineRewardMgr&&) = delete;

public:
    static OnlineRewardMgr* instance();

    void InitSystem();
    void LoadConfig();
    inline bool IsEnable() { return _isEnable; };

    // Player hooks
    void AddRewardHistory(ObjectGuid::LowType lowGuid);
    void DeleteRewardHistory(ObjectGuid::LowType lowGuid);

    // World hooks
    void Update(Milliseconds diff);

private:
    void LoadDBData();
    void RewardPlayers();
    bool IsExistHistory(ObjectGuid::LowType lowGuid);
    void RewardPlayersPerOnline(Player* player);
    void RewardPlayersPerTime(Player* player);
    void SaveRewardDB();

    RewardPerOnline* GetHistoryPerOnline(ObjectGuid::LowType lowGuid);
    Seconds GetHistoryPerTime(ObjectGuid::LowType lowGuid);

    void SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, Seconds secondsOnine, bool isPerOnline = true);
    void SaveDataForDB(ObjectGuid::LowType lowGuid, Seconds seconds, bool isPerOnline = true);

private:
    void AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result);

    // Config
    bool _isEnable{ false };
    bool _isPerOnlineEnable{ false };
    bool _isPerTimeEnable{ false };
    bool _isForceMailReward{ true };

    // Config per time
    Seconds _perTimeTime{ 0s };
    uint32 _perTimeItemID{ 0 };
    uint32 _perTimeItemCount{ 0 };

    // Containers
    std::unordered_map<int32 /*time*/, RewardsPair> _rewards; // for per online
    std::unordered_map<ObjectGuid::LowType, RewardHistoryPair> _rewardHistoryDB;
    TaskScheduler scheduler;
    
    QueryCallbackProcessor _queryProcessor;
};

#define sOLMgr OnlineRewardMgr::instance()

#endif /* _ONLINE_REWARD_H_ */
