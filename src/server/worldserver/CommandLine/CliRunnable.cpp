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

/// \addtogroup Trinityd
/// @{
/// \file

#include "CliRunnable.h"
#include "Common.h"
#include "Config.h"
#include "Errors.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Util.h"
#include "World.h"
#include <fmt/core.h>

#if WARHEAD_PLATFORM != WARHEAD_PLATFORM_WINDOWS
#include "Chat.h"
#include "ChatCommand.h"
#include <cstring>
#include <readline/history.h>
#include <readline/readline.h>
#endif

static constexpr char CLI_PREFIX[] = "AC> ";

static inline void PrintCliPrefix()
{
    fmt::print(CLI_PREFIX);
}

#if WARHEAD_PLATFORM != WARHEAD_PLATFORM_WINDOWS
namespace Warhead::Impl::Readline
{
    static std::vector<std::string> vec;
    char* cli_unpack_vector(char const*, int state)
    {
        static size_t i=0;
        if (!state)
            i = 0;
        if (i < vec.size())
            return strdup(vec[i++].c_str());
        else
            return nullptr;
    }

    char** cli_completion(char const* text, int /*start*/, int /*end*/)
    {
        ::rl_attempted_completion_over = 1;
        vec = Warhead::ChatCommands::GetAutoCompletionsFor(CliHandler(nullptr,nullptr), text);
        return ::rl_completion_matches(text, &cli_unpack_vector);
    }

    int cli_hook_func()
    {
        if (World::IsStopped())
            ::rl_done = 1;
        return 0;
    }
}
#endif

void utf8print(void* /*arg*/, std::string_view str)
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    fmt::print(str);
#else
{
    fmt::print(str);
    fflush(stdout);
}
#endif
}

void commandFinished(void*, bool /*success*/)
{
    PrintCliPrefix();
    fflush(stdout);
}

#ifdef linux
// Non-blocking keypress detector, when return pressed, return 1, else always return 0
int kb_hit_return()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

/// %Thread start
void CliThread()
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    // print this here the first time
    // later it will be printed after command queue updates
    PrintCliPrefix();
#else
    ::rl_attempted_completion_function = &Warhead::Impl::Readline::cli_completion;
    {
        static char BLANK = '\0';
        ::rl_completer_word_break_characters = &BLANK;
    }
    ::rl_event_hook = &Warhead::Impl::Readline::cli_hook_func;
#endif

    if (sConfigMgr->GetBoolDefault("BeepAtStart", true))
        printf("\a");                                       // \a = Alert

    ///- As long as the World is running (no World::m_stopEvent), get the command line and handle it
    while (!World::IsStopped())
    {
        fflush(stdout);

        std::string command;

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
        wchar_t commandbuf[256];
        if (fgetws(commandbuf, sizeof(commandbuf), stdin))
        {
            if (!WStrToUtf8(commandbuf, wcslen(commandbuf), command))
            {
                PrintCliPrefix();
                continue;
            }
        }
#else
        char* command_str = readline(CLI_PREFIX);
        ::rl_bind_key('\t', ::rl_complete);
        if (command_str != nullptr)
        {
            command = command_str;
            free(command_str);
        }
#endif

        if (!command.empty())
        {
            std::size_t nextLineIndex = command.find_first_of("\r\n");
            if (nextLineIndex != std::string::npos)
            {
                if (nextLineIndex == 0)
                {
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
                    PrintCliPrefix();
#endif
                    continue;
                }

                command.erase(nextLineIndex);
            }

            fflush(stdout);
            sWorld->QueueCliCommand(new CliCommandHolder(nullptr, command.c_str(), &utf8print, &commandFinished));
#if WARHEAD_PLATFORM != WARHEAD_PLATFORM_WINDOWS
            add_history(command.c_str());
#endif
        }
        else if (feof(stdin))
        {
            World::StopNow(SHUTDOWN_EXIT_CODE);
        }
    }
}