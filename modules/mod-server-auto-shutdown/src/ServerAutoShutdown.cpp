/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
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
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect time in config option 'ServerAutoShutdown.Time' - '%s'", configTime.c_str());
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
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect time in config option 'ServerAutoShutdown.Time' - '%s'", configTime.c_str());
        _isEnableModule = false;
        return;
    }

    uint8 hour = *Warhead::StringTo<uint8>(tokens.at(0));
    uint8 minute = *Warhead::StringTo<uint8>(tokens.at(1));
    uint8 second = *Warhead::StringTo<uint8>(tokens.at(2));

    if (hour > 23)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect hour in config option 'ServerAutoShutdown.Time' - '%s'", configTime.c_str());
        _isEnableModule = false;
    }
    else if (minute >= 60)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect minute in config option 'ServerAutoShutdown.Time' - '%s'", configTime.c_str());
        _isEnableModule = false;
    }
    else if (second >= 60)
    {
        LOG_ERROR("modules", "> ServerAutoShutdown: Incorrect second in config option 'ServerAutoShutdown.Time' - '%s'", configTime.c_str());
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

    LOG_INFO("modules", "> ServerAutoShutdown: Next time to shutdown - %s", Warhead::Time::TimeToHumanReadable(nextResetTime).c_str());
    LOG_INFO("modules", "> ServerAutoShutdown: Remaining time to shutdown - %s", Warhead::Time::ToTimeString<Seconds>(diffToShutdown).c_str());
    LOG_INFO("modules", " ");

    uint32 preAnnounceSeconds = sConfigMgr->GetOption<uint32>("ServerAutoShutdown.PreAnnounce.Seconds", 3600);
    if (preAnnounceSeconds > DAY)
    {
        LOG_INFO("modules", "> ServerAutoShutdown: Ahah, how could this happen? Time to preannouce more 1 day? (%u). Set to 1 hour (3600)", preAnnounceSeconds);
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

    LOG_INFO("modules", "> ServerAutoShutdown: Next time to pre annouce - %s", Warhead::Time::TimeToHumanReadable(timeToPreAnnounce).c_str());
    LOG_INFO("modules", "> ServerAutoShutdown: Remaining time to pre annouce - %s", Warhead::Time::ToTimeString<Seconds>(diffToPreAnnounce).c_str());
    LOG_INFO("modules", " ");

    // Add task for pre shutdown announce
    scheduler.Schedule(Seconds(diffToPreAnnounce), [preAnnounceSeconds](TaskContext /*context*/)
    {
        std::string preAnnounceMessageFormat = sConfigMgr->GetOption<std::string>("ServerAutoShutdown.PreAnnounce.Message", "[SERVER]: Automated (quick) server restart in %s");
        std::string message = Warhead::StringFormat(preAnnounceMessageFormat, Warhead::Time::ToTimeString<Seconds>(preAnnounceSeconds));

        LOG_INFO("modules", "> %s", message.c_str());

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
