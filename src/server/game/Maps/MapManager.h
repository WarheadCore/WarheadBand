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

#ifndef ACORE_MAPMANAGER_H
#define ACORE_MAPMANAGER_H

#include "Common.h"
#include "Define.h"
#include "Map.h"
#include "MapUpdater.h"
#include "Object.h"
#include <ace/Thread_Mutex.h>

class Transport;
class StaticTransport;
class MotionTransport;
struct TransportCreatureProto;

class WH_GAME_API MapManager
{
public:
    static MapManager* instance();

    Map* CreateBaseMap(uint32 mapId);
    Map* FindBaseNonInstanceMap(uint32 mapId) const;
    Map* CreateMap(uint32 mapId, Player* player);
    Map* FindMap(uint32 mapId, uint32 instanceId) const;

    Map* FindBaseMap(uint32 mapId) const // pussywizard: need this public for movemaps (mmaps)
    {
        MapMapType::const_iterator iter = i_maps.find(mapId);
        return (iter == i_maps.end() ? nullptr : iter->second);
    }

    uint32 GetAreaId(uint32 mapid, float x, float y, float z) const
    {
        Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
        return m->GetAreaId(x, y, z);
    }
    uint32 GetZoneId(uint32 mapid, float x, float y, float z) const
    {
        Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
        return m->GetZoneId(x, y, z);
    }
    void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z)
    {
        Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
        m->GetZoneAndAreaId(zoneid, areaid, x, y, z);
    }

    void Initialize(void);
    void Update(uint32);

    void SetMapUpdateInterval(uint32 t)
    {
        if (t < MIN_MAP_UPDATE_DELAY)
            t = MIN_MAP_UPDATE_DELAY;

        i_timer[3].SetInterval(t);
        i_timer[3].Reset();
    }

    //void LoadGrid(int mapid, int instId, float x, float y, const WorldObject* obj, bool no_unload = false);
    void UnloadAll();

    static bool ExistMapAndVMap(uint32 mapid, float x, float y);
    static bool IsValidMAP(uint32 mapid, bool startUp);

    static bool IsValidMapCoord(uint32 mapid, float x, float y)
    {
        return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y);
    }

    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z)
    {
        return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y, z);
    }

    static bool IsValidMapCoord(uint32 mapid, float x, float y, float z, float o)
    {
        return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y, z, o);
    }

    static bool IsValidMapCoord(WorldLocation const& loc)
    {
        return IsValidMapCoord(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation());
    }

    // modulos a radian orientation to the range of 0..2PI
    static float NormalizeOrientation(float o)
    {
        // fmod only supports positive numbers. Thus we have
        // to emulate negative numbers
        if (o < 0)
        {
            float mod = o * -1;
            mod = fmod(mod, 2.0f * static_cast<float>(M_PI));
            mod = -mod + 2.0f * static_cast<float>(M_PI);
            return mod;
        }
        return fmod(o, 2.0f * static_cast<float>(M_PI));
    }

    void DoDelayedMovesAndRemoves();

    bool CanPlayerEnter(uint32 mapid, Player* player, bool loginCheck = false);
    void InitializeVisibilityDistanceInfo();

    /* statistics */
    void GetNumInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas);
    void GetNumPlayersInInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas, uint32& spectators);

    // Instance ID management
    void InitInstanceIds();
    void RegisterInstanceId(uint32 instanceId);
    uint32 GenerateInstanceId();

    MapUpdater* GetMapUpdater() { return &m_updater; }

private:
    typedef std::unordered_map<uint32, Map*> MapMapType;
    typedef std::vector<bool> InstanceIds;

    MapManager();
    ~MapManager();

    MapManager(const MapManager&);
    MapManager& operator=(const MapManager&);

    ACE_Thread_Mutex Lock;
    MapMapType i_maps;
    IntervalTimer i_timer[4]; // continents, bgs/arenas, instances, total from the beginning
    uint8 mapUpdateStep;

    InstanceIds _instanceIds;
    uint32 _nextInstanceId;
    MapUpdater m_updater;
};

#define sMapMgr MapManager::instance()

#endif
