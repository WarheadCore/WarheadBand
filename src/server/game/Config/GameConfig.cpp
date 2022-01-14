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

#include "GameConfig.h"
#include "Config.h"
#include "Log.h"
#include "MapMgr.h"
#include "Object.h"
#include "Player.h"
#include "ServerMotd.h"
#include "StringConvert.h"
#include "StringFormat.h"
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

    template<typename T>
    inline std::string GetDefaultValueString(Optional<T> def)
    {
        std::string defStr;

        if constexpr (std::is_same_v<T, bool>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_BOOL : *def);
        else if constexpr (std::is_integral_v<T>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_INT : *def);
        else if constexpr (std::is_floating_point_v<T>)
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_FLOAT : *def);
        else
            defStr = Warhead::StringFormat("{}", def == std::nullopt ? CONF_DEFAULT_STR : *def);

        return defStr;
    }

    template<typename T>
    constexpr T GetDefaultValue()
    {
        if constexpr (std::is_same_v<T, bool>)
            return CONF_DEFAULT_BOOL;
        else if constexpr (std::is_integral_v<T>)
            return CONF_DEFAULT_INT;
        else if constexpr (std::is_floating_point_v<T>)
            return CONF_DEFAULT_FLOAT;
        else
            return CONF_DEFAULT_STR;
    }
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
WH_GAME_API void GameConfig::AddOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option
    auto const& itr = _configOptions.find(option);
    if (itr != _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is already exists", optionName);
        return;
    }

    _configOptions.emplace(option, sConfigMgr->GetOption<std::string>(option, GetDefaultValueString<T>(def)));
}

// Add option without template
void GameConfig::AddOption(std::string_view optionName, Optional<std::string> def /*= std::nullopt*/) const
{
    AddOption<std::string>(optionName, def);
}

void GameConfig::AddOption(std::initializer_list<std::string> const& optionList) const
{
    for (auto const& option : optionList)
        AddOption(option);
}

// Get option
template<typename T>
WH_GAME_API T GameConfig::GetOption(std::string_view optionName, Optional<T> def /*= std::nullopt*/) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option part 1
    auto itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        AddOption(optionName, def);
    }

    std::string defStr = GetDefaultValueString(def);

    // Check exist option part 2
    itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        LOG_FATAL("server.loading", "> GameConfig: option ({}) is not exists. Returned ({})", optionName, defStr);
        return GetDefaultValue<T>();
    }

    Optional<T> result = {};

    if constexpr (std::is_same_v<T, std::string>)
        result = _configOptions.at(option);
    else
        result = Warhead::StringTo<T>(_configOptions.at(option));

    if (!result)
    {
        LOG_ERROR("server.loading", "> GameConfig: Bad value defined for '{}', use '{}' instead", optionName, defStr);
        return GetDefaultValue<T>();
    }

    return *result;
}

// Set option
template<typename T>
WH_GAME_API void GameConfig::SetOption(std::string_view optionName, T value) const
{
    // copy from string_view
    std::string option{ optionName };

    // Check exist option
    auto const& itr = _configOptions.find(option);
    if (itr == _configOptions.end())
    {
        LOG_ERROR("server.loading", "> GameConfig: option ({}) is not exists", optionName);
        return;
    }

    std::string valueStr{};

    if constexpr (std::is_same_v<T, std::string>)
        valueStr = value;
    else
        valueStr = Warhead::ToString(value);

    _configOptions.erase(option);
    _configOptions.emplace(option, valueStr);
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

    if (reload)
        AddOption<int32>("ClientCacheVersion");

    // Check options can't be changed at worldserver.conf reload
    if (reload)
    {
        for (auto const& [optionName, optionValue] : _notChangeConfigs)
        {
            int32 configValue = sConfigMgr->GetOption(optionName, optionValue);

            if (configValue != optionValue)
                LOG_ERROR("server.loading", "{} option can't be changed at worldserver.conf reload, using current value ({})", optionName, optionValue);

            SetOption<int32>(optionName, optionValue);
        }
    }

    LOG_INFO("server.loading", "> Loaded {} config options", _configOptions.size());
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
    template WH_GAME_API __typename GameConfig::GetOption(std::string_view optionName, Optional<__typename> def /*= std::nullopt*/) const; \
    template WH_GAME_API void GameConfig::SetOption(std::string_view optionName, __typename value) const;

TEMPLATE_GAME_CONFIG_OPTION(bool)
TEMPLATE_GAME_CONFIG_OPTION(uint8)
TEMPLATE_GAME_CONFIG_OPTION(int8)
TEMPLATE_GAME_CONFIG_OPTION(uint16)
TEMPLATE_GAME_CONFIG_OPTION(int16)
TEMPLATE_GAME_CONFIG_OPTION(uint32)
TEMPLATE_GAME_CONFIG_OPTION(int32)
TEMPLATE_GAME_CONFIG_OPTION(uint64)
TEMPLATE_GAME_CONFIG_OPTION(int64)
TEMPLATE_GAME_CONFIG_OPTION(float)
TEMPLATE_GAME_CONFIG_OPTION(std::string)

#undef TEMPLATE_GAME_CONFIG_OPTION
