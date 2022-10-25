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

#include "DiscordEmbedMsg.h"

namespace dpp
{
    class cluster;
}

class Channel;
class Player;
class ObjectGuid;

class WH_GAME_API DiscordMgr
{
public:
    DiscordMgr() = default;
    ~DiscordMgr();

    static DiscordMgr* instance();

    void LoadConfig(bool reload);
    void Start();
    void Stop();

    void SendDefaultMessage(std::string_view message, DiscordChannelType channelType = DiscordChannelType::MaxType);
    void SendEmbedMessage(DiscordEmbedMsg const& embed, DiscordChannelType channelType = DiscordChannelType::MaxType);

    void SendServerStartup(std::string_view duration);
    void SendServerShutdown();

private:
    void ConfigureLogs();
    void ConfigureCommands();
    void CheckGuild();
    void CleanupMessages();
    uint64 GetChannelID(DiscordChannelType channelType);

    std::unique_ptr<dpp::cluster> _bot;

    // Config
    bool _isEnable{};
    std::string _botToken;
    std::size_t _guildID{};

    // Channels
    DiscordChannelsList _channels{};

    DiscordMgr(DiscordMgr const&) = delete;
    DiscordMgr(DiscordMgr&&) = delete;
    DiscordMgr& operator=(DiscordMgr const&) = delete;
    DiscordMgr& operator=(DiscordMgr&&) = delete;
};

#define sDiscordMgr DiscordMgr::instance()

#endif
