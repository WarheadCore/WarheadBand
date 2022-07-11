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

#ifndef _LOG_H
#define _LOG_H

#include "LogCommon.h"
#include <fmt/format.h>
#include <unordered_map>
#include <vector>

namespace Warhead
{
    class Logger;
    class Channel;
    class LogMessage;

    typedef std::shared_ptr<Channel>(*ChannelCreateFn)(std::string_view, LogLevel, std::string_view, std::vector<std::string_view> const&);

    template <class ChannelImpl>
    inline std::shared_ptr<Channel> CreateChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options)
    {
        return std::make_shared<ChannelImpl>(name, level, pattern, options);
    }

    class WH_COMMON_API Log
    {
    private:
        Log();
        ~Log();
        Log(Log const&) = delete;
        Log(Log&&) = delete;
        Log& operator=(Log const&) = delete;
        Log& operator=(Log&&) = delete;

    public:
        static Log* instance();

        void Initialize();
        void LoadFromConfig();

        void SetLoggerLevel(std::string_view name, LogLevel const level);
        void SetChannelLevel(std::string_view name, LogLevel const level);
        bool ShouldLog(std::string_view filter, LogLevel const level);

        template<typename... Args>
        inline void OutMessage(std::string_view filter, LogLevel const level, std::string_view file, std::size_t line, std::string_view function, std::string_view fmt, Args&&... args)
        {
            _OutMessage(filter, level, file, line, function, fmt::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        inline void OutCommand(uint32 account, std::string_view fmt, Args&&... args)
        {
            if (!ShouldLog("commands.gm", LogLevel::Info))
                return;

            _OutCommand(account, fmt::format(fmt, std::forward<Args>(args)...));
        }

        //void OutCharDump(std::string_view str, uint32 accountId, uint64 guid, std::string_view name);

        template<class ChannelImpl>
        void RegisterChannel()
        {
            RegisterChannel(ChannelImpl::ThisChannelType, CreateChannel<ChannelImpl>);
        }

        inline std::string_view GetLogsDir() { return _logsDir; }

        void UsingDefaultLogs(bool value = true);

    private:
        void _OutMessage(std::string_view filter, LogLevel level, std::string_view file, std::size_t line, std::string_view function, std::string_view message);
        void _OutCommand(uint32 accountID, std::string_view message);
        void Write(std::unique_ptr<LogMessage>&& msg);

        void CreateLoggerFromConfig(std::string_view configLoggerName);
        void CreateChannelsFromConfig(std::string_view logChannelName);
        void ReadLoggersFromConfig();
        void ReadChannelsFromConfig();

        void Clear();

        void RegisterChannel(ChannelType type, ChannelCreateFn channelCreateFn);

        Logger* GetLoggerByType(std::string_view type);
        Logger* HasLogger(std::string_view type);
        std::shared_ptr<Channel> HasChannel(std::string_view name);

        std::unordered_map<std::string, std::unique_ptr<Logger>> _loggers;
        std::unordered_map<std::string, std::shared_ptr<Channel>> _channels;
        std::unordered_map<int8, ChannelCreateFn> _channelsCreateFunction;

        LogLevel highestLogLevel;
        std::string _logsDir;

        //
        bool _isUseDefaultLogs{ false };
    };
}

#define sLog Warhead::Log::instance()

#define LOG_EXCEPTION_FREE(filterType__, level__, ...) \
    { \
        try \
        { \
            sLog->OutMessage(filterType__, level__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
        } \
        catch (const std::exception& e) \
        { \
            sLog->OutMessage("server", Warhead::LogLevel::Error, __FILE__, __LINE__, __FUNCTION__, "Wrong format occurred ({}) at '{}:{}'", \
                e.what(), __FILE__, __LINE__); \
        } \
    }

#define LOG_MSG_BODY(filterType__, level__, ...)                        \
        do {                                                            \
            if (sLog->ShouldLog(filterType__, level__))                 \
                LOG_EXCEPTION_FREE(filterType__, level__, __VA_ARGS__); \
        } while (0)

// Fatal - 1
#define LOG_FATAL(filterType__, ...) \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Fatal, __VA_ARGS__)

// Critical - 2
#define LOG_CRIT(filterType__, ...) \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Critical, __VA_ARGS__)

// Error - 3
#define LOG_ERROR(filterType__, ...) \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Error, __VA_ARGS__)

// Warning - 4
#define LOG_WARN(filterType__, ...)  \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Warning, __VA_ARGS__)

// Info - 5
#define LOG_INFO(filterType__, ...)  \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Info, __VA_ARGS__)

// Debug - 6
#define LOG_DEBUG(filterType__, ...) \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Debug, __VA_ARGS__)

// Trace - 7
#define LOG_TRACE(filterType__, ...) \
    LOG_MSG_BODY(filterType__, Warhead::LogLevel::Trace, __VA_ARGS__)

#define LOG_GM(accountId__, ...) \
    sLog->OutCommand(accountId__, __VA_ARGS__);

//#define LOG_CHAR_DUMP(message__, accountId__, playerGuid__, playerName__) \
//    sLog->OutCharDump(message__, accountId__, playerGuid__, playerName__);

#endif
