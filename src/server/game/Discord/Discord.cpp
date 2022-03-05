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

#include "Discord.h"
#include "Channel.h"
#include "GameConfig.h"
#include "GameLocale.h"
#include "GameTime.h"
#include "GitRevision.h"
#include "Log.h"
#include "Player.h"
#include "SharedDefines.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "UpdateTime.h"
#include "WhoListCacheMgr.h"
#include "World.h"
#include <dpp/dpp.h>

Discord* Discord::instance()
{
    static Discord instance;
    return &instance;
}

void Discord::Start()
{
    _isEnable = CONF_GET_BOOL("Discord.Enable");

    if (!_isEnable)
        return;

    std::string botToken = CONF_GET_STR("Discord.Bot.Token");
    if (botToken.empty())
    {
        LOG_FATAL("discord", "> Empty bot token for discord. Disable system");
        _isEnable = false;
        return;
    }

    std::string channelsList = CONF_GET_STR("Discord.Channel.ListID");
    if (channelsList.empty())
    {
        LOG_FATAL("discord", "> Empty channel list ids for discord. Disable system");
        _isEnable = false;
        return;
    }

    auto channelTokens = Warhead::Tokenize(channelsList, ',', false);
    if (channelTokens.size() != MAX_CHANNEL_TYPE)
    {
        LOG_FATAL("discord", "> Incorrect channel list ids for discord. Disable system");
        _isEnable = false;
        return;
    }

    for (size_t i = 0; i < channelTokens.size(); i++)
    {
        std::string _channel = std::string(channelTokens.at(i));
        auto channelID = Warhead::StringTo<int64>(Warhead::String::TrimLeft(_channel));

        if (!channelID)
        {
            LOG_FATAL("discord", "> Incorrect channel id '{}'. Disable system", channelTokens.at(i));
            _isEnable = false;
            return;
        }

        _channels[i] = *channelID;
    }

    if (CONF_GET_BOOL("Discord.GameChatLogs.Enable"))
    {
        std::string gameChatLogsChannelsList = CONF_GET_STR("Discord.GameChatLogs.ChannelsListID");
        if (gameChatLogsChannelsList.empty())
        {
            LOG_FATAL("discord", "> Empty channel list ids for game chat logs. Disable system");
            _isEnable = false;
            return;
        }

        auto gameChatLogsChannelTokens = Warhead::Tokenize(gameChatLogsChannelsList, ',', false);
        if (gameChatLogsChannelTokens.size() != MAX_CHANNEL_CHAT_TYPE)
        {
            LOG_FATAL("discord", "> Incorrect channel list ids for game chat logs. Disable system");
            _isEnable = false;
            return;
        }

        for (size_t i = 0; i < gameChatLogsChannelTokens.size(); i++)
        {
            std::string _channel = std::string(gameChatLogsChannelTokens.at(i));
            auto channelID = Warhead::StringTo<int64>(Warhead::String::TrimLeft(_channel));

            if (!channelID)
            {
                LOG_FATAL("discord", "> Incorrect channel id '{}' for game chat logs. Disable system", gameChatLogsChannelTokens.at(i));
                _isEnable = false;
                return;
            }

            _chatChannels[i] = *channelID;
        }
    }

    _bot = std::make_unique<dpp::cluster>(botToken, dpp::i_all_intents);

    _bot->on_ready([this]([[maybe_unused]] const auto& event)
    {
        LOG_INFO("discord.bot", "> DiscordBot: Logged in as {}", _bot->me.username);
    });

    _bot->on_log([](const dpp::log_t& event)
    {
        switch (event.severity)
        {
        case dpp::ll_trace:
            LOG_TRACE("discord.bot", "> DiscordBot: {}", event.message);
            break;
        case dpp::ll_debug:
            LOG_DEBUG("discord.bot", "> DiscordBot: {}", event.message);
            break;
        case dpp::ll_info:
            LOG_INFO("discord.bot", "> DiscordBot: {}", event.message);
            break;
        case dpp::ll_warning:
            LOG_WARN("discord.bot", "> DiscordBot: {}", event.message);
            break;
        case dpp::ll_error:
            LOG_ERROR("discord.bot", "> DiscordBot: {}", event.message);
            break;
        case dpp::ll_critical:
            LOG_CRIT("discord.bot", "> DiscordBot: {}", event.message);
        default:
            break;
        }
    });

    if (CONF_GET_BOOL("Discord.Channel.Command.Enable"))
    {
        _bot->on_message_create([this](dpp::message_create_t const& event)
        {
            if (!IsCorrectChannel(event.msg.channel_id, DiscordChannelType::Commands))
                return;

            if (event.msg.content == ".core info")
            {
                dpp::embed embed = dpp::embed();
                embed.set_color(static_cast<uint32>(DiscordMessageColor::Indigo));
                embed.set_author(GitRevision::GetCompanyNameStr(), GitRevision::GetUrlOrigin(), "https://avatars.githubusercontent.com/u/50081821?v=4");
                embed.set_url(Warhead::StringFormat("{}/commit/{}", GitRevision::GetUrlOrigin(), GitRevision::GetFullHash()));
                embed.set_title(Warhead::StringFormat("[{}/WarheadBand] ", GitRevision::GetCompanyNameStr()));
                embed.add_field("Commit info", GitRevision::GetFullVersion());
                embed.add_field("Uptime", Warhead::Time::ToTimeString(GameTime::GetUptime(), 4, TimeFormat::FullText));
                embed.add_field("Diff", Warhead::StringFormat("Update time diff: {}ms, Average: {}ms", sWorldUpdateTime.GetLastUpdateTime(), sWorldUpdateTime.GetAverageUpdateTime()));
                embed.add_field("Realm info", Warhead::StringFormat("Realm name: {}. Players online: {}. Max players online: {}", sWorld->GetRealmName(), sWorld->GetPlayerCount(), sWorld->GetMaxActiveSessionCount()));
                embed.set_timestamp(GameTime::GetGameTime().count());

                _bot->message_create(dpp::message(GetChannelIDForType(DiscordChannelType::Commands), embed).set_reference(event.msg.id));
            }

            if (event.msg.content == ".players online")
            {
                auto const& playersCount = sWorld->GetPlayerCount();

                dpp::embed embed = dpp::embed();
                embed.set_color(static_cast<uint32>(DiscordMessageColor::Indigo));
                embed.set_title("Онлайн сервера");

                if (!playersCount)
                    embed.set_description("Нет игроков на сервере");
                else
                {
                    uint8 count = 0;

                    for (auto const& target : sWhoListCacheMgr->GetWhoList())
                    {
                        // player can see MODERATOR, GAME MASTER, ADMINISTRATOR only if CONFIG_GM_IN_WHO_LIST
                        /*if (target.GetSecurity() > AccountTypes(CONF_GET_INT("GM.InWhoList.Level")))
                        {
                            continue;
                        }

                        if (!target.IsVisible())
                            continue;*/

                        auto _race = sGameLocale->GetRaseString(target.GetRace());
                        auto _class = sGameLocale->GetClassString(target.GetClass());
                        auto gender = target.GetGender();
                        auto raceStr = _race ? _race->GetText(sGameLocale->GetDBCLocaleIndex(), gender) : "Неизвестно";
                        auto classStr = _class ? _class->GetText(sGameLocale->GetDBCLocaleIndex(), gender) : "Неизвестно";

                        embed.add_field(Warhead::StringFormat("{}. {}", ++count, target.GetPlayerName()), Warhead::StringFormat("{} {}. Уровень {}", raceStr, classStr, target.GetLevel()));
                    }
                }

                embed.set_timestamp(GameTime::GetGameTime().count());

                _bot->message_create(dpp::message(GetChannelIDForType(DiscordChannelType::Commands), embed).set_reference(event.msg.id));
            }
        });
    }

    _bot->start(true);
}

void Discord::SendServerStartup(std::string_view duration)
{
    if (!_isEnable)
        return;

    dpp::embed embed = dpp::embed();
    embed.set_color(static_cast<uint32>(DiscordMessageColor::Blue));
    embed.set_title("Статус игрового мира");
    embed.set_description(Warhead::StringFormat("Игровой мир `{}` инициализирован за `{}`", sWorld->GetRealmName(), duration));
    embed.set_timestamp(GameTime::GetGameTime().count());

    _bot->message_create(dpp::message(GetChannelIDForType(DiscordChannelType::ServerStatus), embed));
}

void Discord::SendServerShutdown()
{
    if (!_isEnable)
        return;

    dpp::embed embed = dpp::embed();
    embed.set_color(static_cast<uint32>(DiscordMessageColor::Orange));
    embed.set_title("Статус игрового мира");
    embed.set_description(Warhead::StringFormat("Игровой мир `{}` отключен", sWorld->GetRealmName()));
    embed.set_timestamp(GameTime::GetGameTime().count());

    _bot->message_create(dpp::message(GetChannelIDForType(DiscordChannelType::ServerStatus), embed));
}

bool Discord::IsCorrectChannel(int64 channelID, DiscordChannelType channelType)
{
    if (!_isEnable)
        return false;

    ASSERT(!_channels.empty());

    return _channels.at(static_cast<uint8>(channelType)) == channelID;
}

int64 Discord::GetChannelIDForType(DiscordChannelType channelType)
{
    if (!_isEnable)
        return false;

    ASSERT(!_channels.empty());

    return _channels.at(static_cast<uint8>(channelType));
}

int64 Discord::GetChannelIDForType(DiscordСhatChannelType channelType)
{
    if (!_isEnable)
        return false;

    ASSERT(!_chatChannels.empty());

    return _chatChannels.at(static_cast<uint8>(channelType));
}

void Discord::SendDefaultMessage(std::string_view title, std::string_view description, DiscordMessageColor color /*= DiscordMessageColor::Blue*/)
{
    if (!_isEnable)
        return;

    dpp::embed embed = dpp::embed();
    embed.set_color(static_cast<uint32>(color));
    embed.set_title(std::string(title));
    embed.set_description(std::string(description));
    embed.set_timestamp(GameTime::GetGameTime().count());

    _bot->message_create(dpp::message(GetChannelIDForType(DiscordChannelType::General), embed));
}

void Discord::LogChat(Player* player, uint32 type, std::string_view msg, Channel* channel /*= nullptr*/)
{
    if (!_isEnable || !CONF_GET_BOOL("Discord.GameChatLogs.Enable") || !player)
        return;

    std::string message{};
    auto channelType = DiscordСhatChannelType::Say;
    uint32 messageColor = static_cast<uint32>(DiscordMessageColor::White);

    if (channel && channel->IsLFG())
    {
        message = Warhead::StringFormat("**[{}][{}]**: {}", channel->GetName(), player->GetName(), msg);
        channelType = DiscordСhatChannelType::Channel;
        messageColor = static_cast<uint32>(DiscordMessageColor::Gray);
    }
    else if (type == CHAT_MSG_SAY)
        message = Warhead::StringFormat("**[{}]**: {}", player->GetName(), msg);
    else
        return;

    dpp::embed embed = dpp::embed();
    embed.set_color(messageColor);
    embed.set_title(Warhead::StringFormat("Чат игрового мира {}", sWorld->GetRealmName()));
    embed.set_description(message);
    embed.set_timestamp(GameTime::GetGameTime().count());

    _bot->message_create(dpp::message(GetChannelIDForType(channelType), embed));
}
