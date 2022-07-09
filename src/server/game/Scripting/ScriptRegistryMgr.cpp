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

#include "ScriptRegistryMgr.h"
#include "ScriptObject.h"
#include "Containers.h"
#include "Log.h"
#include "ScriptMgr.h"

template<class T>
void ScriptRegistryMgr<T>::AddScript(T* const script)
{
    ASSERT(script);

    if (!CheckSameMemory(script))
        return;

    if (script->isAfterLoadScript())
    {
        _scriptAfterLoadDBList.emplace_back(std::unique_ptr<T>(script));
        return;
    }

    script->checkValidity();

    LOG_INFO("server", "> Add base script {}", script->GetName());

    // We're dealing with a code-only script; just add it.
    _scriptPointerList.emplace(_scriptIdCounter++, std::unique_ptr<T>(script));
    sScriptMgr->IncrementScriptCount();
}

template<class T>
void ScriptRegistryMgr<T>::AddAfterLoadDBScripts()
{
    for (auto const& afterLoadDBScripts : _scriptAfterLoadDBList)
    {
        afterLoadDBScripts->checkValidity();

        if (afterLoadDBScripts->IsDatabaseBound())
        {
            if (!CheckSameMemory(afterLoadDBScripts.get()))
            {
                return;
            }

            // Get an ID for the script. An ID only exists if it's a script that is assigned in the database
            // through a script name (or similar).
            uint32 id = sObjectMgr->GetScriptId(afterLoadDBScripts->GetName());
            if (id)
            {
                // Try to find an existing script.
                for (auto const& [scriptID, script] : _scriptPointerList)
                {
                    // If the script names match...
                    if (script->GetName() == afterLoadDBScripts->GetName())
                    {
                        // ... It exists.
                        _scriptPointerList.erase(scriptID);
                        break;
                    }
                }

                // Assign new script!
                _scriptPointerList.emplace(id, std::unique_ptr<T>(afterLoadDBScripts.get()));
                LOG_INFO("server", "> Add DatabaseBound script {}", script->GetName());

                // Increment script count only with new scripts
                sScriptMgr->IncrementScriptCount();
            }
            else
            {
                // The script uses a script name from database, but isn't assigned to anything.
                if (afterLoadDBScripts->GetName().find("Smart") == std::string::npos)
                    LOG_ERROR("sql.sql", "Script named '{}' is not assigned in the database.",
                        afterLoadDBScripts->GetName());
            }
        }
        else
        {
            // We're dealing with a code-only script; just add it.
            _scriptPointerList.emplace(_scriptIdCounter++, std::unique_ptr<T>(afterLoadDBScripts.get()));
            LOG_INFO("server", "> Add non DatabaseBound script {}", script->GetName());
            sScriptMgr->IncrementScriptCount();
        }
    }
}

// Gets a script by its ID (assigned by ObjectMgr)
template<class T>
T* ScriptRegistryMgr<T>::GetScriptById(uint32 id)
{
    auto const& itr = _scriptPointerList.find(id);
    if (itr == _scriptPointerList.end())
        return nullptr;

    return itr->second.get();
}

// See if the script is using the same memory as another script. If this happens, it means that
// someone forgot to allocate new memory for a script
template<class T>
bool ScriptRegistryMgr<T>::CheckSameMemory(T* const script)
{
    // See if the script is using the same memory as another script. If this happens, it means that
    // someone forgot to allocate new memory for a script.
    for (auto const& [scriptID, _script] : _scriptPointerList)
    {
        if (_script.get() == script)
        {
            LOG_ERROR("scripts", "Script '{}' has same memory pointer as '{}'.",
                script->GetName(), _script->GetName());

            return false;
        }
    }

    return true;
}

template class WH_GAME_API ScriptRegistryMgr<AccountScript>;
template class WH_GAME_API ScriptRegistryMgr<AchievementCriteriaScript>;
template class WH_GAME_API ScriptRegistryMgr<AchievementScript>;
template class WH_GAME_API ScriptRegistryMgr<AllCreatureScript>;
template class WH_GAME_API ScriptRegistryMgr<AllGameObjectScript>;
template class WH_GAME_API ScriptRegistryMgr<AllItemScript>;
template class WH_GAME_API ScriptRegistryMgr<AllMapScript>;
template class WH_GAME_API ScriptRegistryMgr<AreaTriggerScript>;
template class WH_GAME_API ScriptRegistryMgr<ArenaScript>;
template class WH_GAME_API ScriptRegistryMgr<ArenaTeamScript>;
template class WH_GAME_API ScriptRegistryMgr<AuctionHouseScript>;
template class WH_GAME_API ScriptRegistryMgr<BGScript>;
template class WH_GAME_API ScriptRegistryMgr<BattlegroundMapScript>;
template class WH_GAME_API ScriptRegistryMgr<BattlegroundScript>;
template class WH_GAME_API ScriptRegistryMgr<CommandSC>;
template class WH_GAME_API ScriptRegistryMgr<CommandScript>;
template class WH_GAME_API ScriptRegistryMgr<ConditionScript>;
template class WH_GAME_API ScriptRegistryMgr<CreatureScript>;
template class WH_GAME_API ScriptRegistryMgr<DatabaseScript>;
template class WH_GAME_API ScriptRegistryMgr<DynamicObjectScript>;
template class WH_GAME_API ScriptRegistryMgr<ElunaScript>;
template class WH_GAME_API ScriptRegistryMgr<FormulaScript>;
template class WH_GAME_API ScriptRegistryMgr<GameEventScript>;
template class WH_GAME_API ScriptRegistryMgr<GameObjectScript>;
template class WH_GAME_API ScriptRegistryMgr<GlobalScript>;
template class WH_GAME_API ScriptRegistryMgr<GroupScript>;
template class WH_GAME_API ScriptRegistryMgr<GuildScript>;
template class WH_GAME_API ScriptRegistryMgr<InstanceMapScript>;
template class WH_GAME_API ScriptRegistryMgr<ItemScript>;
template class WH_GAME_API ScriptRegistryMgr<LootScript>;
template class WH_GAME_API ScriptRegistryMgr<MailScript>;
template class WH_GAME_API ScriptRegistryMgr<MiscScript>;
template class WH_GAME_API ScriptRegistryMgr<ModuleScript>;
template class WH_GAME_API ScriptRegistryMgr<MovementHandlerScript>;
template class WH_GAME_API ScriptRegistryMgr<OutdoorPvPScript>;
template class WH_GAME_API ScriptRegistryMgr<PetScript>;
template class WH_GAME_API ScriptRegistryMgr<PlayerScript>;
template class WH_GAME_API ScriptRegistryMgr<ServerScript>;
template class WH_GAME_API ScriptRegistryMgr<SpellSC>;
template class WH_GAME_API ScriptRegistryMgr<SpellScriptLoader>;
template class WH_GAME_API ScriptRegistryMgr<TransportScript>;
template class WH_GAME_API ScriptRegistryMgr<UnitScript>;
template class WH_GAME_API ScriptRegistryMgr<VehicleScript>;
template class WH_GAME_API ScriptRegistryMgr<WeatherScript>;
template class WH_GAME_API ScriptRegistryMgr<WorldMapScript>;
template class WH_GAME_API ScriptRegistryMgr<WorldObjectScript>;
template class WH_GAME_API ScriptRegistryMgr<WorldScript>;
