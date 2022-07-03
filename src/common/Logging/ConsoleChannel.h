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

#ifndef _WARHEAD_CONSOLE_CHANNEL_H_
#define _WARHEAD_CONSOLE_CHANNEL_H_

#include "Channel.h"

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <mutex>
#endif

namespace Warhead
{
#if WARHEAD_PLATFORM == WH_PLATFORM_WINDOWS
    class WH_COMMON_API WindowsConsoleChannel : public Channel
    {
    public:
        static constexpr auto ThisChannelType{ ChannelType::Console };

        WindowsConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options);
        WindowsConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::string_view colors);
        virtual ~WindowsConsoleChannel() = default;

        void Write(LogMessage const& msg) override;

        inline void EnableColors(bool val = true) { _enableColors = val; }
        void SetColors(std::string_view colors);

    private:
        enum Color
        {
            CC_BLACK        = 0x0000,
            CC_RED          = 0x0004,
            CC_GREEN        = 0x0002,
            CC_BROWN        = 0x0006,
            CC_BLUE         = 0x0001,
            CC_MAGENTA      = 0x0005,
            CC_CYAN         = 0x0003,
            CC_GRAY         = 0x0007,
            CC_DARKGRAY     = 0x0008,
            CC_LIGHTRED     = 0x000C,
            CC_LIGHTGREEN   = 0x000A,
            CC_YELLOW       = 0x000E,
            CC_LIGHTBLUE    = 0x0009,
            CC_LIGHTMAGENTA = 0x000D,
            CC_LIGHTCYAN    = 0x000B,
            CC_WHITE        = 0x000F
        };

        void InitDefaultColors();
        WORD ParseColor(std::string_view color) const;

        bool _enableColors{ true };
        HANDLE _hConsole;
        bool   _isFile{ false };
        WORD   _colors[MAX_LOG_LEVEL];
    };
#else
    class WH_COMMON_API UnixConsoleChannel : public Channel
    {
    public:
        static constexpr auto ThisChannelType{ ChannelType::Console };

        UnixConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options);
        UnixConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::string_view colors);
        virtual ~UnixConsoleChannel() = default;

        void Write(LogMessage const& msg) override;

        inline void EnableColors(bool val = true) { _enableColors = val; }
        void SetColors(std::string_view colors);

    private:
        enum Color
        {
            CC_DEFAULT      = 0x0027,
            CC_BLACK        = 0x001e,
            CC_RED          = 0x001f,
            CC_GREEN        = 0x0020,
            CC_BROWN        = 0x0021,
            CC_BLUE         = 0x0022,
            CC_MAGENTA      = 0x0023,
            CC_CYAN         = 0x0024,
            CC_GRAY         = 0x0025,
            CC_DARKGRAY     = 0x011e,
            CC_LIGHTRED     = 0x011f,
            CC_LIGHTGREEN   = 0x0120,
            CC_YELLOW       = 0x0121,
            CC_LIGHTBLUE    = 0x0122,
            CC_LIGHTMAGENTA = 0x0123,
            CC_LIGHTCYAN    = 0x0124,
            CC_WHITE        = 0x0125
        };

        void InitDefaultColors();
        Color ParseColor(std::string_view color) const;

        bool _enableColors{ true };
        Color  _colors[MAX_LOG_LEVEL];
        std::mutex _mutex;
    };
#endif

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    using ConsoleChannel = WindowsConsoleChannel;
#else
    using ConsoleChannel = UnixConsoleChannel;
#endif

} // namespace Warhead

#endif //
