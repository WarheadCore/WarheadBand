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

#include "Chat.h"
#include "GameTime.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptObject.h"

class PlayerInfoAtLogin_Player : public PlayerScript
{
public:
    PlayerInfoAtLogin_Player() : PlayerScript("PlayerInfoAtLogin_Player") {}

    void OnLogin(Player* player) override
    {
        if (!MOD_CONF_GET_BOOL("PlayerInfoAtLogin.Enable"))
            return;

        uint8 accountLevel = static_cast<uint8>(player->GetSession()->GetSecurity());

        // #1. Prefix
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_PREFIX");

        // #2. Welcome msg
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_HI", player->GetName());

        // #3. Account level if > 0
        if (accountLevel)
            sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_GM_LEVEL", accountLevel);

        // #4. IP address
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_IP", player->GetSession()->GetRemoteAddress());

        // #5. World online
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_ONLINE", sWorld->GetPlayerCount(), sWorld->GetMaxActiveSessionCount());

        // #6. World uptime
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_UPTIME", Warhead::Time::ToTimeString(GameTime::GetUptime()));

        // #7. Prefix again
        sModuleLocale->SendPlayerMessage(player, "PIAL_LOCALE_MSG_PREFIX");
    }
};

// Group all custom scripts
void AddSC_PlayerInfoAtLogin()
{
    new PlayerInfoAtLogin_Player();
}
