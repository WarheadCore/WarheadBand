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

#include "DBCacheMgr.h"

void DBCacheMgr::InitItemStrings()
{
    _queryStrings.emplace(DBCacheTable::ItemEnchantmentTemplate, "SELECT entry, ench, chance FROM item_enchantment_template");
    _queryStrings.emplace(DBCacheTable::ItemTemplate, "SELECT entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, FlagsExtra, BuyCount, BuyPrice, SellPrice, InventoryType, "
                                                      "AllowableClass, AllowableRace, ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell, requiredhonorrank, "
                                                      "RequiredCityRank, RequiredReputationFaction, RequiredReputationRank, maxcount, stackable, ContainerSlots, StatsCount, stat_type1, "
                                                      "stat_value1, stat_type2, stat_value2, stat_type3, stat_value3, stat_type4, stat_value4, stat_type5, stat_value5, stat_type6, "
                                                      "stat_value6, stat_type7, stat_value7, stat_type8, stat_value8, stat_type9, stat_value9, stat_type10, stat_value10, "
                                                      "ScalingStatDistribution, ScalingStatValue, dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2, armor, holy_res, fire_res, "
                                                      "nature_res, frost_res, shadow_res, arcane_res, delay, ammo_type, RangedModRange, spellid_1, spelltrigger_1, spellcharges_1, "
                                                      "spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1, spellid_2, spelltrigger_2, spellcharges_2, "
                                                      "spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2, spellid_3, spelltrigger_3, spellcharges_3, "
                                                      "spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3, spellid_4, spelltrigger_4, spellcharges_4, "
                                                      "spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4, spellid_5, spelltrigger_5, spellcharges_5, "
                                                      "spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5, bonding, description, PageText, LanguageID, PageMaterial, "
                                                      "startquest, lockid, Material, sheath, RandomProperty, RandomSuffix, block, itemset, MaxDurability, area, Map, BagFamily, "
                                                      "TotemCategory, socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3, socketBonus, "
                                                      "GemProperties, RequiredDisenchantSkill, ArmorDamageModifier, duration, ItemLimitCategory, HolidayId, ScriptName, DisenchantID, "
                                                      "FoodType, minMoneyLoot, maxMoneyLoot, flagsCustom FROM item_template");
    _queryStrings.emplace(DBCacheTable::ItemSetNames, "SELECT `entry`, `name`, `InventoryType` FROM `item_set_names`");
}
