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

#ifndef _VIP_H_
#define _VIP_H_

#include "Define.h"
#include <string>

class Player;
class ChatHandler;
class ObjectGuid;

enum class VipRate
{
    XP,
    Honor,
    ArenaPoint,
    Reputation
};

class Vip
{
    Vip() = default;
    ~Vip() = default;

    Vip(Vip const&) = delete;
    Vip(Vip&&) = delete;
    Vip& operator= (Vip const&) = delete;
    Vip& operator= (Vip&&) = delete;

public:
    static Vip* Instance();

    void InitSystem(bool reload);

    void Update(uint32 diff);
    bool Add(uint32 accountID, int64 endTime, uint8 level, bool force = false);
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

    template<VipRate>
    float GetRate(Player* player);

private:
    void LoadRates();
    void LoadAccounts();
    void LoadUnbinds();

    void LearnSpells(Player* player);
    void UnLearnSpells(Player* player, bool unlearnMount = true);
};

#define sVip Vip::Instance()

#endif /* _VIP_H_ */
