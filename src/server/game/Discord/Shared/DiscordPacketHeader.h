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

#ifndef __DISCORD_PACKET_HEADER_H__
#define __DISCORD_PACKET_HEADER_H__

#include "Define.h"
#include "DiscordSharedDefines.h"

#pragma pack(push, 1)
struct DiscordServerPktHeader
{
    /**
     * size is the length of the payload _plus_ the length of the opcode
     */
    DiscordServerPktHeader(uint32 size, uint16 cmd) : size(size)
    {
        uint8 headerIndex{ 0 };
        header[headerIndex++] = 0xFF & (size >> 24);
        header[headerIndex++] = 0xFF & (size >> 16);
        header[headerIndex++] = 0xFF & (size >> 8);
        header[headerIndex++] = 0xFF & size;

        header[headerIndex++] = 0xFF & cmd;
        header[headerIndex++] = 0xFF & (cmd >> 8);
    }

    uint8 GetHeaderLength()
    {
        // cmd = 2 bytes, size = 4 bytes
        return sizeof(header);
    }

    const uint32 size;
    uint8 header[sizeof(uint32) + sizeof(uint16)];
};

struct DiscordClientPktHeader
{
    uint32 size;
    uint16 cmd;

    bool IsValidSize() const { return size >= 2 && size < 10000; }
    bool IsValidOpcode() const { return cmd < NUM_DISCORD_CODE_HANDLERS; }
};
#pragma pack(pop)

#endif
