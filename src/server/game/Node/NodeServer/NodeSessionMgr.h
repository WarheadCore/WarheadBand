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

#ifndef _NODE_SESSION_MANAGER_H_
#define _NODE_SESSION_MANAGER_H_

#include "Define.h"
#include <unordered_map>

class NodeSession;

 /// Manages all sockets connected to peers and network threads
class WH_GAME_API NodeSessionMgr
{
public:
    static NodeSessionMgr* instance();

    void Update();
    void Unload();

    void AddSession(std::shared_ptr<NodeSession> session);
    void DeleteSession();
    std::shared_ptr<NodeSession> GetSession();

private:
    std::shared_ptr<NodeSession> _nodeSession;
};

#define sNodeSessionMgr NodeSessionMgr::instance()

#endif
