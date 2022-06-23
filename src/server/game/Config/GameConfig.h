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

#ifndef __GAME_CONFIG
#define __GAME_CONFIG

#include "Common.h"
#include "Optional.h"
#include "Types.h"
#include <unordered_map>

class WH_GAME_API GameConfig
{
    GameConfig(GameConfig const&) = delete;
    GameConfig(GameConfig&&) = delete;
    GameConfig& operator= (GameConfig const&) = delete;
    GameConfig& operator= (GameConfig&&) = delete;

    GameConfig() = default;
    ~GameConfig() = default;

public:
    static GameConfig* instance();

    void Load(bool reload);
    void CheckOptions(bool reload = false);

    // Add config option
    template<Warhead::Types::ConfigValue T>
    void AddOption(std::string_view optionName, Optional<T> def = {});

    // Add option without template
    void AddOption(std::string_view optionName, Optional<std::string> def = {});

    // Add option list without template
    void AddOption(std::initializer_list<std::string> const& optionList);

    // Get config options
    template<Warhead::Types::ConfigValue T>
    T GetOption(std::string_view optionName, Optional<T> = std::nullopt);

    // Set config option
    template<Warhead::Types::ConfigValue T>
    void SetOption(std::string_view optionName, T value);

private:
    void LoadConfigs(bool reload = false);

    std::unordered_map<std::string /*name*/, std::string /*value*/> _configOptions;
};

#define sGameConfig GameConfig::instance()

#define CONF_GET_BOOL(__optionName) sGameConfig->GetOption<bool>(__optionName)
#define CONF_GET_STR(__optionName) sGameConfig->GetOption<std::string>(__optionName)
#define CONF_GET_INT(__optionName) sGameConfig->GetOption<int32>(__optionName)
#define CONF_GET_UINT(__optionName) sGameConfig->GetOption<uint32>(__optionName)
#define CONF_GET_FLOAT(__optionName) sGameConfig->GetOption<float>(__optionName)

#endif // __GAME_CONFIG
