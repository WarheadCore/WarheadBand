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

#include "BgOrArenaReward.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "ModulesConfig.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptedGossip.h"

using namespace Warhead::ChatCommands;

class BgOrArenaReward_CS : public CommandScript
{
public:
    BgOrArenaReward_CS() : CommandScript("BgOrArenaReward_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable boarCommandTable =
        {
            { "reload", HandleBOARReloadCommand, SEC_ADMINISTRATOR,  Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "boar", boarCommandTable }
        };

        return commandTable;
    }

    static bool HandleBOARReloadCommand(ChatHandler* handler)
    {
        if (!sBOARMgr->IsEnable())
        {
            handler->PSendSysMessage("> Module disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }

        sBOARMgr->LoadDBData();
        handler->PSendSysMessage("> BOAR data reloaded");
        return true;
    }
};

class BgOrArenaReward_Player : public BGScript
{
public:
    BgOrArenaReward_Player() : BGScript("BgOrArenaReward_Player") { }

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId winnerTeamId) override
    {
        sBOARMgr->RewardPlayer(player, bg, winnerTeamId);
    }
};

class BgOrArenaReward_World : public WorldScript
{
public:
    BgOrArenaReward_World() : WorldScript("BgOrArenaReward_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sModulesConfig->AddOption<bool>("BOAR.Enable");
        sBOARMgr->LoadConig();
    }

    void OnStartup() override
    {
        sBOARMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_BgOrArenaReward()
{
    new BgOrArenaReward_CS();
    new BgOrArenaReward_Player();
    new BgOrArenaReward_World();
}
