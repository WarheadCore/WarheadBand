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

#include "AnticheatMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

class AnticheatPlayerScript : public PlayerScript
{
public:
    AnticheatPlayerScript() : PlayerScript("AnticheatPlayerScript") { }

    void OnLogout(Player* player) override
    {
        sAnticheatMgr->HandlePlayerLogout(player);
    }

    void OnLogin(Player* player) override
    {
        sAnticheatMgr->HandlePlayerLogin(player);
    }
};

class AnticheatWorldScript : public WorldScript
{
public:
    AnticheatWorldScript() : WorldScript("AnticheatWorldScript") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sAnticheatMgr->LoadConfig(reload);
    }
};

class AnticheatMovementHandlerScript : public MovementHandlerScript
{
public:
    AnticheatMovementHandlerScript() : MovementHandlerScript("AnticheatMovementHandlerScript") { }

    void OnPlayerMove(Player* player, MovementInfo* mi, uint32 opcode) override
    {
        sAnticheatMgr->StartHackDetection(player, mi, opcode);
    }
};

void AddSC_Anticheat()
{
    new AnticheatWorldScript();
    new AnticheatPlayerScript();
    new AnticheatMovementHandlerScript();
}
