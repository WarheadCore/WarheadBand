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

#ifndef _CLIENT_SOCKET_MGR_H_
#define _CLIENT_SOCKET_MGR_H_

#include "Socket.h"
#include "WorldPacket.h"
#include "ObjectGuid.h"
#include "Duration.h"
#include "PacketQueue.h"
#include <unordered_map>
#include <functional>
#include <mutex>

namespace Warhead::Asio
{
    class IoContext;
    class DeadlineTimer;
}

class ClientSocket;
class WorldSocket;

class WH_GAME_API ClientSocketMgr
{
public:
    static ClientSocketMgr* instance();

    void Initialize(Warhead::Asio::IoContext& ioContext);

    void ConnectToWorldSocket(uint32 nodeID, uint32 accountID, std::function<void(bool)>&& execute);
    void ConnectToOtherWorldSocket(uint32 oldNodeID, uint32 newNodeID, uint32 accountID, ObjectGuid playerGuid);

    void DisconnectNode(uint32 accountID);
    void DisconnectAccount(uint32 accountID);
    void DisconnectAll();
    void UpdateAll();

    void AddPacketToQueue(uint32 accountID, WorldPacket const& packet);

private:
    Seconds GetConnectedTime(uint32 accountID);
    void SetConnectedTime(uint32 accountID, Seconds connectedTime);
    void AddNewSockets();
    void SendPacket(uint32 accountID, WorldPacket const& packet);

    std::mutex _newSocketsLock;
    std::atomic<bool> _stopped{ false };
    std::unique_ptr<Warhead::Asio::DeadlineTimer> _updateTimer{ nullptr };
    std::unique_ptr<Warhead::Asio::DeadlineTimer> _newSocketsTimer{ nullptr };
    std::unordered_map<uint32/*accID*/, std::shared_ptr<ClientSocket>> _nodeWorldSockets;
    std::unordered_map<uint32/*accID*/, std::shared_ptr<ClientSocket>> _newNodeWorldSockets;
    std::unordered_map<uint32/*accID*/, Seconds> _accountConnectTime;

    PacketQueue<WorldPacket, uint32> _bufferQueue;
};

#define sClientSocketMgr ClientSocketMgr::instance()

#endif
