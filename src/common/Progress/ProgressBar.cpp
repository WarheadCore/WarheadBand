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

#include "ProgressBar.h"

#ifdef WH_DISABLE_PROGRESS_BAR
#include "Log.h"
#else
#include "Errors.h"
#include <indicators/cursor_control.hpp>
#include <indicators/cursor_movement.hpp>
#include <indicators/terminal_size.hpp>
#include <iostream>

constexpr std::size_t ADDITIONAL_INFO_SIZE = 15;
constexpr std::size_t DEFAULT_POST_INFO_SIZE = 50;
constexpr std::size_t DEFAULT_POST_BAR_SIZE = ADDITIONAL_INFO_SIZE + DEFAULT_POST_INFO_SIZE;
#endif

ProgressBar::ProgressBar(std::string_view prefixText, std::size_t size, std::size_t current /*= 0*/)
{
#ifndef WH_DISABLE_PROGRESS_BAR
    _bar = std::make_unique<indicators::ProgressBar>();
    _size = size;
    Init(prefixText, size, current);
#endif
}

#ifndef WH_DISABLE_PROGRESS_BAR
void ProgressBar::Init(std::string_view prefixText, std::size_t size, std::size_t current /*= 0*/) const
{
    std::cout << "\n";

    if (!prefixText.empty())
        _bar->set_option(indicators::option::PrefixText{ std::string(prefixText) + " " });

    std::size_t terminalWidth{ GetTerninalWidth() };

    // CI
    if (!terminalWidth || terminalWidth > 120)
        terminalWidth = 120;

    ASSERT(terminalWidth > DEFAULT_POST_BAR_SIZE, "For using ProgressBar need terminal width > {} chars. You {}.", DEFAULT_POST_BAR_SIZE, terminalWidth);

    _bar->set_option(indicators::option::BarWidth{ terminalWidth - DEFAULT_POST_BAR_SIZE });
    _bar->set_option(indicators::option::Start{ "[" });
    _bar->set_option(indicators::option::Fill{ "=" });
    _bar->set_option(indicators::option::Lead{ ">" });
    _bar->set_option(indicators::option::Remainder{ " " });
    _bar->set_option(indicators::option::End{ "]" });
    _bar->set_option(indicators::option::ForegroundColor{ indicators::Color::green });
    _bar->set_option(indicators::option::ShowRemainingTime{ true });
    _bar->set_option(indicators::option::ShowPercentage{ true });
    _bar->set_option(indicators::option::MinProgress{ current });
    _bar->set_option(indicators::option::MaxProgress{ size });
    _bar->set_option(indicators::option::FontStyles{ std::vector<indicators::FontStyle>{ indicators::FontStyle::bold } });

    //_bar->set_option(indicators::option::ShowElapsedTime{ true });

    indicators::show_console_cursor(false);
}
#endif

void ProgressBar::Stop(bool hide /*= false*/) const
{
#ifndef WH_DISABLE_PROGRESS_BAR
    if (hide)
        ClearLine();

    // Show cursor
    indicators::show_console_cursor(true);

    if (!_bar->is_completed())
        _bar->mark_as_completed();

    // Reset color and print new line
    std::cout << termcolor::reset;
    std::cout << "\n";
#else
    LOG_INFO("server", "");
#endif
}

void ProgressBar::Update(std::size_t progress /*= 0*/)
{
#ifndef WH_DISABLE_PROGRESS_BAR
    if (IsCompleted())
        return;

    SaveUnprintProgress(progress);

    if (!CanPrintProgress())
        return;

    if (_unprintProgress)
        _bar->set_progress(_bar->current() + _unprintProgress);
    else
        _bar->tick();

    ClearUnprintProgress();
#endif
}

#ifndef WH_DISABLE_PROGRESS_BAR
bool ProgressBar::IsCompleted() const
{
    return _bar->is_completed();
}

std::size_t ProgressBar::GetProgress() const
{
    return _bar->current();
}
#endif

void ProgressBar::ClearLine() const
{
#ifndef WH_DISABLE_PROGRESS_BAR
    indicators::move_up(1);
    indicators::erase_line();
    std::cout << std::flush;
#endif
}

void ProgressBar::UpdatePrefixText(std::string_view text) const
{
#ifndef WH_DISABLE_PROGRESS_BAR
    _bar->set_option(indicators::option::PrefixText{ std::string(text) + " " });
#else
    LOG_INFO("server", "{}", text);
#endif
}

void ProgressBar::UpdatePostfixText(std::string_view text) const
{
#ifndef WH_DISABLE_PROGRESS_BAR
    if (text.length() > DEFAULT_POST_INFO_SIZE - 3)
        text = text.substr(0, DEFAULT_POST_INFO_SIZE - 3);

    _bar->set_option(indicators::option::PostfixText{ std::string(text) });
#else
    UpdatePrefixText(text);
#endif
}

#ifndef WH_DISABLE_PROGRESS_BAR
std::size_t ProgressBar::GetTerninalWidth() const
{
    return indicators::terminal_width();
}

void ProgressBar::SaveUnprintProgress(std::size_t progress)
{
    if (progress)
        _unprintProgress += progress;
    else
        _unprintProgress++;
}

void ProgressBar::ClearUnprintProgress()
{
    _unprintProgress = 0;
}

bool ProgressBar::CanPrintProgress()
{
    if (_bar->current() + _unprintProgress >= _size)
        return true;

    auto timeNow = std::chrono::duration_cast<Microseconds>(std::chrono::steady_clock::now().time_since_epoch());
    if (_lastUpdateTime > 0us && _lastUpdateTime + _tickTime > timeNow)
        return false;

    _lastUpdateTime = timeNow;
    return true;
}

void ProgressBar::SetTickTime(Microseconds tickTime)
{
    _tickTime = tickTime;
}
#endif
