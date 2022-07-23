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

#include "Chat.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "MuteMgr.h"
#include "Player.h"
#include "ScriptObject.h"

class NotifyMuted_Player : public PlayerScript
{
public:
    NotifyMuted_Player() : PlayerScript("NotifyMuted_Player") {}

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, Player* receiver) override
    {
        if (!MOD_CONF_GET_BOOL("NotCanSpeakMsg.Enable"))
            return;

        if (receiver->GetSession()->CanSpeak())
            return;

        sModuleLocale->SendPlayerMessage(player, "NM_LOCALE_MESSAGE", ChatHandler(receiver->GetSession()).playerLink(receiver->GetName()), sMute->GetMuteTimeString(receiver->GetSession()->GetAccountId()));
    }
};

// Group all custom scripts
void AddSC_NotifyMuted()
{
    new NotifyMuted_Player();
}
