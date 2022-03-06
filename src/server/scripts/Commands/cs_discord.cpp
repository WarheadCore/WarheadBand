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
#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace Warhead::ChatCommands;

class discord_commandscript : public CommandScript
{
public:
    discord_commandscript() : CommandScript("discord_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable achievementCommandTable =
        {
            { "add",    HandleDiscordAddCommand,    SEC_PLAYER,     Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "discord", achievementCommandTable }
        };

        return commandTable;
    }

    static bool HandleDiscordAddCommand(ChatHandler* handler)
    {
        Player* target = handler->GetPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_PLAYER);
            handler->SetSentErrorMessage(true);
            return false;
        }

        sDiscord->AddCode(target->GetGUID());

        return true;
    }
};

void AddSC_discord_commandscript()
{
    new discord_commandscript();
}
