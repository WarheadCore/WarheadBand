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

void DBCacheMgr::InitSpellsStrings()
{
    _queryStrings.emplace(DBCacheTable::SpellRanks, "SELECT first_spell_id, spell_id, `rank` from spell_ranks ORDER BY first_spell_id, `rank`");
    _queryStrings.emplace(DBCacheTable::SpellCustomAttr, "SELECT spell_id, attributes FROM spell_custom_attr");
    _queryStrings.emplace(DBCacheTable::SpellRequired, "SELECT spell_id, req_spell from spell_required");
    _queryStrings.emplace(DBCacheTable::SpellGroup, "SELECT id, spell_id, special_flag FROM spell_group");
    _queryStrings.emplace(DBCacheTable::SpellProc, "SELECT SpellId, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, SpellFamilyMask2, ProcFlags, SpellTypeMask, SpellPhaseMask, HitMask, AttributesMask, ProcsPerMinute, Chance, Cooldown, Charges FROM spell_proc");
    _queryStrings.emplace(DBCacheTable::SpellProcEvent, "SELECT entry, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, SpellFamilyMask2, procFlags, procEx, procPhase, ppmRate, CustomChance, Cooldown FROM spell_proc_event");
    _queryStrings.emplace(DBCacheTable::SpellBonusData, "SELECT entry, direct_bonus, dot_bonus, ap_bonus, ap_dot_bonus FROM spell_bonus_data");
    _queryStrings.emplace(DBCacheTable::SpellThreat, "SELECT entry, flatMod, pctMod, apPctMod FROM spell_threat");
    _queryStrings.emplace(DBCacheTable::SpellMixology, "SELECT entry, pctMod FROM spell_mixology");
    _queryStrings.emplace(DBCacheTable::SpellGroupStackRules, "SELECT group_id, stack_rule FROM spell_group_stack_rules");
    _queryStrings.emplace(DBCacheTable::SpellEnchantProcData, "SELECT entry, customChance, PPMChance, procEx, attributeMask FROM spell_enchant_proc_data");
    _queryStrings.emplace(DBCacheTable::SpellArea, "SELECT spell, area, quest_start, quest_start_status, quest_end_status, quest_end, aura_spell, racemask, gender, autocast FROM spell_area");
    _queryStrings.emplace(DBCacheTable::SpellPetAuras, "SELECT spell, effectId, pet, aura FROM spell_pet_auras");
    _queryStrings.emplace(DBCacheTable::SpellTargetPosition, "SELECT ID, EffectIndex, MapID, PositionX, PositionY, PositionZ, Orientation FROM spell_target_position");
    _queryStrings.emplace(DBCacheTable::SpellLinkedSpell, "SELECT spell_trigger, spell_effect, type FROM spell_linked_spell");
}