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

#include "Log.h"
#include "Config.h"
#include "Errors.h"
#include "FileUtil.h"
#include "StringConvert.h"
#include "Timer.h"
#include "Tokenize.h"
#include <filesystem>
#include <fmt/std.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <utility>

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <spdlog/details/windows_include.h>
#endif

namespace fs = std::filesystem;

namespace
{
    // Const loggers name
    constexpr auto LOGGER_ROOT = "root";
    constexpr auto LOGGER_GM = "commands.gm";
    //constexpr auto LOGGER_PLAYER_DUMP = "entities.player.dump";

    // Prefix's
    constexpr auto PREFIX_LOGGER = "Logger.";
    constexpr auto PREFIX_CHANNEL = "Sink.";
    constexpr auto PREFIX_LOGGER_LENGTH = 7;
    constexpr auto PREFIX_SINK_LENGTH = 5;

    constexpr auto LOG_TIMESTAMP_FMT = "%Y_%m_%d_%H_%M_%S";
}

Warhead::Log::Log()
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    SetConsoleOutputCP(65001);
#endif

    // Set pattern for default logger
    spdlog::set_pattern("[%t] %^%v%$");

    // Global flush policy
    spdlog::flush_every(2s);

    // Set warn level for default logger
    spdlog::set_level(spdlog::level::info);
}

Warhead::Log::~Log()
{
    Clear();
}

Warhead::Log* Warhead::Log::instance()
{
    static Log instance;
    return &instance;
}

void Warhead::Log::Clear()
{
    auto defaultLogger{ spdlog::default_logger() };

    spdlog::shutdown();
    _sinks.clear();

    spdlog::set_default_logger(defaultLogger);
}

void Warhead::Log::Initialize()
{
    _logsDir = sConfigMgr->GetOption<std::string>("LogsDir", "", false);
    Warhead::File::CorrectDirPath(_logsDir);
    ASSERT(Warhead::File::CreateDirIfNeed(_logsDir));
    LoadFromConfig();
}

void Warhead::Log::LoadFromConfig()
{
    _lowestLogLevel = spdlog::level::critical;

    Clear();
    ReadSinksFromConfig();
    ReadLoggersFromConfig();
}

bool Warhead::Log::ShouldLog(std::string_view filter, spdlog::level::level_enum level)
{
    // Don't even look for a logger if the LogLevel is higher than the highest log levels across all loggers
    if (level < _lowestLogLevel)
        return false;

    auto logger = GetLoggerByType(filter);
    if (!logger)
        return false;

    auto logLevel = logger->level();
    return logLevel != spdlog::level::off && logLevel <= level;
}

void Warhead::Log::ReadLoggersFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_LOGGER);
    if (keys.empty())
    {
        spdlog::error("Log::ReadLoggersFromConfig - Not found loggers, change config file!\n");
        return;
    }

    for (auto const& loggerName : keys)
        CreateLoggerFromConfig(loggerName);

    if (!GetLogger(LOGGER_ROOT))
        spdlog::error("Log::ReadLoggersFromConfig - Logger '{0}' not found!\nPlease change or add 'Logger.{0}' option in config file!", LOGGER_ROOT);
}

void Warhead::Log::ReadSinksFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_CHANNEL);
    if (keys.empty())
    {
        spdlog::error("Log::ReadSinksFromConfig - Not found channels, change config file!\n");
        return;
    }

    for (auto const& channelName : keys)
        CreateSinksFromConfig(channelName);
}

void Warhead::Log::CreateLoggerFromConfig(std::string_view configLoggerName)
{
    if (configLoggerName.empty())
        return;

    if (GetLogger(configLoggerName))
    {
        spdlog::error("Log::CreateLoggerFromConfig: {} is exist\n", configLoggerName);
        return;
    }

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{ configLoggerName }, "");
    auto loggerName = configLoggerName.substr(PREFIX_LOGGER_LENGTH);

    if (options.empty())
    {
        spdlog::error("Log::CreateLoggerFromConfig: Missing config option Logger.{}\n", loggerName);
        return;
    }

    auto const& tokens = Warhead::Tokenize(options, ',', true);
    if (tokens.size() != 2)
    {
        spdlog::error("Log::CreateLoggerFromConfig: Bad config options for Logger ({})\n", loggerName);
        return;
    }

    auto loggerLevel = Warhead::StringTo<int>(tokens[0]);
    if (!loggerLevel || *loggerLevel >= spdlog::level::n_levels)
    {
        spdlog::error("Log::CreateLoggerFromConfig: Wrong Log Level for logger {}\n", loggerName);
        return;
    }

    auto level = static_cast<spdlog::level::level_enum>(*loggerLevel);

    if (level < _lowestLogLevel)
        _lowestLogLevel = level;

    auto logger = std::make_shared<spdlog::logger>(std::string{ loggerName });
    logger->set_level(level);

    auto& sinks = logger->sinks();

    for (std::string_view sinkName : Warhead::Tokenize(tokens[1], ' ', false))
    {
        if (auto sink = GetSink(sinkName))
        {
            sinks.emplace_back(sink);
            continue;
        }

        spdlog::error("Error while configuring Sink '{}' in Logger {}. Sink does not exist", sinkName, loggerName);
    }

    // Set flush policy
    logger->flush_on(spdlog::level::warn);

    spdlog::register_logger(std::move(logger));
}

void Warhead::Log::CreateSinksFromConfig(std::string_view configSinkName)
{
    if (configSinkName.empty())
        return;

    if (GetSink(configSinkName))
    {
        spdlog::error("Log::CreateSinksFromConfig: {} is exist", configSinkName);
        return;
    }

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{configSinkName }, "");
    auto sinkName = configSinkName.substr(PREFIX_SINK_LENGTH);

    auto const& tokens = Warhead::Tokenize(options, ',', true);
    if (tokens.size() < 3 || tokens.size() > 7)
    {
        spdlog::error("Log::CreateSinksFromConfig: Wrong config option for {}", configSinkName);
        return;
    }

    auto sinkType = Warhead::StringTo<int8>(tokens[0]);
    if (!sinkType || sinkType >= MAX_SINK_TYPE)
    {
        spdlog::error("Log::CreateSinksFromConfig: Wrong sink type for {}", configSinkName);
        return;
    }

    auto type = static_cast<SinkType>(*sinkType);

    auto loggerLevel = Warhead::StringTo<uint8>(tokens[1]);
    if (!loggerLevel || *loggerLevel >= spdlog::level::n_levels)
    {
        spdlog::error("Log::CreateSinksFromConfig: Wrong Log Level for {}\n", configSinkName);
        return;
    }

    auto level = static_cast<spdlog::level::level_enum>(*loggerLevel);

    auto const& pattern = tokens[2];
    if (pattern.empty())
    {
        spdlog::error("Log::CreateSinksFromConfig: Empty pattern for {}\n", configSinkName);
        return;
    }

    spdlog::sink_ptr sink;

    if (type == SinkType::Console)
        sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
    else if (type == SinkType::File)
    {
        std::string fileName{ tokens[3] };
        bool truncate = false;

        if (tokens.size() >= 5)
            truncate = Warhead::StringTo<bool>(tokens[4]).value_or(false);

        if (tokens.size() >= 6 && Warhead::StringTo<bool>(tokens[5]).value_or(false))
        {
            auto timeStamp = "_" + Warhead::Time::TimeToTimestampStr(GetEpochTime(), LOG_TIMESTAMP_FMT);
            std::size_t dot_pos = fileName.find_last_of('.');
            dot_pos != std::string::npos ? fileName.insert(dot_pos, timeStamp) : fileName += timeStamp;
        }

        sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(_logsDir + std::string{ fileName }, truncate);
    }

    sink->set_level(level);
    sink->set_pattern(std::string{ pattern });

    AddSink(sinkName, std::move(sink));
}

spdlog::logger* Warhead::Log::GetLoggerByType(std::string_view type)
{
    if (auto logger = GetLogger(type))
        return logger;

    if (type == LOGGER_ROOT)
        return nullptr;

    std::string_view parentLogger{ LOGGER_ROOT };
    auto found = type.find_last_of('.');
    if (found != std::string_view::npos)
        parentLogger = type.substr(0, found);

    return GetLoggerByType(parentLogger);
}

spdlog::logger* Warhead::Log::GetLogger(std::string_view loggerName)
{
    if (auto logger = spdlog::get(std::string{ loggerName }))
        return logger.get();

    return nullptr;
}

void Warhead::Log::SetLoggerLevel(std::string_view loggerName, int level)
{
    auto spdlogLevel = static_cast<spdlog::level::level_enum>(level);
    if (spdlogLevel < spdlog::level::trace || spdlogLevel > spdlog::level::off)
    {
        spdlog::error("Trying set non defined log level ({}) to logger '{}'", level, loggerName);
        return;
    }

    auto logger = GetLogger(loggerName);
    if (!logger)
    {
        spdlog::error("Trying set log level ({}) to non existing logger '{}'", level, loggerName);
        return;
    }

    if (spdlogLevel != spdlog::level::off && spdlogLevel < _lowestLogLevel)
        _lowestLogLevel = spdlogLevel;

    logger->set_level(spdlogLevel);
}

spdlog::sink_ptr Warhead::Log::GetSink(std::string_view sinkName) const
{
    auto itr = _sinks.find(std::string{ sinkName });
    if (itr != _sinks.end())
        return itr->second;

    return nullptr;
}

void Warhead::Log::AddSink(std::string_view sinkName, spdlog::sink_ptr sink)
{
    if (GetSink(sinkName))
    {
        spdlog::error("Trying add existing sink with name: '{}'", sinkName);
        return;
    }

    _sinks.emplace(sinkName, std::move(sink));
}

void Warhead::Log::SetSinkLevel(std::string_view sinkName, int level)
{
    auto spdlogLevel = static_cast<spdlog::level::level_enum>(level);
    if (spdlogLevel < spdlog::level::trace || spdlogLevel > spdlog::level::off)
    {
        spdlog::error("Trying set non defined log level ({}) to sink '{}'", level, sinkName);
        return;
    }

    auto sink = GetSink(sinkName);
    if (!sink)
    {
        spdlog::error("Trying set log level ({}) to non existing sink '{}'", level, sinkName);
        return;
    }

    if (spdlogLevel != spdlog::level::off && spdlogLevel < _lowestLogLevel)
        _lowestLogLevel = spdlogLevel;

    sink->set_level(spdlogLevel);
}

void Warhead::Log::Write(std::string_view filter, spdlog::source_loc source, spdlog::level::level_enum level, std::string_view message)
{
    auto logger{ GetLoggerByType(filter) };
    if (!logger)
        return;

    logger->log(source, level, message);
}

void Warhead::Log::WriteCommand(uint32 /*accountID*/, std::string_view message)
{
    Write("commands.gm", {}, spdlog::level::info, message);
}