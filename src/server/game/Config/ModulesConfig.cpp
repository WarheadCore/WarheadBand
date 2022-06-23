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
#include "CommonConfig.h"
#include "Config.h"
#include "Log.h"
#include "StringConvert.h"

ModulesConfig* ModulesConfig::instance()
{
    static ModulesConfig instance;
    return &instance;
}

// Add option
template<Warhead::Types::ConfigValue T>
WH_GAME_API void ModulesConfig::AddOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/)
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option
    if (_configOptions.find(option) != _configOptions.end())
        _configOptions.erase(option);

    _configOptions.emplace(option, sConfigMgr->GetOption<std::string>(option, Warhead::Config::GetDefaultValueString<T>(def)));
}

// Add option without template
void ModulesConfig::AddOption(std::string_view optionName, Optional<std::string> def /*= std::nullopt*/)
{
    AddOption<std::string>(optionName, def);
}

// Get option
template<Warhead::Types::ConfigValue T>
WH_GAME_API T ModulesConfig::GetOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/)
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option part 1
    auto itr = _configOptions.find(option);
    if (itr == _configOptions.end())
        AddOption(optionName, def);

    std::string defStr = Warhead::Config::GetDefaultValueString(def);

    // Check exist option part 2
    itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        LOG_FATAL("server.loading", "> ModulesConfig: option ({}) is not exists. Returned ({})", optionName, defStr);
        return Warhead::Config::GetDefaultValue<T>();
    }

    Optional<T> result;

    if constexpr (std::is_same_v<T, std::string>)
        result = itr->second;
    else
        result = Warhead::StringTo<T>(itr->second);

    if (!result)
    {
        LOG_ERROR("server.loading", "> ModulesConfig: Bad value defined for '{}', use '{}' instead", optionName, defStr);
        return Warhead::Config::GetDefaultValue<T>();
    }

    return *result;
}

// Set option
template<Warhead::Types::ConfigValue T>
WH_GAME_API void ModulesConfig::SetOption(std::string_view optionName, T value)
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

    std::string valueStr;

    if constexpr (std::is_same_v<T, std::string>)
        valueStr = value;
    else
        valueStr = Warhead::ToString(value);

    _configOptions.erase(option);
    _configOptions.emplace(option, valueStr);
}

#define TEMPLATE_GAME_CONFIG_OPTION(__typename) \
    template WH_GAME_API __typename ModulesConfig::GetOption(std::string_view optionName, Optional<__typename> def /*= std::nullopt*/); \
    template WH_GAME_API void ModulesConfig::SetOption(std::string_view optionName, __typename value);

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
