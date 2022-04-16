/*
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

#include "Language.h"
#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "AnticheatMgr.h"
#include "Player.h"

using namespace Warhead::ChatCommands;

class anticheat_commandscript : public CommandScript
{
public:
    anticheat_commandscript() : CommandScript("anticheat_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable anticheatCommandTable =
        {
            { "player", HandleAntiCheatPlayerCommand,   SEC_GAMEMASTER,     Console::Yes },
            { "delete", HandleAntiCheatDeleteCommand,   SEC_ADMINISTRATOR,  Console::Yes },
            { "jail",   HandleAnticheatJailCommand,     SEC_GAMEMASTER,     Console::No },
            { "parole", HandleAnticheatParoleCommand,   SEC_GAMEMASTER,     Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "anticheat", anticheatCommandTable},
        };

        return commandTable;
    }

    static bool HandleAnticheatJailCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!sAnticheatMgr->IsEnable())
        {
            handler->PSendSysMessage("> Anticheat disable");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target)
            target = PlayerIdentifier::FromTarget(handler);

        if (!target)
            return false;

        auto player = target->GetConnectedPlayer();
        if (!player)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // teleport both to jail.
        handler->GetSession()->GetPlayer()->TeleportTo(1,16226.5f,16403.6f,-64.5f,3.2f);

        WorldLocation loc;
        loc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f); // GM Jail Location
        player->TeleportTo(loc);
        player->SetHomebind(loc, 876); // GM Jail Homebind location
        player->CastSpell(player, static_cast<uint32>(SpellMisc::Shackles)); // Shackle him in place to ensure no exploit happens for jail break attempt
        player->AddAura(static_cast<uint32>(SpellMisc::LfgDeserter), player);
        player->AddAura(static_cast<uint32>(SpellMisc::BgDeserter), player);

        return true;
    }

    static bool HandleAnticheatParoleCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!sAnticheatMgr->IsEnable())
        {
            handler->PSendSysMessage("> Anticheat disable");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target)
            target = PlayerIdentifier::FromTarget(handler);

        if (!target)
            return false;

        auto player = target->GetConnectedPlayer();
        if (!player)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        WorldLocation Aloc;
        WorldLocation Hloc;
        Aloc = WorldLocation(0, -8833.37f, 628.62f, 94.00f, 1.06f);// Stormwind
        Hloc = WorldLocation(1, 1569.59f, -4397.63f, 16.06f, 0.54f);// Orgrimmar

        if (player->GetTeamId() == TEAM_ALLIANCE)
        {
            player->TeleportTo(0, -8833.37f, 628.62f, 94.00f, 1.06f);//Stormwind
            player->SetHomebind(Aloc, 1519);// Stormwind Homebind location
        }
        else
        {
            player->TeleportTo(1, 1569.59f, -4397.63f, 7.7f, 0.54f);//Orgrimmar
            player->SetHomebind(Hloc, 1653); // Orgrimmar Homebind location
        }

        player->CastSpell(player, static_cast<uint32>(SpellMisc::Shackles)); // Shackle him in place to ensure no exploit happens for jail break attempt
        player->AddAura(static_cast<uint32>(SpellMisc::LfgDeserter), player);
        player->AddAura(static_cast<uint32>(SpellMisc::BgDeserter), player);
        sAnticheatMgr->AnticheatDeleteCommand(player->GetGUID());// deletes auto reports on player

        return true;
    }

    static bool HandleAntiCheatDeleteCommand(ChatHandler* handler, Variant<PlayerIdentifier, EXACT_SEQUENCE("all")> command)
    {
        if (!sAnticheatMgr->IsEnable())
        {
            handler->PSendSysMessage("> Anticheat disable");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (command.holds_alternative<EXACT_SEQUENCE("all")>())
        {
            sAnticheatMgr->AnticheatDeleteCommand(ObjectGuid::Empty);
            return true;
        }

        PlayerIdentifier target = command.get<PlayerIdentifier>();
        auto targetGuid = target.GetGUID();

        if (targetGuid.IsEmpty())
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        sAnticheatMgr->AnticheatDeleteCommand(targetGuid);
        return true;
    }

    static bool HandleAntiCheatPlayerCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!sAnticheatMgr->IsEnable())
        {
            handler->PSendSysMessage("> Anticheat disable");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto targetGuid = target->GetGUID();

        float average = sAnticheatMgr->GetAverage(targetGuid);
        uint32 totalReports = sAnticheatMgr->GetTotalReports(targetGuid);
        uint32 speedReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::SpeedHack);
        uint32 flyReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::FlyHack);
        uint32 jumpReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::JumpHack);
        uint32 waterWalkReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::WaterWalk);
        uint32 teleportPlaneReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::TeleportPlaneHack);
        uint32 climbReports = sAnticheatMgr->GetTypeReports(targetGuid, AnticheatDetectionType::ClimbHack);

        handler->PSendSysMessage("Information about player {}", target->GetName());
        handler->PSendSysMessage("Average: {} || Total Reports: {} ", average, totalReports);
        handler->PSendSysMessage("Speed Reports: {} || Fly Reports: {} || Jump Reports: {}", speedReports, flyReports, jumpReports);
        handler->PSendSysMessage("Walk On Water Reports: {} || Teleport To Plane Reports: {}", waterWalkReports, teleportPlaneReports);
        handler->PSendSysMessage("Climb Reports: {}", climbReports);
        return true;
    }
};

void AddSC_anticheat_commandscript()
{
    new anticheat_commandscript();
}
