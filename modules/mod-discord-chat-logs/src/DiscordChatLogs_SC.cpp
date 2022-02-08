/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Discord.h"
#include "ModulesConfig.h"

class DiscordChatLogs_Player : public PlayerScript
{
public:
    DiscordChatLogs_Player() : PlayerScript("DiscordChatLogs_Player") { }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg) override
    {
        if (!MOD_CONF_GET_BOOL("Discord.ChatLogs.Enable"))
            return;

        sDiscord->LogChat(player, type, msg);
    }
    
    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        if (!MOD_CONF_GET_BOOL("Discord.ChatLogs.Enable"))
            return;

        sDiscord->LogChat(player, type, msg, channel);
    }
};

class DiscordChatLogs_World : public WorldScript
{
public:
    DiscordChatLogs_World() : WorldScript("DiscordChatLogs_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sModulesConfig->AddOption("Discord.ChatLogs.Enable");
    }
};

// Group all custom scripts
void AddSC_DiscordChatLogs()
{
    new DiscordChatLogs_Player();
    new DiscordChatLogs_World();
}
