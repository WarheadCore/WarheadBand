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

#include "ClientSocketMgr.h"
#include "DiscordClient.h"
#include "DiscordSharedDefines.h"
#include "IoContext.h"
#include "ModulesConfig.h"
#include "ScriptMgr.h"

class DiscordClient_World : public WorldScript
{
public:
    DiscordClient_World() : WorldScript("DiscordClient_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        if (!reload)
            sModulesConfig->AddOption({ "Discord.Server.Host", "Discord.Server.Port", });

        sModulesConfig->AddOption({
            "Discord.Enable",
            "Discord.Server.ID",
            "Discord.DefaultChannel.ListID",
            "Discord.GameChatLogs.Enable",
            "Discord.GameChatLogs.ChannelsListID",
            "Discord.GameLoginLogs.Enable",
            "Discord.GameLoginLogs.ChannelsListID" });

        sDiscord->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sDiscord->Initialize();
    }

    void OnShutdown() override
    {
        sDiscord->SendServerShutdown();
        sClientSocketMgr->Disconnect();
    }

    void OnUpdate(uint32 diff) override
    {
        sDiscord->Update(Milliseconds(diff));
    }

    void OnBeforeWorldInitialized(Microseconds elapsed) override
    {
        sDiscord->SendServerStartup(elapsed);
    }
};

class DiscordClient_Server : public ServerScript
{
public:
    DiscordClient_Server() : ServerScript("DiscordClient_Server") { }

    void OnIoContext(std::weak_ptr<Warhead::Asio::IoContext> ioContext) override
    {
        if (auto context = ioContext.lock())
            sClientSocketMgr->InitializeIo(*context);
    }
};

class DiscordClient_Player : public PlayerScript
{
public:
    DiscordClient_Player() : PlayerScript("DiscordClient_Player") { }

    void OnLogin(Player* player) override
    {
        sDiscord->LogLogin(player);
    }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg) override
    {
        sDiscord->LogChat(player, type, msg);
    }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        sDiscord->LogChat(player, type, msg, channel);
    }
};

// Group all custom scripts
void AddSC_DiscordClient()
{
    new DiscordClient_World();
    new DiscordClient_Server();
    new DiscordClient_Player();
}
