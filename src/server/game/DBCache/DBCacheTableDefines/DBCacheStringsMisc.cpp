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
    _queryStrings.emplace(DBCacheTable::ExplorationBasexp, "SELECT level, basexp FROM exploration_basexp");
    _queryStrings.emplace(DBCacheTable::PetNameGeneration, "SELECT word, entry, half FROM pet_name_generation");
    _queryStrings.emplace(DBCacheTable::CharacterPetMaxId, "SELECT MAX(id) FROM character_pet");
    _queryStrings.emplace(DBCacheTable::PetLevelstats, "SELECT creature_entry, level, hp, mana, str, agi, sta, inte, spi, armor, min_dmg, max_dmg FROM pet_levelstats");
    _queryStrings.emplace(DBCacheTable::MailLevelReward, "SELECT level, raceMask, mailTemplateId, senderEntry FROM mail_level_reward");
    _queryStrings.emplace(DBCacheTable::MailServerTemplate, "SELECT `id`, `reqLevel`, `reqPlayTime`, `moneyA`, `moneyH`, `itemA`, `itemCountA`, `itemH`,`itemCountH`, `subject`, `body`, `active` FROM `mail_server_template`");
}
