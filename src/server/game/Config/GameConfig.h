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

#ifndef __GAME_CONFIG
#define __GAME_CONFIG

#include "Common.h"
#include "Optional.h"

class WH_GAME_API GameConfig
{
public:
    static GameConfig* instance();

    void Load(bool reload);
    void CheckOptions(bool reload = false);

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

private:
    void LoadConfigs(bool reload = false);
};

#define sGameConfig GameConfig::instance()

#define CONF_GET_BOOL(__optionName) sGameConfig->GetOption<bool>(__optionName)
#define CONF_GET_STR(__optionName) sGameConfig->GetOption<std::string>(__optionName)
#define CONF_GET_INT(__optionName) sGameConfig->GetOption<int32>(__optionName)
#define CONF_GET_UINT(__optionName) sGameConfig->GetOption<uint32>(__optionName)
#define CONF_GET_FLOAT(__optionName) sGameConfig->GetOption<float>(__optionName)

#endif // __GAME_CONFIG
