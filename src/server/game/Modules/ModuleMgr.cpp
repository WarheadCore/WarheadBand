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

#include "ModuleMgr.h"
#include "Tokenize.h"

namespace
{
    std::string _modulesList;
}

void Warhead::Module::SetEnableModulesList(std::string_view modulesList)
{
    _modulesList = modulesList;
}

std::vector<std::string_view> Warhead::Module::GetEnableModulesList()
{
    std::vector<std::string_view> _list;

    for (auto const& modName : Warhead::Tokenize(_modulesList, ',', false))
        _list.emplace_back(modName);

    return _list;
}
