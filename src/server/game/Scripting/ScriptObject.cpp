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

#include "ScriptObject.h"
#include "Log.h"

void WorldMapScript::checkValidity()
{
    checkMap();

    if (GetEntry() && !GetEntry()->IsWorldMap())
        LOG_ERROR("maps.script", "WorldMapScript for map {} is invalid.", GetEntry()->MapID);
}

void InstanceMapScript::checkValidity()
{
    checkMap();

    if (GetEntry() && !GetEntry()->IsDungeon())
        LOG_ERROR("maps.script", "InstanceMapScript for map {} is invalid.", GetEntry()->MapID);
}

void BattlegroundMapScript::checkValidity()
{
    checkMap();

    if (GetEntry() && !GetEntry()->IsBattleground())
        LOG_ERROR("maps.script", "BattlegroundMapScript for map {} is invalid.", GetEntry()->MapID);
}

template<class TMap>
void MapScript<TMap>::checkMap()
{
    _mapEntry = sMapStore.LookupEntry(_mapId);

    if (!_mapEntry)
        LOG_ERROR("maps.script", "Invalid MapScript for {}; no such map ID.", _mapId);
}

template class WH_GAME_API MapScript<Map>;
template class WH_GAME_API MapScript<InstanceMap>;
template class WH_GAME_API MapScript<BattlegroundMap>;
