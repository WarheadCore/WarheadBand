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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ItemLevelUp.h"
#include "ScriptObject.h"

class ItemLevelUp_Item : public ItemScript
{
public:
    ItemLevelUp_Item() : ItemScript("ItemLevelUp_Item") {}

    bool OnUse(Player* player, Item* item, const SpellCastTargets& /*Targets*/) override
    {
        return sItemLevelUpMgr->OnPlayerItemUse(player, item);
    }
};

class ItemLevelUp_World : public WorldScript
{
public:
    ItemLevelUp_World() : WorldScript("ItemLevelUp_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sItemLevelUpMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sItemLevelUpMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_ItemLevelUp()
{
    new ItemLevelUp_Item();
    new ItemLevelUp_World();
}
