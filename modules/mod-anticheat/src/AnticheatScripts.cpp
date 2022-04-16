/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
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

#include "AccountMgr.h"
#include "AnticheatMgr.h"
#include "Chat.h"
#include "GameTime.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"
//#include "World.h"

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

    void OnUpdate(uint32 /*diff*/) override
    {
        if (GameTime::GetGameTime() > _resetTime)
        {
            LOG_INFO("module", "Anticheat: Resetting daily report states.");
            sAnticheatMgr->ResetDailyReportStates();
            UpdateReportResetTime();
            LOG_INFO("module", "Anticheat: Next daily report reset: {}", Warhead::Time::TimeToHumanReadable(_resetTime));
        }

        if (GameTime::GetUptime() > _lastIterationPlayer)
        {
            _lastIterationPlayer = GameTime::GetUptime() + Seconds(MOD_CONF_GET_UINT("Anticheat.SaveReportsTime"));

            LOG_INFO("module", "Saving reports for {} players.", sWorld->GetPlayerCount());

            for (SessionMap::const_iterator itr = sWorld->GetAllSessions().begin(); itr != sWorld->GetAllSessions().end(); ++itr)
                if (Player* plr = itr->second->GetPlayer())
                    sAnticheatMgr->SavePlayerData(plr);
        }
    }

    void OnAfterConfigLoad(bool reload) override
    {
        sAnticheatMgr->LoadConfig(reload);
    }

private:
    void UpdateReportResetTime()
    {
        _resetTime = Seconds(Warhead::Time::GetNextTimeWithDayAndHour(-1, 6));
    }

    Seconds _resetTime = 0s;
    Seconds _lastIterationPlayer = GameTime::GetUptime() + 30s; //TODO: change 30 secs static to a configurable option
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
