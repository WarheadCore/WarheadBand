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

#include "Battleground.h"
#include "Chat.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptObject.h"

class BGReward_Player : public BGScript
{
public:
    BGReward_Player() : BGScript("BGReward_Player") { }

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId winnerTeamId) override
    {
        if (!MOD_CONF_GET_BOOL("BGReward.Enable"))
            return;

        // Not reward on end arena
        if (bg->isArena())
            return;

        TeamId bgTeamId = player->GetBgTeamId();
        uint32 RewardCount = 0;

        bgTeamId == winnerTeamId ? RewardCount = MOD_CONF_GET_INT("BGReward.WinnerTeam.Count") : RewardCount = MOD_CONF_GET_INT("BGReward.LoserTeam.Count");

        switch (player->GetZoneId())
        {
            case 3277: // Warsong Gulch
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.WSG"), RewardCount);
                break;
            case 3358: // Arathi Basin
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.Arathi"), RewardCount);
                break;
            case 3820: // Eye of the Storm
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.Eye"), RewardCount);
                break;
            case 4710: // Isle of Conquest
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.Isle"), RewardCount);
                break;
            case 4384: // Strand of the Ancients
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.Ancients"), RewardCount);
                break;
            case 2597: // Alterac Valley
                player->AddItem(MOD_CONF_GET_INT("BGReward.ItemID.Alterac"), RewardCount);
                break;
            default:
                break;
        }
    }
};

// Group all custom scripts
void AddSC_BGReward()
{
    new BGReward_Player();
}
