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

#ifndef _NODE_SOCKET_H_
#define _NODE_SOCKET_H_

#include "Define.h"
#include "NodeCodes.h"
#include "Socket.h"
#include "WorldPacket.h"
#include "PacketQueue.h"
#include <boost/asio/ip/tcp.hpp>

enum class NodeConnectResponce : uint8
{
    Ok,
    Error
};

class WH_GAME_API NodeSocket : public Socket<NodeSocket>
{
    typedef Socket<NodeSocket> BaseSocket;

public:
    NodeSocket(tcp::socket&& socket);
    ~NodeSocket();

    NodeSocket(WorldSocket const& right) = delete;
    NodeSocket& operator=(NodeSocket const& right) = delete;

    void Start() override;
    bool Update() override;

    void SendPacket(WorldPacket const& packet);

    void SetSendBufferSize(std::size_t sendBufferSize) { _sendBufferSize = sendBufferSize; }

protected:
    void OnClose() override;
    void ReadHandler() override;
    bool ReadHeaderHandler();

    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();

private:
    /// writes network.nodeCode log
    /// accessing WorldSession is not threadsafe, only do it when holding _worldSessionLock
    void LogNodeCodeText(OpcodeNode nodeCode, std::unique_lock<std::mutex> const& guard) const;

    /// sends and logs network.nodeCode without accessing WorldSession
    void SendPacketAndLogNodeCode(WorldPacket const& packet);
    void HandleAuthNodeSession(WorldPacket& recvPacket);
    void SendNodeConnectResponce(NodeConnectResponce code);

    std::mutex _nodeSessionLock;
    std::shared_ptr<NodeSession> _nodeSession;
    bool _authed;
    std::unique_ptr<NodeInfo> _nodeInfo;

    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;
    PacketQueue<WorldPacket> _bufferQueue;
    std::size_t _sendBufferSize;

    QueryCallbackProcessor _queryProcessor;
};

#endif
