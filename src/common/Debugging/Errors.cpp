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

#include "Errors.h"
#include "StringFormat.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <thread>

/**
    @file Errors.cpp

    @brief This file contains definitions of functions used for reporting critical application errors

    It is very important that (std::)abort is NEVER called in place of *((volatile int*)NULL) = 0;
    Calling abort() on Windows does not invoke unhandled exception filters - a mechanism used by WheatyExceptionReport
    to log crashes. exit(1) calls here are for static analysis tools to indicate that calling functions defined in this file
    terminates the application.
 */

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <Windows.h>
#define Crash(message) \
    ULONG_PTR execeptionArgs[] = { reinterpret_cast<ULONG_PTR>(strdup(message)), reinterpret_cast<ULONG_PTR>(_ReturnAddress()) }; \
    RaiseException(EXCEPTION_ASSERTION_FAILURE, 0, 2, execeptionArgs);
#else
// should be easily accessible in gdb
extern "C" { WH_COMMON_API char const* TrinityAssertionFailedMessage = nullptr; }
#define Crash(message) \
    TrinityAssertionFailedMessage = strdup(message); \
    *((volatile int*)nullptr) = 0; \
    exit(1);
#endif

namespace Warhead
{
    template<typename... Args>
    void Assert(char const* file, int line, char const* function, std::string const& debugInfo, std::string_view fmt, Args&&... args)
    {
        std::string fmtMessage = StringFormat(fmt, std::forward<Args>(args)...);
        std::string formattedMessage = StringFormat("\n{}:{} in {} ASSERTION FAILED:\n  {}\n", file, line, function, fmtMessage) + '\n' + debugInfo + '\n';

        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        Crash(formattedMessage.c_str());
    }

    template<typename... Args>
    void Fatal(char const* file, int line, char const* function, std::string_view fmt, Args&&... args)
    {
        std::string fmtMessage = StringFormat(fmt, std::forward<Args>(args)...);
        std::string formattedMessage = StringFormat("\n{}:{} in {} FATAL ERROR:\n  {}\n", file, line, function, fmtMessage) + '\n' + debugInfo + '\n';

        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        Crash(formattedMessage.c_str());
    }

    template<typename... Args>
    void Error(char const* file, int line, char const* function, std::string_view fmt, Args&&... args)
    {
        std::string fmtMessage = StringFormat(fmt, std::forward<Args>(args)...);
        std::string formattedMessage = StringFormat("\n{}:{} in {} ERROR:\n  {}\n", file, line, function, fmtMessage) + '\n' + debugInfo + '\n';

        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        Crash(formattedMessage.c_str());
    }

    void Warning(char const* file, int line, char const* function, std::string_view fmt, Args&&... args)
    {
        std::string fmtMessage = StringFormat(fmt, std::forward<Args>(args)...);
        fprintf(stderr, "\n%s:%i in %s WARNING:\n  %s\n", file, line, function, fmtMessage);
    }

    void Abort(char const* file, int line, char const* function)
    {
        std::string formattedMessage = StringFormat("\n{}:{} in {} ABORTED.\n", file, line, function);
        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);
        Crash(formattedMessage.c_str());
    }

    template<typename... Args>
    void Abort(char const* file, int line, char const* function, std::string_view fmt, Args&&... args)
    {
        std::string fmtMessage = StringFormat(fmt, std::forward<Args>(args)...);
        std::string formattedMessage = StringFormat("\n{}:{} in {} ABORTED:\n  {}\n", file, line, function, fmtMessage) + '\n' + debugInfo + '\n';

        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        Crash(formattedMessage.c_str());
    }

    void AbortHandler(int sigval)
    {
        // nothing useful to log here, no way to pass args
        std::string formattedMessage = StringFormat("Caught signal {}\n", sigval);
        fprintf(stderr, "%s", formattedMessage.c_str());
        fflush(stderr);
        Crash(formattedMessage.c_str());
    }

} // namespace Warhead

std::string GetDebugInfo()
{
    return "";
}
