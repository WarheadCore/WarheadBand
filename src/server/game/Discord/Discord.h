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

#ifndef _DISCORD_H_
#define _DISCORD_H_

#include "Define.h"
#include <array>
#include <memory>

namespace dpp
{
    class cluster;
}

enum class DiscordMessageColor : uint32
{
    Blue = 0x28a745,
    Red = 0xdc3545,
    Orange = 0xfd7e14,
    Purple = 0x6f42c1,
    Indigo = 0x6610f2,
    Yellow = 0xffc107,
    Teal = 0x20c997,
    Cyan = 0x17a2b8,
    Gray = 0xadb5bd
};

enum class DiscordChannelType : uint8
{
    General,
    ServerStatus,
    ChatLogs,
    Commands,

    MaxType,
};

constexpr uint8 MAX_CHANNEL_TYPE = static_cast<uint8>(DiscordChannelType::MaxType);

class WH_GAME_API Discord
{
    Discord() = default;
    ~Discord() = default;
    Discord(Discord const&) = delete;
    Discord(Discord&&) = delete;
    Discord& operator=(Discord const&) = delete;
    Discord& operator=(Discord&&) = delete;

public:
    static Discord* instance();

    void Start();
    void SendServerStatus(bool isStartup);
    void SendDefaultMessage(std::string_view title, std::string_view description, DiscordMessageColor color = DiscordMessageColor::Blue);

    // Channels
    bool IsCorrectChannel(int64 channelID, DiscordChannelType channelType);
    int64 GetChannelIDForType(DiscordChannelType channelType);

private:
    bool _isEnable = false;
    std::array<int64, MAX_CHANNEL_TYPE> _channels = {};
    std::unique_ptr<dpp::cluster> _bot = {};
};

#define sDiscord Discord::instance()

#endif // _DISCORD_H_
