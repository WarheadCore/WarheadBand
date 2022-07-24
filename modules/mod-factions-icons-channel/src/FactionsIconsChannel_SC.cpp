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

#include "AccountMgr.h"
#include "Channel.h"
#include "Chat.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptObject.h"
#include "StringFormat.h"

constexpr auto ICON_HORDE = "|TInterface\\PVPFrame\\PVP-Currency-Horde:18:18:-3:-3|t";
constexpr auto ICON_ALLIANCE = "|TInterface\\PVPFrame\\PVP-Currency-Alliance:18:18:-3:-3|t";

class FactionsIconsChannel_Player : public PlayerScript
{
public:
    FactionsIconsChannel_Player() : PlayerScript("FactionsIconsChannel_Player") { }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        if (!player || !channel)
            return;

        if (!MOD_CONF_GET_BOOL("ChannelIconFaction.Enable"))
            return;

        if (MOD_CONF_GET_BOOL("ChannelIconFaction.OnlyLFG") && !channel->IsLFG())
            return;

        if (!MOD_CONF_GET_BOOL("ChannelIconFaction.GM") && !AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()))
            return;

        msg = Warhead::StringFormat("{}", player->GetTeamId() == TEAM_HORDE ? ICON_HORDE : ICON_ALLIANCE);
    }
};

// Group all custom scripts
void AddSC_FactionsIconsChannel()
{
    new FactionsIconsChannel_Player();
}
