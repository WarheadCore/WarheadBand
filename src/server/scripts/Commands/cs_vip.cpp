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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "CharacterCache.h"
#include "Chat.h"
#include "Creature.h"
#include "GameLocale.h"
#include "GameTime.h"
#include "ScriptObject.h"
#include "Timer.h"
#include "Vip.h"

using namespace Warhead::ChatCommands;

class vip_commandscript : public CommandScript
{
public:
    vip_commandscript() : CommandScript("vip_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable vipListCommandTable =
        {
            { "rates",      HandleVipListRatesCommand,   SEC_ADMINISTRATOR,  Console::Yes }
        };

        static ChatCommandTable vipVendorCommandTable =
        {
            { "info",      HandleVipVendorInfoCommand,   SEC_ADMINISTRATOR,  Console::No },
            { "add",       HandleVipVendorAddCommand,    SEC_ADMINISTRATOR,  Console::No },
            { "delete",    HandleVipVendorDeleteCommand, SEC_ADMINISTRATOR,  Console::No }
        };

        static ChatCommandTable vipCommandTable =
        {
            { "add",        HandleVipAddCommand,        SEC_ADMINISTRATOR,  Console::Yes },
            { "delete",     HandleVipDeleteCommand,     SEC_ADMINISTRATOR,  Console::Yes },
            { "unbind",     HandleVipUnbindCommand,     SEC_PLAYER,         Console::No },
            { "info",       HandleVipInfoCommand,       SEC_PLAYER,         Console::Yes },
            { "list",       vipListCommandTable },
            { "vendor",     vipVendorCommandTable },
        };

        static ChatCommandTable commandTable =
        {
            { "vip", vipCommandTable }
        };

        return commandTable;
    }

    static bool HandleVipAddCommand(ChatHandler* handler, Optional<PlayerIdentifier> target, uint32 duration, uint8 level)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

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

        if (sVip->Add(data->AccountId, GameTime::GetGameTime() + Days(duration), level, true))
        {
            handler->PSendSysMessage("> Обновление статуста премиум аккаунта для игрока {}.", target->GetName());
            handler->PSendSysMessage("> Уровень {}. Оставшееся время {}.", level, Warhead::Time::ToTimeString(Days(duration)));
            return true;
        }

        return false;
    }

    static bool HandleVipUnbindCommand(ChatHandler* handler)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        sVip->UnBindInstances(handler->GetPlayer());
        return true;
    }

    static bool HandleVipDeleteCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

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

    static bool HandleVipInfoCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

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

        sVip->SendVipInfo(handler, target->GetGUID());
        return true;
    }

    static bool HandleVipListRatesCommand(ChatHandler* handler)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        sVip->SendVipListRates(handler);
        return true;
    }

    static bool HandleVipVendorInfoCommand(ChatHandler* handler)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        Creature* vendor = handler->getSelectedCreature();
        if (!vendor)
        {
            handler->SendSysMessage("Нужно выбрать существо");
            return true;
        }

        auto creatureEntry = vendor->GetEntry();
        auto creatureName = sGameLocale->GetCreatureNamelocale(vendor->GetEntry(), handler->GetSessionDbLocaleIndex());

        if (!vendor->IsVendor())
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' не является вендором", creatureEntry, creatureName);
            return true;
        }

        if (!sVip->IsVipVendor(creatureEntry))
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' не является вип вендором", creatureEntry, creatureName);
            return true;
        }

        handler->PSendSysMessage("# Существо `{}` - '{}' доступно для вип '{}' уровня и выше", creatureEntry, creatureName, sVip->GetVendorVipLevel(creatureEntry));
        return true;
    }

    static bool HandleVipVendorAddCommand(ChatHandler* handler, uint8 vendorVipLevel)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        Creature* vendor = handler->getSelectedCreature();
        if (!vendor)
        {
            handler->SendSysMessage("Нужно выбрать существо");
            return true;
        }

        auto creatureEntry = vendor->GetEntry();
        auto creatureName = sGameLocale->GetCreatureNamelocale(vendor->GetEntry(), handler->GetSessionDbLocaleIndex());

        if (!vendor->IsVendor())
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' не является вендором", creatureEntry, creatureName);
            return true;
        }

        if (sVip->IsVipVendor(creatureEntry))
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' уже было вип вендором", creatureEntry, creatureName);
            sVip->DeleteVendorVipLevel(creatureEntry);
        }

        sVip->AddVendorVipLevel(creatureEntry, vendorVipLevel);
        handler->PSendSysMessage("# Существо `{}` - '{}' теперь доступно для вип '{}' уровня и выше", creatureEntry, creatureName, sVip->GetVendorVipLevel(creatureEntry));
        return true;
    }

    static bool HandleVipVendorDeleteCommand(ChatHandler* handler)
    {
        if (!sVip->IsEnable())
        {
            handler->PSendSysMessage("> Модуль отключен");
            return true;
        }

        Creature* vendor = handler->getSelectedCreature();
        if (!vendor)
        {
            handler->SendSysMessage("Нужно выбрать существо");
            return true;
        }

        auto creatureEntry = vendor->GetEntry();
        auto creatureName = sGameLocale->GetCreatureNamelocale(vendor->GetEntry(), handler->GetSessionDbLocaleIndex());

        if (!vendor->IsVendor())
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' не является вендором", creatureEntry, creatureName);
            return true;
        }

        if (!sVip->IsVipVendor(creatureEntry))
        {
            handler->PSendSysMessage("# Существо `{}` - '{}' не является вип вендором", creatureEntry, creatureName);
            return true;
        }

        sVip->DeleteVendorVipLevel(creatureEntry);
        handler->PSendSysMessage("# Существо `{}` - '{}' доступно для всех", creatureEntry, creatureName);
        return true;
    }
};

void AddSC_vip_commandscript()
{
    new vip_commandscript();
}
