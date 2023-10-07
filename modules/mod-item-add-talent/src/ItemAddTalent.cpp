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

#include "ItemAddTalent.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "Item.h"

ItemAddTalentMgr* ItemAddTalentMgr::instance()
{
    static ItemAddTalentMgr instance;
    return &instance;
}

void ItemAddTalentMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("ItemAddTalent.Enable");
}

bool ItemAddTalentMgr::OnPlayerItemUse(Player* player, Item* item)
{
    if (!_isEnable)
        return false;

    player->RewardExtraBonusTalentPoints(1);
    player->InitTalentForLevel();
    player->DestroyItemCount(item->GetEntry(), 1, true);
    return true;
}