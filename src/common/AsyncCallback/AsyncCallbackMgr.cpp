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

#include "AsyncCallbackMgr.h"
#include "Duration.h"

/*static*/ Warhead::Async::AsyncCallbackMgr* Warhead::Async::AsyncCallbackMgr::instance()
{
    static AsyncCallbackMgr instance;
    return &instance;
}

bool Warhead::Async::AsyncCallback::InvokeIfReady()
{
    if (_future.valid() && _future.wait_for(0s) == std::future_status::ready)
        return true;

    return false;
}

void Warhead::Async::AsyncCallbackMgr::AddAsyncCallback(std::function<void()>&& execute, Microseconds delay /*= 0us*/)
{
    _asyncCallbacks.AddCallback(AsyncCallback(std::async(std::launch::async, [delay, execute = std::move(execute)]()
    {
        if (delay > 0us)
            std::this_thread::sleep_for(delay);

        execute();
    })));
}

void Warhead::Async::AsyncCallbackMgr::ProcessReadyCallbacks()
{
    _asyncCallbacks.ProcessReadyCallbacks();
}
