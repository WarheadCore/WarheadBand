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

#ifndef SC_ACMGR_H
#define SC_ACMGR_H

#include "Common.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include "TaskScheduler.h"
#include <array>
#include <unordered_map>

class Player;
struct MovementInfo;
struct Position;

// Time between server sends acknowledgement, and client is actually acknowledged
constexpr Seconds ALLOWED_ACK_LAG = 2s;

enum class AnticheatDetectionType : uint8
{
    SpeedHack,
    FlyHack,
    WaterWalk,
    JumpHack,
    TeleportPlaneHack,
    ClimbHack,
    TeleportHack,
    IngnoreControl,
    ZaxisHack,
    AntiswimHack,
    GravityHack,
    AntiKnockBackHack,
    NoFallDamageHack,
    OpAckHack,

    Max
};

constexpr uint8 MAX_ANTICHEAT_DETECT_TYPE = static_cast<uint8>(AnticheatDetectionType::Max);

enum class SpellMisc : uint32
{
    Shackles                = 38505,
    LfgDeserter             = 71041,
    BgDeserter              = 26013,
    Silenced                = 23207,
    ResurrectionSickness    = 15007
};

class ServerOrderData
{
public:
    ServerOrderData(uint32 serv, uint32 resp) : serverOpcode1(serv), clientResp(resp) { }
    ServerOrderData(uint32 serv1, uint32 serv2, uint32 resp) : serverOpcode1(serv1), serverOpcode2(serv2), clientResp(resp) {}

    uint32 serverOpcode1{};
    uint32 serverOpcode2{};
    uint32 clientResp{};

    uint32 lastSent{};
    uint32 lastRcvd{};
    int32 counter{};
};

struct AnticheatData
{
public:
    AnticheatData() :
        _lastMovementInfo(std::make_unique<MovementInfo>()) { }

    inline void SetLastOpcode(uint32 opcode) { _lastOpcode = opcode; }
    inline uint32 GetLastOpcode() const { return _lastOpcode; }

    inline MovementInfo const* GetLastMovementInfo() const { return _lastMovementInfo.get(); }
    inline void SetLastMovementInfo(MovementInfo const* moveInfo) { _lastMovementInfo = std::make_unique<MovementInfo>(*moveInfo); }

    void SetPosition(Position const& pos);

    inline uint32 GetTotalReports() const { return _totalReports; }
    inline void SetTotalReports(uint32 totalReports) { _totalReports = totalReports; }

    inline uint32 GetTypeReports(AnticheatDetectionType type) const { return _reportsType.at(static_cast<uint8>(type)); }
    inline void SetTypeReports(AnticheatDetectionType type, uint32 amount) { _reportsType[static_cast<uint8>(type)] = amount; }

    inline float GetAverage() const { return _average; }
    inline void SetAverage(float average) { _average = average; }

    inline Milliseconds GetCreationTime() const { return _creationTime; }
    inline void SetCreationTime(Milliseconds creationTime) { _creationTime = creationTime; }

    inline uint32 GetTempReports(AnticheatDetectionType type) const { return _reportsTemp.at(static_cast<uint8>(type)); }
    inline void SetTempReports(uint32 amount, AnticheatDetectionType type) { _reportsTemp[static_cast<uint8>(type)] = amount; }

    inline Milliseconds GetTempReportsTimer(AnticheatDetectionType type) const { return _reportsTempTimer.at(static_cast<uint8>(type)); }
    inline void SetTempReportsTimer(Milliseconds time, AnticheatDetectionType type) { _reportsTempTimer[static_cast<uint8>(type)] = time; }

    inline void Clear()
    {
        SetTotalReports(0);
        SetAverage(0);
        SetCreationTime(0ms);

        _reportsType.fill(0);
        _reportsTemp.fill(0);
        _reportsTempTimer.fill(0ms);
    }

private:
    uint32 _lastOpcode{ 0 };
    std::unique_ptr<MovementInfo> _lastMovementInfo;
    uint32 _totalReports{ 0 };
    std::array<uint32, MAX_ANTICHEAT_DETECT_TYPE> _reportsType{};
    float _average{ 0.0f };
    Milliseconds _creationTime{ 0ms };
    std::array<uint32, MAX_ANTICHEAT_DETECT_TYPE> _reportsTemp{};
    std::array<Milliseconds, MAX_ANTICHEAT_DETECT_TYPE> _reportsTempTimer{};
};

class AnticheatMgr
{
    AnticheatMgr(AnticheatMgr const&) = delete;
    AnticheatMgr(AnticheatMgr&&) = delete;
    AnticheatMgr& operator=(AnticheatMgr const&) = delete;
    AnticheatMgr& operator=(AnticheatMgr&&) = delete;

    AnticheatMgr() = default;
    ~AnticheatMgr() = default;

public:
    static AnticheatMgr* instance();

    inline bool IsEnable() const { return _enable; }
    void LoadConfig(bool reload);
    void Update(uint32 diff);

    void StartHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);

    void HandlePlayerLogin(Player* player);
    void HandlePlayerLogout(Player* player);

    uint32 GetTotalReports(ObjectGuid guid);
    float GetAverage(ObjectGuid guid);
    uint32 GetTypeReports(ObjectGuid guid, AnticheatDetectionType type);

    void AnticheatDeleteCommand(ObjectGuid guid);

private:
    void SpeedHackDetection(Player* player, MovementInfo* movementInfo);
    void FlyHackDetection(Player* player, MovementInfo* movementInfo);
    void JumpHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void TeleportPlaneHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void ClimbHackDetection(Player* player,MovementInfo* movementInfo,uint32 opcode);
    void TeleportHackDetection(Player* player, MovementInfo* movementInfo);
    void IgnoreControlHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void GravityHackDetection(Player* player, MovementInfo* movementInfo);
    void WalkOnWaterHackDetection(Player* player, MovementInfo* movementInfo);
    void ZAxisHackDetection(Player* player, MovementInfo* movementInfo);
    void AntiSwimHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void AntiKnockBackHackDetection(Player* player, MovementInfo* movementInfo);
    void NoFallDamageDetection(Player* player, MovementInfo* movementInfo);
    void BGreport(Player* player);
    void BGStartExploit(Player* player, MovementInfo* movementInfo);
    void BuildReport(Player* player, AnticheatDetectionType type);

    static bool MustCheckTempReports(AnticheatDetectionType type);

    uint16 GetLastOpcode(ObjectGuid guid) const;
    void SetLastOpcode(ObjectGuid guid, uint16 opcode);
    MovementInfo const* GetLastMovementInfo(ObjectGuid guid) const;
    void SetLastMovementInfo(ObjectGuid guid, MovementInfo* movementInfo);
    uint32 GetTotalReports(ObjectGuid guid) const;
    AnticheatData* GetAnticheatData(ObjectGuid guid);

    // Config
    bool _enable{};
    bool _enableGM{};

    bool _detectFly{};
    bool _detectJump{};
    bool _detectWaterWalk{};
    bool _detectTelePlain{};
    bool _detectSpeed{};
    bool _detectClimb{};
    bool _detectTeleport{};
    bool _detectZaxis{};
    bool _ignoreControlHack{};
    bool _antiSwimHack{};
    bool _detectGravityHack{};
    bool _antiKnockBackHack{};
    bool _noFallDamageHack{};
    bool _detectBgStartHack{};
    bool _opAckOrderHack{};

    bool _stricterFlyHack{};
    bool _stricterJumpHack{};

    float _configFallDamage{};

    std::unordered_map<ObjectGuid, AnticheatData> _players;
    TaskScheduler _scheduler;
};

#define sAnticheatMgr AnticheatMgr::instance()

#endif
