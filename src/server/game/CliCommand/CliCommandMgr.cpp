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

#include "CliCommandMgr.h"
#include "Chat.h"
#include <utility>

CliCommand::CliCommand(std::string_view command, Print&& print, CommandFinished&& commandFinished) :
    _command(command), _print(std::move(print)), _commandFinish(std::move(commandFinished)) { }

void CliCommand::ExecuteCommand()
{
    CliHandler handler(&_print);
    handler.ParseCommands(_command);

    if (_commandFinish)
        _commandFinish(!handler.HasSentErrorMessage());
}

/*static*/ CliCommandMgr* CliCommandMgr::instance()
{
    static CliCommandMgr instance;
    return &instance;
}

void CliCommandMgr::AddCommand(std::string_view command, CliCommand::Print&& print, CliCommand::CommandFinished&& commandFinish)
{
    auto cmd = new CliCommand(command, std::move(print), std::move(commandFinish));
    _queue.Enqueue(cmd);
}

// This handles the issued and queued CLI commands
void CliCommandMgr::ProcessCliCommands()
{
    CliCommand* command{ nullptr };

    while (_queue.Dequeue(command))
    {
        LOG_INFO("server.worldserver", "CLI command under processing...");

        command->ExecuteCommand();
        delete command;
    }
}
