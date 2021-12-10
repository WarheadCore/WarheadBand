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
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "Chat.h"
#include "ExternalMail.h"
#include "ModuleLocale.h"
#include "Tokenize.h"
#include "StringConvert.h"

OnlineReward* OnlineReward::instance()
{
    static OnlineReward instance;
    return &instance;
}

void OnlineReward::InitSystem()
{
    if (MOD_CONF_GET_BOOL("OnlineReward.Enable") && !MOD_CONF_GET_BOOL("OnlineReward.PerOnline.Enable") && !MOD_CONF_GET_BOOL("OnlineReward.PerTime.Enable"))
    {
        sModulesConfig->SetOption<bool>("OnlineReward.Enable", false);
        return;
    }

    LoadRewards();
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

        uint32 seconds = fields[0].GetUInt32();

        RewardPlayedTime RPT;
        RPT.ItemID = fields[1].GetUInt32();
        RPT.ItemCount = fields[2].GetUInt32();

        // Проверка
        if (!seconds)
        {
            LOG_ERROR("module", "-> Time = 0? Really? Skip...");
            continue;
        }

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(RPT.ItemID);
        if (!itemTemplate)
        {
            LOG_ERROR("module", "-> Item with number {} not found. Skip", RPT.ItemID);
            continue;
        }

        if (!RPT.ItemCount)
        {
            LOG_ERROR("module", "-> Item count for number {} - 0. Set to 1", RPT.ItemID);
            RPT.ItemCount = 1;
        }

        _rewards.emplace(seconds, RPT);

    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} reward in {} ms", _rewards.size(), GetMSTimeDiffToNow(msTime));
    LOG_INFO("module", "");
}

void OnlineReward::AddRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (IsExistHistory(lowGuid))
        return;

    QueryResult result = CharacterDatabase.PQuery("SELECT `RewardedPerOnline`, `RewardedPerHour` FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid);
    if (!result)
        return;

    auto fields = result->Fetch();

    RewardTimeHistory _data;

    for (auto const& itr : Warhead::Tokenize(fields[0].GetStringView(), ',', true))
        _data.PerOnline.insert(Warhead::StringTo<uint32>(itr).value_or(0));

    _data.PerTime = fields[1].GetUInt32();

    _rewardHistoryDB.emplace(lowGuid, _data);
}

void OnlineReward::DeleteRewardHistory(ObjectGuid::LowType lowGuid)
{
    if (!IsExistHistory(lowGuid))
        return;

    _rewardHistoryDB.erase(lowGuid);
}

void OnlineReward::RewardPlayers()
{
    if (!MOD_CONF_GET_BOOL("OnlineReward.Enable"))
        return;

    auto sessions = sWorld->GetAllSessions();
    if (sessions.empty())
        return;

    // Send reward
    for (auto const& itr : sessions)
    {
        auto player = itr.second->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        if (MOD_CONF_GET_BOOL("OnlineReward.PerOnline.Enable"))
            RewardPerOnline(player);

        if (MOD_CONF_GET_BOOL("OnlineReward.PerTime.Enable"))
            RewardPerTime(player);
    }

    // Save data to DB
    SaveRewardDB();
}

void OnlineReward::RewardPerOnline(Player* player)
{
    ChatHandler handler(player->GetSession());
    uint32 playedTimeSec = player->GetTotalPlayedTime();
    auto lowGuid = player->GetGUID().GetCounter();

    auto IsRewarded = [&](uint32 rewardSeconds) -> bool
    {
        if (!IsExistHistory(lowGuid))
            return false;

        auto const& secondsFind = GetHistoryPerOnline(lowGuid)->find(rewardSeconds);
        if (secondsFind != GetHistoryPerOnline(lowGuid)->end())
            return true;

        return false;
    };

    for (auto const& [seconds, data] : _rewards)
    {
        // Go next is rewarded
        if (IsRewarded(seconds))
            continue;

        // End reward :/
        if (seconds > playedTimeSec)
            continue;

        SendRewardForPlayer(player, data.ItemID, data.ItemCount, seconds);
    }
}

void OnlineReward::RewardPerTime(Player* player)
{
    auto lowGuid = player->GetGUID().GetCounter();
    uint32 LastRewarded = GetHistoryPerTime(lowGuid);
    uint32 playedTimeSec = player->GetTotalPlayedTime();
    uint32 diffTime = 0;

    while (diffTime < playedTimeSec)
    {
        if (LastRewarded < diffTime)
            SendRewardForPlayer(player, MOD_CONF_GET_INT("OnlineReward.PerTime.ItemID"), MOD_CONF_GET_INT("OnlineReward.PerTime.ItemCount"), diffTime, false);

        diffTime += MOD_CONF_GET_INT("OnlineReward.PerTime.Time");
    }
}

void OnlineReward::SaveRewardDB()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Save data for exist history
    for (auto const& itr : _rewardHistoryDB)
    {
        uint32 lowGuid = itr.first;

        auto GetRewardedData = [&]() -> RewardTimeHistory *
        {
            auto const& _historyData = _rewardHistoryDB.find(lowGuid);
            if (_historyData != _rewardHistoryDB.end())
                return &_historyData->second;

            return nullptr;
        };

        auto _saveData = GetRewardedData();

        // Check data
        if (!_saveData)
            return;

        uint32 dataPerTime = _saveData->PerTime;
        std::string dataPerOnline;

        if (!_saveData->PerOnline.empty())
        {
            // Add rewarded seconds
            for (auto const& itr : _saveData->PerOnline)
                dataPerOnline += std::to_string(itr) + ',';

            // Delete last (,)
            if (!dataPerOnline.empty())
                dataPerOnline.erase(dataPerOnline.end() - 1, dataPerOnline.end());
        }

        // Delele old data
        trans->PAppend("DELETE FROM `online_reward_history` WHERE `PlayerGuid` = {}", lowGuid);

        // Insert new data
        trans->PAppend("INSERT INTO `online_reward_history`(`PlayerGuid`, `RewardedPerOnline`, `RewardedPerHour`) VALUES ({}, '{}', {})", lowGuid, dataPerOnline, dataPerTime);
    }

    CharacterDatabase.CommitTransaction(trans);
}

std::unordered_set<uint32>* OnlineReward::GetHistoryPerOnline(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return nullptr;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return &itr->second.PerOnline;

    return nullptr;
}

uint32 OnlineReward::GetHistoryPerTime(ObjectGuid::LowType lowGuid)
{
    if (_rewardHistoryDB.empty())
        return 0;

    auto const& itr = _rewardHistoryDB.find(lowGuid);
    if (itr != _rewardHistoryDB.end())
        return itr->second.PerTime;

    return 0;
}

void OnlineReward::SendRewardForPlayer(Player* player, uint32 itemID, uint32 itemCount, uint32 secondsOnine, bool isPerOnline /*= true*/)
{
    ChatHandler handler(player->GetSession());
    std::string subject, text;
    std::string playedTimeSecStr = Warhead::Time::ToTimeString<Seconds>(secondsOnine);
    uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());

    subject = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_SUBJECT", localeIndex), playedTimeSecStr);
    text = Warhead::StringFormat(*sModuleLocale->GetModuleString("OR_LOCALE_TEXT", localeIndex), player->GetName(), playedTimeSecStr);

    // Send External mail
    sEM->AddMail(player->GetName(), subject, text, itemID, itemCount, 37688);

    // Save data to DB
    SaveDataForDB(player->GetGUID().GetCounter(), secondsOnine, isPerOnline);

    // Send chat text
    sModuleLocale->SendPlayerMessage(player, "OR_LOCALE_MESSAGE", playedTimeSecStr);
}

void OnlineReward::SaveDataForDB(ObjectGuid::LowType lowGuid, uint32 seconds, bool isPerOnline /*= true*/)
{
    auto const& _historyData = _rewardHistoryDB.find(lowGuid);
    if (_historyData == _rewardHistoryDB.end())
    {
        RewardTimeHistory _data;

        if (isPerOnline)
        {
            _data.PerOnline.insert(seconds);
            _data.PerTime = 0;
        }
        else
            _data.PerTime = seconds;

        _rewardHistoryDB.emplace(lowGuid, _data);
    }
    else
    {
        auto& __data = _historyData->second;

        if (isPerOnline)
            __data.PerOnline.emplace(seconds);
        else
            __data.PerTime = seconds;
    }
}

bool OnlineReward::IsExistHistory(ObjectGuid::LowType lowGuid)
{
    return _rewardHistoryDB.find(lowGuid) != _rewardHistoryDB.end();
}
