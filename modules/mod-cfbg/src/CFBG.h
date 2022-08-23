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

#ifndef _CFBG_H_
#define _CFBG_H_

#include "Battleground.h"
#include "BattlegroundQueue.h"
#include "Player.h"
#include <unordered_map>
#include <vector>

enum FakeMorphs
{
    FAKE_M_FEL_ORC        = 21267,
    FAKE_F_ORC            = 20316,
    FAKE_M_DWARF          = 20317,
    FAKE_M_NIGHT_ELF      = 20318,
    FAKE_F_DRAENEI        = 20323,
    FAKE_M_BROKEN_DRAENEI = 21105,
    FAKE_M_TROLL          = 20321,
    FAKE_M_HUMAN          = 19723,
    FAKE_F_HUMAN          = 19724,
    FAKE_M_BLOOD_ELF      = 20578,
    FAKE_F_BLOOD_ELF      = 20579,
    FAKE_F_GNOME          = 20320,
    FAKE_M_GNOME          = 20580,
    FAKE_F_TAUREN         = 20584,
    FAKE_M_TAUREN         = 20585
};

constexpr auto FACTION_FROSTWOLF_CLAN = 729;
constexpr auto FACTION_STORMPIKE_GUARD = 730;

struct FakePlayer
{
    // Fake
    uint8   FakeRace;
    uint32  FakeMorph;
    TeamId  FakeTeamID;

    // Real
    uint8   RealRace;
    uint32  RealMorph;
    uint32  RealNativeMorph;
    TeamId  RealTeamID;
};

struct CrossFactionGroupInfo
{
    explicit CrossFactionGroupInfo(GroupQueueInfo* groupInfo);

    uint32 AveragePlayersLevel{ 0 };
    uint32 AveragePlayersItemLevel{ 0 };
    bool IsHunterJoining{ false };
    uint32 SumAverageItemLevel{ 0 };
    uint32 SumPlayerLevel{ 0 };

    CrossFactionGroupInfo() = delete;
    CrossFactionGroupInfo(CrossFactionGroupInfo&&) = delete;
};

struct CrossFactionQueueInfo
{
    explicit CrossFactionQueueInfo(BattlegroundQueue* bgQueue);

    TeamId GetLowerTeamIdInBG(GroupQueueInfo* groupInfo);

    std::array<uint32, 2> PlayersCount{};
    std::array<uint32, 2> SumAverageItemLevel{};
    std::array<uint32, 2> SumPlayerLevel{};

private:
    TeamId SelectBgTeam(GroupQueueInfo* groupInfo);
    TeamId GetLowerAverageItemLevelTeam();

    CrossFactionQueueInfo() = delete;
    CrossFactionQueueInfo(CrossFactionQueueInfo&&) = delete;
};

class CFBG
{
public:
    using RandomSkinInfo = std::pair<uint8/*race*/, uint32/*morph*/>;
    using GroupsList = std::vector<GroupQueueInfo*>;
    using SameCountGroupsList = std::vector<std::pair<GroupQueueInfo*, GroupsList>>;

    static CFBG* instance();

    void LoadConfig();

    inline bool IsEnableSystem() const { return _IsEnableSystem; }
    inline bool IsEnableAvgIlvl() const { return _IsEnableAvgIlvl; }
    inline bool IsEnableBalancedTeams() const { return _IsEnableBalancedTeams; }
    inline bool IsEnableBalanceClassLowLevel() const { return _IsEnableBalanceClassLowLevel; }
    inline bool IsEnableEvenTeams() const { return _IsEnableEvenTeams; }
    inline bool IsEnableResetCooldowns() const { return _IsEnableResetCooldowns; }
    inline uint32 EvenTeamsMaxPlayersThreshold() const { return _EvenTeamsMaxPlayersThreshold; }
    inline uint32 GetMaxPlayersCountInGroup() const { return _MaxPlayersCountInGroup; }
    inline uint8 GetBalanceClassMinLevel() const { return _balanceClassMinLevel; }
    inline uint8 GetBalanceClassMaxLevel() const { return _balanceClassMaxLevel; }
    inline uint8 GetBalanceClassLevelDiff() const { return _balanceClassLevelDiff; }
    inline bool RandomizeRaces() const { return _randomizeRaces; }

    uint32 GetBGTeamAverageItemLevel(Battleground* bg, TeamId team);
    uint32 GetBGTeamSumPlayerLevel(Battleground* bg, TeamId team);
    uint32 GetAllPlayersCountInBG(Battleground* bg);

    TeamId GetLowerTeamIdInBG(Battleground* bg, BattlegroundQueue* bgQueue, GroupQueueInfo* groupInfo);
    TeamId GetLowerAvgIlvlTeamInBg(Battleground* bg);
    TeamId SelectBgTeam(Battleground* bg, GroupQueueInfo* groupInfo, CrossFactionQueueInfo* cfQueueInfo);

    bool IsAvgIlvlTeamsInBgEqual(Battleground* bg);
    bool SendRealNameQuery(Player* player);
    bool IsPlayerFake(Player* player);
    bool ShouldForgetInListPlayers(Player* player);
    bool IsPlayingNative(Player* player);

    void ValidatePlayerForBG(Battleground* bg, Player* player, TeamId teamId);
    void SetFakeRaceAndMorph(Player* player);
    void SetFactionForRace(Player* player, uint8 Race);
    void ClearFakePlayer(Player* player);
    void DoForgetPlayersInList(Player* player);
    void FitPlayerInTeam(Player* player, bool action, Battleground* bg);
    void DoForgetPlayersInBG(Player* player, Battleground* bg);
    void SetForgetBGPlayers(Player* player, bool value);
    bool ShouldForgetBGPlayers(Player* player);
    void SetForgetInListPlayers(Player* player, bool value);
    void UpdateForget(Player* player);
    void SendMessageQueue(BattlegroundQueue* bgQueue, Battleground* bg, PvPDifficultyEntry const* bracketEntry, Player* leader);

    bool FillPlayersToCFBG(BattlegroundQueue* bgqueue, Battleground* bg, BattlegroundBracketId bracket_id);
    bool CheckCrossFactionMatch(BattlegroundQueue* bgqueue, BattlegroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers);

    bool IsRaceValidForFaction(uint8 teamId, uint8 race);
    TeamId getTeamWithLowerClass(Battleground* bg, Classes c);
    uint8 getBalanceClassMinLevel(const Battleground* bg) const;

    void OnAddGroupToBGQueue(GroupQueueInfo* ginfo, Group* group);

private:
    bool isClassJoining(uint8 _class, Player* player, uint32 minLevel);

    RandomSkinInfo GetRandomRaceMorph(TeamId team, uint8 playerClass, uint8 gender);

    uint32 GetMorphFromRace(uint8 race, uint8 gender);
    FakePlayer const* GetFakePlayer(Player* player) const;

    void InviteSameCountGroups(GroupsList& groups, BattlegroundQueue* bgQueue, uint32 maxAli, uint32 maxHorde, Battleground* bg = nullptr);
    TeamId InviteGroupToBG(GroupQueueInfo* gInfo, BattlegroundQueue* bgQueue, uint32 maxAli, uint32 maxHorde, Battleground* bg = nullptr);

    std::unordered_map<Player*, FakePlayer> _fakePlayerStore;
    std::unordered_map<Player*, ObjectGuid> _fakeNamePlayersStore;
    std::unordered_map<Player*, bool> _forgetBGPlayersStore;
    std::unordered_map<Player*, bool> _forgetInListPlayersStore;
    std::unordered_map<GroupQueueInfo*, CrossFactionGroupInfo> _groupsInfo;

    // For config
    bool _IsEnableSystem;
    bool _IsEnableAvgIlvl;
    bool _IsEnableBalancedTeams;
    bool _IsEnableBalanceClassLowLevel;
    bool _IsEnableEvenTeams;
    bool _IsEnableResetCooldowns;
    bool _showPlayerName;
    bool _randomizeRaces;
    uint32 _EvenTeamsMaxPlayersThreshold;
    uint32 _MaxPlayersCountInGroup;
    uint8 _balanceClassMinLevel;
    uint8 _balanceClassMaxLevel;
    uint8 _balanceClassLevelDiff;
};

#define sCFBG CFBG::instance()

#endif // _KARGATUM_CFBG_H_
