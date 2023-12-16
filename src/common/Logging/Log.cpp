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
#include "Containers.h"
#include "Tokenize.h"
#include <filesystem>
#include <utility>

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <spdlog/details/windows_include.h>
#endif

// Quill
#include <quill/Quill.h>
#include <quill/Logger.h>

namespace
{
    // Const loggers name
    constexpr auto LOGGER_ROOT = "root";

    // Prefix's
    constexpr auto PREFIX_LOGGER = "Logger.";
    constexpr auto PREFIX_HANDLER = "Handler.";
}

namespace fs = std::filesystem;

Warhead::Log::Log()
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    SetConsoleOutputCP(65001);
#endif

    // Start the logging backend thread
    quill::start();
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
   //
}

void Warhead::Log::Initialize()
{
    _logsDir = sConfigMgr->GetOption<std::string>("LogsDir", "", false);
    File::CorrectDirPath(_logsDir);
    ASSERT(File::CreateDirIfNeed(_logsDir));
    LoadFromConfig();
}

void Warhead::Log::LoadFromConfig()
{
    _lowestLogLevel = quill::LogLevel::Critical;

    Clear();
    ReadSinksFromConfig();
    ReadLoggersFromConfig();
}

bool Warhead::Log::ShouldLog(std::string_view filter, quill::LogLevel level)
{
    // Don't even look for a logger if the LogLevel is higher than the highest log levels across all loggers
    if (level < _lowestLogLevel)
        return false;

    auto logger = GetLoggerByType(filter);
    if (!logger)
        return false;

    auto logLevel = logger->log_level();
    return logLevel != quill::LogLevel::None && logLevel <= level;
}

void Warhead::Log::ReadLoggersFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_LOGGER);
    if (keys.empty())
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::ReadLoggersFromConfig - Not found loggers, change config file!");
        return;
    }

    for (auto const& loggerName : keys)
        CreateLoggerFromConfig(loggerName);

    if (!GetLogger(LOGGER_ROOT))
        QUILL_LOG_ERROR(quill::get_logger(), "Log::ReadLoggersFromConfig - Logger '{0}' not found!\nPlease change or add 'Logger.{0}' option in config file!", LOGGER_ROOT);
}

void Warhead::Log::ReadSinksFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_HANDLER);
    if (keys.empty())
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::ReadSinksFromConfig - Not found channels, change config file!");
        return;
    }

    for (auto const& channelName : keys)
        CreateHandlersFromConfig(channelName);
}

quill::Logger* Warhead::Log::GetQuillLogger(std::string const& loggerName) const
{
    std::lock_guard const lock{_rmutex};
    return Containers::MapGetValuePtr(_loggers, loggerName);
}

void Warhead::Log::CreateLoggerFromConfig(std::string_view configLoggerName)
{
    if (configLoggerName.empty())
        return;

    if (GetLogger(configLoggerName))
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateLoggerFromConfig: {} is exist", configLoggerName);
        return;
    }

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{ configLoggerName }, "");
    auto loggerName = configLoggerName.substr(strlen(PREFIX_LOGGER));

    if (options.empty())
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateLoggerFromConfig: Missing config option Logger.{}", loggerName);
        return;
    }

    auto const& tokens = Warhead::Tokenize(options, ',', true);
    if (tokens.size() != 2)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateLoggerFromConfig: Bad config options for Logger ({})", loggerName);
        return;
    }

    auto loggerLevel = Warhead::StringTo<int>(tokens[0]);
    if (!loggerLevel || *loggerLevel > static_cast<int>(quill::LogLevel::None))
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateLoggerFromConfig: Wrong log level for logger {}", loggerName);
        return;
    }

    auto level = static_cast<quill::LogLevel>(*loggerLevel);
    if (level < _lowestLogLevel)
        _lowestLogLevel = level;

    std::vector<std::shared_ptr<quill::Handler>> handlers;

    for (std::string_view handlerName : Warhead::Tokenize(tokens[1], ' ', false))
    {
        if (auto handler = GetHandler(std::string{ handlerName }))
        {
            handlers.emplace_back(handler);
            continue;
        }

        QUILL_LOG_ERROR(quill::get_logger(), "Error while configuring Handler '{}' in Logger {}. Handler does not exist", handlerName, loggerName);
    }

    auto logger = quill::create_logger(std::string{ loggerName }, std::move(handlers));
    logger->set_log_level(level);

    _loggers.emplace(loggerName, logger);
}

void Warhead::Log::CreateHandlersFromConfig(std::string_view configHandlerName)
{
    if (configHandlerName.empty())
        return;

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{configHandlerName }, "");
    auto handlerName = configHandlerName.substr(strlen(PREFIX_HANDLER));

    if (GetHandler(std::string{ handlerName }))
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateHandlersFromConfig: {} is exist", configHandlerName);
        return;
    }

    auto const& tokens = Tokenize(options, ',', true);
    if (tokens.size() < 3 || tokens.size() > 7)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateHandlersFromConfig: Wrong config option for {}", configHandlerName);
        return;
    }

    auto handlerType = Warhead::StringTo<int8>(tokens[0]);
    if (!handlerType || handlerType >= MAX_HANDLER_TYPE)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateHandlersFromConfig: Wrong sink type for {}", configHandlerName);
        return;
    }

    auto const type = static_cast<HandlerType>(*handlerType);

    auto loggerLevel = Warhead::StringTo<uint8>(tokens[1]);
    if (!loggerLevel || *loggerLevel > static_cast<int>(quill::LogLevel::None))
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateHandlersFromConfig: Wrong Log Level for {}", configHandlerName);
        return;
    }

    auto level = static_cast<quill::LogLevel>(*loggerLevel);

    auto const& pattern = tokens[2];
    if (pattern.empty())
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Log::CreateHandlersFromConfig: Empty pattern for {}", configHandlerName);
        return;
    }

    std::shared_ptr<quill::Handler> handler;

    if (type == HandlerType::Console)
    {
        // Create a ConsoleColours class that we will pass to `quill::stdout_handler(...)`
        quill::ConsoleColours consoleHandler;

        // Enable using default colours to get the default colour scheme
        consoleHandler.set_default_colours();

        // Make color console handler
        handler = quill::stdout_handler(std::string{ handlerName }, consoleHandler);
    }
    else if (type == HandlerType::File)
    {
        std::string const fileName{ tokens[3] };

        quill::RotatingFileHandlerConfig cfg;
        cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);

        if (tokens.size() >= 5)
            if (auto maxFiles = StringTo<uint32>(tokens[4]))
                cfg.set_max_backup_files(*maxFiles ? *maxFiles : std::numeric_limits<uint32_t>::max());

        if (tokens.size() >= 6)
            cfg.set_rotation_time_daily(tokens[5].data());

        handler = quill::rotating_file_handler(fileName, cfg);
    }

    handler->set_log_level(level);
    handler->set_pattern(std::string{ pattern });

    AddHandler(std::string{ handlerName }, handler.get());
}

quill::Logger* Warhead::Log::GetLoggerByType(std::string_view type)
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

quill::Logger* Warhead::Log::GetLogger(std::string_view loggerName) const
{
    return GetQuillLogger(std::string{ loggerName });
}

void Warhead::Log::SetLoggerLevel(std::string_view loggerName, int level)
{
    auto logLevel = static_cast<quill::LogLevel>(level);
    if (logLevel < quill::LogLevel::TraceL3 || logLevel >= quill::LogLevel::None)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Trying set non defined log level ({}) to logger '{}'", level, loggerName);
        return;
    }

    auto logger = GetLogger(loggerName);
    if (!logger)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Trying set log level ({}) to non existing logger '{}'", level, loggerName);
        return;
    }

    if (logLevel != quill::LogLevel::None && logLevel < _lowestLogLevel)
        _lowestLogLevel = logLevel;

    logger->set_log_level(logLevel);
}

void Warhead::Log::AddHandler(std::string const& name, quill::Handler* handler)
{
    _handlers.emplace(name, handler);
}

std::shared_ptr<quill::Handler> Warhead::Log::GetHandler(std::string const& name)
{
    auto const itr = _handlers.find(name);
    if (itr == _handlers.end())
        return nullptr;

    return itr->second;
}

void Warhead::Log::SetHandlerLevel(std::string_view handlerName, int level)
{
    auto logLevel = static_cast<quill::LogLevel>(level);
    if (logLevel < quill::LogLevel::TraceL3 || logLevel >= quill::LogLevel::None)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Trying set non defined log level ({}) to handler '{}'", level, handlerName);
        return;
    }

    auto handler = GetHandler(handlerName.data());
    if (!handler)
    {
        QUILL_LOG_ERROR(quill::get_logger(), "Trying set log level ({}) to non existing handler '{}'", level, handlerName);
        return;
    }

    if (logLevel != quill::LogLevel::None && logLevel < _lowestLogLevel)
        _lowestLogLevel = logLevel;

    handler->set_log_level(logLevel);
}
