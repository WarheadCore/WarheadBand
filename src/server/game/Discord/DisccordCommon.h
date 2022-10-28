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

#ifndef _DISCORD_COMMON_H__
#define _DISCORD_COMMON_H__

#include "Define.h"
#include <array>

enum class DiscordMessageColor : uint32
{
    Blue    = 0x28a745,
    Red     = 0xdc3545,
    Orange  = 0xfd7e14,
    Purple  = 0x6f42c1,
    Indigo  = 0x6610f2,
    Yellow  = 0xffc107,
    Teal    = 0x20c997,
    Cyan    = 0x17a2b8,
    Gray    = 0xadb5bd,
    White   = 0xffffff
};

enum class DiscordChannelType : std::size_t
{
    General,
    ServerStatus,
    Commands,

    ChatChannel,

    LoginPlayer,
    LoginGM,
    LoginAdmin,

    CoreLogs,

    MaxType
};

constexpr auto DEFAULT_CHANNELS_COUNT = static_cast<std::size_t>(DiscordChannelType::MaxType);
using DiscordChannelsList = std::array<uint64, DEFAULT_CHANNELS_COUNT>;

#endif
