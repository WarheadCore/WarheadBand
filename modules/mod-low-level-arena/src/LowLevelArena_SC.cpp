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

class LowLevelArena_BG : public BGScript
{
public:
    LowLevelArena_BG() : BGScript("LowLevelArena_BG") { }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId) override
    {
        if (!sConfigMgr->GetOption<bool>("LLA.Enable", false))
            return;

        // Not reward for bg or rated arena
        if (bg->isBattleground() || bg->isRated())
        {
            return;
        }

        sLLA->Reward(bg, winnerTeamId);
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
    new LowLevelArena_BG();
    new LowLevelArena_World();
}
