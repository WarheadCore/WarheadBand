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

void DBCacheMgr::InitCreatureStrings()
{
    _queryStrings.emplace(DBCacheTable::CreatureModelInfo, "SELECT DisplayID, BoundingRadius, CombatReach, Gender, DisplayID_Other_Gender FROM creature_model_info");
    _queryStrings.emplace(DBCacheTable::CreatureTemplate, "SELECT entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, "
                                                          "modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction, npcflag, speed_walk, speed_run, speed_swim, speed_flight, "
                                                          "detection_range, scale, `rank`, dmgschool, DamageModifier, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, "
                                                          "dynamicflags, family, trainer_type, trainer_spell, trainer_class, trainer_race, type, "
                                                          "type_flags, lootid, pickpocketloot, skinloot, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, "
                                                          "ctm.Ground, ctm.Swim, ctm.Flight, ctm.Rooted, ctm.Chase, ctm.Random, ctm.InteractionPauseTimer, HoverHeight, HealthModifier, ManaModifier, ArmorModifier, ExperienceModifier, "
                                                          "RacialLeader, movementId, RegenHealth, mechanic_immune_mask, spell_school_immune_mask, flags_extra, ScriptName "
                                                          "FROM creature_template ct LEFT JOIN creature_template_movement ctm ON ct.entry = ctm.CreatureId");
    _queryStrings.emplace(DBCacheTable::CreatureEquipTemplate, "SELECT CreatureID, ID, ItemID1, ItemID2, ItemID3 FROM creature_equip_template");
    _queryStrings.emplace(DBCacheTable::CreatureTemplateAddon, "SELECT entry, path_id, mount, bytes1, bytes2, emote, visibilityDistanceType, auras FROM creature_template_addon");
    _queryStrings.emplace(DBCacheTable::CreatureOnkillReputation, "SELECT creature_id, RewOnKillRepFaction1, RewOnKillRepFaction2, "
                                                              "IsTeamAward1, MaxStanding1, RewOnKillRepValue1, IsTeamAward2, MaxStanding2, RewOnKillRepValue2, TeamDependent "
                                                              "FROM creature_onkill_reputation");
    _queryStrings.emplace(DBCacheTable::CreatureClassLevelStats, "SELECT level, class, basehp0, basehp1, basehp2, basemana, basearmor, attackpower, rangedattackpower, damage_base, damage_exp1, damage_exp2 FROM creature_classlevelstats");
    _queryStrings.emplace(DBCacheTable::Creature, "SELECT creature.guid, id1, id2, id3, map, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, "
                                                                 "currentwaypoint, curhealth, curmana, MovementType, spawnMask, phaseMask, eventEntry, pool_entry, creature.npcflag, creature.unit_flags, creature.dynamicflags, "
                                                                 "creature.ScriptName "
                                                                 "FROM creature "
                                                                 "LEFT OUTER JOIN game_event_creature ON creature.guid = game_event_creature.guid "
                                                                 "LEFT OUTER JOIN pool_creature ON creature.guid = pool_creature.guid");
    _queryStrings.emplace(DBCacheTable::CreatureSummonGroups, "SELECT summonerId, summonerType, groupId, entry, position_x, position_y, position_z, orientation, summonType, summonTime FROM creature_summon_groups");
    _queryStrings.emplace(DBCacheTable::CreatureAddon, "SELECT guid, path_id, mount, bytes1, bytes2, emote, visibilityDistanceType, auras FROM creature_addon");
    _queryStrings.emplace(DBCacheTable::CreatureMovementOverride, "SELECT cmo.SpawnId,"
                                                                  "COALESCE(cmo.Ground, ctm.Ground),"
                                                                  "COALESCE(cmo.Swim, ctm.Swim),"
                                                                  "COALESCE(cmo.Flight, ctm.Flight),"
                                                                  "COALESCE(cmo.Rooted, ctm.Rooted),"
                                                                  "COALESCE(cmo.Chase, ctm.Chase),"
                                                                  "COALESCE(cmo.Random, ctm.Random),"
                                                                  "COALESCE(cmo.InteractionPauseTimer, ctm.InteractionPauseTimer) "
                                                                  "FROM creature_movement_override AS cmo "
                                                                  "LEFT JOIN creature AS c ON c.guid = cmo.SpawnId "
                                                                  "LEFT JOIN creature_template_movement AS ctm ON ctm.CreatureId = c.id1");
    _queryStrings.emplace(DBCacheTable::CreatureQuestItem, "SELECT CreatureEntry, ItemId FROM creature_questitem ORDER BY Idx ASC");
    _queryStrings.emplace(DBCacheTable::NpcSpellClickSpells, "SELECT npc_entry, spell_id, cast_flags, user_type FROM npc_spellclick_spells");
    _queryStrings.emplace(DBCacheTable::CreatureFormations, "SELECT leaderGUID, memberGUID, dist, angle, groupAI, point_1, point_2 FROM creature_formations ORDER BY leaderGUID");
}
