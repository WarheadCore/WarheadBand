/*
<<<<<<< HEAD
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
=======
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
>>>>>>> master
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
<<<<<<< HEAD
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
=======
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
>>>>>>> master
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AZEROTHCORE_COMMON_H
#define AZEROTHCORE_COMMON_H

#include "Define.h"
#include <array>
#include <memory>
#include <string>
#include <utility>

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#include <ws2tcpip.h>
#  if WARHEAD_COMPILER == WARHEAD_COMPILER_INTEL
#    if !defined(BOOST_ASIO_HAS_MOVE)
#      define BOOST_ASIO_HAS_MOVE
#    endif // !defined(BOOST_ASIO_HAS_MOVE)
#  endif // if WARHEAD_COMPILER == WARHEAD_COMPILER_INTEL
#else
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if WARHEAD_COMPILER == WARHEAD_COMPILER_MICROSOFT
#define atoll _atoi64
#define llabs _abs64
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define STRINGIZE(a) #a

// TimeConstants
constexpr auto MINUTE = 60;
constexpr auto HOUR = MINUTE * 60;
constexpr auto DAY = HOUR * 24;
constexpr auto WEEK = DAY * 7;
constexpr auto MONTH = DAY * 30;
constexpr auto YEAR = MONTH * 12;
constexpr auto IN_MILLISECONDS = 1000;

enum AccountTypes
{
    SEC_PLAYER         = 0,
    SEC_MODERATOR      = 1,
    SEC_GAMEMASTER     = 2,
    SEC_ADMINISTRATOR  = 3,
    SEC_CONSOLE        = 4                                  // must be always last in list, accounts must have less security level always also
};

enum LocaleConstant
{
    LOCALE_enUS = 0,
    LOCALE_koKR = 1,
    LOCALE_frFR = 2,
    LOCALE_deDE = 3,
    LOCALE_zhCN = 4,
    LOCALE_zhTW = 5,
    LOCALE_esES = 6,
    LOCALE_esMX = 7,
    LOCALE_ruRU = 8,

    TOTAL_LOCALES
};

#define DEFAULT_LOCALE LOCALE_enUS

#define MAX_LOCALES 8
#define MAX_ACCOUNT_TUTORIAL_VALUES 8

WH_COMMON_API extern char const* localeNames[TOTAL_LOCALES];

WH_COMMON_API LocaleConstant GetLocaleByName(const std::string& name);
WH_COMMON_API void CleanStringForMysqlQuery(std::string& str);

// we always use stdlibc++ std::max/std::min, undefine some not C++ standard defines (Win API and some other platforms)
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#define MAX_QUERY_LEN 32*1024

namespace Warhead
{
    template<class ArgumentType, class ResultType>
    struct unary_function
    {
        typedef ArgumentType argument_type;
        typedef ResultType result_type;
    };
}

#endif
