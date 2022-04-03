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

#include "NodeCodes.h"
#include "Log.h"
#include "NodeSession.h"
#include <iomanip>
#include <sstream>

template<class PacketClass, void(NodeSession::* HandlerFunction)(PacketClass&)>
class PacketHandler : public ClientNodeCodeHandler
{
public:
    PacketHandler(std::string_view name) :
        ClientNodeCodeHandler(name) { }

    void Call(NodeSession* session, WorldPacket& packet) const override
    {
        PacketClass nicePacket(std::move(packet));
        nicePacket.Read();
        (session->*HandlerFunction)(nicePacket);
    }
};

template<void(NodeSession::* HandlerFunction)(WorldPacket&)>
class PacketHandler<WorldPacket, HandlerFunction> : public ClientNodeCodeHandler
{
public:
    PacketHandler(std::string_view name) :
        ClientNodeCodeHandler(name) { }

    void Call(NodeSession* session, WorldPacket& packet) const override
    {
        (session->*HandlerFunction)(packet);
    }
};

NodeCodeTable nodeCodeTable;

template<typename T>
struct get_packet_class
{
};

template<typename PacketClass>
struct get_packet_class<void(NodeSession::*)(PacketClass&)>
{
    using type = PacketClass;
};

NodeCodeTable::NodeCodeTable()
{
    _internalTableClient.fill(0);
}

NodeCodeTable::~NodeCodeTable()
{
    for (uint16 i = 0; i < NUM_NODE_CODE_HANDLERS; ++i)
        delete _internalTableClient[i];
}

template<typename Handler, Handler HandlerFunction>
void NodeCodeTable::ValidateAndSetNodeCode(OpcodeNode nodeCode, std::string_view name)
{
    if (uint32(nodeCode) == NULL_OPCODE)
    {
        LOG_ERROR("network", "NodeCode {} does not have a value", name);
        return;
    }

    if (uint32(nodeCode) >= NUM_NODE_CODE_HANDLERS)
    {
        LOG_ERROR("network", "Tried to set handler for an invalid nodeCode {}", uint32(nodeCode));
        return;
    }

    if (_internalTableClient[nodeCode] != nullptr)
    {
        LOG_ERROR("network", "Tried to override client handler of {} with {} (nodeCode {})", nodeCodeTable[nodeCode]->Name, name, uint32(nodeCode));
        return;
    }

    _internalTableClient[nodeCode] = new PacketHandler<typename get_packet_class<Handler>::type, HandlerFunction>(name);
}

/// Correspondence between nodeCodes and their names
void NodeCodeTable::Initialize()
{
#define DEFINE_HANDLER(nodeCode, handler) \
    ValidateAndSetNodeCode<decltype(handler), handler>(nodeCode, #nodeCode)

    DEFINE_HANDLER(NODE_CONNECT_NEW_NODE,       &NodeSession::Handle_ServerSide);
    DEFINE_HANDLER(NODE_CONNECT_RESPONCE,       &NodeSession::Handle_ServerSide);
    DEFINE_HANDLER(NODE_SEND_TEST_PACKET,       &NodeSession::HandleTest);
    DEFINE_HANDLER(NODE_ADD_ACCOUNT,            &NodeSession::Handle_ServerSide);
    DEFINE_HANDLER(NODE_CHANGE_NODE,            &NodeSession::Handle_ServerSide);
    DEFINE_HANDLER(NODE_CHANGE_NODE_SESSION,    &NodeSession::Handle_ServerSide);

#undef DEFINE_HANDLER
}
