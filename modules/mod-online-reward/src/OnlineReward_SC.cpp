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

#include "Log.h"
#include "ModulesConfig.h"
#include "OnlineReward.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <map>

class OnlineReward_Player : public PlayerScript
{
public:
    OnlineReward_Player() : PlayerScript("OnlineReward_Player") { }

    void OnLogin(Player* player) override
    {
        sOLMgr->AddRewardHistory(player->GetGUID().GetCounter());
    }

    void OnLogout(Player* player) override
    {
        sOLMgr->DeleteRewardHistory(player->GetGUID().GetCounter());
    }
};

class OnlineReward_World : public WorldScript
{
public:
    OnlineReward_World() : WorldScript("OnlineReward_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sOLMgr->LoadConfig();
    }

    void OnStartup() override
    {
        sOLMgr->InitSystem();
    }

    void OnUpdate(uint32 diff) override
    {
        sOLMgr->Update(Milliseconds(diff));
    }
};

// Group all custom scripts
void AddSC_OnlineReward()
{
    new OnlineReward_Player();
    new OnlineReward_World();
}
