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

#include "NodeSession.h"
#include "NodeCodes.h"
#include "Log.h"

NodeSession::NodeSession(uint32 id, std::string_view name, std::shared_ptr<NodeSocket> sock) :
    _nodeId(id), _nodeName(std::string(name)), _socket(sock)
{
    if (sock)
        _address = sock->GetRemoteIpAddress().to_string();
}

NodeSession::~NodeSession()
{
    /// - If have unclosed socket, close it
    if (_socket)
    {
        _socket->CloseSocket();
        _socket = nullptr;
    }
}

std::string NodeSession::GetNodeInfo() const
{
    return Warhead::StringFormat("[Node: {}, {}]", _nodeName, _nodeId);
}

/// Send a packet to the client
void NodeSession::SendPacket(WorldPacket const* packet)
{
    if (packet->GetOpcode() == NULL_OPCODE)
    {
        LOG_ERROR("network.node", "{} send NULL_OPCODE", GetNodeInfo());
        return;
    }

    if (!_socket)
        return;

    //LOG_INFO("node.code", "S->C: {} {}", GetNodeInfo(), GetOpcodeNameForLogging(static_cast<OpcodeNode>(packet->GetOpcode())));
    _socket->SendPacket(*packet);
}

/// Add an incoming packet to the queue
void NodeSession::QueuePacket(WorldPacket const& packet)
{
    _recvQueue.AddPacket(new WorldPacket(packet));
}

/// Logging helper for unexpected opcodes
void NodeSession::LogUnprocessedTail(WorldPacket* packet)
{
    if (!sLog->ShouldLog("node", LogLevel::LOG_LEVEL_TRACE) || packet->rpos() >= packet->wpos())
        return;

    LOG_TRACE("node", "Unprocessed tail data (read stop at {} from {}) NodeCode {} from {}",
        uint32(packet->rpos()), uint32(packet->wpos()), GetOpcodeNameForLogging(static_cast<OpcodeNode>(packet->GetOpcode())), GetNodeInfo());

    packet->print_storage();
}

/// Update the NodeSession (triggered by World update)
bool NodeSession::Update()
{
    ///- Retrieve packets from the receive queue and call the appropriate handlers
    /// not process packets if socket already closed
    WorldPacket* packet{ nullptr };

    while (_socket && _recvQueue.GetNextPacket(packet))
    {
        OpcodeNode nodeCode = static_cast<OpcodeNode>(packet->GetOpcode());
        ClientNodeCodeHandler const* nodeCodeHandle = nodeCodeTable[nodeCode];

        try
        {
            nodeCodeHandle->Call(this, *packet);
            LogUnprocessedTail(packet);
        }
        catch (ByteBufferException const&)
        {
            LOG_ERROR("network", "NodeSession::Update ByteBufferException occured while parsing a packet (nodeCode: {}) from client {}, id={}. Skipped packet.", packet->GetOpcode(), GetRemoteAddress(), GetNodeId());
            if (sLog->ShouldLog("network", LogLevel::LOG_LEVEL_DEBUG))
            {
                LOG_DEBUG("network", "Dumping error causing packet:");
                packet->hexlike();
            }
        }

        delete packet;
    }

    ProcessQueryCallbacks();
    return true;
}

bool NodeSession::HandleSocketClosed()
{
    if (_socket && !_socket->IsOpen() && !World::IsStopped())
    {
        _socket = nullptr;
        return true;
    }

    return false;
}

bool NodeSession::IsSocketClosed() const
{
    return !_socket || !_socket->IsOpen();
}

void NodeSession::Handle_NULL(WorldPacket& null)
{
    LOG_ERROR("network.node", "Received unhandled nodeCode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeNode>(null.GetOpcode())), GetNodeInfo());
}

void NodeSession::Handle_EarlyProccess(WorldPacket& recvPacket)
{
    LOG_ERROR("network.node", "Received nodeCode {} that must be processed in WorldSocket::ReadDataHandler from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeNode>(recvPacket.GetOpcode())), GetNodeInfo());
}

void NodeSession::Handle_ServerSide(WorldPacket& recvPacket)
{
    LOG_ERROR("network.node", "Received server-side nodeCode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeNode>(recvPacket.GetOpcode())), GetNodeInfo());
}

void NodeSession::Handle_Deprecated(WorldPacket& recvPacket)
{
    LOG_ERROR("network.node", "Received deprecated nodeCode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeNode>(recvPacket.GetOpcode())), GetNodeInfo());
}

void NodeSession::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
}

void NodeSession::ChangeNodeSession(uint32 oldNodeID, uint32 newNodeID, uint32 accountID, ObjectGuid playerGuid)
{
    WorldPacket packet(NODE_CHANGE_NODE_SESSION, 1);
    packet << uint32(oldNodeID);
    packet << uint32(newNodeID);
    packet << uint32(accountID);
    packet << playerGuid;
    SendPacket(&packet);
}
