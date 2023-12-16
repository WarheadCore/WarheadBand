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

#ifndef WARHEAD_LOG_H_
#define WARHEAD_LOG_H_

#include "StringFormat.h"
#include <quill/detail/LogMacros.h>
#include <mutex>
#include <unordered_map>

namespace quill
{
    class Handler;
}

namespace Warhead
{
    enum class HandlerType : uint8_t
    {
        Console = 1,
        File,

        Max
    };

    constexpr uint8_t MAX_HANDLER_TYPE = static_cast<uint8_t>(HandlerType::Max);

    constexpr std::string_view GetHandlerName(HandlerType type)
    {
        switch (type)
        {
            case HandlerType::Console:
                return "Console";
            case HandlerType::File:
                return "File";
            default:
                return "Unknown";
        }
    }

//    typedef spdlog::sink_ptr(*SinkCreateFn)(std::string_view, spdlog::level::level_enum, std::string_view, std::vector<std::string_view> const&);
//
//    template <class SinkImpl>
//    inline spdlog::sink_ptr CreateSink(std::string_view name, spdlog::level::level_enum level, std::string_view pattern, std::vector<std::string_view> const& options)
//    {
//        return std::make_shared<SinkImpl>(name, level, pattern, options);
//    }

    class WH_COMMON_API Log
    {
        Log();
        ~Log();
        Log(Log const&) = delete;
        Log(Log&&) = delete;
        Log& operator=(Log const&) = delete;
        Log& operator=(Log&&) = delete;

    public:
        static Log* instance();

        void Initialize();
        void Clear();
        bool ShouldLog(std::string_view filter, quill::LogLevel level);
        void LoadFromConfig();

        // Logger functions
        [[nodiscard]] quill::Logger* GetLoggerByType(std::string_view type);
        [[nodiscard]] quill::Logger* GetLogger(std::string_view loggerName) const;
        void SetLoggerLevel(std::string_view loggerName, int level);

        // Handler functions
        void AddHandler(std::string const& name, quill::Handler* handler);
        [[nodiscard]] std::shared_ptr<quill::Handler> GetHandler(std::string const& name);
        void SetHandlerLevel(std::string_view handlerName, int level);

       [[nodiscard]] std::string_view GetLogsDir() const { return _logsDir; }

    private:
        void CreateLoggerFromConfig(std::string_view configLoggerName);
        void CreateHandlersFromConfig(std::string_view configHandlerName);
        void ReadLoggersFromConfig();
        void ReadSinksFromConfig();
        [[nodiscard]] quill::Logger* GetQuillLogger(std::string const& loggerName) const;

        std::string _logsDir;

        mutable std::recursive_mutex _rmutex; /**< Thread safe access to logger map, Mutable to have a const get_logger() function  */
        std::unordered_map<std::string, std::shared_ptr<quill::Handler>> _handlers;
        std::unordered_map<std::string, quill::Logger*> _loggers;
        quill::LogLevel _lowestLogLevel{ quill::LogLevel::Critical };
    };
}

#define sLog Warhead::Log::instance()

#define LOG_CALL(filterType__, level__, fmt__, ...) \
        do \
        { \
            if (sLog->ShouldLog(filterType__, level__)) \
            { \
                auto logger__{ sLog->GetLoggerByType(filterType__) }; \
                if (!logger__) \
                    break; \
                \
                QUILL_LOGGER_CALL(QUILL_LIKELY, logger__, level__, fmt__, ##__VA_ARGS__); \
            } \
        } while (false)

// Critical - 7
#define LOG_CRIT(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

// Error - 6
#define LOG_ERROR(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

// Warning - 5
#define LOG_WARN(filterType__, fmt, ...)  \
    LOG_CALL(filterType__, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

// Info - 4
#define LOG_INFO(filterType__, fmt, ...)  \
    LOG_CALL(filterType__, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

// Debug - 3
#define LOG_DEBUG(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

// Trace - TraceL1
#define LOG_TRACE(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

// TraceL1 - 2
#define LOG_TRACE_L1(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

// TraceL2 - 1
#define LOG_TRACE_L2(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

// TraceL3 - 0
#define LOG_TRACE_L3(filterType__, fmt, ...) \
    LOG_CALL(filterType__, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

#define LOG_GM(accountId__, fmt, ...) \
        LOG_CALL("commands.gm", quill::LogLevel::Info, fmt, ##__VA_ARGS__)

#endif
