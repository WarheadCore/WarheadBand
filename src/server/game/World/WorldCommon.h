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

#ifndef _WORLD_COMMON_H_
#define _WORLD_COMMON_H_

#include "Define.h"

enum ShutdownMask
{
    SHUTDOWN_MASK_RESTART = 1,
    SHUTDOWN_MASK_IDLE    = 2,
};

enum ShutdownExitCode
{
    SHUTDOWN_EXIT_CODE = 0,
    ERROR_EXIT_CODE    = 1,
    RESTART_EXIT_CODE  = 2,
};

/// Timers for different object refresh rates
enum WorldTimers
{
    WUPDATE_AUCTIONS,
    WUPDATE_WEATHERS,
    WUPDATE_UPTIME,
    WUPDATE_CORPSES,
    WUPDATE_EVENTS,
    WUPDATE_AUTOBROADCAST,
    WUPDATE_MAILBOXQUEUE,
    WUPDATE_PINGDB,
    WUPDATE_5_SECS,
    WUPDATE_WHO_LIST,
    WUPDATE_COUNT
};

/// Can be used in SMSG_AUTH_RESPONSE packet
enum BillingPlanFlags
{
    SESSION_NONE            = 0x00,
    SESSION_UNUSED          = 0x01,
    SESSION_RECURRING_BILL  = 0x02,
    SESSION_FREE_TRIAL      = 0x04,
    SESSION_IGR             = 0x08,
    SESSION_USAGE           = 0x10,
    SESSION_TIME_MIXTURE    = 0x20,
    SESSION_RESTRICTED      = 0x40,
    SESSION_ENABLE_CAIS     = 0x80,
};

enum RealmZone
{
    REALM_ZONE_UNKNOWN       = 0,   // any language
    REALM_ZONE_DEVELOPMENT   = 1,   // any language
    REALM_ZONE_UNITED_STATES = 2,   // extended-Latin
    REALM_ZONE_OCEANIC       = 3,   // extended-Latin
    REALM_ZONE_LATIN_AMERICA = 4,   // extended-Latin
    REALM_ZONE_TOURNAMENT_5  = 5,   // basic-Latin at create, any at login
    REALM_ZONE_KOREA         = 6,   // East-Asian
    REALM_ZONE_TOURNAMENT_7  = 7,   // basic-Latin at create, any at login
    REALM_ZONE_ENGLISH       = 8,   // extended-Latin
    REALM_ZONE_GERMAN        = 9,   // extended-Latin
    REALM_ZONE_FRENCH        = 10,  // extended-Latin
    REALM_ZONE_SPANISH       = 11,  // extended-Latin
    REALM_ZONE_RUSSIAN       = 12,  // Cyrillic
    REALM_ZONE_TOURNAMENT_13 = 13,  // basic-Latin at create, any at login
    REALM_ZONE_TAIWAN        = 14,  // East-Asian
    REALM_ZONE_TOURNAMENT_15 = 15,  // basic-Latin at create, any at login
    REALM_ZONE_CHINA         = 16,  // East-Asian
    REALM_ZONE_CN1           = 17,  // basic-Latin at create, any at login
    REALM_ZONE_CN2           = 18,  // basic-Latin at create, any at login
    REALM_ZONE_CN3           = 19,  // basic-Latin at create, any at login
    REALM_ZONE_CN4           = 20,  // basic-Latin at create, any at login
    REALM_ZONE_CN5           = 21,  // basic-Latin at create, any at login
    REALM_ZONE_CN6           = 22,  // basic-Latin at create, any at login
    REALM_ZONE_CN7           = 23,  // basic-Latin at create, any at login
    REALM_ZONE_CN8           = 24,  // basic-Latin at create, any at login
    REALM_ZONE_TOURNAMENT_25 = 25,  // basic-Latin at create, any at login
    REALM_ZONE_TEST_SERVER   = 26,  // any language
    REALM_ZONE_TOURNAMENT_27 = 27,  // basic-Latin at create, any at login
    REALM_ZONE_QA_SERVER     = 28,  // any language
    REALM_ZONE_CN9           = 29,  // basic-Latin at create, any at login
    REALM_ZONE_TEST_SERVER_2 = 30,  // any language
    REALM_ZONE_CN10          = 31,  // basic-Latin at create, any at login
    REALM_ZONE_CTC           = 32,  
    REALM_ZONE_CNC           = 33,  
    REALM_ZONE_CN1_4         = 34,  // basic-Latin at create, any at login
    REALM_ZONE_CN2_6_9       = 35,  // basic-Latin at create, any at login
    REALM_ZONE_CN3_7         = 36,  // basic-Latin at create, any at login
    REALM_ZONE_CN5_8         = 37   // basic-Latin at create, any at login
};

enum WorldStates
{
    WS_ARENA_DISTRIBUTION_TIME                 = 20001, // Next arena distribution time
    WS_WEEKLY_QUEST_RESET_TIME                 = 20002, // Next weekly reset time
    WS_BG_DAILY_RESET_TIME                     = 20003, // Next daily BG reset time
    WS_CLEANING_FLAGS                          = 20004, // Cleaning Flags
    WS_DAILY_QUEST_RESET_TIME                  = 20005, // Next daily reset time
    WS_GUILD_DAILY_RESET_TIME                  = 20006, // Next guild cap reset time
    WS_MONTHLY_QUEST_RESET_TIME                = 20007, // Next monthly reset time
    WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME = 20008  // Next daily calendar deletions of old events time
};

#endif
