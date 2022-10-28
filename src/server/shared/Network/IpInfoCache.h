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

#ifndef _IP_INFO_CACHE_H_
#define _IP_INFO_CACHE_H_

#include "Define.h"
#include "Duration.h"
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

struct IPInfo
{
    IPInfo(std::string_view ip, bool isBanned);

    std::string IP;
    bool IsBanned{};
    Milliseconds LastCheck{};
    std::size_t DefaultCheckCount{};
    std::size_t DDOSCheckCount{};

    void ResetChecks();
};

class WH_SHARED_API IPInfoCacheMgr
{
public:
    IPInfoCacheMgr();
    ~IPInfoCacheMgr() = default;

    static IPInfoCacheMgr* instance();

    IPInfo* GetIpInfo(std::string_view ip);
    bool CanCheckIpFromDB(std::string_view ip);
    bool IsIPBanned(std::string_view ip);
    void UpdateIPInfo(std::string_view ip, bool isBanned = false);
    void AddBanForIP(std::string_view ip, Seconds duration, std::string_view reason);

private:
    void AddDDOSBan(std::string_view ip, Seconds duration, std::string_view reason);

    std::unordered_map<std::string, IPInfo> _cache;
    std::mutex _mutex;

    // Config
    Milliseconds _updateDelay{ 1min };
    std::size_t _maxDefaultCheckCount{ 10 };
    std::size_t _maxDDOSCheckCount{ 50 };
    bool _banEnable{ true };
    Seconds _defaultBanDuration{ 5min };
    Seconds _ddosBanDuration{ 7_days };
};

#define sIPInfoCacheMgr IPInfoCacheMgr::instance()

#endif // _IP_INFO_CACHE_H_
