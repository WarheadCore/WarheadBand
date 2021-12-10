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

#include "LowLevelArena.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "Config.h"
#include "DisableMgr.h"
#include "GroupMgr.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include <unordered_set>
//#include <unordered_map>
//#include <tuple>

//namespace
//{
//    using LLAReward = std::tuple<uint32 /*min level*/, uint32 /*max level*/, bool /*enable*/, uint32 /*count lose*/, uint32 /*count winner*/>;
//    std::unordered_map<uint32 /*min level*/, LLAReward> _arenaRewards;
//}

LLA* LLA::instance()
{
    static LLA instance;
    return &instance;
}

void LLA::AddQueue(Player* leader)
{
    if (!leader)
    {
        return;
    }

    // Default 5v5
    uint8 arenaSlot = 2;
    uint8 arenaType = ARENA_TYPE_5v5;

    Group* group = leader->GetGroup();
    if (!group || group->GetMembersCount() < 3)
    {
        arenaType = ARENA_TYPE_2v2;
        arenaSlot = 0;
    }
    else if (group->GetMembersCount() == 3)
    {
        arenaType = ARENA_TYPE_3v3;
        arenaSlot = 1;
    }

    // get template for all arenas
    Battleground* bgt = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bgt)
        return;

    // arenas disabled
    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, nullptr))
    {
        ChatHandler(leader->GetSession()).PSendSysMessage(LANG_ARENA_DISABLED);
        return;
    }

    BattlegroundTypeId bgTypeId = bgt->GetBgTypeID();
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, arenaType);

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgt->GetMapId(), leader->getLevel());
    if (!bracketEntry)
        return;

    // must have free queue slot
    // pussywizard: allow being queued only in one arena queue, and it even cannot be together with bg queues
    if (leader->InBattlegroundQueue())
    {
        WorldPacket data;
        sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, ERR_BATTLEGROUND_CANNOT_QUEUE_FOR_RATED);
        leader->GetSession()->SendPacket(&data);
        return;
    }

    // queue result (default ok)
    GroupJoinBattlegroundResult err = GroupJoinBattlegroundResult(bgt->GetBgTypeID());

    // id 29533
    /*if (!sScriptMgr->CanJoinInArenaQueue(leader, guid, arenaslot, bgTypeId, asGroup, isRated, err) && err <= 0)
    {
        WorldPacket data;
        sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, err);
        SendPacket(&data);
        return;
    }*/

    auto CheckPlayerEnterQueue = [&](Player* member)
    {
        if (member->InBattleground()) // currently in battleground
            err = ERR_BATTLEGROUND_NOT_IN_BATTLEGROUND;
        else if (member->isUsingLfg()) // using lfg system
            err = ERR_LFG_CANT_USE_BATTLEGROUND;
        else if (member->InBattlegroundQueue())
            err = ERR_BATTLEGROUND_TOO_MANY_QUEUES;
        // don't let Death Knights join BG queues when they are not allowed to be teleported yet
        else if (leader->getClass() == CLASS_DEATH_KNIGHT && member->GetMapId() == 609 && !member->IsGameMaster() && !member->HasSpell(50977))
            err = ERR_BATTLEGROUND_NONE;

        if (err <= 0)
        {
            WorldPacket data;
            sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, err);
            member->GetSession()->SendPacket(&data);
            return false;
        }

        return true;
    };

    ChatHandler handler(leader->GetSession());

    // check if player can queue:
    if (!group)
    {
        if (!CheckPlayerEnterQueue(leader))
        {
            handler.PSendSysMessage("# You cannot queue");
            return;
        }

        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
        GroupQueueInfo* ginfo = bgQueue.AddGroup(leader, nullptr, bracketEntry, false, false, 0, 0, 0);
        uint32 avgWaitTime = bgQueue.GetAverageQueueWaitTime(ginfo);
        uint32 queueSlot = leader->AddBattlegroundQueueId(bgQueueTypeId);

        WorldPacket data;
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bgt, queueSlot, STATUS_WAIT_QUEUE, avgWaitTime, 0, arenaType, TEAM_NEUTRAL);
        leader->GetSession()->SendPacket(&data);

        sScriptMgr->OnPlayerJoinArena(leader);

        handler.PSendSysMessage("# You entered arena skirmish queue {}/{} ({:2}v{:2})",
            bracketEntry->minLevel, bracketEntry->maxLevel, arenaType);
    }
    // check if group can queue:
    else
    {
        // No group or not a leader group
        if (group->GetLeaderGUID() != leader->GetGUID())
        {
            handler.PSendSysMessage("# You are not leader of group");
            return;
        }

        // pussywizard: for party members - remove queues for which leader is not queued to!
        std::unordered_set<uint32> leaderQueueTypeIds;

        for (uint32 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            leaderQueueTypeIds.emplace(leader->GetBattlegroundQueueTypeId(i));

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member)
            {
                continue;
            }

            for (uint32 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                BattlegroundQueueTypeId mqtid = member->GetBattlegroundQueueTypeId(i);
                if (mqtid == BATTLEGROUND_QUEUE_NONE)
                {
                    continue;
                }

                auto const& itr = leaderQueueTypeIds.find((uint32)mqtid);
                if (itr != leaderQueueTypeIds.end())
                {
                    continue;
                }

                BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(mqtid);

                if (bgQueue.IsPlayerInvitedToRatedArena(member->GetGUID()))
                {
                    WorldPacket data;
                    sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, ERR_BATTLEGROUND_JOIN_FAILED);
                    leader->GetSession()->SendPacket(&data);
                    return;
                }

                bgQueue.RemovePlayer(member->GetGUID(), false, i);
                member->RemoveBattlegroundQueueId(mqtid);
            }
        }

        err = group->CanJoinBattlegroundQueue(bgt, bgQueueTypeId, arenaType, arenaType, false, arenaSlot);

        uint32 avgWaitTime = 0;
        if (err > 0)
        {
            BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
            GroupQueueInfo* ginfo = bgQueue.AddGroup(leader, group, bracketEntry, false, false, 0, 0, 0);
            avgWaitTime = bgQueue.GetAverageQueueWaitTime(ginfo);
        }

        WorldPacket data;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member)
                continue;

            if (err <= 0)
            {
                sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            uint32 queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId);

            // send status packet
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bgt, queueSlot, STATUS_WAIT_QUEUE, avgWaitTime, 0, arenaType, TEAM_NEUTRAL, false);
            member->GetSession()->SendPacket(&data);

            sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, err);
            member->GetSession()->SendPacket(&data);

            sScriptMgr->OnPlayerJoinArena(member);
        }

        handler.PSendSysMessage("# You entered arena skirmish queue {}/{} ({}v{}) in group",
            bracketEntry->minLevel, bracketEntry->maxLevel, arenaType, arenaType);
    }
}

/*
void LLA::LoadConfig()
{
    if (!sConfigMgr->GetOption<bool>("LLA.Enable", false))
        return;

    auto AddReward = [](std::string const& levels)
    {
        if (levels.empty())
            return;

        std::string const& options = sConfigMgr->GetOption<std::string>(levels, "");
        std::string const& levelbrackets = options.substr(6);
    };

    auto const& keys = sConfigMgr->GetKeysByString("LLA.Levels.");
}

void LLA::Reward(Battleground* bg, TeamId winnerTeamId)
{
    //LOG_DEBUG("module.lla", "> Reward start");

    //for (auto const& [guid, player] : bg->GetPlayers())
    //{
    //    auto bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
    //    auto levelsString = Warhead::ToString(bracketEntry->minLevel) + "." + Warhead::ToString(bracketEntry->minLevel);
    //    auto configIsEnable = Warhead::StringFormat("LLA.{}.Enable", levelsString);

    //    // If disable reward for this bracket - skip
    //    if (sConfigMgr->GetOption<bool>(configIsEnable, false))
    //    {
    //        break;
    //    }

    //    bool isWinner = player->GetBgTeamId() == winnerTeamId;
    //    auto configCountWinner = sConfigMgr->GetOption<uint32>(Warhead::StringFormat("LLA.{}.ArenaPoint.Count.Winner", levelsString), 0);
    //    auto configCountLoser = sConfigMgr->GetOption<uint32>(Warhead::StringFormat("LLA.{}.ArenaPoint.Count.Loser", levelsString), 0);

    //    player->ModifyArenaPoints(isWinner ? configCountWinner : configCountLoser);
    //}
}
*/
