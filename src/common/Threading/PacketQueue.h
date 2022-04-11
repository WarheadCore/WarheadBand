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

#ifndef _PACKET_QUEUE_H_
#define _PACKET_QUEUE_H_

#include <deque>
#include <functional>
#include <mutex>

namespace Warhead::Impl
{
    template <typename Packet>
    class DefaultPacketQueue
    {
        //! Lock access to the queue.
        std::mutex _lock;

        //! Storage backing the queue.
        std::deque<Packet*> _queue;

        //! Cancellation flag.
        volatile bool _canceled{ false };

    public:
        //! Create a PacketQueue.
        DefaultPacketQueue() = default;

        //! Destroy a PacketQueue.
        ~DefaultPacketQueue()
        {
            //! Empty incoming PacketQueue
            for (auto& packet : _queue)
                delete packet;

            _queue.clear();
        }

        //! Adds an item to the queue.
        void AddPacket(Packet* packet)
        {
            std::lock_guard<std::mutex> lock(_lock);
            _queue.emplace_back(packet);
        }

        //! Adds items back to front of the queue
        template<class Container>
        void ReadContainer(Container container)
        {
            std::lock_guard<std::mutex> lock(_lock);
            _queue.insert(_queue.begin(), std::begin(container), std::end(container));
        }

        //! Gets the next result in the queue, if any.
        bool GetNextPacket(Packet*& result)
        {
            std::lock_guard<std::mutex> lock(_lock);

            if (_queue.empty())
                return false;

            result = _queue.front();
            _queue.pop_front();

            return true;
        }

        bool GetNextPacket(Packet*& result, std::function<bool()> const& check)
        {
            std::lock_guard<std::mutex> lock(_lock);

            if (_queue.empty())
            {
                return false;
            }

            result = _queue.front();
            if (!check())
            {
                return false;
            }

            _queue.pop_front();
            return true;
        }

        //! Cancels the queue.
        void Cancel()
        {
            std::lock_guard<std::mutex> lock(_lock);
            _canceled = true;
        }

        //! Checks if the queue is cancelled.
        bool Cancelled()
        {
            std::lock_guard<std::mutex> lock(_lock);
            return _canceled;
        }

        ///! Checks if we're empty or not with locks held
        bool IsEmpty()
        {
            std::lock_guard<std::mutex> lock(_lock);
            return _queue.empty();
        }
    };

    template <typename Packet, typename Check>
    class CheckPacketQueue
    {
    public:
        using Element = std::pair<Packet*, Check>;

        //! Create a PacketQueue.
        CheckPacketQueue() = default;

        //! Destroy a PacketQueue.
        ~CheckPacketQueue()
        {
            for (auto& element : _queue)
            {
                delete element->first;
                delete element;
            }
        }

        //! Adds an item to the queue.
        void AddPacket(Packet* packet, Check nodeID)
        {
            std::lock_guard<std::mutex> lock(_lock);
            _queue.emplace_back(new std::pair(packet, nodeID));
        }

        //! Adds items back to front of the queue
        template<class Container>
        void ReadContainer(Container& container)
        {
            std::lock_guard<std::mutex> lock(_lock);
            _queue.insert(_queue.begin(), std::begin(container), std::end(container));
        }

        //! Gets the next result in the queue with check node id
        bool GetNextPacket(Packet*& packet, Check& nodeID)
        {
            std::lock_guard<std::mutex> lock(_lock);

            if (_queue.empty())
                return false;

            auto result = _queue.front();
            packet = result->first;
            nodeID = result->second;

            _queue.pop_front();

            return true;
        }

        //! Cancels the queue.
        void Cancel()
        {
            std::lock_guard<std::mutex> lock(_lock);
            _canceled = true;
        }

        //! Checks if the queue is cancelled.
        bool Cancelled()
        {
            std::lock_guard<std::mutex> lock(_lock);
            return _canceled;
        }

        ///! Checks if we're empty or not with locks held
        bool IsEmpty()
        {
            std::lock_guard<std::mutex> lock(_lock);
            return _queue.empty();
        }

    private:
        //! Lock access to the queue.
        std::mutex _lock;

        //! Storage backing the queue.
        std::deque<Element*> _queue;

        //! Cancellation flag.
        volatile bool _canceled{ false };
    };
}

template <typename Packet, typename Check = void>
using PacketQueue = std::conditional_t<std::is_integral<Check>::value, Warhead::Impl::CheckPacketQueue<Packet, Check>, Warhead::Impl::DefaultPacketQueue<Packet>>;

#endif
