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

#include "DiscordMgr.h"
#include "GameConfig.h"
#include "Log.h"
#include "StopWatch.h"
#include "Util.h"
#include "GameTime.h"
#include "GameLocale.h"
#include "UpdateTime.h"
#include <dpp/cluster.h>
#include <dpp/message.h>

namespace
{
    constexpr auto DEFAULT_CATEGORY_NAME = "core-logs";

    // General
    constexpr auto CHANNEL_NAME_GENERAL = "general";
    constexpr auto CHANNEL_NAME_SERVER_STATUS = "server-status";
    constexpr auto CHANNEL_NAME_COMMANDS = "commands";

    // Chat logs
    constexpr auto CHANNEL_NAME_CHAT_SAY = "chat-say";
    constexpr auto CHANNEL_NAME_CHAT_CHANNEL = "chat-channel";

    // Login
    constexpr auto CHANNEL_NAME_LOGIN_PLAYER = "login-player";
    constexpr auto CHANNEL_NAME_LOGIN_GM = "login-gm";
    constexpr auto CHANNEL_NAME_LOGIN_ADMIN = "login-admin";

    // Owner
    constexpr auto OWNER_ID = 365169287926906883; // Winfidonarleyan | <@365169287926906883>
    constexpr auto OWNER_MENTION = "<@365169287926906883>";

    constexpr DiscordChannelType GetDiscordChannelType(std::string_view channelName)
    {
        if (channelName == CHANNEL_NAME_GENERAL)
            return DiscordChannelType::General;

        if (channelName == CHANNEL_NAME_SERVER_STATUS)
            return DiscordChannelType::ServerStatus;

        if (channelName == CHANNEL_NAME_COMMANDS)
            return DiscordChannelType::Commands;

        if (channelName == CHANNEL_NAME_CHAT_SAY)
            return DiscordChannelType::ChatSay;

        if (channelName == CHANNEL_NAME_CHAT_CHANNEL)
            return DiscordChannelType::ChatChannel;

        if (channelName == CHANNEL_NAME_LOGIN_PLAYER)
            return DiscordChannelType::LoginPlayer;

        if (channelName == CHANNEL_NAME_LOGIN_GM)
            return DiscordChannelType::LoginGM;

        if (channelName == CHANNEL_NAME_LOGIN_ADMIN)
            return DiscordChannelType::LoginAdmin;

        return DiscordChannelType::MaxType;
    }

    std::string GetChannelName(DiscordChannelType type)
    {
        switch (type)
        {
            case DiscordChannelType::General:
                return CHANNEL_NAME_GENERAL;
            case DiscordChannelType::ServerStatus:
                return CHANNEL_NAME_SERVER_STATUS;
            case DiscordChannelType::Commands:
                return CHANNEL_NAME_COMMANDS;
            case DiscordChannelType::ChatSay:
                return CHANNEL_NAME_CHAT_SAY;
            case DiscordChannelType::ChatChannel:
                return CHANNEL_NAME_CHAT_CHANNEL;
            case DiscordChannelType::LoginPlayer:
                return CHANNEL_NAME_LOGIN_PLAYER;
            case DiscordChannelType::LoginGM:
                return CHANNEL_NAME_LOGIN_GM;
            case DiscordChannelType::LoginAdmin:
                return CHANNEL_NAME_LOGIN_ADMIN;
            default:
                return "";
        }
    }

    constexpr bool IsHiddenChannel(DiscordChannelType type)
    {
        switch (type)
        {
            case DiscordChannelType::LoginAdmin:
            case DiscordChannelType::LoginGM:
                return true;
            default:
                return false;
        }
    }
}

DiscordMgr* DiscordMgr::instance()
{
    static DiscordMgr instance;
    return &instance;
}

DiscordMgr::~DiscordMgr()
{
    SendServerShutdown();

    if (_bot)
        _bot->shutdown();
}

void DiscordMgr::LoadConfig(bool reload)
{
    auto oldEnable{ _isEnable };
    _isEnable = CONF_GET_BOOL("Discord.Bot.Enable");

    if (!reload && !_isEnable)
        return;

    // If after reload config disable - stop bot
    if (reload && oldEnable && !_isEnable)
    {
        LOG_WARN("discord", "Stop discord bot after config reload");
        Stop();
        return;
    }

    // Load config options
    {
        _botToken = CONF_GET_STR("Discord.Bot.Token");
        if (_botToken.empty())
        {
            LOG_FATAL("discord", "> Empty bot token for discord. Disable system");
            _isEnable = false;
            return;
        }

        _guildID = sGameConfig->GetOption<int64>("Discord.Guild.ID");
        if (!_guildID)
        {
            LOG_FATAL("discord", "> Empty guild id for discord. Disable system");
            _isEnable = false;
            return;
        }
    }

    // Start bot if after reload config option set to enable
    if (reload && !oldEnable && _isEnable)
        Start();
}

void DiscordMgr::Start()
{
    if (!_isEnable)
        return;

    LOG_INFO("server.loading", "Loading discord bot...");

    StopWatch sw;

    _bot = std::make_unique<dpp::cluster>(_botToken, dpp::i_all_intents);

    // Prepare logs
    ConfigureLogs();

    // Prepare commands
    ConfigureCommands();

    _bot->start(dpp::st_return);

    // Check bot in guild, category and text channels
    CheckGuild();

    LOG_INFO("server.loading", ">> Discord bot is initialized in {}", sw);
    LOG_INFO("server.loading", "");
}

void DiscordMgr::Stop()
{
    if (_bot)
        _bot->shutdown();

    _bot.reset();
}

void DiscordMgr::SendDefaultMessage(std::string_view message, DiscordChannelType channelType /*= DiscordChannelType::MaxType*/)
{
    if (!_isEnable)
        return;

    dpp::message discordMessage;
    discordMessage.channel_id = GetChannelID(channelType == DiscordChannelType::MaxType ? DiscordChannelType::General : channelType);
    discordMessage.content = std::string(message);

    _bot->message_create(discordMessage);
}

void DiscordMgr::SendEmbedMessage(DiscordEmbedMsg const& embed, DiscordChannelType channelType /*= DiscordChannelType::MaxType*/)
{
    if (!_isEnable)
        return;

    _bot->message_create(dpp::message(GetChannelID(channelType == DiscordChannelType::MaxType ? DiscordChannelType::General : channelType), *embed.GetMessage()));
}

void DiscordMgr::SendServerStartup(std::string_view duration)
{
    if (!_isEnable)
        return;

    DiscordEmbedMsg embedMsg;
    embedMsg.SetColor(DiscordMessageColor::Blue);
    embedMsg.SetTitle("Статус игрового мира");
    embedMsg.SetDescription(Warhead::StringFormat("Игровой мир `{}` инициализирован за `{}`", sWorld->GetRealmName(), duration));

    SendEmbedMessage(embedMsg, DiscordChannelType::ServerStatus);
}

void DiscordMgr::SendServerShutdown()
{
    if (!_isEnable)
        return;

    DiscordEmbedMsg embedMsg;
    embedMsg.SetColor(DiscordMessageColor::Orange);
    embedMsg.SetTitle("Статус игрового мира");
    embedMsg.SetDescription(Warhead::StringFormat("Игровой мир `{}` отключен", sWorld->GetRealmName()));
    embedMsg.AddEmbedField("Аптайм", GameLocale::ToTimeString(GameTime::GetUptime(), 8, false), true);
    embedMsg.AddEmbedField("Среднее время обновления", Warhead::StringFormat("{} мс", sWorldUpdateTime.GetAverageUpdateTime()), true);

    SendEmbedMessage(embedMsg, DiscordChannelType::ServerStatus);
}

void DiscordMgr::ConfigureLogs()
{
    if (!_isEnable)
        return;

    _bot->on_ready([this](const auto&)
    {
        LOG_INFO("discord.bot", "DiscordBot: Logged in as {}", _bot->me.username);
    });

    _bot->on_log([](const dpp::log_t& event)
    {
        switch (event.severity)
        {
            case dpp::ll_trace:
                LOG_TRACE("discord.bot", "DiscordBot: {}", event.message);
                break;
            case dpp::ll_debug:
                LOG_DEBUG("discord.bot", "DiscordBot: {}", event.message);
                break;
            case dpp::ll_info:
                LOG_INFO("discord.bot", "DiscordBot: {}", event.message);
                break;
            case dpp::ll_warning:
                LOG_WARN("discord.bot", "DiscordBot: {}", event.message);
                break;
            case dpp::ll_error:
                LOG_ERROR("discord.bot", "DiscordBot: {}", event.message);
                break;
            case dpp::ll_critical:
                LOG_CRIT("discord.bot", "DiscordBot: {}", event.message);
                break;
            default:
                break;
        }
    });
}

void DiscordMgr::ConfigureCommands()
{

}

void DiscordMgr::CheckGuild()
{
    auto const& guilds = _bot->current_user_get_guilds_sync();
    if (guilds.empty())
    {
        LOG_ERROR("discord", "DiscordBot: Not found guilds. Disable bot");
        return;
    }

    bool isExistGuild{};

    for (auto const& [guildID, guild] : guilds)
    {
        if (guildID == _guildID)
        {
            isExistGuild = true;
            break;
        }
    }

    if (!isExistGuild)
    {
        LOG_ERROR("discord", "DiscordBot: Not found config guild: {}. Disable bot", _guildID);
        _isEnable = false;
        Stop();
        return;
    }

    LOG_DEBUG("discord", "DiscordBot: Found config guild: {}", _guildID);
    LOG_DEBUG("discord", "> Start check channels for guild id: {}", _guildID);

    auto GetCategory = [this](dpp::channel_map const& channels) -> uint64
    {
        for (auto const& [channelID, channel] : channels)
        {
            if (!channel.is_category())
                continue;

            if (channel.name == DEFAULT_CATEGORY_NAME)
            {
                LOG_DEBUG("discord", "> Category with name '{}' exist. ID {}", DEFAULT_CATEGORY_NAME, channelID);
                return channelID;
            }
        }

        return 0;
    };

    auto CreateCategory = [this]() -> uint64
    {
        try
        {
            dpp::channel categoryToCreate;
            categoryToCreate.set_guild_id(_guildID);
            categoryToCreate.set_name(DEFAULT_CATEGORY_NAME);
            categoryToCreate.set_flags(dpp::CHANNEL_CATEGORY);

            auto createdCategory = _bot->channel_create_sync(categoryToCreate);
            return createdCategory.id;
        }
        catch (dpp::rest_exception const& error)
        {
            LOG_ERROR("discord", "DiscordBot::CheckChannels: Error at create category: {}", error.what());
        }

        return 0;
    };

    auto GetTextChannels = [this](dpp::channel_map const& channels, uint64 findCategoryID)
    {
        for (auto const& [channelID, channel] : channels)
        {
            if (!channel.is_text_channel() || channel.parent_id != findCategoryID)
                continue;

            auto channelType = GetDiscordChannelType(channel.name);
            if (channelType == DiscordChannelType::MaxType)
                continue;

            _channels[static_cast<std::size_t>(channelType)] = channelID;
        }
    };

    auto CreateTextChannels = [this](uint64 findCategoryID)
    {
        for (size_t i = 0; i < DEFAULT_CHANNELS_COUNT; i++)
        {
            auto& channelID = _channels[i];
            if (!channelID)
            {
                auto channelType{ static_cast<DiscordChannelType>(i) };
                auto channelName = GetChannelName(channelType);
                if (channelName.empty())
                {
                    LOG_ERROR("discord", "> Empty get channel name for type {}", i);
                    continue;
                }

                try
                {
                    dpp::channel channelToCreate;
                    channelToCreate.set_guild_id(_guildID);
                    channelToCreate.set_name(channelName);
                    channelToCreate.set_flags(dpp::CHANNEL_TEXT);
                    channelToCreate.set_parent_id(findCategoryID);

                    if (IsHiddenChannel(channelType))
                        channelToCreate.add_permission_overwrite(_guildID, dpp::overwrite_type::ot_role, 0, dpp::permissions::p_view_channel);

                    auto createdChannel = _bot->channel_create_sync(channelToCreate);
                    channelID = createdChannel.id;
                    LOG_INFO("discord", "> Created channel {}. ID {}. Is hiddden: {}", channelName, createdChannel.id, IsHiddenChannel(channelType));
                }
                catch (dpp::rest_exception const& error)
                {
                    LOG_ERROR("discord", "DiscordBot::CheckChannels: Error at create text channel: {}", error.what());
                    continue;
                }
            }
        }
    };

    // Clear channels
    _channels.fill(0);

    try
    {
        uint64 findCategoryID{};

        auto const& channels = _bot->channels_get_sync(_guildID);
        if (channels.empty())
        {
            LOG_FATAL("discord", "> Empty channels in guild. Guild is new?");
            findCategoryID = CreateCategory();
        }

        // Exist any channel
        if (!findCategoryID)
            findCategoryID = GetCategory(channels);

        // Not found DEFAULT_CATEGORY_NAME
        if (!findCategoryID)
        {
            LOG_ERROR("discord", "> Category with name '{}' not found. Start creating", DEFAULT_CATEGORY_NAME);
            findCategoryID = CreateCategory();

            if (!findCategoryID)
            {
                LOG_INFO("discord", "> Error after create category with name '{}'", DEFAULT_CATEGORY_NAME);
                return;
            }

            LOG_INFO("discord", "> Category with name '{}' created. ID: {}", DEFAULT_CATEGORY_NAME, findCategoryID);
        }

        // Exist DEFAULT_CATEGORY_NAME
        GetTextChannels(channels, findCategoryID);
        CreateTextChannels(findCategoryID);
    }
    catch (dpp::rest_exception const& error)
    {
        LOG_ERROR("discord", "DiscordBot::CheckChannels: Error at check channels: {}", error.what());
        return;
    }

    LOG_DEBUG("discord", "> End check text channels in guild");

    CleanupMessages();
}

void DiscordMgr::CleanupMessages()
{
    LOG_DEBUG("discord", "> Start cleanup messages");

    StopWatch sw;

    auto timeAllow{ GameTime::GetGameTime() - 1_days };
    auto channelID{ GetChannelID(DiscordChannelType::ServerStatus) };
    std::vector<dpp::snowflake> messagesToDelete;

    auto GetUTCTimeFromLocal = [](time_t seconds)
    {
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
        return seconds - _timezone;
#else
        return seconds - timezone;
#endif
    };

    auto const& messages = _bot->messages_get_sync(channelID, 0, 0, 0, 100);

    for (auto const& [messageID, message] : messages)
    {
        auto sentTime{ Seconds{ GetUTCTimeFromLocal(message.sent) } };
        if (_bot->me.id != message.author.id || sentTime >= timeAllow)
            continue;

        messagesToDelete.emplace_back(messageID);
    }

    if (messagesToDelete.empty())
        return;

    if (messagesToDelete.size() > 1)
        _bot->message_delete_bulk(messagesToDelete, channelID);
    else
        _bot->message_delete(messagesToDelete.front(), channelID);

    LOG_DEBUG("discord", "Deleted {} messages in {}", messagesToDelete.size(), sw);
}

uint64 DiscordMgr::GetChannelID(DiscordChannelType channelType)
{
    if (channelType >= DiscordChannelType::MaxType)
    {
        LOG_ERROR("discord", "> Incorrect channel type {}", AsUnderlyingType(channelType));
        return 0;
    }

    auto channelID{ _channels.at(AsUnderlyingType(channelType)) };
    if (!channelID)
        LOG_ERROR("discord", "> Empty channel id for type {}", AsUnderlyingType(channelType));

    return channelID;
}
