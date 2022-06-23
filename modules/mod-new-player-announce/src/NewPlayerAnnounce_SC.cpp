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

#include "ScriptMgr.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ModuleLocale.h"

class NewPlayerAnnounce_Player : public PlayerScript
{
public:
    NewPlayerAnnounce_Player() : PlayerScript("NewPlayerAnnounce_Player") { }

    void OnFirstLogin(Player* player) override
    {
        if (!MOD_CONF_GET_BOOL("NPA.Enable"))
            return;

        sModuleLocale->SendGlobalMessage(false, "NPA_MESSAGE", player->GetName());
    }
};

// Group all custom scripts
void AddSC_NewPlayerAnnounce()
{
    new NewPlayerAnnounce_Player();
}
