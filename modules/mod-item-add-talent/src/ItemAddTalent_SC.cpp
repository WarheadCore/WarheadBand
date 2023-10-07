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
#include "ScriptObject.h"

class ItemAddTalent_Item : public ItemScript
{
public:
    ItemAddTalent_Item() : ItemScript("ItemAddTalent_Item") {}

    bool OnUse(Player* player, Item* item, const SpellCastTargets& /*Targets*/) override
    {
        return sItemAddTalentMgr->OnPlayerItemUse(player, item);
    }
};

class ItemAddTalent_World : public WorldScript
{
public:
    ItemAddTalent_World() : WorldScript("ItemAddTalent_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sItemAddTalentMgr->LoadConfig(reload);
    }
};

// Group all custom scripts
void AddSC_ItemAddTalent()
{
    new ItemAddTalent_Item();
    new ItemAddTalent_World();
}