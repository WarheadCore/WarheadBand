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

#include "MapInstanced.h"
#include "Battleground.h"
#include "DBCEnums.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "MMapFactory.h"
#include "MapMgr.h"
#include "MapUpdater.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

MapInstanced::MapInstanced(uint32 id) : Map(id, 0, DUNGEON_DIFFICULTY_NORMAL)
{
    // Initialize instanced maps list
    _instancedMaps.clear();
}

void MapInstanced::InitVisibilityDistance()
{
    if (_instancedMaps.empty())
        return;

    // Initialize visibility distances for all instance copies
    for (auto const& [instanceID, map] : _instancedMaps)
        map->InitVisibilityDistance();
}

void MapInstanced::Update(uint32 t, uint32 s_diff, bool /*thread*/)
{
    // take care of loaded _gridMaps (when unused, unload it!)
    Map::Update(t, s_diff, false);

    std::vector<uint32> toEraseMaps;

    // update the instanced maps
    for (auto const& [instanceID, map] : _instancedMaps)
    {
        if (map->CanUnload(t))
        {
            if (DestroyInstance(map.get()))
                toEraseMaps.emplace_back(instanceID);

            continue;
        }

        // update only here, because it may schedule some bad things before delete
        if (sMapMgr->GetMapUpdater()->IsActive())
        {
            sMapMgr->GetMapUpdater()->ScheduleUpdate(*map.get(), t, s_diff);
            continue;
        }

        map->Update(t, s_diff);
    }

    // Erase maps
    for (auto instanceID : toEraseMaps)
        _instancedMaps.erase(instanceID);
}

void MapInstanced::DelayedUpdate(uint32 diff)
{
    for (auto const& [instanceID, map] : _instancedMaps)
        map->DelayedUpdate(diff);

    Map::DelayedUpdate(diff); // this may be removed
}

void MapInstanced::UnloadAll()
{
    // Unload instanced maps
    for (auto const& [mapID, map] : _instancedMaps)
        map->UnloadAll();

    // Delete the maps only after everything is unloaded to prevent crashes
    _instancedMaps.clear();

    // Unload own grids (just dummy(placeholder) grids, neccesary to unload _gridMaps!)
    Map::UnloadAll();
}

/*
- return the right instance for the object, based on its InstanceId
- create the instance if it's not created already
- the player is not actually added to the instance (only in InstanceMap::Add)
*/
Map* MapInstanced::CreateInstanceForPlayer(const uint32 mapId, Player* player)
{
    if (GetId() != mapId || !player)
        return nullptr;

    Map* map{ nullptr };

    if (IsBattlegroundOrArena())
    {
        // instantiate or find existing bg map for player
        // the instance id is set in battlegroundid
        uint32 newInstanceId = player->GetBattlegroundId();
        if (!newInstanceId)
            return nullptr;

        map = sMapMgr->FindMap(mapId, newInstanceId);
        if (!map)
        {
            Battleground* bg = player->GetBattleground(true);
            if (bg && bg->GetStatus() < STATUS_WAIT_LEAVE)
                map = CreateBattleground(newInstanceId, bg);
            else
            {
                player->TeleportToEntryPoint();
                return nullptr;
            }
        }
    }
    else
    {
        Difficulty realdiff = player->GetDifficulty(IsRaid());
        uint32 destInstId = sInstanceSaveMgr->PlayerGetDestinationInstanceId(player, GetId(), realdiff);

        if (destInstId)
        {
            InstanceSave* pSave = sInstanceSaveMgr->GetInstanceSave(destInstId);
            ASSERT(pSave); // pussywizard: must exist

            map = FindInstanceMap(destInstId);
            if (!map)
                map = CreateInstance(destInstId, pSave, realdiff);
            else if (IsSharedDifficultyMap(mapId) && !map->HavePlayers() && map->GetDifficulty() != realdiff)
            {
                if (player->isBeingLoaded()) // pussywizard: crashfix (assert(passengers.empty) fail in ~transport), could be added to a transport during loading from db
                    return nullptr;

                if (!map->AllTransportsEmpty())
                    map->AllTransportsRemovePassengers(); // pussywizard: gameobjects / summons (assert(passengers.empty) fail in ~transport)

                for (auto const& [instanceID, _map] : _instancedMaps)
                {
                    if (instanceID == destInstId)
                    {
                        DestroyInstance(_map.get());
                        map = CreateInstance(destInstId, pSave, realdiff);
                        break;
                    }
                }
            }
        }
        else
        {
            uint32 newInstanceId = sMapMgr->GenerateInstanceId();
            ASSERT(!FindInstanceMap(newInstanceId)); // pussywizard: instance with new id can't exist
            Difficulty diff = player->GetGroup() ? player->GetGroup()->GetDifficulty(IsRaid()) : player->GetDifficulty(IsRaid());
            map = CreateInstance(newInstanceId, nullptr, diff);
        }
    }

    return map;
}

Map* MapInstanced::FindInstanceMap(uint32 instanceId) const
{
    auto map{ Warhead::Containers::MapGetValuePtr(_instancedMaps, instanceId) };
    if (!map)
        return nullptr;

    return map->get();
}

InstanceMap* MapInstanced::CreateInstance(uint32 InstanceId, InstanceSave* save, Difficulty difficulty)
{
    // load/create a map
    std::lock_guard<std::mutex> guard(Lock);

    // make sure we have a valid map id
    MapEntry const* entry = sMapStore.LookupEntry(GetId());
    if (!entry)
    {
        LOG_ERROR("maps", "CreateInstance: no entry for map {}", GetId());
        ABORT();
    }

    InstanceTemplate const* iTemplate = sObjectMgr->GetInstanceTemplate(GetId());
    if (!iTemplate)
    {
        LOG_ERROR("maps", "CreateInstance: no instance template for map {}", GetId());
        ABORT();
    }

    // some instances only have one difficulty
    GetDownscaledMapDifficultyData(GetId(), difficulty);

    LOG_DEBUG("maps", "MapInstanced::CreateInstance: {} map instance {} for {} created with difficulty {}", save ? "" : "new ", InstanceId, GetId(), difficulty ? "heroic" : "normal");

    auto map = std::make_unique<InstanceMap>(GetId(), InstanceId, difficulty, this);
    ASSERT(map->IsDungeon());

    map->LoadRespawnTimes();
    map->LoadCorpseData();

    if (save)
        map->CreateInstanceScript(true, save->GetInstanceData(), save->GetCompletedEncounterMask());
    else
        map->CreateInstanceScript(false, "", 0);

    if (!save) // this is for sure a dungeon (assert above), no need to check here
        sInstanceSaveMgr->AddInstanceSave(GetId(), InstanceId, difficulty);

    _instancedMaps[InstanceId] = std::move(map);
    return _instancedMaps[InstanceId].get()->ToInstanceMap();
}

BattlegroundMap* MapInstanced::CreateBattleground(uint32 InstanceId, Battleground* bg)
{
    // load/create a map
    std::lock_guard<std::mutex> guard(Lock);

    LOG_DEBUG("maps", "MapInstanced::CreateBattleground: map bg {} for {} created.", InstanceId, GetId());

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), bg->GetMinLevel());
    uint8 spawnMode = bracketEntry ? bracketEntry->difficulty : REGULAR_DIFFICULTY;

    auto map = std::make_unique<BattlegroundMap>(GetId(), InstanceId, this, spawnMode);
    ASSERT(map->IsBattlegroundOrArena());
    map->SetBG(bg);
    bg->SetBgMap(map.get());

    _instancedMaps[InstanceId] = std::move(map);
    return _instancedMaps[InstanceId].get()->ToBattlegroundMap();
}

// increments the iterator after erase
bool MapInstanced::DestroyInstance(Map* map)
{
    map->RemoveAllPlayers();

    if (map->HavePlayers())
        return false;

    sScriptMgr->OnDestroyInstance(this, map);

    map->UnloadAll();
    LOG_DEBUG("maps", "DestroyInstance. {}", map->GetDebugInfo());

    // erase map
    return true;
}

MapEnterState MapInstanced::CannotEnter(Player* /*player*/, bool /*loginCheck*/)
{
    //ABORT();
    return CAN_ENTER;
}
