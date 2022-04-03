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

#include "NodeMgr.h"
#include "Containers.h"
#include "GameConfig.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StopWatch.h"

std::string NodeInfo::GetInfo()
{
    return Warhead::StringFormat("[ID {}, Name '{}']", ID, Name);
}

NodeMgr* NodeMgr::instance()
{
    static NodeMgr instance;
    return &instance;
}

void NodeMgr::Initialize()
{
    StopWatch sw;

    _listNodes.clear();

    auto result = NodeDatabase.Query(NodeDatabase.GetPreparedStatement(NODE_SEL_NODE_INFO));
    if (!result)
    {
        ABORT("> No data about nodes");
        return;
    }

    do
    {
        auto const& [id, type, ip, worldPort, nodePort, name]
            = result->FetchTuple<uint32, uint8, std::string, uint16, uint16, std::string>();

        ASSERT(type < NODE_TYPE_MAX);

        AddNodeInfo(id, NodeInfo(id, static_cast<NodeType>(type), ip, worldPort, nodePort, name));
    }
    while (result->NextRow());

    LOG_INFO("server.loading", "> Loaded {} nodes in {}", _listNodes.size(), sw);
    LOG_INFO("server.loading", "");

    auto nodeInfo = Warhead::Containers::MapGetValuePtr(_listNodes, CONF_GET_UINT("Node.ID"));
    ASSERT(nodeInfo, "Not found data for this node!");

    _thisNodeInfo = std::make_unique<NodeInfo>(nodeInfo);
}

void NodeMgr::AddNodeInfo(uint32 id, NodeInfo info)
{
    if (GetNodeInfo(id))
    {
        LOG_ERROR("node", "> Node with id {} exist", id);
        return;
    }

    _listNodes.emplace(id, std::move(info));
}

void NodeMgr::AddNodeInfo(uint32 id, NodeType type, std::string_view ip, uint16 worldPort, uint16 nodePort, std::string_view name)
{
    NodeInfo info = NodeInfo(id, type, ip, worldPort, nodePort, name);
    AddNodeInfo(id, info);
}

NodeInfo const* NodeMgr::GetNodeInfo(uint32 id)
{
    return Warhead::Containers::MapGetValuePtr(_listNodes, id);
}

NodeInfo const* NodeMgr::GetNodeInfo(NodeType type)
{
    for (auto const& [id, nodeInfo] : _listNodes)
    {
        if (nodeInfo.Type == type)
            return &nodeInfo;
    }

    ABORT("Not found node data for type {}", static_cast<uint8>(type));

    return nullptr;
}

NodeInfo const* NodeMgr::GetThisNodeInfo()
{
    ASSERT(_thisNodeInfo);
    return _thisNodeInfo.get();
}

NodeType NodeMgr::GetThisNodeType()
{
    ASSERT(_thisNodeInfo);
    return _thisNodeInfo->Type;
}
