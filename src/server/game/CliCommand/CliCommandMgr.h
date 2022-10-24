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

#ifndef _CLI_COMMAND_MGR_H_
#define _CLI_COMMAND_MGR_H_

#include "Define.h"
#include "MPSCQueue.h"
#include <functional>
#include <string>
#include <string_view>

struct WH_GAME_API CliCommand
{
public:
    using Print = std::function<void(std::string_view)>;
    using CommandFinished = std::function<void(bool)>;

    CliCommand(std::string_view command, Print&& print, CommandFinished&& commandFinish);
    ~CliCommand() = default;

    void ExecuteCommand();

    std::atomic<CliCommand*> QueueLink;

private:
    std::string _command;
    Print _print;
    CommandFinished _commandFinish;

    CliCommand(CliCommand const& right) = delete;
    CliCommand& operator=(CliCommand const& right) = delete;
};

class WH_GAME_API CliCommandMgr
{
public:
    CliCommandMgr() = default;
    ~CliCommandMgr() = default;

    static CliCommandMgr* instance();

    void AddCommand(std::string_view command, CliCommand::Print&& print, CliCommand::CommandFinished&& commandFinish);
    void ProcessCliCommands();

private:
    MPSCQueue<CliCommand, &CliCommand::QueueLink> _queue;

    CliCommandMgr(CliCommandMgr const& right) = delete;
    CliCommandMgr& operator=(CliCommandMgr const& right) = delete;
};

#define sCliCommandMgr CliCommandMgr::instance()

#endif
