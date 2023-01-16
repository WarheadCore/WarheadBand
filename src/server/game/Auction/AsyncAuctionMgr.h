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

#ifndef WARHEAD_ASYNC_AUCTION_MGR_H_
#define WARHEAD_ASYNC_AUCTION_MGR_H_

#include "AuctionFwd.h"
#include <mutex>
#include <thread>
#include <memory>

template <typename T>
class ProducerConsumerQueue;

class AsyncAuctionOperation;
class Player;
class TaskScheduler;

class WH_GAME_API AsyncAuctionMgr
{
public:
    static AsyncAuctionMgr* instance();

    void Initialize();
    void Update(Milliseconds diff);

    void ListOwnerItemsForPlayer(ObjectGuid playerGuid, ObjectGuid creatureGuid);
    void ListItemsForPlayer(std::shared_ptr<AuctionListItems> listItems);

    inline std::mutex& GetLock() { return _mutex; }

private:
    void ExecuteAsyncQueue();
    void Enqueue(AsyncAuctionOperation* operation);

    std::unique_ptr<ProducerConsumerQueue<AsyncAuctionOperation*>> _queue;
    std::unique_ptr<std::thread> _thread;
    std::unique_ptr<TaskScheduler> _scheduler;
    std::mutex _mutex;

    AsyncAuctionMgr() = default;
    ~AsyncAuctionMgr();
    AsyncAuctionMgr(AsyncAuctionMgr const&) = delete;
    AsyncAuctionMgr(AsyncAuctionMgr&&) = delete;
    AsyncAuctionMgr& operator=(AsyncAuctionMgr const&) = delete;
    AsyncAuctionMgr& operator=(AsyncAuctionMgr&&) = delete;
};

#define sAsyncAuctionMgr AsyncAuctionMgr::instance()

#endif
