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

#ifndef _QUEST_CONDITIONS_H_
#define _QUEST_CONDITIONS_H_

#include "ObjectGuid.h"
#include <unordered_map>

class Player;
class Quest;
struct QuestStatusData;

enum class QuestConditionType : uint8
{
    UseSpell,
    WinBG,
    WinArena,
    CompleteAchievement,
    CompleteQuest,
    EquipItem,

    Max
};

constexpr uint8 MAX_QUEST_CONDITION_TYPE = static_cast<uint8>(QuestConditionType::Max);

struct QuestCondition
{
    uint32 QuestID{ 0 };
    uint32 UseSpellID{ 0 };
    uint32 UseSpellCount{ 0 };
    uint32 WinBGType{ 0 };
    uint32 WinBGCount{ 0 };
    uint32 WinArenaType{ 0 };
    uint32 WinArenaCount{ 0 };
    uint32 CompleteAchievementID{ 0 };
    uint32 CompleteQuestID{ 0 };
    uint32 EquipItemID{ 0 };
};

using QuestConditions = std::unordered_map<uint32, QuestCondition>;

class QuestConditionsMgr
{
public:
    static QuestConditionsMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);

    // Hooks
    bool CanCompleteQuest(Player* player, Quest const* questInfo);
    void OnPlayerCompleteQuest(Player* player, Quest const* quest);
    void OnPlayerAddQuest(Player* player, Quest const* quest);

    // Add/Delete QC data
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);

private:
    void LoadQuestConditionsKilledMonsterCredit();
    void LoadQuestConditions();

    QuestCondition const* GetQuestCondition(uint32 questID);
    QuestConditions* GetPlayerConditions(ObjectGuid playerGuid);
    QuestCondition* MakeQuestConditionForPlayer(ObjectGuid playerGuid, uint32 questID);
    void SavePlayerConditionToDB(ObjectGuid playerGuid, uint32 questID);

    uint32 const* GetKilledMonsterCredit(uint32 value, QuestConditionType type);

    void UpdateQuestConditionForPlayer(Player* player, uint32 questID, uint32 completedQuestID);

    bool _isEnable{ false };
    QuestConditions _conditions;
    std::unordered_map<ObjectGuid, QuestConditions> _playerConditions;
    std::unordered_map<uint64, uint32> _kmc;
};

#define sQuestConditionsMgr QuestConditionsMgr::instance()

#endif
