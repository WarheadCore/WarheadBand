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
#include "ScriptObject.h"

class Arena1v1_ArenaTeam : public ArenaTeamScript
{
public:
    Arena1v1_ArenaTeam() : ArenaTeamScript("Arena1v1_ArenaTeam") { }

    void OnGetSlotByType(const uint32 type, uint8& slot) override
    {
        sArena1v1Mgr->OnGetSlotByType(type, slot);
    }

    void OnGetArenaPoints(ArenaTeam* at, float& points) override
    {
        sArena1v1Mgr->OnGetArenaPoints(at, points);
    }

    void OnTypeIDToQueueID(const BattlegroundTypeId, const uint8 arenaType, uint32& bgQueueTypeId) override
    {
        sArena1v1Mgr->OnTypeIDToQueueID(arenaType, bgQueueTypeId);
    }

    void OnQueueIdToArenaType(const BattlegroundQueueTypeId bgQueueTypeId, uint8& arenaType) override
    {
        sArena1v1Mgr->OnQueueIdToArenaType(bgQueueTypeId, arenaType);
    }

    void OnSetArenaMaxPlayersPerTeam(const uint8 type, uint32& maxPlayersPerTeam) override
    {
        sArena1v1Mgr->OnSetArenaMaxPlayersPerTeam(type, maxPlayersPerTeam);
    }
};

class Arena1v1_Creature : public CreatureScript
{
public:
    Arena1v1_Creature() : CreatureScript("Arena1v1_Creature") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        return sArena1v1Mgr->OnGossipHello(player, creature);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        return sArena1v1Mgr->OnGossipSelect(player, creature, action);
    }
};

class Arena1v1_Player : public PlayerScript
{
public:
    Arena1v1_Player() : PlayerScript("Arena1v1_Player") { }

    void OnGetMaxPersonalArenaRatingRequirement(const Player* player, uint32 minslot, uint32& maxArenaRating) const override
    {
        sArena1v1Mgr->OnGetMaxPersonalArenaRatingRequirement(player, minslot, maxArenaRating);
    }
};

class Arena1v1_World : public WorldScript
{
public:
    Arena1v1_World() : WorldScript("Arena1v1_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sArena1v1Mgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sArena1v1Mgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_Arena1v1()
{
    new Arena1v1_ArenaTeam();
    new Arena1v1_Creature();
    new Arena1v1_Player();
    new Arena1v1_World();
}
