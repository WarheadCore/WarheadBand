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

}
