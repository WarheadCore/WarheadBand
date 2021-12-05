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

#include "GameConfig.h"
#include "Config.h"
#include "Log.h"
#include "MapMgr.h"
#include "Object.h"
#include "Player.h"
#include "ServerMotd.h"
#include "StringConvert.h"
#include "World.h"
#include <unordered_map>

namespace
{
    std::unordered_map<std::string /*name*/, std::string /*value*/> _configOptions;

    // Default values if no exist in config files
    constexpr auto CONF_DEFAULT_BOOL = false;
    constexpr auto CONF_DEFAULT_STR = "";
    constexpr auto CONF_DEFAULT_INT = 0;
    constexpr auto CONF_DEFAULT_FLOAT = 1.0f;
}

GameConfig* GameConfig::instance()
{
    static GameConfig instance;
    return &instance;
}

void GameConfig::Load(bool reload)
{
    LOG_INFO("server.loading", "{} game configuraton:", reload ? "Reloading" : "Loading");

    LoadConfigs(reload);
    CheckOptions(reload);

    LOG_INFO("server.loading", " ");
}

// Add option
template<typename T>
WH_GAME_API void GameConfig::AddOption(std::string const& optionName, Optional<T> def /*= std::nullopt*/) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is already exists", optionName);
        return;
    }

    std::string value = Warhead::ToString<T>(sConfigMgr->GetOption<T>(optionName, def == std::nullopt ? CONF_DEFAULT_INT : *def));

    _configOptions.emplace(optionName, value);
}

template<>
WH_GAME_API void GameConfig::AddOption<bool>(std::string const& optionName, Optional<bool> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is already exists", optionName);
        return;
    }

    std::string value = Warhead::ToString(sConfigMgr->GetOption<bool>(optionName, def == std::nullopt ? CONF_DEFAULT_BOOL : *def));

    _configOptions.emplace(optionName, value);
}

template<>
WH_GAME_API void GameConfig::AddOption<std::string>(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is already exists", optionName);
        return;
    }

    std::string value = sConfigMgr->GetOption<std::string>(optionName, def == std::nullopt ? CONF_DEFAULT_STR : *def);

    _configOptions.emplace(optionName, value);
}

template<>
WH_GAME_API void GameConfig::AddOption<float>(std::string const& optionName, Optional<float> def /*= std::nullopt*/) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is already exists", optionName);
        return;
    }

    std::string value = Warhead::ToString(sConfigMgr->GetOption<float>(optionName, def == std::nullopt ? CONF_DEFAULT_FLOAT : *def));

    _configOptions.emplace(optionName, value);
}

// Add option without template
void GameConfig::AddOption(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    AddOption<std::string>(optionName, def);
}

void GameConfig::AddOption(std::initializer_list<std::string> optionList) const
{
    for (auto const& option : optionList)
        AddOption(option);
}

// Get option
template<typename T>
WH_GAME_API T GameConfig::GetOption(std::string const& optionName, Optional<T> def /*= std::nullopt*/) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    auto retValueDef = def == std::nullopt ? CONF_DEFAULT_INT : *def;
    std::string defStr = fmt::format("{}", retValueDef);

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists. Returned ({})", optionName, defStr);
        return retValueDef;
    }

    Optional<T> result = Warhead::StringTo<T>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> GameConfig: Bad value defined for '{}', use '{}' instead", optionName, defStr);
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API bool GameConfig::GetOption<bool>(std::string const& optionName, Optional<bool> def /*= std::nullopt*/) const
{
    auto retValueDef = def == std::nullopt ? CONF_DEFAULT_BOOL : *def;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef ? "true" : "false");
        return retValueDef;
    }

    Optional<bool> result = Warhead::StringTo<bool>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> GameConfig: Bad value defined for '{}', use '{}' instead", optionName, retValueDef ? "true" : "false");
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API float GameConfig::GetOption<float>(std::string const& optionName, Optional<float> def /*= std::nullopt*/) const
{
    auto retValueDef = def == std::nullopt ? CONF_DEFAULT_FLOAT : *def;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef);
        return retValueDef;
    }

    Optional<float> result = Warhead::StringTo<float>(_configOptions.at(optionName));
    if (!result)
    {
        LOG_ERROR("server.loading", "> GameConfig: Bad value defined for '{}', use '{}' instead", optionName, retValueDef);
        return retValueDef;
    }

    return *result;
}

template<>
WH_GAME_API std::string GameConfig::GetOption<std::string>(std::string const& optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    auto retValueDef = def == std::nullopt ? CONF_DEFAULT_STR : *def;

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists. Returned ({})", optionName, retValueDef);
        return retValueDef;
    }

    return _configOptions.at(optionName);
}

// Set option
template<typename T>
WH_GAME_API void GameConfig::SetOption(std::string const& optionName, T value) const
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "Bad config template. Use only integral and no bool");

    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(int32(value)));
}

template<>
WH_GAME_API void GameConfig::SetOption<bool>(std::string const& optionName, bool value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(value));
}

template<>
WH_GAME_API void GameConfig::SetOption<std::string>(std::string const& optionName, std::string value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, value);
}

template<>
WH_GAME_API void GameConfig::SetOption<float>(std::string const& optionName, float value) const
{
    // Check exist option
    auto itr = _configOptions.find(optionName);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists", optionName);
        return;
    }

    _configOptions.erase(optionName);
    _configOptions.emplace(optionName, Warhead::ToString(value));
}

// Loading
void GameConfig::LoadConfigs(bool reload /*= false*/)
{
    std::unordered_map<std::string, int32> _notChangeConfigs;

    if (reload)
    {
        _notChangeConfigs =
        {
            { "WorldServerPort", GetOption<int32>("WorldServerPort") },
            { "GameType", GetOption<int32>("GameType") },
            { "RealmZone", GetOption<int32>("RealmZone") },
            { "MaxPlayerLevel", GetOption<int32>("MaxPlayerLevel") },
            { "Expansion", GetOption<int32>("Expansion") }
        };
    }

    _configOptions.clear();

    /*
     * bool configs
     */
    AddOption({ "ActivateWeather",
    "AddonChannel",
    "AllFlightPaths",
    "Allow.IP.Based.Action.Logging",
    "AllowPlayerCommands",
    "AllowTickets",
    "AllowTwoSide.Accounts",
    "AllowTwoSide.AddFriend",
    "AllowTwoSide.Interaction.Auction",
    "AllowTwoSide.Interaction.Calendar",
    "AllowTwoSide.Interaction.Channel",
    "AllowTwoSide.Interaction.Chat",
    "AllowTwoSide.Interaction.Emote",
    "AllowTwoSide.Interaction.Group",
    "AllowTwoSide.Interaction.Guild",
    "AllowTwoSide.Interaction.Mail",
    "AllowTwoSide.Trade",
    "AllowTwoSide.WhoList",
    "AlwaysMaxSkillForLevel",
    "AlwaysMaxWeaponSkill",
    "Arena.ArenaSeason.ID",
    "Arena.ArenaSeason.InProgress",
    "Arena.AutoDistributePoints",
    "Arena.QueueAnnouncer.Enable",
    "Arena.QueueAnnouncer.PlayerOnly",
    "AutoBroadcast.On",
    "Battleground.CastDeserter",
    "Battleground.DisableQuestShareInBG",
    "Battleground.DisableReadyCheckInBG",
    "Battleground.GiveXPForKills",
    "Battleground.QueueAnnouncer.Enable",
    "Battleground.QueueAnnouncer.PlayerOnly",
    "Battleground.StoreStatistics.Enable",
    "Battleground.TrackDeserters.Enable",
    "Calculate.Creature.Zone.Area.Data",
    "Calculate.Gameoject.Zone.Area.Data",
    "Channel.RestrictedLfg",
    "Channel.SilentlyGMJoin",
    "Chat.MuteFirstLogin",
    "ChatFakeMessagePreventing",
    "CheckGameObjectLoS",
    "CleanCharacterDB",
    "CloseIdleConnections",
    "Console.Enable",
    "Death.Bones.BattlegroundOrArena",
    "Death.Bones.World",
    "Death.CorpseReclaimDelay.PvE",
    "Death.CorpseReclaimDelay.PvP",
    "Debug.Arena",
    "Debug.Battleground",
    "DeclinedNames",
    "DeletedCharacterTicketTrace",
    "DetectPosCollision",
    "Die.Command.Mode",
    "DontCacheRandomMovementPaths",
    "DungeonAccessRequirements.LFGLevelDBCOverride",
    "DungeonAccessRequirements.PortalAvgIlevelCheck",
    "DurabilityLoss.InPvP",
    "EnableLoginAfterDC",
    "GM.AllowFriend",
    "GM.AllowInvite",
    "GM.LowerSecurity",
    "Guild.AllowMultipleGuildMaster",
    "Instance.GMSummonPlayer",
    "Instance.IgnoreLevel",
    "Instance.IgnoreRaid",
    "Instance.SharedNormalHeroicId",
    "IsContinentTransport.Enabled",
    "IsPreloadedContinentTransport.Enabled",
    "Item.SetItemTradeable",
    "ItemDelete.Method",
    "ItemDelete.Vendor",
    "LFG.Location.All",
    "Minigob.Manabonk.Enable",
    "MoveMaps.Enable",
    "Mute.AddAfterLogin.Enable",
    "Network.TcpNodelay",
    "NoResetTalentsCost",
    "NpcRegenHPIfTargetIsUnreachable",
    "OffhandCheckAtSpellUnlearn",
    "PlayerDump.DisallowOverwrite",
    "PlayerDump.DisallowPaths",
    "PlayerSave.Stats.SaveOnlyOnLogout",
    "PlayerStart.AllReputation",
    "PlayerStart.AllSpells",
    "PlayerStart.MapsExplored",
    "PreloadAllNonInstancedMapGrids",
    "PreserveCustomChannels",
    "PvPToken.Enable",
    "Quests.EnableQuestTracker",
    "Quests.IgnoreAutoAccept",
    "Quests.IgnoreAutoComplete",
    "Quests.IgnoreRaid",
    "Ra.Enable",
    "SOAP.Enabled",
    "SaveRespawnTimeImmediately",
    "SetAllCreaturesWithWaypointMovementActive",
    "ShowBanInWorld",
    "ShowKickInWorld",
    "ShowMuteInWorld",
    "SkillChance.Milling",
    "SkillChance.Prospecting",
    "TalentsInspecting",
    "Warden.Enabled",
    "Wintergrasp.Enable",
    "vmap.enableHeight",
    "vmap.enableIndoorCheck",
    "vmap.enableLOS",
    "vmap.petLOS",
    "World.RealmAvailability" });

    /*
     * string configs
     */
    AddOption({ "BindIP",
    "DataDir",
    "IPLocationFile",
    "LogsDir",
    "Motd",
    "PacketLogFile",
    "PidFile",
    "PlayerStart.String",
    "Ra.IP",
    "SOAP.IP" });

    /*
     * int configs
     */
    AddOption({ "AccountInstancesPerHour",
    "Arena.ArenaStartMatchmakerRating",
    "Arena.ArenaStartRating",
    "Arena.RatingDiscardTimer",
    "ArenaTeam.CharterCost.2v2",
    "ArenaTeam.CharterCost.3v3",
    "ArenaTeam.CharterCost.5v5",
    "AutoBroadcast.Center",
    "AutoBroadcast.Timer",
    "Battleground.BerserkingBuffRespawn",
    "Battleground.InvitationType",
    "Battleground.PlayerRespawn",
    "Battleground.PremadeGroupWaitForMatch",
    "Battleground.QueueAnnouncer.Limit.MinLevel",
    "Battleground.QueueAnnouncer.Limit.MinPlayers",
    "Battleground.QueueAnnouncer.SpamProtection.Delay",
    "Battleground.Random.ResetHour",
    "Battleground.ReportAFK",
    "Battleground.ReportAFK.Timer",
    "Battleground.RestorationBuffRespawn",
    "Battleground.RewardLoserHonorFirst",
    "Battleground.RewardLoserHonorLast",
    "Battleground.RewardWinnerArenaFirst",
    "Battleground.RewardWinnerArenaLast",
    "Battleground.RewardWinnerHonorFirst",
    "Battleground.RewardWinnerHonorLast",
    "Battleground.SpeedBuffRespawn",
    "BirthdayTime",
    "Calendar.DeleteOldEventsHour",
    "ChangeWeatherInterval",
    "Channel.ModerationGMLevel",
    "CharDelete.KeepDays",
    "CharDelete.Method",
    "CharDelete.MinLevel",
    "CharacterCreating.Disabled",
    "CharacterCreating.Disabled.ClassMask",
    "CharacterCreating.Disabled.RaceMask",
    "CharacterCreating.MinLevelForHeroicCharacter",
    "CharactersPerAccount",
    "CharactersPerRealm",
    "Chat.MuteTimeFirstLogin",
    "ChatFlood.MessageCount",
    "ChatFlood.MessageDelay",
    "ChatFlood.MuteTime",
    "ChatLevelReq.Channel",
    "ChatLevelReq.Say",
    "ChatLevelReq.Whisper",
    "ChatStrictLinkChecking.Kick",
    "ChatStrictLinkChecking.Severity",
    "ClientCacheVersion",
    "Command.LookupMaxResults",
    "Compression",
    "Corpse.Decay.ELITE",
    "Corpse.Decay.NORMAL",
    "Corpse.Decay.RARE",
    "Corpse.Decay.RAREELITE",
    "Corpse.Decay.WORLDBOSS",
    "CreatureFamilyAssistanceDelay",
    "CreatureFamilyAssistancePeriod",
    "CreatureFamilyFleeDelay",
    "DBC.Locale",
    "Death.SicknessLevel",
    "DisableWaterBreath",
    "DisconnectToleranceInterval",
    "DungeonAccessRequirements.OptionalStringID",
    "DungeonAccessRequirements.PrintMode",
    "DungeonFinder.Expansion",
    "DungeonFinder.OptionsMask",
    "Event.Announce",
    "Expansion",
    "FFAPvPTimer",
    "GM.Chat",
    "GM.InGMList.Level",
    "GM.InWhoList.Level",
    "GM.LoginState",
    "GM.StartLevel",
    "GM.Visible",
    "GM.WhisperingTo",
    "GameType",
    "Guild.BankEventLogRecordsCount",
    "Guild.BankInitialTabs",
    "Guild.BankTabCost0",
    "Guild.BankTabCost1",
    "Guild.BankTabCost2",
    "Guild.BankTabCost3",
    "Guild.BankTabCost4",
    "Guild.BankTabCost5",
    "Guild.CharterCost",
    "Guild.EventLogRecordsCount",
    "Guild.ResetHour",
    "HeroicCharactersPerRealm",
    "HonorPointsAfterDuel",
    "ICC.Buff.Alliance",
    "ICC.Buff.Horde",
    "Instance.ResetTimeHour",
    "Instance.ResetTimeRelativeTimestamp",
    "Instance.UnloadDelay",
    "InstantFlightPaths",
    "InstantLogout",
    "ItemDelete.ItemLevel",
    "ItemDelete.Quality",
    "LFG.MaxKickCount",
    "LevelReq.Auction",
    "LevelReq.Mail",
    "LevelReq.Ticket",
    "LevelReq.Trade",
    "MailDeliveryDelay",
    "MapUpdate.Threads",
    "MapUpdateInterval",
    "MaxAllowedMMRDrop",
    "MaxArenaPoints",
    "MaxCoreStuckTime",
    "MaxHonorPoints",
    "MaxOverspeedPings",
    "MaxPingTime",
    "MaxPlayerLevel",
    "MaxPrimaryTradeSkill",
    "MaxWhoListReturns",
    "MinCharterName",
    "MinDualSpecLevel",
    "MinPetName",
    "MinPetitionSigns",
    "MinPlayerName",
    "MinRecordUpdateTimeDiff",
    "Network.OutKBuff",
    "Network.OutUBuff",
    "Network.Threads",
    "NpcEvadeIfTargetIsUnreachable",
    "NpcRegenHPTimeIfTargetIsUnreachable",
    "PacketSpoof.BanDuration",
    "PacketSpoof.BanMode",
    "PacketSpoof.Policy",
    "PartyLevelReq",
    "PersistentCharacterCleanFlags",
    "PlayerLimit",
    "PlayerSave.Stats.MinLevel",
    "PlayerSaveInterval",
    "PreserveCustomChannelDuration",
    "PreventAFKLogout",
    "PvPToken.ItemCount",
    "PvPToken.ItemID",
    "PvPToken.MapAllowType",
    "Quests.HighLevelHideDiff",
    "Quests.LowLevelHideDiff",
    "Ra.MinLevel",
    "Ra.Port",
    "RealmID",
    "RealmZone",
    "RecordUpdateTimeDiffInterval",
    "RecruitAFriend.MaxDifference",
    "RecruitAFriend.MaxLevel",
    "SOAP.Port",
    "Server.LoginInfo",
    "SessionAddDelay",
    "SkillChance.Green",
    "SkillChance.Grey",
    "SkillChance.MiningSteps",
    "SkillChance.Orange",
    "SkillChance.SkinningSteps",
    "SkillChance.Yellow",
    "SkillGain.Crafting",
    "SkillGain.Defense",
    "SkillGain.Gathering",
    "SkillGain.Weapon",
    "SkipCinematics",
    "SocketTimeOutTime",
    "SocketTimeOutTimeActive",
    "StartArenaPoints",
    "StartHeroicPlayerLevel",
    "StartHonorPoints",
    "StartPlayerLevel",
    "StartPlayerMoney",
    "StrictPlayerNames",
    "TeleportTimeoutFar",
    "TeleportTimeoutNear",
    "ThreadPool",
    "ToggleXP.Cost",
    "UpdateUptimeInterval",
    "Visibility.GroupMode",
    "Warden.BanDuration",
    "Warden.ClientCheckFailAction",
    "Warden.ClientCheckHoldOff",
    "Warden.ClientResponseDelay",
    "Warden.NumLuaChecks",
    "Warden.NumMemChecks",
    "Warden.NumOtherChecks",
    "WaypointMovementStopTimeForPlayer",
    "Wintergrasp.BattleTimer",
    "Wintergrasp.CrashRestartTimer",
    "Wintergrasp.NoBattleTimer",
    "Wintergrasp.PlayerMax",
    "Wintergrasp.PlayerMin",
    "Wintergrasp.PlayerMinLvl",
    "WorldBossLevelDiff",
    "WorldServerPort" });

    /*
     * float configs
     */
    AddOption({ "Arena.ArenaLoseRatingModifier",
    "Arena.ArenaMatchmakerRatingModifier",
    "Arena.ArenaWinRatingModifier1",
    "Arena.ArenaWinRatingModifier2",
    "CreatureFamilyAssistanceRadius",
    "CreatureFamilyFleeAssistanceRadius",
    "DurabilityLoss.OnDeath",
    "DurabilityLossChance.Absorb",
    "DurabilityLossChance.Block",
    "DurabilityLossChance.Damage",
    "DurabilityLossChance.Parry",
    "GM.TicketSystem.ChanceOfGMSurvey",
    "ListenRange.Say",
    "ListenRange.TextEmote",
    "ListenRange.Yell",
    "MaxGroupXPDistance",
    "MaxRecruitAFriendBonusDistance",
    "MonsterSight",
    "Rate.ArenaPoints",
    "Rate.Auction.Cut",
    "Rate.Auction.Deposit",
    "Rate.Auction.Time",
    "Rate.BuyValue.Item.Artifact",
    "Rate.BuyValue.Item.Epic",
    "Rate.BuyValue.Item.Heirloom",
    "Rate.BuyValue.Item.Legendary",
    "Rate.BuyValue.Item.Normal",
    "Rate.BuyValue.Item.Poor",
    "Rate.BuyValue.Item.Rare",
    "Rate.BuyValue.Item.Uncommon",
    "Rate.Corpse.Decay.Looted",
    "Rate.Creature.Aggro",
    "Rate.Creature.Elite.Elite.Damage",
    "Rate.Creature.Elite.Elite.HP",
    "Rate.Creature.Elite.Elite.SpellDamage",
    "Rate.Creature.Elite.RARE.Damage",
    "Rate.Creature.Elite.RARE.HP",
    "Rate.Creature.Elite.RARE.SpellDamage",
    "Rate.Creature.Elite.RAREELITE.Damage",
    "Rate.Creature.Elite.RAREELITE.HP",
    "Rate.Creature.Elite.RAREELITE.SpellDamage",
    "Rate.Creature.Elite.WORLDBOSS.Damage",
    "Rate.Creature.Elite.WORLDBOSS.HP",
    "Rate.Creature.Elite.WORLDBOSS.SpellDamage",
    "Rate.Creature.Normal.Damage",
    "Rate.Creature.Normal.HP",
    "Rate.Creature.Normal.SpellDamage",
    "Rate.Damage.Fall",
    "Rate.Drop.Item.Artifact",
    "Rate.Drop.Item.Epic",
    "Rate.Drop.Item.Legendary",
    "Rate.Drop.Item.Normal",
    "Rate.Drop.Item.Poor",
    "Rate.Drop.Item.Rare",
    "Rate.Drop.Item.Referenced",
    "Rate.Drop.Item.ReferencedAmount",
    "Rate.Drop.Item.Uncommon",
    "Rate.Drop.Money",
    "Rate.Energy",
    "Rate.Focus",
    "Rate.Health",
    "Rate.Honor",
    "Rate.InstanceResetTime",
    "Rate.Mana",
    "Rate.MoveSpeed",
    "Rate.Pet.LevelXP",
    "Rate.Rage.Income",
    "Rate.Rage.Loss",
    "Rate.RepairCost",
    "Rate.Reputation.Gain",
    "Rate.Reputation.LowLevel.Kill",
    "Rate.Reputation.LowLevel.Quest",
    "Rate.Reputation.RecruitAFriendBonus",
    "Rate.Rest.InGame",
    "Rate.Rest.Offline.InTavernOrCity",
    "Rate.Rest.Offline.InWilderness",
    "Rate.RewardBonusMoney",
    "Rate.RunicPower.Income",
    "Rate.RunicPower.Loss",
    "Rate.SellValue.Item.Artifact",
    "Rate.SellValue.Item.Epic",
    "Rate.SellValue.Item.Heirloom",
    "Rate.SellValue.Item.Legendary",
    "Rate.SellValue.Item.Normal",
    "Rate.SellValue.Item.Poor",
    "Rate.SellValue.Item.Rare",
    "Rate.SellValue.Item.Uncommon",
    "Rate.Skill.Discovery",
    "Rate.Talent",
    "Rate.XP.BattlegroundKill",
    "Rate.XP.Explore",
    "Rate.XP.Kill",
    "Rate.XP.Pet",
    "Rate.XP.Quest",
    "TargetPosRecalculateRange",
    "Visibility.Distance.BGArenas",
    "Visibility.Distance.Continents",
    "Visibility.Distance.Instances" });

    if (reload)
        AddOption<int32>("ClientCacheVersion");

    // Check options can't be changed at worldserver.conf reload
    if (reload)
    {
        for (auto const& [optionName, optionValue] : _notChangeConfigs)
        {
            int32 configValue = sConfigMgr->GetOption<int32>(optionName, optionValue);

            if (configValue != optionValue)
                LOG_ERROR("server.loading", "{} option can't be changed at worldserver.conf reload, using current value ({})", optionName, optionValue);

            SetOption<int32>(optionName, optionValue);
        }
    }

    LOG_INFO("server.loading", "> Loaded {} config options", static_cast<uint32>(_configOptions.size()));
}

void GameConfig::CheckOptions(bool reload /*= false*/)
{
    ///- Read the player limit and the Message of the day from the config file
    if (!reload)
        sWorld->SetPlayerAmountLimit(CONF_GET_INT("PlayerLimit"));

    Motd::SetMotd(CONF_GET_STR("Motd"));

    ///- Get string for new logins (newly created characters)
    sWorld->SetNewCharString(CONF_GET_STR("PlayerStart.String"));

    ///- Read all rates from the config file
    auto CheckRate = [this](std::string const& optionName)
    {
        auto _rate = CONF_GET_FLOAT(optionName);

        if (_rate < 0.0f)
        {
            LOG_ERROR("server.loading", "{} ({}) must be > 0. Using 1 instead.", optionName, _rate);
            SetOption<float>(optionName, 1.0f);
        }
    };

    CheckRate("Rate.Health");
    CheckRate("Rate.Mana");
    CheckRate("Rate.MoveSpeed");
    CheckRate("Rate.Rage.Income");
    CheckRate("Rate.Rage.Loss");
    CheckRate("Rate.RepairCost");
    CheckRate("Rate.RunicPower.Income");
    CheckRate("Rate.RunicPower.Loss");
    CheckRate("Rate.Talent");

    int32 tempIntOption = 0;
    float tempFloatOption = 1.0f;

    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
        playerBaseMoveSpeed[i] = baseMoveSpeed[i] * CONF_GET_FLOAT("Rate.MoveSpeed");

    tempFloatOption = CONF_GET_FLOAT("TargetPosRecalculateRange");
    if (tempFloatOption < CONTACT_DISTANCE)
    {
        LOG_ERROR("server.loading", "TargetPosRecalculateRange ({}) must be >= {}. Using {} instead.",
            tempFloatOption, CONTACT_DISTANCE, CONTACT_DISTANCE);

        SetOption<float>("TargetPosRecalculateRange", CONTACT_DISTANCE);
    }
    else if (tempFloatOption > NOMINAL_MELEE_RANGE)
    {
        LOG_ERROR("server.loading", "TargetPosRecalculateRange ({}) must be <= {}. Using {} instead",
            tempFloatOption, NOMINAL_MELEE_RANGE, NOMINAL_MELEE_RANGE);

        SetOption<float>("TargetPosRecalculateRange", NOMINAL_MELEE_RANGE);
    }

    tempFloatOption = CONF_GET_FLOAT("DurabilityLoss.OnDeath");
    if (tempFloatOption < 0.0f)
    {
        LOG_ERROR("server.loading", "DurabilityLoss.OnDeath ({}) must be >= 0. Using 0.0 instead", tempFloatOption);
        SetOption<float>("DurabilityLoss.OnDeath", 0.0f);
    }
    else if (tempFloatOption > 100.0f)
    {
        LOG_ERROR("server.loading", "DurabilityLoss.OnDeath ({}) must be <= 100. Using 100.0 instead", tempFloatOption);
        SetOption<float>("DurabilityLoss.OnDeath", 0.0f);
    }

    // ???
    SetOption<float>("DurabilityLoss.OnDeath", tempFloatOption / 100.0f);

    auto CheckDurabilityLossChance = [this](std::string const& optionName)
    {
        float option = CONF_GET_FLOAT(optionName);
        if (option < 0.0f)
        {
            LOG_ERROR("server.loading", "{} ({}) must be >= 0. Using 0.0 instead", optionName, option);
            SetOption<float>(optionName, 1.0f);
        }
    };

    CheckDurabilityLossChance("DurabilityLossChance.Damage");
    CheckDurabilityLossChance("DurabilityLossChance.Absorb");
    CheckDurabilityLossChance("DurabilityLossChance.Parry");
    CheckDurabilityLossChance("DurabilityLossChance.Block");

    ///- Read other configuration items from the config file
    tempIntOption = CONF_GET_INT("Compression");
    if (tempIntOption < 1 || tempIntOption > 9)
    {
        LOG_ERROR("server.loading", "Compression level ({}) must be in range 1..9. Using default compression level (1).", tempIntOption);
        SetOption<int32>("Compression", 1);
    }

    tempIntOption = CONF_GET_INT("PlayerSave.Stats.MinLevel");
    if (tempIntOption > MAX_LEVEL)
    {
        LOG_ERROR("game.config", "PlayerSave.Stats.MinLevel ({}) must be in range 0..80. Using default, do not save character stats (0).", tempIntOption);
        SetOption<int32>("PlayerSave.Stats.MinLevel", 0);
    }

    tempIntOption = CONF_GET_INT("MapUpdateInterval");
    if (tempIntOption < MIN_MAP_UPDATE_DELAY)
    {
        LOG_ERROR("server.loading", "MapUpdateInterval ({}) must be greater {}. Use this minimal value.", tempIntOption, MIN_MAP_UPDATE_DELAY);
        SetOption<int32>("MapUpdateInterval", MIN_MAP_UPDATE_DELAY);
    }

    if (reload)
        sMapMgr->SetMapUpdateInterval(tempIntOption);

    auto CheckMinName = [this](std::string const& optionName, int32 const& maxNameSymols)
    {
        int32 confSymbols = CONF_GET_INT(optionName);
        if (confSymbols < 1 || confSymbols > maxNameSymols)
        {
            LOG_ERROR("server.loading", "{} ({}) must be in range 1..{}. Set to 2.", optionName, confSymbols, maxNameSymols);
            SetOption<int32>(optionName, 2);
        }
    };

    CheckMinName("MinPlayerName", MAX_PLAYER_NAME);
    CheckMinName("MinCharterName", MAX_CHARTER_NAME);
    CheckMinName("MinPetName", MAX_PET_NAME);

    int32 charactersPerRealm = CONF_GET_INT("CharactersPerRealm");
    if (charactersPerRealm < 1 || charactersPerRealm > 10)
    {
        LOG_ERROR("server.loading", "CharactersPerRealm ({}) must be in range 1..10. Set to 10.", charactersPerRealm);
        SetOption<int32>("CharactersPerRealm", 10);
    }

    // must be after "CharactersPerRealm"
    tempIntOption = CONF_GET_INT("CharactersPerAccount");
    if (tempIntOption < charactersPerRealm)
    {
        LOG_ERROR("server.loading", "CharactersPerAccount ({}) can't be less than CharactersPerRealm ({}).", tempIntOption, charactersPerRealm);
        SetOption<int32>("CharactersPerAccount", charactersPerRealm);
    }

    tempIntOption = CONF_GET_INT("HeroicCharactersPerRealm");
    if (tempIntOption < 0 || tempIntOption > 10)
    {
        LOG_ERROR("server.loading", "HeroicCharactersPerRealm ({}) must be in range 0..10. Set to 1.", tempIntOption);
        SetOption<int32>("HeroicCharactersPerRealm", 1);
    }

    tempIntOption = CONF_GET_INT("SkipCinematics");
    if (tempIntOption < 0 || tempIntOption > 2)
    {
        LOG_ERROR("server.loading", "SkipCinematics ({}) must be in range 0..2. Set to 0.", tempIntOption);
        SetOption<int32>("SkipCinematics", 0);
    }

    int32 maxPlayerLevel = CONF_GET_INT("MaxPlayerLevel");
    if (maxPlayerLevel > MAX_LEVEL)
    {
        LOG_ERROR("server.loading", "MaxPlayerLevel ({}) must be in range 1..{}. Set to {}.", maxPlayerLevel, MAX_LEVEL, MAX_LEVEL);
        SetOption<int32>("MaxPlayerLevel", MAX_LEVEL);
    }

    int32 startPlayerLevel = CONF_GET_INT("StartPlayerLevel");
    if (startPlayerLevel < 1)
    {
        LOG_ERROR("server.loading", "StartPlayerLevel ({}) must be in range 1..MaxPlayerLevel({}). Set to 1.", startPlayerLevel, maxPlayerLevel);
        SetOption<int32>("StartPlayerLevel", 1);
    }
    else if (startPlayerLevel > maxPlayerLevel)
    {
        LOG_ERROR("server.loading", "StartPlayerLevel ({}) must be in range 1..MaxPlayerLevel({}). Set to {}.", tempIntOption, maxPlayerLevel, maxPlayerLevel);
        SetOption<int32>("StartPlayerLevel", maxPlayerLevel);
    }

    tempIntOption = CONF_GET_INT("StartHeroicPlayerLevel");
    if (tempIntOption < 1)
    {
        LOG_ERROR("server.loading", "StartHeroicPlayerLevel ({}) must be in range 1..MaxPlayerLevel({}). Set to 55.", tempIntOption, maxPlayerLevel);
        SetOption<int32>("StartHeroicPlayerLevel", 55);
    }
    else if (tempIntOption > maxPlayerLevel)
    {
        LOG_ERROR("server.loading", "StartHeroicPlayerLevel ({}) must be in range 1..MaxPlayerLevel({}). Set to {}.", tempIntOption, maxPlayerLevel, maxPlayerLevel);
        SetOption<int32>("StartHeroicPlayerLevel", maxPlayerLevel);
    }

    tempIntOption = CONF_GET_INT("StartPlayerMoney");
    if (tempIntOption < 0)
    {
        LOG_ERROR("server.loading", "StartPlayerMoney ({}) must be in range 0..{}. Set to {}.", tempIntOption, MAX_MONEY_AMOUNT, 0);
        SetOption<int32>("StartPlayerMoney", 0);
    }
    else if (tempIntOption > MAX_MONEY_AMOUNT)
    {
        LOG_ERROR("server.loading", "StartPlayerMoney ({}) must be in range 0..{}. Set to {}.", tempIntOption, MAX_MONEY_AMOUNT, MAX_MONEY_AMOUNT);
        SetOption<int32>("StartPlayerMoney", MAX_MONEY_AMOUNT);
    }

    auto CheckPoints = [this](std::string const& startPointsOptionName, std::string const& maxPointsOptionName)
    {
        int32 maxPoints = CONF_GET_INT(maxPointsOptionName);
        if (maxPoints < 0)
        {
            LOG_ERROR("server.loading", "{} ({}) can't be negative. Set to 0.", maxPointsOptionName, maxPoints);
            SetOption<int32>(maxPointsOptionName, 0);
        }

        int32 startPoints = CONF_GET_INT(startPointsOptionName);
        if (startPoints < 0)
        {
            LOG_ERROR("server.loading", "{} ({}) must be in range 0..{}({}). Set to {}.", startPointsOptionName, startPoints, maxPointsOptionName, maxPoints, 0);
            SetOption<int32>(startPointsOptionName, 0);
        }
        else if (startPoints > maxPoints)
        {
            LOG_ERROR("server.loading", "{} ({}) must be in range 0..{}({}). Set to {}.", startPointsOptionName, startPoints, maxPointsOptionName, maxPoints, maxPoints);
            SetOption<int32>(startPointsOptionName, maxPoints);
        }
    };

    CheckPoints("StartHonorPoints", "MaxHonorPoints");
    CheckPoints("StartArenaPoints", "MaxArenaPoints");

    tempIntOption = CONF_GET_INT("RecruitAFriend.MaxLevel");
    if (tempIntOption > maxPlayerLevel)
    {
        LOG_ERROR("server.loading", "RecruitAFriend.MaxLevel ({}) must be in the range 0..MaxLevel({}). Set to {}.",
            tempIntOption, maxPlayerLevel, 60);

        SetOption<int32>("RecruitAFriend.MaxLevel", 60);
    }

    tempIntOption = CONF_GET_INT("MinPetitionSigns");
    if (tempIntOption > 9)
    {
        LOG_ERROR("server.loading", "MinPetitionSigns ({}) must be in range 0..9. Set to 9.", tempIntOption);
        SetOption<int32>("MinPetitionSigns", 9);
    }

    tempIntOption = CONF_GET_INT("GM.StartLevel");
    if (tempIntOption < startPlayerLevel)
    {
        LOG_ERROR("server.loading", "GM.StartLevel ({}) must be in range StartPlayerLevel({})..{}. Set to {}.", tempIntOption, tempIntOption, MAX_LEVEL, tempIntOption);
        SetOption<int32>("GM.StartLevel", startPlayerLevel);
    }
    else if (tempIntOption > MAX_LEVEL)
    {
        LOG_ERROR("server.loading", "GM.StartLevel ({}) must be in range 1..{}. Set to {}.", tempIntOption, MAX_LEVEL, MAX_LEVEL);
        SetOption<int32>("GM.StartLevel", MAX_LEVEL);
    }

    auto CheckQueuesstLevelDiff = [this](std::string const& optionName)
    {
        auto diff = GetOption<int32>(optionName);

        if (diff > MAX_LEVEL)
            SetOption<int32>(optionName, MAX_LEVEL);
    };

    CheckQueuesstLevelDiff("Quests.LowLevelHideDiff");
    CheckQueuesstLevelDiff("Quests.HighLevelHideDiff");

    tempIntOption = CONF_GET_INT("UpdateUptimeInterval");
    if (tempIntOption <= 0)
    {
        LOG_ERROR("server.loading", "UpdateUptimeInterval ({}) must be > 0, set to default 10.", tempIntOption);
        SetOption<int32>("UpdateUptimeInterval", 10); // 10
    }

    // log db cleanup interval
    /*tempIntOption = CONF_GET_INT("LogDB.Opt.ClearInterval");
    if (tempIntOption <= 0)
    {
        LOG_ERROR("server.loading", "LogDB.Opt.ClearInterval ({}) must be > 0, set to default 10.", tempIntOption);
        SetOption<int32>("LogDB.Opt.ClearInterval", 10);
    }

    LOG_TRACE("server.loading", "Will clear `logs` table of entries older than {} seconds every {} minutes.",
        CONF_GET_INT("LogDB.Opt.ClearTime"), CONF_GET_INT("LogDB.Opt.ClearInterval"));*/

    tempIntOption = CONF_GET_INT("MaxOverspeedPings");
    if (tempIntOption != 0 && tempIntOption < 2)
    {
        LOG_ERROR("server.loading", "MaxOverspeedPings ({}) must be in range 2..infinity (or 0 to disable check). Set to 2.", tempIntOption);
        SetOption<int32>("MaxOverspeedPings", 2);
    }

    auto CheckResetTime = [this](std::string const& optionName)
    {
        int32 hours = CONF_GET_INT(optionName);
        if (hours > 23)
        {
            LOG_ERROR("server.loading", "{} ({}) can't be load. Set to 6.", optionName, hours);
            SetOption<int32>(optionName, 6);
        }
    };

    CheckResetTime("Battleground.Random.ResetHour");
    CheckResetTime("Calendar.DeleteOldEventsHour");
    CheckResetTime("Guild.ResetHour");

    tempIntOption = CONF_GET_INT("Battleground.ReportAFK");
    if (tempIntOption < 1 || tempIntOption > 9)
    {
        LOG_ERROR("server.loading", "Battleground.ReportAFK ({}) must be >0 and <10. Using 3 instead.", tempIntOption);
        SetOption<int32>("Battleground.ReportAFK", 3);
    }

    tempIntOption = CONF_GET_INT("Battleground.PlayerRespawn");
    if (tempIntOption < 3)
    {
        LOG_ERROR("server.loading", "Battleground.PlayerRespawn ({}) must be >2. Using 30 instead.", tempIntOption);
        SetOption<int32>("Battleground.PlayerRespawn", 30);
    }

    auto CheckBuffBG = [this](std::string const& optionName, uint32 defaultTime)
    {
        uint32 time = CONF_GET_INT(optionName);
        if (time < 1)
        {
            LOG_ERROR("server.loading", "{} ({}) must be > 0. Using {} instead.", optionName, defaultTime);
            SetOption<int32>(optionName, defaultTime);
        }
    };

    CheckBuffBG("Battleground.RestorationBuffRespawn", 20);
    CheckBuffBG("Battleground.BerserkingBuffRespawn", 120);
    CheckBuffBG("Battleground.SpeedBuffRespawn", 150);

    // always use declined names in the russian client
    SetOption<bool>("DeclinedNames", CONF_GET_INT("RealmZone") == REALM_ZONE_RUSSIAN ? true : CONF_GET_BOOL("DeclinedNames"));

    if (int32 clientCacheId = CONF_GET_INT("ClientCacheVersion"))
    {
        // overwrite DB/old value
        if (clientCacheId)
        {
            SetOption<int32>("ClientCacheVersion", clientCacheId);
            LOG_INFO("server.loading", "Client cache version set to: {}", clientCacheId);
        }
        else
            LOG_ERROR("server.loading", "ClientCacheVersion can't be negative {}, ignored.", clientCacheId);
    }

    auto CheckLogRecordsCount = [this](std::string const& optionName, int32 const& maxRecords)
    {
        int32 records = CONF_GET_INT(optionName);
        if (records > maxRecords)
            SetOption<int32>(optionName, maxRecords);
    };

    CheckLogRecordsCount("Guild.EventLogRecordsCount", GUILD_EVENTLOG_MAX_RECORDS);
    CheckLogRecordsCount("Guild.BankEventLogRecordsCount", GUILD_BANKLOG_MAX_RECORDS);

    if (CONF_GET_BOOL("PlayerStart.AllSpells"))
        LOG_WARN("server.loading", "WORLD: WARNING: PlayerStart.AllSpells enabled - may not function as intended!");

    tempIntOption = CONF_GET_INT("PvPToken.ItemCount");
    if (tempIntOption < 1)
        SetOption<int32>("PvPToken.ItemCount", 1);

    tempIntOption = CONF_GET_INT("PacketSpoof.BanMode");
    if (tempIntOption == 1 || tempIntOption > 2)
    {
        LOG_ERROR("server.loading", "> AntiDOS: Invalid ban mode {}. Set 0", tempIntOption);
        SetOption<int32>("PacketSpoof.BanMode", 0);
    }
}

#define TEMPLATE_GAME_CONFIG_OPTION(__typename) \
    template WH_GAME_API void GameConfig::AddOption(std::string const& optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API __typename GameConfig::GetOption(std::string const& optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API void GameConfig::SetOption(std::string const& optionName, __typename value) const;

TEMPLATE_GAME_CONFIG_OPTION(uint8)
TEMPLATE_GAME_CONFIG_OPTION(int8)
TEMPLATE_GAME_CONFIG_OPTION(uint16)
TEMPLATE_GAME_CONFIG_OPTION(int16)
TEMPLATE_GAME_CONFIG_OPTION(uint32)
TEMPLATE_GAME_CONFIG_OPTION(int32)
TEMPLATE_GAME_CONFIG_OPTION(uint64)
TEMPLATE_GAME_CONFIG_OPTION(int64)

#undef TEMPLATE_GAME_CONFIG_OPTION
