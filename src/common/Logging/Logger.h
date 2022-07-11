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

#ifndef _WARHEAD_LOGGER_H_
#define _WARHEAD_LOGGER_H_

#include "LogMessage.h"
#include <mutex>
#include <vector>

namespace Warhead
{
    class Channel;

    class WH_COMMON_API Logger
    {
    public:
        Logger(std::string_view name, LogLevel level) :
            _name(name), _level(level) { }

        ~Logger() = default;

        inline const std::string_view GetName() const { return _name; }

        void AddChannel(std::shared_ptr<Channel> channel);

        inline void SetLevel(LogLevel level) { _level = level; }
        inline LogLevel GetLevel() const { return _level; }

        void Write(LogMessage const& msg);

        void DeleteChannel(std::string_view name);
        inline void Shutdown() { _channels.clear(); }

    private:
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::string _name;
        std::vector<std::shared_ptr<Channel>> _channels;
        LogLevel _level{ LogLevel::Disabled };

        std::mutex _mutex;
    };
}

#endif
