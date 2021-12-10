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

#ifndef _BAN_MANAGER_H
#define _BAN_MANAGER_H

#include "Common.h"

/// Ban function return codes
enum BanReturn
{
    BAN_SUCCESS,
    BAN_SYNTAX_ERROR,
    BAN_NOTFOUND,
    BAN_LONGER_EXISTS
};

class WH_GAME_API BanMgr
{
public:
    static BanMgr* instance();

    BanReturn BanAccount(std::string const& accountName, std::string_view duration, std::string const& reason, std::string const& author);
    BanReturn BanAccountByPlayerName(std::string const& characterName, std::string_view duration, std::string const& reason, std::string const& author);
    BanReturn BanIP(std::string const& IP, std::string_view duration, std::string const& reason, std::string const& author);
    BanReturn BanCharacter(std::string const& characterName, std::string_view duration, std::string const& reason, std::string const& author);

    bool RemoveBanAccount(std::string const& accountName);
    bool RemoveBanAccountByPlayerName(std::string const& characterName);
    bool RemoveBanIP(std::string const& IP);
    bool RemoveBanCharacter(std::string const& characterName);
};

#define sBan BanMgr::instance()

#endif // _BAN_MANAGER_H
