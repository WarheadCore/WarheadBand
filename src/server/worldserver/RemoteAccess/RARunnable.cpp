/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup Trinityd
 */

#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "RARunnable.h"
#include "RASocket.h"
#include "World.h"
#include <ace/Acceptor.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Reactor_Impl.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/TP_Reactor.h>

RARunnable::RARunnable()
{
    ACE_Reactor_Impl* imp;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)
    imp = new ACE_Dev_Poll_Reactor();
    imp->max_notify_iterations (128);
    imp->restart (1);
#else
    imp = new ACE_TP_Reactor();
    imp->max_notify_iterations (128);
#endif

    m_Reactor = new ACE_Reactor (imp, 1);
}

RARunnable::~RARunnable()
{
    delete m_Reactor;
}

void RARunnable::run()
{
    if (!sConfigMgr->GetOption<bool>("Ra.Enable", false))
        return;

    ACE_Acceptor<RASocket, ACE_SOCK_ACCEPTOR> acceptor;

    uint16 raPort = uint16(sConfigMgr->GetOption<int32>("Ra.Port", 3443));
    std::string stringIp = sConfigMgr->GetOption<std::string>("Ra.IP", "0.0.0.0");
    ACE_INET_Addr listenAddress(raPort, stringIp.c_str());

    if (acceptor.open(listenAddress, m_Reactor) == -1)
    {
        LOG_ERROR("server", "Trinity RA can not bind to port %d on %s (possible error: port already in use)", raPort, stringIp.c_str());
        return;
    }

    LOG_INFO("server", "Starting Trinity RA on port %d on %s", raPort, stringIp.c_str());

    while (!World::IsStopped())
    {
        ACE_Time_Value interval(0, 100000);
        if (m_Reactor->run_reactor_event_loop(interval) == -1)
            break;
    }

#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    LOG_DEBUG("server", "Trinity RA thread exiting");
#endif
}
