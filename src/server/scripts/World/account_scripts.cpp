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

#include "DatabaseEnv.h"
#include "MuteMgr.h"
#include "ScriptObject.h"
#include "World.h"
#include "WorldSession.h"

class LoginMuteTime : public AccountScript
{
public:
    LoginMuteTime() : AccountScript("LoginMuteTime") { }

    void OnAccountLogin(uint32 accountID) override
    {
        sMute->LoginAccount(accountID);
    }

    void OnAccountLogout(uint32 accountID) override
    {
        sMute->DeleteMuteTime(accountID, false);
    }
};

void AddSC_account_script()
{
    new LoginMuteTime();
}