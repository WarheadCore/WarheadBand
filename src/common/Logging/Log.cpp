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
#include "ConsoleChannel.h"
#include "Errors.h"
#include "Exception.h"
#include "FileChannel.h"
#include "FileUtil.h"
#include "Logger.h"
#include "LogMessage.h"
#include "StringConvert.h"
#include "Tokenize.h"

namespace
{
    // Const loggers name
    constexpr auto LOGGER_ROOT = "root";
    constexpr auto LOGGER_GM = "commands.gm";
    //constexpr auto LOGGER_PLAYER_DUMP = "entities.player.dump";

    // Prefix's
    constexpr auto PREFIX_LOGGER = "Logger.";
    constexpr auto PREFIX_CHANNEL = "LogChannel.";
    constexpr auto PREFIX_LOGGER_LENGTH = 7;
    constexpr auto PREFIX_CHANNEL_LENGTH = 11;
}

Warhead::Log::Log()
{
    RegisterChannel<ConsoleChannel>();
    RegisterChannel<FileChannel>();
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
    // Clear all loggers and channels
    _loggers.clear();
    _channels.clear();
}

void Warhead::Log::Initialize()
{
    if (_isUseDefaultLogs)
        return;

    LoadFromConfig();
}

void Warhead::Log::LoadFromConfig()
{
    highestLogLevel = LogLevel::Fatal;

    _logsDir = sConfigMgr->GetOption<std::string>("LogsDir", "", false);
    Warhead::File::CorrectDirPath(_logsDir);

    ASSERT(Warhead::File::CreateDirIfNeed(_logsDir));

    Clear();
    //InitLogsDir();
    ReadChannelsFromConfig();
    ReadLoggersFromConfig();
}

void Warhead::Log::ReadLoggersFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_LOGGER);
    if (keys.empty())
    {
        fmt::print("Log::ReadLoggersFromConfig - Not found loggers, change config file!\n");
        return;
    }

    for (auto const& loggerName : keys)
        CreateLoggerFromConfig(loggerName);

    if (!HasLogger(LOGGER_ROOT))
        fmt::print("Log::ReadLoggersFromConfig - Logger '{0}' not found!\nPlease change or add 'Logger.{0}' option in config file!\n", LOGGER_ROOT);
}

void Warhead::Log::ReadChannelsFromConfig()
{
    auto const& keys = sConfigMgr->GetKeysByString(PREFIX_CHANNEL);
    if (keys.empty())
    {
        fmt::print("Log::ReadChannelsFromConfig - Not found channels, change config file!\n");
        return;
    }

    for (auto const& channelName : keys)
        CreateChannelsFromConfig(channelName);
}

void Warhead::Log::CreateLoggerFromConfig(std::string_view configLoggerName)
{
    if (configLoggerName.empty())
        return;

    if (HasLogger(configLoggerName))
    {
        fmt::print("Log::CreateLoggerFromConfig: {} is exist\n", configLoggerName);
        return;
    }

    LogLevel level = LogLevel::Fatal;

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{ configLoggerName }, "");
    auto loggerName = configLoggerName.substr(PREFIX_LOGGER_LENGTH);

    if (options.empty())
    {
        fmt::print("Log::CreateLoggerFromConfig: Missing config option Logger.{}\n", loggerName);
        return;
    }

    auto const& tokens = Warhead::Tokenize(options, ',', true);
    if (tokens.size() != LOGGER_OPTIONS)
    {
        fmt::print("Log::CreateLoggerFromConfig: Bad config options for Logger ({})\n", loggerName);
        return;
    }

    auto loggerLevel = Warhead::StringTo<uint8>(tokens[0]);
    if (!loggerLevel || *loggerLevel >= MAX_LOG_LEVEL)
    {
        fmt::print("Log::CreateLoggerFromConfig: Wrong Log Level for logger {}\n", loggerName);
        return;
    }

    level = static_cast<LogLevel>(*loggerLevel);

    if (level > highestLogLevel)
        highestLogLevel = level;

    try
    {
        auto logger = std::make_unique<Logger>(loggerName, level);

        for (std::string_view channelName : Warhead::Tokenize(tokens[1], ' ', false))
        {
            if (auto channel = HasChannel(channelName))
            {
                logger->AddChannel(channel);
                continue;
            }

            fmt::print("Error while configuring Channel '{}' in Logger {}. Channel does not exist\n", channelName, loggerName);
        }

        _loggers.emplace(std::string{ loggerName }, std::move(logger));
    }
    catch (const Exception& e)
    {
        fmt::print("Log::CreateLoggerFromConfig: Error at create logger {}\n", e.GetErrorMessage());
    }
}

void Warhead::Log::CreateChannelsFromConfig(std::string_view logChannelName)
{
    if (logChannelName.empty())
        return;

    if (HasChannel(logChannelName))
    {
        fmt::print("Log::CreateChannelsFromConfig: {} is exist\n", logChannelName);
        return;
    }

    LogLevel level = LogLevel::Fatal;

    std::string const& options = sConfigMgr->GetOption<std::string>(std::string{ logChannelName }, "");
    auto channelName = logChannelName.substr(PREFIX_CHANNEL_LENGTH);

    auto const& tokens = Warhead::Tokenize(options, ',', true);
    if (tokens.size() < 3 || tokens.size() > MAX_CHANNEL_OPTIONS)
    {
        fmt::print("Log::CreateChannelsFromConfig: Wrong config option for {}\n", logChannelName);
        return;
    }

    auto channelType = Warhead::StringTo<int8>(tokens[0]);
    if (!channelType)
    {
        fmt::print("Log::CreateChannelsFromConfig: Wrong channel type for {}\n", logChannelName);
        return;
    }

    auto const& createFunctionItr = _channelsCreateFunction.find(*channelType);
    if (createFunctionItr == _channelsCreateFunction.end())
    {
        fmt::print(stderr, "Log::CreateChannelsFromConfig: Undefined type '{}' for {}\n", tokens[0], logChannelName);
        return;
    }

    ChannelType type = static_cast<ChannelType>(*channelType);

    auto loggerLevel = Warhead::StringTo<uint8>(tokens[1]);
    if (!loggerLevel || *loggerLevel >= MAX_LOG_LEVEL)
    {
        fmt::print("Log::CreateChannelsFromConfig: Wrong Log Level for {}\n", logChannelName);
        return;
    }

    level = static_cast<LogLevel>(*loggerLevel);

    auto const& pattern = tokens[2];
    if (pattern.empty())
    {
        fmt::print("Log::CreateChannelsFromConfig: Empty pattern for {}\n", logChannelName);
        return;
    }

    try
    {
        _channels.emplace(std::string{ channelName }, createFunctionItr->second(logChannelName, level, pattern, tokens));
    }
    catch (const Exception& e)
    {
        fmt::print("Log::CreateChannelsFromConfig: Error at create channel {}\n", e.GetErrorMessage());
    }
}

void Warhead::Log::UsingDefaultLogs(bool value /*= true*/)
{
    _isUseDefaultLogs = value;

    if (!_isUseDefaultLogs)
        return;

    // Clear all before create default
    Clear();

    highestLogLevel = LogLevel::Debug;

    try
    {
        auto consoleChannel = std::make_shared<ConsoleChannel>("Console", highestLogLevel, "[%H:%M:%S] %t", "lightRed lightRed red brown cyan lightMagenta green");
        _channels.emplace("Console", consoleChannel);

        auto rootLogger = std::make_unique<Logger>("root", highestLogLevel);
        rootLogger->AddChannel(consoleChannel);
        _loggers.emplace("root", std::move(rootLogger));
    }
    catch (const Exception& e)
    {
        fmt::print("Log::UsingDefaultLogs - {}\n", e.GetErrorMessage());
    }
}

Warhead::Logger* Warhead::Log::GetLoggerByType(std::string_view type)
{
    if (auto logger = HasLogger(type))
        return logger;

    if (type == LOGGER_ROOT)
        return nullptr;

    std::string_view parentLogger{ LOGGER_ROOT };
    size_t found = type.find_last_of('.');
    if (found != std::string_view::npos)
        parentLogger = type.substr(0, found);

    return GetLoggerByType(parentLogger);
}

Warhead::Logger* Warhead::Log::HasLogger(std::string_view type)
{
    auto const& itr = _loggers.find(std::string{ type });
    if (itr != _loggers.end())
        return itr->second.get();

    return nullptr;
}

std::shared_ptr<Warhead::Channel> Warhead::Log::HasChannel(std::string_view name)
{
    auto const& itr = _channels.find(std::string{ name });
    if (itr != _channels.end())
        return itr->second;

    return nullptr;
}

bool Warhead::Log::ShouldLog(std::string_view filter, LogLevel const level)
{
    // Don't even look for a logger if the LogLevel is higher than the highest log levels across all loggers
    if (level > highestLogLevel)
        return false;

    auto const& logger = GetLoggerByType(filter);
    if (!logger)
        return false;

    LogLevel logLevel = logger->GetLevel();
    return logLevel != LogLevel::Disabled && logLevel >= level;
}

void Warhead::Log::_OutMessage(std::string_view filter, LogLevel level, std::string_view file, std::size_t line, std::string_view function, std::string_view message)
{
    Write(std::make_unique<LogMessage>(filter, message, level, file, line, function));
}

void Warhead::Log::_OutCommand(uint32 accountID, std::string_view message)
{
    Write(std::make_unique<LogMessage>(LOGGER_GM, message, LogLevel::Info, Warhead::ToString(accountID)));
}

//void Warhead::Log::OutCharDump(std::string_view str, uint32 accountId, uint64 guid, std::string_view name)
//{
//    if (str.empty())
//        return;
//
//    Write(std::make_unique<LogMessage>(LOGGER_PLAYER_DUMP, Warhead::StringFormat("== START DUMP ==\n(Account: {}. Guid: {}. Name: {})\n{}\n== END DUMP ==\n", accountId, guid, name, str), LogLevel::Info));
//}

void Warhead::Log::Write(std::unique_ptr<LogMessage>&& msg)
{
    if (auto const& logger = GetLoggerByType(msg->GetSource()))
    {
        try
        {
            logger->Write(*msg);
        }
        catch (const Exception& e)
        {
            fmt::print("Error at write: {}", e.GetErrorMessage());
        }
    }
}

void Warhead::Log::RegisterChannel(ChannelType type, ChannelCreateFn channelCreateFn)
{
    ASSERT(type < ChannelType::Max);

    auto const& itr = _channelsCreateFunction.find(static_cast<uint8>(type));
    ASSERT(itr == _channelsCreateFunction.end());
    _channelsCreateFunction.emplace(static_cast<uint8>(type), channelCreateFn);
}

void Warhead::Log::SetLoggerLevel(std::string_view name, LogLevel const level)
{
    if (level >= LogLevel::Max)
        return;

    if (level > highestLogLevel)
        highestLogLevel = level;

    if (auto const& logger = HasLogger(name))
        logger->SetLevel(level);
}

void Warhead::Log::SetChannelLevel(std::string_view name, LogLevel const level)
{
    if (level >= LogLevel::Max)
        return;

    if (auto const& channel = HasChannel(name))
        channel->SetLevel(level);
}
