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

        if (sReforgeMgr->ChangeItem(player, item, 36, 1000))
            handler->PSendSysMessage("> Item changed");
        else
            handler->PSendSysMessage("> Item not changed");

        return true;
    }
};

class Reforge_Player : public PlayerScript
{
public:
    Reforge_Player() : PlayerScript("Reforge_Player") { }

    void OnBeforeApplyItemBonuses(Player* player, Item* item, uint32& statsCount) override
    {
        sReforgeMgr->OnBeforeApplyItemBonuses(player, item, statsCount);
    }

    void OnApplyItemBonuses(Player* player, Item* item, uint32 statIndex, uint32& statType, int32& value) override
    {
        sReforgeMgr->OnApplyItemBonuses(player, item, statIndex, statType, value);
    }
};

// Group all custom scripts
void AddSC_Reforge()
{
    new Reforge_CS();
    new Reforge_Player();
}
