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

#include "QuestConditions.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "StopWatch.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "BattlegroundMgr.h"
#include "AchievementMgr.h"

namespace
{
    constexpr uint64 MAKE_PAIR64(uint32 l, uint32 h)
    {
        return uint64(l | (uint64(h) << 32));
    }
}

QuestConditionsMgr* QuestConditionsMgr::instance()
{
    static QuestConditionsMgr instance;
    return &instance;
}

void QuestConditionsMgr::Initialize()
{
    if (!_isEnable)
        return;

    LoadQuestConditionsKilledMonsterCredit();
    LoadQuestConditions();
}

void QuestConditionsMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("QC.Enable");

    /*if (!_isEnable)
        return;*/
}

void QuestConditionsMgr::LoadQuestConditionsKilledMonsterCredit()
{
    LOG_INFO("server.loading", "Loading warhead quest conditions KMC...");

    StopWatch sw;
    _kmc.clear();

    QueryResult result = WorldDatabase.Query("SELECT `NpcEntry`, `QuestConditionType`, `Value` FROM `wh_quest_conditions_kmc`");
    if (!result)
    {
        LOG_INFO("module", ">> Loaded 0 warhead quest conditions KMC. DB table `wh_quest_conditions_kmc` is empty.");
        LOG_WARN("module", "> Disable this module");
        LOG_INFO("module", "");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [npcEntry, questConditionType, value] = result->FetchTuple<uint32, uint8, uint32>();

        if (!sObjectMgr->GetCreatureTemplate(npcEntry))
        {
            LOG_ERROR("module", "> QuestConditions: Not found creature with id: {}. Skip", npcEntry);
            continue;
        }

        if (questConditionType >= MAX_QUEST_CONDITION_TYPE)
        {
            LOG_ERROR("module", "> QuestConditions: Incorrect quest condition type: {}. Skip", questConditionType);
            continue;
        }

        _kmc.emplace(MAKE_PAIR64(value, questConditionType), npcEntry);

    } while (result->NextRow());

    if (_kmc.empty())
    {
        LOG_INFO("server.loading", ">> Loaded 0 warhead quest conditions KMC in {}. Disable module", sw);
        LOG_INFO("server.loading", "");
        _isEnable = false;
        return;
    }

    LOG_INFO("server.loading", ">> Loaded {} warhead quest conditions KMC in {}", _kmc.size(), sw);
    LOG_INFO("server.loading", "");
}

void QuestConditionsMgr::LoadQuestConditions()
{
    if (!_isEnable)
        return;

    LOG_INFO("server.loading", "Loading warhead quest conditions...");

    StopWatch sw;
    _conditions.clear();

    QueryResult result = WorldDatabase.Query("SELECT `QuestID`, `UseSpellID`, `UseSpellCount`, `WinBGType`, `WinBGCount`, `WinArenaType`, "
        "`WinArenaCount`, `CompleteAchievementID`, `CompleteQuestID`, `EquipItemID` FROM `wh_quest_conditions`");
    if (!result)
    {
        LOG_INFO("module", ">> Loaded 0 warhead quest conditions. DB table `wh_quest_conditions` is empty.");
        LOG_WARN("module", "> Disable this module");
        LOG_INFO("module", "");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [questID, useSpellID, useSpellCount, winBGType, winBGCount, winArenaType, winArenaCount, completeAchievementID, completeQuestID, equipItemID] =
            result->FetchTuple<uint32, uint32, uint32, uint32, uint32, uint32, uint32, uint32, uint32, uint32>();

        if (!sObjectMgr->GetQuestTemplate(questID))
        {
            LOG_ERROR("module", "> QuestConditions: Not found quest with id: {}. Skip", questID);
            continue;
        }

        if (useSpellID && !sSpellMgr->GetSpellInfo(useSpellID))
        {
            LOG_ERROR("module", "> QuestConditions: Not found spell with id: {}. Skip", useSpellID);
            continue;
        }

        if (winBGType && !sBattlegroundMgr->GetBattlegroundTemplate(static_cast<BattlegroundTypeId>(winBGType)))
        {
            LOG_ERROR("module", "> QuestConditions: Not found bg with type: {}. Skip", winBGType);
            continue;
        }

        if (completeAchievementID && !sAchievementMgr->GetAchievement(completeAchievementID))
        {
            LOG_ERROR("module", "> QuestConditions: Not found achievement with id: {}. Skip", completeAchievementID);
            continue;
        }

        if (completeQuestID && !sObjectMgr->GetQuestTemplate(completeQuestID))
        {
            LOG_ERROR("module", "> QuestConditions: Not found quest complete with id: {}. Skip", completeQuestID);
            continue;
        }

        if (equipItemID && !sObjectMgr->GetItemTemplate(equipItemID))
        {
            LOG_ERROR("module", "> QuestConditions: Not found item with id: {}. Skip", equipItemID);
            continue;
        }

        _conditions.emplace(questID, QuestCondition{ questID, useSpellID, useSpellCount, winBGType, winBGCount, winArenaType, winArenaCount, completeAchievementID, completeQuestID, equipItemID });

    } while (result->NextRow());

    if (_conditions.empty())
    {
        LOG_INFO("server.loading", ">> Loaded 0 warhead quest conditions in {}. Disable module", sw);
        LOG_INFO("server.loading", "");
        _isEnable = false;
        return;
    }

    LOG_INFO("server.loading", ">> Loaded {} warhead quest conditions in {}", _conditions.size(), sw);
    LOG_INFO("server.loading", "");
}

// Hooks
bool QuestConditionsMgr::CanCompleteQuest(Player* player, Quest const* questInfo)
{
    if (!_isEnable || !player || !questInfo)
        return true;

    auto const& questCondtion = GetQuestCondition(questInfo->GetQuestId());
    if (!questCondtion)
        return true;

    auto const& playerQuestConditionsStore = GetPlayerConditions(player->GetGUID());
    if (!playerQuestConditionsStore)
        return false;

    auto const& itr = playerQuestConditionsStore->find(questInfo->GetQuestId());
    if (itr == playerQuestConditionsStore->end())
        return false;

    auto const& playerQuestCondition = itr->second;

    if (questCondtion->UseSpellCount && (questCondtion->UseSpellCount != playerQuestCondition.UseSpellCount))
        return false;

    if (questCondtion->WinBGCount && (questCondtion->WinBGCount != playerQuestCondition.WinBGCount))
        return false;

    if (questCondtion->WinArenaCount && (questCondtion->WinArenaCount != playerQuestCondition.WinArenaCount))
        return false;

    if (questCondtion->CompleteAchievementID && (questCondtion->CompleteAchievementID != playerQuestCondition.CompleteAchievementID))
        return false;

    if (questCondtion->CompleteQuestID && (questCondtion->CompleteQuestID != playerQuestCondition.CompleteQuestID))
        return false;

    if (questCondtion->EquipItemID && (questCondtion->EquipItemID != playerQuestCondition.EquipItemID))
        return false;

    return true;
}

void QuestConditionsMgr::OnPlayerCompleteQuest(Player* player, Quest const* quest)
{
    if (!_isEnable || !player || !quest)
        return;

    for (auto const& [questID_, condition] : _conditions)
    {
        if (condition.CompleteQuestID == quest->GetQuestId())
            UpdateQuestConditionForPlayer(player, condition.QuestID, quest->GetQuestId());
    }

    auto const& questCondtion = GetQuestCondition(quest->GetQuestId());
    if (!questCondtion)
        return;

    // Delete non need data
    CharacterDatabase.Execute("DELETE FROM `wh_characters_quest_conditions` WHERE `PlayerGuid` = {} AND `QuestID` = {}", player->GetGUID().GetRawValue(), quest->GetQuestId());
}

void QuestConditionsMgr::OnPlayerAddQuest(Player* player, Quest const* quest)
{
    if (!_isEnable || !player || !quest)
        return;

    auto const& questCondtion = GetQuestCondition(quest->GetQuestId());
    if (!questCondtion)
        return;

    if (!questCondtion->CompleteQuestID)
        return;

    if (player->GetQuestStatus(questCondtion->CompleteQuestID) != QUEST_STATUS_REWARDED)
        return;

    auto playerQuestCondition = MakeQuestConditionForPlayer(player->GetGUID(), quest->GetQuestId());
    ASSERT(playerQuestCondition);

    if (!playerQuestCondition->CompleteQuestID)
        return;

    auto kmc = GetKilledMonsterCredit(questCondtion->CompleteQuestID, QuestConditionType::CompleteQuest);
    ASSERT(kmc);
    player->KilledMonsterCredit(*kmc);
}

QuestCondition const* QuestConditionsMgr::GetQuestCondition(uint32 questID)
{
    return Warhead::Containers::MapGetValuePtr(_conditions, questID);
}

QuestConditions* QuestConditionsMgr::GetPlayerConditions(ObjectGuid playerGuid)
{
    return Warhead::Containers::MapGetValuePtr(_playerConditions, playerGuid);
}

QuestCondition* QuestConditionsMgr::MakeQuestConditionForPlayer(ObjectGuid playerGuid, uint32 questID)
{
    auto const& playerQuestConditionsStore = GetPlayerConditions(playerGuid);
    if (playerQuestConditionsStore)
    {
        auto const& itr = playerQuestConditionsStore->find(questID);
        if (itr != playerQuestConditionsStore->end())
            return &itr->second;

        auto const& [itr2, isEmplace] = playerQuestConditionsStore->emplace(questID, QuestCondition{});
        return &itr2->second;
    }

    QuestConditions qc;
    qc.emplace(questID, QuestCondition{});
    auto const& [itr, isEmplace] = _playerConditions.emplace(playerGuid, std::move(qc));
    return &itr->second.begin()->second;
}

void QuestConditionsMgr::SavePlayerConditionToDB(ObjectGuid playerGuid, uint32 questID)
{
    if (!_isEnable)
        return;

    auto const& playerQuestConditionsStore = GetPlayerConditions(playerGuid);
    if (!playerQuestConditionsStore)
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    auto const& itr = playerQuestConditionsStore->find(questID);
    if (itr == playerQuestConditionsStore->end())
        return;

    auto const& playerQuestCondition = itr->second;

    trans->Append("DELETE FROM `wh_characters_quest_conditions` WHERE `PlayerGuid` = {} AND `QuestID` = {}", playerGuid.GetRawValue(), questID);
    trans->Append("INSERT INTO `wh_characters_quest_conditions` "
        "(`PlayerGuid`, `QuestID`, `UseSpellCount`, `WinBGCount`, `WinArenaCount`, `CompleteAchievementID`, `CompleteQuestID`, `EquipItemID`) "
        "VALUES ({}, {}, {}, {}, {}, {}, {}, {})", playerGuid.GetRawValue(), questID,
        playerQuestCondition.UseSpellCount, playerQuestCondition.WinBGCount, playerQuestCondition.WinArenaCount,
        playerQuestCondition.CompleteAchievementID, playerQuestCondition.CompleteQuestID, playerQuestCondition.EquipItemID);

    CharacterDatabase.CommitTransaction(trans);
}

uint32 const* QuestConditionsMgr::GetKilledMonsterCredit(uint32 value, QuestConditionType type)
{
    auto staticType = static_cast<uint32>(type);

    if (type >= QuestConditionType::Max)
    {
        LOG_ERROR("module", "> QuestConditionsMgr: Incorrect type for get KMC. Value: {}. Type: {}", value, staticType);
        return nullptr;
    }

    return Warhead::Containers::MapGetValuePtr(_kmc, MAKE_PAIR64(value, staticType));
}

void QuestConditionsMgr::UpdateQuestConditionForPlayer(Player* player, uint32 questID, uint32 completedQuestID)
{
    auto playerQuestCondition = MakeQuestConditionForPlayer(player->GetGUID(), questID);
    ASSERT(playerQuestCondition);

    if (playerQuestCondition->CompleteQuestID)
        return;

    playerQuestCondition->CompleteQuestID = completedQuestID;
    SavePlayerConditionToDB(player->GetGUID(), questID);

    if (player->GetQuestStatus(questID) != QUEST_STATUS_INCOMPLETE)
        return;

    auto kmc = GetKilledMonsterCredit(completedQuestID, QuestConditionType::CompleteQuest);
    ASSERT(kmc);
    player->KilledMonsterCredit(*kmc);
}

void QuestConditionsMgr::OnPlayerLogin(Player* player)
{
    if (!_isEnable)
        return;

    auto const& itr = _playerConditions.find(player->GetGUID());
    ASSERT(itr == _playerConditions.end(), "Found conditions for player {}", player->GetGUID().ToString());

    player->GetSession()->GetQueryProcessor().AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `QuestID`, `UseSpellCount`, `WinBGCount`, `WinArenaCount`, "
            "`CompleteAchievementID`, `CompleteQuestID`, `EquipItemID` FROM `wh_characters_quest_conditions` WHERE `PlayerGuid` = {}", player->GetGUID().GetRawValue())).
        WithCallback([this, playerGuid = player->GetGUID()](QueryResult result)
        {
            if (!result)
                return;

            do
            {
                auto const& [questID, useSpellCount, winBGCount, winArenaCount, completeAchievementID, completeQuestID, equipItemID] =
                    result->FetchTuple<uint32, uint32, uint32, uint32, uint32, uint32, uint32>();

                QuestConditions qc;
                qc.emplace(questID, QuestCondition{ questID, 0, useSpellCount, 0, winBGCount, 0, winArenaCount, completeAchievementID, completeQuestID, equipItemID });

                auto const& playerQuestConditionsStore = GetPlayerConditions(playerGuid);
                if (playerQuestConditionsStore)
                {
                    playerQuestConditionsStore->emplace(questID, QuestCondition{ questID, 0, useSpellCount, 0, winBGCount, 0, winArenaCount, completeAchievementID, completeQuestID, equipItemID });
                    continue;
                }

                _playerConditions.emplace(playerGuid, std::move(qc));
            } while (result->NextRow());

            LOG_TRACE("module", "> QuestConditionsMgr: Added conditions for: {}", playerGuid.ToString());
        }));
}

void QuestConditionsMgr::OnPlayerLogout(Player* player)
{
    if (!_isEnable)
        return;

    _playerConditions.erase(player->GetGUID());
}
