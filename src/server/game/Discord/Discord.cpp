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
#include "Random.h"
#include "StopWatch.h"
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

    _serverID = sGameConfig->GetOption<int64>("Discord.Guild.ID");
    if (!_serverID)
    {
        LOG_FATAL("discord", "> Empty guild id for discord. Disable system");
        _isEnable = false;
        return;
    }

    _bot = std::make_unique<dpp::cluster>(botToken, dpp::i_all_intents);

    // Prepare logs
    ConfigureLogs();

    // Prepare commands
    ConfigureCommands();

    // Load account links
    LoadUsers();

    // Prepare channels
    ConfigureChannels();

    _codeList.clear();

    _bot->start(true);
}

void Discord::Update(uint32 diff)
{
    if (!_isEnable)
        return;

    _scheduler.Update(diff);
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
        return 0;

    ASSERT(!_channels.empty());

    return _channels.at(static_cast<uint8>(channelType));
}

int64 Discord::GetChannelIDForType(DiscordСhatChannelType channelType)
{
    if (!_isEnable)
        return 0;

    ASSERT(!_chatChannels.empty());

    return _chatChannels.at(static_cast<uint8>(channelType));
}

int64 Discord::GetChannelIDForType(DiscordLoginChannelType channelType)
{
    if (!_isEnable)
        return 0;

    ASSERT(!_loginChannels.empty());

    return _loginChannels.at(static_cast<uint8>(channelType));
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

void Discord::LogLogin(Player* player)
{
    if (!_isEnable || !player)
        return;

    auto accountID = player->GetSession()->GetAccountId();
    auto channelID = DiscordLoginChannelType::Default;
    auto security = player->GetSession()->GetSecurity();

    if (security > SEC_PLAYER)
        channelID = DiscordLoginChannelType::GM;
    else if (security >= SEC_ADMINISTRATOR)
        channelID = DiscordLoginChannelType::Admin;

    std::string accountName;
    AccountMgr::GetName(player->GetSession()->GetAccountId(), accountName);

    if (auto userID = GetUserID(accountID))
    {
        _bot->user_get(userID, [this, player, security, channelID, accountName](dpp::confirmation_callback_t const callback)
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
    }

    dpp::embed embed = dpp::embed();
    embed.set_color(static_cast<uint32>(DiscordMessageColor::Orange));
    embed.set_title("Вход в игровой мир");
    embed.add_field("Персонаж", "`" + player->GetName() + "`", true);
    embed.add_field("Реалм", "`" + sWorld->GetRealmName() + "`", true);

    if (channelID != DiscordLoginChannelType::Default)
    {
        embed.add_field("Аккаунт", "`" + accountName + "`", true);
        embed.add_field("Айпи", "`" + player->GetSession()->GetRemoteAddress() + "`", true);
    }

    embed.set_timestamp(GameTime::GetGameTime().count());

    _bot->message_create(dpp::message(GetChannelIDForType(channelID), embed));
}

void Discord::ConfigureLogs()
{
    if (!_isEnable)
        return;

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
            break;
        default:
            break;
        }
    });
}

void Discord::ConfigureCommands()
{
    if (!_isEnable)
        return;

    if (!CONF_GET_BOOL("Discord.Channel.Command.Enable"))
        return;

    _bot->on_ready([this](const dpp::ready_t& event)
    {
        dpp::slashcommand coreComand("core", "Категория команд для ядра сервера", _bot->me.id);
        dpp::slashcommand onlineComand("online", "Текущий онлайн сервера", _bot->me.id);
        dpp::slashcommand accountComand("account", "Категория команд для игрового аккаунта", _bot->me.id);

        coreComand.add_option(dpp::command_option(dpp::co_sub_command, "info", "Общая информация о ядре и игровом мире"));
        accountComand.add_option(dpp::command_option(dpp::co_sub_command, "bind", "Привязать игровой аккаунт к Discord"));

        _bot->guild_command_create(coreComand, _serverID);
        _bot->guild_command_create(onlineComand, _serverID);
        _bot->guild_command_create(accountComand, _serverID);
    });

    _bot->on_interaction_create([this](const dpp::interaction_create_t& event)
    {
        if (!IsCorrectChannel(event.command.channel_id, DiscordChannelType::Commands))
            return;

        dpp::command_interaction cmd_data = event.command.get_command_interaction();

        if (event.command.get_command_name() == "core" && cmd_data.options[0].name == "info")
        {
            dpp::embed embed = dpp::embed();
            embed.set_color(static_cast<uint32>(DiscordMessageColor::Indigo));
            embed.set_author(GitRevision::GetCompanyNameStr(), GitRevision::GetUrlOrigin(), "https://avatars.githubusercontent.com/u/50081821?v=4");
            embed.set_url(Warhead::StringFormat("{}/commit/{}", GitRevision::GetUrlOrigin(), GitRevision::GetFullHash()));
            embed.set_title(Warhead::StringFormat("[{}/WarheadBand] ", GitRevision::GetCompanyNameStr()));
            embed.add_field("Commit info", Warhead::StringFormat("`{}`", GitRevision::GetFullVersion()));
            embed.add_field("Uptime", Warhead::StringFormat("`{}`", Warhead::Time::ToTimeString(GameTime::GetUptime(), 4, TimeFormat::FullText)));
            embed.add_field("Diff", Warhead::StringFormat("Update time diff: `{}ms`, Average: `{}ms`", sWorldUpdateTime.GetLastUpdateTime(), sWorldUpdateTime.GetAverageUpdateTime()));
            embed.add_field("Realm info", Warhead::StringFormat("Realm name: `{}`. Players online: `{}`. Max players online: `{}`", sWorld->GetRealmName(), sWorld->GetPlayerCount(), sWorld->GetMaxActiveSessionCount()));
            embed.set_timestamp(GameTime::GetGameTime().count());

            event.reply(dpp::message(GetChannelIDForType(DiscordChannelType::Commands), embed).set_flags(dpp::m_ephemeral));
        }

        if (event.command.get_command_name() == "online")
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

                embed.set_description(Warhead::StringFormat("{} игроков онлайн", sWhoListCacheMgr->GetWhoList().size()));

                for (auto const& target : sWhoListCacheMgr->GetWhoList())
                {
                    if (count > 25)
                        break;

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

            event.reply(dpp::message(GetChannelIDForType(DiscordChannelType::Commands), embed).set_flags(dpp::m_ephemeral));
        }

        if (event.command.get_command_name() == "account" && cmd_data.options[0].name == "bind")
        {
            dpp::interaction_modal_response modal("accBind", "Привязка игрового аккаунта");
            modal.add_component(
                dpp::component().
                set_label("Введите проверочный код").
                set_id("field_id").
                set_type(dpp::cot_text).
                set_placeholder("000000").
                set_min_length(6).
                set_max_length(6).
                set_text_style(dpp::text_short));
            event.dialog(modal);
        }
    });

    _bot->on_form_submit([this](const dpp::form_submit_t& event)
    {
        auto const& value = std::get<std::string>(event.components[0].components[0].value);

        if (auto guid = GetGuidWithCode(*Warhead::StringTo<uint32>(value)))
        {
            Player* player = ObjectAccessor::FindPlayer(guid);
            dpp::message message;

            if (!player)
            {
                message.set_content(Warhead::StringFormat("Введённый код `'{}'` верный, но вашего персонажа нет в игре. Повторите процедуру и не выходите из онлайна на своём персонаже", value));
                message.set_flags(dpp::m_ephemeral);

                event.reply(message);
                return;
            }

            ChatHandler handler(player->GetSession());
            auto accountID = player->GetSession()->GetAccountId();
            auto userID = event.command.member.user_id;

            if (auto userID = GetUserID(accountID))
            {
                message.set_content(Warhead::StringFormat("Введённый код `'{}'` верный, но ваш аккаунт уже привязан", value));
                message.set_flags(dpp::m_ephemeral);

                event.reply(message);
                return;
            }

            if (!AddUser(accountID, userID))
            {
                message.set_content(Warhead::StringFormat("Введённый код `'{}'` верный, но произошла ошибка при привязке", value));
                message.set_flags(dpp::m_ephemeral);

                event.reply(message);
                return;
            }

            handler.PSendSysMessage("> Вы привязали свою учётную запись к Discord аккаунту ({})", event.command.member.user_id);

            std::string accountName;
            AccountMgr::GetName(player->GetSession()->GetAccountId(), accountName);

            dpp::embed embed = dpp::embed();
            embed.set_color(static_cast<uint32>(DiscordMessageColor::Teal));
            embed.set_title("Привязка игрового аккаунта");
            embed.set_description(Warhead::StringFormat("Вы привязали свою учётную запись Discord к игровому аккаунту: `{}`", accountName));
            embed.set_timestamp(GameTime::GetGameTime().count());
            message.add_embed(embed);
            message.set_flags(dpp::m_ephemeral);
            event.reply(message);
        }
        else
        {
            dpp::message m;
            m.set_content(Warhead::StringFormat("Введённый код `'{}'` неверен.", value));
            m.set_flags(dpp::m_ephemeral);

            event.reply(m);
        }
    });
}

void Discord::LoadUsers()
{
    if (!_isEnable)
        return;

    StopWatch sw;

    _accList.clear();

    LOG_INFO("server.loading", "");
    LOG_INFO("server.loading", "Loading Discord account links...");

    PreparedQueryResult result = LoginDatabase.Query(LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_DISCORD));
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 Discord account links. DB table `account_discord_link` is empty.");
        LOG_INFO("server.loading", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _accList.emplace(fields[0].Get<uint32>(), fields[1].Get<int64>());

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Discord account links in {}", _accList.size(), sw);
    LOG_INFO("server.loading", "");
}

void Discord::ConfigureChannels()
{
    if (!_isEnable)
        return;

    ConfigureChannels(_channels, "Discord.Channel.ListID");

    if (CONF_GET_BOOL("Discord.GameChatLogs.Enable"))
        ConfigureChannels(_chatChannels, "Discord.GameChatLogs.ChannelsListID");

    if (CONF_GET_BOOL("Discord.GameLoginLogs.Enable"))
        ConfigureChannels(_loginChannels, "Discord.GameLoginLogs.ChannelsListID");
}

template<size_t S>
void Discord::ConfigureChannels(std::array<int64, S>& store, std::string const& configOption)
{
    std::string channelsList = CONF_GET_STR(configOption);
    if (channelsList.empty())
    {
        LOG_FATAL("discord", "> Empty channel list ids for discord. Option '{}'. Disable system", configOption);
        _isEnable = false;
        return;
    }

    auto channelTokens = Warhead::Tokenize(channelsList, ',', false);
    if (channelTokens.size() != S)
    {
        LOG_FATAL("discord", "> Incorrect channel list ids for discord. Option '{}'. Disable system", configOption);
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

        store[i] = *channelID;
    }
}

void Discord::AddCode(ObjectGuid guid)
{
    if (!_isEnable)
        return;

    Player* player = ObjectAccessor::FindPlayer(guid);
    if (!player)
        return;

    ChatHandler handler(player->GetSession());

    if (GetUserID(player->GetSession()->GetAccountId()))
    {
        handler.PSendSysMessage("> Этот аккаунт уже привязан");
        return;
    }

    auto const& itr = _codeList.find(guid);
    if (itr != _codeList.end())
    {
        handler.PSendSysMessage("> У вас уже был код '{}'", itr->second);
        _codeList.erase(guid);
    }

    auto code = urand(100000, 900000);

    handler.PSendSysMessage("> Для вас создан новый код '{}'. Он действует 5 минут.", code);
    _codeList.emplace(guid, code);

    _scheduler.Schedule(5min, [this, guid](TaskContext /*context*/)
    {
        auto const& itr = _codeList.find(guid);
        if (itr == _codeList.end())
            return;

        Player* player = ObjectAccessor::FindPlayer(guid);
        if (player)
        {
            ChatHandler handler(player->GetSession());
            handler.PSendSysMessage("> Ваш код для привязки Discord был удалён");
        }

        _codeList.erase(guid);
    });
}

ObjectGuid Discord::GetGuidWithCode(uint32 code)
{
    if (!_isEnable)
        return ObjectGuid::Empty;

    for (auto const& [guid, _code] : _codeList)
    {
        if (code == _code)
            return guid;
    }

    return ObjectGuid::Empty;
}

int64 Discord::GetUserID(uint32 accountID)
{
    if (!_isEnable)
        return 0LL;

    auto const& itr = _accList.find(accountID);
    if (itr != _accList.end())
        return itr->second;

    return 0LL;
}

bool Discord::AddUser(uint32 accountID, int64 userID)
{
    if (!_isEnable)
        return false;

    auto const& itr = _accList.find(accountID);
    if (itr != _accList.end())
        return false;

    auto stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_DISCORD);
    stmt->SetArguments(accountID, userID);
    LoginDatabase.Execute(stmt);

    _accList.emplace(accountID, userID);
    return true;
}
