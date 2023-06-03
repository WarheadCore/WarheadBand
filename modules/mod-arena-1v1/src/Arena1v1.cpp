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

#include "Arena1v1.h"
#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "DisableMgr.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "StringConvert.h"
#include "Tokenize.h"

constexpr uint32 ARENA_TEAM_1V1 = 1;
constexpr uint32 ARENA_TYPE_1V1 = 1;
constexpr uint32 BATTLEGROUND_QUEUE_1V1 = 11;
constexpr BattlegroundQueueTypeId bgQueueTypeId = BattlegroundQueueTypeId(BATTLEGROUND_QUEUE_5v5 + 1);

Arena1v1Mgr* Arena1v1Mgr::instance()
{
    static Arena1v1Mgr instance;
    return &instance;
}

void Arena1v1Mgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("Arena1v1.Enable");
    if (!_isEnable)
        return;

    _arenaSlot = MOD_CONF_GET_INT("Arena1v1.ArenaSlotID");
    _vendorRating = MOD_CONF_GET_BOOL("Arena1v1.VendorRating");
    _pointsMulti = MOD_CONF_GET_FLOAT("Arena1v1.ArenaPointsMulti");
    _createTeamCost = MOD_CONF_GET_UINT("Arena1v1.Costs");
    _minLevel = MOD_CONF_GET_UINT("Arena1v1.MinLevel");
    _blockForbiddenTalents = MOD_CONF_GET_BOOL("Arena1v1.BlockForbiddenTalents");

    auto forbiddenTalentsList = MOD_CONF_GET_STR("Arena1v1.ForbiddenTalentsIDs");
    if (forbiddenTalentsList.empty())
        return;

    forbiddenTalents.clear();

    for (auto talendIDStr : Warhead::Tokenize(forbiddenTalentsList, ',', false))
    {
        auto talendID = Warhead::StringTo<uint32>(talendIDStr);
        if (!talendID)
        {
            LOG_ERROR("module", "Incorrect talent id: {}", talendIDStr);
            continue;
        }

        forbiddenTalents.emplace_back(*talendID);
    }
}

void Arena1v1Mgr::Initialize()
{
    if (!_isEnable)
        return;

    ArenaTeam::ArenaSlotByType.emplace(ARENA_TEAM_1V1, _arenaSlot);
    ArenaTeam::ArenaReqPlayersForType.emplace(ARENA_TYPE_1V1, 2);

    BattlegroundMgr::queueToBg.emplace(BATTLEGROUND_QUEUE_1V1,  BATTLEGROUND_AA);
    BattlegroundMgr::QueueToArenaType.emplace(BATTLEGROUND_QUEUE_1V1, ArenaType(ARENA_TYPE_1V1));
    BattlegroundMgr::ArenaTypeToQueue.emplace(ARENA_TYPE_1V1, BattlegroundQueueTypeId(BATTLEGROUND_QUEUE_1V1));
}

void Arena1v1Mgr::OnGetMaxPersonalArenaRatingRequirement(const Player* player, uint32 minslot, uint32& maxArenaRating) const
{
    if (!_isEnable || !_vendorRating || minslot >= _arenaSlot)
        return;

    ArenaTeam* at = sArenaTeamMgr->GetArenaTeamByCaptain(player->GetGUID(), ARENA_TEAM_1V1);
    if (!at)
        return;

    maxArenaRating = std::max(at->GetRating(), maxArenaRating);
}

void Arena1v1Mgr::OnGetSlotByType(const uint32 type, uint8& slot) const
{
    if (!_isEnable || type != ARENA_TEAM_1V1)
        return;

    slot = _arenaSlot;
}

void Arena1v1Mgr::OnGetArenaPoints(ArenaTeam* at, float& points) const
{
    if (!_isEnable || at->GetType() != ARENA_TEAM_1V1)
        return;

    points *= _pointsMulti;
}

void Arena1v1Mgr::OnTypeIDToQueueID(uint8 arenaType, uint32& bgQueueTypeId_) const
{
    if (!_isEnable || arenaType != ARENA_TYPE_1V1)
        return;

    bgQueueTypeId_ = bgQueueTypeId;
}

void Arena1v1Mgr::OnQueueIdToArenaType(BattlegroundQueueTypeId bgQueueTypeId_, uint8& arenaType) const
{
    if (!_isEnable || bgQueueTypeId_ != bgQueueTypeId)
        return;

    arenaType = ARENA_TYPE_1V1;
}

void Arena1v1Mgr::OnSetArenaMaxPlayersPerTeam(const uint8 type, uint32& maxPlayersPerTeam) const
{
    if (!_isEnable || type != ARENA_TYPE_1V1)
        return;

    maxPlayersPerTeam = 1;
}

bool Arena1v1Mgr::OnGossipHello(Player* player, Creature* creature) const
{
    if (!player || !creature)
        return true;

    if (!_isEnable)
    {
        ChatHandler(player->GetSession()).SendSysMessage("Arena 1v1 disabled!");
        return true;
    }

    if (player->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeId))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Queue leave 1v1 Arena", GOSSIP_SENDER_MAIN, 3, "Are you sure?", 0, false);
    else
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Queue enter 1v1 Arena (UnRated)", GOSSIP_SENDER_MAIN, 20);

    if (!player->GetArenaTeamId(ArenaTeam::GetSlotByType(ARENA_TEAM_1V1)))
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Create new 1v1 Arena Team", GOSSIP_SENDER_MAIN, 1, "Are you sure?", _createTeamCost, false);
    else
    {
        if (!player->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeId))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Queue enter 1v1 Arena (Rated)", GOSSIP_SENDER_MAIN, 2);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "ArenaTeam Clear", GOSSIP_SENDER_MAIN, 5, "Are you sure?", 0, false);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Shows your statistics", GOSSIP_SENDER_MAIN, 4);
    }

    SendGossipMenuFor(player, 68, creature);
    return true;
}

bool Arena1v1Mgr::OnGossipSelect(Player* player, Creature* creature, uint32 action)
{
    if (!player || !creature)
        return true;

    ClearGossipMenuFor(player);
    ChatHandler handler{ player->GetSession() };

    switch (action)
    {
        case 1: // Create new Arenateam
        {
            if (_minLevel <= player->getLevel())
            {
                if (player->GetMoney() >= _createTeamCost && CreateArenaTeam(player, creature))
                    player->ModifyMoney(-static_cast<int32>(_createTeamCost));
            }
            else
            {
                handler.PSendSysMessage("You have to be level {} + to create a 1v1 arena team.", _minLevel);
                CloseGossipMenuFor(player);
                return true;
            }
        }
        break;
        case 2: // Join Queue Arena (rated)
        {
            if (Arena1v1CheckTalents(player) && !JoinQueueArena(player, creature, true))
                handler.SendSysMessage("Something went wrong when joining the queue.");

            CloseGossipMenuFor(player);
            return true;
        }
        break;
        case 20: // Join Queue Arena (unrated)
        {
            if (Arena1v1CheckTalents(player) && !JoinQueueArena(player, creature, false))
                handler.SendSysMessage("Something went wrong when joining the queue.");

            CloseGossipMenuFor(player);
            return true;
        }
        break;
        case 3: // Leave Queue
        {
            uint8 arenaType = ARENA_TYPE_1V1;

            if (!player->InBattlegroundQueueForBattlegroundQueueType(bgQueueTypeId))
                return true;

            WorldPacket data;
            data << arenaType << (uint8)0x0 << (uint32)BATTLEGROUND_AA << (uint16)0x0 << (uint8)0x0;
            player->GetSession()->HandleBattleFieldPortOpcode(data);
            CloseGossipMenuFor(player);
            return true;
        }
        break;
        case 4: // get statistics
        {
            ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(ArenaTeam::GetSlotByType(ARENA_TEAM_1V1)));
            if (at)
            {
                auto& stats{ at->GetStats() };
                handler.PSendSysMessage("Rating: {}", stats.Rating);
                handler.PSendSysMessage("Rank: {}", stats.Rank);
                handler.PSendSysMessage("Season Games: {}", stats.SeasonGames);
                handler.PSendSysMessage("Season Wins: {}", stats.SeasonWins);
                handler.PSendSysMessage("Week Games: {}", stats.WeekGames);
                handler.PSendSysMessage("Week Wins: {}", stats.WeekWins);
            }
        }
        break;
        case 5: // Disband arenateam
        {
            WorldPacket data;
            data << player->GetArenaTeamId(ArenaTeam::GetSlotByType(ARENA_TEAM_1V1));
            player->GetSession()->HandleArenaTeamLeaveOpcode(data);

            handler.SendSysMessage("Arenateam deleted!");
            CloseGossipMenuFor(player);
            return true;
        }
        break;
    }

    OnGossipHello(player, creature);
    return true;
}

bool Arena1v1Mgr::JoinQueueArena(Player* player, Creature* creature, bool isRated)
{
    if (!player || !creature)
        return false;

    if (_minLevel > player->getLevel())
        return false;

    uint8 arenaSlot = ArenaTeam::GetSlotByType(ARENA_TEAM_1V1);
    uint8 arenaType = ARENA_TYPE_1V1;
    uint32 arenaRating = 0;
    uint32 matchmakerRating = 0;

    // Ignore if we already in BG or BG queue
    if (player->InBattleground())
        return false;

    // Check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        LOG_ERROR("module", "Battleground: template bg (all arenas) not found");
        return false;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, nullptr))
    {
        ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARENA_DISABLED);
        return false;
    }

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
    if (!bracketEntry)
        return false;

    // Check if already in queue
    if (player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
        return false; // Player is already in this queue

    // Check if it has free queue slots
    if (!player->HasFreeBattlegroundQueueId())
        return false;

    uint32 ateamId = 0;

    if (isRated)
    {
        ateamId = player->GetArenaTeamId(arenaSlot);
        ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(ateamId);

        if (!at)
        {
            player->GetSession()->SendNotInArenaTeamPacket(arenaType);
            return false;
        }

        // get the team rating for queueing
        arenaRating = at->GetRating();
        matchmakerRating = arenaRating;

        // the arenateam id must match for everyone in the group
        if (arenaRating <= 0)
            arenaRating = 1;
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
    BattlegroundTypeId bgTypeId = BATTLEGROUND_AA;

    bg->SetRated(isRated);
    bg->SetMaxPlayersPerTeam(1);

    GroupQueueInfo* ginfo = bgQueue.AddGroup(player, nullptr, bgTypeId, bracketEntry, arenaType, isRated != 0, false, arenaRating, matchmakerRating, ateamId, 0);
    uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo);
    uint32 queueSlot = player->AddBattlegroundQueueId(bgQueueTypeId);

    // Send status packet (in queue)
    WorldPacket data;
    sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, arenaType, TEAM_NEUTRAL, isRated);
    player->GetSession()->SendPacket(&data);

    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, arenaType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
    return true;
}

bool Arena1v1Mgr::CreateArenaTeam(Player* player, Creature* creature)
{
    if (!player || !creature)
        return false;

    uint8 slot = ArenaTeam::GetSlotByType(ARENA_TEAM_1V1);

    // Just to make sure as some other module might edit this value
    if (slot == 0)
        return false;

    // Check if player is already in an arena team
    if (player->GetArenaTeamId(slot))
    {
        player->GetSession()->SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, player->GetName(), "You are already in an arena team!", ERR_ALREADY_IN_ARENA_TEAM);
        return false;
    }

    // Teamname = playername
    // if teamname exist, we have to choose another name (playername + number)
    uint8 i = 0;
    std::string teamName{ Warhead::StringFormat("{}", player->GetName()) };

    do
    {
        if (sArenaTeamMgr->GetArenaTeamByName(teamName)) // teamname exist, so choose another name
            teamName = Warhead::StringFormat("{} - {}", player->GetName(), ++i);
        else
            break;
    } while (i < 100); // should never happen

    // Create arena team
    auto arenaTeam = new ArenaTeam();
    if (!arenaTeam->Create(player->GetGUID(), ARENA_TEAM_1V1, teamName, 4283124816, 45, 4294242303, 5, 4294705149))
    {
        delete arenaTeam;
        return false;
    }

    // Register arena team
    sArenaTeamMgr->AddArenaTeam(arenaTeam);
    ChatHandler(player->GetSession()).SendSysMessage("1v1 Arenateam successfully created!");
    return true;
}

bool Arena1v1Mgr::Arena1v1CheckTalents(Player* player)
{
    if (!player)
        return false;

    if (!_blockForbiddenTalents)
        return true;

    for (auto const& talentEntry : sTalentStore)
    {
        if (std::find(forbiddenTalents.begin(), forbiddenTalents.end(), talentEntry->TalentID) != forbiddenTalents.end())
        {
            ChatHandler(player->GetSession()).SendSysMessage("You can not join because you have forbidden talents.");
            return false;
        }

        for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
            if (talentEntry->RankID[rank] == 0)
                continue;
    }

    return true;
}
