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

#include "NodeDatabase.h"
#include "MySQLPreparedStatement.h"

void NodeDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_NODE_DATABASE_STATEMENTS);

    PrepareStatement(NODE_SEL_NODE_INFO, "SELECT `ID`, `Type`, `IP`, `WorldPort`, `NodePort`, `Name` FROM `node_info`", CONNECTION_SYNCH);
}

NodeDatabaseConnection::NodeDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo)
{
}

NodeDatabaseConnection::NodeDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo)
{
}

NodeDatabaseConnection::~NodeDatabaseConnection()
{
}
