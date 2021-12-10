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

#include "ModulesConfig.h"
#include "Config.h"
#include "Log.h"
#include "MapMgr.h"
#include "Object.h"
#include "Player.h"
#include "ServerMotd.h"
#include "StringConvert.h"
#include "World.h"
#include <unordered_map>

namespace
{
    std::unordered_map<std::string /*name*/, std::string /*value*/> _configOptions;

    // Default values if no exist in config files
    constexpr auto CONF_DEFAULT_BOOL = false;
    constexpr auto CONF_DEFAULT_STR = "";
    constexpr auto CONF_DEFAULT_INT = 0;
    constexpr auto CONF_DEFAULT_FLOAT = 1.0f;
}

ModulesConfig* ModulesConfig::instance()
{
    static ModulesConfig instance;
    return &instance;
}

// Add option
template<typename T>
WH_GAME_API void ModulesConfig::AddOption(std::string const& optionName, Optional<T> def /*= std::nullopt*/) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is already exists", optionName);
        return;
    }

    _configOptions.emplace(optionName, Warhead::ToString<T>(sConfigMgr->GetOption<T>(optionName, !def ? CONF_DEFAULT_INT : *def));
}

template<>
WH_GAME_API void ModulesConfig::AddOption<bool>(std::string const& optionName, Optional<bool> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is already exists", optionName);
        return;
    }

    _configOptions.emplace(optionName, Warhead::ToString(sConfigMgr->GetOption<bool>(optionName, !def ? CONF_DEFAULT_BOOL : *def));
}

template<>
WH_GAME_API void ModulesConfig::AddOption<std::string>(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is already exists", optionName);
        return;
    }

    _configOptions.emplace(optionName, sConfigMgr->GetOption<std::string>(optionName, !def ? CONF_DEFAULT_STR : *def));
}

template<>
WH_GAME_API void ModulesConfig::AddOption<float>(std::string const& optionName, Optional<float> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is already exists", optionName);
        return;
    }

    _configOptions.emplace(optionName, Warhead::ToString(sConfigMgr->GetOption<float>(optionName, def == std::nullopt ? CONF_DEFAULT_FLOAT : *def));
}

// Add option without template
void ModulesConfig::AddOption(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    AddOption<std::string>(optionName, def);
}

void ModulesConfig::AddOption(std::initializer_list<std::string> optionList) const
{
    for (auto const& option : optionList)
        AddOption(option);
}

// Get option
template<typename T>
WH_GAME_API T ModulesConfig::GetOption(std::string const& optionName, Optional<T> def /*= std::nullopt*/) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    auto retValueDef = def ? *def : CONF_DEFAULT_INT;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef);
        return retValueDef;
    }

    Optional<T> result = Warhead::StringTo<T>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> ModulesConfig: Bad value defined for '{}', use '{}' instead", optionName, retValueDef);
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API bool ModulesConfig::GetOption<bool>(std::string const& optionName, Optional<bool> def /*= std::nullopt*/) const
{
    auto retValueDef = def ? *def : CONF_DEFAULT_BOOL;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef ? "true" : "false");
        return retValueDef;
    }

    Optional<bool> result = Warhead::StringTo<bool>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> ModulesConfig: Bad value defined for '{}', use '{}' instead", optionName, retValueDef ? "true" : "false");
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API float ModulesConfig::GetOption<float>(std::string const& optionName, Optional<float> def /*= std::nullopt*/) const
{
    auto retValueDef = def ? *def : CONF_DEFAULT_FLOAT;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef);
        return retValueDef;
    }

    Optional<float> result = Warhead::StringTo<float>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> ModulesConfig: Bad value defined for '{}', use '{}' instead", optionName, retValueDef);
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API std::string ModulesConfig::GetOption<std::string>(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    auto retValueDef = def == std::nullopt ? CONF_DEFAULT_STR : *def;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef);
        return retValueDef;
    }

    return _configOptions.at(optionName);
}

// Set option
template<typename T>
WH_GAME_API void ModulesConfig::SetOption(std::string const& optionName, T value) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(value));
}

template<>
WH_GAME_API void ModulesConfig::SetOption<bool>(std::string const& optionName, bool value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(value));
}

template<>
WH_GAME_API void ModulesConfig::SetOption<std::string>(std::string const& optionName, std::string value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, value);
}

template<>
WH_GAME_API void ModulesConfig::SetOption<float>(std::string const& optionName, float value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(value));
}

#define TEMPLATE_GAME_CONFIG_OPTION(__typename) \
    template WH_GAME_API void ModulesConfig::AddOption(std::string const& optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API __typename ModulesConfig::GetOption(std::string const& optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API void ModulesConfig::SetOption(std::string const& optionName, __typename value) const;

TEMPLATE_GAME_CONFIG_OPTION(uint8)
TEMPLATE_GAME_CONFIG_OPTION(int8)
TEMPLATE_GAME_CONFIG_OPTION(uint16)
TEMPLATE_GAME_CONFIG_OPTION(int16)
TEMPLATE_GAME_CONFIG_OPTION(uint32)
TEMPLATE_GAME_CONFIG_OPTION(int32)
TEMPLATE_GAME_CONFIG_OPTION(uint64)
TEMPLATE_GAME_CONFIG_OPTION(int64)

#undef TEMPLATE_GAME_CONFIG_OPTION
