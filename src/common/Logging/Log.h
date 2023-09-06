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
#include <spdlog/common.h>
#include <spdlog/fwd.h>
#include <unordered_map>
#include <vector>

namespace Warhead
{
    enum class SinkType : uint8_t
    {
        Console = 1,
        File,

        Max
    };

    constexpr uint8_t MAX_SINK_TYPE = static_cast<uint8_t>(SinkType::Max);

    constexpr std::string_view GetSinkName(SinkType type)
    {
        switch (type)
        {
            case SinkType::Console:
                return "Console";
            case SinkType::File:
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
        void Clear();
        bool ShouldLog(std::string_view filter, spdlog::level::level_enum level);
        void LoadFromConfig();

        // Logger functions
        spdlog::logger* GetLoggerByType(std::string_view type);
        static spdlog::logger* GetLogger(std::string_view loggerName);
        void SetLoggerLevel(std::string_view loggerName, int level);

        // Sink functions
        spdlog::sink_ptr GetSink(std::string_view sinkName) const;
        void AddSink(std::string_view sinkName, spdlog::sink_ptr sink);
        void SetSinkLevel(std::string_view sinkName, int level);

        template<typename... Args>
        inline void OutMessage(std::string_view filter, spdlog::source_loc source, spdlog::level::level_enum level, Warhead::FormatString<Args...> fmt, Args&&... args)
        {
            Write(filter, source, level, StringFormat(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        inline void OutCommand(uint32 account, Warhead::FormatString<Args...> fmt, Args&&... args)
        {
            if (!ShouldLog("commands.gm", spdlog::level::info))
                return;

            WriteCommand(account, StringFormat(fmt, std::forward<Args>(args)...));
        }

        std::string_view GetLogsDir() { return _logsDir; }

    private:
        void Write(std::string_view filter, spdlog::source_loc source, spdlog::level::level_enum level, std::string_view message);
        void WriteCommand(uint32 accountID, std::string_view message);

        void CreateLoggerFromConfig(std::string_view configLoggerName);
        void CreateSinksFromConfig(std::string_view configSinkName);
        void ReadLoggersFromConfig();
        void ReadSinksFromConfig();

        std::string _logsDir;
        spdlog::level::level_enum _lowestLogLevel{spdlog::level::critical };
        std::unordered_map<std::string, spdlog::sink_ptr> _sinks;
    };
}

#define sLog Warhead::Log::instance()

#define LOG_CALL(filterType__, level__, ...) \
        do \
        { \
            if (sLog->ShouldLog(filterType__, level__)) \
            { \
                try \
                { \
                    sLog->OutMessage(filterType__, spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level__, __VA_ARGS__); \
                } \
                catch (std::exception const& e) \
                { \
                    sLog->OutMessage("root", spdlog::source_loc{}, spdlog::level::err, "Wrong format occurred ({}) at '{}:{}'", \
                        e.what(), __FILE__, __LINE__); \
                } \
            } \
        } while (0)

// Critical - 5
#define LOG_CRIT(filterType__, ...) \
    LOG_CALL(filterType__, spdlog::level::critical, __VA_ARGS__)

// Error - 4
#define LOG_ERROR(filterType__, ...) \
    LOG_CALL(filterType__, spdlog::level::err, __VA_ARGS__)

// Warning - 3
#define LOG_WARN(filterType__, ...)  \
    LOG_CALL(filterType__, spdlog::level::warn, __VA_ARGS__)

// Info - 2
#define LOG_INFO(filterType__, ...)  \
    LOG_CALL(filterType__, spdlog::level::info, __VA_ARGS__)

// Debug - 1
#define LOG_DEBUG(filterType__, ...) \
    LOG_CALL(filterType__, spdlog::level::debug, __VA_ARGS__)

// Trace - 0
#define LOG_TRACE(filterType__, ...) \
    LOG_CALL(filterType__, spdlog::level::trace, __VA_ARGS__)

#define LOG_GM(accountId__, ...) \
    sLog->OutCommand(accountId__, __VA_ARGS__);

#endif