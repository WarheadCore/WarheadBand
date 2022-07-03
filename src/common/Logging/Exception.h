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

#ifndef _WARHEAD_EXCEPTION_H_
#define _WARHEAD_EXCEPTION_H_

#include "Define.h"
#include <fmt/format.h>

namespace Warhead
{
    class WH_COMMON_API Exception
    {
    public:
        /// Creates an exception with message.
        Exception(std::string_view msg) : _msg(msg) { }

        /// Creates an exception with fmt message.
        template<typename... Args>
        Exception(std::string_view fmt, Args&&... args)
        {
            try
            {
                _msg = fmt::format(fmt, std::forward<Args>(args)...);
            }
            catch (const fmt::format_error& formatError)
            {
                _msg = fmt::format("An error occurred formatting string \"{}\": {}", fmt, formatError.what());
            }
        }

        /// Destroys the exception.
        ~Exception() noexcept = default;

        /// Returns a string consisting of the
        /// message name and the message text.
        inline std::string_view GetErrorMessage() const { return _msg; }

    private:
        std::string _msg;

        Exception(Exception const&) = delete;
        Exception(Exception&&) = delete;
        Exception& operator=(Exception const&) = delete;
        Exception& operator=(Exception&&) = delete;
    };

} // namespace Warhead

#endif //
