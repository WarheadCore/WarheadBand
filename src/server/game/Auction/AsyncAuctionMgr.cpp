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

#include "AsyncAuctionMgr.h"
#include "AsyncAuctionOperation.h"
#include "PCQueue.h"
#include "GameTime.h"
#include "Player.h"
#include "StopWatch.h"
#include "Log.h"
#include "TaskScheduler.h"

constexpr Milliseconds LIST_OWNER_ITEMS_DELAY = 100ms;
constexpr Milliseconds LIST_ITEMS_DELAY = 500ms;

bool AuctionListItems::IsNoFilter() const
{
    return ItemClass == 0xffffffff &&
           ItemSubClass == 0xffffffff &&
           InventoryType == 0xffffffff &&
           Quality == 0xffffffff &&
           LevelMin == 0x00 &&
           LevelMax == 0x00 &&
           Usable == 0x00;
}

AsyncAuctionMgr::~AsyncAuctionMgr()
{
    if (_scheduler)
        _scheduler->CancelAll();

    if (_queue)
        _queue->Cancel();

    if (_thread && _thread->joinable())
        _thread->join();
}

/*static*/ AsyncAuctionMgr* AsyncAuctionMgr::instance()
{
    static AsyncAuctionMgr instance;
    return &instance;
}

void AsyncAuctionMgr::Initialize()
{
    StopWatch sw;

    LOG_INFO("server.loading", "Initialize async auction...");

    _queue = std::make_unique<ProducerConsumerQueue<AsyncAuctionOperation*>>();
    _thread = std::make_unique<std::thread>([this](){ ExecuteAsyncQueue(); });
    _scheduler = std::make_unique<TaskScheduler>();

    LOG_INFO("server.loading", ">> Async auction initialized in {}", sw);
    LOG_INFO("server.loading", "");
}

void AsyncAuctionMgr::Update(Milliseconds diff)
{
    _scheduler->Update(diff);
}

void AsyncAuctionMgr::ListOwnerItemsForPlayer(ObjectGuid playerGuid, ObjectGuid creatureGuid)
{
    _scheduler->Schedule(LIST_OWNER_ITEMS_DELAY, [this, playerGuid, creatureGuid](TaskContext)
    {
        Enqueue(new ListOwnerTask(playerGuid, creatureGuid));
    });
}

void AsyncAuctionMgr::ListItemsForPlayer(std::shared_ptr<AuctionListItems> listItems)
{
    _scheduler->Schedule(LIST_ITEMS_DELAY, [this, listItems = std::move(listItems)](TaskContext)
    {
        Enqueue(new ListItemsTask(listItems));
    });
}

void AsyncAuctionMgr::Enqueue(AsyncAuctionOperation* operation)
{
    _queue->Push(operation);
}

void AsyncAuctionMgr::ExecuteAsyncQueue()
{
    if (!_queue)
        return;

    for (;;)
    {
        AsyncAuctionOperation* task{ nullptr };
        _queue->WaitAndPop(task);

        if (!task)
            break;

        {
            std::lock_guard<std::mutex> guard(_mutex);
            task->Execute();
        }

        delete task;
    }
}
