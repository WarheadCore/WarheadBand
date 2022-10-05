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

#ifndef __NODE_PACKET_HEADER_H__
#define __NODE_PACKET_HEADER_H__

#include "Define.h"
#include "Opcodes.h"

#pragma pack(push, 1)
struct NodeServerPktHeader
{
    /**
     * size is the length of the payload _plus_ the length of the opcode
     */
    NodeServerPktHeader(uint32 size, uint16 cmd, uint32 accountID) : size(size), accoundID(accountID)
    {
        uint8 headerIndex{ 0 };
        header[headerIndex++] = 0xFF & (size >> 24);
        header[headerIndex++] = 0xFF & (size >> 16);
        header[headerIndex++] = 0xFF & (size >> 8);
        header[headerIndex++] = 0xFF & size;

        header[headerIndex++] = 0xFF & cmd;
        header[headerIndex++] = 0xFF & (cmd >> 8);

        header[headerIndex++] = 0xFF & accountID;
        header[headerIndex++] = 0xFF & (accountID >> 8);
        header[headerIndex++] = 0xFF & (accountID >> 16);
        header[headerIndex++] = 0xFF & (accountID >> 24);
    }

    uint8 GetHeaderLength()
    {
        // accoundID = 4 bytes, cmd = 2 bytes, size = 4 bytes
        return sizeof(header);
    }

    const uint32 size;
    const uint32 accoundID;
    uint8 header[10];
};

struct NodeClientPktHeader
{
    uint32 size;
    uint16 cmd;
    uint32 accoundID;

    bool IsValidSize() const { return size >= 2 && size < 25000; }
    bool IsValidOpcode() const { return cmd < NUM_NODE_CODE_HANDLERS; }
    bool IsValidAccountForWorldSocket() const { return accoundID > 0; }
    bool IsValidAccountForNodeSocket() const { return accoundID == 0; }
};
#pragma pack(pop)

#endif
