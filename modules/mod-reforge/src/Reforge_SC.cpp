/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Reforge.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"

using namespace Warhead::ChatCommands;

class Reforge_CS : public CommandScript
{
public:
    Reforge_CS() : CommandScript("Reforge_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable reforgeCommandTable =
        {
            { "test", HandleReforgeTestCommand, SEC_ADMINISTRATOR, Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "reforge", reforgeCommandTable }
        };

        return commandTable;
    }

    static bool HandleReforgeTestCommand(ChatHandler* handler)
    {
        auto const& player = handler->GetPlayer();

        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_HEAD);
        if (!item)
        {
            handler->PSendSysMessage("> Not found item");
            return true;
        }

        handler->PSendSysMessage("> Item {}", item->GetGUID().ToString());

        //item->

        return true;
    }
};

// Group all custom scripts
void AddSC_Reforge()
{
    new Reforge_CS();
}
