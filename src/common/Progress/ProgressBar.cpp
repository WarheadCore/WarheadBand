/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
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

#include "ProgressBar.h"
#include "StringFormat.h"
#include "Log.h"
#include <iostream>
#include <iomanip>
#include <sstream>

constexpr auto LENGTH_OF_PROGRESS_BAR = 50;
constexpr auto PERCENTAGE_BIN_SIZE = 100.0 / LENGTH_OF_PROGRESS_BAR;

namespace
{
    void PrintProgressBar(unsigned int percentage, std::size_t size, std::size_t current)
    {
        const int progress = static_cast<int>(percentage / PERCENTAGE_BIN_SIZE);

        std::ostringstream ss;
        ss << " " << std::setw(3) << std::right << percentage << "% ";
        std::string bar("[" + std::string(LENGTH_OF_PROGRESS_BAR - 2, ' ') + "]");
        //bar.append(" " + Warhead::StringFormat("({}/{})", current, size));

        unsigned int numberOfSymbols = std::min(std::max(0, progress - 1), LENGTH_OF_PROGRESS_BAR - 2);

        bar.replace(1, numberOfSymbols, std::string(numberOfSymbols, '*'));

        ss << bar;
        std::cout << ss.str() << "\r" << std::flush;
    }
}

ProgressBar::ProgressBar(std::size_t totalSize, std::size_t currentSize /*= 0*/) :
    _totalSize(totalSize),
    _currentSize(currentSize),
    mEnded(false)
{
    std::cout << "\n";
    PrintProgressBar(0, totalSize, currentSize);
}

ProgressBar::~ProgressBar()
{
    Stop();
}

void ProgressBar::Update(std::size_t readSize)
{
    if (mEnded)
        throw std::runtime_error("Attempted to use progress bar after having terminated it");

    _currentSize += readSize;

    const unsigned int percentage = static_cast<unsigned int>(_currentSize * 100 / _totalSize);

    PrintProgressBar(percentage, _totalSize, _currentSize);
}

void ProgressBar::Stop()
{
    if (!mEnded)
        std::cout << std::string(2, '\n');

    mEnded = true;
}
