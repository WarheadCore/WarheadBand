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
#include "ServerAutoShutdown.h"
#include "Config.h"
#include "Duration.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "TaskScheduler.h"
#include "Tokenize.h"
#include "Util.h"
#include "World.h"
#include "Timer.h"

namespace
{
    // Scheduler - for update
    TaskScheduler scheduler;

    time_t GetNextResetTime(time_t time, uint8 hour, uint8 minute, uint8 second)
    {
        tm timeLocal = Warhead::Time::TimeBreakdown(time);
        timeLocal.tm_hour = hour;
        timeLocal.tm_min = minute;
        timeLocal.tm_sec = second;

        time_t midnightLocal = mktime(&timeLocal);

        if (midnightLocal <= time)
            midnightLocal += DAY;

        return midnightLocal;
    }
}

/*static*/ ServerAutoShutdown* ServerAutoShutdown::instance()
{
    static ServerAutoShutdown instance;
    return &instance;
}

void ServerAutoShutdown::Init()
{
    _isEnableModule = sConfigMgr->GetOption<bool>("ServerAutoShutdown.Enabled", false);

    if (!_isEnableModule)
        return;

    std::string configTime = sConfigMgr->GetOption<std::string>("ServerAutoShutdown.Time", "04:00:00");
    auto const& tokens = Warhead::Tokenize(configTime, ':', false);

    if (tokens.size() != 3)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect time in config option 'ServerAutoShutdown.Time' - '{}'", configTime);
        _isEnableModule = false;
        return;
    }

    // Check convert to int
    auto CheckTime = [tokens](std::initializer_list<uint8> index)
    {
        for (auto const& itr : index)
        {
            if (Warhead::StringTo<uint8>(tokens.at(itr)) == std::nullopt)
                return false;
        }

        return true;
    };

    if (!CheckTime({ 0, 1, 2 }))
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect time in config option 'ServerAutoShutdown.Time' - '{}'", configTime);
        _isEnableModule = false;
        return;
    }

    uint8 hour = *Warhead::StringTo<uint8>(tokens.at(0));
    uint8 minute = *Warhead::StringTo<uint8>(tokens.at(1));
    uint8 second = *Warhead::StringTo<uint8>(tokens.at(2));

    if (hour > 23)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect hour in config option 'ServerAutoShutdown.Time' - '{}'", configTime);
        _isEnableModule = false;
    }
    else if (minute >= 60)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect minute in config option 'ServerAutoShutdown.Time' - '{}'", configTime);
        _isEnableModule = false;
    }
    else if (second >= 60)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect second in config option 'ServerAutoShutdown.Time' - '{}'", configTime);
        _isEnableModule = false;
    }

    auto nowTime = time(nullptr);
    uint64 nextResetTime = GetNextResetTime(nowTime, hour, minute, second);
    uint32 diffToShutdown = nextResetTime - static_cast<uint32>(nowTime);

    if (diffToShutdown < 10)
    {
        LOG_INFO("modules", "> ServerAutoShutdown: Next time to shutdown < 10 seconds, Set next day");
        nextResetTime += DAY;
    }

    diffToShutdown = nextResetTime - static_cast<uint32>(nowTime);

    LOG_INFO("modules", " ");
    LOG_INFO("modules", "> ServerAutoShutdown: System loading");

    // Cancel all task for support reload config
    scheduler.CancelAll();
    sWorld->ShutdownCancel();

    LOG_INFO("modules", "> ServerAutoShutdown: Next time to shutdown - {}", Warhead::Time::TimeToHumanReadable(nextResetTime));
    LOG_INFO("modules", "> ServerAutoShutdown: Remaining time to shutdown - {}", Warhead::Time::ToTimeString<Seconds>(diffToShutdown));
    LOG_INFO("modules", " ");

    uint32 preAnnounceSeconds = sConfigMgr->GetOption<uint32>("ServerAutoShutdown.PreAnnounce.Seconds", 3600);
    if (preAnnounceSeconds > DAY)
    {
        LOG_INFO("modules", "> ServerAutoShutdown: Ahah, how could this happen? Time to preannouce more 1 day? ({}). Set to 1 hour (3600)", preAnnounceSeconds);
        preAnnounceSeconds = 3600;
    }

    uint32 timeToPreAnnounce = static_cast<uint32>(nextResetTime) - preAnnounceSeconds;
    uint32 diffToPreAnnounce = timeToPreAnnounce - static_cast<uint32>(nowTime);

    // Ingnore pre announce time and set is left
    if (diffToShutdown < preAnnounceSeconds)
    {
        timeToPreAnnounce = static_cast<uint32>(nowTime) + 1;
        diffToPreAnnounce = 1;
        preAnnounceSeconds = diffToShutdown;
    }

    LOG_INFO("modules", "> ServerAutoShutdown: Next time to pre annouce - {}", Warhead::Time::TimeToHumanReadable(timeToPreAnnounce));
    LOG_INFO("modules", "> ServerAutoShutdown: Remaining time to pre annouce - {}", Warhead::Time::ToTimeString<Seconds>(diffToPreAnnounce));
    LOG_INFO("modules", " ");

    // Add task for pre shutdown announce
    scheduler.Schedule(Seconds(diffToPreAnnounce), [preAnnounceSeconds](TaskContext /*context*/)
    {
        std::string preAnnounceMessageFormat = sConfigMgr->GetOption<std::string>("ServerAutoShutdown.PreAnnounce.Message", "[SERVER]: Automated (quick) server restart in %s");
        std::string message = Warhead::StringFormat(preAnnounceMessageFormat, Warhead::Time::ToTimeString<Seconds>(preAnnounceSeconds));

        LOG_INFO("modules", "> {}", message);

        sWorld->SendServerMessage(SERVER_MSG_STRING, message.c_str());
        sWorld->ShutdownServ(preAnnounceSeconds, 0, SHUTDOWN_EXIT_CODE);
    });
}

void ServerAutoShutdown::OnUpdate(uint32 diff)
{
    // If module disable, why do the update? hah
    if (!_isEnableModule)
        return;

    scheduler.Update(diff);
}
