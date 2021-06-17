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

#ifndef _STRING_FORMAT_H_
#define _STRING_FORMAT_H_

#include "Define.h"
#include <fmt/printf.h>

namespace Warhead
{
    /// Default AC string format function.
    template<typename Format, typename... Args>
    inline std::string StringFormat(Format&& fmt, Args&& ... args)
    {
        try
        {
            return fmt::sprintf(std::forward<Format>(fmt), std::forward<Args>(args)...);
        }
        catch (const fmt::format_error& formatError)
        {
            std::string error = "An error occurred formatting string \"" + std::string(fmt) + "\" : " + std::string(formatError.what());
            return error;
        }
    }

    /// Returns true if the given char pointer is null.
    inline bool IsFormatEmptyOrNull(char const* fmt)
    {
        return fmt == nullptr;
    }

    /// Returns true if the given std::string is empty.
    inline bool IsFormatEmptyOrNull(std::string const& fmt)
    {
        return fmt.empty();
    }
}

namespace Warhead::String
{
    template<class Str>
    WH_COMMON_API Str Trim(const Str& s, const std::locale& loc = std::locale());

    WH_COMMON_API std::string Trim(std::string& str);
    WH_COMMON_API std::string TrimLeft(std::string& str);
    WH_COMMON_API std::string TrimLeftInPlace(std::string& str);
    WH_COMMON_API std::string TrimRight(std::string& str);
    WH_COMMON_API std::string TrimRightInPlace(std::string& str);
    WH_COMMON_API std::string Replace(std::string& str, std::string const& from, std::string const& to);
    WH_COMMON_API std::string ReplaceInPlace(std::string& str, std::string const& from, std::string const& to);
}

#endif
