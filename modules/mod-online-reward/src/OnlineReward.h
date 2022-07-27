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

#ifndef _ONLINE_REWARD_H_
#define _ONLINE_REWARD_H_

#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "Define.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include "TaskScheduler.h"
#include <unordered_map>
#include <mutex>
#include <vector>

class Player;
class ChatHandler;

struct OnlineReward
{
    using RewardsPair = std::pair<uint32/*id*/, uint32/*count*/>;
    using RewardsVector = std::vector<RewardsPair>;

    OnlineReward() = delete;

    OnlineReward(uint32 id, bool isPerOnline, Seconds time) :
        ID(id), IsPerOnline(isPerOnline), Seconds(time) { }

    uint32 ID{ 0 };
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

    using RewardHistoryStruct = std::pair<uint32/*reward id*/, Seconds/*rewarded seconds*/>;
    using RewardPendingStruct = uint32/*reward id*/;

    using RewardHistory = std::vector<RewardHistoryStruct>;
    using RewardPending = std::vector<RewardPendingStruct>;

public:
    static OnlineRewardMgr* instance();

    void InitSystem();
    void LoadConfig(bool reload);
    inline bool IsEnable() { return _isEnable; };

    void RewardNow();

    // Player hooks
    void AddRewardHistory(ObjectGuid::LowType lowGuid);
    void DeleteRewardHistory(ObjectGuid::LowType lowGuid);

    // World hooks
    void Update(Milliseconds diff);

    bool AddReward(uint32 id, bool isPerOnline, Seconds seconds, std::string_view items, std::string_view reputations, ChatHandler* handler = nullptr);
    bool DeleteReward(uint32 id);
    bool IsExistReward(uint32 id);

    auto const& GetOnlineRewards() { return &_rewards; }

    void LoadDBData();

private:
    void RewardPlayers();
    bool IsExistHistory(ObjectGuid::LowType lowGuid);
    void SaveRewardHistoryToDB();

    RewardHistory* GetHistory(ObjectGuid::LowType lowGuid);
    Seconds GetHistorySecondsForReward(ObjectGuid::LowType lowGuid, uint32 id);
    OnlineReward const* GetOnlineReward(uint32 id);

    void SendRewardForPlayer(Player* player, uint32 rewardID);
    void AddHistory(ObjectGuid::LowType lowGuid, uint32 rewardId, Seconds playerOnlineTime);

    void AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result);

    void CheckPlayerForReward(ObjectGuid::LowType lowGuid, Seconds playedTime, OnlineReward const* onlineReward);
    void SendRewards();

    void ScheduleReward();

    // Config
    bool _isEnable{ false };
    bool _isPerOnlineEnable{ false };
    bool _isPerTimeEnable{ false };
    bool _isForceMailReward{ true };
    bool _isEnableCorrectHistory{ true };

    // New version
    void CorrectDBData();

    // Containers
    std::unordered_map<uint32, OnlineReward> _rewards;
    std::unordered_map<ObjectGuid::LowType, RewardHistory> _rewardHistory;
    std::unordered_map<ObjectGuid::LowType, RewardPending> _rewardPending;
    TaskScheduler scheduler;

    QueryCallbackProcessor _queryProcessor;
    std::mutex _playerLoadingLock;
};

#define sOLMgr OnlineRewardMgr::instance()

#endif /* _ONLINE_REWARD_H_ */
