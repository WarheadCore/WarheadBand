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

#ifndef _WARHEAD_ERRORS_H_
#define _WARHEAD_ERRORS_H_

#include "StringFormat.h"

namespace Warhead
{
    // Default function
    [[noreturn]] WH_COMMON_API void Assert(std::string_view file, int line, std::string_view function, std::string const& debugInfo, std::string_view message, std::string_view fmtMessage = {});
    [[noreturn]] WH_COMMON_API void Abort(std::string_view file, int line, std::string_view function, std::string_view fmtMessage = {});

    template<typename... Args>
    void Assert(std::string_view file, int line, std::string_view function, std::string const& debugInfo, std::string_view message, FormatString<Args...> fmt, Args&&... args)
    {
        Assert(file, line, function, debugInfo, message, StringFormat(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Abort(std::string_view file, int line, std::string_view function, FormatString<Args...> fmt, Args&&... args)
    {
        Abort(file, line, function, StringFormat(fmt, std::forward<Args>(args)...));
    }

    [[noreturn]] WH_COMMON_API void AbortHandler(int sigval);
} // namespace Warhead

WH_COMMON_API std::string GetDebugInfo();

#define WPAssert(cond, ...) do { if (!(cond)) Warhead::Assert(__FILE__, __LINE__, __FUNCTION__, GetDebugInfo(), #cond, ##__VA_ARGS__); } while(0)
#define WPAssert_NODEBUGINFO(cond) do { if (!(cond)) Warhead::Assert(__FILE__, __LINE__, __FUNCTION__, "", #cond); } while(0)
#define WPAbort(...) do { Warhead::Abort(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)

#ifdef PERFORMANCE_PROFILING
#define ASSERT(cond, ...) ((void)0)
#define ASSERT_NODEBUGINFO(cond, ...) ((void)0)
#else
#define ASSERT WPAssert
#define ASSERT_NODEBUGINFO WPAssert_NODEBUGINFO
#endif

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#define EXCEPTION_ASSERTION_FAILURE 0xC0000420L
#endif

#define ABORT WPAbort

template <typename T>
T* ASSERT_NOTNULL_IMPL(T* pointer, std::string_view expr)
{
    ASSERT(pointer, "{}", expr);
    return pointer;
}

#define ASSERT_NOTNULL(pointer) ASSERT_NOTNULL_IMPL(pointer, #pointer)

#endif
