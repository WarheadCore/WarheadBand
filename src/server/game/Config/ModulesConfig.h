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

#ifndef _MODULES_CONFIG_H_
#define _MODULES_CONFIG_H_

#include "Common.h"
#include "Optional.h"

class WH_GAME_API ModulesConfig
{
    ModulesConfig(ModulesConfig const&) = delete;
    ModulesConfig(ModulesConfig&&) = delete;
    ModulesConfig& operator= (ModulesConfig const&) = delete;
    ModulesConfig& operator= (ModulesConfig&&) = delete;

    ModulesConfig() = default;
    ~ModulesConfig() = default;

public:
    static ModulesConfig* instance();

    // Add config option
    template<typename T>
    void AddOption(std::string_view optionName, Optional<T> def = {}) const;

    // Add option without template
    void AddOption(std::string_view optionName, Optional<std::string> def = {}) const;

    // Add option list without template
    void AddOption(std::initializer_list<std::string> const& optionList) const;

    // Get config options
    template<typename T>
    T GetOption(std::string_view optionName, Optional<T> = std::nullopt) const;

    // Set config option
    template<typename T>
    void SetOption(std::string_view optionName, T value) const;
};

#define sModulesConfig ModulesConfig::instance()

#define MOD_CONF_GET_BOOL(__optionName) sModulesConfig->GetOption<bool>(__optionName)
#define MOD_CONF_GET_STR(__optionName) sModulesConfig->GetOption<std::string>(__optionName)
#define MOD_CONF_GET_INT(__optionName) sModulesConfig->GetOption<int32>(__optionName)
#define MOD_CONF_GET_UINT(__optionName) sModulesConfig->GetOption<uint32>(__optionName)
#define MOD_CONF_GET_FLOAT(__optionName) sModulesConfig->GetOption<float>(__optionName)

#endif // __GAME_CONFIG
