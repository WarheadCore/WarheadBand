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
#include "StringConvert.h"
#include "StringFormat.h"
#include <unordered_map>

namespace
{
    std::unordered_map<std::string /*name*/, std::string /*value*/> _configOptions;

    // Default values if no exist in config files
    constexpr auto CONF_DEFAULT_BOOL = false;
    constexpr auto CONF_DEFAULT_STR = "";
    constexpr auto CONF_DEFAULT_INT = 0;
    constexpr auto CONF_DEFAULT_FLOAT = 1.0f;

    template<typename T>
    inline std::string GetDefaultValueString(Optional<T> def)
    {
        std::string defStr;

        if constexpr (std::is_same_v<T, bool>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_BOOL : *def);
        else if constexpr (std::is_integral_v<T>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_INT : *def);
        else if constexpr (std::is_floating_point_v<T>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_FLOAT : *def);
        else
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_STR : *def);

        return defStr;
    }

    template<typename T>
    constexpr T GetDefaultValue()
    {
        if constexpr (std::is_same_v<T, bool>)
            return CONF_DEFAULT_BOOL;
        else if constexpr (std::is_integral_v<T>)
            return CONF_DEFAULT_INT;
        else if constexpr (std::is_floating_point_v<T>)
            return CONF_DEFAULT_FLOAT;
        else
            return CONF_DEFAULT_STR;
    }
}

ModulesConfig* ModulesConfig::instance()
{
    static ModulesConfig instance;
    return &instance;
}

// Add option
template<typename T>
WH_GAME_API void ModulesConfig::AddOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option
    if (_configOptions.find(option) != _configOptions.end())
        _configOptions.erase(option);

    _configOptions.emplace(option, sConfigMgr->GetOption<std::string>(option, GetDefaultValueString<T>(def)));
}

// Add option without template
void ModulesConfig::AddOption(std::string_view optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    AddOption<std::string>(optionName, def);
}

void ModulesConfig::AddOption(std::initializer_list<std::string> const& optionList) const
{
    for (auto const& option : optionList)
        AddOption(option);
}

// Get option
template<typename T>
WH_GAME_API T ModulesConfig::GetOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option part 1
    auto itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        AddOption(optionName, def);
    }

    std::string defStr = GetDefaultValueString(def);

    // Check exist option part 2
    itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        LOG_FATAL("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, defStr);
        return GetDefaultValue<T>();
    }

    Optional<T> result = {};

    if constexpr (std::is_same_v<T, std::string>)
        result = _configOptions.at(option);
    else
        result = Warhead::StringTo<T>(_configOptions.at(option));

    if (!result)
    {
        LOG_ERROR("server.loading", "> ModulesConfig: Bad value defined for '{}', use '{}' instead", optionName, defStr);
        return GetDefaultValue<T>();
    }

    return *result;
}

// Set option
template<typename T>
WH_GAME_API void ModulesConfig::SetOption(std::string_view optionName, T value) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option
    auto const& itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> ModulesConfig: option ({}) is not exists", optionName);
        return;
    }

    std::string valueStr{};

    if constexpr (std::is_same_v<T, std::string>)
        valueStr = value;
    else
        valueStr = Warhead::ToString(value);

    _configOptions.erase(option);
    _configOptions.emplace(option, valueStr);
}

#define TEMPLATE_GAME_CONFIG_OPTION(__typename) \
    template WH_GAME_API __typename ModulesConfig::GetOption(std::string_view optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API void ModulesConfig::SetOption(std::string_view optionName, __typename value) const;

TEMPLATE_GAME_CONFIG_OPTION(bool)
TEMPLATE_GAME_CONFIG_OPTION(uint8)
TEMPLATE_GAME_CONFIG_OPTION(int8)
TEMPLATE_GAME_CONFIG_OPTION(uint16)
TEMPLATE_GAME_CONFIG_OPTION(int16)
TEMPLATE_GAME_CONFIG_OPTION(uint32)
TEMPLATE_GAME_CONFIG_OPTION(int32)
TEMPLATE_GAME_CONFIG_OPTION(uint64)
TEMPLATE_GAME_CONFIG_OPTION(int64)
TEMPLATE_GAME_CONFIG_OPTION(float)
TEMPLATE_GAME_CONFIG_OPTION(std::string)

#undef TEMPLATE_GAME_CONFIG_OPTION
