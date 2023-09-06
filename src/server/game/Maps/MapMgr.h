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

#ifndef WARHEAD_MAPMANAGER_H
#define WARHEAD_MAPMANAGER_H

#include "Define.h"
#include "Position.h"
#include "Timer.h"
#include <atomic>
#include <functional>
#include <mutex>

class Transport;
class StaticTransport;
class MotionTransport;
class Map;
class MapUpdater;
class MapInstanced;
class Player;

enum MapEnterState : uint8;

class WH_GAME_API MapMgr
{
public:
    static MapMgr* instance();

    Map* CreateBaseMap(uint32 mapId);
    Map* FindBaseNonInstanceMap(uint32 mapId) const;
    Map* CreateMap(uint32 mapId, Player* player);
    Map* FindMap(uint32 mapId, uint32 instanceId) const;
    Map* FindBaseMap(uint32 mapId) const; // pussywizard: need this public for movemaps (mmaps)

    [[nodiscard]] uint32 GetAreaId(uint32 phaseMask, uint32 mapid, float x, float y, float z);
    [[nodiscard]] uint32 GetAreaId(uint32 phaseMask, uint32 mapid, Position const& pos) { return GetAreaId(phaseMask, mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
    [[nodiscard]] uint32 GetAreaId(uint32 phaseMask, WorldLocation const& loc) { return GetAreaId(phaseMask, loc.GetMapId(), loc); }

    [[nodiscard]] uint32 GetZoneId(uint32 phaseMask, uint32 mapid, float x, float y, float z);
    [[nodiscard]] uint32 GetZoneId(uint32 phaseMask, uint32 mapid, Position const& pos) { return GetZoneId(phaseMask, mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
    [[nodiscard]] uint32 GetZoneId(uint32 phaseMask, WorldLocation const& loc) { return GetZoneId(phaseMask, loc.GetMapId(), loc); }

    void GetZoneAndAreaId(uint32 phaseMask, uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z);

    void Initialize();
    void Update(uint32);

    void SetMapUpdateInterval(uint32 t);

    //void LoadGrid(int mapid, int instId, float x, float y, WorldObject const* obj, bool no_unload = false);
    void UnloadAll();

    static bool ExistMapAndVMap(uint32 mapid, float x, float y);
    static bool IsValidMAP(uint32 mapid, bool startUp);

    static inline bool IsValidMapCoord(uint32 mapid, Position const& pos)
    {
        return IsValidMapCoord(mapid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
    }

    static bool IsValidMapCoord(uint32 mapid, float x, float y);
    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z);
    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z, float o);

    static inline bool IsValidMapCoord(WorldLocation const& loc)
    {
        return IsValidMapCoord(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation());
    }

    // modulos a radian orientation to the range of 0..2PI
    static float NormalizeOrientation(float o);

    /**
    * @name GetInstanceIDs
    * @return vector of instance IDs
    */
    std::vector<bool> GetInstanceIDs()
    {
        return _instanceIds;
    }

    void DoDelayedMovesAndRemoves();

    MapEnterState PlayerCannotEnter(uint32 mapid, Player* player, bool loginCheck = false);
    void InitializeVisibilityDistanceInfo();

    /* statistics */
    void GetNumInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas);
    void GetNumPlayersInInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas, uint32& spectators);

    // Instance ID management
    void InitInstanceIds();
    void RegisterInstanceId(uint32 instanceId);
    uint32 GenerateInstanceId();

    MapUpdater* GetMapUpdater();

    void DoForAllMaps(std::function<void(Map*)>&& worker);
    void DoForAllMapsWithMapId(uint32 mapId, std::function<void(Map*)>&& worker);

    uint32 IncreaseScheduledScriptsCount() { return ++_scheduledScripts; }
    uint32 DecreaseScheduledScriptCount() { return --_scheduledScripts; }
    uint32 DecreaseScheduledScriptCount(size_t count) { return _scheduledScripts -= count; }
    bool IsScriptScheduled() const { return _scheduledScripts > 0; }

private:
    MapMgr();
    ~MapMgr() = default;

    MapMgr(const MapMgr&) = delete;
    MapMgr& operator=(const MapMgr&) = delete;

    std::mutex _lock;
    std::unordered_map<uint32, std::unique_ptr<Map>> _maps;
    IntervalTimer _timer[4]; // continents, bgs/arenas, instances, total from the beginning
    uint8 mapUpdateStep{};

    std::vector<bool> _instanceIds;
    uint32 _nextInstanceId{};
    std::unique_ptr<MapUpdater> _updater;

    // atomic op counter for active scripts amount
    std::atomic<uint32> _scheduledScripts;
};

#define sMapMgr MapMgr::instance()

#endif
