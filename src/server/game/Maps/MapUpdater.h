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

#ifndef MAP_UPDATER_H_
#define MAP_UPDATER_H_

#include "Define.h"
#include "PCQueue.h"
#include <thread>

class Map;
class UpdateRequest;

class WH_GAME_API MapUpdater
{
public:
    MapUpdater() = default;
    ~MapUpdater() = default;

    void ScheduleUpdate(Map& map, uint32 diff, uint32 s_diff);
    void ScheduleLfgUpdate(uint32 diff);
    void WaitThreads();
    void InitThreads(std::size_t num_threads);
    void Stop();
    bool IsActive();
    void FinishUpdate();

private:
    void InitializeThread();

    ProducerConsumerQueue<UpdateRequest*> _queue;

    std::vector<std::thread> _workerThreads;
    std::atomic<bool> _cancelationToken;

    std::mutex _lock;
    std::condition_variable _condition;
    std::size_t pending_requests{};
};

#endif
