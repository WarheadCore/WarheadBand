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

#include "CreatureRespawn.h"
#include "ScriptObject.h"
#include "Chat.h"

using namespace Warhead::ChatCommands;

class CreatureRespawn_CS : public CommandScript
{
public:
    CreatureRespawn_CS() : CommandScript("CreatureRespawn_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable boarCommandTable =
        {
            { "reload", HandleCRReloadCommand, SEC_ADMINISTRATOR,  Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "cr", boarCommandTable }
        };

        return commandTable;
    }

    static bool HandleCRReloadCommand(ChatHandler* handler)
    {
        if (!sCreatureRespawnMgr->IsEnable())
        {
            handler->PSendSysMessage("> Module disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }

        sCreatureRespawnMgr->LoadDataFromDB();
        handler->PSendSysMessage("> Creature respawn db data reloaded");
        return true;
    }
};

class CreatureRespawn_AllCreature : public AllCreatureScript
{
public:
    CreatureRespawn_AllCreature() : AllCreatureScript("CreatureRespawn_AllCreature") { }

    void OnCreatureRespawn(Creature* creature) override
    {
        sCreatureRespawnMgr->OnCreatureRespawn(creature);
    }
};

class CreatureRespawn_Creature : public CreatureScript
{
public:
    CreatureRespawn_Creature() : CreatureScript("CreatureRespawn_Creature") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        sCreatureRespawnMgr->OnGossipHello(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        sCreatureRespawnMgr->OnGossipSelect(player, creature, sender, action);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, std::string_view code) override
    {
        sCreatureRespawnMgr->OnGossipSelectCode(player, creature, sender, action, code);
        return true;
    }
};

class CreatureRespawn_Player : public PlayerScript
{
public:
    CreatureRespawn_Player() : PlayerScript("CreatureRespawn_Player") {}

    void OnCreatureKill(Player* player, Creature* creature) override
    {
        sCreatureRespawnMgr->OnCreatureKill(player, creature);
    }
};

class CreatureRespawn_World : public WorldScript
{
public:
    CreatureRespawn_World() : WorldScript("CreatureRespawn_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sCreatureRespawnMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sCreatureRespawnMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_CreatureRespawn()
{
    new CreatureRespawn_CS();
    new CreatureRespawn_AllCreature();
    new CreatureRespawn_Creature();
    new CreatureRespawn_Player();
    new CreatureRespawn_World();
}
