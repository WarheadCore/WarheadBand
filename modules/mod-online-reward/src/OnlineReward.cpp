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

#include "OnlineReward.h"
#include "Chat.h"
#include "ExternalMail.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "StringConvert.h"
#include "TaskScheduler.h"
#include "Tokenize.h"

namespace
{
    using RewardPerOnline = std::vector<Seconds>;
    using RewardPerTime = Seconds;
    using RewardsPair = std::pair<uint32/*item id*/, uint32/*item count*/>;
    using RewardHistoryPair = std::pair<RewardPerOnline, RewardPerTime>;

    std::unordered_map<int32 /*time*/, RewardsPair> _rewards; // for per online
    std::unordered_map<ObjectGuid::LowType, RewardHistoryPair> _rewardHistoryDB;
    TaskScheduler scheduler;
    bool IsSystemEnable = false;

    void AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result)
    {
        if (!result)
            return;

        auto fields = result->Fetch();

        RewardPerOnline data = {};

        for (auto const& itr : Warhead::Tokenize(fields[0].GetStringView(), ',', false))
            data.emplace_back(Seconds(Warhead::StringTo<int32>(itr).value_or(0)));

        _rewardHistoryDB.emplace(lowGuid, std::make_pair(data, RewardPerTime(fields[1].GetInt32())));
    }
}

OnlineReward* OnlineReward::instance()
{
    static OnlineReward instance;
    return &instance;
}

void OnlineReward::InitSystem()
{
    if (!MOD_CONF_GET_BOOL("OnlineReward.Enable"))
        return;

    if (!MOD_CONF_GET_BOOL("OnlineReward.PerOnline.Enable") && !MOD_CONF_GET_BOOL("OnlineReward.PerTime.Enable"))
    {
        sModulesConfig->SetOption<bool>("OnlineReward.Enable", false);
        return;
    }

    IsSystemEnable = true;

    LoadRewards();

    scheduler.Schedule(30s, [this](TaskContext context)
    {
        RewardPlayers();

        context.Repeat(10s, 1min);
    });
}

void OnlineReward::Update(uint32 diff)
{
    if (!IsSystemEnable)
        return;

    scheduler.Update(diff);
    _queryProcessor.ProcessReadyCallbacks();
}

void OnlineReward::LoadRewards()
{
    LOG_INFO("module", "Loading online rewards...");

    _rewards.clear();
    uint32 msTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT RewardPlayedTime, ItemID, Count FROM online_reward");
    if (!result)
    {
        LOG_WARN("module", "> DB table `online_reward` is empty!");
        LOG_WARN("module", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        auto seconds = fields[0].GetInt32();
        auto itemID = fields[1].GetUInt32();
        auto itemCount = fields[2].GetUInt32();

        // Проверка
        if (!seconds)
        {
            LOG_ERROR("module", "-> Time = 0? Really? Skip...");
            continue;
        }

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
        if (!itemTemplate)
        {
            LOG_ERROR("module", "-> Item with number {} not found. Skip", itemID);
            continue;
        }

        if (!itemCount)
        {
            LOG_ERROR("module", "-> Item count for number {} - 0. Set to 1", itemID);
            itemCount = 1;
        }

        _rewards.emplace(seconds, std::make_pair(itemID, itemCount));

    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} reward in {} ms", _rewards.size(), GetMSTimeDiffToNow(msTime));
    LOG_INFO("module", "");
}

void OnlineReward::AddRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!IsSystemEnable)
        return;

    _queryProcessor.AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `RewardedPerOnline`, `RewardedPerHour` FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid).c_str()).
        WithCallback(std::bind(&AddRewardHistoryAsync, lowGuid, std::placeholders::_1)));
}

void OnlineReward::DeleteRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!IsSystemEnable)
        return;

    _rewardHistoryDB.erase(lowGuid);
}

void OnlineReward::RewardPlayers()
{
    auto const& sessions = sWorld->GetAllSessions();
    if (sessions.empty())
        return;

    // Send reward
    for (auto const& itr : sessions)
    {
        auto player = itr.second->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        if (MOD_CONF_GET_BOOL("OnlineReward.PerOnline.Enable"))
            RewardPlayersPerOnline(player);

        if (MOD_CONF_GET_BOOL("OnlineReward.PerTime.Enable"))
            RewardPlayersPerTime(player);
    }

    // Save data to DB
    SaveRewardDB();
}

void OnlineReward::RewardPlayersPerOnline(Player* player)
{
    ChatHandler handler(player->GetSession());
    int32 playedTimeSec = player->GetTotalPlayedTime();
    auto lowGuid = player->GetGUID().GetCounter();

    auto IsRewarded = [this, lowGuid](int32 rewardSeconds)
    {
        if (!IsExistHistory(lowGuid))
            return false;

        for (auto const& seconds : *GetHistoryPerOnline(lowGuid))
        {
            if (seconds.count() == rewardSeconds)
                return true;
        }

        return false;
    };

    for (auto const& [seconds, items] : _rewards)
    {
        // Go next if rewarded
        if (IsRewarded(seconds))
            continue;

        // End reward :/
        if (seconds > playedTimeSec)
            continue;

        SendRewardForPlayer(player, items.first, items.second, Seconds(seconds));
    }
}

void OnlineReward::RewardPlayersPerTime(Player* player)
{
    auto lowGuid = player->GetGUID().GetCounter();
    Seconds lastRewarded = GetHistoryPerTime(lowGuid);
    Seconds playedTimeSec = Seconds(player->GetTotalPlayedTime());
    Seconds diffTime = 0s;

    while (diffTime < playedTimeSec)
    {
        if (lastRewarded < diffTime)
            SendRewardForPlayer(player, MOD_CONF_GET_INT("OnlineReward.PerTime.ItemID"), MOD_CONF_GET_INT("OnlineReward.PerTime.ItemCount"), diffTime, false);

        diffTime += Seconds(MOD_CONF_GET_INT("OnlineReward.PerTime.Time"));
    }
}

void OnlineReward::SaveRewardDB()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Save data for exist history
    for (auto const& [lowGuid, saveDataPair] : _rewardHistoryDB)
    {
        auto const& [dataPerOnline, dataPerTime] = saveDataPair;
        std::string saveDataPerOnline;

        if (!dataPerOnline.empty())
        {
            // Add rewarded seconds
            for (auto const& itr : dataPerOnline)
                saveDataPerOnline += std::to_string(itr.count()) + ',';

            // Delete last (,)
            if (!dataPerOnline.empty())
                saveDataPerOnline.erase(saveDataPerOnline.end() - 1, saveDataPerOnline.end());
        }

        // Delele old data
        trans->PAppend("DELETE FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid);

        // Insert new data
        trans->PAppend("INSERT INTO `online_reward_history` (`PlayerGuid`, `RewardedPerOnline`, `RewardedPerHour`) VALUES ({}, '{}', {})", lowGuid, saveDataPerOnline, dataPerTime.count());
    }

    CharacterDatabase.CommitTransaction(trans);
}

std::vector<Seconds>* OnlineReward::GetHistoryPerOnline(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return nullptr;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return &itr->second.first;

    return nullptr;
}

Seconds OnlineReward::GetHistoryPerTime(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return 0s;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return itr->second.second;

    return 0s;
}

void OnlineReward::SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, Seconds secondsOnine, bool isPerOnline /*= true*/)
{
    ChatHandler handler(player->GetSession());
    std::string subject, text;
    std::string playedTimeSecStr = Warhead::Time::ToTimeString(secondsOnine);
    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());

    subject = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_SUBJECT", localeIndex), playedTimeSecStr);
    text = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_TEXT", localeIndex), player->GetName(), playedTimeSecStr);

    // Send External mail
    sExternalMail->AddMail(player->GetName(), subject, text, itemID, itemCount, 37688);

    // Save data to DB
    SaveDataForDB(player->GetGUID().GetCounter(), secondsOnine, isPerOnline);

    // Send chat text
    sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE", playedTimeSecStr);
}

void OnlineReward::SaveDataForDB(ObjectGuid::LowType lowGuid, Seconds seconds, bool isPerOnline /*= true*/)
{
    auto const& _historyData = _rewardHistoryDB.find(lowGuid);
    if (_historyData == _rewardHistoryDB.end())
    {
        RewardHistoryPair rewardHistoryPair = {};

        if (isPerOnline)
            rewardHistoryPair.first.emplace_back(seconds);
        else
            rewardHistoryPair.second = seconds;

        _rewardHistoryDB.emplace(lowGuid, rewardHistoryPair);
    }
    else
    {
        RewardHistoryPair& rewardHistoryPair = _historyData->second;

        if (isPerOnline)
            rewardHistoryPair.first.emplace_back(seconds);
        else
            rewardHistoryPair.second = seconds;
    }
}

bool OnlineReward::IsExistHistory(ObjectGuid::LowType lowGuid)
{
    return _rewardHistoryDB.find(lowGuid) != _rewardHistoryDB.end();
}
