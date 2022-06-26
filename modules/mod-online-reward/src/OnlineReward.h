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

#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "Define.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include "TaskScheduler.h"
#include <tuple>
#include <unordered_map>
#include <vector>

class Player;
class ChatHandler;

using RewardsPair = std::pair<uint32/*id*/, uint32/*count*/>;
using RewardsVector = std::vector<RewardsPair>;

struct OnlineRewards
{
    OnlineRewards() = delete;

    OnlineRewards(bool isPerOnline, Seconds time) :
        IsPerOnline(isPerOnline), Seconds(time) { }

    bool IsPerOnline{ true };
    Seconds Seconds{ 0s };

    RewardsVector Items;
    RewardsVector Reputations;
};

class OnlineRewardMgr
{
    OnlineRewardMgr() = default;
    ~OnlineRewardMgr() = default;

    OnlineRewardMgr(OnlineRewardMgr const&) = delete;
    OnlineRewardMgr(OnlineRewardMgr&&) = delete;
    OnlineRewardMgr& operator= (OnlineRewardMgr const&) = delete;
    OnlineRewardMgr& operator= (OnlineRewardMgr&&) = delete;

    using RewardHistoryPerTime = std::unordered_map<int32/*per time*/, Seconds/*reward time*/>;
    using RewardHistory = std::pair<std::vector<Seconds>, RewardHistoryPerTime>;
    using RewardPending = std::tuple<Seconds, RewardsVector/*items*/, RewardsVector/*reputations*/>;

public:
    static OnlineRewardMgr* instance();

    void InitSystem();
    void LoadConfig();
    inline bool IsEnable() { return _isEnable; };

    void RewardNow();

    // Player hooks
    void AddRewardHistory(ObjectGuid::LowType lowGuid);
    void DeleteRewardHistory(ObjectGuid::LowType lowGuid);

    // World hooks
    void Update(Milliseconds diff);

    bool AddReward(bool isPerOnline, Seconds seconds, std::string_view items, std::string_view reputations, ChatHandler* handler = nullptr);
    bool DeleteReward(bool isPerOnline, Seconds seconds);
    bool IsExistReward(bool isPerOnline, Seconds seconds);

    auto const& GetOnlineRewards() { return &_rewards; }

    void LoadDBData();

private:
    void RewardPlayers();
    bool IsExistHistory(ObjectGuid::LowType lowGuid);
    void SaveRewardHistoryToDB();

    RewardHistory* GetHistory(ObjectGuid::LowType lowGuid);

    void SendRewardForPlayer(Player* player, Seconds secondsOnine, RewardsVector const& items, RewardsVector const& reputations);
    void AddHistory(ObjectGuid::LowType lowGuid, Seconds seconds);
    void AddHistory(ObjectGuid::LowType lowGuid, Seconds perTime, Seconds rewardTime);

    void AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result);

    bool IsRewarded(ObjectGuid::LowType lowGuid, Seconds seconds);
    bool IsRewarded(ObjectGuid::LowType lowGuid, Seconds perTime, Seconds rewardTime);

    void CheckPlayersForReward(bool isPerOnline, Seconds seconds, RewardsVector const& items, RewardsVector const& reputations);
    void SendRewards();

    void ScheduleReward();

    // Config
    bool _isEnable{ false };
    bool _isPerOnlineEnable{ false };
    bool _isPerTimeEnable{ false };
    bool _isForceMailReward{ true };

    // Containers
    std::vector<OnlineRewards> _rewards;
    std::unordered_map<ObjectGuid::LowType, RewardHistory> _rewardHistory;
    std::unordered_map<ObjectGuid::LowType, std::vector<RewardPending>> _rewardPending;
    TaskScheduler scheduler;

    QueryCallbackProcessor _queryProcessor;
};

#define sOLMgr OnlineRewardMgr::instance()

#endif /* _ONLINE_REWARD_H_ */
