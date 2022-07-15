/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "StatControl.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "StopWatch.h"

namespace
{
    constexpr StatControlType GetStatControlTypeFromString(std::string_view name)
    {
        if (name == "AttackPowerFromAgility")
            return StatControlType::AttackPowerFromAgility;
        else if (name == "DoodgeFromAgility")
            return StatControlType::DoodgeFromAgility;
        else if (name == "ArmorFromAgility")
            return StatControlType::ArmorFromAgility;
        else if (name == "CritFromAgility")
            return StatControlType::CritFromAgility;
        else if (name == "CritFromIntellect")
            return StatControlType::CritFromIntellect;
        else if (name == "ManaFromIntellect")
            return StatControlType::ManaFromIntellect;
        else if (name == "BlockFromStrenght")
            return StatControlType::BlockFromStrenght;

        return StatControlType::Max;
    }
}

StatControlMgr* StatControlMgr::instance()
{
    static StatControlMgr instance;
    return &instance;
}

void StatControlMgr::Initialize()
{
    if (!_isEnable)
        return;

    LoadDBData();
}

void StatControlMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("SC.Enable");

    /*if (!_isEnable)
        return;*/
}

void StatControlMgr::LoadDBData()
{
    LOG_INFO("server.loading", "Loading stat conrol data...");

    StopWatch sw;
    _store.clear();

    QueryResult result = WorldDatabase.Query("SELECT `StatControlType`, `Value` FROM `wh_stats_control`");
    if (!result)
    {
        LOG_FATAL("sql.sql", ">> Loaded 0 stats control. DB table `wh_stats_control` is empty.");
        LOG_WARN("server.loading", "> Disable this module");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [typeString, value] = result->FetchTuple<std::string_view, float>();

        if (value == 0.0f)
        {
            LOG_ERROR("module", "> StatControl: Incorrect value {} for stat control type: {}. Skip", value, typeString);
            continue;
        }

        auto type = GetStatControlTypeFromString(typeString);
        if (type == StatControlType::Max)
        {
            LOG_ERROR("module", "> StatControl: Incorrect stat control type: {}. Skip", typeString);
            continue;
        }

        _store.emplace(static_cast<uint8>(type), value);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} stats control in {}", _store.size(), sw);
    LOG_INFO("server.loading", "");
}

float const* StatControlMgr::GetMultiplier(StatControlType type)
{
    return Warhead::Containers::MapGetValuePtr(_store, static_cast<uint8>(type));
}

void StatControlMgr::CorrectStat(float& value, StatControlType type)
{
    if (!_isEnable)
        return;

    auto multipler = GetMultiplier(type);
    if (!multipler)
        return;

    value *= *multipler;
}
