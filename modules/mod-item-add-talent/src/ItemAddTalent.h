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

#ifndef _ITEM_ADD_TALENT_H_
#define _ITEM_ADD_TALENT_H_

#include "Define.h"

class Player;
class Item;

class ItemAddTalentMgr
{
public:
    static ItemAddTalentMgr* instance();

    void LoadConfig(bool reload);
    bool OnPlayerItemUse(Player* player, Item* item);

private:
    bool _isEnable{ false };

    ItemAddTalentMgr() = default;
    ~ItemAddTalentMgr() = default;

    ItemAddTalentMgr(ItemAddTalentMgr const&) = delete;
    ItemAddTalentMgr(ItemAddTalentMgr&&) = delete;
    ItemAddTalentMgr& operator=(ItemAddTalentMgr const&) = delete;
    ItemAddTalentMgr& operator=(ItemAddTalentMgr&&) = delete;
};

#define sItemAddTalentMgr ItemAddTalentMgr::instance()

#endif // _ITEM_ADD_TALENT_H_