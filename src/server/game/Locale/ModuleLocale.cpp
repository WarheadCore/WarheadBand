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

#include "ModuleLocale.h"
#include "AccountMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "LocaleCommon.h"
#include "Log.h"
#include "Optional.h"
#include "Player.h"
#include "Tokenize.h"
#include "World.h"
#include "StopWatch.h"

ModuleLocale* ModuleLocale::instance()
{
    static ModuleLocale instance;
    return &instance;
}

void ModuleLocale::Init()
{
    LOG_INFO("server.loading", " ");
    LOG_INFO("server.loading", ">> Loading modules strings");
    LoadModuleString();
}

Optional<std::string> ModuleLocale::GetModuleString(std::string const& entry, uint8 _locale) const
{
    if (entry.empty())
    {
        LOG_ERROR("locale.module", "> ModulesLocales: Entry is empty!");
        return std::nullopt;
    }

    auto const& itr = _modulesStringStore.find(entry);
    if (itr == _modulesStringStore.end())
    {
        LOG_FATAL("locale.module", "> ModulesLocales: Not found strings for entry ({})", entry);
        ABORT();
        return {};
    }

    return itr->second.GetText(_locale);
}

void ModuleLocale::LoadModuleString()
{
    StopWatch sw;
    _modulesStringStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT `Entry`, `Locale`, `Text` FROM `string_module`");
    if (!result)
    {
        LOG_INFO("server.loading", "> DB table `string_module` is empty");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        auto fields = result->Fetch();

        auto& data = _modulesStringStore[fields[0].Get<std::string>()];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string>(), GetLocaleByName(fields[1].Get<std::string>()), data.Content);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} module strings in {}", _modulesStringStore.size(), sw);
    LOG_INFO("server.loading", " ");
}

void ModuleLocale::SendPlayerMessageFmt(Player* player, std::function<std::string(uint8)> const& msg)
{
    if (!msg)
        return;

    std::string message{ msg(player->GetSession()->GetSessionDbLocaleIndex()) };

    for (std::string_view line : Warhead::Tokenize(message, '\n', false))
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        SendPlayerPacket(player, &data);
    }
}

void ModuleLocale::SendGlobalMessageFmt(bool gmOnly, std::function<std::string(uint8)> const& msg)
{
    if (!msg)
        return;

    std::unordered_map<uint8, std::vector<WorldPacket>> packetCache;
    packetCache.rehash(TOTAL_LOCALES - 1);

    for (uint8 i = LOCALE_enUS; i < TOTAL_LOCALES; ++i)
    {
        std::string message{ msg(i) };

        for (std::string_view line : Warhead::Tokenize(message, '\n', true))
        {
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
            packetCache[i].emplace_back(std::move(data));
        }
    }

    for (auto const& [accountID, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            return;

        if (AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()) && gmOnly)
            continue;

        auto& packetList = packetCache[static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex())];
        ASSERT(!packetList.empty());

        for (auto& packet : packetList)
            SendPlayerPacket(player, &packet);
    }
}

void ModuleLocale::SendPlayerPacket(Player* player, WorldPacket* data)
{
    player->SendDirectMessage(data);
}
