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

#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Optional.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <tuple>
#include <vector>

using namespace Warhead::ChatCommands;

class queue_commandscript : public CommandScript
{
public:
    queue_commandscript() : CommandScript("queue_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable queueShowArenaCommandTable = // .queue show arena
        {
            { "rated",  HandleQueueShowArenaRatedCommand,   SEC_PLAYER, Console::Yes },
            { "normal", HandleQueueShowArenaNormalCommand,  SEC_PLAYER, Console::Yes }
        };

        static ChatCommandTable queueShowCommandTable = // .queue show
        {
            { "bg",     HandleQueueShowBgCommand,           SEC_PLAYER, Console::Yes },
            { "arena", queueShowArenaCommandTable }
        };

        static ChatCommandTable queueCommandTable = // .queue
        {
            { "show", queueShowCommandTable }
        };

        static ChatCommandTable commandTable =
        {
            { "queue", queueCommandTable }
        };

        return commandTable;
    }

    static bool HandleQueueShowArenaRatedCommand(ChatHandler* handler)
    {
        using QueueTemplate = std::tuple<
            char const*, // team name
            uint8, // arena type
            uint32>; // team rating

        // Core queue
        std::vector<QueueTemplate> queueList;

        // Modules queue
        //

        // Rated arena
        for (uint32 qtype = BATTLEGROUND_QUEUE_2v2; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        {
            BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

            for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
            {
                BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

                // Skip if all empty
                if (bgQueue.IsAllQueuesEmpty(bracketID))
                    continue;

                for (uint8 ii = BG_QUEUE_PREMADE_ALLIANCE; ii <= BG_QUEUE_PREMADE_HORDE; ii++)
                {
                    for (auto const& itr : bgQueue.m_QueuedGroups[bracketID][ii])
                    {
                        if (!itr->IsRated)
                            continue;

                        ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(itr->ArenaTeamId);
                        if (!team)
                            continue;

                        queueList.emplace_back(team->GetName().c_str(), itr->ArenaType, itr->ArenaTeamRating);
                    }
                }
            }
        }

        if (queueList.empty())
        {
            handler->SendSysMessage("> All queues empty");
            return true;
        }
        else
        {
            handler->SendSysMessage("# Queue status for rated arena:");

            for (auto const& [teamName, arenaType, teamRating] : queueList)
            {
                // Queue status
                handler->PSendSysMessage("> %s (%uv%u): %u", teamName, arenaType, arenaType, teamRating);
            }
        }

        return true;
    }

    static bool HandleQueueShowArenaNormalCommand(ChatHandler* handler)
    {
        using QueueTemplate = std::tuple<
            std::string, // arena type
            uint32, // level min
            uint32, // level max
            uint32, // queued total
            uint32>; // players need

        using QueueList = std::vector<QueueTemplate>;

        // Core queue
        QueueList queueListNormal;

        // Modules queue
        //

        auto GetTemplate = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID) -> Optional<QueueTemplate>
        {
            Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgQueue.GetBGTypeID());
            if (!bg || bg->isBattleground())
                return std::nullopt;

            PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracketID);
            if (!bracketEntry)
                return std::nullopt;

            uint8 arenaType = bgQueue.GetArenaType();

            if (!arenaType)
                return std::nullopt;

            auto arenatype = Warhead::StringFormat("%uv%u", arenaType, arenaType);
            uint32 playersNeed = ArenaTeam::GetReqPlayersForType(arenaType);
            uint32 minLevel = std::min(bracketEntry->minLevel, (uint32)80);
            uint32 maxLevel = std::min(bracketEntry->maxLevel, (uint32)80);
            uint32 qPlayers = bgQueue.GetPlayersCountInGroupsQueue(bracketID, BG_QUEUE_NORMAL_HORDE) + bgQueue.GetPlayersCountInGroupsQueue(bracketID, BG_QUEUE_NORMAL_ALLIANCE);

            if (!qPlayers)
                return std::nullopt;

            return std::make_tuple(arenatype, minLevel, maxLevel, qPlayers, playersNeed);
        };

        // Non rated arena
        for (uint32 qtype = BATTLEGROUND_QUEUE_2v2; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        {
            BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

            for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
            {
                BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

                // Skip if all empty
                if (bgQueue.IsAllQueuesEmpty(bracketID))
                    continue;

                auto _queueList = GetTemplate(bgQueue, bracketID);
                if (_queueList)
                    queueListNormal.emplace_back(*_queueList);
            }
        }

        if (queueListNormal.empty())
        {
            handler->SendSysMessage("> All queues empty");
        }
        else
        {
            handler->SendSysMessage("# Queue status for non rated arena:");

            for (auto const& [arenaType, minLevel, maxLevel, qTotal, MaxPlayers] : queueListNormal)
            {
                // Queue status
                handler->PSendSysMessage("> %u-%u %s: %u/%u", minLevel, maxLevel, arenaType.c_str(), qTotal, MaxPlayers);
            }
        }

        return true;
    }

    static bool HandleQueueShowBgCommand(ChatHandler* handler)
    {
        using QueueTemplate = std::tuple<
            char const*, // bg name
            uint32, // level min
            uint32, // level max
            uint32, // queued total
            uint32>; // players need

        using QueueList = std::vector<QueueTemplate>;

        // Core queue
        QueueList queueListNormal;
        QueueList queueListPremade;

        // Modules queue
        QueueList queueListCFBG; // mod-cfbg

        auto GetTemplate = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID,
            BattlegroundQueueGroupTypes queueHorde, BattlegroundQueueGroupTypes queueAlliance) -> Optional<QueueTemplate>
        {
            Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgQueue.GetBGTypeID());
            if (!bg || bg->isArena())
                return std::nullopt;

            PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracketID);
            if (!bracketEntry)
                return std::nullopt;

            char const* bgName = bg->GetName();
            uint32 MinPlayers = bg->GetMinPlayersPerTeam();
            uint32 MaxPlayers = MinPlayers * 2;
            uint32 q_min_level = std::min(bracketEntry->minLevel, (uint32)80);
            uint32 q_max_level = std::min(bracketEntry->maxLevel, (uint32)80);
            uint32 qHorde = bgQueue.GetPlayersCountInGroupsQueue(bracketID, queueHorde);
            uint32 qAlliance = bgQueue.GetPlayersCountInGroupsQueue(bracketID, queueAlliance);
            auto qTotal = qHorde + qAlliance;

            if (!qTotal)
                return std::nullopt;

            return std::make_tuple(bgName, q_min_level, q_max_level, qTotal, MaxPlayers);
        };

        auto GetTemplateOtherQueue = [&](BattlegroundQueue& bgQueue, BattlegroundBracketId bracketID, BattlegroundQueueGroupTypes queueType)
        {
            return GetTemplate(bgQueue, bracketID, queueType, queueType);
        };

        for (uint32 qtype = BATTLEGROUND_QUEUE_AV; qtype < MAX_BATTLEGROUND_QUEUE_TYPES; ++qtype)
        {
            BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BattlegroundQueueTypeId(qtype));

            for (uint32 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
            {
                BattlegroundBracketId bracketID = BattlegroundBracketId(bracket);

                // Skip if all empty
                if (bgQueue.IsAllQueuesEmpty(bracketID))
                    continue;

                Optional<QueueTemplate> _queueListNormal = GetTemplate(bgQueue, bracketID, BG_QUEUE_NORMAL_HORDE, BG_QUEUE_NORMAL_ALLIANCE);
                Optional<QueueTemplate> _queueListPremade = GetTemplate(bgQueue, bracketID, BG_QUEUE_PREMADE_HORDE, BG_QUEUE_PREMADE_ALLIANCE);
                Optional<QueueTemplate> _queueListCFBG = GetTemplateOtherQueue(bgQueue, bracketID, BattlegroundQueueGroupTypes(4));

                if (_queueListNormal)
                    queueListNormal.emplace_back(*_queueListNormal);

                if (_queueListPremade)
                    queueListPremade.emplace_back(*_queueListPremade);

                if (_queueListCFBG)
                    queueListCFBG.emplace_back(*_queueListCFBG);
            }
        }

        if (queueListNormal.empty() && queueListPremade.empty() && queueListCFBG.empty())
        {
            handler->SendSysMessage("> All queues empty");
            return true;
        }

        auto ShowQueueStatus = [&](QueueList& listQueue)
        {
            if (listQueue.empty())
                return;

            for (auto const& [bgName, minLevel, maxLevel, qTotal, MaxPlayers] : listQueue)
            {
                // Queue status
                handler->PSendSysMessage("> %u-%u %s: %u/%u", minLevel, maxLevel, bgName, qTotal, MaxPlayers);
            }
        };

        // If mod-cfbg enale, show only cfbg queue
        if (!queueListCFBG.empty())
        {
            handler->SendSysMessage("# Queue status for cross faction battleground:");
            ShowQueueStatus(queueListCFBG);
        }
        else if (!queueListNormal.empty() || !queueListPremade.empty())
        {
            handler->SendSysMessage("# Queue status for battleground:");
            ShowQueueStatus(queueListNormal);
            ShowQueueStatus(queueListPremade);
        }

        return true;
    }
};

void AddSC_queue_commandscript()
{
    new queue_commandscript();
}
