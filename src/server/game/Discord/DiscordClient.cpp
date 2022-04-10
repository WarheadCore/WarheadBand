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

#include "DiscordClient.h"
#include "DiscordPacket.h"
#include "ClientSocketMgr.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "GameTime.h"
#include "TaskScheduler.h"
#include "Tokenize.h"
#include "StringConvert.h"
#include "World.h"
#include "Timer.h"
#include "Channel.h"
#include "Player.h"

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

    _serverID = sModulesConfig->GetOption<int64>("Discord.Server.ID");
    if (!_serverID)
    {
        LOG_ERROR("module.discord", "> Incorrect server id. Disable module");
        _isEnable = false;
        return;
    }

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

    // Prepare channels
    ConfigureChannels();
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

    auto channelID = GetChannelIDForType(DiscordDefaultChannelType::ServerStatus);
    auto color = DiscordMessageColor::Blue;
    auto title = "Статус игрового мира";
    auto description = Warhead::StringFormat("Игровой мир `{}` инициализирован за `{}`", sWorld->GetRealmName(), Warhead::Time::ToTimeString(elapsed));

    SendEmbedMessage(channelID, color, title, description);
}

void DiscordClient::SendServerShutdown()
{
    if (!_isEnable)
        return;

    auto channelID = GetChannelIDForType(DiscordDefaultChannelType::ServerStatus);
    auto color = DiscordMessageColor::Orange;
    auto title = "Статус игрового мира";
    auto description = Warhead::StringFormat("Игровой мир `{}` отключен", sWorld->GetRealmName());

    SendEmbedMessage(channelID, color, title, description);
}

void DiscordClient::LogChat(Player* player, uint32 type, std::string_view msg)
{
    if (!_isEnable || !MOD_CONF_GET_BOOL("Discord.GameChatLogs.Enable") || !player)
        return;

    // Check message
    if (type != CHAT_MSG_SAY)
        return;

    auto channelID = GetChannelIDForType(DiscordСhatChannelType::Say);
    auto color = DiscordMessageColor::White;
    auto title = Warhead::StringFormat("Чат игрового мира {}", sWorld->GetRealmName());
    auto description = Warhead::StringFormat("**[{}]**: {}", player->GetName(), msg);

    SendEmbedMessage(channelID, color, title, description);
}

void DiscordClient::LogChat(Player* player, uint32 /*type*/, std::string_view msg, Channel* channel)
{
    if (!_isEnable || !MOD_CONF_GET_BOOL("Discord.GameChatLogs.Enable") || !player)
        return;

    // Check message
    if (!channel || !channel->IsLFG())
        return;

    auto channelID = GetChannelIDForType(DiscordСhatChannelType::Channel);
    auto color = DiscordMessageColor::Gray;
    auto title = Warhead::StringFormat("Чат игрового мира {}", sWorld->GetRealmName());
    auto description = Warhead::StringFormat("**[{}][{}]**: {}", channel->GetName(), player->GetName(), msg);

    SendEmbedMessage(channelID, color, title, description);
}

void DiscordClient::LogLogin(Player* player)
{
    if (!_isEnable || !player)
        return;

    auto channelType = DiscordLoginChannelType::Default;
    auto color = DiscordMessageColor::Orange;

    auto accountID = player->GetSession()->GetAccountId();
    auto channelID = DiscordLoginChannelType::Default;
    auto security = player->GetSession()->GetSecurity();
    auto title = "Вход в игровой мир";
    //auto description = "Вход в игровой мир";

    if (security > SEC_PLAYER)
        channelID = DiscordLoginChannelType::GM;
    else if (security >= SEC_ADMINISTRATOR)
        channelID = DiscordLoginChannelType::Admin;

    std::string accountName;
    AccountMgr::GetName(player->GetSession()->GetAccountId(), accountName);

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

    if (channelID != DiscordLoginChannelType::Default)
    {
        fields.emplace_back(EmbedField("Аккаунт", "`" + accountName + "`", true));
        fields.emplace_back(EmbedField("Айпи", "`" + player->GetSession()->GetRemoteAddress() + "`", true));
    }

    SendEmbedMessage(GetChannelIDForType(channelID), color, title, {}, &fields);
}

void DiscordClient::SendDefaultMessage(int64 channelID, std::string_view message)
{
    DiscordPacket packet(CLIENT_SEND_MESSAGE);
    packet << int64(channelID);
    packet << message;
    sClientSocketMgr->AddPacketToQueue(packet);
}

void DiscordClient::SendEmbedMessage(int64 channelID, DiscordMessageColor color, std::string_view title, std::string_view description, DiscordEmbedFields const* fields)
{
    if (fields && fields->size() > WARHEAED_DISCORD_MAX_EMBED_FIELDS)
    {
        LOG_ERROR("module.discord", "> Try send incorrect embed message. Fields size ({}) > {}", fields->size(), WARHEAED_DISCORD_MAX_EMBED_FIELDS);
        return;
    }

    DiscordPacket packet(CLIENT_SEND_MESSAGE_EMBED);
    packet << int64(channelID);
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

int64 DiscordClient::GetChannelIDForType(DiscordDefaultChannelType channelType)
{
    if (!_isEnable)
        return 0;

    ASSERT(!_channels.empty());
    return _channels.at(static_cast<uint8>(channelType));
}

int64 DiscordClient::GetChannelIDForType(DiscordСhatChannelType channelType)
{
    if (!_isEnable)
        return 0;

    ASSERT(!_chatChannels.empty());
    return _chatChannels.at(static_cast<uint8>(channelType));
}

int64 DiscordClient::GetChannelIDForType(DiscordLoginChannelType channelType)
{
    if (!_isEnable)
        return 0;

    ASSERT(!_loginChannels.empty());
    return _loginChannels.at(static_cast<uint8>(channelType));
}

void DiscordClient::ConfigureChannels()
{
    if (!_isEnable)
        return;

    ConfigureChannels(_channels, "Discord.DefaultChannel.ListID");

    if (MOD_CONF_GET_BOOL("Discord.GameChatLogs.Enable"))
        ConfigureChannels(_chatChannels, "Discord.GameChatLogs.ChannelsListID");

    if (MOD_CONF_GET_BOOL("Discord.GameLoginLogs.Enable"))
        ConfigureChannels(_loginChannels, "Discord.GameLoginLogs.ChannelsListID");
}

template<size_t S>
void DiscordClient::ConfigureChannels(std::array<int64, S>& store, std::string const& configOption)
{
    std::string channelsList = sModulesConfig->GetOption<std::string>(configOption);
    if (channelsList.empty())
    {
        LOG_FATAL("module.discord", "> Empty channel list ids for discord. Option '{}'. Disable system", configOption);
        _isEnable = false;
        return;
    }

    auto const& channelTokens = Warhead::Tokenize(channelsList, ',', false);
    if (channelTokens.size() != S)
    {
        LOG_FATAL("module.discord", "> Incorrect channel list ids for discord. Option '{}'. Disable system", configOption);
        _isEnable = false;
        return;
    }

    for (size_t i = 0; i < channelTokens.size(); i++)
    {
        std::string _channel = std::string(channelTokens.at(i));
        auto channelID = Warhead::StringTo<int64>(Warhead::String::TrimLeft(_channel));

        if (!channelID)
        {
            LOG_FATAL("module.discord", "> Incorrect channel id '{}'. Disable system", channelTokens.at(i));
            _isEnable = false;
            return;
        }

        store[i] = *channelID;
    }
}
