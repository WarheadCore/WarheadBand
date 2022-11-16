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

void DBCacheMgr::InitPoolStrings()
{
    _queryStrings.emplace(DBCacheTable::PoolTemplate, "SELECT entry, max_limit FROM pool_template");
    _queryStrings.emplace(DBCacheTable::PoolCreature, "SELECT guid, pool_entry, chance FROM pool_creature");
    _queryStrings.emplace(DBCacheTable::PoolGameobject, "SELECT guid, pool_entry, chance FROM pool_gameobject");
    _queryStrings.emplace(DBCacheTable::PoolPool, "SELECT pool_id, mother_pool, chance FROM pool_pool");
    _queryStrings.emplace(DBCacheTable::PoolObjects, "SELECT DISTINCT pool_template.entry, pool_pool.pool_id, pool_pool.mother_pool FROM pool_template"
                                                  " LEFT JOIN game_event_pool ON pool_template.entry=game_event_pool.pool_entry"
                                                  " LEFT JOIN pool_pool ON pool_template.entry=pool_pool.pool_id WHERE game_event_pool.pool_entry IS NULL");
}
