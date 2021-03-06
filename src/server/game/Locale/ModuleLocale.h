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

#ifndef _MODULES_LOCALE_H_
#define _MODULES_LOCALE_H_

#include "Common.h"
#include "Optional.h"
#include "StringFormat.h"
#include <functional>
#include <unordered_map>
#include <vector>

class Player;
class WorldPacket;

struct ModuleString
{
    ModuleString()
    {
        Content.resize(DEFAULT_LOCALE + 1);
    }

    std::vector<std::string> Content;

    Optional<std::string> GetText(uint8 locale = 0) const
    {
        if (Content.size() > size_t(locale) && !Content[locale].empty())
            return Content[locale];

        if (!Content[0].empty())
            return Content[0];

        return std::nullopt;
    }
};

class WH_GAME_API ModuleLocale
{
private:
    ModuleLocale() = default;
    ~ModuleLocale() = default;

public:
    static ModuleLocale* instance();

    void Init();
    void LoadModuleString();
    void CheckStrings(std::string const& moduleName, uint32 maxString);

    Optional<std::string> GetModuleString(std::string const& moduleName, uint32 id, uint8 _locale) const;
    uint32 GetStringsCount(std::string const& moduleName);

    // Get localized message
    template<typename... Args>
    inline std::string GetLocaleMessage(std::string const& moduleName, uint32 id, uint8 localeIndex, Args&&... args)
    {
        return Warhead::StringFormat(*GetModuleString(moduleName, id, localeIndex), std::forward<Args>(args)...);
    }

    void SendPlayerMessage(Player* player, std::function<std::string_view()> const& msg);
    void SendGlobalMessage(bool gmOnly, std::function<std::string_view()> const& msg);

    ModuleLocale(ModuleLocale const&) = delete;
    ModuleLocale(ModuleLocale&&) = delete;
    ModuleLocale& operator= (ModuleLocale const&) = delete;
    ModuleLocale& operator= (ModuleLocale&&) = delete;

private:
    using ModuleStringContainer = std::unordered_map<uint32, ModuleString>;
    using AllModulesStringContainer = std::unordered_map<std::string, ModuleStringContainer>;

    AllModulesStringContainer _modulesStringStore;

    void AddModuleString(std::string const& moduleName, ModuleStringContainer& data);

    // Send packets
    void SendPlayerPacket(Player* player, WorldPacket* data);
};

#define sModuleLocale ModuleLocale::instance()

#endif // _MODULES_LOCALE_H_
