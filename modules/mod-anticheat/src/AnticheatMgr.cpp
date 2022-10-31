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

#include "AnticheatMgr.h"
#include "AccountMgr.h"
#include "BanMgr.h"
#include "Chat.h"
#include "GameTime.h"
#include "GameConfig.h"
#include "Log.h"
#include "MapMgr.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "SpellAuras.h"

namespace
{
    constexpr float CLIMB_ANGLE = 1.87f;

//    std::vector<ServerOrderData> _opackorders
//    {
//        { SMSG_FORCE_WALK_SPEED_CHANGE, CMSG_FORCE_WALK_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_RUN_SPEED_CHANGE, CMSG_FORCE_RUN_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_RUN_BACK_SPEED_CHANGE, CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_SWIM_SPEED_CHANGE, CMSG_FORCE_SWIM_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_SWIM_BACK_SPEED_CHANGE, CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_TURN_RATE_CHANGE, CMSG_FORCE_TURN_RATE_CHANGE_ACK },
//        { SMSG_FORCE_PITCH_RATE_CHANGE, CMSG_FORCE_PITCH_RATE_CHANGE_ACK },
//        { SMSG_FORCE_FLIGHT_SPEED_CHANGE, CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE, CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK },
//        { SMSG_FORCE_MOVE_ROOT, CMSG_FORCE_MOVE_ROOT_ACK },
//        { SMSG_FORCE_MOVE_UNROOT, CMSG_FORCE_MOVE_UNROOT_ACK },
//        { SMSG_MOVE_KNOCK_BACK, CMSG_MOVE_KNOCK_BACK_ACK },
//        { SMSG_MOVE_FEATHER_FALL, SMSG_MOVE_NORMAL_FALL, CMSG_MOVE_FEATHER_FALL_ACK },
//        { SMSG_MOVE_SET_HOVER, SMSG_MOVE_UNSET_HOVER, CMSG_MOVE_HOVER_ACK },
//        { SMSG_MOVE_SET_CAN_FLY, SMSG_MOVE_UNSET_CAN_FLY, CMSG_MOVE_SET_CAN_FLY_ACK },
//        { SMSG_MOVE_WATER_WALK, SMSG_MOVE_LAND_WALK, CMSG_MOVE_WATER_WALK_ACK },
//        { SMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY, SMSG_MOVE_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY, CMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY_ACK },
//        { SMSG_MOVE_GRAVITY_ENABLE, CMSG_MOVE_GRAVITY_ENABLE_ACK },
//        { SMSG_MOVE_GRAVITY_DISABLE, CMSG_MOVE_GRAVITY_DISABLE_ACK },
//        { SMSG_MOVE_SET_COLLISION_HGT, CMSG_MOVE_SET_COLLISION_HGT_ACK }
//    };
}

void AnticheatData::SetPosition(Position const& pos)
{
    _lastMovementInfo->pos = pos;
}

/*static*/ AnticheatMgr* AnticheatMgr::instance()
{
    static AnticheatMgr instance;
    return &instance;
}

void AnticheatMgr::LoadConfig(bool /*reload*/)
{
    _enable = MOD_CONF_GET_BOOL("Anticheat.Enable");
    if (!_enable)
        return;

    _enableGM = MOD_CONF_GET_BOOL("Anticheat.GM.Enable");

    _detectFly = MOD_CONF_GET_BOOL("Anticheat.Detect.FlyHack");
    _detectJump = MOD_CONF_GET_BOOL("Anticheat.Detect.JumpHack");
    _detectWaterWalk = MOD_CONF_GET_BOOL("Anticheat.Detect.WaterWalkHack");
    _detectTelePlain = MOD_CONF_GET_BOOL("Anticheat.Detect.TelePlaneHack");
    _detectSpeed = MOD_CONF_GET_BOOL("Anticheat.Detect.SpeedHack");
    _detectClimb = MOD_CONF_GET_BOOL("Anticheat.Detect.ClimbHack");
    _detectTeleport = MOD_CONF_GET_BOOL("Anticheat.Detect.TelePortHack");
    _detectZaxis = MOD_CONF_GET_BOOL("Anticheat.Detect.ZaxisHack");
    _ignoreControlHack = MOD_CONF_GET_BOOL("Anticheat.IgnoreControlHack");
    _antiSwimHack = MOD_CONF_GET_BOOL("Anticheat.AntiSwimHack");
    _detectGravityHack = MOD_CONF_GET_BOOL("Anticheat.DetectGravityHack");
    _antiKnockBackHack = MOD_CONF_GET_BOOL("Anticheat.AntiKnockBack");
    _noFallDamageHack = MOD_CONF_GET_BOOL("Anticheat.NoFallDamage");
    _detectBgStartHack = MOD_CONF_GET_BOOL("Anticheat.DetectBGStartHack");
    _opAckOrderHack = MOD_CONF_GET_BOOL("Anticheat.OpAckOrderHack");

    _stricterFlyHack = MOD_CONF_GET_BOOL("Anticheat.StricterFlyHackCheck");
    _stricterJumpHack = MOD_CONF_GET_BOOL("Anticheat.StricterDetectJumpHack");

    _configFallDamage = CONF_GET_FLOAT("Rate.Damage.Fall");
}

void AnticheatMgr::Update(uint32 diff)
{
    if (!_enable)
        return;

    _scheduler.Update(diff);
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
    JumpHackDetection(player, movementInfo, opcode);
    TeleportPlaneHackDetection(player, movementInfo, opcode);
    ClimbHackDetection(player, movementInfo, opcode);
    TeleportHackDetection(player, movementInfo);
    IgnoreControlHackDetection(player, movementInfo, opcode);
    GravityHackDetection(player, movementInfo);

    if (player->GetLiquidData().Status == LIQUID_MAP_WATER_WALK)
        WalkOnWaterHackDetection(player, movementInfo);
    else
        ZAxisHackDetection(player, movementInfo);

    AntiSwimHackDetection(player, movementInfo, opcode);
    AntiKnockBackHackDetection(player, movementInfo);
    NoFallDamageDetection(player, movementInfo);
    BGStartExploit(player, movementInfo);

    SetLastMovementInfo(playerGuid, movementInfo);
    SetLastOpcode(playerGuid, opcode);
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
        }
    }

    auto distance2D = (uint32)movementInfo->pos.GetExactDist2d(&lastMovement->pos);

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
    auto speedRate = static_cast<uint32>(player->GetSpeed(moveType));

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
    if (clientSpeedRate <= speedRate * 1.05f)
        return;

    LOG_WARN("module.anticheat", "Anticheat: Speed-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::SpeedHack);
}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectFly)
        return;

    if (player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)) // Overkill but wth
        return;

    /* Thanks to @LilleCarl for info to check extra flag */
    bool stricterChecks{ true };
    if (_stricterFlyHack)
        stricterChecks = !(movementInfo->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && !player->IsInWater());

    if (!movementInfo->HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && !movementInfo->HasMovementFlag(MOVEMENTFLAG_FLYING) && stricterChecks)
        return;

    BuildReport(player, AnticheatDetectionType::FlyHack);
    LOG_WARN("module.anticheat", "Anticheat: Fly-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_detectJump)
        return;

    ObjectGuid playerGuid = player->GetGUID();
    auto lastOpcode{ GetLastOpcode(playerGuid) };

    if (lastOpcode == MSG_MOVE_JUMP && opcode == MSG_MOVE_JUMP)
    {
        BuildReport(player, AnticheatDetectionType::JumpHack);
        LOG_WARN("module.anticheat", "Anticheat: Jump-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        return;
    }

    if (!_stricterJumpHack)
        return;

    if (lastOpcode == MSG_MOVE_JUMP && !player->IsFalling())
        return;

    const float groundZ = movementInfo->pos.GetPositionZ() - player->GetMapHeight(movementInfo->pos.GetPositionX(), movementInfo->pos.GetPositionY(), movementInfo->pos.GetPositionZ());
    const bool noFlyAuras = !(player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED)
                                || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
                                || player->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS));
    const bool noFlyFlags = ((movementInfo->flags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING)) == 0);
    const bool noSwimInWater = !player->IsInWater();
    const bool noSwimAboveWater = movementInfo->pos.GetPositionZ() - 7.0f >= player->GetMap()->GetWaterLevel(movementInfo->pos.GetPositionX(), movementInfo->pos.GetPositionY());
    const bool noSwimWater = noSwimInWater && noSwimAboveWater;

    if (!noFlyAuras || noFlyFlags || noSwimWater)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    auto distance2D = (uint32)movementInfo->pos.GetExactDist2d(lastMovement->pos);

    // This is necessary since MovementHandler fires if you rotate the camera in place
    if (!distance2D)
        return;

    if (!player->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY) && movementInfo->jump.zspeed < -10.0f)
        return;

    if (player->HasAuraType(SPELL_AURA_WATER_WALK) || player->HasAuraType(SPELL_AURA_FEATHER_FALL) || player->HasAuraType(SPELL_AURA_SAFE_FALL))
        return;

    if (groundZ <= 5.0f)
        return;

    if (movementInfo->pos.GetPositionZ() < player->GetPositionZ())
        return;

    BuildReport(player, AnticheatDetectionType::JumpHack);
    LOG_WARN("module.anticheat", "Anticheat: Stricter Jump-Hack detected player {} ({}). Latency {} ms", player->GetName(), player->GetGUID().ToString(), player->GetSession()->GetLatency());
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_detectTelePlain)
        return;

    // Celestial Planetarium Observer Battle has a narrow path that false flags
    if (player && GetWMOAreaTableEntryByTripple(5202, 0, 24083))
        return;

    if (player->HasAuraType(SPELL_AURA_WATER_WALK))
        return;

    if (player->HasAuraType(SPELL_AURA_WATER_BREATHING))
        return;

    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    ObjectGuid playerGuid = player->GetGUID();

    if (GetLastOpcode(playerGuid) == MSG_MOVE_JUMP)
        return;

    if (opcode == (MSG_MOVE_FALL_LAND))
        return;

    if (player->GetLiquidData().Status == LIQUID_MAP_ABOVE_WATER)
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_SWIMMING))
        return;

    // If he is flying we dont need to check
    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING))
        return;

    auto const& lastMovement = GetLastMovementInfo(playerGuid);
    if (!lastMovement)
        return;

    auto distance2D = (uint32)movementInfo->pos.GetExactDist2d(lastMovement->pos);

    // We don't need to check for a water walking hack if the player hasn't moved
    // This is necessary since MovementHandler fires if you rotate the camera in place
    if (!distance2D)
        return;

    float pos_z = player->GetPositionZ();
    float ground_Z = player->GetFloorZ();
    float groundZ = player->GetMapHeight(player->GetPositionX(), player->GetPositionY(), MAX_HEIGHT);
    float floorZ = player->GetMapHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());

    if (groundZ != floorZ)
        return;

    auto const& pos = player->GetPosition();

    // we are not really walking there
    if (std::fabs(ground_Z - pos_z) <= 2.0f)
        return;

    BuildReport(player, AnticheatDetectionType::TeleportPlaneHack);
    LOG_WARN("module.anticheat", "Anticheat: Teleport To Plane - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
}

void AnticheatMgr::ClimbHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_detectClimb)
        return;

    // If the player jumped, we dont want to check for climb hack
    // This can lead to false positives for climbing game objects legit
    if (opcode == MSG_MOVE_JUMP)
        return;

    // in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
    if (player->IsInWater() || player->IsFlying() || player->IsFalling())
        return;

    if (player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    if (player->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING))
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    Position playerPos = player->GetPosition();
    float diffZ = std::fabs(playerPos.GetPositionZ() - movementInfo->pos.GetPositionZ());
    float tanangle = movementInfo->pos.GetExactDist2d(&playerPos) / diffZ;

    if (movementInfo->pos.GetPositionZ() <= playerPos.GetPositionZ())
        return;

    if (diffZ <= 1.87f)
        return;

    if (tanangle >= 0.57735026919f) // 30 degrees
        return;

    LOG_WARN("module.anticheat", "Anticheat: Climb-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::ClimbHack);
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

    auto isPossibleTeleportHack = [this, lastMovement, movementInfo, player]
    {
        float lastX = lastMovement->pos.GetPositionX();
        float newX = movementInfo->pos.GetPositionX();

        float lastY = lastMovement->pos.GetPositionY();
        float newY = movementInfo->pos.GetPositionY();

        float lastZ = lastMovement->pos.GetPositionZ();
        float newZ = movementInfo->pos.GetPositionZ();

        float xDiff = std::fabs(lastX - newX);
        float yDiff = std::fabs(lastY - newY);
        float zDiff = std::fabs(lastZ - newZ);

        if (xDiff >= 50.0f || yDiff >= 50.0f || (zDiff >= 10.0f && !player->IsFlying() && !player->IsFalling()) && !player->CanTeleport())
            return true;

        return false;
    }();

    if (player->duel)
    {
        if (isPossibleTeleportHack)
        {
            Player* opponent = player->duel->Opponent;
            LOG_WARN("module.anticheat", "Anticheat: Teleport-Hack detected for player {} ({}) while dueling {} ({})", player->GetName(), player->GetGUID().ToString(), opponent->GetName(), opponent->GetGUID().ToString());
            BuildReport(player, AnticheatDetectionType::TeleportHack);
            sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_TELEPORT_DUELING", player->GetName(), opponent->GetName());
        }
        else if (player->CanTeleport())
            player->SetCanTeleport(false);
    }

    if (isPossibleTeleportHack)
    {
        if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
            sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_TELEPORT", player->GetName());

        LOG_WARN("module.anticheat", "Anticheat: Teleport-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        BuildReport(player, AnticheatDetectionType::TeleportHack);
    }
    else if (player->CanTeleport())
        player->SetCanTeleport(false);
}

void AnticheatMgr::IgnoreControlHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (!_ignoreControlHack)
        return;

    if (opcode == MSG_MOVE_FALL_LAND)
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_SWIMMING))
        return;

    if (!player->HasAuraType(SPELL_AURA_MOD_ROOT) || player->GetVehicle() || player->GetSession()->GetLatency() >= 400)
        return;

    ObjectGuid playerGuid = player->GetGUID();

    if (GetLastOpcode(playerGuid) == MSG_MOVE_JUMP)
        return;

    auto const& lastMovement = GetLastMovementInfo(playerGuid);
    if (!lastMovement)
        return;

    float lastX = lastMovement->pos.GetPositionX();
    float newX = movementInfo->pos.GetPositionX();

    float lastY = lastMovement->pos.GetPositionY();
    float newY = movementInfo->pos.GetPositionY();

    if (newX == lastX && newY == lastY)
        return;

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_IGNORE_CONTROL", player->GetName());

    LOG_WARN("module.anticheat", "Anticheat: Ignore Control - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::IngnoreControl);
}

void AnticheatMgr::GravityHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectGravityHack)
        return;

    if (player->HasAuraType(SPELL_AURA_FEATHER_FALL))
        return;

    if (GetLastOpcode(player->GetGUID()) != MSG_MOVE_JUMP)
        return;

    if (player->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY) || movementInfo->jump.zspeed >= -10.0f)
        return;

    LOG_WARN("module.anticheat", "Anticheat: Gravity-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::GravityHack);
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectWaterWalk)
        return;

    auto playerGuid{ player->GetGUID() };

    auto const& lastMovement = GetLastMovementInfo(playerGuid);
    if (!lastMovement)
        return;

    auto distance2D = (uint32)movementInfo->pos.GetExactDist2d(lastMovement->pos);

    // We don't need to check for a water walking hack if the player hasn't moved
    // This is necessary since MovementHandler fires if you rotate the camera in place
    if (!distance2D)
        return;

    auto MakeReport = [this, player, playerGuid]()
    {
        BuildReport(player, AnticheatDetectionType::WaterWalk);
        LOG_WARN("module.anticheat", "Anticheat: Walk on Water - Hack detected player {} ({})", player->GetName(), playerGuid.ToString());
    };

    if (player->GetLiquidData().Status == LIQUID_MAP_WATER_WALK && !player->IsFlying())
        MakeReport();

    // ghost can water walk
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    if (GetLastOpcode(playerGuid) == MSG_DELAY_GHOST_TELEPORT)
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

    MakeReport();
}

void AnticheatMgr::ZAxisHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_detectZaxis)
        return;

    // If he is flying we dont need to check
    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_CAN_FLY))
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_FLYING))
        return;

    // If the player is allowed to waterwalk (or he is dead because he automatically waterwalks then) we dont need to check any further
    // We also stop if the player is in water, because otherwise you get a false positive for swimming
    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) || player->IsInWater() || !player->IsAlive())
        return;

    //Celestial Planetarium Observer Battle has a narrow path that false flags
    if (GetWMOAreaTableEntryByTripple(5202, 0, 24083))
        return;

    // We want to exclude this LiquidStatus from detection because it leads to false positives on boats, docks etc.
    // Basically everytime you stand on a game object in water
    if (player->GetLiquidData().Status == LIQUID_MAP_ABOVE_WATER)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    if (lastMovement->pos.GetPositionZ() != movementInfo->pos.GetPositionZ())
        return;

    auto distance2D = (uint32)movementInfo->pos.GetExactDist2d(lastMovement->pos);

    // We don't need to check for a ignore z if the player hasn't moved
    // This is necessary since MovementHandler fires if you rotate the camera in place
    if (!distance2D)
        return;

    if (player->GetPositionZ() < player->GetFloorZ() + 2.5f)
        return;

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_ALERT", player->GetName());

    LOG_WARN("module.anticheat", "Anticheat: Ignore Zaxis - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::ZaxisHack);
}

void AnticheatMgr::AntiSwimHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode)
{
    if (_antiSwimHack)
        return;

    if (player->GetLiquidData().Status != LIQUID_MAP_UNDER_WATER)
        return;

    auto areaID{ player->GetAreaId() };
    if (areaID == 2100) // Maraudon https://github.com/azerothcore/azerothcore-wotlk/issues/2437
        return;

    if (player->GetLiquidData().Status == (LIQUID_MAP_ABOVE_WATER | LIQUID_MAP_WATER_WALK | LIQUID_MAP_IN_WATER))
        return;

    if (opcode == MSG_MOVE_JUMP)
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_SWIMMING))
        return;

    if (movementInfo->HasMovementFlag(MOVEMENTFLAG_SWIMMING))
        return;

    LOG_WARN("module.anticheat", "Anticheat: Anti-Swim-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::AntiswimHack);
}

void AnticheatMgr::AntiKnockBackHackDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_antiKnockBackHack)
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    // if a knockback helper is not passed then we ignore
    // if player has root state we ignore, knock back does not break root
    if (!player->CanKnockback() || player->HasUnitState(UNIT_STATE_ROOT))
        return;

    if (movementInfo->pos == lastMovement->pos)
    {
        LOG_WARN("module.anticheat", "Anticheat: Anti-Knock Back - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        BuildReport(player, AnticheatDetectionType::AntiKnockBackHack);
    }
    else if (player->CanKnockback())
        player->SetCanKnockback(false);
}

void AnticheatMgr::NoFallDamageDetection(Player* player, MovementInfo* movementInfo)
{
    if (!_noFallDamageHack)
        return;

    // ghost can not get damaged
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    // players with water walk aura jumping on to the water from ledge would not get damage and neither will safe fall and feather fall
    if (((player->HasAuraType(SPELL_AURA_WATER_WALK) && player->GetLiquidData().Status == LIQUID_MAP_WATER_WALK && !player->IsFlying())) ||
        player->HasAuraType(SPELL_AURA_FEATHER_FALL) || player->HasAuraType(SPELL_AURA_SAFE_FALL))
        return;

    auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
    if (!lastMovement)
        return;

    float lastZ = lastMovement->pos.GetPositionZ();
    float newZ = movementInfo->pos.GetPositionZ();
    float zDiff = std::fabs(lastZ - newZ);
    int32 safeFall = player->GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);
    float damagePerc = 0.018f * (zDiff - float(safeFall)) - 0.2426f;
    auto damage = uint32((damagePerc * float(player->GetMaxHealth()) * _configFallDamage));

    // in the Player::Handlefall 14.57f is used to calculated the damageperc formula below to 0 for fall damamge
    if (zDiff <= 14.57f || movementInfo->pos.GetPositionZ() >= lastMovement->pos.GetPositionZ())
        return;

    if (!movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING) && !lastMovement->HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    if (!damage && player->IsImmunedToDamageOrSchool(SPELL_SCHOOL_MASK_NORMAL))
        return;

    LOG_WARN("module.anticheat", "Anticheat: No Fall Damage - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::NoFallDamageHack);
}

void AnticheatMgr::BGreport(Player* player)
{
    LOG_WARN("module.anticheat", "Anticheat: BG Start Spot Exploit-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    BuildReport(player, AnticheatDetectionType::TeleportHack);

    if (GetTotalReports(player->GetGUID()) > MOD_CONF_GET_UINT("Anticheat.ReportsForIngameWarnings"))
        sModuleLocale->SendGlobalMessage(true, "ANTICHEAT_LOCALE_TELEPORT", player->GetName());
}

void AnticheatMgr::BGStartExploit(Player* player, MovementInfo* movementInfo)
{
    if (!_detectBgStartHack)
        return;

    auto IsBgStatusWaitJoin = [player]()
    {
        Battleground* bg = player->GetBattleground();
        if (!bg)
            return false;

        return bg->GetStatus() == STATUS_WAIT_JOIN;
    };

    auto teamID{ player->GetTeamId() };

    switch (player->GetMapId())
    {
        case 30: // Alterac Valley
        {
            if (IsBgStatusWaitJoin())
                return;

            // Outside of starting area before BG has started.
            if ((teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() < 770.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() > 940.31f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() < -525.0f))
                BGreport(player);

            if ((teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() > -536.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionX() > -1283.33f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() < -716.0f))
                BGreport(player);

            break;
        }
        case 489: // Warsong Gulch
        {
            // Only way to get this high is with engineering items malfunction.
            auto const& lastMovement = GetLastMovementInfo(player->GetGUID());
            if (!lastMovement)
                return;

            if (!(movementInfo->HasMovementFlag(MOVEMENTFLAG_FALLING_FAR) || GetLastOpcode(player->GetGUID()) == MSG_MOVE_JUMP) && movementInfo->pos.GetPositionZ() > 380.0f)
                BGreport(player);

            if (IsBgStatusWaitJoin())
                return;

            // Outside of starting area before BG has started.
            if ((teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() < 1490.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() > 1500.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() < 1450.0f))
                BGreport(player);

            if ((teamID == TEAM_HORDE && movementInfo->pos.GetPositionX() > 957.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() < 1416.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() > 1466.0f))
                BGreport(player);

            break;
        }
        case 529: // Arathi Basin
        {
            if (IsBgStatusWaitJoin())
                return;

            // Outside of starting area before BG has started.
            if ((teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() < 1270.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() < 1258.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() > 1361.0f))
                BGreport(player);

            if ((teamID == TEAM_HORDE && movementInfo->pos.GetPositionX() > 730.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() > 724.8f))
                BGreport(player);

            break;
        }
        case 566: // Eye of the Storm
        {
            if (IsBgStatusWaitJoin())
                return;

            // Outside of starting area before BG has started.
            if ((teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() < 2512.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() > 1610.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() < 1584.0f))
                BGreport(player);

            if ((teamID == TEAM_HORDE && movementInfo->pos.GetPositionX() > 1816.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() > 1554.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() < 1526.0f))
                BGreport(player);

            break;
        }
        case 628: // Island Of Conquest
        {
            if (IsBgStatusWaitJoin())
                return;

            // Outside of starting area before BG has started.
            if ((teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionX() > 412.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() < -911.0f) ||
                (teamID == TEAM_ALLIANCE && movementInfo->pos.GetPositionY() > -760.0f))
                BGreport(player);

            if ((teamID == TEAM_HORDE && movementInfo->pos.GetPositionX() < 1147.8f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() < -855.0f) ||
                (teamID == TEAM_HORDE && movementInfo->pos.GetPositionY() > -676.0f))
                BGreport(player);

            break;
        }
        default:
            return;
    }
}

void AnticheatMgr::HandlePlayerLogin(Player* player)
{
    if (!_enable)
        return;

    auto playerGuid = player->GetGUID();
    auto anticheatData = GetAnticheatData(playerGuid);
    ASSERT(!anticheatData);

    _players.emplace(playerGuid, AnticheatData());

    anticheatData = GetAnticheatData(playerGuid);
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
        case AnticheatDetectionType::GravityHack:
        case AnticheatDetectionType::AntiKnockBackHack:
        case AnticheatDetectionType::NoFallDamageHack:
        case AnticheatDetectionType::OpAckHack:
            return false;
        default:
            return true;
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

        WorldLocation loc{ 1, 16226.5f, 16403.6f, -64.5f, 3.2f }; // GM Jail Location
        player->TeleportTo(loc);
        player->SetHomebind(loc, 876); // GM Jail Homebind location
        player->CastSpell(player, static_cast<uint32>(SpellMisc::Shackles)); // Shackle him in place to ensure no exploit happens for jail break attempt

        if (Aura* auraLfgDeserter = player->AddAura(static_cast<uint32>(SpellMisc::LfgDeserter), player))
            auraLfgDeserter->SetDuration(-1);

        if (Aura* auraBgDeserter = player->AddAura(static_cast<uint32>(SpellMisc::BgDeserter), player))
            auraBgDeserter->SetDuration(-1);

        if (Aura* auraSilinced = player->AddAura(static_cast<uint32>(SpellMisc::Silenced), player))
            auraSilinced->SetDuration(-1);

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
