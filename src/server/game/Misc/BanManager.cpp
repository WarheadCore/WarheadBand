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

#include "BanManager.h"
#include "AccountMgr.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"

BanManager* BanManager::instance()
{
    static BanManager instance;
    return &instance;
}

/// Ban an account, duration will be parsed using Warhead::Time::TimeStringTo<Seconds> if it is positive, otherwise permban
BanReturn BanManager::BanAccount(std::string const& accountName, std::string_view duration, std::string const& reason, std::string const& author)
{
    if (accountName.empty() || duration.empty())
        return BAN_SYNTAX_ERROR;

    uint32 DurationSecs = Warhead::Time::TimeStringTo<Seconds>(duration);

    uint32 AccountID = AccountMgr::GetId(accountName);
    if (!AccountID)
        return BAN_NOTFOUND;

    ///- Disconnect all affected players (for IP it can be several)
    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

    // pussywizard: check existing ban to prevent overriding by a shorter one! >_>
    LoginDatabasePreparedStatement* stmtAccountBanned = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED);
    stmtAccountBanned->setUInt32(0, AccountID);

    PreparedQueryResult banresult = LoginDatabase.Query(stmtAccountBanned);
    if (banresult && ((*banresult)[0].GetUInt32() == (*banresult)[1].GetUInt32() || ((*banresult)[1].GetUInt32() > GameTime::GetGameTime() + DurationSecs && DurationSecs)))
        return BAN_LONGER_EXISTS;

    // make sure there is only one active ban
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
    stmt->setUInt32(0, AccountID);
    trans->Append(stmt);

    // No SQL injection with prepared statements
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_BANNED);
    stmt->setUInt32(0, AccountID);
    stmt->setUInt32(1, DurationSecs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    trans->Append(stmt);

    if (WorldSession* session = sWorld->FindSession(AccountID))
        if (session->GetPlayerName() != author)
            session->KickPlayer("Ban Account at condition 'FindSession(account)->GetPlayerName() != author'");

    if (WorldSession* session = sWorld->FindOfflineSession(AccountID))
        if (session->GetPlayerName() != author)
            session->KickPlayer("Ban Account at condition 'FindOfflineSession(account)->GetPlayerName() != author'");

    LoginDatabase.CommitTransaction(trans);

    if (CONF_GET_BOOL("ShowBanInWorld"))
    {
        if (DurationSecs)
            sWorld->SendWorldText(LANG_BAN_ACCOUNT_YOUBANNEDMESSAGE_WORLD,
                author.c_str(), accountName.c_str(), Warhead::Time::ToTimeString<Seconds>(DurationSecs).c_str(), reason.c_str());
        else
            sWorld->SendWorldText(LANG_BAN_ACCOUNT_YOUPERMBANNEDMESSAGE_WORLD, author.c_str(), accountName.c_str(), reason.c_str());
    }

    return BAN_SUCCESS;
}

/// Ban an account by player name, duration will be parsed using Warhead::Time::TimeStringTo<Seconds> if it is positive, otherwise permban
BanReturn BanManager::BanAccountByPlayerName(std::string const& characterName, std::string_view duration, std::string const& reason, std::string const& author)
{
    if (characterName.empty() || duration.empty())
        return BAN_SYNTAX_ERROR;

    uint32 DurationSecs = Warhead::Time::TimeStringTo<Seconds>(duration);

    uint32 AccountID = sObjectMgr->GetPlayerAccountIdByPlayerName(characterName);
    if (!AccountID)
        return BAN_NOTFOUND;

    ///- Disconnect all affected players (for IP it can be several)
    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

    // pussywizard: check existing ban to prevent overriding by a shorter one! >_>
    LoginDatabasePreparedStatement* stmtAccountBanned = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED);
    stmtAccountBanned->setUInt32(0, AccountID);

    PreparedQueryResult banresult = LoginDatabase.Query(stmtAccountBanned);
    if (banresult && ((*banresult)[0].GetUInt32() == (*banresult)[1].GetUInt32() || ((*banresult)[1].GetUInt32() > GameTime::GetGameTime() + DurationSecs && DurationSecs)))
        return BAN_LONGER_EXISTS;

    // make sure there is only one active ban
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
    stmt->setUInt32(0, AccountID);
    trans->Append(stmt);

    // No SQL injection with prepared statements
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_BANNED);
    stmt->setUInt32(0, AccountID);
    stmt->setUInt32(1, DurationSecs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    trans->Append(stmt);

    if (WorldSession* session = sWorld->FindSession(AccountID))
        if (session->GetPlayerName() != author)
            session->KickPlayer("Ban Account at condition 'FindSession(account)->GetPlayerName() != author'");

    if (WorldSession* session = sWorld->FindOfflineSession(AccountID))
        if (session->GetPlayerName() != author)
            session->KickPlayer("Ban Account at condition 'FindOfflineSession(account)->GetPlayerName() != author'");

    LoginDatabase.CommitTransaction(trans);

    if (CONF_GET_BOOL("ShowBanInWorld"))
    {
        std::string accountName;

        AccountMgr::GetName(AccountID, accountName);

        if (DurationSecs)
            sWorld->SendWorldText(LANG_BAN_ACCOUNT_YOUBANNEDMESSAGE_WORLD, author.c_str(), accountName.c_str(),
                Warhead::Time::ToTimeString<Seconds>(DurationSecs).c_str(), reason.c_str());
        else
            sWorld->SendWorldText(LANG_BAN_ACCOUNT_YOUPERMBANNEDMESSAGE_WORLD, author.c_str(), accountName.c_str(), reason.c_str());
    }

    return BAN_SUCCESS;
}

/// Ban an IP address, duration will be parsed using Warhead::Time::TimeStringTo<Seconds> if it is positive, otherwise permban
BanReturn BanManager::BanIP(std::string const& IP, std::string_view duration, std::string const& reason, std::string const& author)
{
    if (IP.empty() || duration.empty())
        return BAN_SYNTAX_ERROR;

    uint32 DurationSecs = Warhead::Time::TimeStringTo<Seconds>(duration);

    // No SQL injection with prepared statements
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_IP);
    stmt->setString(0, IP);

    PreparedQueryResult resultAccounts = LoginDatabase.Query(stmt);
    stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_IP_BANNED);
    stmt->setString(0, IP);
    stmt->setUInt32(1, DurationSecs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    LoginDatabase.Execute(stmt);

    if (CONF_GET_BOOL("ShowBanInWorld"))
    {
        if (!DurationSecs)
            sWorld->SendWorldText(LANG_BAN_IP_YOUPERMBANNEDMESSAGE_WORLD, author.c_str(), IP.c_str(), reason.c_str());
        else
            sWorld->SendWorldText(LANG_BAN_IP_YOUBANNEDMESSAGE_WORLD, author.c_str(), IP.c_str(), Warhead::Time::ToTimeString<Seconds>(DurationSecs).c_str(), reason.c_str());
    }

    if (!resultAccounts)
        return BAN_SUCCESS;

    ///- Disconnect all affected players (for IP it can be several)
    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

    do
    {
        Field* fields = resultAccounts->Fetch();
        uint32 AccountID = fields[0].GetUInt32();

        if (WorldSession* session = sWorld->FindSession(AccountID))
            if (session->GetPlayerName() != author)
                session->KickPlayer("Ban IP at condition 'FindSession(account)->GetPlayerName() != author'");

        if (WorldSession* session = sWorld->FindOfflineSession(AccountID))
            if (session->GetPlayerName() != author)
                session->KickPlayer("Ban IP at condition 'FindOfflineSession(account)->GetPlayerName() != author'");
    } while (resultAccounts->NextRow());

    LoginDatabase.CommitTransaction(trans);

    return BAN_SUCCESS;
}

/// Ban an character, duration will be parsed using Warhead::Time::TimeStringTo<Seconds> if it is positive, otherwise permban
BanReturn BanManager::BanCharacter(std::string const& characterName, std::string_view duration, std::string const& reason, std::string const& author)
{
    Player* target = ObjectAccessor::FindPlayerByName(characterName, false);
    uint32 DurationSecs = Warhead::Time::TimeStringTo<Seconds>(duration);
    ObjectGuid TargetGUID;

    /// Pick a player to ban if not online
    if (!target)
    {
        TargetGUID = sWorld->GetGlobalPlayerGUID(characterName);
        if (!TargetGUID)
            return BAN_NOTFOUND;
    }
    else
        TargetGUID = target->GetGUID();

    // make sure there is only one active ban
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, TargetGUID.GetCounter());
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_BAN);
    stmt->setUInt32(0, TargetGUID.GetCounter());
    stmt->setUInt32(1, DurationSecs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    CharacterDatabase.Execute(stmt);

    if (target)
        target->GetSession()->KickPlayer("Ban");

    if (CONF_GET_BOOL("ShowBanInWorld"))
    {
        if (DurationSecs)
            sWorld->SendWorldText(LANG_BAN_CHARACTER_YOUBANNEDMESSAGE_WORLD, author.c_str(), characterName.c_str(),
                Warhead::Time::ToTimeString<Seconds>(DurationSecs).c_str(), reason.c_str());
        else
            sWorld->SendWorldText(LANG_BAN_CHARACTER_YOUPERMBANNEDMESSAGE_WORLD, author.c_str(), characterName.c_str(), reason.c_str());
    }

    return BAN_SUCCESS;
}

/// Remove a ban from an account
bool BanManager::RemoveBanAccount(std::string const& accountName)
{
    uint32 AccountID = AccountMgr::GetId(accountName);
    if (!AccountID)
        return false;

    // NO SQL injection as account is uint32
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
    stmt->setUInt32(0, AccountID);
    LoginDatabase.Execute(stmt);

    return true;
}

/// Remove a ban from an player name
bool BanManager::RemoveBanAccountByPlayerName(std::string const& characterName)
{
    uint32 AccountID = sObjectMgr->GetPlayerAccountIdByPlayerName(characterName);
    if (!AccountID)
        return false;

    // NO SQL injection as account is uint32
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
    stmt->setUInt32(0, AccountID);
    LoginDatabase.Execute(stmt);

    return true;
}

/// Remove a ban from an account
bool BanManager::RemoveBanIP(std::string const& IP)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_IP_NOT_BANNED);
    stmt->setString(0, IP);
    LoginDatabase.Execute(stmt);

    return true;
}

/// Remove a ban from a character
bool BanManager::RemoveBanCharacter(std::string const& characterName)
{
    Player* pBanned = ObjectAccessor::FindPlayerByName(characterName, false);
    ObjectGuid guid;

    /// Pick a player to ban if not online
    if (!pBanned)
        guid = sWorld->GetGlobalPlayerGUID(characterName);
    else
        guid = pBanned->GetGUID();

    if (!guid)
        return false;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, guid.GetCounter());
    CharacterDatabase.Execute(stmt);
    return true;
}
