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

#ifndef __RASESSION_H__
#define __RASESSION_H__

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <future>
#include <memory>

using boost::asio::ip::tcp;

class RASession : public std::enable_shared_from_this<RASession>
{
public:
    explicit RASession(tcp::socket&& socket) :
        _socket(std::move(socket)) { }

    void Start();

    std::string GetRemoteIpAddress() const { return _socket.remote_endpoint().address().to_string(); }
    unsigned short GetRemotePort() const { return _socket.remote_endpoint().port(); }

private:
    int Send(std::string_view data);
    std::string ReadString();
    bool CheckAccessLevel(const std::string& user);
    bool CheckPassword(const std::string& user, const std::string& pass);
    bool ProcessCommand(std::string& command);

    static void CommandPrint(void* callbackArg, std::string_view text);
    static void CommandFinished(void* callbackArg, bool);

    tcp::socket _socket;
    boost::asio::streambuf _readBuffer;
    boost::asio::streambuf _writeBuffer;
    std::promise<void>* _commandExecuting{ nullptr };
};

#endif
