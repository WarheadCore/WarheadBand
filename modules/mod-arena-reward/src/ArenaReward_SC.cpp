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

#include "ArenaReward.h"
#include "Chat.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

class ArenaReward_BG : public BGScript
{
public:
    ArenaReward_BG() : BGScript("ArenaReward_BG") { }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId) override
    {
        if (!MOD_CONF_GET_BOOL("ArenaReward.Enable"))
            return;

        // Not reward on end bg
        if (bg->isBattleground())
            return;

        sAR->SendRewardArena(bg, winnerTeamId);
    }
};

// Group all custom scripts
void AddSC_ArenaReward()
{
    new ArenaReward_BG();
}
