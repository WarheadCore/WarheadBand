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

void DBCacheMgr::InitMiscStrings()
{
    _queryStrings.emplace(DBCacheTable::ReputationRewardRate, "SELECT faction, quest_rate, quest_daily_rate, quest_weekly_rate, quest_monthly_rate, quest_repeatable_rate, creature_rate, spell_rate FROM reputation_reward_rate");
    _queryStrings.emplace(DBCacheTable::ReputationSpilloverTemplate, "SELECT faction, faction1, rate_1, rank_1, faction2, rate_2, rank_2, faction3, rate_3, rank_3, faction4, rate_4, rank_4 FROM reputation_spillover_template");
    _queryStrings.emplace(DBCacheTable::PointsOfInterest, "SELECT ID, PositionX, PositionY, Icon, Flags, Importance, Name FROM points_of_interest");
    _queryStrings.emplace(DBCacheTable::LinkedRespawn, "SELECT guid, linkedGuid, linkType FROM linked_respawn ORDER BY guid ASC");
    _queryStrings.emplace(DBCacheTable::GameWeather, "SELECT zone, spring_rain_chance, spring_snow_chance, spring_storm_chance,"
                                                       "summer_rain_chance, summer_snow_chance, summer_storm_chance,"
                                                       "fall_rain_chance, fall_snow_chance, fall_storm_chance,"
                                                       "winter_rain_chance, winter_snow_chance, winter_storm_chance,"
                                                       "ScriptName FROM game_weather");
    _queryStrings.emplace(DBCacheTable::VehicleAccessory, "SELECT `guid`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_accessory`");
    _queryStrings.emplace(DBCacheTable::VehicleTemplateAccessory, "SELECT `entry`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_template_accessory`");
    _queryStrings.emplace(DBCacheTable::Areatrigger, "SELECT entry, map, x, y, z, radius, length, width, height, orientation FROM areatrigger");
    _queryStrings.emplace(DBCacheTable::AreatriggerTeleport, "SELECT ID, target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM areatrigger_teleport");
    _queryStrings.emplace(DBCacheTable::AreatriggerInvolvedrelation, "SELECT id, quest FROM areatrigger_involvedrelation");
    _queryStrings.emplace(DBCacheTable::AreatriggerTavern, "SELECT id, faction FROM areatrigger_tavern");
    _queryStrings.emplace(DBCacheTable::LfgDungeonTemplate, "SELECT dungeonId, position_x, position_y, position_z, orientation FROM lfg_dungeon_template");
    _queryStrings.emplace(DBCacheTable::LfgDungeonRewards, "SELECT dungeonId, maxLevel, firstQuestId, otherQuestId FROM lfg_dungeon_rewards ORDER BY dungeonId, maxLevel ASC");
    _queryStrings.emplace(DBCacheTable::GameGraveyard, "SELECT ID, Map, x, y, z, Comment FROM game_graveyard");
    _queryStrings.emplace(DBCacheTable::GraveyardZone, "SELECT ID, GhostZone, Faction FROM graveyard_zone");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfo, "SELECT race, class, map, zone, position_x, position_y, position_z, orientation FROM playercreateinfo");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfoItem, "SELECT race, class, itemid, amount FROM playercreateinfo_item");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfoSkills, "SELECT raceMask, classMask, skill, `rank` FROM playercreateinfo_skills");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfoSpellCustom, "SELECT racemask, classmask, Spell FROM playercreateinfo_spell_custom");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfoCastSpell, "SELECT raceMask, classMask, spell FROM playercreateinfo_cast_spell");
    _queryStrings.emplace(DBCacheTable::PlayerCreateInfoAction, "SELECT race, class, button, action, type FROM playercreateinfo_action");
    _queryStrings.emplace(DBCacheTable::PlayerRaceStats, "SELECT Race, Strength, Agility, Stamina, Intellect, Spirit FROM player_race_stats");
    _queryStrings.emplace(DBCacheTable::PlayerClassStats, "SELECT Class, Level, Strength, Agility, Stamina, Intellect, Spirit, BaseHP, BaseMana FROM player_class_stats");
    _queryStrings.emplace(DBCacheTable::PlayerXpForLevel, "SELECT Level, Experience FROM player_xp_for_level");
    _queryStrings.emplace(DBCacheTable::ExplorationBasexp, "SELECT level, basexp FROM exploration_basexp");
    _queryStrings.emplace(DBCacheTable::PetNameGeneration, "SELECT word, entry, half FROM pet_name_generation");
    _queryStrings.emplace(DBCacheTable::PetLevelstats, "SELECT creature_entry, level, hp, mana, str, agi, sta, inte, spi, armor, min_dmg, max_dmg FROM pet_levelstats");
    _queryStrings.emplace(DBCacheTable::MailLevelReward, "SELECT level, raceMask, mailTemplateId, senderEntry FROM mail_level_reward");
    _queryStrings.emplace(DBCacheTable::SkillDiscoveryTemplate, "SELECT spellId, reqSpell, reqSkillValue, chance FROM skill_discovery_template");
    _queryStrings.emplace(DBCacheTable::SkillExtraItemTemplate, "SELECT spellId, requiredSpecialization, additionalCreateChance, additionalMaxNum FROM skill_extra_item_template");
    _queryStrings.emplace(DBCacheTable::SkillPerfectItemTemplate, "SELECT spellId, requiredSpecialization, perfectCreateChance, perfectItemType FROM skill_perfect_item_template");
    _queryStrings.emplace(DBCacheTable::SkillFishingBaseLevel, "SELECT entry, skill FROM skill_fishing_base_level");
    _queryStrings.emplace(DBCacheTable::AchievementCriteriaData, "SELECT criteria_id, type, value1, value2, ScriptName FROM achievement_criteria_data");
    _queryStrings.emplace(DBCacheTable::AchievementReward, "SELECT ID, TitleA, TitleH, ItemID, Sender, Subject, Body, MailTemplateID FROM achievement_reward");
    _queryStrings.emplace(DBCacheTable::BattlemasterEntry, "SELECT entry, bg_template FROM battlemaster_entry");
    _queryStrings.emplace(DBCacheTable::GameTele, "SELECT id, position_x, position_y, position_z, orientation, map, name FROM game_tele");
    _queryStrings.emplace(DBCacheTable::WaypointData, "SELECT id, point, position_x, position_y, position_z, orientation, move_type, delay, action, action_chance FROM waypoint_data ORDER BY id, point");
    _queryStrings.emplace(DBCacheTable::Conditions, "SELECT SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, "
                                                    " ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId, ScriptName FROM conditions");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeAchievement, "SELECT alliance_id, horde_id FROM player_factionchange_achievement");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeSpells, "SELECT alliance_id, horde_id FROM player_factionchange_spells");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeItems, "SELECT alliance_id, horde_id FROM player_factionchange_items");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeReputations, "SELECT alliance_id, horde_id FROM player_factionchange_reputations");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeTitles, "SELECT alliance_id, horde_id FROM player_factionchange_titles");
    _queryStrings.emplace(DBCacheTable::PlayerFactionchangeQuest, "SELECT alliance_id, horde_id FROM player_factionchange_quests");
    _queryStrings.emplace(DBCacheTable::BattlegroundTemplate, "SELECT ID, MinPlayersPerTeam, MaxPlayersPerTeam, MinLvl, MaxLvl, AllianceStartLoc, AllianceStartO, HordeStartLoc, HordeStartO, StartMaxDist, Weight, ScriptName FROM battleground_template");
    _queryStrings.emplace(DBCacheTable::OutdoorpvpTemplate, "SELECT TypeId, ScriptName FROM outdoorpvp_template");
    _queryStrings.emplace(DBCacheTable::Disables, "SELECT sourceType, entry, flags, params_0, params_1 FROM disables");
    _queryStrings.emplace(DBCacheTable::InstanceTemplate, "SELECT map, parent, script, allowMount FROM instance_template");
    _queryStrings.emplace(DBCacheTable::InstanceEncounters, "SELECT entry, creditType, creditEntry, lastEncounterDungeon FROM instance_encounters");
}