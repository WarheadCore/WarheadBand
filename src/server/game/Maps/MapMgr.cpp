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

#include "MapMgr.h"
#include "Chat.h"
#include "ChatTextBuilder.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GridDefines.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "LFGMgr.h"
#include "Language.h"
#include "Log.h"
#include "Map.h"
#include "MapInstanced.h"
#include "MapUpdater.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "StopWatch.h"
#include "Transport.h"
#include "World.h"
#include "WorldPacket.h"

MapMgr::MapMgr()
{
    _timer[3].SetInterval(CONF_GET_INT("MapUpdateInterval"));
}

MapMgr* MapMgr::instance()
{
    static MapMgr instance;
    return &instance;
}

void MapMgr::Initialize()
{
    StopWatch sw;

    auto threadsCount{ CONF_GET_INT("MapUpdate.Threads") };

    // Init updater
    _updater = std::make_unique<MapUpdater>();

    // Start mtmaps if needed
    if (threadsCount)
        _updater->InitThreads(threadsCount);

    LOG_INFO("server.loading", ">> Added {} threads for map update in {}", threadsCount, sw);
    LOG_INFO("server.loading", "");
}

void MapMgr::InitializeVisibilityDistanceInfo()
{
    for (auto const& [mapID, map] : _maps)
        map->InitVisibilityDistance();
}

Map* MapMgr::CreateBaseMap(uint32 id)
{
    Map* map = FindBaseMap(id);
    if (map)
        return map;

    std::lock_guard<std::mutex> guard(_lock);

    // pussywizard: check again after acquiring mutex
    map = FindBaseMap(id);
    if (map)
        return map;

    MapEntry const* entry = sMapStore.LookupEntry(id);
    ASSERT(entry);

    if (entry->Instanceable())
    {
        _maps[id] = std::make_unique<MapInstanced>(id);
        return _maps[id].get();
    }

    auto newMap{ std::make_unique<Map>(id, 0, REGULAR_DIFFICULTY) };
    newMap->LoadRespawnTimes();
    newMap->LoadCorpseData();
    _maps[id] = std::move(newMap);

    return _maps[id].get();
}

Map* MapMgr::FindBaseNonInstanceMap(uint32 mapId) const
{
    Map* map = FindBaseMap(mapId);
    if (map && map->Instanceable())
        return nullptr;

    return map;
}

Map* MapMgr::CreateMap(uint32 id, Player* player)
{
    Map* map = CreateBaseMap(id);
    if (map && map->Instanceable())
        map = ((MapInstanced*)map)->CreateInstanceForPlayer(id, player);

    return map;
}

Map* MapMgr::FindMap(uint32 mapid, uint32 instanceId) const
{
    Map* map = FindBaseMap(mapid);
    if (!map)
        return nullptr;

    if (!map->Instanceable())
        return instanceId == 0 ? map : nullptr;

    return ((MapInstanced*)map)->FindInstanceMap(instanceId);
}

Map* MapMgr::FindBaseMap(uint32 mapId) const
{
    auto map{ Warhead::Containers::MapGetValuePtr(_maps, mapId) };
    if (!map)
        return nullptr;

    return map->get();
}

uint32 MapMgr::GetAreaId(uint32 phaseMask, uint32 mapid, float x, float y, float z)
{
    Map const* m = CreateBaseMap(mapid);
    return m->GetAreaId(phaseMask, x, y, z);
}

uint32 MapMgr::GetZoneId(uint32 phaseMask, uint32 mapid, float x, float y, float z)
{
    Map const* m = CreateBaseMap(mapid);
    return m->GetZoneId(phaseMask, x, y, z);
}

void MapMgr::GetZoneAndAreaId(uint32 phaseMask, uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z)
{
    Map const* m = const_cast<MapMgr*>(this)->CreateBaseMap(mapid);
    m->GetZoneAndAreaId(phaseMask, zoneid, areaid, x, y, z);
}

MapEnterState MapMgr::PlayerCannotEnter(uint32 mapid, Player* player, bool loginCheck)
{
    MapEntry const* entry = sMapStore.LookupEntry(mapid);
    if (!entry)
        return CANNOT_ENTER_NO_ENTRY;

    if (!entry->IsDungeon())
        return CAN_ENTER;

    InstanceTemplate const* instance = sObjectMgr->GetInstanceTemplate(mapid);
    if (!instance)
        return CANNOT_ENTER_UNINSTANCED_DUNGEON;

    Difficulty targetDifficulty, requestedDifficulty;
    targetDifficulty = requestedDifficulty = player->GetDifficulty(entry->IsRaid());

    // Get the highest available difficulty if current setting is higher than the instance allows
    MapDifficulty const* mapDiff = GetDownscaledMapDifficultyData(entry->MapID, targetDifficulty);
    if (!mapDiff)
    {
        player->SendTransferAborted(mapid, TRANSFER_ABORT_DIFFICULTY, requestedDifficulty);
        return CANNOT_ENTER_DIFFICULTY_UNAVAILABLE;
    }

    // Bypass checks for GMs
    if (player->IsGameMaster())
        return CAN_ENTER;

    char const* mapName = entry->name[player->GetSession()->GetSessionDbcLocale()];

    if (!sScriptMgr->CanEnterMap(player, entry, instance, mapDiff, loginCheck))
        return CANNOT_ENTER_UNSPECIFIED_REASON;

    Group* group = player->GetGroup();
    if (entry->IsRaid())
    {
        // can only enter in a raid group
        if ((!group || !group->isRaidGroup()) && !CONF_GET_BOOL("Instance.IgnoreRaid"))
        {
            // probably there must be special opcode, because client has this string constant in GlobalStrings.lua
            // TODO: this is not a good place to send the message
            Warhead::Text::SendAreaTriggerMessage(player->GetSession(), LANG_INSTANCE_RAID_GROUP_ONLY);
            LOG_DEBUG("maps", "MAP: Player '{}' must be in a raid group to enter instance '{}'", player->GetName(), mapName);
            return CANNOT_ENTER_NOT_IN_RAID;
        }
    }

    // xinef: dont allow LFG Group to enter other instance that is selected
    if (group && group->isLFGGroup() && !sLFGMgr->inLfgDungeonMap(group->GetGUID(), mapid, targetDifficulty))
    {
        player->SendTransferAborted(mapid, TRANSFER_ABORT_MAP_NOT_ALLOWED);
        return CANNOT_ENTER_UNSPECIFIED_REASON;
    }

    if (!player->IsAlive())
    {
        if (player->HasCorpse())
        {
            // let enter in ghost mode in instance that connected to inner instance with corpse
            uint32 corpseMap = player->GetCorpseLocation().GetMapId();
            do
            {
                if (corpseMap == mapid)
                    break;

                InstanceTemplate const* corpseInstance = sObjectMgr->GetInstanceTemplate(corpseMap);
                corpseMap = corpseInstance ? corpseInstance->Parent : 0;
            } while (corpseMap);

            if (!corpseMap)
            {
                WorldPacket data(SMSG_CORPSE_NOT_IN_INSTANCE, 0);
                player->GetSession()->SendPacket(&data);
                LOG_DEBUG("maps", "MAP: Player '{}' does not have a corpse in instance '{}' and cannot enter.", player->GetName(), mapName);
                return CANNOT_ENTER_CORPSE_IN_DIFFERENT_INSTANCE;
            }
            LOG_DEBUG("maps", "MAP: Player '{}' has corpse in instance '{}' and can enter.", player->GetName(), mapName);
        }
        else
        {
            LOG_DEBUG("maps", "Map::CanPlayerEnter - player '{}' is dead but does not have a corpse!", player->GetName());
        }
    }

    // if map exists - check for being full, etc.
    if (!loginCheck) // for login this is done by the calling function
    {
        uint32 destInstId = sInstanceSaveMgr->PlayerGetDestinationInstanceId(player, mapid, targetDifficulty);
        if (destInstId)
            if (Map* boundMap = sMapMgr->FindMap(mapid, destInstId))
                if (MapEnterState denyReason = boundMap->CannotEnter(player, loginCheck))
                    return denyReason;
    }

    // players are only allowed to enter 5 instances per hour
    if (entry->IsDungeon() && (!group || !group->isLFGGroup() || !group->IsLfgRandomInstance()))
    {
        uint32 instaceIdToCheck = 0;
        if (InstanceSave* save = sInstanceSaveMgr->PlayerGetInstanceSave(player->GetGUID(), mapid, player->GetDifficulty(entry->IsRaid())))
            instaceIdToCheck = save->GetInstanceId();

        // instaceIdToCheck can be 0 if save not found - means no bind so the instance is new
        if (!player->CheckInstanceCount(instaceIdToCheck))
        {
            player->SendTransferAborted(mapid, TRANSFER_ABORT_TOO_MANY_INSTANCES);
            return CANNOT_ENTER_TOO_MANY_INSTANCES;
        }
    }

    // Other requirements
    return player->Satisfy(sObjectMgr->GetAccessRequirement(mapid, targetDifficulty), mapid, true) ? CAN_ENTER : CANNOT_ENTER_UNSPECIFIED_REASON;
}

void MapMgr::Update(uint32 diff)
{
    for (auto& timer : _timer)
        timer.Update(diff);

    // pussywizard: lfg compatibles update, schedule before maps so it is processed from the very beginning
    //if (mapUpdateStep == 0)
    {
        if (_updater->IsActive())
            _updater->ScheduleLfgUpdate(diff);
        else
            sLFGMgr->Update(diff, 1);
    }

    for (auto const& [mapID, map] : _maps)
    {
        bool full = mapUpdateStep < 3 &&
                ((mapUpdateStep == 0 && !map->IsBattlegroundOrArena() && !map->IsDungeon()) ||
                (mapUpdateStep == 1 && map->IsBattlegroundOrArena()) ||
                (mapUpdateStep == 2 && map->IsDungeon()));

        auto _diff{ uint32(full ? _timer[mapUpdateStep].GetCurrent() : 0) };

        if (_updater->IsActive())
            _updater->ScheduleUpdate(*map, _diff, diff);
        else
            map->Update(_diff, diff);
    }

    if (_updater->IsActive())
        _updater->WaitThreads();

    if (mapUpdateStep < 3)
    {
        for (auto const& [mapID, map] : _maps)
            if ((mapUpdateStep == 0 && !map->IsBattlegroundOrArena() && !map->IsDungeon()) || (mapUpdateStep == 1 && map->IsBattlegroundOrArena()) || (mapUpdateStep == 2 && map->IsDungeon()))
                map->DelayedUpdate(uint32(_timer[mapUpdateStep].GetCurrent()));

        _timer[mapUpdateStep].SetCurrent(0);
        ++mapUpdateStep;
    }

    if (mapUpdateStep == 3 && _timer[3].Passed())
    {
        mapUpdateStep = 0;
        _timer[3].SetCurrent(0);
    }
}

void MapMgr::SetMapUpdateInterval(uint32 t)
{
    if (t < MIN_MAP_UPDATE_DELAY)
        t = MIN_MAP_UPDATE_DELAY;

    _timer[3].SetInterval(t);
    _timer[3].Reset();
}

void MapMgr::DoDelayedMovesAndRemoves()
{
}

bool MapMgr::ExistMapAndVMap(uint32 mapid, float x, float y)
{
    GridCoord p = Warhead::ComputeGridCoord(x, y);

    int gx = 63 - p.x_coord;
    int gy = 63 - p.y_coord;

    return Map::ExistMap(mapid, gx, gy) && Map::ExistVMap(mapid, gx, gy);
}

bool MapMgr::IsValidMAP(uint32 mapid, bool startUp)
{
    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);

    if (startUp)
    {
        return mEntry != nullptr;
    }
    else
    {
        return mEntry && (!mEntry->IsDungeon() || sObjectMgr->GetInstanceTemplate(mapid));
    }

    // TODO: add check for battleground template
}

/*static*/ bool MapMgr::IsValidMapCoord(uint32 mapid, float x, float y)
{
    return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y);
}

/*static*/ bool MapMgr::IsValidMapCoord(uint32 mapid, float x, float y, float z)
{
    return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y, z);
}

/*static*/ bool MapMgr::IsValidMapCoord(uint32 mapid, float x, float y, float z, float o)
{
    return IsValidMAP(mapid, false) && Warhead::IsValidMapCoord(x, y, z, o);
}

/*static*/ float MapMgr::NormalizeOrientation(float o)
{
    // fmod only supports positive numbers. Thus, we have
    // to emulate negative numbers
    if (o < 0)
    {
        float mod = o * -1;
        mod = std::fmod(mod, 2.0f * static_cast<float>(M_PI));
        mod = -mod + 2.0f * static_cast<float>(M_PI);
        return mod;
    }

    return std::fmod(o, 2.0f * static_cast<float>(M_PI));
}

void MapMgr::UnloadAll()
{
    for (auto const& [mapID, map] : _maps)
        map->UnloadAll();

    _maps.clear();

    if (_updater->IsActive())
        _updater->Stop();
}

void MapMgr::GetNumInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas)
{
    for (auto const& [mapID, map] : _maps)
    {
        if (!map->Instanceable())
            continue;

        for (auto const& [instanceID, mapInstance] : map->ToMapInstanced()->GetInstancedMaps())
        {
            if (mapInstance->IsDungeon())
                dungeons++;
            else if (mapInstance->IsBattleground())
                battlegrounds++;
            else if (mapInstance->IsBattleArena())
                arenas++;
        }
    }
}

void MapMgr::GetNumPlayersInInstances(uint32& dungeons, uint32& battlegrounds, uint32& arenas, uint32& spectators)
{
    for (auto const& [mapID, map] : _maps)
    {
        if (!map->Instanceable())
            continue;

        for (auto const& [instanceID, mapInstanced] : map->ToMapInstanced()->GetInstancedMaps())
        {
            if (mapInstanced->IsDungeon())
                dungeons += mapInstanced->ToInstanceMap()->GetPlayers().getSize();
            else if (mapInstanced->IsBattleground())
                battlegrounds += mapInstanced->ToInstanceMap()->GetPlayers().getSize();
            else if (mapInstanced->IsBattleArena())
            {
                uint32 spect = 0;
                if (BattlegroundMap* bgmap = mapInstanced->ToBattlegroundMap())
                    if (Battleground* bg = bgmap->GetBG())
                        spect = bg->GetSpectators().size();

                arenas += mapInstanced->ToInstanceMap()->GetPlayers().getSize() - spect;
                spectators += spect;
            }
        }
    }
}

void MapMgr::InitInstanceIds()
{
    _nextInstanceId = 1;

    QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM instance");
    if (result)
    {
        uint32 maxId = (*result)[0].Get<uint32>();
        _instanceIds.resize(maxId + 1);
    }
}

void MapMgr::RegisterInstanceId(uint32 instanceId)
{
    // Allocation was done in InitInstanceIds()
    _instanceIds[instanceId] = true;

    // Instances are pulled in ascending order from db and _nextInstanceId is initialized with 1,
    // so if the instance id is used, increment
    if (_nextInstanceId == instanceId)
        ++_nextInstanceId;
}

uint32 MapMgr::GenerateInstanceId()
{
    uint32 newInstanceId = _nextInstanceId;

    // find the lowest available id starting from the current _nextInstanceId
    while (_nextInstanceId < 0xFFFFFFFF && ++_nextInstanceId < _instanceIds.size() && _instanceIds[_nextInstanceId]);

    if (_nextInstanceId == 0xFFFFFFFF)
    {
        LOG_ERROR("server.worldserver", "Instance ID overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return newInstanceId;
}

MapUpdater* MapMgr::GetMapUpdater()
{
    return _updater.get();
}

void MapMgr::DoForAllMaps(std::function<void(Map*)>&& worker)
{
    std::lock_guard<std::mutex> guard(_lock);

    for (auto const& [mapID, map] : _maps)
    {
        if (MapInstanced* mapInstanced = map->ToMapInstanced())
        {
            for (auto& [instanceID, _mapInstanced] : mapInstanced->GetInstancedMaps())
                worker(_mapInstanced.get());
        }
        else
            worker(map.get());
    }
}

void MapMgr::DoForAllMapsWithMapId(uint32 mapId, std::function<void(Map*)>&& worker)
{
    std::lock_guard<std::mutex> guard(_lock);

    auto const& itr = _maps.find(mapId);
    if (itr == _maps.end())
        return;

    auto map{ itr->second.get() };

    if (MapInstanced* mapInstanced = map->ToMapInstanced())
    {
        for (auto& [instanceID, _mapInstanced] : mapInstanced->GetInstancedMaps())
            worker(_mapInstanced.get());
    }
    else
        worker(map);
}
