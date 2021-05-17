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
#include "BattlegroundMgr.h"
#include "Config.h"
#include "StringConvert.h"
#include "StringFormat.h"

LLA* LLA::instance()
{
    static LLA instance;
    return &instance;
}

void LLA::Reward(Battleground* bg, TeamId winnerTeamId)
{
    LOG_INFO("module", "> Reward start");

    for (auto const& [guid, player] : bg->GetPlayers())
    {
        auto bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
        auto levelsString = Warhead::ToString(bracketEntry->minLevel) + "." + Warhead::ToString(bracketEntry->minLevel);
        auto configIsEnable = Warhead::StringFormat("LLA.%s.Enable", levelsString.c_str());

        // If disable reward for this bracket - skip
        if (sConfigMgr->GetOption<bool>(configIsEnable, false))
        {
            break;
        }

        bool isWinner = player->GetBgTeamId() == winnerTeamId;
        auto configCountWinner = sConfigMgr->GetOption<uint32>(Warhead::StringFormat("LLA.%s.ArenaPoint.Count.Winner", levelsString.c_str()), 0);
        auto configCountLoser = sConfigMgr->GetOption<uint32>(Warhead::StringFormat("LLA.%s.ArenaPoint.Count.Loser", levelsString.c_str()), 0);

        player->ModifyArenaPoints(isWinner ? configCountWinner : configCountLoser);
    }
}
