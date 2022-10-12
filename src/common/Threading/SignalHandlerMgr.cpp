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

#include "SignalHandlerMgr.h"
#include "Errors.h"
#include "IoContextMgr.h"
#include "IoContext.h"
#include "Log.h"
#include <csignal>

static void SignalHandler(boost::system::error_code const& error, int signalNumber)
{
    if (!error)
    {
        if (auto ioContext = sIoContextMgr->GetIoContextPtr())
            ioContext->stop();

#if WARHEAD_PLATFORM != WARHEAD_PLATFORM_WINDOWS
        LOG_FATAL("server", "Caught signal {}: {}", signalNumber, strsignal(signalNumber));
#else
        LOG_FATAL("server", "Caught signal {}", signalNumber);
#endif
    }
}

Warhead::SignalHandlerMgr::~SignalHandlerMgr()
{
    Stop();
}

/*static*/ Warhead::SignalHandlerMgr* Warhead::SignalHandlerMgr::instance()
{
    static SignalHandlerMgr instance;
    return &instance;
}

void Warhead::SignalHandlerMgr::Initialize(std::function<void()>&& execute /*= {}*/)
{
    signal(SIGABRT, &Warhead::AbortHandler);

    // Set signal handlers
    _signalSet = std::make_unique<boost::asio::signal_set>(sIoContextMgr->GetIoContext(), SIGINT, SIGTERM);

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    _signalSet->add(SIGBREAK);
#endif

    _signalSet->async_wait([execute = std::move(execute)](boost::system::error_code const& error, int signalNumber)
    {
        SignalHandler(error, signalNumber);

        if (execute)
            execute();
    });
}

void Warhead::SignalHandlerMgr::Stop()
{
    if (_signalSet)
        _signalSet->cancel();
}
