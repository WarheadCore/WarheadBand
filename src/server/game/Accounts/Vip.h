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
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class ChatHandler;
class ObjectGuid;
class Creature;
class WorldSession;
class VipQueryHolder;
class TempSummon;

using VipSpelLList = std::vector<uint32>;

struct VipLevelInfo
{
    uint32 Level{};
    uint32 MountSpell{};
    VipSpelLList LearnSpells;
    bool CanUseUnbindCommands{};
};

enum class VipRateType
{
    XP,
    Honor,
    ArenaPoint,
    Reputation,
    Profession,

    Max
};

constexpr std::size_t MAX_VIP_RATE_TYPE = static_cast<std::size_t>(VipRateType::Max);

struct VipRates
{
    uint32 VipLevel{};
    std::array<float, MAX_VIP_RATE_TYPE> RateValue{};

    float GetVipRate(VipRateType type) const;
};

struct VipInfo
{
    uint32 AccountID{};
    uint32 Level{};
    Seconds StartTime{};
    Seconds EndTime{};
};

class WH_GAME_API Vip
{
    Vip() = default;
    ~Vip() = default;

    Vip(Vip const&) = delete;
    Vip(Vip&&) = delete;
    Vip& operator=(Vip const&) = delete;
    Vip& operator=(Vip&&) = delete;

public:
    static Vip* Instance();

    void LoadConfig();
    void InitSystem();

    inline bool IsEnable() const { return _isEnable; }

    void Update(uint32 diff);
    bool Add(uint32 accountID, Seconds endTime, uint32 level, bool force = false);
    bool Delete(uint32 accountID);

    // For player target
    void OnLoginPlayer(Player* player);
    void OnLogoutPlayer(Player* player);
    void UnSet(uint32 accountID);
    bool IsVip(Player* player);
    bool IsVip(uint32 accountID);
    uint32 GetLevel(Player* player);
    std::string GetDuration(Player* player);
    std::string GetDuration(uint32 accountID, int8 locale = 0);
    void RemoveCooldown(Player* player, uint32 spellID);
    void UnBindInstances(Player* player);
    void SendVipInfo(ChatHandler* handler, ObjectGuid targetGuid);
    void SendVipListRates(ChatHandler* handler);
    bool CanUsingVendor(Player* player, Creature* creature);

    // Creature
    bool IsVipVendor(uint32 entry);
    uint32 GetVendorVipLevel(uint32 entry);
    void AddVendorVipLevel(uint32 entry, uint32 vendorVipLevel);
    void DeleteVendorVipLevel(uint32 entry);

    float GetRateForPlayer(Player* player, VipRateType rate);

    // Load system
    void LoadRates();
    void LoadVipVendors();
    void LoadVipLevels();

    // Info
    VipLevelInfo* GetVipLevelInfo(uint32 level);

    // Async load
    void LoadInfoForSession(VipQueryHolder const& holder);

    // Vip menu
    void SendVipMenu(Player* player);
    void VipMenuHandle(Player* player, uint32 menuID, uint32 action);

private:
    void LoadUnbinds();

    void LearnSpells(Player* player, uint32 vipLevel);
    void UnLearnSpells(Player* player, bool unlearnMount = true);
    void IterateVipSpellsForPlayer(Player* player, bool isLearn);

    VipInfo* GetVipInfo(uint32 accountID);
    VipRates* GetVipRates(uint32 vipLevel);
    Seconds* GetUnbindTime(uint64 guid);
    static Player* GetPlayerFromAccount(uint32 accountID);
    static std::string GetDuration(VipInfo* vipInfo, int8 locale = 0);

    TempSummon* GetTempSummon(Player* player);
    void AddTempSummon(Player* player, TempSummon* summon);
    void DeleteTempSummon(Player* player);

    bool _isEnable{};
    Seconds _updateDelay{};
    Seconds _unbindDuration{ 1_days };
    uint32 _maxLevel{};
    bool _isCommandBuffEnable{};

    std::unordered_map<uint32/*acc id*/, VipInfo> _store;
    std::unordered_map<uint32/*level*/, VipRates> _storeRates;
    std::unordered_map<uint64/*guid*/, Seconds/*unbindtime*/> _storeUnbind;
    std::unordered_map<uint32/*creature entry*/, uint32/*vip level*/> _storeVendors;
    std::unordered_map<uint32/*vip level*/, VipLevelInfo> _vipLevelsInfo;
    std::vector<uint32> _spellBuffs;

    std::unordered_map<Player*, TempSummon*> _tempSummons;

    TaskScheduler scheduler;
    std::mutex _mutex;
};

#define sVip Vip::Instance()

#endif /* _VIP_H_ */
