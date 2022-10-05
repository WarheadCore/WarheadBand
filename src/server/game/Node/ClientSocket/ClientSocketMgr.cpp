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

#include "ClientSocketMgr.h"
#include "ClientSocket.h"
#include "IoContext.h"
#include "DeadlineTimer.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "NodeMgr.h"
#include "World.h"
#include "GameTime.h"
#include "Chat.h"

constexpr Seconds MIN_TIME_FOR_CHANGE_NODE = 3s;

/*static*/ ClientSocketMgr* ClientSocketMgr::instance()
{
    static ClientSocketMgr instance;
    return &instance;
}

void ClientSocketMgr::DisconnectNode(uint32 accountID)
{
    auto const& itr = _nodeWorldSockets.find(accountID);
    if (itr == _nodeWorldSockets.end())
        return;

    auto& socket = itr->second;
    if (socket)
    {
        if (socket->IsOpen())
            socket->CloseSocket();

        socket = nullptr;
    }

    _nodeWorldSockets.erase(accountID);
}

void ClientSocketMgr::DisconnectAccount(uint32 accountID)
{
    auto const& itr = _nodeWorldSockets.find(accountID);
    if (itr == _nodeWorldSockets.end())
        return;

    auto& socket = itr->second;
    if (socket)
    {
        /*if (socket->IsOpen())
            socket->DelayedCloseSocket();*/

        if (socket->IsOpen())
            socket->Stop();

        socket = nullptr;
    }

    _nodeWorldSockets.erase(accountID);
}

void ClientSocketMgr::DisconnectAll()
{
    _stopped = true;

    _updateTimer->cancel();
    _newSocketsTimer->cancel();

    for (auto& [__, socket] : _nodeWorldSockets)
    {
        if (socket)
        {
            if (socket->IsOpen())
                socket->Stop();

            socket = nullptr;
        }
    }

    _nodeWorldSockets.clear();
}

void ClientSocketMgr::UpdateAll()
{
    if (_stopped)
        return;

    _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _updateTimer->async_wait([this](boost::system::error_code const&) { UpdateAll(); });

    AddNewSockets();

    if (_nodeWorldSockets.empty())
        return;

    for (auto& [accountID, socket] : _nodeWorldSockets)
    {
        if (!socket)
        {
            LOG_WARN("node", "!socket");
            //_nodeWorldSockets.erase(accountID);
            continue;
        }

        if (socket->IsStoped())
        {
            socket->Stop();
            socket.reset();
            _nodeWorldSockets.erase(accountID);
        }

        if (_bufferQueue.IsEmpty())
            continue;

        using PacketElement = PacketQueue<WorldPacket, uint32>::Element;

        WorldPacket* queuedPacket{ nullptr };
        uint32 packetAccountID{ 0 };
        std::vector<PacketElement*> queuePackets;

        while (_bufferQueue.GetNextPacket(queuedPacket, packetAccountID))
        {
            if (packetAccountID != accountID)
            {
                queuePackets.emplace_back(new PacketElement(queuedPacket, packetAccountID));
                continue;
            }

            SendPacket(accountID, *queuedPacket);
            delete queuedPacket;
        }

        _bufferQueue.ReadContainer(queuePackets);
    }
}

void ClientSocketMgr::Initialize(Warhead::Asio::IoContext& ioContext)
{
    ASSERT(sNodeMgr->GetThisNodeType() == NodeType::Realm);

    _updateTimer = std::make_unique<Warhead::Asio::DeadlineTimer>(ioContext);
    _newSocketsTimer = std::make_unique<Warhead::Asio::DeadlineTimer>(ioContext);
}

void ClientSocketMgr::AddPacketToQueue(uint32 accountID, WorldPacket const& packet)
{
    //LOG_TRACE("node", "Player({})->Node: {}", accountID, GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())));
    _bufferQueue.AddPacket(new WorldPacket(packet), accountID);
}

void ClientSocketMgr::SendPacket(uint32 accountID, WorldPacket const& packet)
{
    auto const& itr = _nodeWorldSockets.find(accountID);
    if (itr == _nodeWorldSockets.end())
    {
        LOG_ERROR("node", "{}: Not found socket for account id {}. Skip send packet.", __FUNCTION__, accountID);
        return;
    }

    auto& socket = itr->second;
    if (socket)
    {
        //LOG_TRACE("node", "Player({})->Node: {}", accountID, GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())));
        socket->AddPacketToQueue(packet);
    }
}

void ClientSocketMgr::ConnectToWorldSocket(uint32 nodeID, uint32 accountID, std::function<void(bool)>&& execute)
{
    _newSocketsTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _newSocketsTimer->async_wait([this, nodeID, accountID, execute = std::move(execute)](boost::system::error_code const&)
    {
        std::lock_guard<std::mutex> lock(_newSocketsLock);

        auto nodeInfoTarget = sNodeMgr->GetNodeInfo(nodeID);
        if (!nodeInfoTarget)
        {
            execute(false);
            return;
        }

        auto worldSession = sWorld->FindSession(accountID);
        if (!worldSession)
        {
            LOG_ERROR("node", "> Not found world session for account id {}", accountID);
            execute(false);
            return;
        }

        auto connectedTime = GetConnectedTime(accountID); // 1000
        auto normalTime = connectedTime + MIN_TIME_FOR_CHANGE_NODE; // 1003
        if (connectedTime != 0s && normalTime > GameTime::GetGameTime()) // 1003 > 1001
        {
            auto waitTime = normalTime - GameTime::GetGameTime(); // 1000 + 3 - 1001
            LOG_WARN("node", "> Wait {} sec for next change node", waitTime.count());
            execute(false);
            return;
        }

        DisconnectAccount(accountID);

        try
        {
            tcp::socket nodeSocket(_updateTimer->get_executor().context());
            nodeSocket.connect(tcp::endpoint(boost::asio::ip::make_address(nodeInfoTarget->IP), nodeInfoTarget->WorldPort));

            auto socket = std::make_shared<ClientSocket>(std::move(nodeSocket), Warhead::Asio::get_io_context(*_updateTimer));
            socket->SetNodeID(nodeInfoTarget->ID);
            socket->SetAccountID(worldSession->GetAccountId());
            socket->SetAccountName(worldSession->GetAccountName());
            socket->SetWorldSocket(worldSession->GetWorldSocket());
            socket->Start();

            _newNodeWorldSockets.emplace(accountID, socket);

            _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
            _updateTimer->async_wait([this](boost::system::error_code const&) { UpdateAll(); });

            execute(true);

            SetConnectedTime(accountID, GameTime::GetGameTime());
        }
        catch (boost::system::system_error const& err)
        {
            LOG_WARN("node", "Failed connet. Error {}", err.what());
            execute(false);
            return;
        }
    });
}

void ClientSocketMgr::ConnectToOtherWorldSocket(uint32 oldNodeID, uint32 newNodeID, uint32 accountID, ObjectGuid playerGuid)
{
    if (oldNodeID == newNodeID)
    {
        LOG_ERROR("node", "> Try connect to old node. All nodes same. {}/{}", oldNodeID, newNodeID);
        return;
    }

    auto const& nodeInfoOld = sNodeMgr->GetNodeInfo(oldNodeID);
    auto const& nodeInfoNew = sNodeMgr->GetNodeInfo(newNodeID);
    if (!nodeInfoOld)
    {
        LOG_ERROR("node", "> Not found data for old node id {}", oldNodeID);
        return;
    }

    if (!nodeInfoNew)
    {
        LOG_ERROR("node", "> Not found data for new node id {}", newNodeID);
        return;
    }

    auto const& itr = _nodeWorldSockets.find(accountID);
    if (itr == _nodeWorldSockets.end())
    {
        LOG_ERROR("node", "> Not found socket for account {}", accountID);
        return;
    }

    auto& socket = itr->second;
    if (socket && socket->GetNodeID() == newNodeID)
    {
        LOG_ERROR("node", "> Try connect to old node. All nodes same. {}/{}", oldNodeID, newNodeID);
        return;
    }

    auto worldSession = sWorld->FindSession(accountID);
    if (!worldSession)
    {
        LOG_ERROR("node", "> Not found world session for account id {}", accountID);
        return;
    }

    worldSession->ConnectToNode(newNodeID, [this, worldSession, newNodeID, accountID, playerGuid](bool isComplete)
    {
        if (!isComplete)
        {
            LOG_ERROR("node", "> Can't connect to node {}. Account id {}", newNodeID, accountID);
            return;
        }

        worldSession->LoginFromNode(newNodeID, playerGuid);
        worldSession->ChangeNode(newNodeID);
    });
}

Seconds ClientSocketMgr::GetConnectedTime(uint32 accountID)
{
    auto connectedtime = Warhead::Containers::MapGetValuePtr(_accountConnectTime, accountID);
    if (!connectedtime)
        return 0s;

    return *connectedtime;
}

void ClientSocketMgr::SetConnectedTime(uint32 accountID, Seconds connectedTime)
{
    auto const& itr = _accountConnectTime.find(accountID);
    if (itr != _accountConnectTime.end())
    {
        itr->second = connectedTime;
        return;
    }

    _accountConnectTime.emplace(accountID, connectedTime);
}

void ClientSocketMgr::AddNewSockets()
{
    std::lock_guard<std::mutex> lock(_newSocketsLock);

    if (_newNodeWorldSockets.empty())
        return;

    for (auto& [nodeId, socket] : _newNodeWorldSockets)
    {
        if (socket->IsOpen())
            _nodeWorldSockets.emplace(nodeId, socket);
    }

    _newNodeWorldSockets.clear();
}
