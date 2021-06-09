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

#ifndef MPQ_MANAGER_H
#define MPQ_MANAGER_H

#include "MPQ.h"
#include "PolicyLock.h"
#include <map>
#include <mutex>
#include <set>

class DBC;
class MPQManager
{
public:
    MPQManager() {}
    ~MPQManager() {}

    void Initialize();
    FILE* GetFile(const std::string& path);
    FILE* GetFileFrom(const std::string& path, MPQArchive* file);
    DBC* GetDBC(const std::string& name);
    std::vector<std::string> GetAllFiles(std::string extension);

    std::deque<MPQArchive*> Archives;
    int32 BaseLocale;
    std::set<uint32> AvailableLocales;
    std::map<uint32, MPQArchive*> LocaleFiles;

    static char const* Files[];
    static char const* Languages[];
protected:
    void InitializeDBC();
private:
    std::mutex mutex;
};

extern MPQManager* MPQHandler;
#endif