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

#ifndef _IP_CACHE_H_
#define _IP_CACHE_H_

#include "Define.h"
#include "Duration.h"
#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Warhead
{
    enum class ApplicationType : uint8
    {
        AuthServer,
        WorldServer
    };

    enum class CheckIpType : uint8
    {
        LoginFlood,
        Malformed,

        Max
    };

    enum class BanIpType : uint8
    {
        LoginFlood,
        Malformed,
        DDOS,

        Max
    };

    constexpr uint8 CHECK_IP_TYPE_SIZE = static_cast<uint8>(CheckIpType::Max);
    constexpr uint8 BAN_IP_TYPE_SIZE = static_cast<uint8>(BanIpType::Max);

    struct IPCache
    {
        explicit IPCache(std::string_view ip);

        std::string IP;
        bool IsCoreBanned{};
        bool IsGuardBanned{};
        Milliseconds LastCheck{};

        void ResetChecks();
        std::size_t GetIncreaseCheckCount(CheckIpType type);

    private:
        std::array<std::size_t, CHECK_IP_TYPE_SIZE> _checkCount{};
    };

    class WH_SHARED_API IPCacheMgr
    {
    public:
        IPCacheMgr() = default;
        ~IPCacheMgr() = default;

        static IPCacheMgr* instance();

        void Initialize(ApplicationType type);

        IPCache* GetIpInfo(std::string_view ip);
        bool CanCheckIpFromDB(std::string_view ip);
        void CheckIp(std::string_view ip, CheckIpType type = CheckIpType::LoginFlood);
        bool IsBanned(std::string_view ip);
        void SetBannedForIP(std::string_view ip, CheckIpType type = CheckIpType::LoginFlood);

    private:
        void AddBan(BanIpType type, std::string_view ip, std::string_view reason = {});

        std::unordered_map<std::string, IPCache> _cache;
        std::mutex _mutex;
        ApplicationType _appType{ ApplicationType::WorldServer };

        // Config
        bool _isEnable{};
        Milliseconds _updateDelay{ 1min };
        std::array<std::size_t, BAN_IP_TYPE_SIZE> _maxCheckCount{};
        std::array<bool, BAN_IP_TYPE_SIZE> _banEnable{};
        std::array<Seconds, BAN_IP_TYPE_SIZE> _banDuration{};
    };
}

#define sIPCacheMgr Warhead::IPCacheMgr::instance()

#endif // _IP_INFO_CACHE_H_
