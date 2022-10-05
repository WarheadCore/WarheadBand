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

#ifndef _NODE_CODES_H_
#define _NODE_CODES_H_

#include "Opcodes.h"

class NodeSession;
class WorldPacket;

class NodeCodeHandler
{
public:
    NodeCodeHandler(std::string_view name) :
        Name(std::string(name)) { }

    virtual ~NodeCodeHandler() = default;

    std::string Name;
};

class ClientNodeCodeHandler : public NodeCodeHandler
{
public:
    ClientNodeCodeHandler(std::string_view name)
        : NodeCodeHandler(name) { }

    virtual void Call(NodeSession* session, WorldPacket& packet) const = 0;
};

class ServerNodeCodeHandler : public NodeCodeHandler
{
public:
    ServerNodeCodeHandler(std::string_view name)
        : NodeCodeHandler(name) { }
};

class NodeCodeTable
{
public:
    NodeCodeTable();

    NodeCodeTable(NodeCodeTable const&) = delete;
    NodeCodeTable& operator=(NodeCodeTable const&) = delete;

    ~NodeCodeTable();

    void Initialize();

    ClientNodeCodeHandler const* operator[](Opcodes index) const
    {
        return _internalTableClient[index];
    }

private:
    template<typename Handler, Handler HandlerFunction>
    void ValidateAndSetNodeCode(OpcodeNode opcode, std::string_view name);

    std::array<ClientNodeCodeHandler*, NUM_NODE_CODE_HANDLERS> _internalTableClient;
};

extern NodeCodeTable nodeCodeTable;

#endif
