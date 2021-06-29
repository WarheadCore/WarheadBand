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

#include "LowLevelArena.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptedGossip.h"

#if WARHEAD_COMPILER == WARHEAD_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

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

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> llaQueueCommandTable =
        {
            { "solo",   SEC_PLAYER, false,  &HandleLLAQueueSolo, "" },
            { "group",  SEC_PLAYER, false,  &HandleLLAQueueGroup, "" }
        };

        static std::vector<ChatCommand> llaCommandTable =
        {
            { "queue",  SEC_PLAYER, false, nullptr, "", llaQueueCommandTable }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "lla",    SEC_PLAYER, false, nullptr, "", llaCommandTable }
        };

        return commandTable;
    }

    static bool AddQueue(Player* player, std::string_view arenaType, bool joinAsGroup)
    {
        ChatHandler handler(player->GetSession());

        if (!sConfigMgr->GetOption<bool>("LLA.Enable", false))
        {
            handler.PSendSysMessage("> Module disable!");
            return false;
        }

        if (arenaType.empty())
        {
            handler.PSendSysMessage("> Arena type '{}' is incorrect. Please use 2v2, 3v3, 5v5", arenaType);
            return false;
        }

        // Try join queue
        if (arenaType == "2v2")
        {
            // Join 2v2
            sLLA->AddQueue(player, ARENA_TYPE_2v2, joinAsGroup);
        }
        else if (arenaType == "3v3")
        {
            // Join 3v3
            sLLA->AddQueue(player, ARENA_TYPE_3v3, joinAsGroup);
        }
        else if (arenaType == "5v5")
        {
            // Join 5v5
            sLLA->AddQueue(player, ARENA_TYPE_5v5, joinAsGroup);
        }
        else
        {
            handler.PSendSysMessage("> Arena type '{}' is incorrect. Please use 2v2, 3v3, 5v5", arenaType);
            return false;
        }

        return true;
    }

    static bool HandleLLAQueueSolo(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!AddQueue(player, args, false))
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandleLLAQueueGroup(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!AddQueue(player, args, true))
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
};

class LowLevelArena_World : public WorldScript
{
public:
    LowLevelArena_World() : WorldScript("LowLevelArena_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        // Add conigs options configiration
    }
};

// Group all custom scripts
void AddSC_LowLevelArena()
{
    //new LowLevelArena_BG();
    new LowLevelArena_Command();
    new LowLevelArena_World();
}
