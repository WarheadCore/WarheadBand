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

#ifndef _WARHEAD_MESSAGE_H_
#define _WARHEAD_MESSAGE_H_

#include "LogCommon.h"
#include "Timer.h"
#include <memory>

namespace Warhead
{
    class WH_COMMON_API LogMessage
    {
    public:
        using MessageLevel = LogLevel;

        LogMessage(std::string_view source, std::string_view text, MessageLevel level, std::string_view option = {});
        LogMessage(std::string_view source, std::string_view text, MessageLevel level, std::string_view file, std::size_t line, std::string_view function, std::string_view option = {});

        ~LogMessage() = default;

        inline void SetSource(std::string_view src) { _source = src; }
        inline std::string_view GetSource() const { return _source; }

        inline void SetText(std::string_view text) { _text = text; }
        inline std::string_view GetText() const { return _text; }

        inline void SetLevel(MessageLevel level) { _level = level; };
        inline MessageLevel GetLevel() const { return _level; }

        inline void SetTime(SystemTimePoint time) { _time = time; }
        inline SystemTimePoint GetTime() const { return _time; }

        inline void SetSourceFile(std::string_view file) { _file = file; }
        inline std::string_view GetSourceFile() const { return _file; }

        inline void SetSourceLine(std::size_t line) { _line = line; }
        inline std::size_t GetSourceLine() const { return _line; }

        inline void SetOption(std::string_view option) { _option = option; }
        inline std::string_view GetOption() const { return _option; }

    private:
        std::string _source;
        std::string _text;
        MessageLevel _level{ MessageLevel::Fatal };
        SystemTimePoint _time{ std::chrono::system_clock::now() };

        std::string _file;
        std::size_t _line{ 0 };
        std::string _function;
        std::string _option;
    };
}

#endif
