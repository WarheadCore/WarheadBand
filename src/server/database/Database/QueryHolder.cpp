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

#include "QueryHolder.h"
#include "Errors.h"
#include "Log.h"
#include "PreparedStatement.h"
#include "QueryResult.h"
#include "MySQLConnection.h"
#include <utility>

bool SQLQueryHolderBase::SetPreparedQuery(std::size_t index, PreparedStatement stmt)
{
    if (_queries.size() <= index)
    {
        LOG_ERROR("db.query", "Query index ({}) out of range (size: {}) for prepared statement", index, _queries.size());
        return false;
    }

    _queries[index].first = std::move(stmt);
    return true;
}

PreparedQueryResult SQLQueryHolderBase::GetPreparedResult(std::size_t index) const
{
    // Don't call to this function if the index is of a prepared statement
    ASSERT(index < _queries.size(), "Query holder result index out of range, tried to access index {} but there are only {} results",
        index, _queries.size());

    return _queries[index].second;
}

void SQLQueryHolderBase::SetPreparedResult(std::size_t index, PreparedQueryResult result)
{
    if (result && !result->GetRowCount())
        result.reset();

    /// store the result in the holder
    if (index < _queries.size())
        _queries[index].second = PreparedQueryResult(result);
}

void SQLQueryHolderBase::SetSize(size_t size)
{
    /// to optimize push_back, reserve the number of queries about to be executed
    _queries.resize(size);
}

void SQLQueryHolderTask::ExecuteQuery()
{
    /// execute all queries in the holder and pass the results
    for (size_t i = 0; i < _holder->_queries.size(); ++i)
        if (auto stmt = _holder->_queries[i].first)
            _holder->SetPreparedResult(i, _connection->Query(stmt));

    _result.set_value();
}

bool SQLQueryHolderCallback::InvokeIfReady()
{
    if (_future.valid() && _future.wait_for(0s) == std::future_status::ready)
    {
        _callback(*_holder);
        return true;
    }

    return false;
}
