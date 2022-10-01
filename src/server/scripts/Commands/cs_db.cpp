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

#include "Chat.h"
#include "DatabaseEnv.h"
#include "DatabaseEnvFwd.h"
#include "ScriptObject.h"

using namespace Warhead::ChatCommands;

class db_commandscript : public CommandScript
{
public:
    db_commandscript() : CommandScript("db_commandscript") { }

    [[nodiscard]] ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable dbCommandTable =
        {
            { "info",                   HandleDBInfoCommand,               SEC_ADMINISTRATOR, Console::Yes },
            { "add dynamic connects",   HandleDBAddDynamicConnectsCommand,  SEC_ADMINISTRATOR, Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "db",  dbCommandTable },
        };

        return commandTable;
    }

    static bool HandleDBInfoCommand(ChatHandler* handler)
    {
        AuthDatabase.GetPoolInfo([handler](std::string_view info)
        {
            handler->PSendSysMessage("# {}", info);
        });

        handler->PSendSysMessage("# --");

        CharacterDatabase.GetPoolInfo([handler](std::string_view info)
        {
            handler->PSendSysMessage("# {}", info);
        });

        handler->PSendSysMessage("# --");

        WorldDatabase.GetPoolInfo([handler](std::string_view info)
        {
            handler->PSendSysMessage("# {}", info);
        });

        handler->PSendSysMessage("# --");
        return true;
    }

    static bool HandleDBAddDynamicConnectsCommand(ChatHandler* handler, uint8 dbType, bool isAsync)
    {
        switch (dbType)
        {
            case 1:
                isAsync ? AuthDatabase.OpenDynamicAsyncConnect() : AuthDatabase.OpenDynamicSyncConnect();
                handler->PSendSysMessage("Added new {} dynamic connection for {} DB", isAsync ? "async" : "sync", AuthDatabase.GetPoolName());
                break;
            case 2:
                isAsync ? CharacterDatabase.OpenDynamicAsyncConnect() : CharacterDatabase.OpenDynamicSyncConnect();
                handler->PSendSysMessage("Added new {} dynamic connection for {} DB", isAsync ? "async" : "sync", CharacterDatabase.GetPoolName());
                break;
            case 3:
                isAsync ? WorldDatabase.OpenDynamicAsyncConnect() : WorldDatabase.OpenDynamicSyncConnect();
                handler->PSendSysMessage("Added new {} dynamic connection for {} DB", isAsync ? "async" : "sync", WorldDatabase.GetPoolName());
                break;
            default:
                handler->PSendSysMessage("Incorrect db type: {}", dbType);
                break;
        }

        return true;
    }
};

void AddSC_db_commandscript()
{
    new db_commandscript();
}
