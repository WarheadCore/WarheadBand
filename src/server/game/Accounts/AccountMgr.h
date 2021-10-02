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

#ifndef _ACCMGR_H
#define _ACCMGR_H

#include "Define.h"
#include <string>

enum AccountOpResult
{
    AOR_OK,
    AOR_NAME_TOO_LONG,
    AOR_PASS_TOO_LONG,
    AOR_NAME_ALREADY_EXIST,
    AOR_NAME_NOT_EXIST,
    AOR_DB_INTERNAL_ERROR
};

#define MAX_ACCOUNT_STR 20
#define MAX_PASS_STR 16

namespace AccountMgr
{
    WH_GAME_API AccountOpResult CreateAccount(std::string username, std::string password);
    WH_GAME_API AccountOpResult DeleteAccount(uint32 accountId);
    WH_GAME_API AccountOpResult ChangeUsername(uint32 accountId, std::string newUsername, std::string newPassword);
    WH_GAME_API AccountOpResult ChangePassword(uint32 accountId, std::string newPassword);
    WH_GAME_API bool CheckPassword(uint32 accountId, std::string password);

    WH_GAME_API uint32 GetId(std::string const& username);
    WH_GAME_API uint32 GetSecurity(uint32 accountId);
    WH_GAME_API uint32 GetSecurity(uint32 accountId, int32 realmId);
    WH_GAME_API bool GetName(uint32 accountId, std::string& name);
    WH_GAME_API uint32 GetCharactersCount(uint32 accountId);

    WH_GAME_API bool IsPlayerAccount(uint32 gmlevel);
    WH_GAME_API bool IsGMAccount(uint32 gmlevel);
    WH_GAME_API bool IsAdminAccount(uint32 gmlevel);
    WH_GAME_API bool IsConsoleAccount(uint32 gmlevel);
};

#endif
