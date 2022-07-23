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

#ifndef _LOW_LEVEL_ARENA_H_
#define _LOW_LEVEL_ARENA_H_

#include "Define.h"

//class Battleground;
class Player;

class LLA
{
    LLA() = default;
    ~LLA() = default;

    LLA(LLA const&) = delete;
    LLA(LLA&&) = delete;
    LLA& operator= (LLA const&) = delete;
    LLA& operator= (LLA&&) = delete;

public:
    static LLA* instance();

    //void Reward(Battleground* bg, TeamId winnerTeamId);
    //void LoadConfig();
    void AddQueue(Player* leader);
};

#define sLLA LLA::instance()

#endif /* _LOW_LEVEL_ARENA_H_ */
