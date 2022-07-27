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

void OnlineRewardMgr::LoadConfig(bool reload)
{
    if (reload)
        scheduler.CancelAll();

    _isEnable = MOD_CONF_GET_BOOL("OR.Enable");
    if (!_isEnable)
        return;

    _isPerOnlineEnable = MOD_CONF_GET_BOOL("OR.PerOnline.Enable");
    _isPerTimeEnable = MOD_CONF_GET_BOOL("OR.PerTime.Enable");
    _isForceMailReward = MOD_CONF_GET_BOOL("OR.ForceSendMail.Enable");
    _isEnableCorrectHistory = MOD_CONF_GET_BOOL("OR.CoorectHistory.Enable");

    if (!_isPerOnlineEnable && !_isPerTimeEnable)
    {
        _isEnable = false;
        LOG_ERROR("module.or", "> In module Online Reward disabled all function. Disable module.");
        //return;
    }

    if (reload)
        ScheduleReward();
}

void OnlineRewardMgr::InitSystem()
{
    if (!_isEnable)
        return;

    LoadDBData();

    // Check new rewards
    CorrectDBData();

    // If data empty no need reward players
    if (!_isEnable)
        return;

    ScheduleReward();
}

void OnlineRewardMgr::ScheduleReward()
{
    if (!_isEnable)
        return;

    scheduler.Schedule(30s, [this](TaskContext context)
    {
        RewardPlayers();
        context.Repeat(15min);
    });
}

void OnlineRewardMgr::Update(Milliseconds diff)
{
    if (!_isEnable)
        return;

    scheduler.Update(diff);
    _queryProcessor.ProcessReadyCallbacks();
}

void OnlineRewardMgr::RewardNow()
{
    scheduler.CancelAll();
    RewardPlayers();
    ScheduleReward();
}

void OnlineRewardMgr::LoadDBData()
{
    StopWatch sw;

    LOG_INFO("module.or", "Loading online rewards...");

    _rewards.clear();

    QueryResult result = CharacterDatabase.Query("SELECT `ID`, `IsPerOnline`, `Seconds`, `Items`, `Reputations` FROM `wh_online_rewards`");
    if (!result)
    {
        LOG_WARN("module.or", "> DB table `wh_online_rewards` is empty! Disable module");
        LOG_WARN("module.or", "");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [id, isPerOnline, seconds, items, reputations] =
            result->FetchTuple<uint32, bool, int32, std::string_view, std::string_view>();

        AddReward(id, isPerOnline, Seconds(seconds), items, reputations);
    } while (result->NextRow());

    if (_rewards.empty())
    {
        LOG_INFO("module.or", ">> Loaded 0 online rewards in {}", _rewards.size(), sw);
        LOG_WARN("module.or", ">> Disable module");
        LOG_INFO("module.or", "");
        _isEnable = false;
        return;
    }

    LOG_INFO("module.or", ">> Loaded {} online rewards in {}", _rewards.size(), sw);
    LOG_INFO("module.or", "");
}

bool OnlineRewardMgr::AddReward(uint32 id, bool isPerOnline, Seconds seconds, std::string_view items, std::string_view reputations, ChatHandler* handler /*= nullptr*/)
{
    auto SendErrorMesasge = [handler](std::string_view message)
    {
        LOG_ERROR("module.or", message);

        if (handler)
            handler->SendSysMessage(message);
    };

    if (IsExistReward(id))
    {
        SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::AddReward: Reward with id {} is exist!", id));
        return false;
    }

    // Start checks
    if (seconds == 0s)
    {
        SendErrorMesasge("> OnlineRewardMgr::AddReward: Seconds = 0? Really? Skip...");
        return false;
    }

    auto data = OnlineReward(id, isPerOnline, seconds);
    auto const& itemData = Warhead::Tokenize(items, ',', false);
    auto const& reputationsData = Warhead::Tokenize(reputations, ',', false);

    if (itemData.empty() && reputationsData.empty())
    {
        SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::AddReward: Not found rewards. IsPerOnline?: {}. Seconds: {}", isPerOnline, seconds.count()));
        return false;
    }

    if (!itemData.empty())
    {
        // Items
        for (auto const& pairItems : Warhead::Tokenize(items, ',', false))
        {
            auto itemTokens = Warhead::Tokenize(pairItems, ':', false);
            if (itemTokens.size() != 2)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: Error at extract `itemTokens` from '{}'", pairItems));
                continue;
            }

            auto itemID = Warhead::StringTo<uint32>(itemTokens.at(0));
            auto itemCount = Warhead::StringTo<uint32>(itemTokens.at(1));

            if (!itemID || !itemCount)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: Error at extract `itemID` or `itemCount` from '{}'", pairItems));
                continue;
            }

            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(*itemID);
            if (!itemTemplate)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: Item with number {} not found. Skip", *itemID));
                continue;
            }

            if (*itemID && !*itemCount)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: For item with number {} item count set 0. Skip", *itemID));
                continue;
            }

            data.Items.emplace_back(*itemID, *itemCount);
        }
    }

    if (!reputationsData.empty())
    {
        // Reputations
        for (auto const& pairReputations : Warhead::Tokenize(reputations, ',', false))
        {
            auto reputationsTokens = Warhead::Tokenize(pairReputations, ':', false);
            if (reputationsTokens.size() != 2)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: Error at extract `reputationsTokens` from '{}'", pairReputations));
                continue;
            }

            auto factionID = Warhead::StringTo<uint32>(reputationsTokens.at(0));
            auto reputationCount = Warhead::StringTo<uint32>(reputationsTokens.at(1));

            if (!factionID || !reputationCount)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::LoadDBData: Error at extract `factionID` or `reputationCount` from '{}'", pairReputations));
                continue;
            }

            FactionEntry const* factionEntry = sFactionStore.LookupEntry(*factionID);
            if (!factionEntry)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineReward: Not found faction with id {}. Skip", *factionID));
                continue;
            }

            if (factionEntry->reputationListID < 0)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineReward: Faction {} can't have reputation. Skip", *factionID));
                continue;
            }

            if (*reputationCount > ReputationMgr::Reputation_Cap)
            {
                SendErrorMesasge(Warhead::StringFormat("> OnlineReward: reputation count {} > repitation cap {}. Skip", *reputationCount, ReputationMgr::Reputation_Cap));
                continue;
            }

            data.Reputations.emplace_back(*factionID, *reputationCount);
        }
    }

    if (data.Items.empty() && data.Reputations.empty())
    {
        SendErrorMesasge(Warhead::StringFormat("> OnlineRewardMgr::AddReward: Not found rewards after check items and reputations. IsPerOnline?: {}. Seconds: {}", isPerOnline, seconds.count()));
        return false;
    }

    _rewards.emplace(id, data);

    // If add from command - save to db
    if (handler)
    {
        CharacterDatabase.Execute("INSERT INTO `wh_online_rewards` (`ID`, `IsPerOnline`, `Seconds`, `Items`, `Reputations`) VALUES ({}, {:d}, {}, '{}', '{}')",
            id, isPerOnline, seconds.count(), items, reputations);
    }

    return true;
}

void OnlineRewardMgr::AddRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
        return;

    if (IsExistHistory(lowGuid))
        return;

    _queryProcessor.AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `RewardID`, `RewardedSeconds` FROM `wh_online_rewards_history` WHERE `PlayerGuid` = {}", lowGuid)).
        WithCallback(std::bind(&OnlineRewardMgr::AddRewardHistoryAsync, this, lowGuid, std::placeholders::_1)));
}

void OnlineRewardMgr::DeleteRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!_isEnable)
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

        LOG_DEBUG("module.or", "> OR: Start rewars players...");

        auto const& sessions = sWorld->GetAllSessions();
        if (sessions.empty())
            return;

        for (auto const& [accountID, session] : sessions)
        {
            auto const& player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            auto const lowGuid = player->GetGUID().GetCounter();
            Seconds playedTimeSec{ player->GetTotalPlayedTime() };

            for (auto const& [rewardID, reward] : _rewards)
            {
                if (reward.IsPerOnline && !_isPerOnlineEnable)
                    continue;

                if (!reward.IsPerOnline && !_isPerTimeEnable)
                    continue;

                CheckPlayerForReward(lowGuid, playedTimeSec, &reward);
            }
        }

        // Send reward
        SendRewards();

        // Save data to DB
        SaveRewardHistoryToDB();

        LOG_DEBUG("module.or", "> OR: End rewars players. Elapsed {}", sw);
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
        // Delele old data
        trans->Append("DELETE FROM `wh_online_rewards_history` WHERE `PlayerGuid` = {}", lowGuid);

        for (auto const& [rewardID, seconds] : history)
        {
             // Insert new data
            trans->Append("INSERT INTO `wh_online_rewards_history` (`PlayerGuid`, `RewardID`, `RewardedSeconds`) VALUES ({}, '{}', {})", lowGuid, rewardID, seconds.count());
        }
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

Seconds OnlineRewardMgr::GetHistorySecondsForReward(ObjectGuid::LowType lowGuid, uint32 id)
{
    if (_rewardHistory.empty())
        return 0s;

    auto const& itr = _rewardHistory.find(lowGuid);
    if (itr == _rewardHistory.end())
        return 0s;

    for (auto const& [rewardID, seconds] : itr->second)
        if (rewardID == id)
            return seconds;

    return 0s;
}

void OnlineRewardMgr::SendRewardForPlayer(Player* player, uint32 rewardID)
{
    //LOG_TRACE("module.or", "Send reward for player guid {}. RewardSeconds {}", player->GetGUID().GetCounter(), secondsOnine.count());

    auto const& onlineReward = GetOnlineReward(rewardID);
    if (!onlineReward)
        return;

    ChatHandler handler{ player->GetSession() };
    std::string playedTimeSecStr{ Warhead::Time::ToTimeString(onlineReward->Seconds, 3, TimeFormat::FullText) };
    uint8 localeIndex{ static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex()) };

    auto SendItemsViaMail = [player, onlineReward, playedTimeSecStr, &localeIndex]()
    {
        auto const mailSubject = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_SUBJECT", localeIndex), playedTimeSecStr);
        auto const MailText = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_TEXT", localeIndex), player->GetName(), playedTimeSecStr);

        // Send External mail
        for (auto const& [itemID, itemCount] : onlineReward->Items)
            sExternalMail->AddMail(player->GetName(), mailSubject, MailText, itemID, itemCount, 37688);
    };

    if (!onlineReward->Reputations.empty())
    {
        for (auto const& [faction, reputation] : onlineReward->Reputations)
        {
            ReputationMgr& repMgr = player->GetReputationMgr();
            auto const& factionEntry = sFactionStore.LookupEntry(faction);
            if (factionEntry)
            {
                repMgr.SetOneFactionReputation(factionEntry, reputation, true);
                repMgr.SendState(repMgr.GetState(factionEntry));
            }
        }
    }

    if (_isForceMailReward && !onlineReward->Items.empty())
    {
        SendItemsViaMail();

        // Send chat text
        sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE_MAIL", playedTimeSecStr);
        return;
    }

    if (!onlineReward->Items.empty())
    {
        for (auto const& [itemID, itemCount] : onlineReward->Items)
        {
            if (!player->AddItem(itemID, itemCount))
            {
                SendItemsViaMail();

                // Send chat text
                sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_NOT_ENOUGH_BAG");
            }
        }
    }

    // Send chat text
    sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE_IN_GAME", playedTimeSecStr);
}

void OnlineRewardMgr::AddHistory(ObjectGuid::LowType lowGuid, uint32 rewardId, Seconds playerOnlineTime)
{
    auto history = GetHistory(lowGuid);
    if (!history)
    {
        _rewardHistory.emplace(lowGuid, RewardHistory{ { rewardId, playerOnlineTime } });
        return;
    }

    for (auto& [rewardID, seconds] : *history)
    {
        if (rewardID == rewardId)
        {
            seconds = playerOnlineTime;
            return;
        }
    }

    history->emplace_back(rewardId, playerOnlineTime);
}

bool OnlineRewardMgr::IsExistHistory(ObjectGuid::LowType lowGuid)
{
    return _rewardHistory.contains(lowGuid);
}

void OnlineRewardMgr::AddRewardHistoryAsync(ObjectGuid::LowType lowGuid, QueryResult result)
{
    if (!result)
        return;

    std::lock_guard<std::mutex> guard(_playerLoadingLock);

    if (_rewardHistory.contains(lowGuid))
    {
        LOG_FATAL("module.or", "> OR: Time to ping @Winfidonarleyan. Code 2");
        _rewardHistory.erase(lowGuid);
    }

    RewardHistory rewardHistory;

    do
    {
        auto const& [rewardID, rewardedSeconds] = result->FetchTuple<uint32, Seconds>();
        rewardHistory.emplace_back(rewardID, rewardedSeconds);

    } while (result->NextRow());

    _rewardHistory.emplace(lowGuid, rewardHistory);
    LOG_DEBUG("module.or", "> OR: Added history for player with guid {}", lowGuid);
}

void OnlineRewardMgr::CheckPlayerForReward(ObjectGuid::LowType lowGuid, Seconds playedTime, OnlineReward const* onlineReward)
{
    if (!onlineReward || !lowGuid || playedTime == 0s)
        return;

    auto AddToStore = [this, onlineReward](ObjectGuid::LowType playerGuid)
    {
        auto const& itr = _rewardPending.find(playerGuid);
        if (itr == _rewardPending.end())
        {
            _rewardPending.emplace(playerGuid, RewardPending{ onlineReward->ID });
            return;
        }

        itr->second.emplace_back(onlineReward->ID);
    };

    auto rewardedSeconds = GetHistorySecondsForReward(lowGuid, onlineReward->ID);

    if (onlineReward->IsPerOnline)
    {
        if (playedTime >= onlineReward->Seconds && rewardedSeconds == 0s)
        {
            AddToStore(lowGuid);
            AddHistory(lowGuid, onlineReward->ID, playedTime);
        }

        return;
    }

    if (!onlineReward->IsPerOnline)
    {
        for (Seconds diffTime{ onlineReward->Seconds }; diffTime < playedTime; diffTime += onlineReward->Seconds)
        {
            if (rewardedSeconds < diffTime)
                AddToStore(lowGuid);
        }
    }

    AddHistory(lowGuid, onlineReward->ID, playedTime);
}

void OnlineRewardMgr::SendRewards()
{
    if (_rewardPending.empty())
        return;

    for (auto const& [lowGuid, rewards] : _rewardPending)
    {
        auto player = ObjectAccessor::FindPlayerByLowGUID(lowGuid);
        if (!player)
        {
            LOG_FATAL("module.or", "> OR::RewardPlayers: Try reward non existing player (maybe offline) with guid {}. Skip reward, try next time", lowGuid);
            DeleteRewardHistory(lowGuid);
            continue;
        }

        for (auto const& rewardID : rewards)
            SendRewardForPlayer(player, rewardID);
    }

    _rewardPending.clear();
}

bool OnlineRewardMgr::IsExistReward(uint32 id)
{
    return _rewards.contains(id);
}

bool OnlineRewardMgr::DeleteReward(uint32 id)
{
    auto const& erased = _rewards.erase(id);
    if (!erased)
        return false;

    CharacterDatabase.Execute("DELETE FROM `wh_online_rewards` WHERE `ID` = {}", id);
    return true;
}

OnlineReward const* OnlineRewardMgr::GetOnlineReward(uint32 id)
{
    return Warhead::Containers::MapGetValuePtr(_rewards, id);
}

void OnlineRewardMgr::CorrectDBData()
{
    if (!_isEnable)
        return;

    // Drop old table
    CharacterDatabase.Execute("DROP TABLE IF EXISTS `online_reward_history`");

    if (!_isEnableCorrectHistory)
        return;

    LOG_INFO("module.or", "> OR: Start check new rewards and fill history");

    StopWatch sw;
    std::size_t count{ 0 };

    QueryResult result = CharacterDatabase.Query("SELECT `or`.`ID`, `orh`.`RewardID` FROM `wh_online_rewards_history` AS `orh` RIGHT JOIN `wh_online_rewards` AS `or` ON `orh`.`RewardID` = `or`.`ID`");
    if (!result)
    {
        LOG_FATAL("module.or", "> OR: Time to ping @Winfidonarleyan. Code 1");
        return;
    }

    do
    {
        auto const& fileds = result->Fetch();

        if (!fileds[1].IsNull())
            continue;

        auto const rewardID = fileds[0].Get<uint32>();

        auto testRewardQuery = CharacterDatabase.Query("SELECT `ch`.`guid` FROM `characters` AS `ch`, `wh_online_rewards` AS `or` WHERE `ch`.`totaltime` >= `or`.`Seconds` AND `or`.`ID` = {}", rewardID);
        if (!testRewardQuery)
            continue;

        LOG_INFO("module.or", "> OR: Fill rewards history for reward with id: {}", rewardID);

        CharacterDatabase.Execute("INSERT INTO `wh_online_rewards_history` (`PlayerGuid`, `RewardID`, `RewardedSeconds`) "
            "(SELECT `ch`.`guid`, `or`.`ID`, `ch`.`totaltime` FROM `characters` AS `ch`, `wh_online_rewards` AS `or` WHERE `ch`.`totaltime` >= `or`.`Seconds` AND `or`.`ID` = {})", rewardID);

        count++;

    } while (result->NextRow());

    if (!count)
    {
        LOG_INFO("module.or", "> OR: Not found new rewards");
        LOG_INFO("module.or", "");
        return;
    }

    LOG_INFO("module.or", "> OR: Filled {} rewards in {}", count, sw);
    LOG_INFO("module.or", "");
}
