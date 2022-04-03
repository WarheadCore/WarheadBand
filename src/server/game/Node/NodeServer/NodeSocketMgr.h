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

#ifndef _NODE_SOCKET_MANAGER_H_
#define _NODE_SOCKET_MANAGER_H_

#include "SocketMgr.h"

class NodeSocket;

 /// Manages all sockets connected to peers and network threads
class WH_GAME_API NodeSocketMgr : public SocketMgr<NodeSocket>
{
    typedef SocketMgr<NodeSocket> BaseSocketMgr;

public:
    static NodeSocketMgr& instance();

    /// Start network, listen at address:port.
    bool StartWorldNetwork(Warhead::Asio::IoContext& ioContext, std::string const& bindIp, uint16 port, int networkThreads);

    /// Stops all network threads, It will wait for all running threads.
    void StopNetwork() override;

    void OnSocketOpen(tcp::socket&& sock, uint32 threadIndex) override;

    std::size_t GetApplicationSendBufferSize() const { return _socketApplicationSendBufferSize; }

protected:
    NodeSocketMgr();

    NetworkThread<NodeSocket>* CreateThreads() const override;

    static void OnSocketAccept(tcp::socket&& sock, uint32 threadIndex)
    {
        instance().OnSocketOpen(std::forward<tcp::socket>(sock), threadIndex);
    }

private:
    int32 _socketSystemSendBufferSize;
    int32 _socketApplicationSendBufferSize;
    bool _tcpNoDelay;
};

#define sNodeSocketMgr NodeSocketMgr::instance()

#endif // _MUTE_MANAGER_H_
