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

#include "ScriptMgr.h"
#include "Log.h"
#include "OnlineReward.h"
#include "ModulesConfig.h"
#include "Player.h"
#include <map>

class OnlineReward_Player : public PlayerScript
{
public:
    OnlineReward_Player() : PlayerScript("OnlineReward_Player") { }

    void OnLogin(Player* player) override
    {
        sOL->AddRewardHistory(player->GetGUID().GetCounter());
    }

    void OnLogout(Player* player) override
    {
        sOL->DeleteRewardHistory(player->GetGUID().GetCounter());
    }
};

class OnlineReward_World : public WorldScript
{
public:
    OnlineReward_World() : WorldScript("OnlineReward_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sModulesConfig->AddOption<bool>("OnlineReward.Enable");
        sModulesConfig->AddOption<bool>("OnlineReward.PerOnline.Enable");
        sModulesConfig->AddOption<bool>("OnlineReward.PerTime.Enable");
        sModulesConfig->AddOption<int32>("OnlineReward.PerTime.Time", 3600);
        sModulesConfig->AddOption<int32>("OnlineReward.PerTime.ItemID", 47241);
        sModulesConfig->AddOption<int32>("OnlineReward.PerTime.ItemCount", 1);
    }

    void OnStartup() override
    {
        sOL->InitSystem();
    }

    void OnUpdate(uint32 diff) override
    {
        sOL->Update(diff);
    }
};

// Group all custom scripts
void AddSC_OnlineReward()
{
    new OnlineReward_Player();
    new OnlineReward_World();
}
