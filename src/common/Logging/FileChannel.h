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

#ifndef _WARHEAD_FILE_CHANNEL_H_
#define _WARHEAD_FILE_CHANNEL_H_

#include "Channel.h"
#include <mutex>

namespace Warhead
{
    class WH_COMMON_API FileChannel : public Channel
    {
    public:
        static constexpr auto ThisChannelType{ ChannelType::File };

        FileChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& options);
        virtual ~FileChannel();

        void Write(LogMessage const& msg) override;

    private:
        bool OpenFile();
        void CloseFile();

        std::string _logsDir;
        std::string _fileName;
        std::unique_ptr<std::ofstream> _logFile;
        bool _isDynamicFileName{ false };
        bool _isFlush{ true };
        bool _isOpenModeAppend{ true };
        bool _isAddTimestamp{ false };
        std::mutex _mutex;
    };

} // namespace Warhead

#endif //
