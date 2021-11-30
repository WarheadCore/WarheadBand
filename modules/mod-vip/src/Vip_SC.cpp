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
#include "AccountMgr.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Chat.h"
#include "Player.h"
#include "Timer.h"
#include "ScriptedGossip.h"
#include "StringConvert.h"
#include "WorldSession.h"
#include "Tokenize.h"
#include "CharacterCache.h"
#include "Spell.h"

using namespace Warhead::ChatCommands;

class Vip_CS : public CommandScript
{
public:
    Vip_CS() : CommandScript("Vip_CS") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable arenaCommandTable =
        {
            { "add",        HandleVipAddCommand,        SEC_ADMINISTRATOR,  Console::Yes },
            { "delete",     HandleVipDeleteCommand,     SEC_ADMINISTRATOR,  Console::Yes },
            { "unbind",     HandleVipUnbindCommand,     SEC_PLAYER,         Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "vip", arenaCommandTable }
        };

        return commandTable;
    }

    static bool HandleVipAddCommand(ChatHandler* handler, Optional<PlayerIdentifier> target, uint32 duration, uint8 level)
    {
        if (!target)
        {
            handler->PSendSysMessage("> Не указано имя игрока, имя будет выбрано из цели");
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target)
        {
            handler->PSendSysMessage("> Не выбрана цель");
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto data = sCharacterCache->GetCharacterCacheByGuid(target->GetGUID());

        if (!data)
        {
            handler->PSendSysMessage("> Игрок {} не найден в базе", target->GetName());
            return false;
        }

        if (sVip->IsVip(data->AccountId))
            handler->PSendSysMessage("> Игрок {} уже был випом.", target->GetName());

        if (sVip->Add(data->AccountId, GameTime::GetGameTime().count() + uint64(duration * DAY), level, true))
        {
            handler->PSendSysMessage("> Обновление статуста премиум аккаунта для игрока {}.", target->GetName());
            handler->PSendSysMessage("> Уровень {}. Оставшееся время {}.", level, Warhead::Time::ToTimeString<Seconds>(duration * DAY));
            return true;
        }

        return false;
    }

    static bool HandleVipUnbindCommand(ChatHandler* handler)
    {
        sVip->UnBindInstances(handler->GetPlayer());
        return true;
    }

    static bool HandleVipDeleteCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!target)
        {
            handler->PSendSysMessage("> Не указано имя игрока, имя будет выбрано из цели");
            target = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!target)
        {
            handler->PSendSysMessage("> Не выбрана цель");
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto data = sCharacterCache->GetCharacterCacheByGuid(target->GetGUID());

        if (!data)
        {
            handler->PSendSysMessage("> Игрок {} не найден в базе", target->GetName());
            return false;
        }

        if (!sVip->IsVip(data->AccountId))
        {
            handler->PSendSysMessage("> Игрок {} не имеет премиум статуса", target->GetName());
            return true;
        }

        sVip->UnSet(data->AccountId);

        handler->PSendSysMessage("> Игрок {} больше не имеет премиум статуса", target->GetName());

        return true;
    }
};

class Vip_Player : public PlayerScript
{
public:
    Vip_Player() : PlayerScript("Vip_Player") { }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        if (!sVip->IsVip(player))
            return;

        amount *= sVip->GetRate<VipRate::XP>(player);
    }

    void OnGiveHonorPoints(Player* player, float& points, Unit* /*victim*/) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        if (!sVip->IsVip(player))
            return;

        points *= sVip->GetRate<VipRate::Honor>(player);
    }

    void OnReputationChange(Player* player, uint32 /* factionID */, int32& standing, bool /* incremental */) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        if (!sVip->IsVip(player))
            return;

        standing *= sVip->GetRate<VipRate::Reputation>(player);
    }

    void OnLogin(Player* player) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        if (!sVip->IsVip(player))
            return;

        sVip->OnLoginPlayer(player);
    }

    void OnLogout(Player* player) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        if (!sVip->IsVip(player))
            return;

        sVip->OnLogoutPlayer(player);
    }

    void OnAfterResurrect(Player* player, float /* restore_percent */, bool applySickness) override
    {
        if (!sVip->IsVip(player) || !applySickness)
            return;

        if (player->HasSpell(SPELL_RESSURECTION_SICKESS))
            player->removeSpell(SPELL_RESSURECTION_SICKESS, SPEC_MASK_ALL, false);
    }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player)
            return;

        if (spell->GetSpellInfo()->Id == SPELL_HEARTHSTONE)
            sVip->RemoveColldown(player, SPELL_HEARTHSTONE);
    }

private:
    enum VipSpells
    {
        SPELL_RESSURECTION_SICKESS = 15007,
        SPELL_HEARTHSTONE = 8690
    };
};

class Vip_World : public WorldScript
{
public:
    Vip_World() : WorldScript("Vip_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sGameConfig->AddOption({ "VIP.Enable",
            "VIP.Update.Delay",
            "VIP.Mount.MinLevel",
            "VIP.Mount.SpellID",
            "VIP.Spells.List",
            "VIP.Unbind.Duration" });

        sVip->InitSystem(true);
    }

    void OnStartup() override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        sVip->InitSystem(false);
    }

    void OnUpdate(uint32 diff) override
    {
        if (!CONF_GET_BOOL("VIP.Enable"))
            return;

        sVip->Update(diff);
    }
};

// Group all custom scripts
void AddSC_Vip()
{
    new Vip_World();
    new Vip_Player();
    new Vip_CS();
}
