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

#ifndef _CLIENT_SOCKET_H_
#define _CLIENT_SOCKET_H_

#include "Socket.h"
#include "PacketQueue.h"
#include "WorldPacket.h"
#include <boost/asio/io_context.hpp>
#include <mutex>

namespace Warhead::Asio
{
    class DeadlineTimer;
}

class WorldSocket;

/// Manages all sockets connected to peers and network threads
class WH_GAME_API ClientSocket : public Socket<ClientSocket>
{
    using BaseSocket = Socket<ClientSocket>;

public:
    ClientSocket(tcp::socket&& socket, boost::asio::io_context& ioContext);

    void Start() override;
    void Stop();
    bool Update() override;

    inline bool IsStoped() { return _stop; }
    inline bool IsAuthed() { return _authed; }
    
    void AddPacketToQueue(WorldPacket const& packet);

    inline void SetNodeID(uint32 id) { _nodeID = id; }
    inline uint32 GetNodeID() { return _nodeID; }

    inline void SetAccountID(uint32 id) { _accountID = id; }
    inline uint32 GetAccountID() { return _accountID; }

    inline void SetAccountName(std::string_view accountName) { _accountName = std::string(accountName); }
    inline std::string const& GetAccountName() { return _accountName; }
    
    inline void SetWorldSocket(std::shared_ptr<WorldSocket> socket) { _worldSocket = socket; }
    inline std::shared_ptr<WorldSocket> GetWorldSocket() { return _worldSocket; }

protected:
    void OnClose() override;
    void ReadHandler() override;

private:
    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();
    bool ReadHeaderHandler();

    void HandleAuthResponce(WorldPacket& packet);
    void LogOpcode(OpcodeClient opcode);

    void SendPacket(WorldPacket const* packet);

    uint32 _nodeID{ 0 };
    std::mutex _nodeSessionLock;
    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;
    bool _authed{ false };

    std::string _thisNodeName;
    std::string _targetNodeName;
    std::string _accountName;
    uint32 _accountID;

    std::shared_ptr<WorldSocket> _worldSocket;

    PacketQueue<WorldPacket> _bufferQueue;

    std::unique_ptr<Warhead::Asio::DeadlineTimer> _updateTimer;
    std::atomic<bool> _stop{ false };
};

#endif
