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
#include "Chat.h"
#include "Common.h"
#include "GameLocale.h"
#include "Object.h"
#include "Player.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include "World.h"
#include "WorldSession.h"
#include <unordered_map>
#include <vector>

namespace Warhead::Text
{
    template<typename... Args>
    inline std::string GetLocaleText(uint32 stringID, uint8 localeIndex, Args&&... args)
    {
        return StringFormat(sGameLocale->GetWarheadString(stringID, LocaleConstant(localeIndex)), std::forward<Args>(args)...);
    }

    template<typename Worker, typename... Args>
    inline void DoLocalizedPacket(uint32 stringID, uint8 localeIndex, ChatMsg type, WorldObject const* sender, WorldObject const* receiver, Worker&& worker, Args&&... args)
    {
        std::string text = GetLocaleText(stringID, localeIndex, std::forward<Args>(args)...);

        for (std::string_view line : Warhead::Tokenize(text, '\n', true))
        {
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, type, LANG_UNIVERSAL, sender, receiver, line);
            worker(&data);
        }
    }

    /// Send a System Message to all players (except self if mentioned)
    template<typename... Args>
    void SendWorldText(uint32 string_id, Args&&... args)
    {
        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                return;

            DoLocalizedPacket(string_id, player->GetSession()->GetSessionDbLocaleIndex(), CHAT_MSG_SYSTEM, nullptr, nullptr, [player](WorldPacket* data)
            {
                player->SendDirectMessage(data);
            }, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void SendGMText(uint32 string_id, Args&&... args)
    {
        for (auto const& [accountId, session] : sWorld->GetAllSessions())
        {
            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                return;

            if (AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()))
                continue;

            DoLocalizedPacket(string_id, player->GetSession()->GetSessionDbLocaleIndex(), CHAT_MSG_SYSTEM, nullptr, nullptr, [player](WorldPacket* data)
            {
                player->SendDirectMessage(data);
            }, std::forward<Args>(args)...);
        }
    }
}

#endif // _TEXT_BUILDER_H_
