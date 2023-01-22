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

#include "GameMoney.h"
#include "ByteBuffer.h"
#include "StringFormat.h"
#include <limits>

constexpr auto MAX_MONEY_COUNT = std::numeric_limits<int32>::max() - 1; // 0x7FFFFFFF - 1

std::string MoneyToString(Copper money, bool isShort /*= true*/)
{
    auto gold{ money.GetCount() / 10000 };
    auto silver{ (money.GetCount() % 10000) / 100 };
    auto copper{ (money.GetCount() % 10000) % 100 };

    std::string out;

    if (gold)
        out.append(Warhead::StringFormat("{}{} ", gold, isShort ? "g" : gold == 1 ? " gold" : " golds"));

    if (silver)
        out.append(Warhead::StringFormat("{}{} ", silver, isShort ? "s" : silver == 1 ? " silver" : " silvers"));

    if (copper)
        out.append(Warhead::StringFormat("{}{} ", copper, isShort ? "c" : copper == 1 ? " copper" : " coppers"));

    return std::string{ Warhead::String::TrimRight(out) };
}

template<uint32 Multiplier>
bool Money<Multiplier>::IsMax() const
{
    return _money == MAX_MONEY_COUNT;
}

template<uint32 Multiplier>
bool Money<Multiplier>::IsOverMax() const
{
    return _money >= MAX_MONEY_COUNT;
}

ByteBuffer& operator<<(ByteBuffer& buf, Copper const& money)
{
    buf << uint32(money.GetCount());
    return buf;
}

ByteBuffer& operator>>(ByteBuffer& buf, Copper& money)
{
    money.Set(buf.read<uint32>());
    return buf;
}
