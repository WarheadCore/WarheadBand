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

#include "TextBuilder.h"
#include "Battleground.h"
#include "Chat.h"
#include "Player.h"
#include "Tokenize.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
    void SendPlayerMessage(Player* player, ChatMsg type, std::string_view message)
    {
        for (std::string_view line : Warhead::Tokenize(message, '\n', true))
        {
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, type, LANG_UNIVERSAL, nullptr, nullptr, line);
            player->SendDirectMessage(&data);
        }
    }

    // Default send message
    void SendPlayerMessage(Player* player, std::string_view message)
    {
        SendPlayerMessage(player, CHAT_MSG_SYSTEM, message);
    }
}

void Warhead::Text::SendWorldTextFmt(WarheadFmtText const& msg)
{
    if (!msg)
    {
        //LOG_ERROR("", "> SendWorldText. Empty message. Skip");
        return;
    }

    for (auto const& [accountID, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            return;

        SendPlayerMessage(player, msg(player->GetSession()->GetSessionDbLocaleIndex()));
    }
}

void Warhead::Text::SendGMTextFmt(WarheadFmtText const& msg)
{
    if (!msg)
    {
        //LOG_ERROR("", "> SendGMText. Empty message. Skip");
        return;
    }

    for (auto const& [accountID, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            return;

        if (AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()))
            continue;

        SendPlayerMessage(player, msg(player->GetSession()->GetSessionDbLocaleIndex()));
    }
}

void Warhead::Text::SendBattlegroundWarningToAllFmt(Battleground* bg, WarheadFmtText const& msg)
{
    if (!bg)
        return;

    for (auto const& itr : bg->GetPlayers())
    {
        SendPlayerMessage(itr.second, CHAT_MSG_RAID_BOSS_EMOTE, msg(itr.second->GetSession()->GetSessionDbLocaleIndex()));
    }
}

void Warhead::Text::SendBattlegroundMessageToAllFmt(Battleground* bg, ChatMsg type, WarheadFmtText const& msg)
{
    if (!bg)
        return;

    for (auto const& itr : bg->GetPlayers())
    {
        SendPlayerMessage(itr.second, type, msg(itr.second->GetSession()->GetSessionDbLocaleIndex()));
    }
}
