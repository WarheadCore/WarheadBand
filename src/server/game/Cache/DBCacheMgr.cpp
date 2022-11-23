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
#include "DatabaseEnv.h"
#include "Util.h"
#include "StopWatch.h"
#include "GameConfig.h"

/*static*/ DBCacheMgr* DBCacheMgr::instance()
{
    static DBCacheMgr instance;
    return &instance;
}

void DBCacheMgr::Initialize()
{
    StopWatch sw;

    LOG_INFO("server.loading", "Initialize async db query list");

    _isEnableWaitAtAdd = CONF_GET_BOOL("DBCache.WaitAtAdd.Enable");

    InitializeDefines();
    InitializeQuery();

    LOG_INFO("server.loading", ">> Initialized async db query list in {}", sw);
    LOG_INFO("server.loading", "");
}

void DBCacheMgr::AddQuery(DBCacheTable index)
{
    if (_queryList.contains(index))
    {
        LOG_ERROR("db.async", "Query with index {} exist!");
        return;
    }

    auto sql{ GetStringQuery(index) };
    if (sql.empty())
    {
        LOG_ERROR("db.async", "Empty string for index {}", AsUnderlyingType(index));
        return;
    }

    auto task = new BasicStatementTask(sql, true);
    auto result = task->GetFuture();
    WorldDatabase.Enqueue(task);

    if (_isEnableWaitAtAdd)
        result.wait();

    _queryList.emplace(index, std::move(result));
}

QueryResult DBCacheMgr::GetResult(DBCacheTable index)
{
    auto const& itr = _queryList.find(index);
    if (itr == _queryList.end())
    {
        LOG_ERROR("db.async", "Not found query with index {}", AsUnderlyingType(index));

        auto sql{ GetStringQuery(index) };
        if (sql.empty())
        {
            LOG_ERROR("db.async", "Empty string for index {}", AsUnderlyingType(index));
            return nullptr;
        }

        return WorldDatabase.Query(sql);
    }

    itr->second.wait();
    auto result{ itr->second.get() };
    _queryList.erase(index);
    return result;
}

std::string_view DBCacheMgr::GetStringQuery(DBCacheTable index)
{
    auto const& itr = _queryStrings.find(index);
    if (itr == _queryStrings.end())
        return {};

    return itr->second;
}

void DBCacheMgr::InitializeQuery()
{
    for (std::size_t i{}; i < AsUnderlyingType(DBCacheTable::Max); i++)
        AddQuery(static_cast<DBCacheTable>(i));
}

void DBCacheMgr::InitializeDefines()
{
    // Prepare
    _queryStrings.rehash(AsUnderlyingType(DBCacheTable::Max));

    InitGameLocaleStrings();
    InitGameEventStrings();
    InitSpellsStrings();
    InitScriptStrings();
    InitGossipStrings();
    InitGameObjectStrings();
    InitItemStrings();
    InitCreatureStrings();
    InitMiscStrings();
    InitQuestStrings();
    InitPoolStrings();
}
