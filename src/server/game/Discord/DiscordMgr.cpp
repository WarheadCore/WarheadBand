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
#include "AccountMgr.h"
#include "Channel.h"
#include "CliCommandMgr.h"
#include "GameConfig.h"
#include "GameLocale.h"
#include "GameTime.h"
#include "Log.h"
#include "Player.h"
#include "StopWatch.h"
#include "UpdateTime.h"
#include "Util.h"
#include "World.h"
#include <Warhead/RegularExpression.h>
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
    constexpr auto CHANNEL_NAME_CHAT_CHANNEL = "chat-channel";

    // Login
    constexpr auto CHANNEL_NAME_LOGIN_PLAYER = "login-player";
    constexpr auto CHANNEL_NAME_LOGIN_GM = "login-gm";
    constexpr auto CHANNEL_NAME_LOGIN_ADMIN = "login-admin";

    // Ban list
    constexpr auto CHANNEL_NAME_CORE_LOGS = "core-logs";

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

        if (channelName == CHANNEL_NAME_CHAT_CHANNEL)
            return DiscordChannelType::ChatChannel;

        if (channelName == CHANNEL_NAME_LOGIN_PLAYER)
            return DiscordChannelType::LoginPlayer;

        if (channelName == CHANNEL_NAME_LOGIN_GM)
            return DiscordChannelType::LoginGM;

        if (channelName == CHANNEL_NAME_LOGIN_ADMIN)
            return DiscordChannelType::LoginAdmin;

        if (channelName == CHANNEL_NAME_CORE_LOGS)
            return DiscordChannelType::CoreLogs;

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
            case DiscordChannelType::ChatChannel:
                return CHANNEL_NAME_CHAT_CHANNEL;
            case DiscordChannelType::LoginPlayer:
                return CHANNEL_NAME_LOGIN_PLAYER;
            case DiscordChannelType::LoginGM:
                return CHANNEL_NAME_LOGIN_GM;
            case DiscordChannelType::LoginAdmin:
                return CHANNEL_NAME_LOGIN_ADMIN;
            case DiscordChannelType::CoreLogs:
                return CHANNEL_NAME_CORE_LOGS;
            default:
                return "";
        }
    }

    constexpr bool IsHiddenChannel(DiscordChannelType type)
    {
        switch (type)
        {
            case DiscordChannelType::Commands:
            case DiscordChannelType::LoginAdmin:
            case DiscordChannelType::LoginGM:
            case DiscordChannelType::CoreLogs:
                return true;
            default:
                return false;
        }
    }

    static void ReplaceLinks(std::string_view text, std::string& normalText)
    {
        static std::string pattern{ R"(\|cff[a-zA-Z0-9_]{6}\|Hitem:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}:[a-zA-Z0-9_]{1,8}\|h)" };
        static Warhead::RegularExpression reBefore(pattern, Warhead::RegularExpression::RE_MULTILINE);
        static Warhead::RegularExpression reAfter(R"(\|h\|r)", Warhead::RegularExpression::RE_MULTILINE);

        normalText = text;

        reBefore.subst(normalText, "", Warhead::RegularExpression::RE_GLOBAL);
        reAfter.subst(normalText, "", Warhead::RegularExpression::RE_GLOBAL);
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

        _isEnableChatLogs = CONF_GET_BOOL("Discord.GameChatLogs.Enable");
        _isEnableLoginLogs = CONF_GET_BOOL("Discord.GameLoginLogs.Enable");
        _isEnableBanLogs = CONF_GET_BOOL("Discord.BanLogs.Enable");
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

    _bot = std::make_unique<dpp::cluster>(_botToken, dpp::i_unverified_default_intents, 0, 0, 1, true,
        dpp::cache_policy_t{ dpp::cp_aggressive, dpp::cp_aggressive, dpp::cp_aggressive }, 3);

    // Prepare logs
    ConfigureLogs();

    _bot->start(dpp::st_return);

    // Check bot in guild, category and text channels
    CheckGuild();

    // Prepare commands
    ConfigureCommands();

    // Send prepared messages before bot init
    SendBeforeStartMessages();

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
    if (!_isEnable || !_bot)
        return;

    dpp::message discordMessage;
    discordMessage.channel_id = GetChannelID(channelType == DiscordChannelType::MaxType ? DiscordChannelType::General : channelType);
    discordMessage.content = std::string(message);

    _bot->message_create(discordMessage);
}

void DiscordMgr::SendEmbedMessage(DiscordEmbedMsg const& embed, DiscordChannelType channelType /*= DiscordChannelType::MaxType*/)
{
    if (!_isEnable || !_bot)
        return;

    _bot->message_create(dpp::message(GetChannelID(channelType == DiscordChannelType::MaxType ? DiscordChannelType::General : channelType), *embed.GetMessage()));
}

void DiscordMgr::SendLogMessage(std::unique_ptr<DiscordEmbedMsg>&& embed, DiscordChannelType channelType)
{
    if (!_isEnable)
        return;

    if (!_bot)
    {
        _queue.Enqueue(new DiscordQueueMessage(std::move(embed), channelType));
        return;
    }

    _bot->message_create(dpp::message(GetChannelID(channelType == DiscordChannelType::MaxType ? DiscordChannelType::General : channelType), *embed->GetMessage()));
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

void DiscordMgr::LogChat(Player* player, std::string_view msg, Channel* channel /*= nullptr*/)
{
    if (!_isEnable || !_isEnableChatLogs || !player || !channel || !channel->IsLFG())
        return;

    std::string replacedText;
    ReplaceLinks(msg, replacedText);

    DiscordEmbedMsg embedMsg;
    embedMsg.SetTitle(Warhead::StringFormat("Чат игрового мира: `{}`", sWorld->GetRealmName()));
    embedMsg.SetDescription(Warhead::StringFormat("**[{}]**: {}", player->GetName(), replacedText));
    embedMsg.SetColor(DiscordMessageColor::Gray);

    SendEmbedMessage(embedMsg, DiscordChannelType::ChatChannel);
}

void DiscordMgr::LogLogin(Player* player)
{
    if (!_isEnable || !_isEnableLoginLogs || !player)
        return;

    auto session{ player->GetSession() };
    auto accountID = session->GetAccountId();
    auto channelType{ DiscordChannelType::LoginPlayer };
    auto security = session->GetSecurity();

    if (security >= SEC_ADMINISTRATOR)
        channelType = DiscordChannelType::LoginAdmin;
    else if (security > SEC_PLAYER)
        channelType = DiscordChannelType::LoginGM;

    std::string accountName;
    AccountMgr::GetName(accountID, accountName);

    DiscordEmbedMsg embedMsg;
    embedMsg.SetTitle(Warhead::StringFormat("Вход в игровой мир: `{}`", sWorld->GetRealmName()));
    embedMsg.SetColor(DiscordMessageColor::Orange);
    embedMsg.AddEmbedField("Персонаж", "`" + player->GetName() + "`", true);

    if (channelType != DiscordChannelType::LoginPlayer)
    {
        embedMsg.AddEmbedField("Аккаунт", "`" + accountName + "`", true);
        embedMsg.AddEmbedField("IP", "`" + session->GetRemoteAddress() + "`", true);
    }

    SendEmbedMessage(embedMsg, channelType);
}

void DiscordMgr::LogBan(std::string_view type, std::string_view banned, std::string_view time, std::string_view author, std::string_view reason)
{
    DiscordEmbedMsg embedMsg;
    embedMsg.SetTitle(Warhead::StringFormat("Ban {}: `{}`", type, banned));
    embedMsg.SetColor(DiscordMessageColor::Orange);
    embedMsg.AddEmbedField("Time", Warhead::StringFormat("`{}`", time.empty() ? "Permanent" : time), true);
    embedMsg.AddEmbedField("Author", Warhead::StringFormat("`{}`", author), true);
    embedMsg.AddEmbedField("Reason", Warhead::StringFormat("`{}`", reason), true);

    SendEmbedMessage(embedMsg, DiscordChannelType::CoreLogs);
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
    // Server commands
    _bot->on_message_create([this](dpp::message_create_t const& event)
    {
        // Commands allowed only from specific channel
        if (event.msg.channel_id != GetChannelID(DiscordChannelType::Commands))
            return;

        // Skip check bot messages
        if (event.msg.author.id == _bot->me.id)
            return;

        std::string_view command{ event.msg.content};
        auto embedMsg = std::make_shared<DiscordEmbedMsg>();
        embedMsg->SetTitle(Warhead::StringFormat("Команда: `{}`", command));

        if (!command.starts_with('.'))
        {
            embedMsg->SetColor(DiscordMessageColor::Red);
            embedMsg->SetDescription("Ошибка: Все команды начинаются с `.`");

            event.reply(dpp::message(event.msg.channel_id, *embedMsg->GetMessage()).set_flags(dpp::m_ephemeral));
            _bot->message_delete(event.msg.id, event.msg.channel_id);
            return;
        }

        auto commandExecuting = std::make_shared<std::promise<void>>();

        sCliCommandMgr->AddCommand(command, [this, embedMsg](std::string_view command)
        {
            embedMsg->AddDescription(command);
        }, [this, commandExecuting, embedMsg](bool success)
        {
            embedMsg->SetColor(success ? DiscordMessageColor::Teal : DiscordMessageColor::Red);
            commandExecuting->set_value();
        });

        // Wait execute command
        commandExecuting->get_future().wait();

        dpp::message replyMessage{ event.msg.channel_id, *embedMsg->GetMessage() };
        replyMessage.set_flags(dpp::m_ephemeral);

        event.reply(replyMessage);
    });

    // Message clean commands
    dpp::slashcommand messageClean{ "clean-messages", "Удалить сообщения в текущем канале", _bot->me.id };
    dpp::command_option countOption{ dpp::co_integer, "count", "Количество последних сообщений", true };
    countOption.set_min_value(1);
    countOption.set_max_value(100);
    messageClean.add_option(countOption);
    messageClean.set_default_permissions(0);

    _bot->guild_command_create(messageClean, _guildID, [](const dpp::confirmation_callback_t& callback)
    {
        if (callback.is_error())
            std::cout << callback.http_info.body << "\n";
    });

    _bot->on_slashcommand([this](dpp::slashcommand_t const& event)
    {
        if (event.command.get_command_name() != "clean-messages")
            return;

        if (event.command.msg.guild_id != _guildID)
            return;

        auto channelID{ event.command.channel_id };

        // Get count parameters
        auto count = std::get<int64>(event.get_parameter("count"));

        // Start make message
        auto embedMsg = std::make_shared<DiscordEmbedMsg>();
        embedMsg->SetTitle("Удаление сообщений");

        auto const& messages = _bot->messages_get_sync(channelID, 0, 0, 0, count);
        if (messages.empty())
        {
            embedMsg->SetColor(DiscordMessageColor::Red);
            embedMsg->SetDescription("В этом канале нет сообщений");

            dpp::message replyMessage{ channelID, *embedMsg->GetMessage() };
            replyMessage.set_flags(dpp::m_ephemeral);

            event.reply(replyMessage);
            return;
        }

        std::vector<dpp::snowflake> messagesToDelete;

        for (auto const& [messageID, message] : messages)
            messagesToDelete.emplace_back(messageID);

        if (messagesToDelete.size() > 1)
            _bot->message_delete_bulk(messagesToDelete, channelID);
        else
            _bot->message_delete(messagesToDelete.front(), channelID);

        embedMsg->SetColor(DiscordMessageColor::Orange);
        embedMsg->SetDescription(Warhead::StringFormat("Удалено `{}` сообщений", messagesToDelete.size()));

        dpp::message replyMessage{ channelID, *embedMsg->GetMessage() };
        replyMessage.set_flags(dpp::m_ephemeral);

        event.reply(replyMessage);
    });
}

void DiscordMgr::CheckGuild()
{
    StopWatch sw;

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
                LOG_DEBUG("discord", "> Category with name '{}' exist. ID {}", DEFAULT_CATEGORY_NAME, uint64(channelID));
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
                    LOG_INFO("discord", "> Created channel {}. ID {}. Is hidden: {}", channelName, uint64(createdChannel.id), IsHiddenChannel(channelType));
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

    LOG_DEBUG("discord", "> End check text channels guild. Elapsed: {}", sw);

    CleanupMessages(DiscordChannelType::ServerStatus);
    CleanupMessages(DiscordChannelType::Commands);
}

void DiscordMgr::CleanupMessages(DiscordChannelType channelType)
{
    StopWatch sw;
    auto channelID{ GetChannelID(channelType) };

    LOG_DEBUG("discord", "> Start cleanup messages for channel: {} ({})", GetChannelName(channelType), channelID);

    auto timeAllow{ GameTime::GetGameTime() - 1_days };
    std::vector<dpp::snowflake> messagesToDelete;

    auto const& messages = _bot->messages_get_sync(channelID, 0, 0, 0, 100);
    if (messages.empty())
    {
        LOG_DEBUG("discord", "> Not found messages for channel: {} ({})", GetChannelName(channelType), channelID);
        return;
    }

    auto GetUTCTimeFromLocal = [](time_t seconds)
    {
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
        return seconds - _timezone;
#else
        return seconds - timezone;
#endif
    };

    for (auto const& [messageID, message] : messages)
    {
         if (Seconds{ GetUTCTimeFromLocal(message.sent) } >= timeAllow)
            continue;

        messagesToDelete.emplace_back(messageID);
    }

    if (messagesToDelete.empty())
    {
        LOG_DEBUG("discord", "> Not found old messages for channel: {} ({})", GetChannelName(channelType), channelID);
        return;
    }

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

void DiscordMgr::SendBeforeStartMessages()
{
    DiscordQueueMessage* msg{ nullptr };

    while (_queue.Dequeue(msg))
    {
        _bot->message_create(dpp::message(GetChannelID(msg->GetChannelType()), *msg->GetMessage()->GetMessage()));
        delete msg;
    }
}