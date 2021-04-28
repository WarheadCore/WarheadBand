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

#ifndef AZEROTHCORE_COMMON_H
#define AZEROTHCORE_COMMON_H

#include "Define.h"
#include <cassert>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <memory>
#include <utility>

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

#include <set>
#include <list>
#include <string>
#include <map>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
// XP winver - needed to compile with standard leak check in MemoryLeaks.h
// uncomment later if needed
//#define _WIN32_WINNT 0x0501
#  include <ws2tcpip.h>
//#undef WIN32_WINNT
#else
#  include <sys/types.h>
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  include <netdb.h>
#endif

#if WARHEAD_COMPILER == WARHEAD_COMPILER_MICROSOFT

#include <float.h>

#define I32FMT "%08I32X"
#define I64FMT "%016I64X"
#define atoll _atoi64
#define vsnprintf _vsnprintf
#define llabs _abs64

#else

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define I32FMT "%08X"
#define I64FMT "%016llX"

#endif

using namespace std;

inline float finiteAlways(float f) { return isfinite(f) ? f : 0.0f; }

inline bool myisfinite(float f) { return isfinite(f) && !isnan(f); }

#define STRINGIZE(a) #a

#define MAX_NETCLIENT_PACKET_SIZE (32767 - 1)               // Client hardcap: int16 with trailing zero space otherwise crash on memory free

enum TimeConstants
{
    MINUTE          = 60,
    HOUR            = MINUTE * 60,
    DAY             = HOUR * 24,
    WEEK            = DAY * 7,
    MONTH           = DAY * 30,
    YEAR            = MONTH * 12,
    IN_MILLISECONDS = 1000
};

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
    LOCALE_ruRU = 8
};

const uint8 TOTAL_LOCALES = 9;
#define DEFAULT_LOCALE LOCALE_enUS

#define MAX_LOCALES 8
#define MAX_ACCOUNT_TUTORIAL_VALUES 8

WH_COMMON_API extern char const* localeNames[TOTAL_LOCALES];

WH_COMMON_API LocaleConstant GetLocaleByName(const std::string& name);
WH_COMMON_API void CleanStringForMysqlQuery(std::string& str);

typedef std::vector<std::string> StringVector;

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
