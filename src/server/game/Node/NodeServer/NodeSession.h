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

#ifndef _NODE_SESSION_H
#define _NODE_SESSION_H

#include "Define.h"
#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "NodeSocket.h"
#include "WorldPacket.h"
#include "NodeMgr.h"
#include "PacketQueue.h"

class WH_GAME_API NodeSession : std::enable_shared_from_this<NodeSession>
{
public:
    NodeSession(uint32 id, std::string_view name, std::shared_ptr<NodeSocket> sock);
    ~NodeSession();

    void SendPacket(WorldPacket const* packet);
    void ChangeNodeSession(uint32 oldNodeID, uint32 newNodeID, uint32 accountID, ObjectGuid playerGuid);

    std::shared_ptr<NodeSocket> GetNodeSocket() { return _socket; }

    uint32 GetNodeId() const { return _nodeId; }
    std::string GetNodeName() const { return _nodeName; }
    std::string const& GetRemoteAddress() { return _address; }

    bool HandleSocketClosed();
    bool IsSocketClosed() const;

    void QueuePacket(WorldPacket const& packet);
    bool Update();

    // opcodes handlers
    void Handle_NULL(WorldPacket& null);                // not used
    void Handle_EarlyProccess(WorldPacket& recvPacket); // just mark packets processed in WorldSocket::OnRead
    void Handle_ServerSide(WorldPacket& recvPacket);    // sever side only, can't be accepted from client
    void Handle_Deprecated(WorldPacket& recvPacket);    // never used anymore by client

    std::string GetNodeInfo() const;

    // Packets
    void HandleTest(WorldPacket& recvPacket);
    void HandleAddAccount(WorldPacket& recvPacket);
    void HandleChangeNodeForSession(WorldPacket& recvPacket);

    /*
     * CALLBACKS
     */

    QueryCallbackProcessor& GetQueryProcessor() { return _queryProcessor; }

    std::shared_ptr<NodeSession> GetPtr() { return shared_from_this(); }

private:
    void ProcessQueryCallbacks();

    QueryCallbackProcessor _queryProcessor;

    // logging helper
    void LogUnprocessedTail(WorldPacket* packet);

    std::shared_ptr<NodeSocket> _socket;
    std::string _address;
    uint32 _nodeId;
    std::string _nodeName;
    std::atomic<uint32> _latency;
    PacketQueue<WorldPacket> _recvQueue;
    NodeType _nodeType{ NodeType::Master };

    NodeSession(NodeSession const& right) = delete;
    NodeSession& operator=(NodeSession const& right) = delete;
};

#endif
