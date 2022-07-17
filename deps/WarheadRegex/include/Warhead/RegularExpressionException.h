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

#ifndef _WARHEAD_RE_EXCEPTION_H_
#define _WARHEAD_RE_EXCEPTION_H_

#include <exception>
#include <string>
#include <string_view>

namespace Warhead
{
    class RegularExpressionException
    {
    public:
        /// Creates an exception with message.
        RegularExpressionException(std::string_view msg) : _msg(msg) { }

        /// Destroys the exception.
        ~RegularExpressionException() noexcept = default;

        /// Returns a string consisting of the message text.
        inline std::string_view GetErrorMessage() const { return _msg; }

    private:
        std::string _msg;

        RegularExpressionException(RegularExpressionException const&) = delete;
        RegularExpressionException(RegularExpressionException&&) = delete;
        RegularExpressionException& operator=(RegularExpressionException const&) = delete;
        RegularExpressionException& operator=(RegularExpressionException&&) = delete;
    };
}

#endif // _WARHEAD_RE_EXCEPTION_H_
