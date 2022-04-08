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

#include "AsioHacksFwd.h"
#include "DiscordPacket.h"
#include "PacketQueue.h"
#include <mutex>

namespace Warhead::Asio
{
    class IoContext;
    class DeadlineTimer;
}

class ClientSocket;
class DiscordPacket;

class ClientSocketMgr
{
public:
    static ClientSocketMgr* instance();

    void InitializeIo(Warhead::Asio::IoContext& ioContext);
    void Initialize();
    void ConnectToServer(uint32 reconnectCount = 1);
    void Disconnect();
    void Update();
    void AddPacketToQueue(DiscordPacket const& packet);
    void SendPingMessage();

private:
    void SendPacket(DiscordPacket const& packet);

    std::mutex _newConnectLock;
    std::atomic<bool> _stopped{ false };
    std::unique_ptr<Warhead::Asio::DeadlineTimer> _updateTimer{ nullptr };
    std::shared_ptr<ClientSocket> _clientSocket;
    std::unique_ptr<boost::asio::ip::address> _address;

    PacketQueue<DiscordPacket> _bufferQueue;
};

#define sClientSocketMgr ClientSocketMgr::instance()

#endif
