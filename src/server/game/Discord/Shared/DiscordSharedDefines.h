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

#ifndef __DISCORD_SHARED_DEFINES_H__
#define __DISCORD_SHARED_DEFINES_H__

#include "Define.h"
#include <string_view>
#include <vector>

// EnumUtils: DESCRIBE THIS
enum DiscordCode : uint16
{
    CLIENT_SEND_HELLO = 1,
    CLIENT_AUTH_SESSION,
    CLIENT_SEND_MESSAGE,
    CLIENT_SEND_MESSAGE_EMBED,
    CLIENT_SEND_PING,

    SERVER_SEND_AUTH_RESPONSE,
    SERVER_SEND_PONG,

    MAX_DISCORD_CODE
};

constexpr uint32 NUM_DISCORD_CODE_HANDLERS = MAX_DISCORD_CODE;
constexpr uint32 NULL_DISCORD_CODE = 0x0000;

// EnumUtils: DESCRIBE THIS
enum class DiscordAuthResponseCodes : uint8
{
    Ok,
    Failed,
    IncorrectKey,
    UnknownAccount,
    BannedAccount,
    BannedIP,
    BannedPermanentlyAccount,
    BannedPermanentlyIP,
    ServerOffline
};

// EnumUtils: DESCRIBE THIS
enum class AccountResponceResult : uint8
{
    Ok,
    LongName,
    LongKey,
    NameAlreadyExist,
    NameNotExist,
    DBError
};

enum class DiscordMessageColor : uint32
{
    Blue    = 0x28a745,
    Red     = 0xdc3545,
    Orange  = 0xfd7e14,
    Purple  = 0x6f42c1,
    Indigo  = 0x6610f2,
    Yellow  = 0xffc107,
    Teal    = 0x20c997,
    Cyan    = 0x17a2b8,
    Gray    = 0xadb5bd,
    White   = 0xffffff
};

struct EmbedField
{
    EmbedField(std::string_view name, std::string_view value, bool isInline) :
        Name(name), Value(value), IsInline(isInline) { }

    std::string Name;
    std::string Value;
    bool IsInline{ false };

    bool IsCorrectName() const
    {
        return Name.size() <= 256;
    }

    bool IsCorrectValue() const
    {
        return Value.size() <= 1024;
    }
};

constexpr std::size_t WARHEAED_DISCORD_MAX_EMBED_FIELDS = 24;
using DiscordEmbedFields = std::vector<EmbedField>;

constexpr auto WARHEAD_DISCORD_VERSION = 100000;
constexpr auto GetVersionMajor() { return WARHEAD_DISCORD_VERSION / 100000; }
constexpr auto GetVersionMinor() { return WARHEAD_DISCORD_VERSION / 100 % 1000; }
constexpr auto GetVersionPatch() { return WARHEAD_DISCORD_VERSION % 100; }

#endif // __DISCORD_SHARED_DEFINES_H__
