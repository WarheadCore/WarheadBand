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

#ifndef _NODE_CLIENT_MGR_H_
#define _NODE_CLIENT_MGR_H_

#include "Socket.h"
#include "PacketQueue.h"
#include "WorldPacket.h"
#include <unordered_map>
#include <mutex>

namespace Warhead::Asio
{
    class IoContext;
    class DeadlineTimer;
}

class NodeClient;
class WorldSocket;

class WH_GAME_API NodeClientMgr
{
public:
    static NodeClientMgr* instance();

    void Initialize(Warhead::Asio::IoContext& ioContext);

    bool AsyncConnectToNodeSocket(uint32 nodeID);

    void Disconnect(uint32 nodeID);
    void DisconnectAll();
    void Update();

    void AddPacketToQueue(uint32 nodeID, WorldPacket const& packet);

private:
    void SendPacket(uint32 nodeID, WorldPacket const& packet);

    std::mutex _mutex;
    std::atomic<bool> _stopped{ false };
    std::unique_ptr<Warhead::Asio::DeadlineTimer> _updateTimer{ nullptr };
    std::unordered_map<uint32/*nodeID*/, std::shared_ptr<NodeClient>> _nodeClients;

    PacketQueue<WorldPacket, uint32> _bufferQueue;
};

#define sNodeClientMgr NodeClientMgr::instance()

#endif
