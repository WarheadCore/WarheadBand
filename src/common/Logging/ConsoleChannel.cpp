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

#include "ConsoleChannel.h"
#include "Exception.h"
#include "LogMessage.h"
#include "Tokenize.h"
#include "Util.h"
#include <iostream>

constexpr auto MAX_OPTIONS = 4;

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
Warhead::WindowsConsoleChannel::WindowsConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options) :
    Channel(ThisChannelType, name, level, pattern),
    _hConsole(INVALID_HANDLE_VALUE)
{
    if (options.size() > 4)
        throw Exception("Incorrect options count ({})", options.size());

    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // check whether the console has been redirected
    DWORD mode;
    _isFile = (GetConsoleMode(_hConsole, &mode) == 0);
    InitDefaultColors();

    if (options.size() > 3)
        SetColors(options[3]);
}

Warhead::WindowsConsoleChannel::WindowsConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::string_view colors) :
    Channel(ThisChannelType, name, level, pattern),
    _hConsole(INVALID_HANDLE_VALUE)
{
    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // check whether the console has been redirected
    DWORD mode;
    _isFile = (GetConsoleMode(_hConsole, &mode) == 0);
    InitDefaultColors();

    if (!colors.empty())
        SetColors(colors);
}

void Warhead::WindowsConsoleChannel::Write(LogMessage const& msg)
{
    if (msg.GetLevel() > GetLevel())
        return;

    std::string text{ msg.GetText() };

    // Replace text with pattern
    Format(msg, text);

    text += "\r\n";

    if (_enableColors && !_isFile)
    {
        WORD attr = _colors[0];
        attr &= 0xFFF0;
        attr |= _colors[static_cast<WORD>(msg.GetLevel())];
        SetConsoleTextAttribute(_hConsole, attr);
    }

    if (_isFile)
    {
        DWORD written;
        WriteFile(_hConsole, text.data(), static_cast<DWORD>(text.size()), &written, NULL);
    }
    else
    {
        std::wstring utext;
        Utf8toWStr(text, utext);
        DWORD written;
        WriteConsoleW(_hConsole, utext.data(), static_cast<DWORD>(utext.size()), &written, NULL);
    }

    if (_enableColors && !_isFile)
        SetConsoleTextAttribute(_hConsole, _colors[0]);
}

void Warhead::WindowsConsoleChannel::SetColors(std::string_view colors)
{
    auto colorTokens = Warhead::Tokenize(colors, ' ', true);
    if (colorTokens.size() != MAX_LOG_LEVEL - 1)
        throw Exception("WindowsConsoleChannel::SetColors: Invalid colors size. Size {}. Colors '{}'", colorTokens.size(), colors);

    std::size_t count{ 0 };

    for (auto const& color : colorTokens)
        _colors[++count] = ParseColor(color);
}

WORD Warhead::WindowsConsoleChannel::ParseColor(std::string_view color) const
{
    if (StringEqualI(color, "default"))
        return _colors[0];
    else if (StringEqualI(color, "black"))
        return CC_BLACK;
    else if (StringEqualI(color, "red"))
        return CC_RED;
    else if (StringEqualI(color, "green"))
        return CC_GREEN;
    else if (StringEqualI(color, "brown"))
        return CC_BROWN;
    else if (StringEqualI(color, "blue"))
        return CC_BLUE;
    else if (StringEqualI(color, "magenta"))
        return CC_MAGENTA;
    else if (StringEqualI(color, "cyan"))
        return CC_CYAN;
    else if (StringEqualI(color, "gray"))
        return CC_GRAY;
    else if (StringEqualI(color, "darkGray"))
        return CC_DARKGRAY;
    else if (StringEqualI(color, "lightRed"))
        return CC_LIGHTRED;
    else if (StringEqualI(color, "lightGreen"))
        return CC_LIGHTGREEN;
    else if (StringEqualI(color, "yellow"))
        return CC_YELLOW;
    else if (StringEqualI(color, "lightBlue"))
        return CC_LIGHTBLUE;
    else if (StringEqualI(color, "lightMagenta"))
        return CC_LIGHTMAGENTA;
    else if (StringEqualI(color, "lightCyan"))
        return CC_LIGHTCYAN;
    else if (StringEqualI(color, "white"))
        return CC_WHITE;
    else
        throw Exception("Invalid color value", color);
}

void Warhead::WindowsConsoleChannel::InitDefaultColors()
{
    if (!_isFile)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(_hConsole, &csbi);
        _colors[0] = csbi.wAttributes;
    }
    else
        _colors[0] = CC_WHITE;

    _colors[static_cast<std::size_t>(LogLevel::Fatal)]      = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Critical)]   = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Error)]      = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Warning)]    = CC_YELLOW;
    _colors[static_cast<std::size_t>(LogLevel::Info)]       = _colors[0];
    _colors[static_cast<std::size_t>(LogLevel::Debug)]      = CC_GRAY;
    _colors[static_cast<std::size_t>(LogLevel::Trace)]      = CC_GRAY;
}

#else // Unix console
constexpr auto CSI = "\033[";

Warhead::UnixConsoleChannel::UnixConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options) :
    Channel(ThisChannelType, name, level, pattern)
{
    if (options.size() > 4)
        throw Exception("Incorrect options count ({})", options.size());

    InitDefaultColors();

    if (options.size() > 3)
        SetColors(options[3]);
}

Warhead::UnixConsoleChannel::UnixConsoleChannel(std::string_view name, LogLevel level, std::string_view pattern, std::string_view colors) :
    Channel(ThisChannelType, name, level, pattern)
{
    InitDefaultColors();

    if (!colors.empty())
        SetColors(colors);
}

void Warhead::UnixConsoleChannel::Write(LogMessage const& msg)
{
    if (msg.GetLevel() > GetLevel())
        return;

    std::string text{ msg.GetText() };

    // Replace text with pattern
    Format(msg, text);

    std::lock_guard<std::mutex> guard(_mutex);

    if (_enableColors)
    {
        int32 color = _colors[static_cast<int32>(msg.GetLevel())];
        if (color & 0x100)
            std::cout << CSI << "1m";

        color &= 0xff;
        std::cout << CSI << color << "m";
    }

    std::cout << text;

    if (_enableColors)
        std::cout << CSI << "0m";

    std::cout << std::endl;
}

void Warhead::UnixConsoleChannel::SetColors(std::string_view colors)
{
    auto colorTokens = Warhead::Tokenize(colors, ' ', true);
    if (colorTokens.size() != MAX_LOG_LEVEL - 1)
        throw Exception("WindowsConsoleChannel::SetColors: Invalid colors size. Size {}. Colors '{}'", colorTokens.size(), colors);

    std::size_t count{ 0 };

    for (auto const& color : colorTokens)
        _colors[++count] = ParseColor(color);
}

Warhead::UnixConsoleChannel::Color Warhead::UnixConsoleChannel::ParseColor(std::string_view color) const
{
    if (StringEqualI(color, "default"))
        return CC_DEFAULT;
    else if (StringEqualI(color, "black"))
        return CC_BLACK;
    else if (StringEqualI(color, "red"))
        return CC_RED;
    else if (StringEqualI(color, "green"))
        return CC_GREEN;
    else if (StringEqualI(color, "brown"))
        return CC_BROWN;
    else if (StringEqualI(color, "blue"))
        return CC_BLUE;
    else if (StringEqualI(color, "magenta"))
        return CC_MAGENTA;
    else if (StringEqualI(color, "cyan"))
        return CC_CYAN;
    else if (StringEqualI(color, "gray"))
        return CC_GRAY;
    else if (StringEqualI(color, "darkGray"))
        return CC_DARKGRAY;
    else if (StringEqualI(color, "lightRed"))
        return CC_LIGHTRED;
    else if (StringEqualI(color, "lightGreen"))
        return CC_LIGHTGREEN;
    else if (StringEqualI(color, "yellow"))
        return CC_YELLOW;
    else if (StringEqualI(color, "lightBlue"))
        return CC_LIGHTBLUE;
    else if (StringEqualI(color, "lightMagenta"))
        return CC_LIGHTMAGENTA;
    else if (StringEqualI(color, "lightCyan"))
        return CC_LIGHTCYAN;
    else if (StringEqualI(color, "white"))
        return CC_WHITE;
    else
        throw Exception("Invalid color value", color);
}

void Warhead::UnixConsoleChannel::InitDefaultColors()
{
    _colors[static_cast<std::size_t>(LogLevel::Disabled)] = CC_DEFAULT;
    _colors[static_cast<std::size_t>(LogLevel::Fatal)] = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Critical)] = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Error)] = CC_LIGHTRED;
    _colors[static_cast<std::size_t>(LogLevel::Warning)] = CC_YELLOW;
    _colors[static_cast<std::size_t>(LogLevel::Info)] = CC_DEFAULT;
    _colors[static_cast<std::size_t>(LogLevel::Debug)] = CC_GRAY;
    _colors[static_cast<std::size_t>(LogLevel::Trace)] = CC_GRAY;
}
#endif
