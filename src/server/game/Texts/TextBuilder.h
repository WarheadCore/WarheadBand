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

#ifndef _TEXT_BUILDER_H_
#define _TEXT_BUILDER_H_

#include "GameLocale.h"
#include <functional>

class Battleground;

namespace Warhead::Text
{
    using WarheadFmtText = std::function<std::string_view(uint8)>;

    // Get localized message
    template<typename... Args>
    inline std::string GetLocaleMessage(uint8 localeIndex, uint32 id, Args&&... args)
    {
        return Warhead::StringFormat(sGameLocale->GetWarheadString(id, LocaleConstant(localeIndex)), std::forward<Args>(args)...);
    }

    // Send a System Message to all players (except self if mentioned)
    void SendWorldText(WarheadFmtText const& msg);

    // Send a System Message to all GMs (except self if mentioned)
    void SendGMText(WarheadFmtText const& msg);

    // Send a Battleground warning message to all players
    void SendBattlegroundWarningToAll(Battleground* bg, WarheadFmtText const& msg);

    // Send a Battleground with type message to all players
    void SendBattlegroundMessageToAll(Battleground* bg, ChatMsg type, WarheadFmtText const& msg);
}

#endif // _TEXT_BUILDER_H_
