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

#ifndef _VIP_H_
#define _VIP_H_

#include "Duration.h"
#include "TaskScheduler.h"
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class Player;
class ChatHandler;
class ObjectGuid;
class Creature;

enum class VipRate
{
    XP,
    Honor,
    ArenaPoint,
    Reputation
};

constexpr auto MAX_VIP_LEVEL = 3;

class Vip
{
    Vip() = default;
    ~Vip() = default;

    Vip(Vip const&) = delete;
    Vip(Vip&&) = delete;
    Vip& operator= (Vip const&) = delete;
    Vip& operator= (Vip&&) = delete;

public:
    using WarheadVip = std::tuple<Seconds/*start*/, Seconds/*endtime*/, uint8/*level*/>;
    using WarheadVipRates = std::tuple<float/*XP*/, float/*Honor*/, float/*ArenaPoint*/, float/*Reputation*/>;

    static Vip* Instance();

    void LoadConfig();
    void InitSystem();

    inline bool IsEnable() { return _isEnable; }

    void Update(uint32 diff);
    bool Add(uint32 accountID, Seconds endTime, uint8 level, bool force = false);
    bool Delete(uint32 accountID);

    // For player targer
    void OnLoginPlayer(Player* player);
    void OnLogoutPlayer(Player* player);
    void UnSet(uint32 accountID);
    bool IsVip(Player* player);
    bool IsVip(uint32 accountID);
    uint8 GetLevel(Player* player);
    std::string GetDuration(Player* player);
    std::string GetDuration(uint32 accountID);
    void RemoveColldown(Player* player, uint32 spellID);
    void UnBindInstances(Player* player);
    void SendVipInfo(ChatHandler* handler, ObjectGuid targetGuid);
    void SendVipListRates(ChatHandler* handler);
    bool CanUsingVendor(Player* player, Creature* creature);

    // Creature
    bool IsVipVendor(uint32 entry);
    uint8 GetVendorVipLevel(uint32 entry);
    void AddVendorVipLevel(uint32 entry, uint8 vendorVipLevel);
    void DeleteVendorVipLevel(uint32 entry);

    template<VipRate>
    float GetRate(Player* player);

private:
    void LoadRates();
    void LoadAccounts();
    void LoadUnbinds();
    void LoadVipVendors();

    void LearnSpells(Player* player, uint8 vipLevel);
    void UnLearnSpells(Player* player, bool unlearnMount = true);

    Optional<WarheadVip> GetVipInfo(uint32 accountID);
    Optional<WarheadVipRates> GetVipRateInfo(uint8 vipLevel);
    Optional<Seconds> GetUndindTime(uint64 guid);
    Player* GetPlayerFromAccount(uint32 accountID);
    std::string GetDuration(Optional<WarheadVip> vipInfo);

    bool _isEnable{ false };
    Seconds _updateDelay{ 0s };
    uint8 _mountVipLevel{ 3 };
    uint32 _mountVipSpellID{ 0 };
    std::vector<uint32> _spellList;
    Seconds _unbindDuration{ 1_days };

    std::unordered_map<uint32/*acc id*/, WarheadVip> store;
    std::unordered_map<uint8/*level*/, WarheadVipRates> storeRates;
    std::unordered_map<uint64/*guid*/, Seconds/*unbindtime*/> storeUnbind;
    std::unordered_map<uint32/*creature entry*/, uint8/*vip level*/> storeVendors;

    TaskScheduler scheduler;
};

#define sVip Vip::Instance()

#endif /* _VIP_H_ */
