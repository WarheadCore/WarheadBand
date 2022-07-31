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

#ifndef _ITEM_LEVEL_UP_H_
#define _ITEM_LEVEL_UP_H_

#include "Define.h"
#include <memory>

class Item;
class Player;
class WorldLocation;

class ItemLevelUpMgr
{
public:
    static ItemLevelUpMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);

    // Hook
    bool OnPlayerItemUse(Player* player, Item* item);

private:
    void PlayerTeleport(Player* player);
    void SetMaxWeaponSkills(Player* player);

    bool _isEnable{ false };
    bool _isSaveExpEnable{ false };
    bool _isMaxSkillsEnable{ false };
    bool _isTeleportEnable{ false };
    uint8 _maxConfigLevel{ 80 };
    uint8 _levelUpCount{ 1 };
    std::unique_ptr<WorldLocation> _location;

    ItemLevelUpMgr() = default;
    ~ItemLevelUpMgr() = default;

    ItemLevelUpMgr(ItemLevelUpMgr const&) = delete;
    ItemLevelUpMgr(ItemLevelUpMgr&&) = delete;
    ItemLevelUpMgr& operator=(ItemLevelUpMgr const&) = delete;
    ItemLevelUpMgr& operator=(ItemLevelUpMgr&&) = delete;
};

#define sItemLevelUpMgr ItemLevelUpMgr::instance()

#endif // _ITEM_LEVEL_UP_H_
