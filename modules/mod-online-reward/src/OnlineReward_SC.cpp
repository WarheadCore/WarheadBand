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

#include "Log.h"
#include "ModulesConfig.h"
#include "OnlineReward.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Chat.h"

using namespace Warhead::ChatCommands;

class OnlineReward_CS : public CommandScript
{
public:
    OnlineReward_CS() : CommandScript("OnlineReward_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable vipCommandTable =
        {
            { "add",        HandleOnlineRewardAddCommand,       SEC_ADMINISTRATOR,  Console::Yes },
            { "delete",     HandleOnlineRewardDeleteCommand,    SEC_ADMINISTRATOR,  Console::Yes },
            { "list",       HandleOnlineRewardListCommand,      SEC_ADMINISTRATOR,  Console::Yes },
            { "reload",     HandleOnlineRewardReloadCommand,    SEC_ADMINISTRATOR,  Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "or", vipCommandTable }
        };

        return commandTable;
    }

    static bool HandleOnlineRewardAddCommand(ChatHandler* handler, bool isPerOnline, uint32 secs, Tail items, Tail reputations)
    {
        if (!sOLMgr->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        if (!secs)
        {
            handler->PSendSysMessage("> Нужно указать количество секунд");
            return true;
        }

        Seconds seconds{ secs };
        auto timeString = Warhead::Time::ToTimeString(seconds);
        timeString.append(Warhead::StringFormat(" ({})", seconds.count()));

        if (sOLMgr->IsExistReward(isPerOnline, seconds))
        {
            handler->PSendSysMessage("> Награда для {} уже существует. IsPerOnline? {}", timeString, isPerOnline);
            return true;
        }

        if (sOLMgr->AddReward(isPerOnline, seconds, items, reputations, handler))
            handler->PSendSysMessage("> Награда добавлена");

        return true;
    }

    static bool HandleOnlineRewardDeleteCommand(ChatHandler* handler, bool isPerOnline, uint32 secs)
    {
        handler->PSendSysMessage("> Награда {}была удалена", sOLMgr->DeleteReward(isPerOnline, Seconds(secs)) ? "" : "не ");
        return true;
    }

    static bool HandleOnlineRewardListCommand(ChatHandler* handler)
    {
        handler->PSendSysMessage("> Список наград за онлайн:");

        std::size_t count{ 0 };

        for (auto const& onlineRewards : *sOLMgr->GetOnlineRewards())
        {
            handler->PSendSysMessage("{}. {}. IsPerOnline? {}", ++count, Warhead::Time::ToTimeString(onlineRewards.Seconds), onlineRewards.IsPerOnline);

            if (!onlineRewards.Items.empty())
            {
                handler->SendSysMessage("-- Предметы:");

                for (auto const& [itemID, itemCount] : onlineRewards.Items)
                    handler->PSendSysMessage("> {}/{}", itemID, itemCount);
            }

            if (!onlineRewards.Reputations.empty())
            {
                handler->SendSysMessage("-- Репутация:");

                for (auto const& [faction, reputation] : onlineRewards.Reputations)
                    handler->PSendSysMessage("> {}/{}", faction, reputation);
            }

            handler->SendSysMessage("--");
        }

        return true;
    }

    static bool HandleOnlineRewardReloadCommand(ChatHandler* handler)
    {
        sOLMgr->LoadDBData();
        handler->PSendSysMessage("> награды перезагружены");
        return true;
    }
};

class OnlineReward_Player : public PlayerScript
{
public:
    OnlineReward_Player() : PlayerScript("OnlineReward_Player") { }

    void OnLogin(Player* player) override
    {
        sOLMgr->AddRewardHistory(player->GetGUID().GetCounter());
    }

    void OnLogout(Player* player) override
    {
        sOLMgr->DeleteRewardHistory(player->GetGUID().GetCounter());
    }
};

class OnlineReward_World : public WorldScript
{
public:
    OnlineReward_World() : WorldScript("OnlineReward_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sOLMgr->LoadConfig();
    }

    void OnStartup() override
    {
        sOLMgr->InitSystem();
    }

    void OnUpdate(uint32 diff) override
    {
        sOLMgr->Update(Milliseconds(diff));
    }
};

// Group all custom scripts
void AddSC_OnlineReward()
{
    new OnlineReward_CS();
    new OnlineReward_Player();
    new OnlineReward_World();
}
