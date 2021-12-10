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
    void AddOption(std::string const& optionName, Optional<T> def = std::nullopt) const;

    // Add option without template
    void AddOption(std::string const& optionName, Optional<std::string> def = std::nullopt) const;

    // Add option list without template
    void AddOption(std::initializer_list<std::string> optionList) const;

    // Get config options
    template<typename T>
    T GetOption(std::string const& optionName, Optional<T> = std::nullopt) const;

    // Set config option
    template<typename T>
    void SetOption(std::string const& optionName, T value) const;
};

#define sModulesConfig ModulesConfig::instance()

inline bool MOD_CONF_GET_BOOL(std::string const& __optionName)
{
    return sModulesConfig->GetOption<bool>(__optionName);
}

inline std::string MOD_CONF_GET_STR(std::string const& __optionName)
{
    return sModulesConfig->GetOption<std::string>(__optionName);
}

inline int32 MOD_CONF_GET_INT(std::string const& __optionName)
{
    return sModulesConfig->GetOption<int32>(__optionName);
}

inline uint32 MOD_CONF_GET_UINT(std::string const& __optionName)
{
    return sModulesConfig->GetOption<uint32>(__optionName);
}

inline float MOD_CONF_GET_FLOAT(std::string const& __optionName)
{
    return sModulesConfig->GetOption<float>(__optionName);
}

#endif // __GAME_CONFIG
