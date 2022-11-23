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

#ifndef _ARENA_1V1_H_
#define _ARENA_1V1_H_

#include "Define.h"
#include <vector>

class ArenaTeam;
class Creature;
class Player;

//enum BattlegroundTypeId : uint8;
enum BattlegroundQueueTypeId : uint8;

class Arena1v1Mgr
{
public:
    static Arena1v1Mgr* instance();

    void Initialize();
    void LoadConfig(bool reload);

    [[nodiscard]] inline bool IsEnable() const { return _isEnable; }

    // Hooks
    void OnGetMaxPersonalArenaRatingRequirement(const Player* player, uint32 minslot, uint32& maxArenaRating) const;
    void OnGetSlotByType(uint32 type, uint8& slot) const;
    void OnGetArenaPoints(ArenaTeam* at, float& points) const;
    void OnTypeIDToQueueID(uint8 arenaType, uint32& bgQueueTypeId_) const;
    void OnQueueIdToArenaType(BattlegroundQueueTypeId bgQueueTypeId_, uint8& arenaType) const;
    void OnSetArenaMaxPlayersPerTeam(uint8 type, uint32& maxPlayersPerTeam) const;

    bool OnGossipHello(Player* player, Creature* creature) const;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 action);

private:
    bool JoinQueueArena(Player* player, Creature* creature, bool isRated);
    bool CreateArenaTeam(Player* player, Creature* creature);
    bool Arena1v1CheckTalents(Player* player);

    bool _isEnable{};
    uint32 _arenaSlot{ 3 };
    bool _vendorRating{};
    float _pointsMulti{};
    uint32 _createTeamCost{};
    uint32 _minLevel{};
    bool _blockForbiddenTalents{};
    std::vector<uint32> forbiddenTalents;

    Arena1v1Mgr() = default;
    ~Arena1v1Mgr() = default;

    Arena1v1Mgr(Arena1v1Mgr const&) = delete;
    Arena1v1Mgr(Arena1v1Mgr&&) = delete;
    Arena1v1Mgr& operator=(Arena1v1Mgr const&) = delete;
    Arena1v1Mgr& operator=(Arena1v1Mgr&&) = delete;
};

#define sArena1v1Mgr Arena1v1Mgr::instance()

#endif // _ARENA_1V1_H_
