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

void DBCacheMgr::InitGameEventStrings()
{
    _queryStrings.emplace(DBCacheTable::GameEventMaxEvent, "SELECT MAX(eventEntry) FROM game_event");
    _queryStrings.emplace(DBCacheTable::HolidayDates, "SELECT id, date_id, date_value, holiday_duration FROM holiday_dates");
    _queryStrings.emplace(DBCacheTable::GameEvent, "SELECT eventEntry, UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(end_time), occurence, length, holiday, holidayStage, description, world_event, announce FROM game_event");
    _queryStrings.emplace(DBCacheTable::GameEventSave, "SELECT eventEntry, state, next_start FROM game_event_save");
    _queryStrings.emplace(DBCacheTable::GameEventPrerequisite, "SELECT eventEntry, prerequisite_event FROM game_event_prerequisite");
    _queryStrings.emplace(DBCacheTable::GameEventCreature, "SELECT guid, eventEntry FROM game_event_creature");
    _queryStrings.emplace(DBCacheTable::GameEventGameobject, "SELECT guid, eventEntry FROM game_event_gameobject");
    _queryStrings.emplace(DBCacheTable::GameEventModelEquip, "SELECT creature.guid, creature.id1, creature.id2, creature.id3, game_event_model_equip.eventEntry, game_event_model_equip.modelid, game_event_model_equip.equipment_id "
                                                             "FROM creature JOIN game_event_model_equip ON creature.guid=game_event_model_equip.guid");
    _queryStrings.emplace(DBCacheTable::GameEventCreatureQuest, "SELECT id, quest, eventEntry FROM game_event_creature_quest");
    _queryStrings.emplace(DBCacheTable::GameEventGameobjectQuest, "SELECT id, quest, eventEntry FROM game_event_gameobject_quest");
    _queryStrings.emplace(DBCacheTable::GameEventQuestCondition, "SELECT quest, eventEntry, condition_id, num FROM game_event_quest_condition");
    _queryStrings.emplace(DBCacheTable::GameEventCondition, "SELECT eventEntry, condition_id, req_num, max_world_state_field, done_world_state_field FROM game_event_condition");
    _queryStrings.emplace(DBCacheTable::GameEventConditionSave, "SELECT eventEntry, condition_id, done FROM game_event_condition_save");
    _queryStrings.emplace(DBCacheTable::GameEventNpcFlag, "SELECT guid, eventEntry, npcflag FROM game_event_npcflag");
    _queryStrings.emplace(DBCacheTable::GameEventSeasonalQuestrelation, "SELECT questId, eventEntry FROM game_event_seasonal_questrelation");
    _queryStrings.emplace(DBCacheTable::GameEventNpcVendor, "SELECT eventEntry, guid, item, maxcount, incrtime, ExtendedCost FROM game_event_npc_vendor ORDER BY guid, slot ASC");
    _queryStrings.emplace(DBCacheTable::GameEventBattlegroundHoliday, "SELECT eventEntry, bgflag FROM game_event_battleground_holiday");
    _queryStrings.emplace(DBCacheTable::GameEventPool, "SELECT pool_template.entry, game_event_pool.eventEntry FROM pool_template"
                                                       " JOIN game_event_pool ON pool_template.entry = game_event_pool.pool_entry");
}
