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

#include "Chat.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "StringConvert.h"
#include "UnbindInstance.h"
#include "ModuleLocale.h"
#include "GameTime.h"
#include "AccountMgr.h"

using namespace Warhead::ChatCommands;

class UnbindInstance_CS : public CommandScript
{
public:
    UnbindInstance_CS() : CommandScript("UnbindInstance_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "ui", HandleUnbindInstanceCommand, SEC_PLAYER, Console::No }
        };

        return commandTable;
    }

    static bool HandleUnbindInstanceCommand(ChatHandler* handler, Variant<uint16, EXACT_SEQUENCE("all")> mapArg, Optional<uint8> difficultyArg)
    {
        if (!sUI->IsEnableCommand() && AccountMgr::IsPlayerAccount(handler->GetPlayer()->GetSession()->GetSecurity()))
        {
            handler->PSendSysMessage("> This comamnd is disable");
            handler->SetSentErrorMessage(true);
            return true;
        }

        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetPlayer();

        if (!player)
            return false;

        uint16 counter = 0;
        uint16 mapId = 0;

        if (mapArg.holds_alternative<uint16>())
        {
            mapId = mapArg.get<uint16>();
            if (!mapId)
                return false;
        }

        bool isExistDisabledMap{ false };
        std::vector<std::pair<uint32/*mapID*/, Difficulty>> toUnbind;

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            auto const& boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), Difficulty(i));
            if (boundInstances.empty())
                continue;

            for (auto const& [bindingMapID, instancePlayerBind] : boundInstances)
            {
                MapEntry const* mapEntry = sMapStore.LookupEntry(bindingMapID);

                if (sUI->IsDisabledMap(bindingMapID))
                {
                    // Instance {} ({}) forbidden to unbind
                    sModuleLocale->SendPlayerMessage(player, "UI_COMMAND_FORBIDEN_RESET", bindingMapID, mapEntry ? mapEntry->name[handler->GetSessionDbcLocale()] : "<unknown>");
                    continue;
                }

                InstanceSave const* save = instancePlayerBind.save;
                if (bindingMapID != player->GetMapId() && (!mapId || mapId == bindingMapID) && (!difficultyArg || difficultyArg == save->GetDifficulty()))
                {
                    Seconds resetTime = instancePlayerBind.extended ? Seconds(save->GetExtendedResetTime()) : Seconds(save->GetResetTime());
                    Seconds ttr = resetTime >= GameTime::GetGameTime() ? resetTime - GameTime::GetGameTime() : 0s;

                    // Unbinding map: {} ({}), inst: {}, perm: {}, diff: {}, canReset: {}, TTR: {}{}
                    sModuleLocale->SendPlayerMessage(player, "UI_COMMAND_UNBINDING",
                        bindingMapID, mapEntry ? mapEntry->name[handler->GetSessionDbcLocale()] : "<unknown>", save->GetInstanceId(), instancePlayerBind.perm ? "yes" : "no", save->GetDifficulty(),
                        save->CanReset() ? "yes" : "no", Warhead::Time::ToTimeString(ttr), instancePlayerBind.extended ? " (extended)" : "");

                    toUnbind.emplace_back(std::pair{ bindingMapID, Difficulty(i) });
                }
            }
        }

        // Instances unbound: {}
        sModuleLocale->SendPlayerMessage(player, "UI_COMMAND_UNBOUND", toUnbind.size());

        if (toUnbind.empty())
            return true;

        for (auto const& [toUnbindMapID, diff] : toUnbind)
            sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), toUnbindMapID, diff, true, player);

        return true;
    }
};

class UnbindInstance_Creature : public CreatureScript
{
public:
    UnbindInstance_Creature() : CreatureScript("UnbindInstance_Creature") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!MOD_CONF_GET_BOOL("UnbindInsance.Enable"))
            return true;

        sUI->SendGossipHello(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (sender >= GOSSIP_SENDER_DIFFICULTY)
        {
            sUI->BindInfo(player, creature, sender, action);
            return true;
        }

        switch (action)
        {
            case GOSSIP_ACTION_INFO_MAIN_MENU:
                sUI->SendGossipHello(player, creature);
                break;
            case GOSSIP_ACTION_INFO_DUNGEON_HEROIC:
                sUI->SendBindList(player, creature, 1, false);
                break;
            case GOSSIP_ACTION_INFO_RAID_10_NORMAL:
                sUI->SendBindList(player, creature, 0);
                break;
            case GOSSIP_ACTION_INFO_RAID_25_NORMAL:
                sUI->SendBindList(player, creature, 1);
                break;
            case GOSSIP_ACTION_INFO_RAID_10_HEROIC:
                sUI->SendBindList(player, creature, 2);
                break;
            case GOSSIP_ACTION_INFO_RAID_25_HEROIC:
                sUI->SendBindList(player, creature, 3);
                break;
            default:
                break;
        }

        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, char const* code) override
    {
        ClearGossipMenuFor(player);
        sUI->Unbind(player, creature, sender, action, Warhead::StringTo<uint32>(code).value_or(0));

        return true;
    }
};

class UnbindInstance_World : public WorldScript
{
public:
    UnbindInstance_World() : WorldScript("UnbindInstance_World") { }

    void OnStartup() override
    {
        sUI->Init();
    }
};

// Group all custom scripts
void AddSC_UnbindInstance()
{
    new UnbindInstance_CS();
    new UnbindInstance_Creature();
    new UnbindInstance_World();
}
