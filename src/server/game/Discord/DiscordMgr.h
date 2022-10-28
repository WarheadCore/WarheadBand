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

#ifndef _DISCORD_MGR_H_
#define _DISCORD_MGR_H_

#include <utility>

#include "DiscordEmbedMsg.h"
#include "MPSCQueue.h"

namespace dpp
{
    class cluster;
}

class Channel;
class Player;
class ObjectGuid;

struct WH_GAME_API DiscordQueueMessage
{
public:
    DiscordQueueMessage(std::unique_ptr<DiscordEmbedMsg>&& message, DiscordChannelType channelType) :
        _message(std::move(message)), _channelType(channelType) { }

    ~DiscordQueueMessage() = default;

    std::atomic<DiscordQueueMessage*> QueueLink;

    inline DiscordEmbedMsg const* GetMessage() const { return _message.get(); }
    inline DiscordChannelType GetChannelType() const { return _channelType; }

private:
    std::unique_ptr<DiscordEmbedMsg> _message{};
    DiscordChannelType _channelType{ DiscordChannelType::CoreLogs };

    DiscordQueueMessage(DiscordQueueMessage const& right) = delete;
    DiscordQueueMessage& operator=(DiscordQueueMessage const& right) = delete;
};

class WH_GAME_API DiscordMgr
{
public:
    DiscordMgr() = default;
    ~DiscordMgr();

    static DiscordMgr* instance();

    inline bool IsEnable() { return _isEnable; }

    void LoadConfig(bool reload);
    void Start();
    void Stop();

    void SendDefaultMessage(std::string_view message, DiscordChannelType channelType = DiscordChannelType::MaxType);
    void SendEmbedMessage(DiscordEmbedMsg const& embed, DiscordChannelType channelType = DiscordChannelType::MaxType);
    void SendLogMessage(std::unique_ptr<DiscordEmbedMsg>&& embed, DiscordChannelType channelType);

    void SendServerStartup(std::string_view duration);
    void SendServerShutdown();

    // Chat
    void LogChat(Player* player, std::string_view msg, Channel* channel);

    // Character login/logout
    void LogLogin(Player* player);

    // Ban
    void LogBan(std::string_view type, std::string_view banned, std::string_view time, std::string_view author, std::string_view reason);

private:
    void ConfigureLogs();
    void ConfigureCommands();
    void CheckGuild();
    void CleanupMessages(DiscordChannelType channelType);
    uint64 GetChannelID(DiscordChannelType channelType);
    void SendBeforeStartMessages();

    std::unique_ptr<dpp::cluster> _bot;

    // Config
    bool _isEnable{};
    std::string _botToken;
    std::size_t _guildID{};
    bool _isEnableChatLogs{};
    bool _isEnableLoginLogs{};
    bool _isEnableBanLogs{};

    // Channels
    DiscordChannelsList _channels{};

    // Before start up queue
    MPSCQueue<DiscordQueueMessage, &DiscordQueueMessage::QueueLink> _queue;

    DiscordMgr(DiscordMgr const&) = delete;
    DiscordMgr(DiscordMgr&&) = delete;
    DiscordMgr& operator=(DiscordMgr const&) = delete;
    DiscordMgr& operator=(DiscordMgr&&) = delete;
};

#define sDiscordMgr DiscordMgr::instance()

#endif
