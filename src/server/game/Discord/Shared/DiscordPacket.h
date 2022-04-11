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

#ifndef WARHEAD_DISCORD_PACKET_H_
#define WARHEAD_DISCORD_PACKET_H_

#include "ByteBuffer.h"
#include "DiscordSharedDefines.h"

class DiscordPacket : public ByteBuffer
{
public:
    // just container for later use
    DiscordPacket() : ByteBuffer(0) { }

    explicit DiscordPacket(uint16 opcode, size_t res = 1) :
        ByteBuffer(res), _opcode(opcode) { }

    DiscordPacket(DiscordPacket&& packet) noexcept :
        ByteBuffer(std::move(packet)), _opcode(packet._opcode) { }

    DiscordPacket(DiscordPacket const& right) :
        ByteBuffer(right), _opcode(right._opcode) { }

    DiscordPacket& operator=(DiscordPacket const& right)
    {
        if (this != &right)
        {
            _opcode = right._opcode;
            ByteBuffer::operator=(right);
        }

        return *this;
    }

    DiscordPacket& operator=(DiscordPacket&& right) noexcept
    {
        if (this != &right)
        {
            _opcode = right._opcode;
            ByteBuffer::operator=(std::move(right));
        }

        return *this;
    }

    DiscordPacket(uint16 opcode, MessageBuffer&& buffer) :
        ByteBuffer(std::move(buffer)), _opcode(opcode) { }

    void Initialize(uint16 opcode, size_t newres = 200)
    {
        clear();
        _storage.reserve(newres);
        _opcode = opcode;
    }

    [[nodiscard]] uint16 GetOpcode() const { return _opcode; }
    void SetOpcode(uint16 opcode) { _opcode = opcode; }

protected:
    uint16 _opcode{ NULL_DISCORD_CODE };
};
#endif
