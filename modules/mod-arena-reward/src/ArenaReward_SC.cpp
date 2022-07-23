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

#include "ArenaReward.h"
#include "ScriptObject.h"

class ArenaReward_BG : public BGScript
{
public:
    ArenaReward_BG() : BGScript("ArenaReward_BG") { }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId) override
    {
        sAR->SendRewardArena(bg, winnerTeamId);
    }
};

class ArenaReward_World : public WorldScript
{
public:
    ArenaReward_World() : WorldScript("ArenaReward_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sAR->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sAR->Init();
    }
};

// Group all custom scripts
void AddSC_ArenaReward()
{
    new ArenaReward_BG();
    new ArenaReward_World();
}
