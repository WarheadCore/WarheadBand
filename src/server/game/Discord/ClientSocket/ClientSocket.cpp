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
#include "ClientSocketMgr.h"
#include "DeadlineTimer.h"
#include "DiscordClient.h"
#include "DiscordPacketHeader.h"
#include "Errors.h"
#include "GitRevision.h"
#include "IoContext.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "SmartEnum.h"
#include "Timer.h"

namespace
{
    inline std::string GetDiscordCodeNameForLogging(DiscordCode opcode)
    {
        if (opcode < NUM_DISCORD_CODE_HANDLERS)
            return Warhead::StringFormat("[{} 0x{:04X} ({})]",
                EnumUtils::ToTitle(opcode), opcode, opcode);

        return Warhead::StringFormat("[INVALID OPCODE 0x{:04X} ({})]", opcode, opcode);
    }
}

ClientSocket::ClientSocket(tcp::socket&& socket) :
    Socket(std::move(socket))
{
    _headerBuffer.Resize(sizeof(DiscordClientPktHeader));
    SetNoDelay(true);
}

void ClientSocket::Start()
{
    LOG_DEBUG("module.discord", "Start process auth from server. Account name '{}'", sDiscord->GetAccountName());
    SendAuthSession();
}

void ClientSocket::OnClose()
{
    LOG_DEBUG("module.discord", "> Disconnected from server");
}

bool ClientSocket::Update()
{
    if (_authed)
    {
        DiscordPacket* queuedPacket{ nullptr };

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
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(DiscordClientPktHeader));

    DiscordClientPktHeader* header = reinterpret_cast<DiscordClientPktHeader*>(_headerBuffer.GetReadPointer());
    EndianConvertReverse(header->size);
    EndianConvert(header->cmd);

    if (!header->IsValidSize() || !header->IsValidOpcode())
    {
        LOG_ERROR("module.discord", "ClientSocket::ReadHeaderHandler(): node sent malformed packet (size: {}, cmd: {})", header->size, header->cmd);
        return false;
    }

    header->size -= sizeof(header->cmd);
    _packetBuffer.Resize(header->size);
    return true;
}

ClientSocket::ReadDataHandlerResult ClientSocket::ReadDataHandler()
{
    DiscordClientPktHeader* header = reinterpret_cast<DiscordClientPktHeader*>(_headerBuffer.GetReadPointer());
    DiscordCode opcode = static_cast<DiscordCode>(header->cmd);

    DiscordPacket packet(opcode, std::move(_packetBuffer));

    std::unique_lock<std::mutex> sessionGuard(_sessionLock, std::defer_lock);

    LogOpcode(opcode);

    switch (opcode)
    {
        case SERVER_SEND_AUTH_RESPONSE:
        {
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("module.discord", "{}: received duplicate SERVER_SEND_AUTH_RESPONSE", __FUNCTION__);

                sClientSocketMgr->Disconnect();
                return ReadDataHandlerResult::Error;
            }

            HandleAuthResponce(packet);
            return ReadDataHandlerResult::Ok;
        }
        case SERVER_SEND_PONG:
        {
            if (!_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("module.discord", "{}: received SERVER_SEND_PONG without auth", __FUNCTION__);

                sClientSocketMgr->Disconnect();
                return ReadDataHandlerResult::Error;
            }

            HandlePong(packet);
            return ReadDataHandlerResult::Ok;
        }
        default:
            return ReadDataHandlerResult::Error;
    }
}

void ClientSocket::LogOpcode(DiscordCode opcode)
{
    LOG_TRACE("network.opcode", "C->S: {}", ::GetDiscordCodeNameForLogging(opcode));
}

void ClientSocket::SendPacket(DiscordPacket const* packet)
{
    if (!IsOpen())
    {
        LOG_ERROR("module.discord", "{}: Not open socket!", __FUNCTION__);
        return;
    }

    MessageBuffer buffer;

    DiscordServerPktHeader header(packet->size() + sizeof(packet->GetOpcode()), packet->GetOpcode());
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

void ClientSocket::AddPacketToQueue(DiscordPacket const& packet)
{
    _bufferQueue.AddPacket(new DiscordPacket(packet));
}

void ClientSocket::HandleAuthResponce(DiscordPacket& packet)
{
    using namespace std::chrono;

    Microseconds diff = duration_cast<Microseconds>(steady_clock::now() - _startTime);

    uint8 code;
    packet >> code;

    DiscordAuthResponseCodes responseCode = static_cast<DiscordAuthResponseCodes>(code);
    auto const& codeString = EnumUtils::ToTitle(responseCode);

    if (responseCode != DiscordAuthResponseCodes::Ok)
    {
        LOG_INFO("module.discord", "Auth incorrect. Code {}", codeString);
        sClientSocketMgr->Disconnect();
        return;
    }

    LOG_INFO("module.discord", "[{}] Auth correct '{}'", Warhead::Time::ToTimeString(diff), codeString);
    _authed = true;

    SendPingMessage();
}

void ClientSocket::SendPingMessage()
{
    using namespace std::chrono;

    Microseconds timeNow = duration_cast<Microseconds>(steady_clock::now().time_since_epoch());

    DiscordPacket packetPing(CLIENT_SEND_PING, 1);
    packetPing << int64(timeNow.count());
    packetPing << int64(_latency.count());
    SendPacket(&packetPing);
}

void ClientSocket::HandlePong(DiscordPacket& packet)
{
    int64 timePacket;
    packet >> timePacket;

    using namespace std::chrono;

    Microseconds timeNow = duration_cast<Microseconds>(steady_clock::now().time_since_epoch());
    _latency = duration_cast<Microseconds>(timeNow - Microseconds(timePacket));

    LOG_DEBUG("module.discord", "> Latency {}", Warhead::Time::ToTimeString(_latency));
}

void ClientSocket::SendAuthSession()
{
    DiscordPacket packet(CLIENT_AUTH_SESSION, 1);
    packet << sDiscord->GetAccountName();
    packet << sDiscord->GetAccountKey();
    packet << GitRevision::GetCompanyNameStr();
    packet << GitRevision::GetFileVersionStr();
    packet << uint32(WARHEAD_DISCORD_VERSION);
    packet << int64(sDiscord->GetServerID());
    SendPacket(&packet);

    _startTime = std::chrono::steady_clock::now();

    AsyncRead();
}
