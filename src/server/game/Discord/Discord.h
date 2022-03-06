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
#include "TaskScheduler.h"
#include <array>
#include <memory>
#include <unordered_map>
#include <string_view>

namespace dpp
{
    class cluster;
}

class Channel;
class Player;
class ObjectGuid;

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
    Gray = 0xadb5bd,
    White = 0xffffff
};

enum class DiscordChannelType : uint8
{
    General,
    ServerStatus,
    Commands,

    MaxType
};

enum class DiscordСhatChannelType : uint8
{
    Say,
    Channel,

    MaxType
};

enum class DiscordLoginChannelType : uint8
{
    Default,
    GM,
    Admin,

    MaxType
};

constexpr uint8 MAX_CHANNEL_TYPE = static_cast<uint8>(DiscordChannelType::MaxType);
constexpr uint8 MAX_CHANNEL_CHAT_TYPE = static_cast<uint8>(DiscordСhatChannelType::MaxType);
constexpr uint8 MAX_CHANNEL_LOGIN_TYPE = static_cast<uint8>(DiscordLoginChannelType::MaxType);

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

    void Update(uint32 diff);

    void Start();
    void SendServerStartup(std::string_view duration);
    void SendServerShutdown();
    void SendDefaultMessage(std::string_view title, std::string_view description, DiscordMessageColor color = DiscordMessageColor::Blue);

    // Channels
    bool IsCorrectChannel(int64 channelID, DiscordChannelType channelType);
    int64 GetChannelIDForType(DiscordChannelType channelType);
    int64 GetChannelIDForType(DiscordСhatChannelType channelType);
    int64 GetChannelIDForType(DiscordLoginChannelType channelType);

    // Chat
    void LogChat(Player* player, uint32 type, std::string_view msg, Channel* channel = nullptr);

    // Character login/logout
    void LogLogin(Player* player);
    //void LogLogout(Player* player);

    // Auth
    void AddCode(ObjectGuid guid);
    ObjectGuid GetGuidWithCode(uint32 code);

private:
    void ConfigureLogs();
    void ConfigureCommands();
    void LoadUsers();
    void ConfigureChannels();

    template<size_t S>
    void ConfigureChannels(std::array<int64, S>& store, std::string const& configOption);

    int64 GetUserID(uint32 accountID);
    bool AddUser(uint32 accountID, int64 userID);

    bool _isEnable{ false };
    int64 _serverID{ 0 };

    std::array<int64, MAX_CHANNEL_TYPE> _channels = {};
    std::array<int64, MAX_CHANNEL_CHAT_TYPE> _chatChannels = {};
    std::array<int64, MAX_CHANNEL_LOGIN_TYPE> _loginChannels = {};

    std::unique_ptr<dpp::cluster> _bot = {};
    std::unordered_map<ObjectGuid, uint32/*code*/> _codeList;
    std::unordered_map<uint32/*accid*/, int64/*userid*/> _accList;
    TaskScheduler _scheduler;
};

#define sDiscord Discord::instance()

#endif // _DISCORD_H_
