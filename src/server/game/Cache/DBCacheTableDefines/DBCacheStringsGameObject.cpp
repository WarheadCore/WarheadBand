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

#include "DBCacheMgr.h"

void DBCacheMgr::InitGameObjectStrings()
{
    _queryStrings.emplace(DBCacheTable::GameobjectTemplate, "SELECT entry, type, displayId, name, IconName, castBarCaption, unk1, size, "
                                                            "Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Data9, Data10, Data11, Data12, "
                                                            "Data13, Data14, Data15, Data16, Data17, Data18, Data19, Data20, Data21, Data22, Data23, AIName, ScriptName "
                                                            "FROM gameobject_template");
    _queryStrings.emplace(DBCacheTable::GameobjectTemplateAddon, "SELECT entry, faction, flags, mingold, maxgold, artkit0, artkit1, artkit2, artkit3 FROM gameobject_template_addon");
    _queryStrings.emplace(DBCacheTable::TransportTemplates, "SELECT entry FROM gameobject_template WHERE type = 15 ORDER BY entry ASC");
    _queryStrings.emplace(DBCacheTable::Gameobject, "SELECT gameobject.guid, id, map, position_x, position_y, position_z, orientation, "
                                                    "rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, spawnMask, phaseMask, eventEntry, pool_entry, "
                                                    "ScriptName "
                                                    "FROM gameobject LEFT OUTER JOIN game_event_gameobject ON gameobject.guid = game_event_gameobject.guid "
                                                    "LEFT OUTER JOIN pool_gameobject ON gameobject.guid = pool_gameobject.guid");
    _queryStrings.emplace(DBCacheTable::GameobjectAddon, "SELECT guid, invisibilityType, invisibilityValue FROM gameobject_addon");
    _queryStrings.emplace(DBCacheTable::GameObjectQuestItem, "SELECT GameObjectEntry, ItemId FROM gameobject_questitem ORDER BY Idx ASC");
}
