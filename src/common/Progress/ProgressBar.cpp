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
#include <indicators/cursor_control.hpp>
#include <indicators/cursor_movement.hpp>
#include <iostream>

ProgressBar::ProgressBar(std::string_view prefixText, std::size_t size, std::size_t current /*= 0*/)
{
    _bar = std::make_unique<indicators::ProgressBar>();
    Init(prefixText, size, current);
}

void ProgressBar::Init(std::string_view prefixText, std::size_t size, std::size_t current /*= 0*/)
{
    std::cout << "\n";

    _bar->set_option(indicators::option::BarWidth{ 40 });
    _bar->set_option(indicators::option::Start{ "[" });
    _bar->set_option(indicators::option::Fill{ "=" });
    _bar->set_option(indicators::option::Lead{ ">" });
    _bar->set_option(indicators::option::Remainder{ " " });
    _bar->set_option(indicators::option::End{ "]" });
    _bar->set_option(indicators::option::PrefixText{ std::string(prefixText) + " " });
    _bar->set_option(indicators::option::ForegroundColor{ indicators::Color::green });
    //_bar->set_option(indicators::option::ShowElapsedTime{ true });
    _bar->set_option(indicators::option::ShowRemainingTime{ true });
    _bar->set_option(indicators::option::ShowPercentage{ true });
    _bar->set_option(indicators::option::MinProgress{ current });
    _bar->set_option(indicators::option::MaxProgress{ size });
    _bar->set_option(indicators::option::FontStyles{ std::vector<indicators::FontStyle>{ indicators::FontStyle::bold } });

    indicators::show_console_cursor(false);
}

void ProgressBar::Stop(bool hide /*= false*/)
{
    if (hide)
        ClearLine();

    // Show cursor
    indicators::show_console_cursor(true);

    // Reset color and print new line
    std::cout << termcolor::reset;
    std::cout << "\n";

    if (IsCompleted())
        return;

    _bar->mark_as_completed();
}

void ProgressBar::Update(std::size_t progress /*= 0*/)
{
    if (IsCompleted())
        return;

    if (progress)
        _bar->set_progress(progress);

    _bar->tick();
}

bool ProgressBar::IsCompleted()
{
    return _bar->is_completed();
}

std::size_t ProgressBar::GetProgress()
{
    return _bar->current();
}

void ProgressBar::ClearLine()
{
    indicators::move_up(1);
    indicators::erase_line();
    std::cout << std::flush;
}

void ProgressBar::UpdatePrefixText(std::string_view text)
{
    _bar->set_option(indicators::option::PrefixText{ std::string(text) + " " });
}

void ProgressBar::UpdatePostfixText(std::string_view text)
{
    _bar->set_option(indicators::option::PostfixText{ std::string(text) + " " });
}
