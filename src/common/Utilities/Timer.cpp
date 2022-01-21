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

#include "Timer.h"
#include "StringFormat.h"
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timespan.h>
#include <iomanip>
#include <sstream>

template<>
WH_COMMON_API uint32 Warhead::Time::TimeStringTo<Seconds>(std::string_view timestring)
{
    uint32 secs = 0;
    uint32 buffer = 0;
    uint32 multiplier = 0;

    for (char itr : timestring)
    {
        if (isdigit(itr))
        {
            buffer *= 10;
            buffer += itr - '0';
        }
        else
        {
            switch (itr)
            {
            case 'd':
                multiplier = DAY;
                break;
            case 'h':
                multiplier = HOUR;
                break;
            case 'm':
                multiplier = MINUTE;
                break;
            case 's':
                multiplier = 1;
                break;
            default:
                return 0; // bad format
            }

            buffer *= multiplier;
            secs += buffer;
            buffer = 0;
        }
    }

    return secs;
}

template<>
WH_COMMON_API std::string Warhead::Time::ToTimeString<Microseconds>(uint64 durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    Poco::Timespan span(durationTime);

    uint32 microsecs = span.microseconds();
    uint32 millisecs = span.milliseconds();
    uint32 secs = span.seconds();
    uint32 minutes = span.minutes();
    uint32 hours = span.hours();
    uint32 days = span.days();

    if (timeFormat == TimeFormat::Numeric)
    {
        if (days)
            return Warhead::StringFormat("{}:{:02}:{:02}:{:02}:{:02}:{:02}", days, hours, minutes, secs, millisecs, microsecs);
        else if (hours)
            return Warhead::StringFormat("{}::{:02}{:02}:{:02}:{:02}", hours, minutes, secs, millisecs, microsecs);
        else if (minutes)
            return Warhead::StringFormat("{}:{:02}:{:02}:{:02}", minutes, secs, millisecs, microsecs);
        else if (secs)
            return Warhead::StringFormat("{}:{:02}:{:02}", secs, millisecs, microsecs);
        else if (millisecs)
            return Warhead::StringFormat("{}:{:02}", millisecs, microsecs);
        else if (millisecs) // microsecs
            return Warhead::StringFormat("{}", microsecs);
    }

    std::ostringstream ss;
    std::string stringTime;

    auto GetStringFormat = [&](uint32 timeType, std::string_view shortText, std::string_view fullText1, std::string_view fullText)
    {
        ss << timeType;

        switch (timeFormat)
        {
            case TimeFormat::ShortText:
                ss << shortText;
                break;
            case TimeFormat::FullText:
                ss << (timeType == 1 ? fullText1 : fullText);
                break;
            default:
                ss << "<Unknown time format>";
        }
    };

    if (days)
        GetStringFormat(days, "d ", " Day ", " Days ");

    if (timeOutput == TimeOutput::Days)
        stringTime = ss.str();

    if (hours)
        GetStringFormat(hours, "h ", " Hour ", " Hours ");

    if (timeOutput == TimeOutput::Hours)
        stringTime = ss.str();

    if (minutes)
        GetStringFormat(minutes, "m ", " Minute ", " Minutes ");

    if (timeOutput == TimeOutput::Minutes)
        stringTime = ss.str();

    if (secs)
        GetStringFormat(secs, "s ", " Second ", " Seconds ");

    if (timeOutput == TimeOutput::Seconds)
        stringTime = ss.str();

    if (millisecs)
        GetStringFormat(millisecs, "ms ", " Millisecond ", " Milliseconds ");

    if (timeOutput == TimeOutput::Milliseconds)
        stringTime = ss.str();

    if (microsecs)
        GetStringFormat(microsecs, "us ", " Microsecond ", " Microseconds ");

    if (timeOutput == TimeOutput::Microseconds)
        stringTime = ss.str();

    return Warhead::String::TrimRightInPlace(stringTime);
}

template<>
WH_COMMON_API std::string Warhead::Time::ToTimeString<Milliseconds>(uint64 durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    return ToTimeString<Microseconds>(durationTime * Poco::Timespan::MILLISECONDS, timeOutput, timeFormat);
}

template<>
WH_COMMON_API std::string Warhead::Time::ToTimeString<Seconds>(uint64 durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    return ToTimeString<Microseconds>(durationTime * Poco::Timespan::SECONDS, timeOutput, timeFormat);
}

template<>
WH_COMMON_API std::string Warhead::Time::ToTimeString<Minutes>(uint64 durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    return ToTimeString<Microseconds>(durationTime * Poco::Timespan::MINUTES, timeOutput, timeFormat);
}

template<>
WH_COMMON_API std::string Warhead::Time::ToTimeString<Seconds>(std::string_view durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    return ToTimeString<Seconds>(TimeStringTo<Seconds>(durationTime), timeOutput, timeFormat);
}

std::string Warhead::Time::ToTimeString(Microseconds durationTime, TimeOutput timeOutput /*= TimeOutput::Seconds*/, TimeFormat timeFormat /*= TimeFormat::ShortText*/)
{
    return ToTimeString<Microseconds>(durationTime.count(), timeOutput, timeFormat);
}

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
struct tm* localtime_r(time_t const* time, struct tm* result)
{
    localtime_s(result, time);
    return result;
}
#endif

std::tm Warhead::Time::TimeBreakdown(time_t time /*= 0*/)
{
    if (!time)
    {
        time = GetEpochTime().count();
    }

    std::tm timeLocal;
    localtime_r(&time, &timeLocal);
    return timeLocal;
}

time_t Warhead::Time::LocalTimeToUTCTime(time_t time)
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    return time + _timezone;
#else
    return time + timezone;
#endif
}

time_t Warhead::Time::GetLocalHourTimestamp(time_t time, uint8 hour, bool onlyAfterTime)
{
    tm timeLocal = TimeBreakdown(time);
    timeLocal.tm_hour = 0;
    timeLocal.tm_min = 0;
    timeLocal.tm_sec = 0;

    time_t midnightLocal = mktime(&timeLocal);
    time_t hourLocal = midnightLocal + hour * HOUR;

    if (onlyAfterTime && hourLocal <= time)
        hourLocal += DAY;

    return hourLocal;
}

std::string Warhead::Time::TimeToTimestampStr(Seconds time /*= 0s*/, std::string_view fmt /*= {}*/)
{
    std::stringstream ss;
    std::string format{ fmt };
    time_t t = time.count();

    if (format.empty())
    {
        format = "%Y-%m-%d %X";
    }

    ss << std::put_time(std::localtime(&t), format.c_str());
    return ss.str();
}

std::string Warhead::Time::TimeToHumanReadable(Seconds time /*= 0s*/, std::string_view fmt /*= {}*/)
{
    std::stringstream ss;
    std::string format{ fmt };
    time_t t = time.count();

    if (format.empty())
    {
        format = "%a %b %d %Y %X";
    }

    ss << std::put_time(std::localtime(&t), format.c_str());
    return ss.str();
}

time_t Warhead::Time::GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour)
{
    if (hour < 0 || hour > 23)
    {
        hour = 0;
    }

    tm localTm = TimeBreakdown();
    localTm.tm_hour = hour;
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    if (dayOfWeek < 0 || dayOfWeek > 6)
    {
        dayOfWeek = (localTm.tm_wday + 1) % 7;
    }

    uint32 add;

    if (localTm.tm_wday >= dayOfWeek)
    {
        add = (7 - (localTm.tm_wday - dayOfWeek)) * DAY;
    }
    else
    {
        add = (dayOfWeek - localTm.tm_wday) * DAY;
    }

    return mktime(&localTm) + add;
}

time_t Warhead::Time::GetNextTimeWithMonthAndHour(int8 month, int8 hour)
{
    if (hour < 0 || hour > 23)
    {
        hour = 0;
    }

    tm localTm = TimeBreakdown();
    localTm.tm_mday = 1;
    localTm.tm_hour = hour;
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    if (month < 0 || month > 11)
    {
        month = (localTm.tm_mon + 1) % 12;

        if (!month)
        {
            localTm.tm_year += 1;
        }
    }
    else if (localTm.tm_mon >= month)
    {
        localTm.tm_year += 1;
    }

    localTm.tm_mon = month;
    return mktime(&localTm);
}

uint32 Warhead::Time::GetSeconds(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_sec;
}

uint32 Warhead::Time::GetMinutes(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_min;
}

uint32 Warhead::Time::GetHours(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_hour;
}

uint32 Warhead::Time::GetDayInWeek(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_wday;
}

uint32 Warhead::Time::GetDayInMonth(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_mday;
}

uint32 Warhead::Time::GetDayInYear(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_yday;
}

uint32 Warhead::Time::GetMonth(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_mon;
}

uint32 Warhead::Time::GetYear(Seconds time /*= 0s*/)
{
    if (time == 0s)
    {
        time = GetEpochTime();
    }

    return TimeBreakdown(time.count()).tm_year;
}
