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

#include "FileUtil.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

void Warhead::File::CorrectDirPath(std::string& path)
{
    if (path.empty())
    {
        path = fs::absolute(fs::current_path()).generic_string();
        return;
    }

    std::replace(std::begin(path), std::end(path), '\\', '/');

    if (path.at(path.length() - 1) != '/')
        path.push_back('/');
}

bool Warhead::File::CreateDirIfNeed(std::string_view path)
{
    if (path.empty())
        return true;

    fs::path dirPath{ path };

    if (fs::exists(dirPath) && fs::is_directory(path))
        return true;

    try
    {
        return fs::create_directory(dirPath);
    }
    catch (...)
    {
        return false;
    }
}
