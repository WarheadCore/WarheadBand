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

#include "MuteMgr.h"
#include "AsyncCallbackMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "ChatTextBuilder.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Language.h"
#include "Player.h"
#include "Timer.h"
#include "World.h"

MuteMgr* MuteMgr::instance()
{
    static MuteMgr instance;
    return &instance;
}

void MuteMgr::MutePlayer(std::string const& targetName, Seconds muteTime, std::string const& muteBy, std::string const& muteReason)
{
    uint32 accountId = sCharacterCache->GetCharacterAccountIdByName(targetName);
    auto targetSession = sWorld->FindSession(accountId);

    // INSERT INTO `account_muted` (`accountid`, `mutedate`, `mutetime`, `mutedby`, `mutereason`, `active`) VALUES (?, ?, ?, ?, ?, 1)
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_MUTE);
    stmt->SetData(0, accountId);

    /*
     * Mute will be in effect right away.
     * If Mute.AddAfterLogin mute will be in effect starting from the next login.
     */
    uint64 muteDate = CONF_GET_BOOL("Mute.AddAfterLogin.Enable") && !targetSession ? 0 : GameTime::GetGameTime().count();

    if (targetSession)
        AddMuteTime(accountId, muteTime);

    stmt->SetData(1, muteDate);
    stmt->SetData(2, muteTime);
    stmt->SetData(3, muteBy);
    stmt->SetData(4, muteReason);
    LoginDatabase.Execute(stmt);

    auto GetPlayerLink = [targetName]()
    {
        return "|cffffffff|Hplayer:" + targetName + "|h[" + targetName + "]|h|r";
    };

    if (CONF_GET_BOOL("ShowMuteInWorld"))
        Warhead::Text::SendWorldText(LANG_COMMAND_MUTEMESSAGE_WORLD, muteBy, GetPlayerLink(), Warhead::Time::ToTimeString(muteTime), muteReason);

    if (targetSession)
        ChatHandler(targetSession).PSendSysMessage(LANG_YOUR_CHAT_DISABLED, Warhead::Time::ToTimeString(muteTime), muteBy, muteReason);
}

void MuteMgr::UnMutePlayer(std::string const& targetName)
{
    uint32 accountId = sCharacterCache->GetCharacterAccountIdByName(targetName);

    DeleteMuteTime(accountId);

    if (auto targetSession = sWorld->FindSession(accountId))
        ChatHandler(targetSession).SendSysMessage(LANG_YOUR_CHAT_ENABLED);
}

void MuteMgr::SetMuteTime(uint32 accountID, Seconds muteDate)
{
    std::lock_guard<std::mutex> guard(_mutex);

    // Check empty
    auto itr = _listSessions.find(accountID);
    if (itr != _listSessions.end())
        _listSessions.erase(accountID);

    LOG_DEBUG("entities.player.session", "> Set mute {} for account id {}", Warhead::Time::TimeToHumanReadable(Seconds(muteDate)), accountID);

    _listSessions.emplace(accountID, muteDate);
}

void MuteMgr::AddMuteTime(uint32 accountID, Seconds muteTime)
{
    SetMuteTime(accountID, GameTime::GetGameTime() + muteTime);
}

Seconds MuteMgr::GetMuteDate(uint32 accountID)
{
    // Check exist
    auto itr = _listSessions.find(accountID);
    if (itr == _listSessions.end())
        return 0s;

    return itr->second;
}

void MuteMgr::DeleteMuteTime(uint32 accountID, bool delFromDB /*= true*/)
{
    if (delFromDB)
    {
        // UPDATE `account_muted` SET `active` = 0 WHERE `active` = 1 AND `accountid` = ?
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_MUTE);
        stmt->SetData(0, accountID);
        LoginDatabase.Execute(stmt);
    }

    // Check exist account muted
    auto itr = _listSessions.find(accountID);
    if (itr == _listSessions.end())
        return;

    _listSessions.erase(accountID);
}

void MuteMgr::CheckMuteExpired(uint32 accountID)
{
    auto muteTime = GetMuteDate(accountID);
    auto timeNow = GameTime::GetGameTime();

    if (muteTime == 0s || muteTime > timeNow)
        return;

    DeleteMuteTime(accountID);
}

std::string const MuteMgr::GetMuteTimeString(uint32 accountID)
{
    return Warhead::Time::ToTimeString(GetMuteDate(accountID) - GameTime::GetGameTime());
}

bool MuteMgr::CanSpeak(uint32 accountID)
{
    return GetMuteDate(accountID) <= GameTime::GetGameTime();
}

void MuteMgr::LoginAccount(uint32 accountID)
{
    // Set inactive if expired
    // UPDATE `account_muted` SET `active` = 0 WHERE `active` = 1 AND `mutetime` > 0 AND mutedate > 0 AND `accountid` = ? AND UNIX_TIMESTAMP() >= `mutedate` + `mutetime`
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_MUTE_EXPIRED);
    stmt->SetData(0, accountID);
    LoginDatabase.Execute(stmt);

    sAsyncCallbackMgr->AddAsyncCallback([this, accountID]()
    {
        // Get info about mute time after update active
        // SELECT `mutedate`, `mutetime`, `mutereason`, `mutedby` FROM `account_muted` WHERE `accountid` = ? AND `active` = 1 ORDER BY `mutedate` + `mutetime` DESC LIMIT 1
        auto stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_MUTE);
        stmt->SetData(0, accountID);

        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (!result)
        {
            // If no info - no mute time :)
            return;
        }

        Field* fields = result->Fetch();
        Seconds mutedate = fields[0].Get<Seconds>(false);
        Seconds mutetime = fields[1].Get<Seconds>();

        if (mutedate == 0s)
        {
            // Set now time (add mute after login)
            mutedate = GameTime::GetGameTime();
        }

        UpdateMuteAccount(accountID, mutedate);
        SetMuteTime(accountID, mutedate + mutetime);
    });
}

void MuteMgr::UpdateMuteAccount(uint32 accountID, Seconds muteDate)
{
    // UPDATE `account_muted` SET `mutedate` = ? WHERE `accountid` = ? AND `active` = 1 LIMIT 1
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_MUTE_DATE);
    stmt->SetData(0, muteDate);
    stmt->SetData(1, accountID);
    LoginDatabase.Execute(stmt);
}

Optional<MuteInfo> MuteMgr::GetMuteInfo(uint32 accountID)
{
    // SELECT `mutedate`, `mutetime`, `mutereason`, `mutedby` FROM `account_muted` WHERE `accountid` = ? AND `active` = 1 ORDER BY `mutedate` + `mutetime` DESC LIMIT 1
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_MUTE);
    stmt->SetData(0, accountID);

    PreparedQueryResult result = LoginDatabase.Query(stmt);
    if (!result)
        return std::nullopt;

    Field* fields = result->Fetch();
    return std::make_tuple(fields[0].Get<Seconds>(false), fields[1].Get<Seconds>(), fields[2].Get<std::string>(), fields[3].Get<std::string>());
}

void MuteMgr::CheckSpeakTime(uint32 accountID, Seconds muteDate)
{
    if (GetMuteDate(accountID) < muteDate)
        SetMuteTime(accountID, muteDate);
}
