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
#include "Containers.h"
#include "GroupMgr.h"
#include "Language.h"
#include "Log.h"
#include "Opcodes.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "GameTime.h"
#include "ModulesConfig.h"
#include "GameConfig.h"
#include "ChatTextBuilder.h"

constexpr uint32 FactionFrostwolfClan  = 729;
constexpr uint32 FactionStormpikeGuard = 730;
constexpr uint32 MapAlteracValley = 30;

CrossFactionGroupInfo::CrossFactionGroupInfo(GroupQueueInfo* groupInfo)
{
    uint32 sumLevels = 0;
    uint32 sumAverageItemLevels = 0;
    uint32 playersCount = 0;

    for (auto const& playerGuid : groupInfo->Players)
    {
        auto player = ObjectAccessor::FindConnectedPlayer(playerGuid);
        if (!player)
            continue;

        if (player->getClass() == CLASS_HUNTER && !IsHunterJoining)
            IsHunterJoining = true;

        sumLevels += player->GetAverageItemLevel();
        sumAverageItemLevels += player->getLevel();
        playersCount++;
    }

    if (!playersCount)
        return;

    AveragePlayersLevel = sumLevels / playersCount;
    AveragePlayersItemLevel = sumAverageItemLevels / playersCount;
}

CFBG* CFBG::instance()
{
    static CFBG instance;
    return &instance;
}

void CFBG::LoadConfig()
{
    _IsEnableSystem = sModulesConfig->GetOption<bool>("CFBG.Enable", false);
    _IsEnableAvgIlvl = sModulesConfig->GetOption<bool>("CFBG.Include.Avg.Ilvl.Enable", false);
    _IsEnableBalancedTeams = sModulesConfig->GetOption<bool>("CFBG.BalancedTeams", false);
    _IsEnableEvenTeams = sModulesConfig->GetOption<bool>("CFBG.EvenTeams.Enabled", false);
    _IsEnableBalanceClassLowLevel = sModulesConfig->GetOption<bool>("CFBG.BalancedTeams.Class.LowLevel", true);
    _IsEnableResetCooldowns = sModulesConfig->GetOption<bool>("CFBG.ResetCooldowns", false);
    _showPlayerName = sModulesConfig->GetOption<bool>("CFBG.Show.PlayerName", false);
    _EvenTeamsMaxPlayersThreshold = sModulesConfig->GetOption<uint32>("CFBG.EvenTeams.MaxPlayersThreshold", 5);
    _MaxPlayersCountInGroup = sModulesConfig->GetOption<uint32>("CFBG.Players.Count.In.Group", 3);
    _balanceClassMinLevel = sModulesConfig->GetOption<uint8>("CFBG.BalancedTeams.Class.MinLevel", 10);
    _balanceClassMaxLevel = sModulesConfig->GetOption<uint8>("CFBG.BalancedTeams.Class.MaxLevel", 19);
    _balanceClassLevelDiff = sModulesConfig->GetOption<uint8>("CFBG.BalancedTeams.Class.LevelDiff", 2);
}

uint32 CFBG::GetBGTeamAverageItemLevel(Battleground* bg, TeamId team)
{
    if (!bg)
    {
        return 0;
    }

    uint32 sum = 0;
    uint32 count = 0;

    for (auto const& [playerGuid, player] : bg->GetPlayers())
    {
        if (player && player->GetTeamId() == team)
        {
            sum += player->GetAverageItemLevel();
            count++;
        }
    }

    if (!count || !sum)
    {
        return 0;
    }

    return sum / count;
}

uint32 CFBG::GetBGTeamSumPlayerLevel(Battleground* bg, TeamId team)
{
    if (!bg)
    {
        return 0;
    }

    uint32 sum = 0;

    for (auto const& [playerGuid, player] : bg->GetPlayers())
    {
        if (player && player->GetTeamId() == team)
        {
            sum += player->getLevel();
        }
    }

    return sum;
}

TeamId CFBG::GetLowerTeamIdInBG(Battleground* bg, GroupQueueInfo* groupInfo)
{
    int32 plCountA = bg->GetPlayersCountByTeam(TEAM_ALLIANCE);
    int32 plCountH = bg->GetPlayersCountByTeam(TEAM_HORDE);
    uint32 diff = std::abs(plCountA - plCountH);

    if (IsEnableBalancedTeams())
    {
        return SelectBgTeam(bg, groupInfo);
    }

    if (diff)
    {
        return plCountA < plCountH ? TEAM_ALLIANCE : TEAM_HORDE;
    }

    if (IsEnableAvgIlvl() && !IsAvgIlvlTeamsInBgEqual(bg))
    {
        return GetLowerAvgIlvlTeamInBg(bg);
    }

    return urand(0, 1) ? TEAM_ALLIANCE : TEAM_HORDE;
}

TeamId CFBG::SelectBgTeam(Battleground* bg, GroupQueueInfo* groupInfo)
{
    uint32 allianceLevels = GetBGTeamSumPlayerLevel(bg, TeamId::TEAM_ALLIANCE);
    uint32 hordeLevels = GetBGTeamSumPlayerLevel(bg, TeamId::TEAM_HORDE);

    // First select team - where the sum of the levels is less
    TeamId team = (allianceLevels < hordeLevels) ? TEAM_ALLIANCE : TEAM_HORDE;

    ASSERT(groupInfo);

    // Config option `CFBG.EvenTeams.Enabled = 1`
    // if players in queue is equal to an even number
    if (IsEnableEvenTeams() && groupInfo->Players.size() % 2 == 0)
    {
        auto cfGroupInfo = CrossFactionGroupInfo(groupInfo);
        auto playerLevel = cfGroupInfo.AveragePlayersLevel;

        // if CFBG.BalancedTeams.LowLevelClass is enabled, check the quantity of hunter per team if the player is an hunter
        if (IsEnableBalanceClassLowLevel() &&
            (playerLevel >= _balanceClassMinLevel && playerLevel <= _balanceClassMaxLevel) &&
            (playerLevel >= getBalanceClassMinLevel(bg)) &&
            (cfGroupInfo.IsHunterJoining)) // if the current player is hunter OR there is a hunter in the joining queue while a non-hunter player is joining
        {
            team = getTeamWithLowerClass(bg, CLASS_HUNTER);
        }
        else
        {
            auto playerCountH = bg->GetPlayersCountByTeam(TEAM_HORDE);
            auto playerCountA = bg->GetPlayersCountByTeam(TEAM_ALLIANCE);

            // We need to have a diff of 0.5f
            // Range of calculation: [minBgLevel, maxBgLevel], i.e: [10,20)
            float avgLvlAlliance = allianceLevels / (float)playerCountA;
            float avgLvlHorde = hordeLevels / (float)playerCountH;

            if (std::abs(avgLvlAlliance - avgLvlHorde) >= 0.5f)
            {
                team = avgLvlAlliance < avgLvlHorde ? TEAM_ALLIANCE : TEAM_HORDE;
            }
            else // it's balanced, so we should only check the ilvl
                team = GetLowerAvgIlvlTeamInBg(bg);
        }
    }
    else if (allianceLevels == hordeLevels)
    {
        team = GetLowerAvgIlvlTeamInBg(bg);
    }

    return team;
}

uint8 CFBG::getBalanceClassMinLevel(const Battleground* bg) const
{
    return static_cast<uint8>(bg->GetMaxLevel()) - _balanceClassLevelDiff;
}

TeamId CFBG::getTeamWithLowerClass(Battleground *bg, Classes c)
{
    uint16 hordeClassQty = 0;
    uint16 allianceClassQty = 0;

    for (auto const& [playerGuid, player] : bg->GetPlayers())
    {
        if (player && player->getClass() == c)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                allianceClassQty++;
            }
            else
            {
                hordeClassQty++;
            }
        }
    }

    return hordeClassQty > allianceClassQty ? TEAM_ALLIANCE : TEAM_HORDE;
}

TeamId CFBG::GetLowerAvgIlvlTeamInBg(Battleground* bg)
{
    return (GetBGTeamAverageItemLevel(bg, TeamId::TEAM_ALLIANCE) < GetBGTeamAverageItemLevel(bg, TeamId::TEAM_HORDE)) ? TEAM_ALLIANCE : TEAM_HORDE;
}

bool CFBG::IsAvgIlvlTeamsInBgEqual(Battleground* bg)
{
    return GetBGTeamAverageItemLevel(bg, TeamId::TEAM_ALLIANCE) == GetBGTeamAverageItemLevel(bg, TeamId::TEAM_HORDE);
}

void CFBG::ValidatePlayerForBG(Battleground* bg, Player* player, TeamId teamId)
{
    if (player->GetTeamId(true) == teamId)
        return;

    BGData& bgdata = player->GetBGData();

    if (bgdata.bgTeamId != teamId)
        bgdata.bgTeamId = teamId;

    SetFakeRaceAndMorph(player);

    if (bg->GetMapId() == MapAlteracValley)
    {
        if (teamId == TEAM_HORDE)
        {
            player->GetReputationMgr().ApplyForceReaction(FactionFrostwolfClan, REP_FRIENDLY, true);
            player->GetReputationMgr().ApplyForceReaction(FactionStormpikeGuard, REP_HOSTILE, true);
        }
        else
        {
            player->GetReputationMgr().ApplyForceReaction(FactionFrostwolfClan, REP_HOSTILE, true);
            player->GetReputationMgr().ApplyForceReaction(FactionStormpikeGuard, REP_FRIENDLY, true);
        }

        player->GetReputationMgr().SendForceReactions();
    }
}

uint32 CFBG::GetAllPlayersCountInBG(Battleground* bg)
{
    return bg->GetPlayersSize();
}

uint8 CFBG::GetRandomRace(std::initializer_list<uint32> races)
{
    return Warhead::Containers::SelectRandomContainerElement(races);
}

uint32 CFBG::GetMorphFromRace(uint8 race, uint8 gender)
{
    if (gender == GENDER_MALE)
    {
        switch (race)
        {
            case RACE_ORC:
                return FAKE_M_FEL_ORC;
            case RACE_DWARF:
                return FAKE_M_DWARF;
            case RACE_NIGHTELF:
                return FAKE_M_NIGHT_ELF;
            case RACE_DRAENEI:
                return FAKE_M_BROKEN_DRAENEI;
            case RACE_TROLL:
                return FAKE_M_TROLL;
            case RACE_HUMAN:
                return FAKE_M_HUMAN;
            case RACE_BLOODELF:
                return FAKE_M_BLOOD_ELF;
            case RACE_GNOME:
                return FAKE_M_GNOME;
            case RACE_TAUREN:
                return FAKE_M_TAUREN;
            default:
                return FAKE_M_BLOOD_ELF; // this should never happen, it's to fix a warning about return value
        }
    }
    else
    {
        switch (race)
        {
            case RACE_ORC:
                return FAKE_F_ORC;
            case RACE_DRAENEI:
                return FAKE_F_DRAENEI;
            case RACE_HUMAN:
                return FAKE_F_HUMAN;
            case RACE_BLOODELF:
                return FAKE_F_BLOOD_ELF;
            case RACE_GNOME:
                return FAKE_F_GNOME;
            case RACE_TAUREN:
                return FAKE_F_TAUREN;
            default:
                return FAKE_F_BLOOD_ELF; // this should never happen, it's to fix a warning about return value
        }
    }
}

void CFBG::RandomRaceMorph(uint8* race, uint32* morph, TeamId team, uint8 _class, uint8 gender)
{
    // if alliance find a horde race
    if (team == TEAM_ALLIANCE)
    {
        // default race because UNDEAD morph is missing
        *race = RACE_BLOODELF;

        /*
        * TROLL FEMALE morph is missing
        * therefore MALE and FEMALE are handled in a different way
        *
        * UNDEAD is missing too but for both gender
        */
        if (gender == GENDER_MALE)
        {
            *morph = FAKE_M_BLOOD_ELF;

            switch (_class)
            {
                case CLASS_DRUID:
                    *race = RACE_TAUREN;
                    *morph = FAKE_M_TAUREN;
                    break;
                case CLASS_SHAMAN:
                case CLASS_WARRIOR:
                    // UNDEAD missing (only for WARRIOR)
                    *race = GetRandomRace({ RACE_ORC, RACE_TAUREN, RACE_TROLL });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_PALADIN:
                    // BLOOD ELF, so default race
                    break;
                case CLASS_HUNTER:
                case CLASS_DEATH_KNIGHT:
                    // UNDEAD missing (only for DEATH_KNIGHT)
                    *race = GetRandomRace({ RACE_ORC, RACE_TAUREN, RACE_TROLL, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_ROGUE:
                    // UNDEAD missing
                    *race = GetRandomRace({ RACE_ORC, RACE_TROLL, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_MAGE:
                case CLASS_PRIEST:
                    // UNDEAD missing
                    *race = GetRandomRace({ RACE_TROLL, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_WARLOCK:
                    // UNDEAD missing
                    *race = GetRandomRace({ RACE_ORC, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
            }
        }
        else
        {
            *morph = FAKE_F_BLOOD_ELF;

            switch (_class)
            {
                case CLASS_DRUID:
                    *race = RACE_TAUREN;
                    *morph = FAKE_F_TAUREN;
                    break;
                case CLASS_SHAMAN:
                case CLASS_WARRIOR:
                    // UNDEAD missing (only for WARRIOR)
                    // TROLL FEMALE missing
                    *race = GetRandomRace({ RACE_ORC, RACE_TAUREN });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_HUNTER:
                case CLASS_DEATH_KNIGHT:
                    // TROLL FEMALE is missing
                    // UNDEAD is missing (only for DEATH_KNIGHT)
                    *race = GetRandomRace({ RACE_ORC, RACE_TAUREN, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_ROGUE:
                case CLASS_WARLOCK:
                    // UNDEAD is missing
                    // TROLL FEMALE is missing (only for Rogue)
                    *race = GetRandomRace({ RACE_ORC, RACE_BLOODELF });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_PALADIN:
                    // BLOOD ELF, so default race
                case CLASS_MAGE:
                case CLASS_PRIEST:
                    // UNDEAD and TROLL FEMALE morph are missing so use BLOOD ELF (default race)
                    break;
            }
        }

    }
    else // otherwise find an alliance race
    {
        // default race
        *race = RACE_HUMAN;

        /*
        * FEMALE morphs DWARF and NIGHT ELF are missing
        * therefore MALE and FEMALE are handled in a different way
        *
        * removed RACE NIGHT_ELF to prevent client crash
        */
        if (gender == GENDER_MALE)
        {
            *morph = FAKE_M_HUMAN;

            switch (_class)
            {
                case CLASS_DRUID:
                    *race = RACE_HUMAN; /* RACE_NIGHTELF; */
                    *morph = FAKE_M_NIGHT_ELF;
                    break;
                case CLASS_SHAMAN:
                    *race = RACE_DRAENEI;
                    *morph = FAKE_M_BROKEN_DRAENEI;
                    break;
                case CLASS_WARRIOR:
                case CLASS_DEATH_KNIGHT:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_DWARF, RACE_GNOME, /* RACE_NIGHTELF, */ RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_PALADIN:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_DWARF, RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_HUNTER:
                    *race = GetRandomRace({ RACE_DWARF, /* RACE_NIGHTELF, */ RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_ROGUE:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_DWARF, RACE_GNOME/* , RACE_NIGHTELF */ });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_PRIEST:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_DWARF, /* RACE_NIGHTELF,*/ RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_MAGE:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_GNOME, RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_WARLOCK:
                    *race = GetRandomRace({ RACE_HUMAN, RACE_GNOME });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
            }
        }
        else
        {
            *morph = FAKE_F_HUMAN;

            switch (_class)
            {
                case CLASS_DRUID:
                    // FEMALE NIGHT ELF is missing
                    break;
                case CLASS_SHAMAN:
                case CLASS_HUNTER:
                    // FEMALE DWARF and NIGHT ELF are missing (only for HUNTER)
                    *race = RACE_DRAENEI;
                    *morph = FAKE_F_DRAENEI;
                    break;
                case CLASS_WARRIOR:
                case CLASS_DEATH_KNIGHT:
                case CLASS_MAGE:
                    // DWARF and NIGHT ELF are missing (only for WARRIOR and DEATH_KNIGHT)
                    *race = GetRandomRace({ RACE_HUMAN, RACE_GNOME, RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_PALADIN:
                case CLASS_PRIEST:
                    // DWARF is missing
                    *race = GetRandomRace({ RACE_HUMAN, RACE_DRAENEI });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
                case CLASS_ROGUE:
                case CLASS_WARLOCK:
                    // DWARF and NIGHT ELF are missing (only for ROGUE)
                    *race = GetRandomRace({ RACE_HUMAN, RACE_GNOME });
                    *morph = GetMorphFromRace(*race, gender);
                    break;
            }
        }
    }
}

void CFBG::SetFakeRaceAndMorph(Player* player)
{
    if (!player->InBattleground())
    {
        return;
    }

    if (player->GetTeamId(true) == player->GetBgTeamId())
    {
        return;
    }

    if (IsPlayerFake(player))
    {
        return;
    }

    uint8 FakeRace;
    uint32 FakeMorph;

    // generate random race and morph
    RandomRaceMorph(&FakeRace, &FakeMorph, player->GetTeamId(true), player->getClass(), player->getGender());

    FakePlayer fakePlayer;
    fakePlayer.FakeMorph        = FakeMorph;
    fakePlayer.FakeRace         = FakeRace;
    fakePlayer.FakeTeamID       = player->TeamIdForRace(FakeRace);
    fakePlayer.RealMorph        = player->GetDisplayId();
    fakePlayer.RealNativeMorph  = player->GetNativeDisplayId();
    fakePlayer.RealRace         = player->getRace(true);
    fakePlayer.RealTeamID       = player->GetTeamId(true);

    _fakePlayerStore[player] = fakePlayer;

    player->setRace(FakeRace);
    SetFactionForRace(player, FakeRace);
    player->SetDisplayId(FakeMorph);
    player->SetNativeDisplayId(FakeMorph);
}

void CFBG::SetFactionForRace(Player* player, uint8 Race)
{
    player->setTeamId(player->TeamIdForRace(Race));

    ChrRacesEntry const* DBCRace = sChrRacesStore.LookupEntry(Race);
    player->SetFaction(DBCRace ? DBCRace->FactionID : 0);
}

void CFBG::ClearFakePlayer(Player* player)
{
    if (!IsPlayerFake(player))
        return;

    player->setRace(_fakePlayerStore[player].RealRace);
    player->SetDisplayId(_fakePlayerStore[player].RealMorph);
    player->SetNativeDisplayId(_fakePlayerStore[player].RealNativeMorph);
    SetFactionForRace(player, _fakePlayerStore[player].RealRace);

    // Clear forced faction reactions. Rank doesn't matter here, not used when they are removed.
    player->GetReputationMgr().ApplyForceReaction(FactionFrostwolfClan, REP_FRIENDLY, false);
    player->GetReputationMgr().ApplyForceReaction(FactionStormpikeGuard, REP_FRIENDLY, false);

    _fakePlayerStore.erase(player);
}

bool CFBG::IsPlayerFake(Player* player)
{
    return _fakePlayerStore.find(player) != _fakePlayerStore.end();
}

void CFBG::DoForgetPlayersInList(Player* player)
{
    // m_FakePlayers is filled from a vector within the battleground
    // they were in previously so all players that have been in that BG will be invalidated.
    for (auto const& itr : _fakeNamePlayersStore)
    {
        WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
        data << itr.second;
        player->GetSession()->SendPacket(&data);

        if (Player* _player = ObjectAccessor::FindPlayer(itr.second))
            player->GetSession()->SendNameQueryOpcode(_player->GetGUID());
    }

    _fakeNamePlayersStore.erase(player);
}

void CFBG::FitPlayerInTeam(Player* player, bool action, Battleground* bg)
{
    if (!bg)
        bg = player->GetBattleground();

    if ((!bg || bg->isArena()) && action)
        return;

    if (action)
        SetForgetBGPlayers(player, true);
    else
        SetForgetInListPlayers(player, true);
}

void CFBG::SetForgetBGPlayers(Player* player, bool value)
{
    _forgetBGPlayersStore[player] = value;
}

bool CFBG::ShouldForgetBGPlayers(Player* player)
{
    return _forgetBGPlayersStore[player];
}

void CFBG::SetForgetInListPlayers(Player* player, bool value)
{
    _forgetInListPlayersStore[player] = value;
}

bool CFBG::ShouldForgetInListPlayers(Player* player)
{
    return _forgetInListPlayersStore.find(player) != _forgetInListPlayersStore.end() && _forgetInListPlayersStore[player];
}

void CFBG::DoForgetPlayersInBG(Player* player, Battleground* bg)
{
    for (auto const& itr : bg->GetPlayers())
    {
        // Here we invalidate players in the bg to the added player
        WorldPacket data1(SMSG_INVALIDATE_PLAYER, 8);
        data1 << itr.first;
        player->GetSession()->SendPacket(&data1);

        if (Player* _player = ObjectAccessor::FindPlayer(itr.first))
        {
            player->GetSession()->SendNameQueryOpcode(_player->GetGUID()); // Send namequery answer instantly if player is available

            // Here we invalidate the player added to players in the bg
            WorldPacket data2(SMSG_INVALIDATE_PLAYER, 8);
            data2 << player->GetGUID();
            _player->GetSession()->SendPacket(&data2);
            _player->GetSession()->SendNameQueryOpcode(player->GetGUID());
        }
    }
}

bool CFBG::SendRealNameQuery(Player* player)
{
    if (IsPlayingNative(player))
        return false;

    WorldPacket data(SMSG_NAME_QUERY_RESPONSE, (8 + 1 + 1 + 1 + 1 + 1 + 10));
    data << player->GetGUID().WriteAsPacked();                  // player guid
    data << uint8(0);                                           // added in 3.1; if > 1, then end of packet
    data << player->GetName();                                  // played name
    data << uint8(0);                                           // realm name for cross realm BG usage
    data << uint8(player->getRace(true));
    data << uint8(player->getGender());
    data << uint8(player->getClass());
    data << uint8(0);                                           // is not declined
    player->GetSession()->SendPacket(&data);

    return true;
}

bool CFBG::IsPlayingNative(Player* player)
{
    return player->GetTeamId(true) == player->GetBGData().bgTeamId;
}

bool CFBG::CheckCrossFactionMatch(BattlegroundQueue* queue, BattlegroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers)
{
    uint32 freeA = maxPlayers;
    uint32 freeH = maxPlayers;

    queue->m_SelectionPools[TEAM_ALLIANCE].Init();
    queue->m_SelectionPools[TEAM_HORDE].Init();

    std::list<GroupQueueInfo*> groups = queue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG];
    groups.sort([](GroupQueueInfo* a, GroupQueueInfo* b) { return a->JoinTime < b->JoinTime; });

    bool startable = false;

    for (auto const& gInfo : groups)
    {
        if (gInfo->IsInvitedToBGInstanceGUID)
            continue;

        bool AddAsAlly = freeA == freeH ? gInfo->RealTeamID == TEAM_ALLIANCE : freeA > freeH; // true ? true : false

        gInfo->teamId = AddAsAlly ? TEAM_ALLIANCE : TEAM_HORDE;

        if (queue->m_SelectionPools[AddAsAlly ? TEAM_ALLIANCE : TEAM_HORDE].AddGroup(gInfo, AddAsAlly ? maxPlayers : maxPlayers))
            AddAsAlly ? freeA -= gInfo->Players.size() : freeH -= gInfo->Players.size();
        else
            break;

        // Return when we're ready to start a BG, if we're in startup process
        if (queue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount() >= minPlayers &&
            queue->m_SelectionPools[TEAM_HORDE].GetPlayerCount() >= minPlayers)
            startable = true;
    }

    if (startable)
        return true;

    // If we're in BG testing one player is enough
    if (sBattlegroundMgr->isTesting() && queue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount() + queue->m_SelectionPools[TEAM_HORDE].GetPlayerCount() > 0)
        return true;

    // Return false when we didn't manage to fill the BattleGround in Filling "mode".
    // reset selectionpool for further attempts
    queue->m_SelectionPools[TEAM_ALLIANCE].Init();
    queue->m_SelectionPools[TEAM_HORDE].Init();
    return false;
}

bool CFBG::FillPlayersToCFBG(BattlegroundQueue* bgqueue, Battleground* bg, BattlegroundBracketId bracket_id)
{
    if (!IsEnableSystem() || bg->isArena() || bg->isRated())
        return false;

    uint32 freeA = bg->GetFreeSlotsForTeam(TEAM_ALLIANCE);
    uint32 freeH = bg->GetFreeSlotsForTeam(TEAM_HORDE);

    uint32 maxA = freeA;
    uint32 maxH = freeH;

    std::list<GroupQueueInfo*> groups = bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG];
    groups.sort([](GroupQueueInfo* a, GroupQueueInfo* b) { return a->JoinTime < b->JoinTime; });

    for (auto const& gInfo : groups)
    {
        if (gInfo->IsInvitedToBGInstanceGUID)
            continue;

        TeamId targetTeam = GetLowerTeamIdInBG(bg, gInfo);
        gInfo->teamId = targetTeam;

        if (bgqueue->m_SelectionPools[targetTeam].AddGroup(gInfo, targetTeam == TEAM_ALLIANCE ? maxA : maxH))
            targetTeam == TEAM_ALLIANCE ? freeA -= gInfo->Players.size() : freeH -= gInfo->Players.size();
    }

    // If we're in BG testing one player is enough
    if (sBattlegroundMgr->isTesting() && bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount() + bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount() > 0)
        return true;

    return true;
}

bool CFBG::isClassJoining(uint8 _class, Player* player, uint32 minLevel)
{
    if (!player)
    {
        return false;
    }

    return player->getClass() == _class && (player->getLevel() >= minLevel);
}

void CFBG::UpdateForget(Player* player)
{
    Battleground* bg = player->GetBattleground();
    if (bg)
    {
        if (ShouldForgetBGPlayers(player) && bg)
        {
            DoForgetPlayersInBG(player, bg);
            SetForgetBGPlayers(player, false);
        }
    }
    else if (ShouldForgetInListPlayers(player))
    {
        DoForgetPlayersInList(player);
        SetForgetInListPlayers(player, false);
    }
}

std::unordered_map<ObjectGuid, Seconds> BGSpamProtectionCFBG;
void CFBG::SendMessageQueue(BattlegroundQueue* bgQueue, Battleground* bg, PvPDifficultyEntry const* bracketEntry, Player* leader)
{
    BattlegroundBracketId bracketId = bracketEntry->GetBracketId();

    auto bgName = bg->GetName();
    uint32 q_min_level = std::min(bracketEntry->minLevel, (uint32)80);
    uint32 q_max_level = std::min(bracketEntry->maxLevel, (uint32)80);
    uint32 MinPlayers = bg->GetMinPlayersPerTeam() * 2;
    uint32 qTotal = bgQueue->GetPlayersCountInGroupsQueue(bracketId, (BattlegroundQueueGroupTypes)BG_QUEUE_CFBG);

    if (CONF_GET_BOOL("Battleground.QueueAnnouncer.PlayerOnly"))
    {
        ChatHandler(leader->GetSession()).PSendSysMessage("CFBG %s (Levels: %u - %u). Registered: %u/%u", bgName.c_str(), q_min_level, q_max_level, qTotal, MinPlayers);
    }
    else
    {
        if (CONF_GET_BOOL("Battleground.QueueAnnouncer.Timed"))
        {
            if (bgQueue->GetQueueAnnouncementTimer(bracketEntry->bracketId) < 0)
                bgQueue->SetQueueAnnouncementTimer(bracketEntry->bracketId, CONF_GET_INT("Battleground.QueueAnnouncer.Timer"));
        }
        else
        {
            auto searchGUID = BGSpamProtectionCFBG.find(leader->GetGUID());

            if (searchGUID == BGSpamProtectionCFBG.end())
                BGSpamProtectionCFBG[leader->GetGUID()] = 0s;

            // Skip if spam time < 30 secs (default)
            if (GameTime::GetGameTime() - BGSpamProtectionCFBG[leader->GetGUID()] < Seconds(CONF_GET_UINT("Battleground.QueueAnnouncer.SpamProtection.Delay")))
            {
                return;
            }

            // When limited, it announces only if there are at least CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_LIMIT_MIN_PLAYERS in queue
            auto limitQueueMinLevel = CONF_GET_UINT("Battleground.QueueAnnouncer.Limit.MinLevel");
            if (limitQueueMinLevel != 0 && q_min_level >= limitQueueMinLevel)
            {
                // limit only RBG for 80, WSG for lower levels
                auto bgTypeToLimit = q_min_level == 80 ? BATTLEGROUND_RB : BATTLEGROUND_WS;

                if (bg->GetBgTypeID() == bgTypeToLimit && qTotal < CONF_GET_UINT("Battleground.QueueAnnouncer.Limit.MinPlayers"))
                {
                    return;
                }
            }

            BGSpamProtectionCFBG[leader->GetGUID()] = GameTime::GetGameTime();

            if (_showPlayerName)
            {
                std::string msg = Warhead::StringFormat("{} |cffffffffHas Joined|r |cffff0000{}|r|cffffffff(|r|cff00ffff{}|r|cffffffff/|r|cff00ffff{}|r|cffffffff)|r",
                    leader->GetPlayerName(), bg->GetName(), qTotal, MinPlayers);

                for (auto const& session : sWorld->GetAllSessions())
                {
                    if (Player* player = session.second->GetPlayer())
                    {
                        WorldPacket data(SMSG_CHAT_SERVER_MESSAGE, (msg.size() + 1));
                        data << uint32(3);
                        data << msg;
                        player->GetSession()->SendPacket(&data);
                    }
                }
            }
            else
                Warhead::Text::SendWorldText(LANG_BG_QUEUE_ANNOUNCE_WORLD, bgName, q_min_level, q_max_level, qTotal, MinPlayers);
        }
    }
}
