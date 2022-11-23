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

void DBCacheMgr::InitScriptStrings()
{
    _queryStrings.emplace(DBCacheTable::ScriptNames, "SELECT DISTINCT(ScriptName) FROM achievement_criteria_data WHERE ScriptName <> '' AND type = 11 "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM battleground_template WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM creature WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM gameobject WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM areatrigger_scripts WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM spell_script_names WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM transports WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM game_weather WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM conditions WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(ScriptName) FROM outdoorpvp_template WHERE ScriptName <> '' "
                                                     "UNION "
                                                     "SELECT DISTINCT(script) FROM instance_template WHERE script <> ''");
    _queryStrings.emplace(DBCacheTable::AreatriggerScripts, "SELECT entry, ScriptName FROM areatrigger_scripts");
    _queryStrings.emplace(DBCacheTable::SpellScriptNames, "SELECT spell_id, ScriptName FROM spell_script_names");
}
