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

#include "Player.h"
#include "ScriptObject.h"
#include "GuildLevelMgr.h"

enum ApprenticeAnglerQuestEnum
{
    QUEST_APPRENTICE_ANGLER = 8194
};

class QuestApprenticeAnglerPlayerScript : public PlayerScript
{
public:
    QuestApprenticeAnglerPlayerScript() : PlayerScript("QuestApprenticeAnglerPlayerScript")
    {
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_APPRENTICE_ANGLER)
        {
            uint32 level = player->getLevel();
            int32 moneyRew = 0;
            if (level <= 10)
                moneyRew = 85;
            else if (level <= 60)
                moneyRew = 2300;
            else if (level <= 69)
                moneyRew = 9000;
            else if (level <= 70)
                moneyRew = 11200;
            else if (level <= 79)
                moneyRew = 12000;
            else
                moneyRew = 19000;

            player->ModifyMoney(moneyRew);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD, uint32(moneyRew));
            player->SaveToDB(false, false);

            // Send packet with money
            WorldPacket data(SMSG_QUESTGIVER_QUEST_COMPLETE, (4 + 4 + 4 + 4 + 4));
            data << uint32(quest->GetQuestId());
            data << uint32(0);
            data << uint32(moneyRew);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            player->SendDirectMessage(&data);
        }
    }
};

class GuildLevelPlayerScript : public PlayerScript
{
public:
    GuildLevelPlayerScript() : PlayerScript("GuildLevelPlayerScript") { }

    void OnLogin(Player* player) override
    {
        if (player) {
            sGuildLevelMgr->LearnOrRemoveSpell(player);     
        }
    }

    void OnCreatureKill(Player* player, Creature* boss) override {
        if (!player || !boss->isWorldBoss())
            return;

        sGuildLevelMgr->RewardOnKillBoss(player);            
    }    

    void OnGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {    
        if (!player)
            return;

        // 99 - это номер меню, которое мы создали в OnGossipHello
        if (menu_id != 99)
            return;

        ClearGossipMenuFor(player);
        switch (sender) {
            // Глобальные функции
            case GOSSIP_SENDER_MAIN: {
                switch (action) {
                    case 0: { // Старт меню гильдии
                        sGuildLevelMgr->GuildLevelWindow(player);
                    } break;
                    case 1: { // Вложение в гильдию
                        sGuildLevelMgr->GuildLevelInvestment(player);
                    } break;
                    case 2: { // История вложения всех членов гильдии
                        sGuildLevelMgr->GuildLevelInvestmentHistory(player, true);
                    } break;
                    case 3: { // Покупка Дома
                        sGuildLevelMgr->GuildLevelBuyHouse(player);
                    } break;
                    case 4: { // Доступные заклинания по уровню
                        sGuildLevelMgr->GuildLevelSpell(player);
                    } break;
                    case 5: { // История вложения одного члена гильдии
                        sGuildLevelMgr->GuildLevelInvestmentHistory(player, false);
                    } break;
                    default: break;
                }
            } break;
            default: break;
        }
    }
};

void AddSC_player_scripts()
{
    new QuestApprenticeAnglerPlayerScript();
    new GuildLevelPlayerScript();
}
