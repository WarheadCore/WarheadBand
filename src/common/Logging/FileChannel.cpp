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

#include "FileChannel.h"
#include "Exception.h"
#include "Log.h"
#include "LogMessage.h"
#include "StringConvert.h"
#include "Util.h"
#include <filesystem>
#include <fstream>

constexpr auto MIN_OPTIONS = 4;

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
        auto timeStamp = "_" + Warhead::Time::TimeToTimestampStr(GetEpochTime(), "%Y-%m-%d_%H_%M_%S");

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
