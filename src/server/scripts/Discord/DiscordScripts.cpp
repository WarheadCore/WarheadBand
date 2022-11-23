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

#include "DiscordMgr.h"
#include "ScriptObject.h"

class Discord_Player : public PlayerScript
{
public:
    Discord_Player() : PlayerScript("Discord_Player") { }

    void OnLogin(Player* player) override
    {
        sDiscordMgr->LogLogin(player);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        sDiscordMgr->LogChat(player, msg, channel);
    }
};

// Group all
void AddSC_Discord()
{
    new Discord_Player();
}
