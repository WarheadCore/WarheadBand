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

#include "QuestConditions.h"
#include "AchievementMgr.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "GameLocale.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "StopWatch.h"

namespace
{
    constexpr uint64 MAKE_PAIR64(uint32 l, uint32 h)
    {
        return uint64(l | (uint64(h) << 32));
    }
}

bool QuestCondition::IsFoundConditionForType(QuestConditionType type) const
{
    switch (type)
    {
        case QuestConditionType::UseSpell: return UseSpellID > 0;
        case QuestConditionType::WinBG: return WinBGType > 0;
        case QuestConditionType::WinArena: return WinArenaType > 0;
        case QuestConditionType::CompleteAchievement: return CompleteAchievementID > 0;
        case QuestConditionType::CompleteQuest: return CompleteQuestID > 0;
        case QuestConditionType::EquipItem: return EquipItemID > 0;
        default:
            ABORT("> Incorrect QuestConditionType: {}", static_cast<uint8>(type));
            break;
    }

    return false;
}

bool QuestCondition::IsValidConditionForType(QuestConditionType type, uint32 value) const
{
    switch (type)
    {
        case QuestConditionType::UseSpell:
            return UseSpellID == value;
        case QuestConditionType::WinBG:
            return WinBGType == value;
        case QuestConditionType::WinArena:
            return WinArenaType == value;
        case QuestConditionType::CompleteAchievement:
            return CompleteAchievementID == value;
        case QuestConditionType::CompleteQuest:
            return CompleteQuestID == value;
        case QuestConditionType::EquipItem:
            return EquipItemID == value;
        default:
            ABORT("> Incorrect QuestConditionType: {}", static_cast<uint8>(type));
            break;
    }

    return false;
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

        auto const& quest = sObjectMgr->GetQuestTemplate(questID);
        if (!quest)
        {
            LOG_ERROR("module", "> QuestConditions: Not found quest with id: {}. Skip", questID);
            continue;
        }

        if (useSpellID)
        {
            if (!sSpellMgr->GetSpellInfo(useSpellID))
            {
                LOG_ERROR("module", "> QuestConditions: Not found spell with id: {}. Skip", useSpellID);
                continue;
            }

            if (!GetKilledMonsterCredit(useSpellID, QuestConditionType::UseSpell))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for spell with id: {}. Skip", useSpellID);
                continue;
            }
        }

        if (winBGType)
        {
            if (!sBattlegroundMgr->GetBattlegroundTemplate(static_cast<BattlegroundTypeId>(winBGType)))
            {
                LOG_ERROR("module", "> QuestConditions: Not found bg with type: {}. Skip", winBGType);
                continue;
            }

            if (!GetKilledMonsterCredit(winBGType, QuestConditionType::WinBG))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for bg with type: {}. Skip", winBGType);
                continue;
            }

            if (!quest->IsPVPQuest())
            {
                LOG_ERROR("module", "> QuestConditions: For quest {} need type QUEST_TYPE_PVP. Skip", questID);
                continue;
            }

            if (winBGCount <= 0)
            {
                LOG_ERROR("module", "> QuestConditions: Incorrect win bg count ({}) for quest with id: {}. Skip", winBGCount, questID);
                continue;
            }
        }

        if (winArenaType)
        {
            if (!GetKilledMonsterCredit(winArenaType, QuestConditionType::WinArena))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for bg arena type: {}. Skip", winArenaType);
                continue;
            }

            if (!quest->IsPVPQuest())
            {
                LOG_ERROR("module", "> QuestConditions: For quest {} need type QUEST_TYPE_PVP. Skip", questID);
                continue;
            }

            if (winArenaCount <= 0)
            {
                LOG_ERROR("module", "> QuestConditions: Incorrect win arena count ({}) for quest with id: {}. Skip", winArenaCount, questID);
                continue;
            }
        }

        if (completeAchievementID)
        {
            if (!sAchievementMgr->GetAchievement(completeAchievementID))
            {
                LOG_ERROR("module", "> QuestConditions: Not found achievement with id: {}. Skip", completeAchievementID);
                continue;
            }

            if (!GetKilledMonsterCredit(completeAchievementID, QuestConditionType::CompleteAchievement))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for achievement with id: {}. Skip", completeAchievementID);
                continue;
            }
        }

        if (completeQuestID)
        {
            if (!sObjectMgr->GetQuestTemplate(completeQuestID))
            {
                LOG_ERROR("module", "> QuestConditions: Not found quest complete with id: {}. Skip", completeQuestID);
                continue;
            }

            if (!GetKilledMonsterCredit(completeQuestID, QuestConditionType::CompleteQuest))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for quest with id: {}. Skip", completeQuestID);
                continue;
            }
        }

        if (equipItemID)
        {
            if (!sObjectMgr->GetItemTemplate(equipItemID))
            {
                LOG_ERROR("module", "> QuestConditions: Not found item with id: {}. Skip", equipItemID);
                continue;
            }

            if (!GetKilledMonsterCredit(equipItemID, QuestConditionType::EquipItem))
            {
                LOG_ERROR("module", "> QuestConditions: Not found KMC for item with id: {}. Skip", equipItemID);
                continue;
            }
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
bool QuestConditionsMgr::CanPlayerCompleteQuest(Player* player, Quest const* questInfo)
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

    UpdateQuestConditionForPlayer(player, QuestConditionType::CompleteQuest, quest->GetQuestId());

    if (!HasQuestCondition(quest->GetQuestId()))
        return;

    DeleteQuestConditionsHistory(player->GetGUID(), quest->GetQuestId());
}

void QuestConditionsMgr::OnPlayerAddQuest(Player* player, Quest const* quest)
{
    if (!_isEnable || !player || !quest)
        return;

    auto const& questCondtion = GetQuestCondition(quest->GetQuestId());
    if (!questCondtion)
        return;

    if (!questCondtion->CompleteQuestID && !questCondtion->CompleteAchievementID && !questCondtion->EquipItemID)
        return;

    auto playerQuestCondition = MakeQuestConditionForPlayer(player->GetGUID(), quest->GetQuestId());
    ASSERT(playerQuestCondition);

    if (questCondtion->CompleteQuestID && !playerQuestCondition->CompleteQuestID)
    {
        if (!IsQuestComplete(player, questCondtion->CompleteQuestID))
            return;

        auto kmc = GetKilledMonsterCredit(questCondtion->CompleteQuestID, QuestConditionType::CompleteQuest);
        ASSERT(kmc);
        player->KilledMonsterCredit(*kmc);
    }

    if (questCondtion->CompleteAchievementID && !playerQuestCondition->CompleteAchievementID)
    {
        if (!player->GetAchievementMgr()->HasAchieved(questCondtion->CompleteAchievementID))
            return;

        auto kmc = GetKilledMonsterCredit(questCondtion->CompleteAchievementID, QuestConditionType::CompleteAchievement);
        ASSERT(kmc);
        player->KilledMonsterCredit(*kmc);
    }

    if (questCondtion->EquipItemID && !playerQuestCondition->EquipItemID)
    {
        for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!item)
                continue;

            if (item->GetEntry() != questCondtion->EquipItemID)
                continue;

            auto kmc = GetKilledMonsterCredit(questCondtion->EquipItemID, QuestConditionType::EquipItem);
            ASSERT(kmc);
            player->KilledMonsterCredit(*kmc);
        }
    }
}

void QuestConditionsMgr::OnPlayerQuestAbandon(Player* player, uint32 questID)
{
    if (!HasQuestCondition(questID))
        return;

    DeleteQuestConditionsHistory(player->GetGUID(), questID);
}

void QuestConditionsMgr::OnPlayerSpellCast(Player* player, Spell* spell)
{
    if (!_isEnable || !player || !spell)
        return;

    UpdateQuestConditionForPlayer(player, QuestConditionType::UseSpell, spell->GetSpellInfo()->Id);
}

void QuestConditionsMgr::OnPlayerItemEquip(Player* player, Item* item)
{
    if (!_isEnable || !player || !item)
        return;

    UpdateQuestConditionForPlayer(player, QuestConditionType::EquipItem, item->GetEntry());
}

void QuestConditionsMgr::OnPlayerAchievementComplete(Player* player, AchievementEntry const* achievement)
{
    if (!_isEnable || !player || !achievement)
        return;

    UpdateQuestConditionForPlayer(player, QuestConditionType::CompleteAchievement, achievement->ID);
}

void QuestConditionsMgr::OnBattlegoundEnd(Battleground* bg, TeamId winnerTeam)
{
    if (!_isEnable || !bg)
        return;

    auto const& bgPlayers = bg->GetPlayers();
    if (bgPlayers.empty())
        return;

    auto realBGTypeID{ static_cast<uint32>(bg->GetBgTypeID()) };
    auto BGTypeID{ static_cast<uint32>(bg->GetBgTypeID(bg->IsRandom())) };
    auto isArena{ bg->isArena() };
    auto arenaType{ bg->GetArenaType() };

    for (auto const& [playerGuid, player] : bgPlayers)
    {
        if (player->GetBgTeamId() != winnerTeam)
            continue;

        if (isArena)
            UpdateQuestConditionForPlayer(player, QuestConditionType::WinArena, arenaType);
        else
        {
            UpdateQuestConditionForPlayer(player, QuestConditionType::WinBG, realBGTypeID);

            if (bg->IsRandom())
                UpdateQuestConditionForPlayer(player, QuestConditionType::WinBG, BGTypeID);
        }
    }
}

bool QuestConditionsMgr::HasQuestCondition(uint32 questID)
{
    return _conditions.contains(questID);
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

void QuestConditionsMgr::DeleteQuestConditionsHistory(ObjectGuid playerGuid, uint32 questID)
{
    auto const& playerQuestConditionsStore = GetPlayerConditions(playerGuid);
    if (!playerQuestConditionsStore)
        return;

    playerQuestConditionsStore->erase(questID);

    // Delete from DB
    CharacterDatabase.Execute("DELETE FROM `wh_characters_quest_conditions` WHERE `PlayerGuid` = {} AND `QuestID` = {}", playerGuid.GetRawValue(), questID);
}

uint32 const* QuestConditionsMgr::GetKilledMonsterCredit(uint32 value, QuestConditionType type)
{
    auto staticType = static_cast<uint8>(type);

    if (type >= QuestConditionType::Max)
    {
        LOG_ERROR("module", "> QuestConditionsMgr: Incorrect type for get KMC. Value: {}. Type: {}", value, staticType);
        return nullptr;
    }

    return Warhead::Containers::MapGetValuePtr(_kmc, MAKE_PAIR64(value, staticType));
}

void QuestConditionsMgr::UpdateQuestConditionForPlayer(Player* player, QuestConditionType type, uint32 value)
{
    for (auto const& [questID_, condition] : _conditions)
    {
        // Skip check if quest not found
        if (player->GetQuestStatus(questID_) != QUEST_STATUS_INCOMPLETE)
            continue;

        if (!condition.IsFoundConditionForType(type) || !condition.IsValidConditionForType(type, value))
            continue;

        auto playerQuestCondition = MakeQuestConditionForPlayer(player->GetGUID(), questID_);
        ASSERT(playerQuestCondition);

        switch (type)
        {
            case QuestConditionType::UseSpell:
            {
                if (playerQuestCondition->UseSpellCount >= condition.UseSpellCount)
                    continue;

                playerQuestCondition->UseSpellCount = std::min(condition.UseSpellCount, playerQuestCondition->UseSpellCount + 1);
                break;
            }
            case QuestConditionType::WinBG:
            {
                if (playerQuestCondition->WinBGCount >= condition.WinBGCount)
                    continue;

                playerQuestCondition->WinBGCount = std::min(condition.WinBGCount, playerQuestCondition->WinBGCount + 1);
                break;
            }
            case QuestConditionType::WinArena:
            {
                if (playerQuestCondition->WinArenaCount >= condition.WinArenaCount)
                    continue;

                playerQuestCondition->WinArenaCount = std::min(condition.WinArenaCount, playerQuestCondition->WinArenaCount + 1);
                break;
            }
            case QuestConditionType::CompleteAchievement:
            {
                if (playerQuestCondition->CompleteAchievementID)
                    continue;

                playerQuestCondition->CompleteAchievementID = value;
                break;
            }
            case QuestConditionType::CompleteQuest:
            {
                if (playerQuestCondition->CompleteQuestID)
                    continue;

                playerQuestCondition->CompleteQuestID = value;
                break;
            }
            case QuestConditionType::EquipItem:
            {
                if (playerQuestCondition->EquipItemID)
                    continue;

                playerQuestCondition->EquipItemID = value;
                break;
            }
            default:
                ABORT("> Incorrect QuestConditionType: {}", static_cast<uint8>(type));
                break;
        }

        SavePlayerConditionToDB(player->GetGUID(), questID_);

        auto kmc = GetKilledMonsterCredit(value, type);
        ASSERT(kmc);
        player->KilledMonsterCredit(*kmc);
    }
}

bool QuestConditionsMgr::IsQuestComplete(Player* player, uint32 questID)
{
    if (!player)
        return false;

    auto const& quest = sObjectMgr->GetQuestTemplate(questID);
    if (!quest)
        return false;

    if (!quest->IsRepeatable())
        return player->GetQuestStatus(questID) == QUEST_STATUS_REWARDED;

    if (quest->IsDaily())
        return player->IsDailyQuestDone(questID);

    if (quest->IsWeekly())
        return player->IsWeeklyQuestDone(questID);

    if (quest->IsMonthly())
        return player->IsMonthlyQuestDone(questID);

    return false;
}

void QuestConditionsMgr::SendQuestConditionInfo(ChatHandler* handler, uint32 questID)
{
    auto const& questCondtion = GetQuestCondition(questID);
    if (!questCondtion)
    {
        handler->PSendSysMessage("> Дополнительных условий для квеста {} не найдено", questID);
        return;
    }

    auto const& quest = sObjectMgr->GetQuestTemplate(questID);
    if (!quest)
    {
        handler->PSendSysMessage("> Квест {} не найден", questID);
        return;
    }

    std::string questTitle{ quest->GetTitle() };

    int localeConstant = handler->GetSessionDbLocaleIndex();

    if (QuestLocale const* locale = sGameLocale->GetQuestLocale(questID))
    {
        std::string_view name = GameLocale::GetLocaleString(locale->Title, localeConstant);
        if (!name.empty())
            questTitle = name;
    }

    handler->PSendSysMessage("###############");
    handler->PSendSysMessage("> Квест {} ({})", questTitle, questID);
    handler->PSendSysMessage("###############");

    if (questCondtion->UseSpellID)
    {
        std::string spellName = sGameLocale->GetSpellLink(questCondtion->UseSpellID, localeConstant);
        handler->PSendSysMessage("> Использовать заклинение: {}. {} раз", spellName, questCondtion->UseSpellCount);
    }

    if (questCondtion->WinBGType)
    {
        auto bg = sBattlegroundMgr->GetBattlegroundTemplate(static_cast<BattlegroundTypeId>(questCondtion->WinBGType));
        handler->PSendSysMessage("> Выйграть бг: {}. {} раз", bg->GetName(), questCondtion->WinBGCount);
    }

    if (questCondtion->WinArenaType)
        handler->PSendSysMessage("> Выйграть арену: {}vs{}. {} раз", questCondtion->WinArenaType, questCondtion->WinArenaType, questCondtion->WinArenaCount);

    if (questCondtion->CompleteAchievementID)
    {
        auto achievement = sAchievementMgr->GetAchievement(questCondtion->CompleteAchievementID);
        handler->PSendSysMessage("> Получить достижение: {}", achievement->name[localeConstant]);
    }

    if (questCondtion->CompleteQuestID)
    {
        auto const& completeQuest = sObjectMgr->GetQuestTemplate(questCondtion->CompleteQuestID);
        std::string completeQuestTitle{ completeQuest->GetTitle() };

        if (QuestLocale const* locale = sGameLocale->GetQuestLocale(questCondtion->CompleteQuestID))
        {
            std::string_view name = GameLocale::GetLocaleString(locale->Title, localeConstant);
            if (!name.empty())
                completeQuestTitle = name;
        }

        handler->PSendSysMessage("> Завершить квест: {} - |cffffffff|Hquest:{}:{}|h[{}]|h|r", completeQuest->GetQuestId(), completeQuest->GetQuestLevel(), questTitle);
    }

    if (questCondtion->EquipItemID)
        handler->PSendSysMessage("> Одеть предмет: {}", sGameLocale->GetItemLink(questCondtion->EquipItemID, localeConstant));

    handler->PSendSysMessage("###############");
}

void QuestConditionsMgr::OnPlayerLogin(Player* player)
{
    if (!_isEnable)
        return;

    player->GetSession()->GetQueryProcessor().AddCallback(
        CharacterDatabase.AsyncQuery(Warhead::StringFormat("SELECT `QuestID`, `UseSpellCount`, `WinBGCount`, `WinArenaCount`, "
            "`CompleteAchievementID`, `CompleteQuestID`, `EquipItemID` FROM `wh_characters_quest_conditions` WHERE `PlayerGuid` = {}", player->GetGUID().GetRawValue())).
        WithCallback([this, playerGuid = player->GetGUID()](QueryResult result)
        {
            if (!result)
                return;

            std::lock_guard<std::mutex> guard(_playerConditionsMutex);
            _playerConditions.erase(playerGuid);

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

    std::lock_guard<std::mutex> guard(_playerConditionsMutex);
    _playerConditions.erase(player->GetGUID());
}
