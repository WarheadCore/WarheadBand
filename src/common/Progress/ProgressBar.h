/*
 * This file is part of the WarheadApp Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include "Define.h"
#include <string_view>
#include <functional>

/**
 * RAII implementation of a progress bar.
 */
class ProgressBar
{
public:
    /**
     * Constructor.
     * It takes two values: the expected number of iterations whose progress we
     * want to monitor and an initial message to be displayed on top of the bar
     * (which can be updated with updateLastPrintedMessage()).
     */
    ProgressBar(std::size_t totalSize, std::size_t currentSize = 0);

    /**
     * Destructor to guarantee RAII.
     */
    ~ProgressBar();

    // Make the object non-copyable
    ProgressBar(const ProgressBar& o) = delete;
    ProgressBar& operator=(const ProgressBar& o) = delete;

    /**
     * Must be invoked when the progress bar is no longer needed to restore the
     * position of the cursor to the end of the output.
     * It is automatically invoked when the object is destroyed.
     */
    void Stop();

    /**
     * Overloaded prefix operator, used to indicate that the has been a new
     * iteration.
     */
    void Update(std::size_t readSize);

private:
    std::size_t _currentSize{ 0 };
    std::size_t _totalSize{ 0 };
    bool mEnded;
};

#endif /* PROGRESS_BAR_H */
