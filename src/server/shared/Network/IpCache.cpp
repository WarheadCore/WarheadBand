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

#include "IpCache.h"
#include "Config.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StopWatch.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include "Util.h"

namespace
{
    constexpr std::string_view GetBanNameFromType(Warhead::BanIpType type)
    {
        switch (type)
        {
            case Warhead::BanIpType::LoginFlood:
                return "LoginFlood";
            case Warhead::BanIpType::Malformed:
                return "Malformed";
            case Warhead::BanIpType::DDOS:
                return "DDOS";
            default:
                return "Unknown ban type";
        }
    }

    constexpr std::string_view GetAppNameFromType(Warhead::ApplicationType type)
    {
        switch (type)
        {
            case Warhead::ApplicationType::AuthServer:
                return "Auth";
            case Warhead::ApplicationType::WorldServer:
                return "World";
            default:
                return "Unknown app type";
        }
    }
}

Warhead::IPCache::IPCache(std::string_view ip) :
    IP(ip), LastCheck(GetTimeMS()) { }

void Warhead::IPCache::ResetChecks()
{
    LastCheck = GetTimeMS();
    _checkCount.fill(0);
}

std::size_t Warhead::IPCache::GetIncreaseCheckCount(CheckIpType type)
{
    return ++_checkCount[AsUnderlyingType(type)];
}

Warhead::IPCacheMgr* Warhead::IPCacheMgr::instance()
{
    static IPCacheMgr instance;
    return &instance;
}

void Warhead::IPCacheMgr::Initialize(ApplicationType type)
{
    LOG_INFO("server.loading", "Loading ip info cache...");

    _isEnable = sConfigMgr->GetOption<bool>("IPCache.Enable", false);
    if (!_isEnable)
    {
        LOG_WARN("server.loading", "IP cache disabled in config");
        LOG_INFO("server.loading", "");
        return;
    }

    StopWatch sw;

    auto SetBanConfig = [this](std::string const& optionName, uint8 type)
    {
        auto options = sConfigMgr->GetOption<std::string>(optionName, "10 3 50");
        auto const tokens = Warhead::Tokenize(options, ' ', false);
        ASSERT(tokens.size() == BAN_IP_TYPE_SIZE, "Incorrect config option {}", optionName);

        if (type == 1)
        {
            for (std::size_t i{}; i < tokens.size(); i++)
            {
                auto strCheckCount{ tokens[i] };
                auto checkCount{ Warhead::StringTo<std::size_t>(strCheckCount) };
                ASSERT(checkCount, "Incorrect get check count from '{}'", strCheckCount);

                _maxCheckCount[i] = *checkCount;
            }
        }
        else if (type == 2)
        {
            for (std::size_t i{}; i < tokens.size(); i++)
            {
                auto strBanEnable{ tokens[i] };
                auto banEnable{ Warhead::StringTo<bool>(strBanEnable) };
                ASSERT(banEnable, "Incorrect get ban enable from '{}'", strBanEnable);

                _banEnable[i] = *banEnable;
            }
        }
        else if (type == 3)
        {
            for (std::size_t i{}; i < tokens.size(); i++)
            {
                auto strBanDuration{ tokens[i] };
                auto banDuration{ Warhead::StringTo<std::size_t>(strBanDuration) };
                ASSERT(banDuration, "Incorrect get ban duration from '{}'", strBanDuration);

                _banDuration[i] = Seconds{ *banDuration };
            }
        }
    };

    // Load config options
    _updateDelay = Seconds{ sConfigMgr->GetOption<uint32>("IPCache.Check.Delay", 60) };

    SetBanConfig("IPCache.Check.MaxCount", 1);
    SetBanConfig("IPCache.Ban.Enable", 2);
    SetBanConfig("IPCache.Ban.Duration", 3);

    // Set app type
    _appType = type;

    LOG_INFO("server.loading", "IP info cache loaded in {}", sw);
    LOG_INFO("server.loading", "");
}

Warhead::IPCache* Warhead::IPCacheMgr::GetIpInfo(std::string_view ip)
{
    if (!_isEnable)
        return nullptr;

    return Warhead::Containers::MapGetValuePtr(_cache, std::string{ ip });
}

bool Warhead::IPCacheMgr::CanCheckIpFromDB(std::string_view ip)
{
    if (!_isEnable)
        return true;

    std::lock_guard<std::mutex> guard(_mutex);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
        return true;

    if (ipInfo->LastCheck + _updateDelay <= GetTimeMS())
    {
        ipInfo->ResetChecks();
        return true;
    }

    return false;
}

bool Warhead::IPCacheMgr::IsBanned(std::string_view ip)
{
    if (!_isEnable)
        return false;

    auto ipInfo{ GetIpInfo(ip) };
    if (!ipInfo)
        return false;

    bool isBanned{ ipInfo->IsCoreBanned || ipInfo->IsGuardBanned };
    if (!isBanned)
        return false;

    // Client try login with ban ignore authserver
    if (_appType == ApplicationType::WorldServer)
        AddBan(BanIpType::DDOS, ip, "World: Login without an authserver");

    return true;
}

void Warhead::IPCacheMgr::SetBannedForIP(std::string_view ip, CheckIpType type /*= CheckIpType::LoginFlood*/)
{
    if (!_isEnable || !_banEnable[AsUnderlyingType(type)])
        return;

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        auto [iter, isEmplace] = _cache.emplace(ip, IPCache(ip));

        if (type == CheckIpType::LoginFlood)
            iter->second.IsCoreBanned = true;
        else
            iter->second.IsGuardBanned = true;

        return;
    }

    // Skip if ban queued in guard
    if (ipInfo->IsGuardBanned)
        return;

    // Skip if already ban login flood
    if (type == CheckIpType::LoginFlood && ipInfo->IsCoreBanned)
        return;

    if (type == CheckIpType::LoginFlood)
        ipInfo->IsCoreBanned = true;
    else
        ipInfo->IsGuardBanned = true;

    ipInfo->ResetChecks();
}

void Warhead::IPCacheMgr::CheckIp(std::string_view ip, CheckIpType type /*= CheckIpType::LoginFlood*/)
{
    if (!_isEnable)
        return;

    std::lock_guard<std::mutex> guard(_mutex);

    LOG_TRACE("ipcache", "Update ip cache for IP: {}", ip);

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        _cache.emplace(ip, IPCache(ip));
        return;
    }

    auto checkCount{ ipInfo->GetIncreaseCheckCount(type) };

    if (type == CheckIpType::Malformed)
    {
        if (checkCount >= _maxCheckCount[AsUnderlyingType(BanIpType::Malformed)])
        {
            LOG_WARN("ipcache", "Found malformed packet flood from ip: {}. Check count: {}", ip, checkCount);
            AddBan(BanIpType::Malformed, ip, StringFormat("{}: Malformed packet flood", GetAppNameFromType(_appType)));
        }
    }
    else
    {
        if (checkCount >= _maxCheckCount[AsUnderlyingType(BanIpType::LoginFlood)])
        {
            LOG_WARN("ipcache", "Found login flood from ip: {}. Check count: {}", ip, checkCount);
            AddBan(BanIpType::LoginFlood, ip, StringFormat("{}: Login flood", GetAppNameFromType(_appType)));
        }

        if (checkCount >= _maxCheckCount[AsUnderlyingType(BanIpType::DDOS)])
        {
            LOG_WARN("ipcache", "Found potential DDOS attack via login flood from ip: {}. Check count: {}", ip, checkCount);
            AddBan(BanIpType::DDOS, ip, StringFormat("{}: DDOS Login flood", GetAppNameFromType(_appType)));
        }
    }
}

void Warhead::IPCacheMgr::AddBan(BanIpType type, std::string_view ip, std::string_view reason)
{
    if (!_banEnable[AsUnderlyingType(type)])
        return;

    auto ipInfo = GetIpInfo(ip);
    if (!ipInfo)
    {
        auto [iter, isEmplace] = _cache.emplace(ip, IPCache(ip));

        if (type != BanIpType::LoginFlood)
            iter->second.IsGuardBanned = true;
        else
            iter->second.IsCoreBanned = true;
    }
    else
    {
        // Skip if ban queued in guard
        if (ipInfo->IsGuardBanned)
            return;

        // Skip if already ban login flood
        if (type == BanIpType::LoginFlood && ipInfo->IsCoreBanned)
            return;

        if (type != BanIpType::LoginFlood)
            ipInfo->IsGuardBanned = true;
        else
            ipInfo->IsCoreBanned = true;

        ipInfo->ResetChecks();
    }

    Seconds banDuration{ _banDuration[AsUnderlyingType(type)] };

    if (type == BanIpType::LoginFlood)
    {
        auto stmt = AuthDatabase.GetPreparedStatement(LOGIN_INS_IP_BANNED);
        stmt->SetArguments(ip, banDuration, "WarheadIpCache", reason);
        AuthDatabase.Execute(stmt);
    }
    else
    {
        // INSERT INTO `ddos_protection` (`IP`, `BanDate`, `UnBanDate`, `Reason`) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP() + ?, ?)
        auto stmt{ AuthDatabase.GetPreparedStatement(LOGIN_INS_DDOS_PROTECTION) };
        stmt->SetArguments(ip, banDuration, reason);
        AuthDatabase.Execute(stmt);
    }

    LOG_WARN("ipcache", "Add ban for IP: {}. Type: {}. Duration: {}. Reason: {}", ip, GetBanNameFromType(type), Warhead::Time::ToTimeString(banDuration), reason);
}
