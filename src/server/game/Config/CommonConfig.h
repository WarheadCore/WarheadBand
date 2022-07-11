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

#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

#include "Optional.h"
#include "StringFormat.h"
#include "Types.h"

namespace Warhead::Config
{
    // Default values if no exist in config files
    constexpr auto CONF_DEFAULT_BOOL = false;
    constexpr auto CONF_DEFAULT_STR = "";
    constexpr auto CONF_DEFAULT_INT = 0;
    constexpr auto CONF_DEFAULT_FLOAT = 1.0f;

    template<Warhead::Types::ConfigValue T>
    inline std::string GetDefaultValueString(Optional<T> def)
    {
        std::string defStr;

        if constexpr (std::is_same_v<T, bool>)
            defStr = Warhead::StringFormat("{}", !def ? CONF_DEFAULT_BOOL : *def);
        else if constexpr (std::is_integral_v<T>)
            defStr = Warhead::StringFormat("{}", !def ? CONF_DEFAULT_INT : *def);
        else if constexpr (std::is_floating_point_v<T>)
            defStr = Warhead::StringFormat("{}", !def ? CONF_DEFAULT_FLOAT : *def);
        else
            defStr = Warhead::StringFormat("{}", !def ? CONF_DEFAULT_STR : *def);

        return defStr;
    }

    template<Warhead::Types::ConfigValue T>
    inline constexpr T GetDefaultValue()
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

#endif // _COMMON_CONFIG_H_
