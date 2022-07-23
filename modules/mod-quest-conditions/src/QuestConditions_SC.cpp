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

#include "ScriptObject.h"
#include "QuestConditions.h"

class QuestConditions_BG : public BGScript
{
public:
    QuestConditions_BG() : BGScript("QuestConditions_BG") { }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeam) override
    {
        sQuestConditionsMgr->OnBattlegoundEnd(bg, winnerTeam);
    }
};

class QuestConditions_Player : public PlayerScript
{
public:
    QuestConditions_Player() : PlayerScript("QuestConditions_Player") { }

    void OnLogin(Player* player) override
    {
        sQuestConditionsMgr->OnPlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        sQuestConditionsMgr->OnPlayerLogout(player);
    }

    bool CanCompleteQuest(Player* player, Quest const* questInfo, QuestStatusData const* /*questStatusData*/) override
    {
        return sQuestConditionsMgr->CanPlayerCompleteQuest(player, questInfo);
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        sQuestConditionsMgr->OnPlayerCompleteQuest(player, quest);
    }

    void OnAddQuest(Player* player, Quest const* quest, Object* /*questGiver*/) override
    {
        sQuestConditionsMgr->OnPlayerAddQuest(player, quest);
    }

    void OnQuestAbandon(Player* player, uint32 questId) override
    {
        sQuestConditionsMgr->OnPlayerQuestAbandon(player, questId);
    }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        sQuestConditionsMgr->OnPlayerSpellCast(player, spell);
    }

    void OnEquip(Player* player, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        sQuestConditionsMgr->OnPlayerItemEquip(player, item);
    }

    void OnAchiComplete(Player* player, AchievementEntry const* achievement) override
    {
        sQuestConditionsMgr->OnPlayerAchievementComplete(player, achievement);
    }
};

class QuestConditions_World : public WorldScript
{
public:
    QuestConditions_World() : WorldScript("QuestConditions_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sQuestConditionsMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sQuestConditionsMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_QuestConditions()
{
    new QuestConditions_BG();
    new QuestConditions_Player();
    new QuestConditions_World();
}
