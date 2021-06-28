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

#ifndef _SYSTEM_LOG_H_
#define _SYSTEM_LOG_H_

#include "Common.h"
#include "StringFormat.h"

class WH_COMMON_API SystemLog
{
private:
    SystemLog();
    ~SystemLog() = default;
    SystemLog(SystemLog const&) = delete;
    SystemLog(SystemLog&&) = delete;
    SystemLog& operator=(SystemLog const&) = delete;
    SystemLog& operator=(SystemLog&&) = delete;

public:
    static SystemLog* instance();

    void InitSystemLogger();

    template<typename... Args>
    inline void outSys(uint8 logLevel, std::string_view fmt, Args&& ... args)
    {
        _outMessage(logLevel, Warhead::StringFormat(fmt, std::forward<Args>(args)...));
    }

private:
    void _outMessage(uint8 logLevel, std::string_view message);
};

#define sSysLog SystemLog::instance()

#define SYS_LOG_EXCEPTION_FREE(level__, ...) \
    { \
        try \
        { \
            sSysLog->outSys(level__, fmt::format(__VA_ARGS__)); \
        } \
        catch (const std::exception& e) \
        { \
            sSysLog->outSys(3, "Wrong format occurred ({}) at '{}:{}'", \
                e.what(), __FILE__, __LINE__); \
        } \
    }

// System Error level 3
#define SYS_LOG_ERROR(...) \
    SYS_LOG_EXCEPTION_FREE(3, __VA_ARGS__)

// System Info level 6
#define SYS_LOG_INFO(...) \
    SYS_LOG_EXCEPTION_FREE(6, __VA_ARGS__)

#endif
