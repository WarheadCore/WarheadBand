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

#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "NodeMgr.h"
#include "WorldSession.h"
#include "NodeSession.h"
#include "ClientSocketMgr.h"
#include "NodeSessionMgr.h"

using namespace Warhead::ChatCommands;

class node_commandscript : public CommandScript
{
public:
    node_commandscript() : CommandScript("node_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable arenaCommandTable =
        {
            { "connect",        HandleNodeConnectCommand,   SEC_ADMINISTRATOR, Console::No },
            { "login",          HandleNodeLoginCommand,     SEC_ADMINISTRATOR, Console::No },
            { "change",         HandleNodeChangeCommand,    SEC_ADMINISTRATOR, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "node", arenaCommandTable }
        };

        return commandTable;
    }

    static bool HandleNodeConnectCommand(ChatHandler* handler, uint32 id)
    {
        //handler->GetSession()->ConnectToNode(id);
        return true;
    }

    static bool HandleNodeChangeCommand(ChatHandler* handler, uint32 newNodeId)
    {
        if (sNodeMgr->GetThisNodeType() == NodeType::Realm)
            return false;

        auto const& oldNodeInfo = sNodeMgr->GetThisNodeInfo();
        auto const& newNodeInfo = sNodeMgr->GetNodeInfo(newNodeId);

        if (!newNodeInfo)
        {
            LOG_ERROR("node", "> Incorrect new node info. ID {}", newNodeId);
            return false;
        }

        LOG_INFO("node", "> Start connect from {} to {}", oldNodeInfo->Name, newNodeInfo->Name);

        auto session = sNodeSessionMgr->GetSession();
        if (!session)
            return false;

        session->ChangeNodeSession(oldNodeInfo->ID, newNodeInfo->ID, handler->GetSession()->GetAccountId(), handler->GetPlayer()->GetGUID());

        return true;
    }

    static bool HandleNodeLoginCommand(ChatHandler* handler, uint32 id)
    {
        /*handler->GetSession()->ChangeNode(NodeType::Realm, false);
        handler->GetSession()->ConnectToNode(id);
        handler->GetSession()->LoginFromNode(id, handler->GetPlayer()->GetGUID());
        handler->GetSession()->ChangeNode(id);*/
        return true;
    }
};

void AddSC_node_commandscript()
{
    new node_commandscript();
}
