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

#ifndef _NODE_MANAGER_H_
#define _NODE_MANAGER_H_

#include "Define.h"
#include <unordered_map>

enum class NodeType : uint8
{
    Realm,
    Master,
    PvP,

    Max
};

constexpr uint8 NODE_TYPE_MAX = static_cast<uint8>(NodeType::Max);

struct NodeInfo
{
    explicit NodeInfo(uint32 id, NodeType type, std::string_view ip, uint16 worldPort, uint16 nodePort, std::string_view name) :
        ID(id),
        Type(type),
        IP(std::string(ip)),
        WorldPort(worldPort),
        NodePort(nodePort),
        Name(std::string(name)) { }

    explicit NodeInfo(NodeInfo const* nodeInfo) :
        ID(nodeInfo->ID),
        Type(nodeInfo->Type),
        IP(nodeInfo->IP),
        WorldPort(nodeInfo->WorldPort),
        NodePort(nodeInfo->NodePort),
        Name(nodeInfo->Name) { }

    std::string GetInfo();

    uint32 ID{ 0 };
    NodeType Type{ NodeType::Master };
    std::string IP;
    uint16 WorldPort{ 0 };
    uint16 NodePort{ 0 };
    std::string Name;
};

class WH_GAME_API NodeMgr
{
public:
    static NodeMgr* instance();

    void Initialize();

    NodeInfo const* GetNodeInfo(uint32 id);
    NodeInfo const* GetNodeInfo(NodeType type);

    // This node
    NodeInfo const* GetThisNodeInfo();
    NodeType GetThisNodeType();

    inline auto const GetAllNodesInfo() { return &_listNodes; }
    void AddNodeInfo(uint32 id, NodeInfo info);
    void AddNodeInfo(uint32 id, NodeType type, std::string_view ip, uint16 worldPort, uint16 nodePort, std::string_view name);

private:
    std::unordered_map<uint32, NodeInfo> _listNodes;
    std::unique_ptr<NodeInfo> _thisNodeInfo{ nullptr };
};

#define sNodeMgr NodeMgr::instance()

#endif
