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

#include "AnticheatMgr.h"
#include "Log.h"
#include "MapMgr.h"
#include "Player.h"
#include "SpellAuras.h"
#include "ModulesConfig.h"
#include "ChatTextBuilder.h"
#include "GameTime.h"
#include "ModuleLocale.h"
#include "Chat.h"

constexpr float CLIMB_ANGLE = 1.87f;

AnticheatMgr::~AnticheatMgr()
{
    _players.clear();
}

/*static*/ AnticheatMgr* AnticheatMgr::instance()
{
    static AnticheatMgr instance;
    return &instance;
}

void AnticheatMgr::LoadConfig(bool /*reload*/)
{
    _enable = MOD_CONF_GET_BOOL("Anticheat.Enable");
    _enableGM = MOD_CONF_GET_BOOL("Anticheat.GM.Enable");

    _detectFly = MOD_CONF_GET_BOOL("Anticheat.Detect.FlyHack");
    _detectJump = MOD_CONF_GET_BOOL("Anticheat.Detect.JumpHack");
    _detectWaterWalk = MOD_CONF_GET_BOOL("Anticheat.Detect.WaterWalkHack");
    _detectTelePlain = MOD_CONF_GET_BOOL("Anticheat.Detect.TelePlaneHack");
    _detectSpeed = MOD_CONF_GET_BOOL("Anticheat.Detect.SpeedHack");
    _detectClimb = MOD_CONF_GET_BOOL("Anticheat.Detect.ClimbHack");
    _detectTeleport = MOD_CONF_GET_BOOL("Anticheat.Detect.TelePortHack");
    _detectZaxis = MOD_CONF_GET_BOOL("Anticheat.Detect.ZaxisHack");
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo* /*movementInfo*/, uint32 opcode)
{
    if (!_detectJump)
        return;

    if (GetLastOpcode(player->GetGUID()) == MSG_MOVE_JUMP && opcode == MSG_MOVE_JUMP)
    {
        BuildReport(player, AnticheatDetectionType::JumpHack);
        LOG_WARN("module.anticheat", "Anticheat: Jump-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    }
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectWaterWalk)
        return;

    // ghost can water walk
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    // Prevents the False Positive for water walking when you ressurrect.
    // Aura 15007 (Resurrection sickness) is given while dead before returning back to life.
    if (player->HasAuraType(SPELL_AURA_GHOST) && player->HasAura(15007))
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    /* Thanks to @LilleCarl */
    if (lastMovement->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && movementInfo->HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
    {
        if (player->HasAuraType(SPELL_AURA_WATER_WALK) ||
            player->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
            player->HasAuraType(SPELL_AURA_SAFE_FALL))
            return;
    }
    else if (!lastMovement->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !movementInfo->HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
        return;

    BuildReport(player, AnticheatDetectionType::WaterWalk);
    LOG_WARN("module.anticheat", "Anticheat: Walk on Water - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectFly)
        return;

    if (player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)) // Overkill but wth
        return;

    /* Thanks to @LilleCarl for info to check extra flag */
    bool stricterChecks{ true };
    if (MOD_CONF_GET_BOOL("Anticheat.StricterFlyHackCheck"))
        stricterChecks = !(movementInfo->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && !player->IsInWater());

    if (!movementInfo->HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && !movementInfo->HasMovementFlag(MOVEMENTFLAG_FLYING) && stricterChecks)
        return;

    BuildReport(player, AnticheatDetectionType::FlyHack);
    LOG_WARN("module.anticheat", "Anticheat: Fly-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectTelePlain)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    if (lastMovement->pos.GetPositionZ() != 0.0f || movementInfo->pos.GetPositionZ() != 0.0f)
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    float x, y, z;
    player->GetPosition(x, y, z);

    // we are not really walking there
    if (std::fabs(player->GetMap()->GetHeight(x, y, z) - z) <= 1.0f)
        return;

    BuildReport(player, AnticheatDetectionType::TeleportPlaneHack);
    LOG_WARN("module.anticheat", "Anticheat: Teleport To Plane - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
}

void AnticheatMgr::IgnoreControlHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!MOD_CONF_GET_BOOL("Anticheat.IgnoreControlHack"))
        return;

    if (!player->HasUnitState(UNIT_STATE_ROOT) || player->GetVehicle())
        return;

    auto const& pos = player->GetPosition();

    if (movementInfo->pos.GetPositionX() == pos.GetPositionX() || movementInfo->pos.GetPositionY() == pos.GetPositionY())
        return;

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_IGNORE_CONTROL", player->GetName());

    LOG_WARN("module.anticheat", "Anticheat: Ignore Control - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::IngnoreControl);
}

void AnticheatMgr::ZAxisHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectZaxis)
        return;

    // if we are a ghost we can walk on water may false flag z -axis
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING))
        return;

    switch (player->GetAreaId())
    {
    case 4281: // Acherus: The Ebon Hold
    case 4342: // Acherus: The Ebon Hold
        return;
    default:
        break; // Should never happen
    }

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    if (lastMovement->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && movementInfo->HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
        return;

    auto lastZ = lastMovement->pos.GetPositionY();
    auto newZ = movementInfo->pos.GetPositionY();

    if (lastZ != newZ)
        return;

    auto lastX = lastMovement->pos.GetPositionX();
    auto newX = movementInfo->pos.GetPositionX();

    auto lastY = lastMovement->pos.GetPositionY();
    auto newY = movementInfo->pos.GetPositionY();    

    auto xDiff = std::fabs(lastX - newX);
    auto yDiff = std::fabs(lastY - newY);

    if (xDiff <= 0.0f || yDiff <= 0.0f)
        return;

    float groundZVmap = player->GetMap()->GetHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), true, 50.0f);
    float groundZDyntree = player->GetMap()->GetDynamicMapTree().getHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 50.0f, player->GetPhaseMask());
    float groundZ = std::max<float>(groundZVmap, groundZDyntree);

    if (player->GetPositionZ() < groundZ + 5.0f)
        return;

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_ALERT", player->GetName());

    LOG_WARN("module.anticheat", "Anticheat: Ignore Zaxis - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::ZaxisHack);
}

void AnticheatMgr::TeleportHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectTeleport)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    if (lastMovement->pos.GetPositionX() == movementInfo->pos.GetPositionX())
        return;

    float lastX = lastMovement->pos.GetPositionX();
    float newX = movementInfo->pos.GetPositionX();

    float lastY = lastMovement->pos.GetPositionY();
    float newY = movementInfo->pos.GetPositionY();

    float xDiff = std::fabs(lastX - newX);
    float yDiff = std::fabs(lastY - newY);

    if (player->CanTeleport())
    {
        player->SetCanTeleport(false);
        return;
    }

    if (xDiff < 50.0f || yDiff < 50.0f)
        return;

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_TELEPORT", player->GetName());

    LOG_WARN("module.anticheat", "Anticheat: Teleport-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::TeleportHack);
}

void AnticheatMgr::StartHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_enable)
        return;

    if (player->IsGameMaster())
        return;

    if (!AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()) && !_enableGM)
        return;

    ObjectGuid playerGuid = player->GetGUID();

    if (player->IsInFlight() || player->GetTransport() || player->GetVehicle())
    {
        SetLastMovementInfo(playerGuid, movementInfo);
        SetLastOpcode(playerGuid, opcode);
        return;
    }

    SpeedHackDetection(player, movementInfo);
    FlyHackDetection(player, movementInfo);
    WalkOnWaterHackDetection(player, movementInfo);
    JumpHackDetection(player, movementInfo, opcode);
    TeleportPlaneHackDetection(player, movementInfo);
    ClimbHackDetection(player, movementInfo, opcode);
    TeleportHackDetection(player, movementInfo);
    IgnoreControlHackDetection(player, movementInfo);
    ZAxisHackDetection(player, movementInfo);

    SetLastMovementInfo(playerGuid, movementInfo);
    SetLastOpcode(playerGuid, opcode);
}

// basic detection
void AnticheatMgr::ClimbHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_detectClimb)
        return;

    if (opcode != MSG_MOVE_HEARTBEAT || GetLastOpcode(player->GetGUID()) != MSG_MOVE_HEARTBEAT)
        return;

    // in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
    if (player->IsInWater() || player->IsFlying() || player->IsFalling())
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement || lastMovement->HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    Position playerPos = player->GetPosition();
    if (Position::NormalizeOrientation(std::tan(std::fabs(playerPos.GetPositionZ() - movementInfo->pos.GetPositionZ()) / movementInfo->pos.GetExactDist2d(&playerPos))) <= CLIMB_ANGLE)
        return;

    LOG_WARN("module.anticheat", "Anticheat: Climb-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::ClimbHack);
}

void AnticheatMgr::SpeedHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectSpeed)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    // We also must check the map because the movementFlag can be modified by the client.
    // If we just check the flag, they could always add that flag and always skip the speed hacking detection.
    if (lastMovement->HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && player->GetMapId())
    {
        switch (player->GetMapId())
        {
            case 369: // Transport: DEEPRUN TRAM
            case 607: // Transport: Strands of the Ancients
            case 582: // Transport: Rut'theran to Auberdine
            case 584: // Transport: Menethil to Theramore
            case 586: // Transport: Exodar to Auberdine
            case 587: // Transport: Feathermoon Ferry
            case 588: // Transport: Menethil to Auberdine
            case 589: // Transport: Orgrimmar to Grom'Gol
            case 590: // Transport: Grom'Gol to Undercity
            case 591: // Transport: Undercity to Orgrimmar
            case 592: // Transport: Borean Tundra Test
            case 593: // Transport: Booty Bay to Ratchet
            case 594: // Transport: Howling Fjord Sister Mercy (Quest)
            case 596: // Transport: Naglfar
            case 610: // Transport: Tirisfal to Vengeance Landing
            case 612: // Transport: Menethil to Valgarde
            case 613: // Transport: Orgrimmar to Warsong Hold
            case 614: // Transport: Stormwind to Valiance Keep
            case 620: // Transport: Moa'ki to Unu'pe
            case 621: // Transport: Moa'ki to Kamagua
            case 622: // Transport: Orgrim's Hammer
            case 623: // Transport: The Skybreaker
            case 641: // Transport: Alliance Airship BG
            case 642: // Transport: Horde Airship BG
            case 647: // Transport: Orgrimmar to Thunder Bluff
            case 672: // Transport: The Skybreaker (Icecrown Citadel Raid)
            case 673: // Transport: Orgrim's Hammer (Icecrown Citadel Raid)
            case 712: // Transport: The Skybreaker (IC Dungeon)
            case 713: // Transport: Orgrim's Hammer (IC Dungeon)
            case 718: // Transport: The Mighty Wind (Icecrown Citadel Raid)
                return;
                break;
        default:
            break; // Should never happen
        }
    }

    uint32 distance2D = (uint32)movementInfo->pos.GetExactDist2d(&lastMovement->pos);

    // We don't need to check for a speedhack if the player hasn't moved
    // This is necessary since MovementHandler fires if you rotate the camera in place
    if (!distance2D)
        return;

    UnitMoveType moveType = MOVE_RUN;

    // we need to know HOW is the player moving
    // TO-DO: Should we check the incoming movement flags?
    if (player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        moveType = MOVE_SWIM;
    else if (player->IsFlying())
        moveType = MOVE_FLIGHT;
    else if (player->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
        moveType = MOVE_WALK;

    // how many yards the player can do in one sec.
    // We remove the added speed for jumping because otherwise permanently jumping doubles your allowed speed
    uint32 speedRate = static_cast<uint32>(player->GetSpeed(moveType));

    if (lastMovement->time > movementInfo->time)
        BuildReport(player, AnticheatDetectionType::SpeedHack);

    // how long the player took to move to here.
    uint32 timeDiff = getMSTimeDiff(lastMovement->time, movementInfo->time);
    if (!timeDiff)
        timeDiff = 1;

    // We check the last MovementInfo for the falling flag since falling down a hill and sliding a bit triggered a false positive
    if (lastMovement->HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    if (player->CanTeleport())
        return;

    // this is the distance doable by the player in 1 sec, using the time done to move to this point.
    uint32 clientSpeedRate = distance2D * 1000 / timeDiff;

    // We did the (uint32) cast to accept a margin of tolerance
    if (clientSpeedRate <= speedRate * 1.25f)
        return;

    LOG_WARN("module.anticheat", "Anticheat: Speed-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::SpeedHack);
}

void AnticheatMgr::HandlePlayerLogin(Player* player)
{
    if (!_enable)
        return;

    auto playerGuid = player->GetGUID();
    auto anticheatData = GetAnticheatData(player->GetGUID());
    ASSERT(!anticheatData);

    _players.emplace(player->GetGUID(), AnticheatData());

    anticheatData = GetAnticheatData(player->GetGUID());
    ASSERT(anticheatData);

    // we initialize the pos of lastMovementPosition var.
    anticheatData->SetPosition(player->GetPosition());
}

void AnticheatMgr::HandlePlayerLogout(Player* player)
{
    if (!_enable)
        return;

    // Delete not needed data from the memory.
    _players.erase(player->GetGUID());
}

uint32 AnticheatMgr::GetTotalReports(ObjectGuid guid)
{
    auto anticheatData = GetAnticheatData(guid);
    if (!anticheatData)
        return 0;

    return anticheatData->GetTotalReports();
}

float AnticheatMgr::GetAverage(ObjectGuid guid)
{
    auto anticheatData = GetAnticheatData(guid);
    if (!anticheatData)
        return 0.0f;

    return anticheatData->GetAverage();
}

uint32 AnticheatMgr::GetTypeReports(ObjectGuid guid, AnticheatDetectionType type)
{
    auto anticheatData = GetAnticheatData(guid);
    if (!anticheatData)
        return 0;

    return anticheatData->GetTypeReports(type);
}

bool AnticheatMgr::MustCheckTempReports(AnticheatDetectionType type)
{
    switch (type)
    {
        case AnticheatDetectionType::JumpHack:
        case AnticheatDetectionType::TeleportHack:
        case AnticheatDetectionType::IngnoreControl:
        case AnticheatDetectionType::ZaxisHack:
            return false;
        default: return true;
    }
}

//
// Dear maintainer:
//
// Once you are done trying to 'optimize' this script,
// and have identify potentionally if there was a terrible
// mistake that was here or not, please increment the
// following counter as a warning to the next guy:
//
// total_hours_wasted_here = 42 + 2 (Winfidonarleyan)
//

void AnticheatMgr::BuildReport(Player* player, AnticheatDetectionType type)
{
    if (!player)
        return;

    auto currentTime = GameTime::GetGameTimeMS();
    auto anticheatData = GetAnticheatData(player->GetGUID());

    if (MustCheckTempReports(type))
    {
        auto tempReportsTime = anticheatData->GetTempReportsTimer(type);
        auto tempReports = anticheatData->GetTempReports(type);

        if (tempReportsTime == 0ms)
        {
            anticheatData->SetTempReportsTimer(currentTime, type);
            tempReportsTime = currentTime;
        }

        if (GetMSTimeDiff(tempReportsTime, currentTime) < 3s)
        {
            anticheatData->SetTempReports(++tempReports, type);
            if (tempReports < 3)
                return;
        }
        else
        {
            anticheatData->SetTempReportsTimer(currentTime, type);
            anticheatData->SetTempReports(1, type);
            return;
        }
    }

    auto totalReports = anticheatData->GetTotalReports();
    auto typeReports = anticheatData->GetTypeReports(type);

    // generating creationTime for average calculation
    if (!totalReports)
        anticheatData->SetCreationTime(currentTime);

    auto creationTime = anticheatData->GetCreationTime();

    // increasing total_reports
    anticheatData->SetTotalReports(++totalReports);

    // increasing specific cheat report
    anticheatData->SetTypeReports(type, ++typeReports);

    // diff time for average calculation
    auto diffTime = GetMSTimeDiff(creationTime, currentTime);

    if (diffTime > 0s)
    {
        // Average == Reports per second
        anticheatData->SetAverage(float(totalReports) / float(diffTime.count()));
    }

    if (totalReports > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_ALERT", player->GetName());

    if (MOD_CONF_GET_BOOL("Anticheat.KickPlayer") && totalReports > MOD_CONF_GET_UINT("Anticheat.ReportsForKick"))
    {
        LOG_WARN("module.anticheat", "Anticheat: Reports reached assigned threshhold and counteracted by kicking player {} ({})", player->GetName(), player->GetGUID().ToString());

        player->GetSession()->KickPlayer(true);

        if (MOD_CONF_GET_BOOL("Anticheat.AnnounceKick"))
        {
            std::string colorMsg = "|cff7bbef7";
            std::string colorTag = "|cffff0000";
            auto announceMessage = Warhead::StringFormat("{}[Anticheat]:|r{} player|r {}{}|r {} has been kicked by the anticheat|r",
                colorTag, colorMsg, colorTag, player->GetName(), colorMsg);

            sWorld->SendServerMessage(SERVER_MSG_STRING, announceMessage);
        }
    }

    if (MOD_CONF_GET_BOOL("Anticheat.BanPlayer") && totalReports > MOD_CONF_GET_UINT("Anticheat.ReportsForBan"))
    {
        LOG_WARN("module.anticheat", "Anticheat: Reports reached assigned threshhold and counteracted by banning player {} ({})", player->GetName(), player->GetGUID().ToString());

        std::string accountName;
        AccountMgr::GetName(player->GetSession()->GetAccountId(), accountName);
        sBan->BanAccount(accountName, "0s", "Anticheat Auto Banned Account for Reach Cheat Threshhold", "Server");

        if (MOD_CONF_GET_BOOL("Anticheat.AnnounceBan"))
        {
            std::string colorMsg = "|cff7bbef7";
            std::string colorTag = "|cffff0000";
            auto announceMessage = Warhead::StringFormat("{}[Anticheat]:|r{} player|r {}{}|r {} has been banned by the anticheat|r",
                colorTag, colorMsg, colorTag, player->GetName(), colorMsg);

            sWorld->SendServerMessage(SERVER_MSG_STRING, announceMessage);
        }
    }

    if (MOD_CONF_GET_BOOL("Anticheat.JailPlayer") && totalReports > MOD_CONF_GET_UINT("Anticheat.ReportsForJail"))
    {
        LOG_WARN("module.anticheat", "Anticheat: Reports reached assigned threshhold and counteracted by jailing player {} ({})", player->GetName(), player->GetGUID().ToString());

        WorldLocation loc;
        loc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f); // GM Jail Location
        player->TeleportTo(loc);
        player->SetHomebind(loc, 876); // GM Jail Homebind location
        player->CastSpell(player, static_cast<uint32>(SpellMisc::Shackles)); // Shackle him in place to ensure no exploit happens for jail break attempt
        player->AddAura(static_cast<uint32>(SpellMisc::LfgDeserter), player);
        player->AddAura(static_cast<uint32>(SpellMisc::BgDeserter), player);

        if (MOD_CONF_GET_BOOL("Anticheat.AnnounceJail"))
        {
            std::string colorMsg = "|cff7bbef7";
            std::string colorTag = "|cffff0000";
            auto announceMessage = Warhead::StringFormat("{}[Anticheat]:|r{} player|r {}{}|r {} has been jailed by the anticheat|r",
                colorTag, colorMsg, colorTag, player->GetName(), colorMsg);

            sWorld->SendServerMessage(SERVER_MSG_STRING, announceMessage);
        }
    }
}

void AnticheatMgr::AnticheatDeleteCommand(ObjectGuid guid)
{
    if (!guid)
    {
        for (auto& [guid, data] : _players)
            data.Clear();

        return;
    }

    auto anticheatData = GetAnticheatData(guid);
    if (!anticheatData)
        return;

    anticheatData->Clear();
}

uint16 AnticheatMgr::GetLastOpcode(ObjectGuid guid) const
{
    auto const& itr = _players.find(guid);
    if (itr != _players.end())
        return itr->second.GetLastOpcode();

    return 0;
}

void AnticheatMgr::SetLastOpcode(ObjectGuid guid, uint16 opcode)
{
    auto anticheatData = GetAnticheatData(guid);
    if (!anticheatData)
        return;

    anticheatData->SetLastOpcode(opcode);
}

MovementInfo const* AnticheatMgr::GetLastMovementInfo(ObjectGuid guid) const
{
    auto data = Warhead::Containers::MapGetValuePtr(_players, guid);
    if (!data)
        return nullptr;

    return data->GetLastMovementInfo();
}

void AnticheatMgr::SetLastMovementInfo(ObjectGuid guid, MovementInfo* movementInfo)
{
    auto data = Warhead::Containers::MapGetValuePtr(_players, guid);
    if (!data)
        return;

    data->SetLastMovementInfo(movementInfo);
}

uint32 AnticheatMgr::GetTotalReports(ObjectGuid guid) const
{
    auto const& itr = _players.find(guid);
    if (itr != _players.end())
        return itr->second.GetTotalReports();

    return 0;
}

AnticheatData* AnticheatMgr::GetAnticheatData(ObjectGuid guid)
{
    return Warhead::Containers::MapGetValuePtr(_players, guid);
}
