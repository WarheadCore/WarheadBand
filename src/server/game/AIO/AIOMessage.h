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

#ifndef _AIO_MESSAGE_H_
#define _AIO_MESSAGE_H_

#include "Define.h"
#include <smallfolk.h>
#include <type_traits>

class WH_GAME_API AIOMessage
{
public:
    AIOMessage() = default;
    ~AIOMessage() = default;

    template<typename... Ts>
    AIOMessage(LuaVal const& scriptKey, LuaVal const& handlerKey, Ts&&... pack)
    {
        Add(scriptKey, handlerKey, std::forward<Ts>(pack)...);
    }

    template<typename... Ts>
    inline std::enable_if_t<std::conjunction_v<std::is_same<LuaVal, std::decay_t<Ts>>...>> Add(LuaVal const& scriptKey, LuaVal const& handlerKey, Ts&&... pack)
    {
        LuaVal block(TTABLE);
        uint32 index{ 1 };

        block[1] = 0;
        block[2] = scriptKey;
        block[3] = handlerKey;

        (AddValueToTable(block, index, std::forward<Ts>(pack)), ...);
        block[1] = index;
        _val.insert(block);
    }

    //Appends the last block
    //You can add additional arguments to the last block
    template<typename... Ts>
    inline std::enable_if_t<std::conjunction_v<std::is_same<LuaVal, std::decay_t<Ts>>...>> AppendLast(Ts&&... pack)
    {
        unsigned int lastBlock = _val.len();
        if (!lastBlock)
            return;

        LuaVal& block = _val[lastBlock];
        LuaVal indexNum = block.get(1);
        if (!indexNum.isnumber())
            return;

        uint32 index = static_cast<uint32>(indexNum.num());
        (AddValueToTable(block, index, std::forward<Ts>(pack)), ...);
        block[1] = index;
    }

    //Returns smallfolk dump of the AIO message
    std::string dumps() const { return _val.dumps(); }

private:
    void AddValueToTable(LuaVal& block, uint32& index, LuaVal const& val)
    {
        if (val.isnil())
            return;

        block.insert(val);
        index++;
    }

    LuaVal _val{ TTABLE };

    AIOMessage(AIOMessage const&) = delete;
    AIOMessage(AIOMessage&&) = delete;
    AIOMessage& operator=(AIOMessage const&) = delete;
    AIOMessage& operator=(AIOMessage&&) = delete;
};

#endif //
