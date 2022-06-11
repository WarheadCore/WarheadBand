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
#include "Duration.h"
#include <memory>
#include <string_view>
#include <indicators/progress_bar.hpp>

constexpr Microseconds DEFAULT_TICK_TIME = 100ms;

class ProgressBar
{
public:
    ProgressBar(std::string_view prefixText, std::size_t totalSize, std::size_t currentSize = 0);

    // Initialize ProgressBar
    void Init(std::string_view prefixText, std::size_t size, std::size_t current = 0) const;

    // Stop
    void Stop(bool hide = false) const;

    // Update with progress if need
    void Update(std::size_t progress = 0);

    // Check if completed
    bool IsCompleted() const;

    // Get current progress
    std::size_t GetProgress() const;

    // Clear current line
    void ClearLine() const;

    // Text's
    void UpdatePrefixText(std::string_view text) const;
    void UpdatePostfixText(std::string_view text) const;

    void SetTickTime(Microseconds tickTime);

private:
    std::size_t GetTerninalWidth() const;
    void SaveUnprintProgress(std::size_t current);
    void ClearUnprintProgress();
    bool CanPrintProgress();

    std::unique_ptr<indicators::ProgressBar> _bar;
    std::size_t _unprintProgress{ 0 };
    std::size_t _size{ 0 };
    Microseconds _lastUpdateTime{ 0us };
    Microseconds _tickTime{ DEFAULT_TICK_TIME };

    // Make the object non-copyable
    ProgressBar(const ProgressBar& o) = delete;
    ProgressBar& operator=(const ProgressBar& o) = delete;
};

#endif
