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

#ifndef WARHEAD_ASYNC_DB_LOAD_MGR_H_
#define WARHEAD_ASYNC_DB_LOAD_MGR_H_

#include "AsyncDBLoadStrings.h"
#include "DatabaseEnvFwd.h"
#include <unordered_map>
#include <string>
#include <string_view>

class WH_GAME_API AsyncDBLoadMgr
{
public:
    AsyncDBLoadMgr() = default;
    ~AsyncDBLoadMgr() = default;

    static AsyncDBLoadMgr* instance();

    void Initialize();

    void AddQuery(AsyncDBTable index);
    QueryResult GetResult(AsyncDBTable index);

private:
    void InitializeDefines();
    void InitializeQuery();

    std::string_view GetStringQuery(AsyncDBTable index);

    std::unordered_map<AsyncDBTable, QueryResultFuture> _queryList;
    std::unordered_map<AsyncDBTable, std::string> _queryStrings;

    AsyncDBLoadMgr(AsyncDBLoadMgr const&) = delete;
    AsyncDBLoadMgr(AsyncDBLoadMgr&&) = delete;
    AsyncDBLoadMgr& operator=(AsyncDBLoadMgr const&) = delete;
    AsyncDBLoadMgr& operator=(AsyncDBLoadMgr&&) = delete;
};

#define sAsyncDBLoadMgr AsyncDBLoadMgr::instance()

#endif
