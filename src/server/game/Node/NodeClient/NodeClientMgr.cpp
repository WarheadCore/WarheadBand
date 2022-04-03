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

#include "NodeClientMgr.h"
#include "NodeClient.h"
#include "IoContext.h"
#include "DeadlineTimer.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "NodeMgr.h"

/*static*/ NodeClientMgr* NodeClientMgr::instance()
{
    static NodeClientMgr instance;
    return &instance;
}

void NodeClientMgr::Disconnect(uint32 nodeID)
{
    auto const& itr = _nodeClients.find(nodeID);
    if (itr == _nodeClients.end())
        return;

    auto& nodeClient = itr->second;
    if (nodeClient)
    {
        if (nodeClient)
            nodeClient->CloseSocket();

        nodeClient.reset();
    }

    _nodeClients.erase(nodeID);
}

void NodeClientMgr::DisconnectAll()
{
    _stopped = true;

    _updateTimer->cancel();

    for (auto& [__, nodeClient] : _nodeClients)
    {
        if (nodeClient)
            nodeClient->CloseSocket();

        nodeClient.reset();
    }

    _nodeClients.clear();
}

void NodeClientMgr::Update()
{
    if (_stopped || _nodeClients.empty())
        return;

    for (auto& [nodeID, nodeClient] : _nodeClients)
    {
        if (!nodeClient)
            continue;

        if (!_bufferQueue.IsEmpty())
        {
            using PacketElement = PacketQueue<WorldPacket, uint32>::Element;

            WorldPacket* queuedPacket{ nullptr };
            uint32 packetNodeID{ 0 };
            std::vector<PacketElement*> queuePackets;

            while (_bufferQueue.GetNextPacket(queuedPacket, packetNodeID))
            {
                if (packetNodeID != nodeID)
                {
                    queuePackets.emplace_back(new PacketElement(queuedPacket, packetNodeID));
                    continue;
                }

                SendPacket(nodeID, *queuedPacket);
                delete queuedPacket;
            }

            _bufferQueue.ReadContainer(queuePackets);
        }

        if (!nodeClient->Update())
        {
            nodeClient->CloseSocket();
            nodeClient.reset();
            _nodeClients.erase(nodeID);

            uint32 _nodeID = nodeID;

            LOG_ERROR("node", "{}: Cannot update socket for node id {}. Start reconect.", __FUNCTION__, _nodeID);
            AsyncConnectToNodeSocket(_nodeID);
        }
    }

    _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _updateTimer->async_wait([this](boost::system::error_code const&) { Update(); });
}

void NodeClientMgr::Initialize(Warhead::Asio::IoContext& ioContext)
{
    ASSERT(sNodeMgr->GetThisNodeType() == NodeType::Realm);

    _updateTimer = std::make_unique<Warhead::Asio::DeadlineTimer>(ioContext);

    for (auto const& [nodeID, nodeInfo] : *sNodeMgr->GetAllNodesInfo())
    {
        // Skip connect to this node
        if (nodeInfo.Type == NodeType::Realm)
            continue;

        AsyncConnectToNodeSocket(nodeID);
    }
}

void NodeClientMgr::AddPacketToQueue(uint32 nodeID, WorldPacket const& packet)
{
    LOG_TRACE("node", "Realm->Node: {}", GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())));
    _bufferQueue.AddPacket(new WorldPacket(packet), nodeID);
}

void NodeClientMgr::SendPacket(uint32 nodeID, WorldPacket const& packet)
{
    auto const& itr = _nodeClients.find(nodeID);
    if (itr == _nodeClients.end())
    {
        LOG_ERROR("node", "{}: Not found node client for node id {}. Skip send packet.", __FUNCTION__, nodeID);
        return;
    }

    auto& nodeClient = itr->second;
    if (!nodeClient)
    {
        LOG_ERROR("node", "{}: Not found socket for node client with node id {}. Skip send packet.", __FUNCTION__, nodeID);
        return;
    }

    nodeClient->AddPacketToQueue(packet);
}

bool NodeClientMgr::AsyncConnectToNodeSocket(uint32 nodeID)
{
    auto nodeInfoTarget = sNodeMgr->GetNodeInfo(nodeID);
    if (!nodeInfoTarget)
        return false;

    auto const& itr = _nodeClients.find(nodeID);
    if (itr != _nodeClients.end())
    {
        LOG_ERROR("node", "{}: Existing socket for node id {}", __FUNCTION__, nodeID);
        return false;
    }

    _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
    _updateTimer->async_wait([this, nodeID](boost::system::error_code const&)
    {
        auto ConnectToNode = [this, nodeID]()
        {
            auto nodeInfoTarget = sNodeMgr->GetNodeInfo(nodeID);
            ASSERT(nodeInfoTarget);

            try
            {
                tcp::socket nodeSocket(_updateTimer->get_executor().context());
                nodeSocket.connect(tcp::endpoint(boost::asio::ip::make_address(nodeInfoTarget->IP), nodeInfoTarget->NodePort));

                auto socket = std::make_shared<NodeClient>(std::move(nodeSocket));
                socket->SetTargetNodeInfo(nodeInfoTarget);
                socket->Start();

                _nodeClients.emplace(nodeID, socket);

                _updateTimer->expires_from_now(boost::posix_time::milliseconds(1));
                _updateTimer->async_wait([this](boost::system::error_code const&) { Update(); });

                return true;
            }
            catch (boost::system::system_error const& err)
            {
                LOG_WARN("node", "Failed connet to node {} ({}). Start reconnect after 5 sec", nodeInfoTarget->ID, nodeInfoTarget->Name);
                LOG_DEBUG("node", "Error {}", err.what());
                return false;
            }
        };

        for (size_t i = 0; i < 5; i++)
        {
            if (ConnectToNode())
                break;

            std::this_thread::sleep_for(5s);
        }        
    });

    return true;
}
