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

#ifndef _DISCORD_CLIENT_H_
#define _DISCORD_CLIENT_H_

#include "DiscordSharedDefines.h"
#include "Duration.h"
#include "TaskScheduler.h"
#include <memory>

class Channel;
class Player;

class WH_GAME_API DiscordClient
{
    DiscordClient(DiscordClient const&) = delete;
    DiscordClient(DiscordClient&&) = delete;
    DiscordClient& operator=(DiscordClient const&) = delete;
    DiscordClient& operator=(DiscordClient&&) = delete;

public:
    DiscordClient() = default;
    ~DiscordClient() = default;

    static DiscordClient* instance();

    void LoadConfig(bool reload = false);
    void Initialize();
    void Update(Milliseconds diff);

    inline bool IsEnable() { return _isEnable; }
    inline auto GetAccountName() { return _accountName; }
    inline auto GetAccountKey() { return _accountKey; }

    void SendServerStartup(Microseconds elapsed);
    void SendServerShutdown();

    // Chat
    void LogChat(Player* player, uint32 type, std::string_view msg);
    void LogChat(Player* player, uint32 type, std::string_view msg, Channel* channel);

    // Character login/logout
    void LogLogin(Player* player);
    //void LogLogout(Player* player);

    // Auth
    //void AddCode(ObjectGuid guid);
    //ObjectGuid GetGuidWithCode(uint32 code);

    void SendDefaultMessage(DiscordChannelType channelType, std::string_view message);
    void SendEmbedMessage(DiscordChannelType channelType, DiscordMessageColor color, std::string_view title, std::string_view description, DiscordEmbedFields const* fields = nullptr);

private:
    /*int64 GetUserID(uint32 accountID);
    bool AddUser(uint32 accountID, int64 userID);*/

    bool _isEnable{ false };
    bool _isGameChatLogsEnable{ false };
    bool _isLoginLogsEnable{ false };
    std::string _accountName;
    std::string _accountKey;
    std::unique_ptr<TaskScheduler> _scheduler;

    //std::unordered_map<ObjectGuid, uint32/*code*/> _codeList;
    //std::unordered_map<uint32/*accid*/, int64/*userid*/> _accList;
};

#define sDiscord DiscordClient::instance()

#endif // _DISCORD_CLIENT_H_
