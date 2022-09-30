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

#include "DatabaseWorkerPool.h"
#include "Errors.h"
#include "Log.h"
#include "MySQLWorkaround.h"
#include "QueryResult.h"
#include "MySQLConnection.h"
#include "MySQLPreparedStatement.h"
#include "PreparedStatement.h"
#include "QueryCallback.h"
#include "PCQueue.h"
#include "Transaction.h"
#include "QueryHolder.h"
#include "TaskScheduler.h"
#include "Config.h"
#include <limits>
#include <utility>
#include <mysqld_error.h>

#ifdef WARHEAD_DEBUG
#include <boost/stacktrace.hpp>
#include <sstream>
#endif

#ifdef LIBMARIADB && MARIADB_VERSION_ID >= 100600
#define MIN_DB_SERVER_VERSION 100300u
#define MIN_DB_CLIENT_VERSION 30203u
#else
#define MIN_DB_SERVER_VERSION 50700u
#define MIN_DB_CLIENT_VERSION 50700u
#endif

constexpr auto MAX_SYNC_CONNECTIONS = 32;
constexpr auto MAX_ASYNC_CONNECTIONS = 32;

class PingOperation : public AsyncOperation
{
public:
    explicit PingOperation() :
        AsyncOperation() { }

    //! Operation for idle delay threads
    void ExecuteQuery() override
    {
        _connection->Ping();
    }
};

DatabaseWorkerPool::DatabaseWorkerPool(DatabaseType type) :
    _poolType(type)
{
    ASSERT(mysql_thread_safe(), "Used MySQL library isn't thread-safe");

#ifndef LIBMARIADB || MARIADB_VERSION_ID < 100600u
    bool isSupportClientDB = mysql_get_client_version() >= MIN_DB_CLIENT_VERSION;
    bool isSameClientDB = mysql_get_client_version() == MYSQL_VERSION_ID;
#else // MariaDB 10.6+
    bool isSupportClientDB = mysql_get_client_version() >= MIN_DB_CLIENT_VERSION;
    bool isSameClientDB    = true; // Client version 3.2.3?
#endif

    ASSERT(isSupportClientDB, "WarheadCore does not support MySQL versions below 5.7 and MariaDB 10.3\nSearch the wiki for ACE00043 in Common Errors (https://www.azerothcore.org/wiki/common-errors).");
    ASSERT(isSameClientDB, "Used DB library version ({} id {}) does not match the version id used to compile WarheadCore (id {}).\nSearch the wiki for ACE00046 in Common Errors (https://www.azerothcore.org/wiki/common-errors).",
        mysql_get_client_info(), mysql_get_client_version(), MYSQL_VERSION_ID);

    _scheduler = std::make_unique<TaskScheduler>();
}

DatabaseWorkerPool::~DatabaseWorkerPool()
{
    if (_scheduler)
        _scheduler->CancelAll();
}

void DatabaseWorkerPool::SetConnectionInfo(std::string_view infoString)
{
    _connectionInfo = std::make_unique<MySQLConnectionInfo>(infoString);
}

uint32 DatabaseWorkerPool::Open()
{
    ASSERT(_connectionInfo, "Connection info was not set!");

    LOG_INFO("db.pool", "Opening DatabasePool '{}'", GetDatabaseName());

    // Async connection
    {
        auto [error, connection] = OpenConnection(IDX_ASYNC);
        if (error)
            return error;
    }

    // Sync connection
    auto [error, connection] = OpenConnection(IDX_SYNCH);
    if (error)
        return error;

    LOG_INFO("db.pool", "DatabasePool '{}' opened successfully", GetDatabaseName());
    LOG_INFO("db.pool", "DB server ver: {}", connection->GetServerInfo());
    LOG_INFO("db.pool", "");
    AddTasks();
    return error;
}

void DatabaseWorkerPool::Close()
{
    if (_connections[IDX_ASYNC].empty() && _connections[IDX_SYNCH].empty())
        return;

    LOG_INFO("db.pool", "Closing down DatabasePool '{}' ...", GetDatabaseName());

    //! Closes the actually DB connection.
    _connections[IDX_ASYNC].clear();

    //! Shut down the synchronous connections
    //! There's no need for locking the connection, because DatabaseWorkerPool<>::Close
    //! should only be called after any other thread tasks in the core have exited,
    //! meaning there can be no concurrent access at this point.
    _connections[IDX_SYNCH].clear();

    LOG_INFO("db.pool", "All connections on DatabasePool '{}' closed.", GetDatabaseName());
}

QueryResult DatabaseWorkerPool::Query(std::string_view sql)
{
    auto connection = GetFreeConnection();
    if (!connection)
        return { nullptr };

    auto result = connection->Query(sql);
    connection->Unlock();

    if (!result || !result->GetRowCount() || !result->NextRow())
        return { nullptr };

    return { result };
}

std::pair<uint32, MySQLConnection*> DatabaseWorkerPool::OpenConnection(InternalIndex type, bool isDynamic /*= false*/)
{
    auto connection = std::make_unique<MySQLConnection>(*_connectionInfo, type == IDX_ASYNC, isDynamic);
    if (uint32 error = connection->Open())
    {
        // Failed to open a connection or invalid version
        return { error, nullptr };
    }
    else if (connection->GetServerVersion() < MIN_DB_SERVER_VERSION)
    {
        LOG_ERROR("db.pool", "WarheadCore does not support MySQL versions below 5.7 or MariaDB versions below 10.3");
        return { 1, nullptr };
    }

    auto& itrConnection = _connections[type].emplace_back(std::move(connection));

    // Everything is fine
    return { 0, itrConnection.get() };
}

MySQLConnection* DatabaseWorkerPool::GetFreeConnection()
{
#ifdef WARHEAD_DEBUG
    if (_warnSyncQueries)
    {
        std::ostringstream ss;
        ss << boost::stacktrace::stacktrace();
        LOG_WARN("db.pool", "Sync query at:\n{}", ss.str());
    }
#endif

    std::lock_guard<std::mutex> guardCleanup(_cleanupMutex);

    // Check default connections
    for (auto& connection : _connections[IDX_SYNCH])
        if (connection->LockIfReady())
            return connection.get();

    LOG_WARN("db.pool", "> Not found free sync connection. Connections count: {}", _connections[IDX_SYNCH].size());

    // Try to make new connect if connections count < MAX_SYNC_CONNECTIONS
    OpenDynamicSyncConnect();

    std::lock_guard<std::mutex> guardOpenConnect(_openSyncConnectMutex);

    MySQLConnection* freeConnection{ nullptr };

    uint8 i{};
    auto const num_cons{ _connections[IDX_SYNCH].size()};

    //! Block forever until a connection is free
    for (;;)
    {
        freeConnection = _connections[IDX_SYNCH][++i % num_cons].get();

        //! Must be matched with t->Unlock() or you will get deadlocks
        if (freeConnection->LockIfReady())
            break;
    }

    return freeConnection;
}

std::string_view DatabaseWorkerPool::GetDatabaseName() const
{
    return std::string_view{ _connectionInfo->Database };
}

void DatabaseWorkerPool::Execute(std::string_view sql)
{
    if (sql.empty())
        return;

    Enqueue(new BasicStatementTask(sql));
}

void DatabaseWorkerPool::Execute(PreparedStatement stmt)
{
    if (!stmt)
        return;

    Enqueue(new PreparedStatementTask(std::move(stmt)));
}

void DatabaseWorkerPool::CleanupConnections()
{
    std::lock_guard<std::mutex> guard(_cleanupMutex);

    _connections[IDX_SYNCH].erase(std::remove_if(_connections[IDX_SYNCH].begin(), _connections[IDX_SYNCH].end(), [](std::unique_ptr<MySQLConnection>& connection)
    {
        return connection->CanRemoveConnection();
    }), _connections[IDX_SYNCH].end());

    _connections[IDX_ASYNC].erase(std::remove_if(_connections[IDX_ASYNC].begin(), _connections[IDX_ASYNC].end(), [](std::unique_ptr<MySQLConnection>& connection)
    {
        return connection->CanRemoveConnection();
    }), _connections[IDX_ASYNC].end());
}

bool DatabaseWorkerPool::PrepareStatements()
{
    // Init all prepare statements
    DoPrepareStatements();

    for (auto const& [index, stmt] : _stringPreparedStatement)
        for (auto const& connections : _connections)
            for (auto const& connection : connections)
                connection->PrepareStatement(index, stmt.Query, stmt.ConnectionType);

    auto IsValidPrepareStatements = [this](MySQLConnection* connection)
    {
        if (!connection->LockIfReady())
            return false;

        if (!connection->PrepareStatements())
        {
            connection->Unlock();
            Close();
            return false;
        }
        else
            connection->Unlock();

        auto list = connection->GetPreparedStatementList();
        auto const preparedSize = list->size();

        if (_preparedStatementSize.size() < preparedSize)
            _preparedStatementSize.resize(preparedSize);

        for (uint32 i = 0; i < preparedSize; ++i)
        {
            // already set by another connection
            // (each connection only has prepared statements of its own type sync/async)
            if (_preparedStatementSize[i] > 0)
                continue;

            auto const& itr = list->find(i);
            if (itr != list->end())
            {
                auto stmt = itr->second.get();
                if (stmt)
                {
                    uint32 const paramCount = stmt->GetParameterCount();

                    // WH only supports uint8 indices.
                    ASSERT(paramCount < std::numeric_limits<uint8>::max());

                    _preparedStatementSize[i] = static_cast<uint8>(paramCount);
                }
            }
        }

        return true;
    };

    for (auto const& connections : _connections)
        for (auto const& connection : connections)
            if (!IsValidPrepareStatements(connection.get()))
                return false;

    return true;
}

PreparedStatement DatabaseWorkerPool::GetPreparedStatement(uint32 index)
{
    return std::make_shared<PreparedStatementBase>(index, _preparedStatementSize[index]);
}

void DatabaseWorkerPool::PrepareStatement(uint32 index, std::string_view sql, ConnectionFlags flags)
{
    auto const& itr = _stringPreparedStatement.find(index);
    if (itr != _stringPreparedStatement.end())
    {
        LOG_ERROR("db.pool", "> Prepared statement with index () is exist! Skip", index);
        return;
    }

    _stringPreparedStatement.emplace(index, StringPreparedStatement{ index, sql, flags });
}

PreparedQueryResult DatabaseWorkerPool::Query(PreparedStatement stmt)
{
    auto connection = GetFreeConnection();
    if (!connection)
        return { nullptr };

    auto result = connection->Query(std::move(stmt));
    connection->Unlock();

    if (!result || !result->GetRowCount())
        return { nullptr };

    return { result };
}

void DatabaseWorkerPool::InitPrepareStatement(MySQLConnection* connection)
{
    for (auto const& [index, stmt] : _stringPreparedStatement)
        connection->PrepareStatement(index, stmt.Query, stmt.ConnectionType);
}

QueryCallback DatabaseWorkerPool::AsyncQuery(std::string_view sql)
{
    auto task = new BasicStatementTask(sql, true);
    auto result = task->GetFuture();
    Enqueue(task);
    return QueryCallback(std::move(result));
}

QueryCallback DatabaseWorkerPool::AsyncQuery(PreparedStatement stmt)
{
    auto task = new PreparedStatementTask(std::move(stmt), true);
    auto result = task->GetFuture();
    Enqueue(task);
    return QueryCallback(std::move(result));
}

SQLTransaction DatabaseWorkerPool::BeginTransaction()
{
    return std::make_shared<Transaction>();
}

void DatabaseWorkerPool::CommitTransaction(SQLTransaction transaction)
{
#ifdef WARHEAD_DEBUG
    //! Only analyze transaction weaknesses in Debug mode.
    //! Ideally we catch the faults in Debug mode and then correct them,
    //! so there's no need to waste these CPU cycles in Release mode.
    switch (transaction->GetSize())
    {
    case 0:
        LOG_DEBUG("db.pool", "Transaction contains 0 queries. Not executing.");
        return;
    case 1:
        LOG_DEBUG("db.pool", "Warning: Transaction only holds 1 query, consider removing Transaction context in code.");
        break;
    default:
        break;
    }
#endif // WARHEAD_DEBUG

    Enqueue(new TransactionTask(std::move(transaction)));
}

TransactionCallback DatabaseWorkerPool::AsyncCommitTransaction(SQLTransaction transaction)
{
#ifdef WARHEAD_DEBUG
    //! Only analyze transaction weaknesses in Debug mode.
    //! Ideally we catch the faults in Debug mode and then correct them,
    //! so there's no need to waste these CPU cycles in Release mode.
    switch (transaction->GetSize())
    {
        case 0:
            LOG_DEBUG("db.pool", "Transaction contains 0 queries. Not executing.");
            break;
        case 1:
            LOG_DEBUG("db.pool", "Warning: Transaction only holds 1 query, consider removing Transaction context in code.");
            break;
        default:
            break;
    }
#endif // WARHEAD_DEBUG

    auto task = new TransactionWithResultTask(std::move(transaction));
    TransactionFuture result = task->GetFuture();
    Enqueue(task);
    return TransactionCallback{ std::move(result) };
}

void DatabaseWorkerPool::Enqueue(AsyncOperation* operation)
{
    auto mostFreeConnection{ _connections[IDX_ASYNC].front().get() };
    auto queueSize{ mostFreeConnection->GetQueueSize() };

    if (!queueSize || _connections[IDX_ASYNC].size() == 1)
    {
        mostFreeConnection->Enqueue(operation);
        return;
    }

    std::lock_guard<std::mutex> guardCleanup(_cleanupMutex);

    auto allQueueSize{ GetQueueSize() };
    if (allQueueSize > _maxQueueSize)
    {
        LOG_WARN("db.pool", "Async queue overload. Queue size: {}. Pool name: {}. Connection size: {}", allQueueSize, _poolName, _connections[IDX_ASYNC].size());
        OpenDynamicAsyncConnect();
    }

    std::lock_guard<std::mutex> guardOpenConnect(_openAsyncConnectMutex);

    for (auto const& connection : _connections[IDX_ASYNC])
    {
        auto qSize{ connection->GetQueueSize() };
        if (qSize >= queueSize)
            continue;

        mostFreeConnection = connection.get();
        queueSize = qSize;
    }

    mostFreeConnection->Enqueue(operation);
}

void DatabaseWorkerPool::DirectCommitTransaction(SQLTransaction transaction)
{
    auto connection = GetFreeConnection();

    auto errorCode = connection->ExecuteTransaction(transaction);
    if (!errorCode)
    {
        connection->Unlock(); // OK, operation succesful
        return;
    }

    //! Handle MySQL Errno 1213 without extending deadlock to the core itself
    /// @todo More elegant way
    if (errorCode == ER_LOCK_DEADLOCK)
    {
        //todo: handle multiple sync threads deadlocking in a similar way as async threads
        uint8 loopBreaker = 5;

        for (uint8 i = 0; i < loopBreaker; ++i)
        {
            if (!connection->ExecuteTransaction(transaction))
                break;
        }
    }

    //! Clean up now.
    transaction->Cleanup();
    connection->Unlock();
}

void DatabaseWorkerPool::ExecuteOrAppend(SQLTransaction trans, std::string_view sql)
{
    if (!trans)
        Execute(sql);
    else
        trans->Append(sql);
}

void DatabaseWorkerPool::ExecuteOrAppend(SQLTransaction trans, PreparedStatement stmt)
{
    if (!trans)
        Execute(std::move(stmt));
    else
        trans->Append(std::move(stmt));
}

void DatabaseWorkerPool::DirectExecute(std::string_view sql)
{
    if (sql.empty())
        return;

    auto connection = GetFreeConnection();
    connection->Execute(sql);
    connection->Unlock();
}

void DatabaseWorkerPool::DirectExecute(PreparedStatement stmt)
{
    auto connection = GetFreeConnection();
    connection->Execute(std::move(stmt));
    connection->Unlock();
}

void DatabaseWorkerPool::EscapeString(std::string& str)
{
    if (str.empty())
        return;

    char* buf = new char[str.size() * 2 + 1];
    EscapeString(buf, str.c_str(), uint32(str.size()));
    str = buf;
    delete[] buf;
}

void DatabaseWorkerPool::KeepAlive()
{
    std::lock_guard<std::mutex> guard(_cleanupMutex);

    //! Ping synchronous connection
    auto& connection = _connections[IDX_SYNCH].front();
    if (connection->LockIfReady())
    {
        connection->Ping();
        connection->Unlock();
    }

    //! Ping asynchronous connection
    Enqueue(new PingOperation);
}

std::size_t DatabaseWorkerPool::GetQueueSize() const
{
    std::size_t queueSize{};

    for (auto const& connection : _connections[IDX_ASYNC])
        queueSize += connection->GetQueueSize();

    return queueSize;
}

unsigned long DatabaseWorkerPool::EscapeString(char* to, char const* from, unsigned long length)
{
    if (!to || !from || !length)
        return 0;

    return _connections[IDX_SYNCH].front()->EscapeString(to, from, length);
}

SQLQueryHolderCallback DatabaseWorkerPool::DelayQueryHolder(SQLQueryHolder holder)
{
    auto task = new SQLQueryHolderTask(holder);
    QueryResultHolderFuture result = task->GetFuture();
    Enqueue(task);
    return { std::move(holder), std::move(result) };
}

void DatabaseWorkerPool::Update(Milliseconds diff)
{
    if (_scheduler)
    {
        if (diff > 0ms)
            _scheduler->Update(diff);
        else
            _scheduler->Update();
    }
}

void DatabaseWorkerPool::AddTasks()
{
    if (sConfigMgr->isDryRun())
        return;

    if (_isEnableDynamicConnections)
        _maxQueueSize = sConfigMgr->GetOption<uint32>("MaxQueueSize", 50);

    // DB ping
    _scheduler->Schedule(Minutes{ sConfigMgr->GetOption<uint32>("MaxPingTime", 30) }, [this](TaskContext context)
    {
        LOG_DEBUG("db.connection", "Ping DB to keep connection alive. Pool name: {}", _poolName);
        KeepAlive();
        context.Repeat();
    });

    // Cleanup dynamic connections
    _scheduler->Schedule(10s, [this](TaskContext context)
    {
//        LOG_DEBUG("db.connection", "Cleanup dynamic connections...");
        CleanupConnections();
        context.Repeat();
    });
}

void DatabaseWorkerPool::OpenDynamicAsyncConnect()
{
    std::lock_guard guard(_openAsyncConnectMutex);

    if (_connections[IDX_ASYNC].size() >= MAX_ASYNC_CONNECTIONS || !_isEnableDynamicConnections)
        return;

    LOG_INFO("db.pool", "Add new dynamic async connection...");

    auto [error, connection] = OpenConnection(IDX_ASYNC, true);
    if (!error)
    {
        InitPrepareStatement(connection);
        ASSERT(connection->PrepareStatements(), "Can't register prepare statements for dynamic async connection");
        connection->RegisterThread();
    }
}

void DatabaseWorkerPool::OpenDynamicSyncConnect()
{
    std::lock_guard guard(_openSyncConnectMutex);

    if (_connections[IDX_SYNCH].size() >= MAX_SYNC_CONNECTIONS || !_isEnableDynamicConnections)
        return;

    LOG_INFO("db.pool", "Add new dynamic sync connection...");

    auto [error, connection] = OpenConnection(IDX_SYNCH, true);
    if (!error)
    {
        InitPrepareStatement(connection);
        ASSERT(connection->PrepareStatements());
    }
}