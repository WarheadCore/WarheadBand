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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "StatControl.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
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
        else if (name == "DamageFromAttackPower")
            return StatControlType::DamageFromAttackPower;

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

    QueryResult result = WorldDatabase.Query("SELECT `StatControlType`, `ClassMask`, `Value`, `IsEnable` FROM `wh_stats_control`");
    if (!result)
    {
        LOG_FATAL("sql.sql", ">> Loaded 0 stats control. DB table `wh_stats_control` is empty.");
        LOG_WARN("server.loading", "> Disable this module");
        _isEnable = false;
        return;
    }

    do
    {
        auto const& [typeString, classMask, value, isEnable] = result->FetchTuple<std::string_view, int32, float, bool>();

        if (!isEnable)
            continue;

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

        _store.emplace_back(StatControl{ type, value, classMask });

    } while (result->NextRow());

    if (_store.empty())
    {
        LOG_INFO("server.loading", ">> Loaded 0 stats control in {}. Disable module", _store.size(), sw);
        LOG_INFO("server.loading", "");
        _isEnable = false;
        return;
    }

    LOG_INFO("server.loading", ">> Loaded {} stats control in {}", _store.size(), sw);
    LOG_INFO("server.loading", "");
}

void StatControlMgr::CorrectStat(Player* player, float& value, StatControlType type)
{
    if (!_isEnable || !player)
        return;

    for (auto const& [scType, scValue, classMask] : _store)
    {
        if (scType != type)
            continue;

        if (classMask && !(player->getClassMask() & classMask))
            continue;

        value *= scValue;
    }
}
