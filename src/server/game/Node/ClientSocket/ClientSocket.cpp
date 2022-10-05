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

#include "ClientSocket.h"
#include "NodePktHeader.h"
#include "NodeMgr.h"
#include "WorldSocket.h"
#include "DeadlineTimer.h"
#include "IoContext.h"

ClientSocket::ClientSocket(tcp::socket&& socket, boost::asio::io_context& ioContext) :
    Socket(std::move(socket))
{
    _headerBuffer.Resize(sizeof(NodeClientPktHeader));
    _updateTimer = std::make_unique<Warhead::Asio::DeadlineTimer>(ioContext);

    SetNoDelay(true);
}

void ClientSocket::Start()
{
    auto nodeInfoTarget = sNodeMgr->GetNodeInfo(GetNodeID());
    auto nodeInfoThis = sNodeMgr->GetThisNodeInfo();
    ASSERT(nodeInfoTarget && nodeInfoThis);

    _targetNodeName = nodeInfoTarget->Name;
    _thisNodeName = nodeInfoThis->Name;

    LOG_DEBUG("node", "Start process add account '{}', '{}'->'{}'", _accountName, _thisNodeName, _targetNodeName);

    WorldPacket packet(NODE_ADD_ACCOUNT, 1);
    packet << _accountName;
    SendPacket(&packet);

    _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _updateTimer->async_wait([this](boost::system::error_code const&) { Update(); });

    AsyncRead();
}

void ClientSocket::Stop()
{
    _updateTimer->cancel();
    _stop = true;
    BaseSocket::CloseSocket();
}

void ClientSocket::OnClose()
{
    _stop = true;
    LOG_WARN("node", "> Client '{}' disconnected from {}", _accountName, _targetNodeName);
}

bool ClientSocket::Update()
{
    if (_stop)
        return false;

    _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _updateTimer->async_wait([this](boost::system::error_code const&) { Update(); });

    if (_authed && IsOpen())
    {
        WorldPacket* queuedPacket{ nullptr };

        while (_bufferQueue.GetNextPacket(queuedPacket))
        {
            SendPacket(queuedPacket);
            delete queuedPacket;
        }
    }

    if (!BaseSocket::Update())
        return false;

    return true;
}

void ClientSocket::ReadHandler()
{
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

    AsyncRead();
}

bool ClientSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(NodeClientPktHeader));

    NodeClientPktHeader* header = reinterpret_cast<NodeClientPktHeader*>(_headerBuffer.GetReadPointer());
    EndianConvertReverse(header->size);
    EndianConvert(header->cmd);
    EndianConvert(header->accoundID);

    if (!header->IsValidSize() || !header->IsValidOpcode())
    {
        LOG_ERROR("node", "ClientSocket::ReadHeaderHandler(): node sent malformed packet (size: {}, cmd: {})", header->size, header->cmd);
        return false;
    }

    header->size -= sizeof(header->cmd);
    header->size -= sizeof(header->accoundID);
    _packetBuffer.Resize(header->size);
    return true;
}

ClientSocket::ReadDataHandlerResult ClientSocket::ReadDataHandler()
{
    NodeClientPktHeader* header = reinterpret_cast<NodeClientPktHeader*>(_headerBuffer.GetReadPointer());
    OpcodeClient opcode = static_cast<OpcodeClient>(header->cmd);

    WorldPacket packet(opcode, std::move(_packetBuffer));

    std::unique_lock<std::mutex> sessionGuard(_nodeSessionLock, std::defer_lock);

    LogOpcode(opcode);

    switch (opcode)
    {
        case NODE_CONNECT_RESPONCE:
        {
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("node", "{}: received duplicate NODE_CONNECT_RESPONCE", __FUNCTION__);

                return ReadDataHandlerResult::Error;
            }

            HandleAuthResponce(packet);
            return ReadDataHandlerResult::Ok;
        }
        default:
        {
            if (!_authed)
            {
                LOG_ERROR("node", "{}", __FUNCTION__);
                return ReadDataHandlerResult::Error;
            }

            _worldSocket->SendPacket(packet);
            return ReadDataHandlerResult::Ok;
        }
    }
}

void ClientSocket::LogOpcode(OpcodeClient opcode)
{
    LOG_TRACE("network.opcode", "{}->{}->Player: {}", _thisNodeName, _targetNodeName, GetOpcodeNameForLogging(opcode));
}

void ClientSocket::SendPacket(WorldPacket const* packet)
{
    if (!IsOpen())
    {
        LOG_ERROR("node", "{}: Not open socket!", __FUNCTION__);
        return;
    }

    if (_stop)
    {
        LOG_ERROR("node", "{}: Updating is stopped!", __FUNCTION__);
        return;
    }

    MessageBuffer buffer;

    NodeServerPktHeader header(packet->size() + sizeof(packet->GetOpcode()) + sizeof(GetAccountID()), packet->GetOpcode(), GetAccountID());
    if (buffer.GetRemainingSpace() < packet->size() + header.GetHeaderLength())
    {
        QueuePacket(std::move(buffer));
        //buffer.Resize(4096);
    }

    if (buffer.GetRemainingSpace() >= packet->size() + header.GetHeaderLength())
    {
        buffer.Write(header.header, header.GetHeaderLength());
        if (!packet->empty())
            buffer.Write(packet->contents(), packet->size());
    }
    else // single packet larger than 4096 bytes
    {
        MessageBuffer packetBuffer(packet->size() + header.GetHeaderLength());
        packetBuffer.Write(header.header, header.GetHeaderLength());
        if (!packet->empty())
            packetBuffer.Write(packet->contents(), packet->size());

        QueuePacket(std::move(packetBuffer));
    }

    if (buffer.GetActiveSize() > 0)
        QueuePacket(std::move(buffer));
}

void ClientSocket::AddPacketToQueue(WorldPacket const& packet)
{
    _bufferQueue.AddPacket(new WorldPacket(packet));
}

void ClientSocket::HandleAuthResponce(WorldPacket& packet)
{
    uint8 responceCode{ 0 };
    packet >> responceCode;

    if (responceCode == 0)
    {
        LOG_INFO("node", "> Correct auth from {}", _targetNodeName);
        _authed = true;
    }
}
