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

#include "QuestBuff.h"
#include "ScriptObject.h"

class QuestBuff_Player : public PlayerScript
{
public:
    QuestBuff_Player() : PlayerScript("QuestBuff_Player") { }

    void OnLogin(Player* player) override
    {
        sQuestBuffMgr->OnPlayerLogin(player);
    }
};

class QuestBuff_World : public WorldScript
{
public:
    QuestBuff_World() : WorldScript("QuestBuff_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sQuestBuffMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sQuestBuffMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_QuestBuff()
{
    new QuestBuff_Player();
    new QuestBuff_World();
}