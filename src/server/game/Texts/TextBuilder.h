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

#ifndef _TEXT_BUILDER_H_
#define _TEXT_BUILDER_H_

#include "AccountMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "GameLocale.h"
#include "Player.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include "World.h"
#include "WorldSession.h"

namespace Warhead::Text
{
    /// Send a System Message to all players (except self if mentioned)
    template<typename... Args>
    inline void SendWorldText(uint32 stringID, Args&&... args)
    {
        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                return;

            player->SendDirectMessage(stringID, std::forward<Args>(args)...);
        }
    }

    /// Send a System Message to all GMs (except self if mentioned)
    template<typename... Args>
    inline void SendGMText(uint32 stringID, Args&&... args)
    {
        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                return;

            if (AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()))
                continue;

            player->SendDirectMessage(stringID, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    inline void SendBattlegroundWarningToAll(Battleground* bg, uint32 stringID, Args&&... args)
    {
        if (!stringID || !bg)
            return;

        for (auto const& itr : bg->GetPlayers())
        {
            itr.second->SendDirectMessage(stringID, CHAT_MSG_RAID_BOSS_EMOTE, nullptr, nullptr, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    inline void SendBattlegroundMessageToAll(Battleground* bg, uint32 stringID, ChatMsg type, Args&&... args)
    {
        if (!stringID || !bg)
            return;

        for (auto const& itr : bg->GetPlayers())
        {
            itr.second->SendDirectMessage(stringID, type, nullptr, nullptr, std::forward<Args>(args)...);
        }
    }
}

#endif // _TEXT_BUILDER_H_
