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

#ifndef WARHEAD_TIMER_H
#define WARHEAD_TIMER_H

#include "Common.h"
#include "Duration.h"

enum class TimeFormat : uint8
{
    FullText,       // 1 Days 2 Hours 3 Minutes 4 Seconds 5 Milliseconds
    ShortText,      // 1d 2h 3m 4s 5ms
    Numeric         // 1:2:3:4:5
};

enum class TimeOutput : uint8
{
    Days,           // 1d
    Hours,          // 1d 2h
    Minutes,        // 1d 2h 3m
    Seconds,        // 1d 2h 3m 4s
    Milliseconds    // 1d 2h 3m 4s 5ms
};

namespace Warhead::Time
{
    template<class T>
    WH_COMMON_API uint32 TimeStringTo(std::string_view timestring);

    template<class T>
    WH_COMMON_API std::string ToTimeString(uint64 durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);

    template<class T>
    WH_COMMON_API std::string ToTimeString(std::string_view durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);

    WH_COMMON_API std::string ToTimeString(Microseconds durationTime, TimeOutput timeOutput = TimeOutput::Seconds, TimeFormat timeFormat = TimeFormat::ShortText);

    WH_COMMON_API time_t LocalTimeToUTCTime(time_t time);
    WH_COMMON_API time_t GetLocalHourTimestamp(time_t time, uint8 hour, bool onlyAfterTime = true);
    WH_COMMON_API tm TimeBreakdown(time_t t);
    WH_COMMON_API std::string TimeToTimestampStr(time_t t);
    WH_COMMON_API std::string TimeToHumanReadable(time_t t);
}

WH_COMMON_API struct tm* localtime_r(time_t const* time, struct tm* result);

inline TimePoint GetApplicationStartTime()
{
    static const TimePoint ApplicationStartTime = std::chrono::steady_clock::now();

    return ApplicationStartTime;
}

inline uint32 getMSTime()
{
    using namespace std::chrono;

    return uint32(duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime()).count());
}

inline uint32 getMSTimeDiff(uint32 oldMSTime, uint32 newMSTime)
{
    // getMSTime() have limited data range and this is case when it overflow in this tick
    if (oldMSTime > newMSTime)
        return (0xFFFFFFFF - oldMSTime) + newMSTime;
    else
        return newMSTime - oldMSTime;
}

inline uint32 getMSTimeDiff(uint32 oldMSTime, TimePoint newTime)
{
    using namespace std::chrono;

    uint32 newMSTime = uint32(duration_cast<milliseconds>(newTime - GetApplicationStartTime()).count());
    return getMSTimeDiff(oldMSTime, newMSTime);
}

inline uint32 GetMSTimeDiffToNow(uint32 oldMSTime)
{
    return getMSTimeDiff(oldMSTime, getMSTime());
}

struct IntervalTimer
{
public:
    IntervalTimer()

    {
    }

    void Update(time_t diff)
    {
        _current += diff;
        if (_current < 0)
            _current = 0;
    }

    bool Passed()
    {
        return _current >= _interval;
    }

    void Reset()
    {
        if (_current >= _interval)
            _current %= _interval;
    }

    void SetCurrent(time_t current)
    {
        _current = current;
    }

    void SetInterval(time_t interval)
    {
        _interval = interval;
    }

    [[nodiscard]] time_t GetInterval() const
    {
        return _interval;
    }

    [[nodiscard]] time_t GetCurrent() const
    {
        return _current;
    }

private:
    time_t _interval{0};
    time_t _current{0};
};

struct TimeTracker
{
public:
    TimeTracker(time_t expiry)
        : i_expiryTime(expiry)
    {
    }

    void Update(time_t diff)
    {
        i_expiryTime -= diff;
    }

    [[nodiscard]] bool Passed() const
    {
        return i_expiryTime <= 0;
    }

    void Reset(time_t interval)
    {
        i_expiryTime = interval;
    }

    [[nodiscard]] time_t GetExpiry() const
    {
        return i_expiryTime;
    }

private:
    time_t i_expiryTime;
};

struct TimeTrackerSmall
{
public:
    TimeTrackerSmall(uint32 expiry = 0)
        : i_expiryTime(expiry)
    {
    }

    void Update(int32 diff)
    {
        i_expiryTime -= diff;
    }

    [[nodiscard]] bool Passed() const
    {
        return i_expiryTime <= 0;
    }

    void Reset(uint32 interval)
    {
        i_expiryTime = interval;
    }

    [[nodiscard]] int32 GetExpiry() const
    {
        return i_expiryTime;
    }

private:
    int32 i_expiryTime;
};

struct PeriodicTimer
{
public:
    PeriodicTimer(int32 period, int32 start_time)
        : i_period(period), i_expireTime(start_time)
    {
    }

    bool Update(const uint32 diff)
    {
        if ((i_expireTime -= diff) > 0)
            return false;

        i_expireTime += i_period > int32(diff) ? i_period : diff;
        return true;
    }

    void SetPeriodic(int32 period, int32 start_time)
    {
        i_expireTime = start_time;
        i_period = period;
    }

    // Tracker interface
    void TUpdate(int32 diff) { i_expireTime -= diff; }
    [[nodiscard]] bool TPassed() const { return i_expireTime <= 0; }
    void TReset(int32 diff, int32 period)  { i_expireTime += period > diff ? period : diff; }

private:
    int32 i_period;
    int32 i_expireTime;
};

#endif
