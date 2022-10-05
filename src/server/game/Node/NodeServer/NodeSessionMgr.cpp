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

#include "NodeSessionMgr.h"
#include "Containers.h"
#include "NodeSession.h"
#include "NodeMgr.h"

NodeSessionMgr* NodeSessionMgr::instance()
{
    static NodeSessionMgr instance;
    return &instance;
}

void NodeSessionMgr::AddSession(std::shared_ptr<NodeSession> session)
{
    _nodeSession = session;
}

std::shared_ptr<NodeSession> NodeSessionMgr::GetSession()
{
    return _nodeSession;
}

void NodeSessionMgr::DeleteSession()
{
    if (!_nodeSession)
        return;

    _nodeSession.reset();
}

void NodeSessionMgr::Update()
{
    if (_nodeSession)
        _nodeSession->Update();
}

void NodeSessionMgr::Unload()
{
    DeleteSession();
}
