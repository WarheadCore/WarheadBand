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

#include "AsyncCallbackProcessor.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "ModulesConfig.h"
#include "ExternalMail.h"
#include "TaskScheduler.h"
#include "Player.h"
#include "CharacterCache.h"
#include "Optional.h"
#include "Realm.h"
#include "StopWatch.h"
#include <vector>
#include <unordered_map>

enum class IPSShopType : uint8
{
    Item,           // Item id
    CharRename,     // Character rename
    CharRerace,     // Character rerace
    Faction         // Character faction
};

// `ips_shop_define`
struct IPSShopDefine
{
    IPSShopDefine(uint32 id, uint32 type, uint32 value) :
        ShopID(id), Type(static_cast<IPSShopType>(type)), Value(value) { }

    uint32 ShopID{ 0 };
    IPSShopType Type{ IPSShopType::Item };
    uint32 Value{ 0 };
};

// `ips_shop_link`
struct IPSShopLink
{
    IPSShopLink(uint32 id, std::string_view nick, Optional<IPSShopDefine> itemID, uint32 itemCount) :
        ID(id), NickName(nick), ItemID(itemID), ItemCount(itemCount) { }

    uint32 ID{ 0 };
    std::string NickName;
    Optional<IPSShopDefine> ItemID;
    uint32 ItemCount{ 0 };
};

class DonateIPS
{
public:
    static DonateIPS* instance()
    {
        static DonateIPS instance;
        return &instance;
    }

    inline void LoadConfig()
    {
        sModulesConfig->AddOption("IPSShop.Enable");
        _IsEnable = MOD_CONF_GET_BOOL("IPSShop.Enable");
    }

    inline void Initialize()
    {
        if (!_IsEnable)
            return;

        LOG_INFO("server.loading", "Loading ips shop define...");
        LoadShopStore();

        _scheduler.Schedule(15s, [this](TaskContext context)
        {
            SendDonate();
            context.Repeat(5s);
        });
    }

    inline void LoadShopStore()
    {
        if (!_IsEnable)
            return;

        StopWatch sw;

        _shopStore.clear();

        QueryResult result = LoginDatabase.Query("SELECT `ID`, `Type`, `Value` FROM `ips_shop_define` ORDER BY `ID`");
        if (!result)
        {
            LOG_WARN("server.loading", "> No data for ips shop define");
            LOG_INFO("server.loading", "");
            return;
        }

        do
        {
            auto const& [shopID, type, value] = result->FetchTuple<uint32, uint32, uint32>();

            IPSShopDefine shopDefine = IPSShopDefine(shopID, type, value);

            if (shopDefine.Type == IPSShopType::Item)
            {
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(shopDefine.Value);
                if (!itemTemplate)
                {
                    LOG_ERROR("sql.sql", "> IPS Shop: Предмета под номером {} не существует. Пропуск", shopDefine.Value);
                    continue;
                }
            }
            else if (shopDefine.Type != IPSShopType::Item && shopDefine.Value)
            {
                LOG_ERROR("sql.sql", "> IPS Shop: Шоп айди ({}) не является предметом, для него не нужно указывать количество. Установлено 0", shopDefine.ShopID);
                shopDefine.Value = 0;
            }

            _shopStore.emplace(shopDefine.ShopID, shopDefine);

        } while (result->NextRow());

        LOG_INFO("server.loading", "> Loaded {} shop defines in {}", _shopStore.size(), sw);
        LOG_INFO("server.loading", "");
    }

    inline void SendDonate()
    {
        _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(Warhead::StringFormat("SELECT `ID`, `Nickname`, `ItemID`, `ItemQuantity` FROM `ips_shop_link` WHERE `flag` = 0 AND `RealmName` = '{}'", realm.Name)).WithCallback([this](QueryResult result)
        {
            _ipsShopLinkStore.clear();

            if (!result)
                return;

            LOG_TRACE("modules.ips", "> DonateIPS: SendDonate");

            do
            {
                auto const& [id, nickName, itemID, itemCount] = result->FetchTuple<uint32, std::string, uint32, uint32>();

                std::string playerName{ nickName };

                if (!normalizePlayerName(playerName))
                {
                    LOG_ERROR("module.ips", "> DonateIPS: Некорректное имя персонажа ({})", playerName);
                    continue;
                }

                auto playerGuid = sCharacterCache->GetCharacterGuidByName(playerName);
                if (playerGuid.IsEmpty())
                {
                    LOG_ERROR("module.ips", "> DonateIPS: Неверное имя персонажа ({})", playerName);
                    continue;
                }

                if (!sCharacterCache->GetCharacterNameByGuid(playerGuid, playerName))
                {
                    LOG_ERROR("module.ips", "> DonateIPS: Ошибка получения данных о персонаже ({})", playerName);
                    continue;
                }

                auto shopDefine = GetShopDefine(itemID);
                if (!shopDefine)
                {
                    LOG_ERROR("module.ips", "> Невозможно найти определение для номера {}", itemID);
                    continue;
                }

                _ipsShopLinkStore.emplace_back(IPSShopLink(id, nickName, shopDefine, itemCount));

            } while (result->NextRow());

            for (auto const& shopLink : _ipsShopLinkStore)
                SendRewardForPlayer(&shopLink);
        }));
    }

    inline void Update(Milliseconds diff)
    {
        if (!_IsEnable)
            return;

        _scheduler.Update(diff);
        _queryProcessor.ProcessReadyCallbacks();
    }

private:
    inline void SendRewardForPlayer(IPSShopLink const* ipsShopLink)
    {
        auto ipsShopDefine = ipsShopLink->ItemID;
        if (!ipsShopDefine)
        {
            LOG_FATAL("modules.ips", "> DonateIPS: Невозможно найти определение для номера {}", ipsShopLink->ID);
            return;
        }

        switch (ipsShopDefine->Type)
        {
        case IPSShopType::Item:
            SendRewardItem(ipsShopLink->NickName, ipsShopDefine->Value, ipsShopLink->ItemCount);
            break;
        case IPSShopType::CharRename:
            SendRewardRename(ipsShopLink->NickName);
            break;
        case IPSShopType::CharRerace:
            SendRewardChangeRace(ipsShopLink->NickName);
            break;
        case IPSShopType::Faction:
            SendRewardChangeFaction(ipsShopLink->NickName);
            break;
        default:
            LOG_FATAL("modules.ips", "> DonateIPS: Неверый тип шоп айди ({})", static_cast<uint32>(ipsShopDefine->Type));
            return;
        }

        LoginDatabase.Execute("UPDATE `ips_shop_link` SET `Flag` = 1 WHERE `ID` = {}", ipsShopLink->ID);
    }

    inline void SendRewardItem(std::string_view charName, uint32 itemID, uint32 itemCount)
    {
        sExternalMail->AddMail(charName, _thanksSubject, _thanksText, itemID, itemCount, 37688);
    }

    inline void SendRewardRename(std::string const& charName)
    {
        auto const& targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_RENAME), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    inline void SendRewardChangeRace(std::string const& charName)
    {
        auto targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_CHANGE_RACE), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    inline void SendRewardChangeFaction(std::string const& charName)
    {
        auto targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_CHANGE_FACTION), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    inline Optional<IPSShopDefine> GetShopDefine(uint32 shopID)
    {
        auto const& itr = _shopStore.find(shopID);
        if (itr != _shopStore.end())
            return itr->second;

        LOG_FATAL("modules.ips", "> DonateIPS: невозможно найти данные для шоп айди ({})", shopID);

        return std::nullopt;
    }

private:
    std::vector<IPSShopLink> _ipsShopLinkStore;
    std::unordered_map<uint32 /*shop id*/, IPSShopDefine> _shopStore;

    std::string const _thanksSubject = "Донат магазин";
    std::string const _thanksText = "Спасибо за покупку!";

    QueryCallbackProcessor _queryProcessor;
    TaskScheduler _scheduler;
    bool _IsEnable{ false };
};

#define sDonateIPS DonateIPS::instance()

class DonateIPS_World : public WorldScript
{
public:
    DonateIPS_World() : WorldScript("DonateIPS_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sDonateIPS->LoadConfig();
    }

    void OnStartup() override
    {
        sDonateIPS->Initialize();
    }

    void OnUpdate(uint32 diff) override
    {
        sDonateIPS->Update(Milliseconds(diff));
    }    
};

// Group all custom scripts
void AddSC_DonateIPS()
{
    new DonateIPS_World();
}
