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

#include "LogMessage.h"

Warhead::LogMessage::LogMessage(std::string_view source, std::string_view text, MessageLevel level, std::string_view option /*= {}*/) :
    _source(source),
    _text(text),
    _level(level),
    _option(option)
{
}

Warhead::LogMessage::LogMessage(std::string_view source, std::string_view text, MessageLevel level,
    std::string_view file, std::size_t line, std::string_view function, std::string_view option /*= {}*/) :
    _source(source),
    _text(text),
    _level(level),
    _file(file),
    _line(line),
    _function(function),
    _option(option)
{
}
