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

#ifndef WARHEAD_DB_CACHE_MGR_H_
#define WARHEAD_DB_CACHE_MGR_H_

#include "DBCacheStrings.h"
#include "DatabaseEnvFwd.h"
#include <unordered_map>
#include <string>
#include <string_view>

class WH_GAME_API DBCacheMgr
{
public:
    DBCacheMgr() = default;
    ~DBCacheMgr() = default;

    static DBCacheMgr* instance();

    void Initialize();

    void AddQuery(DBCacheTable index);
    QueryResult GetResult(DBCacheTable index);

private:
    void InitializeDefines();
    void InitializeQuery();

    //
    void InitGameLocaleStrings();
    void InitGameEventStrings();
    void InitSpellsStrings();
    void InitScriptStrings();
    void InitGossipStrings();
    void InitGameObjectStrings();
    void InitItemStrings();
    void InitCreatureStrings();
    void InitMiscStrings();
    void InitQuestStrings();
    void InitPoolStrings();

    std::string_view GetStringQuery(DBCacheTable index);

    std::unordered_map<DBCacheTable, QueryResultFuture> _queryList;
    std::unordered_map<DBCacheTable, std::string> _queryStrings;
    bool _isEnableAsyncLoad{};
    bool _isEnableWaitAtAdd{};

    DBCacheMgr(DBCacheMgr const&) = delete;
    DBCacheMgr(DBCacheMgr&&) = delete;
    DBCacheMgr& operator=(DBCacheMgr const&) = delete;
    DBCacheMgr& operator=(DBCacheMgr&&) = delete;
};

#define sDBCacheMgr DBCacheMgr::instance()

#endif
