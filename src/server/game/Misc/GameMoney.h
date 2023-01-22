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

#ifndef WARHEAD_GAME_MONEY_H_
#define WARHEAD_GAME_MONEY_H_

#include "Define.h"
#include <string>
#include <type_traits>

class ByteBuffer;

template<uint32 Multiplier1, uint32 Multiplier2>
constexpr bool IsConvertibleMultipliers()
{
    return Multiplier1 <= Multiplier2;
}

namespace MoneyTraits
{
    template<uint32 MultiplierLeft, uint32 MultiplierRight>
    using IsConvertible = std::integral_constant<bool, IsConvertibleMultipliers<MultiplierLeft, MultiplierRight>()>;
}

template<uint32 Multiplier>
struct WH_GAME_API Money
{
    static_assert(Multiplier == 10000 || Multiplier == 100 || Multiplier == 1, "Multiplier is incorrect");

public:
    constexpr Money() = default;

    constexpr explicit Money(uint32 count) :
        _money(count) { }

    template<uint32 MultiplierRight, std::enable_if_t<MoneyTraits::IsConvertible<Multiplier, MultiplierRight>::value, std::nullptr_t> = nullptr>
    constexpr Money(Money<MultiplierRight> const& money) :
        _money(money.GetCopper() / Multiplier) { }

    [[nodiscard]] constexpr uint32 GetCount() const { return _money; }
    [[nodiscard]] constexpr uint32 GetCopper() const { return _money * Multiplier; }
    inline void Set(uint32 money) { _money = money; }
    inline void Clear() { _money = 0; }

    [[nodiscard]] bool IsMax() const;
    [[nodiscard]] bool IsOverMax() const;
    [[nodiscard]] constexpr bool IsEmpty() const { return _money == 0; }
    constexpr explicit operator bool() const { return !IsEmpty(); }
    constexpr bool operator!() const { return !(bool(*this)); }

    template<uint32 MultiplierRight>
    constexpr bool operator==(Money<MultiplierRight> const& right) const
    {
        return GetCopper() == right.GetCopper();
    }

    template<uint32 MultiplierRight>
    constexpr auto operator<=>(Money<MultiplierRight> const& right) const
    {
        return GetCopper() <=> right.GetCopper();
    }

    template<uint32 MultiplierRight, std::enable_if_t<MoneyTraits::IsConvertible<Multiplier, MultiplierRight>::value, std::nullptr_t> = nullptr>
    constexpr Money& operator+=(Money<MultiplierRight> const& right)
    {
        _money += right.GetCopper() / Multiplier;
        return *this;
    }

    constexpr Money& operator-=(Money<Multiplier> const& right) {_money -= right.GetCopper() / Multiplier; return *this; }

    constexpr Money& operator*=(uint32 right) { _money -= right; return *this; }
    constexpr Money& operator/=(uint32 right) { _money /= right; return *this; }
    constexpr Money& operator%=(uint32 right) { _money %= right; return *this; }
    constexpr auto operator*(uint32 right) { _money *= right; return Money<Multiplier>(_money); }
    constexpr auto operator/(uint32 right) { _money /= right; return Money<Multiplier>(_money); }
    constexpr auto operator%(uint32 right) { _money %= right; return Money<Multiplier>(_money); }

private:
    uint32 _money{};
};

template<uint32 MultiplierLeft, uint32 MultiplierRight>
constexpr auto operator+(Money<MultiplierLeft> left, Money<MultiplierRight> right) -> Money<MultiplierLeft <= MultiplierRight ? MultiplierLeft : MultiplierRight>
{
    constexpr uint32 NewRatio = MultiplierLeft <= MultiplierRight ? MultiplierLeft : MultiplierRight;
    uint32 leftCopper = left.GetCopper();
    uint32 rightCopper = right.GetCopper();
    return Money<NewRatio>((leftCopper + rightCopper) / NewRatio);
}

template<uint32 MultiplierLeft, uint32 MultiplierRight, std::enable_if_t<MoneyTraits::IsConvertible<MultiplierRight, MultiplierLeft>::value, std::nullptr_t> = nullptr>
constexpr auto operator-(Money<MultiplierLeft> left, Money<MultiplierRight> right) -> Money<MultiplierLeft <= MultiplierRight ? MultiplierLeft : MultiplierRight>
{
    constexpr uint32 NewRatio = MultiplierLeft <= MultiplierRight ? MultiplierLeft : MultiplierRight;
    uint32 leftCopper = left.GetCopper();
    uint32 rightCopper = right.GetCopper();
    return Money<NewRatio>((leftCopper - rightCopper) / NewRatio);
}

using Gold = Money<10000>;
using Silver = Money<100>;
using Copper = Money<1>;

constexpr Gold operator""_gold(unsigned long long value)
{
    return Gold(value);
}

constexpr Silver operator""_silver(unsigned long long value)
{
    return Silver(value);
}

constexpr Copper operator""_copper(unsigned long long value)
{
    return Copper(value);
}

WH_GAME_API ByteBuffer& operator<<(ByteBuffer& buf, Copper const& money);
WH_GAME_API ByteBuffer& operator>>(ByteBuffer& buf, Copper& money);

std::string MoneyToString(Copper money, bool isShort /*= true*/);

namespace std
{
    template<uint32 Multiplier>
    struct hash<Money<Multiplier>>
    {
    public:
        std::size_t operator()(Money<Multiplier> const& key) const
        {
            return std::hash<uint32>()(key.GetCopper());
        }
    };
}

//namespace fmt
//{
//    template<uint32 Multiplier>
//    struct formatter<Money<Multiplier>> : formatter<std::string>
//    {
//        // parse is inherited from formatter<string_view>.
//        template <typename FormatContext>
//        auto format(Money<Multiplier> money, FormatContext& ctx)
//        {
//            std::string info = "<unknown money count>";
//
//            if (!money.IsEmpty())
//                info = MoneyToString(money);
//
//            return formatter<std::string>::format(info, ctx);
//        }
//    };
//}

#endif
