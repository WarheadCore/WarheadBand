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

#include "Vip.h"
#include "Log.h"
#include "GameConfig.h"
#include "TaskScheduler.h"
#include "ObjectGuid.h"
#include "GameTime.h"
#include "Timer.h"
#include "Optional.h"
#include "Tokenize.h"
#include "Chat.h"
#include <unordered_map>
#include <tuple>

namespace
{
    using WarheadVip = std::tuple<uint64/*start*/, uint64/*end*/, uint8/*level*/>;
    using WarheadVipRates = std::tuple<float/*XP*/, float/*Honor*/, float/*ArenaPoint*/, float/*Reputation*/>;

    std::unordered_map<uint32/*acc id*/, WarheadVip> store;
    std::unordered_map<uint8/*level*/, WarheadVipRates> storeRates;
    std::unordered_map<uint64/*guid*/, uint64/*time*/> storeUnbind;
    std::vector<uint32> vipSpellsStore;
    TaskScheduler scheduler;

    Optional<WarheadVip> GetVipInfo(uint32 accountID)
    {
        auto const& itr = store.find(accountID);

        if (itr == store.end())
            return std::nullopt;

        return itr->second;
    }

    Optional<WarheadVipRates> GetVipRateInfo(uint32 accountID)
    {
        auto const& itr = storeRates.find(accountID);

        if (itr == storeRates.end())
            return std::nullopt;

        return itr->second;
    }

    Optional<uint64> GetUndindTime(uint64 guid)
    {
        auto const& itr = storeUnbind.find(guid);

        if (itr == storeUnbind.end())
            return std::nullopt;

        return itr->second;
    }
}

namespace fmt
{
    template<>
    struct formatter<WarheadVip> : formatter<string_view>
    {
        // parse is inherited from formatter<string_view>.
        template <typename FormatContext>
        auto format(Optional<WarheadVip> vipInfo, FormatContext& ctx)
        {
            string_view info = "<unknown>";

            if (vipInfo)
            {
                auto const& [startTime, endTime, level] = *vipInfo;

                info = Warhead::StringFormat("Start time: {}. End time: {}. Level: {}",
                    Warhead::Time::TimeToHumanReadable(startTime), Warhead::Time::TimeToHumanReadable(endTime), level);
            }

            return formatter<string_view>::format(info, ctx);
        }
    };
}

Vip* Vip::Instance()
{
    static Vip instance;
    return &instance;
}

void Vip::InitSystem(bool reload)
{
    Seconds delay = Seconds(CONF_GET_UINT("VIP.Update.Delay"));

    if (delay < 10s)
    {
        LOG_ERROR("modules.vip", "> Vip: Delay < 10 seconds. Set to 10 seconds");
        delay = 10s;
    }

    scheduler.CancelAll();

    scheduler.Schedule(3s, [this, delay](TaskContext context)
    {
        for (auto const& [accountID, vipInfo] : store)
        {
            auto const& [startTime, endTime, level] = vipInfo;

            if (GameTime::GetGameTime() >= endTime)
                UnSet(accountID);
        }

        context.Repeat(delay);
    });

    if (!reload)
    {
        LoadAccounts();
        LoadRates();
        LoadUnbinds();
    }

    std::string configSpells = CONF_GET_STR("VIP.Spells.List");
    for (auto const& spellString : Warhead::Tokenize(configSpells, ',', false))
    {
        auto spellID = Warhead::StringTo<uint32>(spellString);

        if (!spellID)
        {
            LOG_ERROR("modules.vip", "> Incorrect spell '{}' in vip spell list. Skip", spellString);
            continue;
        }

        vipSpellsStore.emplace_back(*spellID);
    }
}

void Vip::Update(uint32 diff)
{
    scheduler.Update(diff);
}

bool Vip::Add(uint32 accountID, uint64 endTime, uint8 level, bool force /*= false*/)
{
    auto const& vipInfo = GetVipInfo(accountID);

    if (vipInfo)
    {
        // Set inactive all in DB
        if (force)
            Delete(accountID);
        else // Add error
        {
            LOG_ERROR("modules.vip", "> Vip: Account {} is premium. {}", accountID, *vipInfo);
            return false;
        }
    }

    auto timeNow = GameTime::GetGameTime();

    store.emplace(accountID, std::make_tuple(timeNow, endTime, level));

    // Add DB
    LoginDatabase.PExecute("INSERT INTO `account_premium` (`AccountID`, `StartTime`, `EndTime`, `Level`, `IsActive`) VALUES ({}, FROM_UNIXTIME({}), FROM_UNIXTIME({}), {}, 1)",
        accountID, timeNow, endTime, level);

    if (auto targetSession = sWorld->FindSession(accountID))
        ChatHandler(targetSession).PSendSysMessage("> У вас обновление премиум статуса. Уровень {}. Окончание через {}", level, Warhead::Time::ToTimeString<Seconds>(endTime - GameTime::GetGameTime(), TimeOutput::Seconds, TimeFormat::FullText));

    return true;
}

bool Vip::Delete(uint32 accountID)
{
    auto const& vipInfo = GetVipInfo(accountID);
    if (!vipInfo)
    {
        LOG_ERROR("modules.vip", "> Vip: Account {} is not premium", accountID);
        return false;
    }

    // Set inactive in DB
    LoginDatabase.PExecute("UPDATE `account_premium` SET `IsActive` = 0 WHERE `AccountID` = {} AND `IsActive` = 1",
        accountID);

    store.erase(accountID);

    return true;
}

void Vip::OnLoginPlayer(Player* player)
{
    auto mountSpellID = CONF_GET_UINT("VIP.Mount.SpellID");

    if (!sVip->IsVip(player))
    {
        // Mount spells
        if (player->HasSpell(mountSpellID))
            player->removeSpell(mountSpellID, SPEC_MASK_ALL, false);

        // Vip spells
        for (auto const& spellID : vipSpellsStore)
        {
            if (player->HasSpell(spellID))
                player->removeSpell(spellID, SPEC_MASK_ALL, false);
        }
    }
    else
    {
        ChatHandler handler(player->GetSession());
        uint8 vipLevel = sVip->GetLevel(player);

        handler.PSendSysMessage("|cffff0000#|r --");
        handler.PSendSysMessage("|cffff0000#|r |cff00ff00Привет,|r {}!", player->GetName());
        handler.PSendSysMessage("|cffff0000#|r |cff00ff00Ваш уровень премиум аккаунта:|r {}", vipLevel);
        handler.PSendSysMessage("|cffff0000#|r |cff00ff00Оставшееся время:|r {}", sVip->GetDuration(player));

        // Learn mount
        if (!player->HasSpell(mountSpellID) && vipLevel >= CONF_GET_UINT("VIP.Mount.MinLevel"))
            player->learnSpell(mountSpellID);

        // Learn vip spells
        for (auto const& spellID : vipSpellsStore)
        {
            if (!player->HasSpell(spellID))
                player->learnSpell(spellID);
        }
    }
}

void Vip::OnLogoutPlayer(Player* player)
{
    for (auto const& spellID : vipSpellsStore)
    {
        if (player->HasSpell(spellID))
            player->removeSpell(spellID, SPEC_MASK_ALL, false);
    }
}

void Vip::UnSet(uint32 accountID)
{
    auto targetSession = sWorld->FindSession(accountID);

    if (targetSession)
        ChatHandler(targetSession).PSendSysMessage("> Вы лишены статуса премиум аккаунта");

    Delete(accountID);
}

bool Vip::IsVip(Player* player)
{
    if (!player)
        return false;

    return GetVipInfo(player->GetSession()->GetAccountId()) != std::nullopt;
}

bool Vip::IsVip(uint32 accountID)
{
    if (!accountID)
        return false;

    return GetVipInfo(accountID) != std::nullopt;
}

uint8 Vip::GetLevel(Player* player)
{
    if (!player)
        return 0;

    auto accountID = player->GetSession()->GetAccountId();

    auto const& vipInfo = GetVipInfo(accountID);
    if (!vipInfo)
    {
        LOG_ERROR("modules.vip", "> Vip::GetLevel: Account {} is not premium", accountID);
        return 0;
    }

    return std::get<2>(*vipInfo);
}

template<>
float Vip::GetRate<VipRate::XP>(Player* player)
{
    if (!IsVip(player))
        return 1.0f;

    auto level = GetLevel(player);
    auto accountID = player->GetSession()->GetAccountId();

    auto const& vipRateInfo = GetVipRateInfo(accountID);
    if (!vipRateInfo)
    {
        LOG_ERROR("modules.vip", "> Vip: Vip Account {} [{}] is incorrect vip level {}. {}", accountID, *GetVipInfo(accountID), level);
        return 1.0f;
    }

    return std::get<0>(*vipRateInfo);
}

template<>
float Vip::GetRate<VipRate::Honor>(Player* player)
{
    if (!IsVip(player))
        return 1.0f;

    auto level = GetLevel(player);
    auto accountID = player->GetSession()->GetAccountId();

    auto const& vipRateInfo = GetVipRateInfo(accountID);
    if (!vipRateInfo)
    {
        LOG_ERROR("modules.vip", "> Vip: Vip Account {} [{}] is incorrect vip level {}. {}", accountID, *GetVipInfo(accountID), level);
        return 1.0f;
    }

    return std::get<1>(*vipRateInfo);
}

template<>
float Vip::GetRate<VipRate::ArenaPoint>(Player* player)
{
    if (!IsVip(player))
        return 1.0f;

    auto level = GetLevel(player);
    auto accountID = player->GetSession()->GetAccountId();

    auto const& vipRateInfo = GetVipRateInfo(accountID);
    if (!vipRateInfo)
    {
        LOG_ERROR("modules.vip", "> Vip: Vip Account {} [{}] is incorrect vip level {}. {}", accountID, *GetVipInfo(accountID), level);
        return 1.0f;
    }

    return std::get<2>(*vipRateInfo);
}

template<>
float Vip::GetRate<VipRate::Reputation>(Player* player)
{
    if (!IsVip(player))
        return 1.0f;

    auto level = GetLevel(player);
    auto accountID = player->GetSession()->GetAccountId();

    auto const& vipRateInfo = GetVipRateInfo(accountID);
    if (!vipRateInfo)
    {
        LOG_ERROR("modules.vip", "> Vip: Vip Account {} [{}] is incorrect vip level {}. {}", accountID, *GetVipInfo(accountID), level);
        return 1.0f;
    }

    return std::get<3>(*vipRateInfo);
}

std::string Vip::GetDuration(Player* player)
{
    std::string duration = "<неизвестно>";

    if (!player)
        return duration;

    auto info = ::GetVipInfo(player->GetSession()->GetAccountId());

    if (!info)
        return duration;

    auto const& [startTime, endTime, level] = *info;

    duration = Warhead::Time::ToTimeString<Seconds>(endTime - GameTime::GetGameTime(), TimeOutput::Seconds, TimeFormat::FullText);

    return duration;
}

std::string Vip::GetDuration(uint32 accountID)
{
    std::string duration = "<неизвестно>";

    if (!accountID)
        return duration;

    auto info = ::GetVipInfo(accountID);

    if (!info)
        return duration;

    auto const& [startTime, endTime, level] = *info;

    duration = Warhead::Time::ToTimeString<Seconds>(endTime - GameTime::GetGameTime(), TimeOutput::Seconds, TimeFormat::FullText);

    return duration;
}

void Vip::RemoveColldown(Player* player, uint32 spellID)
{
    scheduler.Schedule(1s, [this, player, spellID](TaskContext /*context*/)
    {
        if (player && player->HasSpellCooldown(spellID))
            player->RemoveSpellCooldown(spellID, true);
    });
}

void Vip::UnBindInstances(Player* player)
{
    if (!player)
        return;

    ChatHandler handler(player->GetSession());

    if (!IsVip(player))
    {
        handler.PSendSysMessage("> У вас нет премиум статуса");
        return;
    }

    if (GetLevel(player) < 3)
    {
        handler.PSendSysMessage("> У вас недостаточно высокий уровень для этого");
        return;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto unbindInfo = GetUndindTime(guid);
    auto confDuration = CONF_GET_UINT("VIP.Unbind.Duration");

    if (unbindInfo)
    {
        auto duration = GameTime::GetGameTime() - *unbindInfo;

        if (duration < confDuration)
        {
            auto diff = confDuration - duration;

            handler.PSendSysMessage("> Это можно сделать через: {}", Warhead::Time::ToTimeString<Seconds>(diff));
            return;
        }
    }

    auto GetMapName = [](uint32 mapID, uint8 localeIndex)
    {
        MapEntry const* map = sMapStore.LookupEntry(mapID);
        if (!map)
            return "Неизвестно";

        return map->name[localeIndex];
    };

    auto GetDiffName = [](Difficulty diff, bool isRaid)
    {
        if (isRaid)
        {
            switch (diff)
            {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                return "10 об.";
            case RAID_DIFFICULTY_25MAN_NORMAL:
                return "25 об.";
            case RAID_DIFFICULTY_10MAN_HEROIC:
                return "10 гер.";
            case RAID_DIFFICULTY_25MAN_HEROIC:
                return "25 гер.";
            default:
                break;
            }
        }
        else // if dungeon
        {
            switch (diff)
            {
            case DUNGEON_DIFFICULTY_NORMAL:
                return "5 об.";
            case DUNGEON_DIFFICULTY_HEROIC:
                return "5 гер.";
            default:
                break;
            }
        }

        return "###";
    };

    uint32 count = 0;

    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        auto diff = Difficulty(i);

        for (auto const& [mapID, bind] : sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), diff))
        {
            if (player->GetMapId() == mapID)
                continue;

            handler.PSendSysMessage("> {} {}", GetMapName(mapID, handler.GetSessionDbLocaleIndex()), GetDiffName(diff, bind.save->GetMapEntry()->IsRaid()));
            sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), mapID, diff, true, player);
            CharacterDatabase.PExecute("REPLACE INTO `vip_unbind` (`PlayerGuid`) VALUES ({})", player->GetGUID().GetRawValue());            
            count++;
        }
    }

    if (count)
        storeUnbind.emplace(guid, GameTime::GetGameTime());
    else
    {
        handler.PSendSysMessage("> Нечего сбрасывать");
        return;
    }
}

void Vip::LoadRates()
{
    uint32 oldMSTime = getMSTime();

    storeRates.clear(); // for reload case

    LOG_INFO("server.loading", "Load vip rates...");

    QueryResult result = CharacterDatabase.PQuery("SELECT VipLevel, RateXp, RateHonor, RateArenaPoint, RateReputation FROM vip_rates");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 Vip rates. DB table `vip_rates` is empty.");
        LOG_WARN("sql.sql", "");
        return;
    }

    auto CheckRate = [](uint8 level, std::initializer_list<float> rates)
    {
        for (auto const& rate : rates)
        {
            if (rate <= 1.0f)
            {
                LOG_ERROR("sql.sql", "> Vip: Invalid rate value ({}) for level ({})", rate, level);
                return false;
            }
        }

        return true;
    };

    do
    {
        Field* fields = result->Fetch();

        auto level          = fields[0].GetUInt8();
        auto rateXP         = fields[1].GetFloat();
        auto rateHonor      = fields[2].GetFloat();
        auto rateArenaPoint = fields[3].GetFloat();
        auto rateReputaion  = fields[4].GetFloat();

        if (!CheckRate(level, { rateXP, rateHonor, rateArenaPoint, rateReputaion }))
            continue;

        auto rates = std::make_tuple(rateXP, rateHonor, rateArenaPoint, rateReputaion);

        storeRates.emplace(level, rates);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} vip rates in {}", storeRates.size(), Warhead::Time::ToTimeString<Milliseconds>(oldMSTime, TimeOutput::Milliseconds));
    LOG_INFO("server.loading", "");
}

void Vip::LoadAccounts()
{
    uint32 oldMSTime = getMSTime();

    store.clear(); // for reload case

    LOG_INFO("server.loading", "Load vip accounts...");

    QueryResult result = LoginDatabase.PQuery("SELECT AccountID, UNIX_TIMESTAMP(StartTime), UNIX_TIMESTAMP(EndTime), Level FROM account_premium WHERE IsActive = 1");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 vip accounts. DB table `account_premium` is empty.");
        LOG_WARN("sql.sql", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        auto accountID  = fields[0].GetUInt32();
        auto startTime  = fields[1].GetFloat();
        auto endTime    = fields[2].GetFloat();
        auto level      = fields[3].GetFloat();

        if (level > 3)
        {
            LOG_ERROR("sql.sql", "> Account {} has a incorrect level of {}. Skip", level);
            continue;
        }

        auto info = std::make_tuple(startTime, endTime, level);

        store.emplace(accountID, info);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} vip accounts in {}", store.size(), Warhead::Time::ToTimeString<Milliseconds>(oldMSTime, TimeOutput::Milliseconds));
    LOG_INFO("server.loading", "");
}

void Vip::LoadUnbinds()
{
    uint32 oldMSTime = getMSTime();

    storeUnbind.clear(); // for reload case

    LOG_INFO("server.loading", "Load vip unbinds...");

    QueryResult result = CharacterDatabase.PQuery("SELECT PlayerGuid, UNIX_TIMESTAMP(UnbindTime) FROM vip_unbind");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 vip unbinds. DB table `vip_unbind` is empty.");
        LOG_WARN("sql.sql", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        auto guid = fields[0].GetUInt64();
        auto unbindTime = fields[1].GetUInt64();

        storeUnbind.emplace(guid, unbindTime);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} vip unbinds in {}", storeUnbind.size(), Warhead::Time::ToTimeString<Milliseconds>(oldMSTime, TimeOutput::Milliseconds));
    LOG_INFO("server.loading", "");
}
