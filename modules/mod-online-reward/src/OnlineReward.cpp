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
#include "AsyncCallbackMgr.h"
#include "Chat.h"
#include "ExternalMail.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "StopWatch.h"
#include "StringConvert.h"
#include "Tokenize.h"

OnlineRewardMgr* OnlineRewardMgr::instance()
{
    static OnlineRewardMgr instance;
    return &instance;
}

void OnlineRewardMgr::LoadConfig()
{
    // Check module config file
    sModulesConfig->AddOption({ "OR.Enable", "OR.PerOnline.Enable", "OR.PerTime.Enable", "OR.ForceSendMail.Enable" });

    _isEnable = MOD_CONF_GET_BOOL("OR.Enable");
    if (!_isEnable)
        return;

    _isPerOnlineEnable = MOD_CONF_GET_BOOL("OR.PerOnline.Enable");
    _isPerTimeEnable = MOD_CONF_GET_BOOL("OR.PerTime.Enable");
    _isForceMailReward = MOD_CONF_GET_BOOL("OR.ForceSendMail.Enable");

    if (!_isPerOnlineEnable && !_isPerTimeEnable)
    {
        _isEnable = false;
        LOG_ERROR("module.or", "> In module Online Reward disabled all function. Disable module.");
        //return;
    }
}

void OnlineRewardMgr::InitSystem()
{
    if (!_isEnable)
        return;

    LoadDBData();

    // If data empty no need reward players
    if (!_isEnable)
        return;

    scheduler.Schedule(30s, [this](TaskContext context)
    {
        RewardPlayers();
        context.Repeat(1min, 5min);
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

    LOG_INFO("module", "Loading online rewards...");

    _rewards.clear();

    QueryResult result = CharacterDatabase.Query("SELECT IsPerOnline, Seconds, ItemID, ItemCount, FactionID, ReputationCount FROM online_reward");
    if (!result)
    {
        LOG_WARN("module", "> DB table `online_reward` is empty! Disable module");
        LOG_WARN("module", "");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [isPerOnline, seconds, itemID, itemCount, factionID, reputationCount] =
            result->FetchTuple<bool, int32, uint32, uint32, uint32, uint32>();

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

        if (factionID)
        {
            FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionID);
            if (!factionEntry)
            {
                LOG_ERROR("module", "> OnlineReward: Not found faction with id {}. Skip", factionID);
                continue;
            }

            if (factionEntry->reputationListID < 0)
            {
                LOG_ERROR("module", "> OnlineReward: Faction {} can't have reputation. Skip", factionID);
                continue;
            }
        }

        if (!factionID && reputationCount)
        {
            LOG_ERROR("module", "> OnlineReward: reputation count {} for empty faction. Skip", reputationCount);
            continue;
        }
        else if (reputationCount > ReputationMgr::Reputation_Cap)
        {
            LOG_ERROR("module", "> OnlineReward: reputation count {} > repitation cap {}. Skip", reputationCount, ReputationMgr::Reputation_Cap);
            continue;
        }

        _rewards.emplace_back(OnlineRewards(isPerOnline, Seconds(seconds), itemID, itemCount, factionID, reputationCount));
    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} online rewards in {}", _rewards.size(), sw);
    LOG_INFO("module", "");
}

void OnlineRewardMgr::AddRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
        return;

    if (IsExistHistory(lowGuid))
        return;

    _queryProcessor.AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `RewardedPerOnline`, `RewardedPerTime` FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid)).
        WithCallback(std::bind(&OnlineRewardMgr::AddRewardHistoryAsync, this, lowGuid, std::placeholders::_1)));
}

void OnlineRewardMgr::DeleteRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
        return;

    if (!IsExistHistory(lowGuid))
        return;

    _rewardHistory.erase(lowGuid);
}

void OnlineRewardMgr::RewardPlayers()
{
    if (!_isEnable)
        return;

    // Empty world, no need reward
    if (!sWorld->GetPlayerCount())
        return;

    ASSERT(_rewardPending.empty());

    sAsyncCallbackMgr->AddAsyncCallback([this]()
    {
        StopWatch sw;

        LOG_DEBUG("module", "> OR: Start rewars players...");

        for (auto const& [isPerOnline, seconds, itemID, itemCount, factionID, reputationCount] : _rewards)
        {
            if (isPerOnline && !_isPerOnlineEnable)
                continue;

            if (!isPerOnline && !_isPerTimeEnable)
                continue;

            CheckPlayersForReward(isPerOnline, seconds, itemID, itemCount, factionID, reputationCount);
        }

        // Send reward
        SendRewards();

        // Save data to DB
        SaveRewardHistoryToDB();

        LOG_DEBUG("module", "> OR: End rewars players. Eplased {}", sw);
    });
}

void OnlineRewardMgr::SaveRewardHistoryToDB()
{
    if (_rewardHistory.empty())
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Save data for exist history
    for (auto const& [lowGuid, history] : _rewardHistory)
    {
        auto const& [perOnline, perTime] = history;
        std::string saveDataPerOnline;
        std::string saveDataPerTime;

        if (!perOnline.empty())
        {
            // Add rewarded seconds
            for (auto const& secs : perOnline)
                saveDataPerOnline += Warhead::ToString(secs.count()) + ',';

            // Delete last (,)
            if (!saveDataPerOnline.empty() && saveDataPerOnline.back() == ',')
                saveDataPerOnline.erase(saveDataPerOnline.end() - 1, saveDataPerOnline.end());
        }

        if (!perTime.empty())
        {
            // Add rewarded seconds
            for (auto const& [perTime, rewardTime] : perTime)
                saveDataPerTime += Warhead::ToString(perTime) + ':' + Warhead::ToString(rewardTime.count()) + ',';

            // Delete last (,)
            if (!saveDataPerTime.empty() && saveDataPerTime.back() == ',')
                saveDataPerTime.erase(saveDataPerTime.end() - 1, saveDataPerTime.end());
        }

        // Delele old data
        trans->Append("DELETE FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid);

        // Insert new data
        trans->Append("INSERT INTO `online_reward_history` (`PlayerGuid`, `RewardedPerOnline`, `RewardedPerTime`) VALUES ({}, '{}', '{}')", lowGuid, saveDataPerOnline, saveDataPerTime);
    }

    CharacterDatabase.CommitTransaction(trans);
}

OnlineRewardMgr::RewardHistory* OnlineRewardMgr::GetHistory(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistory.empty())
        return nullptr;

    auto const& itr = _rewardHistory.find(lowGuid);
    if (itr == _rewardHistory.end())
        return nullptr;

    return &itr->second;
}

void OnlineRewardMgr::SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, Seconds secondsOnine, uint32 faction, uint32 reputation)
{
    LOG_DEBUG("module", "Send reward for player guid {}. ItemID {}. ItemCount {}. RewardSeconds {}. Faction {}. Reputation {}",
        player->GetGUID().GetCounter(), itemID, itemCount, secondsOnine.count(), faction, reputation);

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

    if (faction)
    {
        ReputationMgr& repMgr = player->GetReputationMgr();
        auto const& factionEntry = sFactionStore.LookupEntry(faction);
        if (factionEntry)
        {
            repMgr.SetOneFactionReputation(factionEntry, reputation, true);
            repMgr.SendState(repMgr.GetState(factionEntry));
        }
    }

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

void OnlineRewardMgr::AddHistory(ObjectGuid::LowType lowGuid, Seconds seconds)
{
    auto const& history = GetHistory(lowGuid);
    if (!history)
    {
        RewardHistory rewardHistory;
        rewardHistory.first.emplace_back(seconds);
        _rewardHistory.emplace(lowGuid, rewardHistory);
        return;
    }

    history->first.emplace_back(seconds);
}

void OnlineRewardMgr::AddHistory(ObjectGuid::LowType lowGuid, Seconds perTime, Seconds rewardTime)
{
    int32 convertedPerTime = static_cast<int32>(perTime.count());

    auto const& history = GetHistory(lowGuid);
    if (!history)
    {
        RewardHistory rewardHistory;
        rewardHistory.second.emplace(convertedPerTime, rewardTime);
        _rewardHistory.emplace(lowGuid, rewardHistory);
        return;
    }

    auto& perTimeStore = history->second;

    auto const& itr = perTimeStore.find(convertedPerTime);
    if (itr != perTimeStore.end())
    {
        itr->second = rewardTime;
        return;
    }

    perTimeStore.emplace(convertedPerTime, rewardTime);
}

bool OnlineRewardMgr::IsExistHistory(ObjectGuid::LowType lowGuid)
{
    return _rewardHistory.find(lowGuid) != _rewardHistory.end();
}

void OnlineRewardMgr::AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result)
{
    if (!result)
        return;

    ASSERT(_rewardHistory.find(lowGuid) == _rewardHistory.end());

    auto const& [perOnline, perTime] = result->FetchTuple<std::string_view, std::string_view>();

    RewardHistory rewardHistory;

    for (auto const& stringSeconds : Warhead::Tokenize(perOnline, ',', false))
    {
        auto const& seconds = Warhead::StringTo<int32>(stringSeconds);
        if (!seconds || !*seconds)
        {
            LOG_ERROR("module", "> OnlineRewardMgr::AddRewardHistoryAsync: Error at extract seconds from '{}'", stringSeconds);
            continue;
        }

        rewardHistory.first.emplace_back(Seconds(*seconds));
    }

    for (auto const& perTimeString : Warhead::Tokenize(perTime, ',', false))
    {
        auto perTimeData = Warhead::Tokenize(perTimeString, ':', false);
        if (perTimeData.size() != 2)
        {
            LOG_ERROR("module", "> OnlineRewardMgr::AddRewardHistoryAsync: Error at extract `perTimeString` from '{}'", perTimeString);
            continue;
        }

        auto perTime = Warhead::StringTo<int32>(perTimeData.at(0));
        auto rewardTime = Warhead::StringTo<int32>(perTimeData.at(1));

        if (!perTime || !rewardTime)
        {
            LOG_ERROR("module", "> OnlineRewardMgr::AddRewardHistoryAsync: Error at extract `perTime` or `rewardTime` from '{}'", perTimeString);
            continue;
        }

        rewardHistory.second.emplace(*perTime, Seconds(*rewardTime));
    }

    _rewardHistory.emplace(lowGuid, rewardHistory);
    LOG_DEBUG("module", "> OR: Added history for player with guid {}", lowGuid);
}

bool OnlineRewardMgr::IsRewarded(ObjectGuid::LowType lowGuid, Seconds seconds)
{
    auto const& history = GetHistory(lowGuid);
    if (!history)
        return false;

    auto const& perOnlineStore = history->first;
    return std::find(std::begin(perOnlineStore), std::end(perOnlineStore), seconds) != std::end(perOnlineStore);
}

bool OnlineRewardMgr::IsRewarded(ObjectGuid::LowType lowGuid, Seconds perTime, Seconds rewardTime)
{
    auto const& history = GetHistory(lowGuid);
    if (!history)
        return false;

    auto const& perTimeStore = history->second;
    auto const& itr = perTimeStore.find(static_cast<int32>(perTime.count()));
    return itr != std::end(perTimeStore) && itr->second >= rewardTime;
}

void OnlineRewardMgr::CheckPlayersForReward(bool isPerOnline, Seconds seconds, uint32 itemID, uint32 itemCount, uint32 faction, uint32 reputation)
{
    auto const& sessions = sWorld->GetAllSessions();
    if (sessions.empty())
        return;

    auto AddToStore = [this](ObjectGuid::LowType playerGuid, Seconds seconds, uint32 itemID, uint32 itemCount, uint32 faction, uint32 reputation)
    {
        auto const& itr = _rewardPending.find(playerGuid);
        if (itr == _rewardPending.end())
        {
            _rewardPending.emplace(playerGuid, std::vector<RewardPending>{ { seconds, itemID, itemCount, faction, reputation } });
            return;
        }

        itr->second.emplace_back(seconds, itemID, itemCount, faction, reputation);
    };

    auto CanAddPendingPerOnline = [this, seconds](ObjectGuid::LowType lowGuid, Seconds playedTimeSec)
    {
        // Go next if rewarded
        if (IsRewarded(lowGuid, seconds))
            return false;

        // End reward :/
        if (seconds > playedTimeSec)
            return false;

        return true;
    };

    auto CanAddPendingPerTime = [this, seconds](ObjectGuid::LowType lowGuid, Seconds playedTimeSec, Seconds diffTime)
    {
        return !IsRewarded(lowGuid, seconds, diffTime);
    };

    // Check all players in world
    for (auto const& [accountID, session] : sessions)
    {
        auto const& player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        auto const lowGuid = player->GetGUID().GetCounter();
        Seconds playedTimeSec{ player->GetTotalPlayedTime() };

        if (isPerOnline && CanAddPendingPerOnline(lowGuid, playedTimeSec))
        {
            AddToStore(lowGuid, seconds, itemID, itemCount, faction, reputation);
            AddHistory(lowGuid, seconds);
        }

        if (!isPerOnline)
        {
            for (Seconds diffTime{ seconds }; diffTime < playedTimeSec; diffTime += seconds)
            {
                if (CanAddPendingPerTime(lowGuid, playedTimeSec, diffTime))
                {
                    AddToStore(lowGuid, diffTime, itemID, itemCount, faction, reputation);
                    AddHistory(lowGuid, seconds, diffTime);
                }
            }
        }
    }
}

void OnlineRewardMgr::SendRewards()
{
    for (auto const& [lowGuid, rewards] : _rewardPending)
    {
        auto player = ObjectAccessor::FindPlayerByLowGUID(lowGuid);
        if (!player)
        {
            LOG_FATAL("module", "> OR::RewardPlayers: Try reward non existing player (maybe offline) with guid {}. Skip reward, try next time", lowGuid);
            DeleteRewardHistory(lowGuid);
            continue;
        }

        for (auto const& [seconds, itemID, itemCount, faction, reputation] : rewards)
            SendRewardForPlayer(player, itemID, itemCount, seconds, faction, reputation);
    }

    _rewardPending.clear();
}
