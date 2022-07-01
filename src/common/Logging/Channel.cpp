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

#include "Channel.h"
#include "Exception.h"
#include "LogMessage.h"
#include "StringConvert.h"
#include "Timer.h"
#include <filesystem>
#include <fmt/format.h>

Warhead::Channel::Channel(ChannelType type, std::string_view name, LogLevel level, std::string_view pattern /*= {}*/) :
    _type(type), _name(name), _level(level), _pattern(pattern)
{
    ParsePattern();
}

void Warhead::Channel::SetPattern(std::string_view pattern)
{
    if (pattern.empty())
        throw Exception("Pattern is empty");

    _pattern = pattern;
    ParsePattern();
}

void Warhead::Channel::ParsePattern()
{
    if (_pattern.empty())
        throw Exception("Pattern is empty");

    _patternActions.clear();
    std::string::const_iterator it = _pattern.begin();
    std::string::const_iterator end = _pattern.end();
    PatternAction endAct;

    while (it != end)
    {
        if (*it != '%')
        {
            endAct.prepend += *it++;
            continue;
        }

        if (++it != end)
        {
            PatternAction act;
            act.prepend = endAct.prepend;
            endAct.prepend.clear();

            if (*it == '[')
            {
                act.key = 'x';
                ++it;

                std::string prop;

                while (it != end && *it != ']')
                    prop += *it++;

                if (it == end)
                    --it;

                act.property = prop;
            }
            else
            {
                act.key = *it;

                if ((it + 1) != end && *(it + 1) == '[')
                {
                    it += 2;

                    std::string number;

                    while (it != end && *it != ']')
                        number += *it++;

                    if (it == end)
                        --it;

                    auto lenght = Warhead::StringTo<std::size_t>(number);
                    if (!lenght)
                        throw Exception("ParsePattern: Error at conver lenght from {}", number);

                    act.length = *lenght;
                }
            }

            _patternActions.emplace_back(act);
            ++it;
        }
    }

    if (endAct.prepend.size())
        _patternActions.emplace_back(endAct);

    if (_patternActions.empty())
        throw Exception("ParsePattern: Error at parse pattern '{}'", _pattern);
}

void Warhead::Channel::Format(LogMessage const& msg, std::string& text)
{
    if (_patternActions.empty())
        return;

    text.clear();

    auto epochTime = std::chrono::duration_cast<Seconds>(msg.GetTime().time_since_epoch());

    for (auto const& pa : _patternActions)
    {
        text.append(pa.prepend);

        switch (pa.key)
        {
            case 's': text.append(msg.GetSource()); break;
            case 't': text.append(msg.GetText()); break;
            case 'l': text.append(Warhead::ToString(static_cast<int8>(msg.GetLevel()))); break;
            case 'p': text.append(GetLogLevelName(msg.GetLevel())); break;
            case 'q': text += GetLogLevelName(msg.GetLevel()).at(0); break;
                //case 'P': NumberFormatter::append(text, msg.getPid()); break;
                //case 'T': text.append(msg.getThread()); break;
                //case 'I': NumberFormatter::append(text, msg.getTid()); break;
                //case 'N': text.append(Environment::nodeName()); break;
            case 'U':
            {
                auto srcFile = msg.GetSourceFile();
                if (srcFile.empty())
                    continue;

                text.append(srcFile);
                break;
            }
            case 'O':
            {
                auto srcFile = msg.GetSourceFile();
                if (srcFile.empty())
                    continue;

                std::filesystem::path path{ srcFile };
                text.append(path.filename().generic_string());
                break;
            }
            case 'u': text.append(Warhead::ToString(msg.GetSourceLine())); break;
            case 'w': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%a")); break;
            case 'W': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%A")); break;
            case 'b': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%b")); break;
            case 'B': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%B")); break;
            case 'd': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%d")); break;
            case 'm': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%m")); break;
            case 'n': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%Om")); break;
            case 'y': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%y")); break;
            case 'Y': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%Y")); break;
            case 'h': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%OH")); break;
            case 'H': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%OI")); break;
            case 'A': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%p")); break;
            case 'M': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%OM")); break;
            case 'S': text.append(Warhead::Time::TimeToTimestampStr(epochTime, "%OS")); break;
            case 'E': text.append(fmt::format("{}", epochTime.count())); break;
            case 'v':
            {
                if (pa.length > msg.GetSource().length())	//append spaces
                    text.append(msg.GetSource()).append(pa.length - msg.GetSource().length(), ' ');
                else if (pa.length && pa.length < msg.GetSource().length()) // crop
                    text.append(msg.GetSource(), msg.GetSource().length() - pa.length, pa.length);
                else
                    text.append(msg.GetSource());
                break;
            }
        }
    }
}
