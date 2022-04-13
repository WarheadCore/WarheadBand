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
#include "Chat.h"
#include "ObjectGuid.h"
#include <array>
#include <unordered_map>

class Player;

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

    Max
};

constexpr uint8 MAX_ANTICHEAT_DETECT_TYPE = static_cast<uint8>(AnticheatDetectionType::Max);

enum class SpellMisc : uint32
{
    Shackles = 38505,
    LfgDeserter = 71041,
    BgDeserter = 26013
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

    //inline void SetPosition(float x, float y, float z, float o) {_lastMovementInfo->pos = { x, y, z, o }; }
    inline void SetPosition(Position const& pos) { _lastMovementInfo->pos = pos; }

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

    inline bool GetDailyReportState() const { return _hasDailyReport; }
    inline void SetDailyReportState(bool value) { _hasDailyReport = value; }

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
    bool _hasDailyReport{ false };
};

class AnticheatMgr
{
    AnticheatMgr(AnticheatMgr const&) = delete;
    AnticheatMgr(AnticheatMgr&&) = delete;
    AnticheatMgr& operator=(AnticheatMgr const&) = delete;
    AnticheatMgr& operator=(AnticheatMgr&&) = delete;

    AnticheatMgr() = default;
    ~AnticheatMgr();

public:
    static AnticheatMgr* instance();

    inline bool IsEnable() { return _enable; }

    void LoadConfig(bool reload);

    void StartHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void SavePlayerData(Player* player);

    void HandlePlayerLogin(Player* player);
    void HandlePlayerLogout(Player* player);

    uint32 GetTotalReports(ObjectGuid guid);
    float GetAverage(ObjectGuid guid);
    uint32 GetTypeReports(ObjectGuid guid, AnticheatDetectionType type);

    void AnticheatGlobalCommand(ChatHandler* handler);
    void AnticheatDeleteCommand(ObjectGuid guid);
    void AnticheatPurgeCommand(ChatHandler* handler);
    void ResetDailyReportStates();
private:
    void SpeedHackDetection(Player* player, MovementInfo* movementInfo);
    void FlyHackDetection(Player* player, MovementInfo* movementInfo);
    void WalkOnWaterHackDetection(Player* player, MovementInfo* movementInfo);
    void JumpHackDetection(Player* player, MovementInfo* movementInfo, uint32 opcode);
    void TeleportPlaneHackDetection(Player* player, MovementInfo* movementInfo);
    void ClimbHackDetection(Player* player,MovementInfo* movementInfo,uint32 opcode);
    void TeleportHackDetection(Player* player, MovementInfo* movementInfo);
    void IgnoreControlHackDetection(Player* player, MovementInfo* movementInfo);
    void ZAxisHackDetection(Player* player, MovementInfo* movementInfo);
    void BuildReport(Player* player, AnticheatDetectionType type);

    bool MustCheckTempReports(AnticheatDetectionType type);

    uint16 GetLastOpcode(ObjectGuid guid) const;
    void SetLastOpcode(ObjectGuid guid, uint16 opcode);
    MovementInfo const* GetLastMovementInfo(ObjectGuid guid) const;
    void SetLastMovementInfo(ObjectGuid guid, MovementInfo* movementInfo);
    uint32 GetTotalReports(ObjectGuid guid) const;
    AnticheatData* GetAnticheatData(ObjectGuid guid);


    // Values
    bool _enable{ false };
    bool _enableGM{ false };

    bool _detectFly{ false };
    bool _detectJump{ false };
    bool _detectWaterWalk{ false };
    bool _detectTelePlain{ false };
    bool _detectSpeed{ false };
    bool _detectClimb{ false };
    bool _detectTeleport{ false };
    bool _detectZaxis{ false };

    std::unordered_map<ObjectGuid, AnticheatData> _players;
};

#define sAnticheatMgr AnticheatMgr::instance()

#endif
