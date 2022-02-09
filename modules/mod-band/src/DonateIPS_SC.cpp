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

#include "Log.h"
#include "ScriptMgr.h"
#include "GameConfig.h"
#include "ExternalMail.h"
#include "TaskScheduler.h"
#include "Player.h"
#include "CharacterCache.h"
#include "Optional.h"
#include <vector>
#include <unordered_map>

enum class IPSShopType : uint8
{
    IPS_SHOP_TYPE_ITEM,             // Item id
    IPS_SHOP_TYPE_CHAR_RENAME,      // Character rename
    IPS_SHOP_TYPE_CHAR_RACE,        // Character race
    IPS_SHOP_TYPE_CHAR_FACTION      // Character faction
};

struct IPSShop
{
    uint32 ShopID;
    IPSShopType Type;
    uint32 Value;
};

struct DonateIPSStruct
{
    uint32 ID;
    std::string CharName;
    Optional<IPSShop> ShopID = {};
    uint32 Value;
};

class DonateIPS
{
public:
    static DonateIPS* instance()
    {
        static DonateIPS instance;
        return &instance;
    }

    void LoadDonate()
    {
        if (!CONF_GET_BOOL("IPSShop.Enable"))
            return;

        _store.clear();

        QueryResult result = CharacterDatabase.Query("SELECT id, nickname, item_id, item_quantity FROM `shop_purchase` WHERE flag = 0");
        if (!result)
            return;

        LOG_TRACE("modules.ips", "> DonateIPS: SendDonate");

        do
        {
            auto const& [id, _playerName, shopID, value] = result->FetchTuple<uint32, std::string, uint32, uint32>();

            std::string playerName{ _playerName };

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

            auto shopData = GetShop(shopID);
            if (!shopData)
                continue;

            DonateIPSStruct _data = {};
            _data.ID = id;
            _data.CharName = playerName;
            _data.ShopID = *shopData;
            _data.Value = value;

            _store.emplace_back(_data);

        } while (result->NextRow());
    }

    void LoadShopStore()
    {
        if (!CONF_GET_BOOL("IPSShop.Enable"))
            return;

        _store.clear();

        QueryResult result = CharacterDatabase.Query("SELECT `ID`, `Type`, `Value` FROM `ips_shop` ORDER BY `ID`");
        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();

            IPSShop _data = {};
            _data.ShopID = fields[0].Get<uint32>();
            _data.Type = static_cast<IPSShopType>(fields[1].Get<uint32>());
            _data.Value = fields[2].Get<uint32>();

            if (_data.Type == IPSShopType::IPS_SHOP_TYPE_ITEM)
            {
                ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(_data.Value);
                if (!itemTemplate)
                {
                    LOG_ERROR("sql.sql", "> IPS Shop: Предмета под номером {} не существует. Пропуск", _data.Value);
                    continue;
                }
            }
            else if (_data.Type != IPSShopType::IPS_SHOP_TYPE_ITEM && _data.Value)
                LOG_ERROR("sql.sql", "> IPS Shop: Шоп айди ({}) не является предметом, для него не нужно указывать количество. Установлено 0", _data.ShopID);

            _shopStore.emplace(_data.ShopID, _data);

        } while (result->NextRow());
    }

    void SendDonate()
    {
        LoadDonate();

        for (auto const& itr : _store)
            SendRewardForPlayer(&itr);
    }
private:
    void SendRewardForPlayer(const DonateIPSStruct* ipsData)
    {
        auto shopID = ipsData->ShopID;
        if (!shopID)
            LOG_FATAL("modules.ips", "> DonateIPS: невозможно найти данные шоп айди для номера ({})", ipsData->ID);

        switch (shopID->Type)
        {
        case IPSShopType::IPS_SHOP_TYPE_ITEM:
            SendRewardItem(ipsData->CharName, shopID->Value, ipsData->Value);
            break;
        case IPSShopType::IPS_SHOP_TYPE_CHAR_RENAME:
            SendRewardRename(ipsData->CharName);
            break;
        case IPSShopType::IPS_SHOP_TYPE_CHAR_RACE:
            SendRewardChangeRace(ipsData->CharName);
            break;
        case IPSShopType::IPS_SHOP_TYPE_CHAR_FACTION:
            SendRewardChangeFaction(ipsData->CharName);
            break;
        default:
            LOG_FATAL("modules.ips", "> DonateIPS: Неверый тип шоп айди ({})", static_cast<uint32>(shopID->Type));
            return;
        }

        CharacterDatabase.Execute("UPDATE `shop_purchase` SET `flag` = 1 WHERE `id` = {}", ipsData->ID);
    }

    void SendRewardItem(std::string_view charName, uint32 itemID, uint32 itemCount)
    {
        sExternalMail->AddMail(charName, thanksSubject, thanksText, itemID, itemCount, 37688);
    }

    void SendRewardRename(std::string const& charName)
    {
        auto const& targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_RENAME), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    void SendRewardChangeRace(std::string const& charName)
    {
        auto targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_CHANGE_RACE), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    void SendRewardChangeFaction(std::string const& charName)
    {
        auto targetGuid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!targetGuid)
            return;

        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->SetArguments(uint16(AT_LOGIN_CHANGE_FACTION), targetGuid.GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    Optional<IPSShop> GetShop(uint32 shopID)
    {
        auto const& itr = _shopStore.find(shopID);
        if (itr != _shopStore.end())
            return itr->second;

        LOG_FATAL("modules.ips", "> DonateIPS: невозможно найти данные для шоп айди ({})", shopID);

        return std::nullopt;
    }

private:
    std::vector<DonateIPSStruct> _store;
    std::unordered_map<uint32 /*shop id*/, IPSShop> _shopStore;

    std::string const thanksSubject = "Донат магазин";
    std::string const thanksText = "Спасибо за покупку!";
};

#define sDonateIPS DonateIPS::instance()

class DonateIPS_World : public WorldScript
{
public:
    DonateIPS_World() : WorldScript("DonateIPS_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sGameConfig->AddOption("IPSShop.Enable");
    }

    void OnStartup() override
    {
        if (!CONF_GET_BOOL("IPSShop.Enable"))
            return;

        sDonateIPS->LoadShopStore();

        scheduler.Schedule(15s, [](TaskContext context)
        {
            sDonateIPS->SendDonate();
            context.Repeat();
        });
    }

    void OnUpdate(uint32 diff) override
    {
        if (!CONF_GET_BOOL("IPSShop.Enable"))
            return;

        scheduler.Update(diff);
    }
    
private:
    TaskScheduler scheduler;
};

// Group all custom scripts
void AddSC_DonateIPS()
{
    new DonateIPS_World();
}
