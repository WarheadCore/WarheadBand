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

#include "Logger.h"
#include "Channel.h"

void Warhead::Logger::AddChannel(std::shared_ptr<Channel> channel)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _channels.emplace_back(channel);
}

void Warhead::Logger::Write(LogMessage const& msg)
{
    std::lock_guard<std::mutex> guard(_mutex);

    if (_level < msg.GetLevel())
        return;

    for (auto const& channel : _channels)
    {
        if (channel->GetLevel() < msg.GetLevel())
            continue;

        channel->Write(msg);
    }
}

void Warhead::Logger::DeleteChannel(std::string_view name)
{
    if (_channels.empty())
        return;

    std::lock_guard<std::mutex> guard(_mutex);

    for (auto const& channel : _channels)
    {
        if (channel->GetName() == name)
        {
            std::erase(_channels, channel);
            return;
        }
    }
}
