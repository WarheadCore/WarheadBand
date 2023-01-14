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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "MapUpdater.h"
#include "DatabaseEnv.h"
#include "LFGMgr.h"
#include "Map.h"
#include "Metric.h"

class UpdateRequest
{
public:
    UpdateRequest() = default;
    virtual ~UpdateRequest() = default;

    virtual void UpdateMap() = 0;
};

class MapUpdateRequest : public UpdateRequest
{
public:
    MapUpdateRequest(Map& m, MapUpdater& u, uint32 d, uint32 sd) :
        _map(m), _updater(u), _mapDiff(d), _sDiff(sd) { }

    void UpdateMap() override
    {
        METRIC_TIMER("map_update_time_diff", METRIC_TAG("map_id", std::to_string(_map.GetId())));
        _map.Update(_mapDiff, _sDiff);
        _updater.FinishUpdate();
    }

private:
    Map& _map;
    MapUpdater& _updater;
    uint32 _mapDiff;
    uint32 _sDiff;
};

class LFGUpdateRequest : public UpdateRequest
{
public:
    LFGUpdateRequest(MapUpdater& u, uint32 d) : _updater(u), _diff(d) {}

    void UpdateMap() override
    {
        sLFGMgr->Update(_diff, 1);
        _updater.FinishUpdate();
    }

private:
    MapUpdater& _updater;
    uint32 _diff;
};

void MapUpdater::InitThreads(std::size_t num_threads)
{
    _workerThreads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
        _workerThreads.emplace_back(&MapUpdater::InitializeThread, this);
}

void MapUpdater::Stop()
{
    _cancelationToken = true;

    WaitThreads();
    _queue.Cancel();

    for (auto& thread : _workerThreads)
        if (thread.joinable())
            thread.join();
}

void MapUpdater::WaitThreads()
{
    std::unique_lock<std::mutex> guard(_lock);

    while (pending_requests)
        _condition.wait(guard);

    guard.unlock();
}

void MapUpdater::ScheduleUpdate(Map& map, uint32 diff, uint32 s_diff)
{
    std::lock_guard<std::mutex> guard(_lock);
    ++pending_requests;
    _queue.Push(new MapUpdateRequest(map, *this, diff, s_diff));
}

void MapUpdater::ScheduleLfgUpdate(uint32 diff)
{
    std::lock_guard<std::mutex> guard(_lock);
    ++pending_requests;
    _queue.Push(new LFGUpdateRequest(*this, diff));
}

bool MapUpdater::IsActive()
{
    return !_workerThreads.empty();
}

void MapUpdater::FinishUpdate()
{
    std::lock_guard<std::mutex> lock(_lock);
    --pending_requests;
    _condition.notify_all();
}

void MapUpdater::InitializeThread()
{
    AuthDatabase.WarnAboutSyncQueries(true);
    CharacterDatabase.WarnAboutSyncQueries(true);
    WorldDatabase.WarnAboutSyncQueries(true);

    for (;;)
    {
        UpdateRequest* request = nullptr;

        _queue.WaitAndPop(request);
        if (_cancelationToken)
            return;

        request->UpdateMap();
        delete request;
    }
}
