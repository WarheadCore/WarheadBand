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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ScriptMgr.h"
#include "AIOMgr.h"

void ScriptMgr::OnAddonMessage(Player* sender, std::string_view message)
{
    if (!sender)
        return;

    LuaVal mainTable = LuaVal::loads(std::string{ message });
    if (!mainTable.istable()) //Unable to parse or incorrect format
        return;

    //Call handlers from all blocks in order
    /*for (auto& itr : mainTable.tbl())
    {
        LuaVal& block = mainTable[1];
        if (!block.istable())
            continue;

        LuaVal& scriptKeyVal = block[2];
        LuaVal& handlerKeyVal = block[3];
        if (!block[1].isnumber() || scriptKeyVal.isnil() || handlerKeyVal.isnil())
            continue;

        if (AIOScript* aioScript = _aioHandlers->GetScript<AIOScript>(scriptKeyVal))
            aioScript->OnHandle(sender, handlerKeyVal, block);
    }*/

    for (size_t i = 1; i <= mainTable.tbl().size(); ++i)
    {
        LuaVal& block = mainTable[1];
        if (!block.istable())
            continue;

        LuaVal& scriptKeyVal = block[2];
        LuaVal& handlerKeyVal = block[3];
        if (!block[1].isnumber() || scriptKeyVal.isnil() || handlerKeyVal.isnil())
            continue;

        if (AIOScript* aioScript = sAIOScriptMgr->GetAIOScript(scriptKeyVal))
            aioScript->OnHandle(sender, handlerKeyVal, block);
    }
}
