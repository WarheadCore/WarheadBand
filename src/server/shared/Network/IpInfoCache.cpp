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

#include "IpInfoCache.h"
#include "Config.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Timer.h"

IPInfo::IPInfo(std::string_view ip, bool isBanned) :
    IP(ip), IsBanned(isBanned), LastCheck(GetTimeMS()) { }

void IPInfo::ResetChecks()
{
    LastCheck = GetTimeMS();
    DefaultCheckCount = 0;
    DDOSCheckCount = 0;
}

IPInfoCacheMgr::IPInfoCacheMgr()
{
    _updateDelay = Seconds{ sConfigMgr->GetOption<uint32>("IPCache.Check.Delay", 60) };
    _maxDefaultCheckCount = sConfigMgr->GetOption<uint32>("IPCache.Check.MaxCount", 10);
    _banEnable = sConfigMgr->GetOption<bool>("IPCache.Ban.Enable", true);
    _defaultBanDuration = Seconds{ sConfigMgr->GetOption<uint32>("IPCache.Ban.Duration", 300) };
    _ddosBanDuration = Seconds{ sConfigMgr->GetOption<uint32>("IPCache.DDOSBan.Duration", 604800) };
}

IPInfoCacheMgr* IPInfoCacheMgr::instance()
{
    static IPInfoCacheMgr instance;
    return &instance;
}

IPInfo* IPInfoCacheMgr::GetIpInfo(std::string_view ip)
{
    return Warhead::Containers::MapGetValuePtr(_cache, std::string{ ip });
}

bool IPInfoCacheMgr::CanCheckIpFromDB(std::string_view ip)
{
    std::lock_guard<std::mutex> guard(_mutex);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
        return true;

    if (ipInfo->LastCheck + _updateDelay <= GetTimeMS())
        return true;

    // Increase default check count
    if (++ipInfo->DefaultCheckCount >= _maxDefaultCheckCount)
    {
        LOG_WARN("ipcache", "Found login flood from ip: {}", ip);

        if (_banEnable && !ipInfo->IsBanned)
            AddBanForIP(ip, _defaultBanDuration, "Login flood");
    }

    if (++ipInfo->DDOSCheckCount >= _maxDDOSCheckCount)
    {
        LOG_WARN("ipcache", "Found potential DDOS attack via login flood from ip: {}", ip);
        AddDDOSBan(ip, _ddosBanDuration, "DDOS Login flood");
    }

    return false;
}

bool IPInfoCacheMgr::IsIPBanned(std::string_view ip)
{
    std::lock_guard<std::mutex> guard(_mutex);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
        return false;

    return ipInfo->IsBanned;
}

void IPInfoCacheMgr::UpdateIPInfo(std::string_view ip, bool isBanned /*= false*/)
{
    std::lock_guard<std::mutex> guard(_mutex);

    LOG_TRACE("ipcache", "> Update ip info cache. IP: {}. Is banned: {}", ip, isBanned);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        _cache.emplace(ip, IPInfo(ip, isBanned));
        return;
    }

    ipInfo->IsBanned = isBanned;
    ipInfo->ResetChecks();
}

void IPInfoCacheMgr::AddBanForIP(std::string_view ip, Seconds duration, std::string_view reason)
{
    // Add ban in DB
    {
        auto stmt = AuthDatabase.GetPreparedStatement(LOGIN_INS_IP_BANNED);
        stmt->SetArguments(ip, duration, "System", reason);
        AuthDatabase.Execute(stmt);
    }

    LOG_WARN("ipcache", "Add ban for IP: {}. Duration: {}. Reason: {}", ip, Warhead::Time::ToTimeString(duration), reason);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        _cache.emplace(ip, IPInfo(ip, true));
        return;
    }

    ipInfo->IsBanned = true;
    ipInfo->ResetChecks();
}

void IPInfoCacheMgr::AddDDOSBan(std::string_view ip, Seconds duration, std::string_view reason)
{
    // Add DDOS ban in DB
    {
        // INSERT INTO `ddos_protection` (`IP`, `BanDate`, `UnBanDate`, `Reason`) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP() + ?, ?)
        auto stmt{ AuthDatabase.GetPreparedStatement(LOGIN_INS_DDOS_PROTECTION) };
        stmt->SetArguments(ip, duration, reason);
        AuthDatabase.Execute(stmt);
    }

    LOG_WARN("ipcache", "Add DDOS ban for IP: {}. Duration: {}. Reason: {}", ip, Warhead::Time::ToTimeString(duration), reason);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        _cache.emplace(ip, IPInfo(ip, true));
        return;
    }

    ipInfo->IsBanned = true;
    ipInfo->ResetChecks();
}
