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

#include "CFBG.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "GroupMgr.h"
#include "Opcodes.h"
#include "ScriptMgr.h"
#include "ReputationMgr.h"

// CFBG custom script
class CFBG_BG : public BGScript
{
public:
    CFBG_BG() : BGScript("CFBG_BG") {}

    void OnBattlegroundBeforeAddPlayer(Battleground* bg, Player* player) override
    {
        if (!sCFBG->IsEnableSystem() || !bg || bg->isArena() || !player)
        {
            return;
        }

        sCFBG->ValidatePlayerForBG(bg, player, player->GetBgTeamId());
    }

    void OnBattlegroundAddPlayer(Battleground* bg, Player* player) override
    {
        if (!sCFBG->IsEnableSystem() || bg->isArena())
        {
            return;
        }

        sCFBG->FitPlayerInTeam(player, true, bg);

        if (sCFBG->IsEnableResetCooldowns())
        {
            player->RemoveArenaSpellCooldowns(true);
        }
    }

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId /*winnerTeamId*/) override
    {
        if (!sCFBG->IsEnableSystem() || !bg || !player || bg->isArena())
        {
            return;
        }

        if (sCFBG->IsPlayerFake(player))
        {
            sCFBG->ClearFakePlayer(player);
        }
    }

    void OnBattlegroundRemovePlayerAtLeave(Battleground* bg, Player* player) override
    {
        if (!sCFBG->IsEnableSystem() || bg->isArena())
        {
            return;
        }

        sCFBG->FitPlayerInTeam(player, false, bg);

        if (sCFBG->IsPlayerFake(player))
        {
            sCFBG->ClearFakePlayer(player);
        }
    }

    void OnAddGroup(BattlegroundQueue* queue, GroupQueueInfo* ginfo, uint32& index, Player* /*leader*/, Group* /*group*/, BattlegroundTypeId /* bgTypeId */, PvPDifficultyEntry const* /* bracketEntry */,
        uint8 /* arenaType */, bool /* isRated */, bool /* isPremade */, uint32 /* arenaRating */, uint32 /* matchmakerRating */, uint32 /* arenaTeamId */, uint32 /* opponentsArenaTeamId */) override
    {
        if (!queue)
        {
            return;
        }

        if (sCFBG->IsEnableSystem() && !ginfo->ArenaType && !ginfo->IsRated)
        {
            index = BG_QUEUE_CFBG;
        }
    }

    bool CanFillPlayersToBG(BattlegroundQueue* queue, Battleground* bg, BattlegroundBracketId bracket_id) override
    {
        if (!sCFBG->IsEnableSystem() || bg->isArena())
        {
            return true;
        }

        if (sCFBG->FillPlayersToCFBG(queue, bg, bracket_id))
        {
            return false;
        }

        return true;
    }

    bool IsCheckNormalMatch(BattlegroundQueue* queue, Battleground* bg, BattlegroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers) override
    {
        if (!sCFBG->IsEnableSystem() || bg->isArena())
        {
            return false;
        }

        return sCFBG->CheckCrossFactionMatch(queue, bracket_id, minPlayers, maxPlayers);
    }

    bool CanSendMessageBGQueue(BattlegroundQueue* queue, Player* leader, Battleground* bg, PvPDifficultyEntry const* bracketEntry) override
    {
        if (bg->isArena() || !sCFBG->IsEnableSystem())
        {
            // if it's arena OR the CFBG is disabled, let the core handle the announcement
            return true;
        }

        // otherwise, let the CFBG module handle the announcement
        sCFBG->SendMessageQueue(queue, bg, bracketEntry, leader);
        return false;
    }
};

class CFBG_Player : public PlayerScript
{
public:
    CFBG_Player() : PlayerScript("CFBG_Player") { }

    void OnLogin(Player* player) override
    {
        if (!sCFBG->IsEnableSystem())
        {
            return;
        }

        if (player->GetTeamId(true) != player->GetBgTeamId())
        {
            sCFBG->FitPlayerInTeam(player, player->GetBattleground() && !player->GetBattleground()->isArena(), player->GetBattleground());
        }
    }

    bool CanJoinInBattlegroundQueue(Player* player, ObjectGuid /*BattlemasterGuid*/ , BattlegroundTypeId /*BGTypeID*/, uint8 joinAsGroup, GroupJoinBattlegroundResult& err) override
    {
        if (!sCFBG->IsEnableSystem())
            return true;

        if (joinAsGroup)
        {
            Group* group = player->GetGroup();
            if (!group)
                return true;

            if (group->isRaidGroup() || group->GetMembersCount() > sCFBG->GetMaxPlayersCountInGroup())
                err = ERR_BATTLEGROUND_JOIN_FAILED;

            return false;
        }

        return true;
    }

    void OnBeforeUpdate(Player* player, uint32 diff) override
    {
        if (timeCheck <= diff)
        {
            sCFBG->UpdateForget(player);
            timeCheck = 10000;
        }
        else
            timeCheck -= diff;
    }

    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& /*msg*/) override
    {
        if (!player || !sCFBG->IsEnableSystem())
            return;

        Battleground* bg = player->GetBattleground();

        if (!bg || bg->isArena())
            return;

        // skip addon lang and universal
        if (lang == LANG_UNIVERSAL || lang == LANG_ADDON)
            return;

        // skip addon and system message
        if (type == CHAT_MSG_ADDON || type == CHAT_MSG_SYSTEM)
            return;

        // to gm lang
        lang = LANG_UNIVERSAL;
    }

    bool OnReputationChange(Player* player, uint32 factionID, int32& standing, bool /*incremental*/) override
    {
        uint32 repGain = player->GetReputation(factionID);
        TeamId teamId = player->GetTeamId(true);

        if ((factionID == FACTION_FROSTWOLF_CLAN && teamId == TEAM_ALLIANCE) ||
            (factionID == FACTION_STORMPIKE_GUARD && teamId == TEAM_HORDE))
        {
            uint32 diff = standing - repGain;
            player->GetReputationMgr().ModifyReputation(sFactionStore.LookupEntry(teamId == TEAM_ALLIANCE ? FACTION_STORMPIKE_GUARD : FACTION_FROSTWOLF_CLAN), diff);
            return false;
        }

        return true;
    }

private:
    uint32 timeCheck = 10000;
};

class CFBG_World : public WorldScript
{
public:
    CFBG_World() : WorldScript("CFBG_World") { }

    void OnAfterConfigLoad(bool /*Reload*/) override
    {
        sCFBG->LoadConfig();
    }
};

void AddSC_CFBG()
{
    new CFBG_BG();
    new CFBG_Player();
    new CFBG_World();
}
