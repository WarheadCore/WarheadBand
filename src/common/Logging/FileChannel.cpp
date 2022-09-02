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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "FileChannel.h"
#include "Exception.h"
#include "Log.h"
#include "LogMessage.h"
#include "StringConvert.h"
#include "Timer.h"
#include "Util.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <range/v3/action/sort.hpp>

constexpr auto MIN_OPTIONS = 4;
constexpr auto LOG_TIMESTAMP_FMT = "%Y_%m_%d_%H_%M_%S";

namespace fs = std::filesystem;

using LogFileInfo = std::pair<fs::path, Seconds>;

namespace
{
    Seconds GetFileCreationDate(std::string_view fmt, std::string_view str)
    {
        if (fmt.empty() || str.empty())
            return 0s;

#define PARSE_NUMBER_N(var, n) \
        { int i = 0; while (i++ < n && it != end && std::isdigit(*it)) var = var*10 + ((*it++) - '0'); }

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;

        auto it = str.begin();
        auto end = str.end();
        auto itf = fmt.begin();
        auto endf = fmt.end();

        while (itf != endf && it != end)
        {
            if (*itf == '%')
            {
                if (++itf != endf)
                {
                    switch (*itf)
                    {
                    case 'Y':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(year, 4);
                        break;
                    case 'm':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(month, 2);
                        break;
                    case 'd':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(day, 2);
                        break;
                    case 'H':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(hour, 2);
                        break;
                    case 'M':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(minute, 2);
                        break;
                    case 'S':
                        while (it != end && !std::isdigit(*it)) ++it;
                        PARSE_NUMBER_N(second, 2);
                        break;
                    }
                    ++itf;
                }
            }
            else
                ++itf;
        }

        if (month == 0)
            month = 1;

        if (day == 0)
            day = 1;

#undef PARSE_NUMBER_N

        struct tm t{ 0 };
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;

        return Seconds(mktime(&t));
    }
}

Warhead::FileChannel::FileChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options) :
    Channel(ThisChannelType, name, level, pattern),
    _logsDir(sLog->GetLogsDir())
{
    if (options.size() < MIN_OPTIONS)
        throw Exception("Incorrect options count ({})", options.size());

    _fileName = options[3];
    _isDynamicFileName = _fileName.find("{}") != std::string::npos;

    if (options.size() >= 5)
    {
        auto isAppendmode = Warhead::StringTo<bool>(options[4]);
        if (!isAppendmode)
            throw Exception("Incorrect option for open mode '{}'", options[4]);

        _isOpenModeAppend = *isAppendmode;
    }

    if (options.size() >= 6)
    {
        auto isFlush = Warhead::StringTo<bool>(options[5]);
        if (!isFlush)
            throw Exception("Incorrect option for flush '{}'", options[5]);

        _isFlush = *isFlush;
    }

    if (options.size() >= 7)
    {
        auto isAddTimestamp = Warhead::StringTo<bool>(options[6]);
        if (!isAddTimestamp)
            throw Exception("Incorrect option for add timestamp '{}'", options[6]);

        _isAddTimestamp = *isAddTimestamp;

        if (options.size() >= 8)
        {
            auto maxCount = Warhead::StringTo<uint32>(options[7]);
            if (!maxCount)
                throw Exception("Incorrect option for max count files '{}'", options[7]);

            _maxCount = *maxCount;

            if (!_maxCount)
                throw Exception("Incorrect option for max count files '{}'. Min 1", options[7]);
        }

        if (options.size() >= 9)
        {
            auto purgeAge = Warhead::StringTo<uint32>(options[8]);
            if (!purgeAge)
                throw Exception("Incorrect option for purge age '{}'", options[8]);

            _purgeAge = *purgeAge;
        }

        ClearOldFiles();
    }
}

Warhead::FileChannel::~FileChannel()
{
    CloseFile();
}

void Warhead::FileChannel::Write(LogMessage const& msg)
{
    if (_isDynamicFileName && msg.GetOption().empty())
        throw Exception("Empty option for dynamic file '{}'", _fileName);

    std::lock_guard<std::mutex> guard(_mutex);

    std::string text{ msg.GetText() };

    // Replace text with pattern
    Format(msg, text);

    if (_isDynamicFileName)
    {
        // Get name for dymamic file
        _fileName = fmt::format(_fileName, msg.GetOption());

        if (!OpenFile())
            throw Exception("Cannot open file '{}'", _fileName);

        *_logFile << text << std::endl;
        CloseFile();
        return;
    }

    if (!OpenFile())
        throw Exception("Cannot open file '{}'", _fileName);

    *_logFile << text;
    _isFlush ? *_logFile << std::endl : *_logFile << "\n";

    if (!_logFile->good())
        throw Exception("Incorrect write to file '{}'", _fileName);
}

bool Warhead::FileChannel::OpenFile()
{
    if (_logFile)
        return _logFile->is_open();

    std::ios::openmode openMode = std::ios::out;
    if (_isOpenModeAppend)
        openMode |= std::ios_base::app;

    if (_isAddTimestamp)
    {
        auto timeStamp = "_" + Warhead::Time::TimeToTimestampStr(GetEpochTime(), LOG_TIMESTAMP_FMT);

        size_t dot_pos = _fileName.find_last_of('.');
        dot_pos != std::string::npos ? _fileName.insert(dot_pos, timeStamp) : _fileName += timeStamp;
    }

    _logFile = std::make_unique<std::ofstream>(_logsDir + _fileName, openMode);
    return _logFile->is_open();
}

void Warhead::FileChannel::CloseFile()
{
    if (!_logFile)
        return;

    _logFile->close();
    _logFile.reset();
}

void Warhead::FileChannel::ClearOldFiles()
{
    // Avoid crash
    if (_logsDir.empty())
        return;

    auto deleteBeforeTime{ GetEpochTime() - Days(_purgeAge) };

    std::string fileNameFmt{ _fileName };
    std::string timestampRe{ "_[0-9-]+[0-9_]+" };

    size_t dot_pos = fileNameFmt.find_last_of('.');
    dot_pos != std::string::npos ? fileNameFmt.insert(dot_pos, timestampRe) : fileNameFmt += timestampRe;

    std::regex const regex(Warhead::StringFormat("^{}$", fileNameFmt));

    std::vector<LogFileInfo> logFiles;

    for (auto const& dirEntry : fs::directory_iterator{ fs::path{ _logsDir } })
    {
        auto const& filePath = dirEntry.path();
        auto const& fileName = filePath.filename().generic_string();

        if (std::regex_match(fileName, regex))
            logFiles.emplace_back(filePath, GetFileCreationDate(LOG_TIMESTAMP_FMT, fileName));
    }

    ranges::sort(logFiles, [](LogFileInfo const& itr1, LogFileInfo const& itr2)
    {
        return itr1.second < itr2.second;
    });

    std::size_t filesCount{ logFiles.size() };

    for (auto const& [filePath, createDate] : logFiles)
    {
        if (filesCount > _maxCount)
        {
            fs::remove(filePath);
            filesCount--;
        }
        else if (createDate < deleteBeforeTime)
            fs::remove(filePath);
    }
}
