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

#include "EventProcessor.h"
#include "Errors.h"

void BasicEvent::ScheduleAbort()
{
    ASSERT(IsRunning()
           && "Tried to scheduled the abortion of an event twice!");
    m_abortState = AbortState::STATE_ABORT_SCHEDULED;
}

void BasicEvent::SetAborted()
{
    ASSERT(!IsAborted()
           && "Tried to abort an already aborted event!");
    m_abortState = AbortState::STATE_ABORTED;
}

EventProcessor::~EventProcessor()
{
    KillAllEvents(true);
}

void EventProcessor::Update(uint32 p_time)
{
    // update time
    m_time += p_time;

    // main event loop
    EventList::iterator i;
    while (((i = m_events.begin()) != m_events.end()) && i->first <= m_time)
    {
        // get and remove event from queue
        BasicEvent* event = i->second;
        m_events.erase(i);

        if (event->IsRunning())
        {
            if (event->Execute(m_time, p_time))
            {
                // completely destroy event if it is not re-added
                delete event;
            }
            continue;
        }

        if (event->IsAbortScheduled())
        {
            event->Abort(m_time);
            // Mark the event as aborted
            event->SetAborted();
        }

        if (event->IsDeletable())
        {
            delete event;
            continue;
        }

        // Reschedule non deletable events to be checked at
        // the next update tick
        AddEvent(event, CalculateTime(1), false, 0);
    }
}

void EventProcessor::KillAllEvents(bool force)
{
    // first, abort all existing events
    for (auto itr = m_events.begin(); itr != m_events.end();)
    {
        // Abort events which weren't aborted already
        if (!itr->second->IsAborted())
        {
            itr->second->SetAborted();
            itr->second->Abort(m_time);
        }

        // Skip non-deletable events when we are
        // not forcing the event cancellation.
        if (!force && !itr->second->IsDeletable())
        {
            ++itr;
            continue;
        }

        delete itr->second;

        if (force)
            ++itr; // Clear the whole container when forcing
        else
            itr = m_events.erase(itr);
    }

    if (force)
        m_events.clear();
}

void EventProcessor::CancelEventGroup(uint8 group)
{
    for (auto itr = m_events.begin(); itr != m_events.end();)
    {
        if (itr->second->m_eventGroup != group)
        {
            ++itr;
            continue;
        }

        // Abort events which weren't aborted already
        if (!itr->second->IsAborted())
        {
            itr->second->SetAborted();
            itr->second->Abort(m_time);
        }

        delete itr->second;
        itr = m_events.erase(itr);
    }
}

void EventProcessor::AddEvent(BasicEvent* Event, uint64 e_time, bool set_addtime, uint8 eventGroup)
{
    if (set_addtime)
        Event->m_addTime = m_time;
    Event->m_execTime = e_time;
    Event->m_eventGroup = eventGroup;
    m_events.insert(std::pair<uint64, BasicEvent*>(e_time, Event));
}

void EventProcessor::ModifyEventTime(BasicEvent* event, Milliseconds newTime)
{
    for (auto itr = m_events.begin(); itr != m_events.end(); ++itr)
    {
        if (itr->second != event)
            continue;

        event->m_execTime = newTime.count();
        m_events.erase(itr);
        m_events.insert(std::pair<uint64, BasicEvent*>(newTime.count(), event));
        break;
    }
}

uint64 EventProcessor::CalculateTime(uint64 t_offset) const
{
    return (m_time + t_offset);
}

uint64 EventProcessor::CalculateQueueTime(uint64 delay) const
{
    return CalculateTime(delay - (m_time % delay));
}
