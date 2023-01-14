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

#ifndef WARHEAD_MAP_INSTANCED_H
#define WARHEAD_MAP_INSTANCED_H

#include "Map.h"

class InstanceSave;

class WH_GAME_API MapInstanced : public Map
{
    friend class MapMgr;

public:
    using InstancedMaps = std::unordered_map<uint32, std::unique_ptr<Map>>;

    explicit MapInstanced(uint32 id);
    ~MapInstanced() override = default;

    // functions overwrite Map versions
    void Update(uint32, uint32, bool thread = true) override;
    void DelayedUpdate(uint32 diff) override;
    void UnloadAll() override;
    MapEnterState CannotEnter(Player* player, bool loginCheck = false) override;

    Map* CreateInstanceForPlayer(uint32 mapId, Player* player);
    Map* FindInstanceMap(uint32 instanceId) const;
    bool DestroyInstance(Map* map);

    InstancedMaps const& GetInstancedMaps() const { return _instancedMaps; }
    InstancedMaps& GetInstancedMaps() { return _instancedMaps; }
    void InitVisibilityDistance() override;

private:
    InstanceMap* CreateInstance(uint32 InstanceId, InstanceSave* save, Difficulty difficulty);
    BattlegroundMap* CreateBattleground(uint32 InstanceId, Battleground* bg);

    InstancedMaps _instancedMaps;
};

#endif
