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

#include "DiscordClient.h"
#include "Channel.h"
#include "ClientSocketMgr.h"
#include "DiscordPacket.h"
#include "GameTime.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "StringConvert.h"
#include "Timer.h"
#include "Tokenize.h"
#include "World.h"

/*static*/ DiscordClient* DiscordClient::instance()
{
    static DiscordClient instance;
    return &instance;
}

void DiscordClient::LoadConfig(bool /*reload*/ /*= false*/)
{
    _isEnable = MOD_CONF_GET_BOOL("Discord.Enable");
    if (!_isEnable)
        return;

    _accountName = MOD_CONF_GET_STR("Discord.Server.Account.Name");
    _accountKey = MOD_CONF_GET_STR("Discord.Server.Account.Key");

    if (_accountName.empty())
    {
        LOG_ERROR("module.discord", "> Empty account name");
        _isEnable = false;
        return;
    }

    if (_accountKey.empty())
    {
        LOG_ERROR("module.discord", "> Empty key");
        _isEnable = false;
        return;
    }

    _isGameChatLogsEnable = MOD_CONF_GET_BOOL("Discord.GameChatLogs.Enable");
    _isLoginLogsEnable = MOD_CONF_GET_BOOL("Discord.GameLoginLogs.Enable");
}

void DiscordClient::Initialize()
{
    if (!_isEnable)
        return;

    sClientSocketMgr->Initialize();
    _scheduler = std::make_unique<TaskScheduler>();

    _scheduler->Schedule(randtime(10s, 30s), [](TaskContext context)
    {
        sClientSocketMgr->SendPingMessage();
        context.Repeat(randtime(10s, 30s));
    });
}

void DiscordClient::Update(Milliseconds diff)
{
    if (!_scheduler || !_isEnable)
        return;

    _scheduler->Update(diff);
}

void DiscordClient::SendServerStartup(Microseconds elapsed)
{
    if (!_isEnable)
        return;

    auto channelType = DiscordChannelType::ServerStatus;
    auto color = DiscordMessageColor::Blue;
    auto title = "Статус игрового мира";
    auto description = Warhead::StringFormat("Игровой мир `{}` инициализирован за `{}`", sWorld->GetRealmName(), Warhead::Time::ToTimeString(elapsed));

    SendEmbedMessage(channelType, color, title, description);
}

void DiscordClient::SendServerShutdown()
{
    if (!_isEnable)
        return;

    auto channelType = DiscordChannelType::ServerStatus;
    auto color = DiscordMessageColor::Orange;
    auto title = "Статус игрового мира";
    auto description = Warhead::StringFormat("Игровой мир `{}` отключен", sWorld->GetRealmName());

    SendEmbedMessage(channelType, color, title, description);
}

void DiscordClient::LogChat(Player* player, uint32 type, std::string_view msg)
{
    if (!_isEnable || !_isGameChatLogsEnable || !player)
        return;

    // Check message
    if (type != CHAT_MSG_SAY)
        return;

    auto channelType = DiscordChannelType::ChatSay;
    auto color = DiscordMessageColor::White;
    auto title = Warhead::StringFormat("Чат игрового мира {}", sWorld->GetRealmName());
    auto description = Warhead::StringFormat("**[{}]**: {}", player->GetName(), msg);

    SendEmbedMessage(channelType, color, title, description);
}

void DiscordClient::LogChat(Player* player, uint32 /*type*/, std::string_view msg, Channel* channel)
{
    if (!_isEnable || !_isGameChatLogsEnable || !player)
        return;

    // Check message
    if (!channel || !channel->IsLFG())
        return;

    auto channelType = DiscordChannelType::ChatChannel;
    auto color = DiscordMessageColor::Gray;
    auto title = Warhead::StringFormat("Чат игрового мира {}", sWorld->GetRealmName());
    auto description = Warhead::StringFormat("**[{}][{}]**: {}", channel->GetName(), player->GetName(), msg);

    SendEmbedMessage(channelType, color, title, description);
}

void DiscordClient::LogLogin(Player* player)
{
    if (!_isEnable || !player || !_isLoginLogsEnable)
        return;

    auto channelType = DiscordChannelType::LoginPlayer;
    auto color = DiscordMessageColor::Orange;
    auto accountID = player->GetSession()->GetAccountId();
    auto security = player->GetSession()->GetSecurity();
    auto title = "Вход в игровой мир";

    if (security >= SEC_ADMINISTRATOR)
        channelType = DiscordChannelType::LoginAdmin;
    else if (security > SEC_PLAYER)
        channelType = DiscordChannelType::LoginGM;

    std::string accountName;
    AccountMgr::GetName(accountID, accountName);

    /*if (auto userID = GetUserID(accountID))
    {
        _bot->user_get(userID, [this, player, channelID, accountName](dpp::confirmation_callback_t const callback)
        {
            dpp::user_identified user = std::get<dpp::user_identified>(callback.value);

            dpp::embed embed = dpp::embed();
            embed.set_color(static_cast<uint32>(DiscordMessageColor::Orange));
            embed.set_title("Вход в игровой мир");
            embed.set_author(user.format_username(), "", user.get_avatar_url());
            embed.add_field("Персонаж", "`" + player->GetName() + "`", true);
            embed.add_field("Реалм", "`" + sWorld->GetRealmName() + "`", true);

            if (channelID != DiscordLoginChannelType::Default)
            {
                embed.add_field("Аккаунт", "`" + accountName + "`", true);
                embed.add_field("Айпи", "`" + player->GetSession()->GetRemoteAddress() + "`", true);
            }

            embed.set_timestamp(GameTime::GetGameTime().count());

            dpp::message message(GetChannelIDForType(channelID), embed);

            _bot->message_create(message);
        });

        return;
    }*/

    DiscordEmbedFields fields;

    fields.emplace_back(EmbedField("Персонаж", "`" + player->GetName() + "`", true));
    fields.emplace_back(EmbedField("Реалм", "`" + sWorld->GetRealmName() + "`", true));

    if (channelType != DiscordChannelType::LoginPlayer)
    {
        fields.emplace_back(EmbedField("Аккаунт", "`" + accountName + "`", true));
        fields.emplace_back(EmbedField("Айпи", "`" + player->GetSession()->GetRemoteAddress() + "`", true));
    }

    SendEmbedMessage(channelType, color, title, {}, &fields);
}

void DiscordClient::SendDefaultMessage(DiscordChannelType channelType, std::string_view message)
{
    if (!_isEnable)
        return;

    DiscordPacket packet(CLIENT_SEND_MESSAGE);
    packet << static_cast<uint8>(channelType);
    packet << message;
    sClientSocketMgr->AddPacketToQueue(packet);
}

void DiscordClient::SendEmbedMessage(DiscordChannelType channelType, DiscordMessageColor color, std::string_view title, std::string_view description, DiscordEmbedFields const* fields)
{
    if (!_isEnable)
        return;

    if (fields && fields->size() > WARHEAED_DISCORD_MAX_EMBED_FIELDS)
    {
        LOG_ERROR("module.discord", "> Try send incorrect embed message. Fields size ({}) > {}", fields->size(), WARHEAED_DISCORD_MAX_EMBED_FIELDS);
        return;
    }

    DiscordPacket packet(CLIENT_SEND_MESSAGE_EMBED);
    packet << static_cast<uint8>(channelType);
    packet << static_cast<uint32>(color);
    packet << title;
    packet << description;
    packet << std::size_t(fields ? fields->size() : 0);

    if (fields)
    {
        for (auto const& embedField : *fields)
        {
            packet << embedField.Name;
            packet << embedField.Value;
            packet << embedField.IsInline;
        }
    }

    packet << static_cast<time_t>(GameTime::GetGameTime().count());
    sClientSocketMgr->AddPacketToQueue(packet);
}
