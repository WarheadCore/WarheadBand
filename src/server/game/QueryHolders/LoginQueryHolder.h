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

#ifndef WARHEAD_LOGIN_QUERY_HOLDER_H_
#define WARHEAD_LOGIN_QUERY_HOLDER_H_

#include "QueryHolder.h"
#include "ObjectGuid.h"

class WH_GAME_API LoginQueryHolder : public CharacterDatabaseQueryHolder
{
public:
    LoginQueryHolder(uint32 accountId, ObjectGuid guid) :
        _accountId(accountId), _guid(guid) { }

    bool Initialize();

    inline ObjectGuid GetGuid() const { return _guid; }
    inline uint32 GetAccountId() const { return _accountId; }

private:
    uint32 _accountId;
    ObjectGuid _guid;
};

#endif
