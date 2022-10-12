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

#include "ThreadPoolMgr.h"
#include "Config.h"
#include "Errors.h"
#include "IoContext.h"
#include "IoContextMgr.h"
#include <boost/asio/thread_pool.hpp>

/*static*/ Warhead::ThreadPoolMgr* Warhead::ThreadPoolMgr::instance()
{
    static ThreadPoolMgr instance;
    return &instance;
}

void Warhead::ThreadPoolMgr::AddWork(std::function<void()>&& work)
{
    boost::asio::post(*_threadPool, std::move(work));
}

void Warhead::ThreadPoolMgr::Initialize(std::size_t numThreads /*= 1*/)
{
    ASSERT(!_threadPool, "Thread poll is initialized!");

    _threadPool = std::make_unique<boost::asio::thread_pool>(numThreads);

    for (int i = 0; i < numThreads; ++i)
        AddWork([]() { sIoContextMgr->GetIoContextPtr()->run(); });
}

void Warhead::ThreadPoolMgr::Wait()
{
    _threadPool->join();
}