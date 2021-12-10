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

#ifndef AZEROTHCORE_CIRCULAR_BUFFER_H
#define AZEROTHCORE_CIRCULAR_BUFFER_H

#include <memory>
#include <mutex>
#include <vector>

template <typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(size_t size) :
        buf_(std::unique_ptr<T[]>(new T[size])),
        max_size_(size)
    {

    }

    void put(T item)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        buf_[head_] = item;

        if (full_)
        {
            tail_ = (tail_ + 1) % max_size_;
        }

        head_ = (head_ + 1) % max_size_;

        full_ = head_ == tail_;
    }

    bool empty() const
    {
        //if head and tail are equal, we are empty
        return (!full_ && (head_ == tail_));
    }

    bool full() const
    {
        //If tail is ahead the head by 1, we are full
        return full_;
    }

    size_t capacity() const
    {
        return max_size_;
    }

    size_t size() const
    {
        size_t size = max_size_;

        if (!full_)
        {
            if (head_ >= tail_)
            {
                size = head_ - tail_;
            }
            else
            {
                size += head_ - tail_;
            }
        }

        return size;
    }

    // the implementation of this function is simplified by the fact that head_ will never be lower than tail_
    // when compared to the original implementation of this class
    std::vector<T> content()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return std::vector<T>(buf_.get(), buf_.get() + size());
    }

    T peak_back()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return empty() ? T() : buf_[tail_];
    }

private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    const size_t max_size_;
    bool full_ = 0;
};
#endif
