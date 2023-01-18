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

#include "ItemTemplate.h"
#include "Define.h"
#include "SmartEnum.h"
#include <stdexcept>

namespace Warhead::Impl::EnumUtilsImpl
{

/****************************************************************\
|* data for enum 'ItemClass' in 'ItemTemplate.h' auto-generated *|
\****************************************************************/
template <>
WH_API_EXPORT EnumText EnumUtils<ItemClass>::ToString(ItemClass value)
{
    switch (value)
    {
        case ITEM_CLASS_CONSUMABLE: return { "ITEM_CLASS_CONSUMABLE", "ITEM_CLASS_CONSUMABLE", "" };
        case ITEM_CLASS_CONTAINER: return { "ITEM_CLASS_CONTAINER", "ITEM_CLASS_CONTAINER", "" };
        case ITEM_CLASS_WEAPON: return { "ITEM_CLASS_WEAPON", "ITEM_CLASS_WEAPON", "" };
        case ITEM_CLASS_GEM: return { "ITEM_CLASS_GEM", "ITEM_CLASS_GEM", "" };
        case ITEM_CLASS_ARMOR: return { "ITEM_CLASS_ARMOR", "ITEM_CLASS_ARMOR", "" };
        case ITEM_CLASS_REAGENT: return { "ITEM_CLASS_REAGENT", "ITEM_CLASS_REAGENT", "" };
        case ITEM_CLASS_PROJECTILE: return { "ITEM_CLASS_PROJECTILE", "ITEM_CLASS_PROJECTILE", "" };
        case ITEM_CLASS_TRADE_GOODS: return { "ITEM_CLASS_TRADE_GOODS", "ITEM_CLASS_TRADE_GOODS", "" };
        case ITEM_CLASS_GENERIC: return { "ITEM_CLASS_GENERIC", "ITEM_CLASS_GENERIC", "" };
        case ITEM_CLASS_RECIPE: return { "ITEM_CLASS_RECIPE", "ITEM_CLASS_RECIPE", "" };
        case ITEM_CLASS_MONEY: return { "ITEM_CLASS_MONEY", "ITEM_CLASS_MONEY", "" };
        case ITEM_CLASS_QUIVER: return { "ITEM_CLASS_QUIVER", "ITEM_CLASS_QUIVER", "" };
        case ITEM_CLASS_QUEST: return { "ITEM_CLASS_QUEST", "ITEM_CLASS_QUEST", "" };
        case ITEM_CLASS_KEY: return { "ITEM_CLASS_KEY", "ITEM_CLASS_KEY", "" };
        case ITEM_CLASS_PERMANENT: return { "ITEM_CLASS_PERMANENT", "ITEM_CLASS_PERMANENT", "" };
        case ITEM_CLASS_MISC: return { "ITEM_CLASS_MISC", "ITEM_CLASS_MISC", "" };
        case ITEM_CLASS_GLYPH: return { "ITEM_CLASS_GLYPH", "ITEM_CLASS_GLYPH", "" };
        default: throw std::out_of_range("value");
    }
}

template <>
WH_API_EXPORT size_t EnumUtils<ItemClass>::Count() { return 17; }

template <>
WH_API_EXPORT ItemClass EnumUtils<ItemClass>::FromIndex(size_t index)
{
    switch (index)
    {
        case 0: return ITEM_CLASS_CONSUMABLE;
        case 1: return ITEM_CLASS_CONTAINER;
        case 2: return ITEM_CLASS_WEAPON;
        case 3: return ITEM_CLASS_GEM;
        case 4: return ITEM_CLASS_ARMOR;
        case 5: return ITEM_CLASS_REAGENT;
        case 6: return ITEM_CLASS_PROJECTILE;
        case 7: return ITEM_CLASS_TRADE_GOODS;
        case 8: return ITEM_CLASS_GENERIC;
        case 9: return ITEM_CLASS_RECIPE;
        case 10: return ITEM_CLASS_MONEY;
        case 11: return ITEM_CLASS_QUIVER;
        case 12: return ITEM_CLASS_QUEST;
        case 13: return ITEM_CLASS_KEY;
        case 14: return ITEM_CLASS_PERMANENT;
        case 15: return ITEM_CLASS_MISC;
        case 16: return ITEM_CLASS_GLYPH;
        default: throw std::out_of_range("index");
    }
}

template <>
WH_API_EXPORT size_t EnumUtils<ItemClass>::ToIndex(ItemClass value)
{
    switch (value)
    {
        case ITEM_CLASS_CONSUMABLE: return 0;
        case ITEM_CLASS_CONTAINER: return 1;
        case ITEM_CLASS_WEAPON: return 2;
        case ITEM_CLASS_GEM: return 3;
        case ITEM_CLASS_ARMOR: return 4;
        case ITEM_CLASS_REAGENT: return 5;
        case ITEM_CLASS_PROJECTILE: return 6;
        case ITEM_CLASS_TRADE_GOODS: return 7;
        case ITEM_CLASS_GENERIC: return 8;
        case ITEM_CLASS_RECIPE: return 9;
        case ITEM_CLASS_MONEY: return 10;
        case ITEM_CLASS_QUIVER: return 11;
        case ITEM_CLASS_QUEST: return 12;
        case ITEM_CLASS_KEY: return 13;
        case ITEM_CLASS_PERMANENT: return 14;
        case ITEM_CLASS_MISC: return 15;
        case ITEM_CLASS_GLYPH: return 16;
        default: throw std::out_of_range("value");
    }
}
}
