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

#include "ChatCommand.h"
#include "AccountMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GameLocale.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Tokenize.h"
#include "WorldSession.h"

using ChatSubCommandMap = std::map<std::string_view, Warhead::Impl::ChatCommands::ChatCommandNode, StringCompareLessI_T>;
static ChatSubCommandMap COMMAND_MAP;

void Warhead::Impl::ChatCommands::ChatCommandNode::LoadFromBuilder(ChatCommandBuilder const& builder)
{
    if (std::holds_alternative<ChatCommandBuilder::InvokerEntry>(builder._data))
    {
        ASSERT(!_invoker, "Duplicate blank sub-command.");
        std::tie(_invoker, _permission) = *(std::get<ChatCommandBuilder::InvokerEntry>(builder._data));
    }
    else
        LoadCommandsIntoMap(this, _subCommands, std::get<ChatCommandBuilder::SubCommandEntry>(builder._data));
}

/*static*/ void Warhead::Impl::ChatCommands::ChatCommandNode::LoadCommandsIntoMap(ChatCommandNode* blank,
    std::map<std::string_view, Warhead::Impl::ChatCommands::ChatCommandNode, StringCompareLessI_T>& map, Warhead::ChatCommands::ChatCommandTable const& commands)
{
    for (ChatCommandBuilder const& builder : commands)
    {
        if (builder._name.empty())
        {
            ASSERT(blank, "Empty name command at top level is not permitted.");
            blank->LoadFromBuilder(builder);
        }
        else
        {
            std::vector<std::string_view> const tokens = Warhead::Tokenize(builder._name, COMMAND_DELIMITER, false);
            ASSERT(!tokens.empty(), "Invalid command name '{}'.", builder._name);
            ChatSubCommandMap* subMap = &map;
            for (size_t i = 0, n = (tokens.size() - 1); i < n; ++i)
                subMap = &((*subMap)[tokens[i]]._subCommands);
            ((*subMap)[tokens.back()]).LoadFromBuilder(builder);
        }
    }
}

/*static*/ ChatSubCommandMap const& Warhead::Impl::ChatCommands::ChatCommandNode::GetTopLevelMap()
{
    if (COMMAND_MAP.empty())
        LoadCommandMap();

    return COMMAND_MAP;
}

/*static*/ void Warhead::Impl::ChatCommands::ChatCommandNode::InvalidateCommandMap()
{
    COMMAND_MAP.clear();
}

/*static*/ void Warhead::Impl::ChatCommands::ChatCommandNode::LoadCommandMap()
{
    InvalidateCommandMap();
    LoadCommandsIntoMap(nullptr, COMMAND_MAP, sScriptMgr->GetChatCommands());

    if (PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_COMMANDS)))
    {
        do
        {
            Field* fields = result->Fetch();
            std::string_view const name = fields[0].Get<std::string_view>();
            std::string_view const help = fields[2].Get<std::string_view>();
            uint32 const secLevel = fields[1].Get<uint8>();

            ChatCommandNode* cmd = nullptr;
            ChatSubCommandMap* map = &COMMAND_MAP;

            for (std::string_view key : Warhead::Tokenize(name, COMMAND_DELIMITER, false))
            {
                auto it = map->find(key);
                if (it != map->end())
                {
                    cmd = &it->second;
                    map = &cmd->_subCommands;
                }
                else
                {
                    LOG_ERROR("sql.sql", "Table `command` contains data for non-existant command '{}'. Skipped.", name);
                    cmd = nullptr;
                    break;
                }
            }

            if (!cmd)
                continue;

            if (cmd->_invoker && (cmd->_permission.RequiredLevel != secLevel))
            {
                LOG_WARN("sql.sql", "Table `command` has permission {} for '{}' which does not match the core ({}). Overriding.",
                    secLevel, name, cmd->_permission.RequiredLevel);

                cmd->_permission.RequiredLevel = secLevel;
            }

            if (!cmd->_help.empty())
            {
                LOG_ERROR("sql.sql", "Table `command` contains duplicate data for command '{}'. Skipped.", name);
            }
            else
            {
                cmd->_help = help;
            }
        } while (result->NextRow());
    }

    for (auto& [name, cmd] : COMMAND_MAP)
        cmd.ResolveNames(std::string(name));
}

void Warhead::Impl::ChatCommands::ChatCommandNode::ResolveNames(std::string name)
{
    if (_invoker && _help.empty())
        LOG_WARN("sql.sql", "Table `command` is missing help text for command '{}'.", name);

    _name = name;

    for (auto& [subToken, cmd] : _subCommands)
    {
        std::string subName(name);
        subName.push_back(COMMAND_DELIMITER);
        subName.append(subToken);
        cmd.ResolveNames(subName);
    }
}

static void LogCommandUsage(WorldSession const& session, std::string_view cmdStr)
{
    if (AccountMgr::IsPlayerAccount(session.GetSecurity()))
        return;

    Player* player = session.GetPlayer();
    ObjectGuid targetGuid = player->GetTarget();
    uint32 areaId = player->GetAreaId();
    uint32 zoneId = player->GetZoneId();
    std::string areaName = "Unknown";
    std::string zoneName = "Unknown";
    LocaleConstant locale = sWorld->GetDefaultDbcLocale();

    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
    {
        areaName = area->area_name[locale];
    }

    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(zoneId))
    {
        zoneName = zone->area_name[locale];
    }

    std::string logMessage = Warhead::StringFormat("Command: {} [Player: {} ({}) (Account: {}) X: {} Y: {} Z: {} Map: {} ({}) Area: {} ({}) Zone: {} ({}) Selected: {} ({})]",
        cmdStr, player->GetName(), player->GetGUID().ToString(),
        session.GetAccountId(),
        player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(),
        player->FindMap() ? player->FindMap()->GetMapName() : "Unknown",
        areaId, areaName, zoneId, zoneName,
        (player->GetSelectedUnit()) ? player->GetSelectedUnit()->GetName() : "",
        targetGuid.ToString());

    LOG_GM(session.GetAccountId(), logMessage);
}

void Warhead::Impl::ChatCommands::ChatCommandNode::SendCommandHelp(ChatHandler& handler) const
{
    bool const hasInvoker = IsInvokerVisible(handler);
    if (hasInvoker)
    {
        if (auto localizeHelp = sGameLocale->GetChatCommandStringHelpLocale(_name, handler.GetSessionDbcLocale()))
        {
            handler.SendSysMessage(*localizeHelp);
        }
        else if (!_help.empty())
        {
            handler.SendSysMessage(_help);
        }
        else
        {
            handler.PSendSysMessage(LANG_CMD_HELP_GENERIC, _name);
            handler.PSendSysMessage(LANG_CMD_NO_HELP_AVAILABLE, _name);
        }
    }

    bool header = false;
    for (auto it = _subCommands.begin(); it != _subCommands.end(); ++it)
    {
        bool const subCommandHasSubCommand = it->second.HasVisibleSubCommands(handler);
        if (!subCommandHasSubCommand && !it->second.IsInvokerVisible(handler))
            continue;
        if (!header)
        {
            if (!hasInvoker)
                handler.PSendSysMessage(LANG_CMD_HELP_GENERIC, _name);

            handler.SendSysMessage(LANG_SUBCMDS_LIST);
            header = true;
        }

        handler.PSendSysMessage(subCommandHasSubCommand ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it->second._name);
    }
}

namespace Warhead::Impl::ChatCommands
{
    struct FilteredCommandListIterator
    {
        public:
            FilteredCommandListIterator(ChatSubCommandMap const& map, ChatHandler const& handler, std::string_view token)
                : _handler{ handler }, _token{ token }, _it{ map.lower_bound(token) }, _end{ map.end() }
            {
                _skip();
            }

            decltype(auto) operator*() const { return _it.operator*(); }
            decltype(auto) operator->() const { return _it.operator->(); }
            FilteredCommandListIterator& operator++()
            {
                ++_it;
                _skip();
                return *this;
            }
            explicit operator bool() const { return (_it != _end); }
            bool operator!() const { return !static_cast<bool>(*this); }

        private:
            void _skip()
            {
                if ((_it != _end) && !StringStartsWithI(_it->first, _token))
                    _it = _end;
                while ((_it != _end) && !_it->second.IsVisible(_handler))
                {
                    ++_it;
                    if ((_it != _end) && !StringStartsWithI(_it->first, _token))
                        _it = _end;
                }
            }

            ChatHandler const& _handler;
            std::string_view const _token;
            ChatSubCommandMap::const_iterator _it, _end;
    };
}

/*static*/ bool Warhead::Impl::ChatCommands::ChatCommandNode::TryExecuteCommand(ChatHandler& handler, std::string_view cmdStr)
{
    ChatCommandNode const* cmd = nullptr;
    ChatSubCommandMap const* map = &GetTopLevelMap();

    while (!cmdStr.empty() && (cmdStr.front() == COMMAND_DELIMITER))
        cmdStr.remove_prefix(1);

    while (!cmdStr.empty() && (cmdStr.back() == COMMAND_DELIMITER))
        cmdStr.remove_suffix(1);

    std::string_view oldTail = cmdStr;
    while (!oldTail.empty())
    {
        /* oldTail = token DELIMITER newTail */
        auto [token, newTail] = tokenize(oldTail);
        ASSERT(!token.empty());

        FilteredCommandListIterator it1(*map, handler, token);
        if (!it1)
            break; /* no matching subcommands found */

        if (!StringEqualI(it1->first, token))
        { /* ok, so it1 points at a partially matching subcommand - let's see if there are others */
            auto it2 = it1;
            ++it2;

            if (it2)
            { /* there are multiple matching subcommands - print possibilities and return */
                if (cmd)
                    handler.PSendSysMessage(LANG_SUBCMD_AMBIGUOUS, cmd->_name, COMMAND_DELIMITER, token);
                else
                    handler.PSendSysMessage(LANG_CMD_AMBIGUOUS, token);

                handler.PSendSysMessage(it1->second.HasVisibleSubCommands(handler) ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it1->first);
                do
                {
                    handler.PSendSysMessage(it2->second.HasVisibleSubCommands(handler) ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it2->first);
                } while (++it2);

                return true;
            }
        }

        /* now we matched exactly one subcommand, and it1 points to it; go down the rabbit hole */
        cmd = &it1->second;
        map = &cmd->_subCommands;

        oldTail = newTail;
    }

    if (cmd)
    { /* if we matched a command at some point, invoke it */
        handler.SetSentErrorMessage(false);
        if (cmd->IsInvokerVisible(handler) && cmd->_invoker(&handler, oldTail))
        { /* invocation succeeded, log this */
            if (!handler.IsConsole())
                LogCommandUsage(*handler.GetSession(), cmdStr);
        }
        else if (!handler.HasSentErrorMessage()) /* invocation failed, we should show usage */
        {
            if (!sScriptMgr->CanExecuteCommand(handler, cmdStr))
            {
                return true;
            }

            cmd->SendCommandHelp(handler);
            handler.SetSentErrorMessage(true);
        }

        return true;
    }

    if (!sScriptMgr->CanExecuteCommand(handler, cmdStr))
    {
        return true;
    }

    return false;
}

/*static*/ void Warhead::Impl::ChatCommands::ChatCommandNode::SendCommandHelpFor(ChatHandler& handler, std::string_view cmdStr)
{
    ChatCommandNode const* cmd = nullptr;
    ChatSubCommandMap const* map = &GetTopLevelMap();

    for (std::string_view token : Warhead::Tokenize(cmdStr, COMMAND_DELIMITER, false))
    {
        FilteredCommandListIterator it1(*map, handler, token);
        if (!it1)
        { /* no matching subcommands found */
            if (cmd)
            {
                cmd->SendCommandHelp(handler);
                handler.PSendSysMessage(LANG_SUBCMD_INVALID, cmd->_name, COMMAND_DELIMITER, token);
            }
            else
                handler.PSendSysMessage(LANG_CMD_INVALID, token);
            return;
        }

        if (!StringEqualI(it1->first, token))
        { /* ok, so it1 points at a partially matching subcommand - let's see if there are others */
            auto it2 = it1;
            ++it2;

            if (it2)
            { /* there are multiple matching subcommands - print possibilities and return */
                if (cmd)
                    handler.PSendSysMessage(LANG_SUBCMD_AMBIGUOUS, cmd->_name, COMMAND_DELIMITER, token);
                else
                    handler.PSendSysMessage(LANG_CMD_AMBIGUOUS, token);

                handler.PSendSysMessage(it1->second.HasVisibleSubCommands(handler) ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it1->first);
                do
                {
                    handler.PSendSysMessage(it2->second.HasVisibleSubCommands(handler) ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it2->first);
                } while (++it2);

                return;
            }
        }

        cmd = &it1->second;
        map = &cmd->_subCommands;
    }

    if (cmd)
        cmd->SendCommandHelp(handler);
    else if (cmdStr.empty())
    {
        FilteredCommandListIterator it(*map, handler, "");
        if (!it)
            return;
        handler.SendSysMessage(LANG_AVAILABLE_CMDS);
        do
        {
            handler.PSendSysMessage(it->second.HasVisibleSubCommands(handler) ? LANG_SUBCMDS_LIST_ENTRY_ELLIPSIS : LANG_SUBCMDS_LIST_ENTRY, it->second._name);
        } while (++it);
    }
    else
        handler.PSendSysMessage(LANG_CMD_INVALID, cmdStr);
}

/*static*/ std::vector<std::string> Warhead::Impl::ChatCommands::ChatCommandNode::GetAutoCompletionsFor(ChatHandler const& handler, std::string_view cmdStr)
{
    std::string path;
    ChatCommandNode const* cmd = nullptr;
    ChatSubCommandMap const* map = &GetTopLevelMap();

    while (!cmdStr.empty() && (cmdStr.front() == COMMAND_DELIMITER))
        cmdStr.remove_prefix(1);

    while (!cmdStr.empty() && (cmdStr.back() == COMMAND_DELIMITER))
        cmdStr.remove_suffix(1);

    std::string_view oldTail = cmdStr;
    while (!oldTail.empty())
    {
        /* oldTail = token DELIMITER newTail */
        auto [token, newTail] = tokenize(oldTail);
        ASSERT(!token.empty());
        FilteredCommandListIterator it1(*map, handler, token);
        if (!it1)
            break; /* no matching subcommands found */

        if (!StringEqualI(it1->first, token))
        { /* ok, so it1 points at a partially matching subcommand - let's see if there are others */
            auto it2 = it1;
            ++it2;

            if (it2)
            { /* there are multiple matching subcommands - terminate here and show possibilities */
                std::vector<std::string> vec;
                auto possibility = ([prefix = std::string_view(path), suffix = std::string_view(newTail)](std::string_view match)
                {
                    if (prefix.empty())
                    {
                        return Warhead::StringFormat("{}{}{}", match, COMMAND_DELIMITER, suffix);
                    }
                    else
                    {
                        return Warhead::StringFormat("{}{}{}{}{}", prefix, COMMAND_DELIMITER, match, COMMAND_DELIMITER, suffix);
                    }
                });

                vec.emplace_back(possibility(it1->first));

                do vec.emplace_back(possibility(it2->first));
                while (++it2);

                return vec;
            }
        }

        /* now we matched exactly one subcommand, and it1 points to it; go down the rabbit hole */
        if (path.empty())
            path.assign(it1->first);
        else
        {
            path = Warhead::StringFormat("{}{}{}", path, COMMAND_DELIMITER, it1->first);
        }

        cmd = &it1->second;
        map = &cmd->_subCommands;

        oldTail = newTail;
    }

    if (!oldTail.empty())
    { /* there is some trailing text, leave it as is */
        if (cmd)
        { /* if we matched a command at some point, auto-complete it */
            return { Warhead::StringFormat("{}{}{}", path, COMMAND_DELIMITER, oldTail) };
        }
        else
            return {};
    }
    else
    { /* offer all subcommands */
        auto possibility = ([prefix = std::string_view(path)](std::string_view match)
        {
            if (prefix.empty())
                return std::string(match);
            else
            {
                return Warhead::StringFormat("{}{}{}", prefix, COMMAND_DELIMITER, match);
            }
        });

        std::vector<std::string> vec;
        for (FilteredCommandListIterator it(*map, handler, ""); it; ++it)
            vec.emplace_back(possibility(it->first));
        return vec;
    }
}

bool Warhead::Impl::ChatCommands::ChatCommandNode::IsInvokerVisible(ChatHandler const& who) const
{
    if (!_invoker)
        return false;

    if (who.IsConsole() && (_permission.AllowConsole == Warhead::ChatCommands::Console::No))
        return false;

    if (who.IsConsole() && (_permission.AllowConsole == Warhead::ChatCommands::Console::Yes))
        return true;

    return !who.IsConsole() && who.IsAvailable(_permission.RequiredLevel);
}

bool Warhead::Impl::ChatCommands::ChatCommandNode::HasVisibleSubCommands(ChatHandler const& who) const
{
    for (auto it = _subCommands.begin(); it != _subCommands.end(); ++it)
        if (it->second.IsVisible(who))
            return true;

    return false;
}

void Warhead::ChatCommands::LoadCommandMap() { Warhead::Impl::ChatCommands::ChatCommandNode::LoadCommandMap(); }
void Warhead::ChatCommands::InvalidateCommandMap() { Warhead::Impl::ChatCommands::ChatCommandNode::InvalidateCommandMap(); }
bool Warhead::ChatCommands::TryExecuteCommand(ChatHandler& handler, std::string_view cmd) { return Warhead::Impl::ChatCommands::ChatCommandNode::TryExecuteCommand(handler, cmd); }
void Warhead::ChatCommands::SendCommandHelpFor(ChatHandler& handler, std::string_view cmd) { Warhead::Impl::ChatCommands::ChatCommandNode::SendCommandHelpFor(handler, cmd); }
std::vector<std::string> Warhead::ChatCommands::GetAutoCompletionsFor(ChatHandler const& handler, std::string_view cmd) { return Warhead::Impl::ChatCommands::ChatCommandNode::GetAutoCompletionsFor(handler, cmd); }
