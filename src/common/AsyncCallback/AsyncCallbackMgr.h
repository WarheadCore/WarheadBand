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

#ifndef ASYNC_CALLBACK_MGR_H_
#define ASYNC_CALLBACK_MGR_H_

#include "AsyncCallbackProcessor.h"
#include "Duration.h"
#include <functional>
#include <future>
#include <memory>

namespace Warhead::Async
{
    using AsyncCallbackFuture = std::future<void>;

    class WH_COMMON_API AsyncCallback
    {
    public:
        AsyncCallback(AsyncCallbackFuture&& future) : _future(std::move(future)) { }
        AsyncCallback(AsyncCallback&&) = default;

        AsyncCallback& operator=(AsyncCallback&&) = default;

        bool InvokeIfReady();

    private:
        AsyncCallbackFuture _future;

        AsyncCallback(AsyncCallback const& right) = delete;
        AsyncCallback& operator=(AsyncCallback const& right) = delete;
    };

    class WH_COMMON_API AsyncCallbackMgr
    {
    public:
        static AsyncCallbackMgr* instance();

        void AddAsyncCallback(std::function<void()>&& execute, Microseconds delay = 0us);
        void ProcessReadyCallbacks();

    private:
        AsyncCallbackMgr() = default;
        ~AsyncCallbackMgr() = default;
        AsyncCallbackMgr(AsyncCallbackMgr const&) = delete;
        AsyncCallbackMgr(AsyncCallbackMgr&&) = delete;
        AsyncCallbackMgr& operator=(AsyncCallbackMgr const&) = delete;
        AsyncCallbackMgr& operator=(AsyncCallbackMgr&&) = delete;

        AsyncCallbackProcessor<AsyncCallback> _asyncCallbacks;
    };
}

#define sAsyncCallbackMgr Warhead::Async::AsyncCallbackMgr::instance()

#endif // ASYNC_CALLBACK_H_
