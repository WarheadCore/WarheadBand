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

#include "ArenaReward.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "ModulesConfig.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptedGossip.h"

class ArenaReward_BG : public BGScript
{
public:
    ArenaReward_BG() : BGScript("ArenaReward_BG") { }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId) override
    {
        if (!MOD_CONF_GET_BOOL("ArenaReward.Enable"))
            return;

        // Not reward on end bg
        if (bg->isBattleground())
            return;

        sAR->SendRewardArena(bg, winnerTeamId);
    }
};

class ArenaReward_World : public WorldScript
{
public:
    ArenaReward_World() : WorldScript("ArenaReward_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sModulesConfig->AddOption<bool>("ArenaReward.Enable");

        sModulesConfig->AddOption<bool>("ArenaReward.Reward.Rating.Enable");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Rating.ItemID");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Rating.ItemCount.WinnerTeam");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Rating.ItemCount.LoserTeam");

        sModulesConfig->AddOption<bool>("ArenaReward.Reward.Skirmish.Enable");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Skirmish.ItemID");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Skirmish.ItemCount.WinnerTeam");
        sModulesConfig->AddOption<int32>("ArenaReward.Reward.Skirmish.ItemCount.LoserTeam");

        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Enable");
        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Check.IP.Enable");
        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Check.Equipment.Enable");
        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Check.Health.Enable");

        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.SpellApply.Enable");

        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Teleport.Enable");
        sModulesConfig->AddOption<std::string>("ArenaReward.AntiFarm.Teleport.Location");

        sModulesConfig->AddOption<bool>("ArenaReward.AntiFarm.Ban.Enable");
        sModulesConfig->AddOption<std::string>("ArenaReward.AntiFarm.Ban.Duration");
    }
};

// Group all custom scripts
void AddSC_ArenaReward()
{
    new ArenaReward_BG();
    new ArenaReward_World();
}
