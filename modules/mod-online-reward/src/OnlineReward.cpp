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
#include "StopWatch.h"
#include "Tokenize.h"

OnlineRewardMgr* OnlineRewardMgr::instance()
{
    static OnlineRewardMgr instance;
    return &instance;
}

void OnlineRewardMgr::LoadConfig()
{
    _isEnable = MOD_CONF_GET_BOOL("OR.Enable");
    if (!_isEnable)
        return;

    _isPerOnlineEnable = MOD_CONF_GET_BOOL("OR.PerOnline.Enable");
    _isPerTimeEnable = MOD_CONF_GET_BOOL("OR.PerTime.Enable");
    _isForceMailReward = MOD_CONF_GET_BOOL("OR.ForceSendMail.Enable");

    _perTimeTime = Seconds(MOD_CONF_GET_UINT("OR.PerTime.Time"));
    _perTimeItemID = MOD_CONF_GET_UINT("OR.PerTime.ItemID");
    _perTimeItemCount = MOD_CONF_GET_UINT("OR.PerTime.ItemCount");

    if (!_isPerOnlineEnable && !_isPerTimeEnable)
    {
        _isEnable = false;
        LOG_ERROR("module.or", "> In module Online reward disabled all function. Disable module.");
        return;
    }
}

void OnlineRewardMgr::InitSystem()
{
    if (!_isEnable)
        return;

    LoadDBData();

    scheduler.Schedule(30s, [this](TaskContext context)
    {
        RewardPlayers();
        context.Repeat(10s, 1min);
    });
}

void OnlineRewardMgr::Update(Milliseconds diff)
{
    if (!_isEnable)
        return;

    scheduler.Update(diff);
    _queryProcessor.ProcessReadyCallbacks();
}

void OnlineRewardMgr::LoadDBData()
{
    StopWatch sw;

    LOG_INFO("module", "Loading online rewards for time...");

    _rewards.clear();

    QueryResult result = CharacterDatabase.Query("SELECT RewardPlayedTime, ItemID, Count FROM online_reward");
    if (!result)
    {
        LOG_WARN("module", "> DB table `online_reward` is empty!");
        LOG_WARN("module", "");
        return;
    }

    do
    {
        auto const& [seconds, itemID, itemCount] = result->FetchTuple<int32, uint32, uint32>();

        // Проверка
        if (!seconds)
        {
            LOG_ERROR("module", "> OnlineReward: RewardPlayedTime = 0? Really? Skip...");
            continue;
        }

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
        if (!itemTemplate)
        {
            LOG_ERROR("module", "> OnlineReward: Item with number {} not found. Skip", itemID);
            continue;
        }

        if (!itemCount)
        {
            LOG_ERROR("module", "> OnlineReward: Item count for item id {} - 0. Skip", itemID);
            continue;
        }

        _rewards.emplace(seconds, std::pair{ itemID, itemCount });

    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} reward for time in {}", _rewards.size(), sw);
    LOG_INFO("module", "");
}

void OnlineRewardMgr::AddRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
        return;

    _queryProcessor.AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `RewardedPerOnline`, `RewardedPerHour` FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid)).
        WithCallback(std::bind(&OnlineRewardMgr::AddRewardHistoryAsync, this, lowGuid, std::placeholders::_1)));
}

void OnlineRewardMgr::DeleteRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
        return;

    _rewardHistoryDB.erase(lowGuid);
}

void OnlineRewardMgr::RewardPlayers()
{
    if (!_isEnable)
        return;

    auto const& sessions = sWorld->GetAllSessions();
    if (sessions.empty())
        return;

    // Send reward
    for (auto const& [accountID, session] : sessions)
    {
        auto const& player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        if (_isPerOnlineEnable)
            RewardPlayersPerOnline(player);

        if (_isPerTimeEnable)
            RewardPlayersPerTime(player);
    }

    // Save data to DB
    SaveRewardDB();
}

void OnlineRewardMgr::RewardPlayersPerOnline(Player* player)
{
    ChatHandler handler{ player->GetSession() };
    Seconds playedTimeSec{ player->GetTotalPlayedTime() };
    auto const& lowGuid = player->GetGUID().GetCounter();

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
        if (seconds > playedTimeSec.count())
            continue;

        SendRewardForPlayer(player, items.first, items.second, Seconds(seconds));
    }
}

void OnlineRewardMgr::RewardPlayersPerTime(Player* player)
{
    auto const& lowGuid = player->GetGUID().GetCounter();
    Seconds lastRewarded{ GetHistoryPerTime(lowGuid) };
    Seconds playedTimeSec{ player->GetTotalPlayedTime() };
    Seconds diffTime{ 0s };

    while (diffTime < playedTimeSec)
    {
        if (lastRewarded < diffTime)
            SendRewardForPlayer(player, _perTimeItemID, _perTimeItemCount, diffTime, false);

        diffTime += _perTimeTime;
    }
}

void OnlineRewardMgr::SaveRewardDB()
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
                saveDataPerOnline += Warhead::ToString(itr.count()) + ',';

            // Delete last (,)
            if (!saveDataPerOnline.empty())
                saveDataPerOnline.erase(saveDataPerOnline.end() - 1, saveDataPerOnline.end());
        }

        // Delele old data
        trans->Append("DELETE FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid);

        // Insert new data
        trans->Append("INSERT INTO `online_reward_history` (`PlayerGuid`, `RewardedPerOnline`, `RewardedPerHour`) VALUES ({}, '{}', {})", lowGuid, saveDataPerOnline, dataPerTime.count());
    }

    CharacterDatabase.CommitTransaction(trans);
}

OnlineRewardMgr::RewardPerOnline* OnlineRewardMgr::GetHistoryPerOnline(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return nullptr;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return &itr->second.first;

    return nullptr;
}

Seconds OnlineRewardMgr::GetHistoryPerTime(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return 0s;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return itr->second.second;

    return 0s;
}

void OnlineRewardMgr::SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, Seconds secondsOnine, bool isPerOnline /*= true*/)
{
    ChatHandler handler(player->GetSession());
    std::string playedTimeSecStr = Warhead::Time::ToTimeString(secondsOnine, 3, TimeFormat::FullText);
    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());

    auto SendItemsViaMail = [player, itemID, itemCount, &playedTimeSecStr, &localeIndex]()
    {
        auto const& mailSubject = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_SUBJECT", localeIndex), playedTimeSecStr);
        auto const& MailText = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_TEXT", localeIndex), player->GetName(), playedTimeSecStr);

        // Send External mail
        sExternalMail->AddMail(player->GetName(), mailSubject, MailText, itemID, itemCount, 37688);
    };

    // Save data to DB
    SaveDataForDB(player->GetGUID().GetCounter(), secondsOnine, isPerOnline);

    if (_isForceMailReward)
    {
        SendItemsViaMail();

        // Send chat text
        sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE_MAIL", playedTimeSecStr);
        return;
    }

    if (!player->AddItem(itemID, itemCount))
    {
        SendItemsViaMail();

        // Send chat text
        sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_NOT_ENOUGH_BAG");
    }

    // Send chat text
    sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE_IN_GAME", playedTimeSecStr);
}

void OnlineRewardMgr::SaveDataForDB(ObjectGuid::LowType lowGuid, Seconds seconds, bool isPerOnline /*= true*/)
{
    auto const& _historyData = _rewardHistoryDB.find(lowGuid);
    if (_historyData == _rewardHistoryDB.end())
    {
        RewardHistoryPair rewardHistoryPair;

        if (isPerOnline)
            rewardHistoryPair.first.emplace_back(seconds);
        else
            rewardHistoryPair.second = seconds;

        _rewardHistoryDB.emplace(lowGuid, rewardHistoryPair);
        return;
    }

    auto& rewardHistoryPair = _historyData->second;

    if (isPerOnline)
        rewardHistoryPair.first.emplace_back(seconds);
    else
        rewardHistoryPair.second = seconds;
}

bool OnlineRewardMgr::IsExistHistory(ObjectGuid::LowType lowGuid)
{
    return _rewardHistoryDB.find(lowGuid) != _rewardHistoryDB.end();
}

void OnlineRewardMgr::AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result)
{
    if (!result)
        return;

    auto const& [rewardPerOnline, rewardPerHour] = result->FetchTuple<std::string_view, Seconds>();

    RewardPerOnline rewardData;

    for (auto const& seconds : Warhead::Tokenize(rewardPerOnline, ',', false))
        rewardData.emplace_back(Seconds(Warhead::StringTo<int32>(seconds).value_or(0)));

    _rewardHistoryDB.emplace(lowGuid, std::make_pair(rewardData, rewardPerHour));
}
