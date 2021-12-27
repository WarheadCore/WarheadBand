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

#include "Chat.h"
#include "Config.h"
#include "Log.h"
#include "LowLevelArena.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

using namespace Warhead::ChatCommands;

//class LowLevelArena_BG : public BGScript
//{
//public:
//    LowLevelArena_BG() : BGScript("LowLevelArena_BG") { }
//
//    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId) override
//    {
//        if (!sConfigMgr->GetOption<bool>("LLA.Enable", false))
//            return;
//
//        // Not reward for bg or rated arena
//        if (bg->isBattleground() || bg->isRated())
//        {
//            return;
//        }
//
//        sLLA->Reward(bg, winnerTeamId);
//    }
//};

class LowLevelArena_Command : public CommandScript
{
public:
    LowLevelArena_Command() : CommandScript("LowLevelArena_Command") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable llaCommandTable =
        {
            { "queue",  HandleLLAQueue, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "lla", llaCommandTable }
        };

        return commandTable;
    }

    static bool AddQueue(Player* player)
    {
        ChatHandler handler(player->GetSession());

        if (!sConfigMgr->GetOption<bool>("LLA.Enable", false))
        {
            handler.PSendSysMessage("> Module disable!");
            return true;
        }

        // Join arena queue
        sLLA->AddQueue(player);

        return true;
    }

    static bool HandleLLAQueue(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!AddQueue(player))
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
};

//class LowLevelArena_World : public WorldScript
//{
//public:
//    LowLevelArena_World() : WorldScript("LowLevelArena_World") { }
//
//    void OnAfterConfigLoad(bool /*reload*/) override
//    {
//        // Add conigs options configiration
//    }
//};

// Group all custom scripts
void AddSC_LowLevelArena()
{
    //new LowLevelArena_BG();
    new LowLevelArena_Command();
    //new LowLevelArena_World();
}
