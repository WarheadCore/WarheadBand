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

#include "NodeSocket.h"
#include "NodeMgr.h"
#include "NodeCodes.h"
#include "NodeSocketMgr.h"
#include "NodeSession.h"
#include "NodeSessionMgr.h"
#include "NodePktHeader.h"
#include "ObjectGuid.h"

using boost::asio::ip::tcp;

NodeSocket::NodeSocket(tcp::socket&& socket)
    : Socket(std::move(socket)), _nodeSession(nullptr), _authed(false), _sendBufferSize(4096)
{
    _headerBuffer.Resize(sizeof(NodeClientPktHeader));
}

NodeSocket::~NodeSocket()
{
    LOG_WARN("server", "~NodeSocket");
}

void NodeSocket::Start()
{
    LOG_INFO("server", "> Connect from {}", GetRemoteIpAddress().to_string());
    AsyncRead();
}

bool NodeSocket::Update()
{
    WorldPacket* queued{ nullptr };
    MessageBuffer buffer(_sendBufferSize);

    while (_bufferQueue.GetNextPacket(queued))
    {
        NodeServerPktHeader header(queued->size() + sizeof(queued->GetOpcode()) + sizeof(uint32), queued->GetOpcode(), 0);
        if (buffer.GetRemainingSpace() < queued->size() + header.GetHeaderLength())
        {
            QueuePacket(std::move(buffer));
            //buffer.Resize(_sendBufferSize);
        }

        if (buffer.GetRemainingSpace() >= queued->size() + header.GetHeaderLength())
        {
            buffer.Write(header.header, header.GetHeaderLength());
            if (!queued->empty())
                buffer.Write(queued->contents(), queued->size());
        }
        else // single packet larger than 4096 bytes
        {
            MessageBuffer packetBuffer(queued->size() + header.GetHeaderLength());
            packetBuffer.Write(header.header, header.GetHeaderLength());
            if (!queued->empty())
                packetBuffer.Write(queued->contents(), queued->size());

            QueuePacket(std::move(packetBuffer));
        }

        delete queued;
    }

    if (buffer.GetActiveSize() > 0)
        QueuePacket(std::move(buffer));

    if (!BaseSocket::Update())
        return false;

    _queryProcessor.ProcessReadyCallbacks();

    return true;
}

void NodeSocket::OnClose()
{
    {
        std::lock_guard<std::mutex> sessionGuard(_nodeSessionLock);
        _nodeSession = nullptr;

        LOG_WARN("node", "> Node {} disconnected", _nodeInfo ? _nodeInfo->Name : "<unknown>");
    }
}

void NodeSocket::ReadHandler()
{
    if (!IsOpen())
        return;

    MessageBuffer& packet = GetReadBuffer();
    while (packet.GetActiveSize() > 0)
    {
        if (_headerBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the header
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _headerBuffer.GetRemainingSpace());
            _headerBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_headerBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole header this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }

            // We just received nice new header
            if (!ReadHeaderHandler())
            {
                CloseSocket();
                return;
            }
        }

        // We have full read header, now check the data payload
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need more data in the payload
            std::size_t readDataSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readDataSize);
            packet.ReadCompleted(readDataSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole data this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }
        }

        // just received fresh new payload
        ReadDataHandlerResult result = ReadDataHandler();
        _headerBuffer.Reset();
        if (result != ReadDataHandlerResult::Ok)
        {
            if (result != ReadDataHandlerResult::WaitingForQuery)
                CloseSocket();

            return;
        }
    }

    packet.Reset();

    AsyncRead();
}

bool NodeSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(NodeClientPktHeader));

    NodeClientPktHeader* header = reinterpret_cast<NodeClientPktHeader*>(_headerBuffer.GetReadPointer());
    EndianConvertReverse(header->size);
    EndianConvert(header->cmd);
    EndianConvert(header->accoundID);

    if (!header->IsValidSize() || !header->IsValidOpcode() || !header->IsValidAccountForNodeSocket())
    {
        OpcodeNode nodeCode = static_cast<OpcodeNode>(header->cmd);

        LOG_ERROR("network", "NodeSocket::ReadHeaderHandler(): client {} sent malformed packet (size: {}, {})",
            GetRemoteIpAddress().to_string(), header->size, header->cmd, GetOpcodeNameForLogging(nodeCode));

        return false;
    }

    header->size -= sizeof(header->cmd);
    header->size -= sizeof(header->accoundID);
    _packetBuffer.Resize(header->size);
    return true;
}

NodeSocket::ReadDataHandlerResult NodeSocket::ReadDataHandler()
{
    NodeClientPktHeader* header = reinterpret_cast<NodeClientPktHeader*>(_headerBuffer.GetReadPointer());
    OpcodeNode nodeCode = static_cast<OpcodeNode>(header->cmd);

    WorldPacket packet(nodeCode, std::move(_packetBuffer));
    WorldPacket* packetToQueue{ nullptr };

    std::unique_lock<std::mutex> sessionGuard(_nodeSessionLock, std::defer_lock);

    switch (nodeCode)
    {
    case NODE_CONNECT_NEW_NODE:
    {
        LogNodeCodeText(nodeCode, sessionGuard);
        if (_authed)
        {
            // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
            if (sessionGuard.try_lock())
                LOG_ERROR("network", "NodeSocket::ProcessIncoming: received duplicate NODE_CONNECT_NEW_NODE from {}", _nodeSession->GetNodeInfo());
            return ReadDataHandlerResult::Error;
        }

        try
        {
            HandleAuthNodeSession(packet);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        catch (ByteBufferException const&) {}

        LOG_ERROR("network", "NodeSocket::ReadDataHandler(): client {} sent malformed NODE_CONNECT_NEW_NODE", GetRemoteIpAddress().to_string());
        return ReadDataHandlerResult::Error;
    }
    default:
        packetToQueue = new WorldPacket(std::move(packet));
        break;
    }

    sessionGuard.lock();

    LogNodeCodeText(nodeCode, sessionGuard);

    if (!_nodeSession)
    {
        LOG_ERROR("network.nodeCode", "ProcessIncoming: Client not authed nodeCode = {}", uint32(nodeCode));
        delete packetToQueue;
        return ReadDataHandlerResult::Error;
    }

    NodeCodeHandler const* handler = nodeCodeTable[nodeCode];
    if (!handler)
    {
        LOG_ERROR("network.nodeCode", "No defined handler for nodeCode {} sent by {}", GetOpcodeNameForLogging(static_cast<OpcodeNode>(packet.GetOpcode())), _nodeSession->GetNodeInfo());
        delete packetToQueue;
        return ReadDataHandlerResult::Error;
    }

    // Copy the packet to the heap before enqueuing
    _nodeSession->QueuePacket(*packetToQueue);

    return ReadDataHandlerResult::Ok;
}

void NodeSocket::LogNodeCodeText(OpcodeNode nodeCode, std::unique_lock<std::mutex> const& guard) const
{
    if (!guard)
    {
        LOG_TRACE("node.code", "C->S: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(nodeCode));
    }
    else
    {
        LOG_TRACE("node.code", "C->S: {} {}", (_nodeSession ? _nodeSession->GetNodeInfo() : GetRemoteIpAddress().to_string()),
            GetOpcodeNameForLogging(nodeCode));
    }
}

void NodeSocket::SendPacketAndLogNodeCode(WorldPacket const& packet)
{
    LOG_TRACE("node.code", "S->C: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(static_cast<OpcodeNode>(packet.GetOpcode())));
    SendPacket(packet);
}

void NodeSocket::SendPacket(WorldPacket const& packet)
{
    if (!IsOpen())
        return;

    _bufferQueue.AddPacket(new WorldPacket(packet));
}

void NodeSocket::HandleAuthNodeSession(WorldPacket& recvPacket)
{
    // Read the content of the packet
    uint32 nodeID{ 0 };
    recvPacket >> nodeID;

    auto const& nodeInfo = sNodeMgr->GetNodeInfo(nodeID);
    if (!nodeInfo)
    {
        LOG_ERROR("node", "{}; Incorrect node id {}", __FUNCTION__, nodeID);
        SendNodeConnectResponce(NodeConnectResponce::Error);
        return;
    }

    if (nodeInfo->Type != NodeType::Realm)
    {
        LOG_ERROR("node", "{}; Try connect from non realm node. ID {}. Name {}.", __FUNCTION__, nodeInfo->ID, nodeInfo->Name);
        SendNodeConnectResponce(NodeConnectResponce::Error);
        return;
    }

    _nodeInfo = std::make_unique<NodeInfo>(sNodeMgr->GetNodeInfo(nodeID));

    LOG_INFO("node.auth", "> Node '{}' connected to this node!", _nodeInfo->Name);

    _authed = true;

    _nodeSession = std::make_shared<NodeSession>(_nodeInfo->ID, nodeInfo->Name, shared_from_this());

    sNodeSessionMgr->AddSession(_nodeSession);

    SendNodeConnectResponce(NodeConnectResponce::Ok);

    AsyncRead();
}

void NodeSocket::SendNodeConnectResponce(NodeConnectResponce code)
{
    WorldPacket responce(NODE_CONNECT_RESPONCE, 1);
    responce << static_cast<uint8>(code);
    SendPacket(responce);
}
