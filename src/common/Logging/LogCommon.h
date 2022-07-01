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

#ifndef _WARHEAD_LOG_COMMON_H_
#define _WARHEAD_LOG_COMMON_H_

#include "Define.h"
#include <string_view>

namespace Warhead
{
    enum class LogLevel : int8
    {
        Disabled,
        Fatal,
        Critical,
        Error,
        Warning,
        Info,
        Debug,
        Trace,

        Max
    };

    constexpr std::string_view GetLogLevelName(LogLevel level)
    {
        switch (level)
        {
        case Warhead::LogLevel::Disabled:
            return "Disabled";
        case Warhead::LogLevel::Fatal:
            return "Fatal";
        case Warhead::LogLevel::Critical:
            return "Critical";
        case Warhead::LogLevel::Error:
            return "Error";
        case Warhead::LogLevel::Warning:
            return "Warning";
        case Warhead::LogLevel::Info:
            return "Info";
        case Warhead::LogLevel::Debug:
            return "Debug";
        case Warhead::LogLevel::Trace:
            return "Trace";
        case Warhead::LogLevel::Max:
            return "Max";
        default:
            return "Unknown";
        }
    }

    enum class ChannelType : int8
    {
        Console = 1,
        File,
        DB,
        Discord,

        Max
    };

    constexpr std::size_t MAX_LOG_LEVEL = static_cast<std::size_t>(LogLevel::Max);
    constexpr std::size_t MAX_CHANNEL_TYPE = static_cast<std::size_t>(ChannelType::Max);
    constexpr std::size_t MAX_CHANNEL_OPTIONS = 7;
    constexpr std::size_t LOGGER_OPTIONS = 2;

} // namespace Warhead

#endif //
