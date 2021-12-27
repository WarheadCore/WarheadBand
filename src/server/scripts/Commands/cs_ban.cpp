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

/* ScriptData
Name: ban_commandscript
%Complete: 100
Comment: All ban related commands
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "BanMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"

/// Ban function modes
enum BanMode
{
    BAN_ACCOUNT,
    BAN_CHARACTER,
    BAN_IP
};

#if WARHEAD_COMPILER == WARHEAD_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Warhead::ChatCommands;

class ban_commandscript : public CommandScript
{
public:
    ban_commandscript() : CommandScript("ban_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable unbanCommandTable =
        {
            { "account",        SEC_ADMINISTRATOR,  true,  &HandleUnBanAccountCommand,          "" },
            { "character",      SEC_ADMINISTRATOR,  true,  &HandleUnBanCharacterCommand,        "" },
            { "playeraccount",  SEC_ADMINISTRATOR,  true,  &HandleUnBanAccountByCharCommand,    "" },
            { "ip",             SEC_ADMINISTRATOR,  true,  &HandleUnBanIPCommand,               "" }
        };

        static ChatCommandTable banlistCommandTable =
        {
            { "account",        SEC_GAMEMASTER,  true,  &HandleBanListAccountCommand,        "" },
            { "character",      SEC_GAMEMASTER,  true,  &HandleBanListCharacterCommand,      "" },
            { "ip",             SEC_GAMEMASTER,  true,  &HandleBanListIPCommand,             "" }
        };

        static ChatCommandTable baninfoCommandTable =
        {
            { "account",        SEC_GAMEMASTER,  true,  &HandleBanInfoAccountCommand,        "" },
            { "character",      SEC_GAMEMASTER,  true,  &HandleBanInfoCharacterCommand,      "" },
            { "ip",             SEC_GAMEMASTER,  true,  &HandleBanInfoIPCommand,             "" }
        };

        static ChatCommandTable banCommandTable =
        {
            { "account",        SEC_GAMEMASTER,  true,  &HandleBanAccountCommand,            "" },
            { "character",      SEC_GAMEMASTER,  true,  &HandleBanCharacterCommand,          "" },
            { "playeraccount",  SEC_GAMEMASTER,  true,  &HandleBanAccountByCharCommand,      "" },
            { "ip",             SEC_GAMEMASTER,  true,  &HandleBanIPCommand,                 "" }
        };

        static ChatCommandTable commandTable =
        {
            { "ban",            SEC_GAMEMASTER,     true,  nullptr, "", banCommandTable },
            { "baninfo",        SEC_GAMEMASTER,     true,  nullptr, "", baninfoCommandTable },
            { "banlist",        SEC_GAMEMASTER,     true,  nullptr, "", banlistCommandTable },
            { "unban",          SEC_ADMINISTRATOR,  true,  nullptr, "", unbanCommandTable }
        };

        return commandTable;
    }

    static bool HandleBanAccountCommand(ChatHandler* handler, char const* args)
    {
        return HandleBanHelper(BAN_ACCOUNT, args, handler);
    }

    static bool HandleBanCharacterCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* nameStr = strtok((char*)args, " ");
        if (!nameStr)
            return false;

        std::string name = nameStr;

        char* durationStr = strtok(nullptr, " ");
        if (!durationStr || !atoi(durationStr))
            return false;

        char* reasonStr = strtok(nullptr, "");
        if (!reasonStr)
            return false;

        if (!normalizePlayerName(name))
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string author = handler->GetSession() ? handler->GetSession()->GetPlayerName() : "Server";
        uint32 duration = Warhead::Time::TimeStringTo<Seconds>(durationStr);

        switch (sBan->BanCharacter(name, durationStr, reasonStr, author))
        {
            case BAN_SUCCESS:
            {
                if (CONF_GET_BOOL("ShowBanInWorld"))
                    break;

                if (duration)
                    handler->PSendSysMessage(LANG_BAN_YOUBANNED, name, Warhead::Time::ToTimeString<Seconds>(duration), reasonStr);
                else
                    handler->PSendSysMessage(LANG_BAN_YOUPERMBANNED, name, reasonStr);
                break;
            }
            case BAN_NOTFOUND:
            {
                handler->PSendSysMessage(LANG_BAN_NOTFOUND, "character", name);
                handler->SetSentErrorMessage(true);
                return false;
            }
            default:
                break;
        }

        return true;
    }

    static bool HandleBanAccountByCharCommand(ChatHandler* handler, char const* args)
    {
        return HandleBanHelper(BAN_CHARACTER, args, handler);
    }

    static bool HandleBanIPCommand(ChatHandler* handler, char const* args)
    {
        return HandleBanHelper(BAN_IP, args, handler);
    }

    static bool HandleBanHelper(BanMode mode, char const* args, ChatHandler* handler)
    {
        if (!*args)
            return false;

        char* cnameOrIP = strtok((char*)args, " ");
        if (!cnameOrIP)
            return false;

        std::string nameOrIP = cnameOrIP;

        char* durationStr = strtok(nullptr, " ");
        if (!durationStr || !atoi(durationStr))
            return false;

        char* reasonStr = strtok(nullptr, "");
        if (!reasonStr)
            return false;

        switch (mode)
        {
            case BAN_ACCOUNT:
                if (!Utf8ToUpperOnlyLatin(nameOrIP))
                {
                    handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, nameOrIP);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_CHARACTER:
                if (!normalizePlayerName(nameOrIP))
                {
                    handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_IP:
                if (!IsIPAddress(nameOrIP.c_str()))
                    return false;
                break;
        }

        std::string author = handler->GetSession() ? handler->GetSession()->GetPlayerName() : "Server";
        BanReturn banReturn;

        switch (mode)
        {
            case BAN_ACCOUNT:
                banReturn = sBan->BanAccount(nameOrIP, durationStr, reasonStr, author);
                break;
            case BAN_CHARACTER:
                banReturn = sBan->BanAccountByPlayerName(nameOrIP, durationStr, reasonStr, author);
                break;
            case BAN_IP:
                banReturn = sBan->BanIP(nameOrIP, durationStr, reasonStr, author);
                break;
            default:
                return false;
        }

        uint32 duration = Warhead::Time::TimeStringTo<Seconds>(durationStr);

        switch (banReturn)
        {
            case BAN_SUCCESS:
                if (CONF_GET_BOOL("ShowBanInWorld"))
                    break;

                if (duration)
                    handler->PSendSysMessage(LANG_BAN_YOUBANNED, nameOrIP, Warhead::Time::ToTimeString<Seconds>(duration), reasonStr);
                else
                    handler->PSendSysMessage(LANG_BAN_YOUPERMBANNED, nameOrIP, reasonStr);
                break;
            case BAN_SYNTAX_ERROR:
                return false;
            case BAN_NOTFOUND:
                switch (mode)
                {
                    default:
                        handler->PSendSysMessage(LANG_BAN_NOTFOUND, "account", nameOrIP);
                        break;
                    case BAN_CHARACTER:
                        handler->PSendSysMessage(LANG_BAN_NOTFOUND, "character", nameOrIP);
                        break;
                    case BAN_IP:
                        handler->PSendSysMessage(LANG_BAN_NOTFOUND, "ip", nameOrIP);
                        break;
                }
                handler->SetSentErrorMessage(true);
                return false;
            case BAN_LONGER_EXISTS:
                handler->PSendSysMessage("Unsuccessful! A longer ban is already present on this account!");
                break;
            default:
                break;
        }

        return true;
    }

    static bool HandleBanInfoAccountCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* nameStr = strtok((char*)args, "");
        if (!nameStr)
            return false;

        std::string accountName = nameStr;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            return true;
        }

        return HandleBanInfoHelper(accountId, accountName.c_str(), handler);
    }

    static bool HandleBanInfoHelper(uint32 accountId, char const* accountName, ChatHandler* handler)
    {
        QueryResult result = LoginDatabase.PQuery("SELECT FROM_UNIXTIME(bandate, '%Y-%m-%{}..%H:%I:%{}') as bandate, unbandate-bandate, active, unbandate, banreason, bannedby FROM account_banned WHERE id = '{}' ORDER BY bandate ASC", accountId);
        if (!result)
        {
            handler->PSendSysMessage(LANG_BANINFO_NOACCOUNTBAN, accountName);
            return true;
        }

        handler->PSendSysMessage(LANG_BANINFO_BANHISTORY, accountName);

        do
        {
            Field* fields = result->Fetch();

            time_t unbanDate = time_t(fields[3].GetUInt32());
            bool active = false;

            if (fields[2].GetBool() && (fields[1].GetUInt64() == uint64(0) || unbanDate >= GameTime::GetGameTime().count()))
                active = true;

            bool permanent = (fields[1].GetUInt64() == uint64(0));
            std::string banTime = permanent ? handler->GetWarheadString(LANG_BANINFO_INFINITE) : Warhead::Time::ToTimeString<Seconds>(fields[1].GetUInt64());
            handler->PSendSysMessage(LANG_BANINFO_HISTORYENTRY,
                                     fields[0].GetCString(), banTime, active ? handler->GetWarheadString(LANG_YES) : handler->GetWarheadString(LANG_NO), fields[4].GetCString(), fields[5].GetCString());
        } while (result->NextRow());

        return true;
    }

    static bool HandleBanInfoCharacterCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target = ObjectAccessor::FindPlayerByName(args, false);
        ObjectGuid targetGuid;
        std::string name(args);

        if (!target)
        {
            targetGuid = sCharacterCache->GetCharacterGuidByName(name);
            if (!targetGuid)
            {
                handler->PSendSysMessage(LANG_BANINFO_NOCHARACTER);
                return false;
            }
        }
        else
            targetGuid = target->GetGUID();

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANINFO);
        stmt->SetData(0, targetGuid.GetCounter());

        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (!result)
        {
            handler->PSendSysMessage(LANG_CHAR_NOT_BANNED, name);
            return true;
        }

        handler->PSendSysMessage(LANG_BANINFO_BANHISTORY, name);
        do
        {
            Field* fields = result->Fetch();
            time_t unbanDate = time_t(fields[3].GetUInt32());
            bool active = false;
            if (fields[2].GetUInt8() && (!fields[1].GetUInt32() || unbanDate >= GameTime::GetGameTime().count()))
                active = true;
            bool permanent = (fields[1].GetUInt32() == uint32(0));
            std::string banTime = permanent ? handler->GetWarheadString(LANG_BANINFO_INFINITE) : Warhead::Time::ToTimeString<Seconds>(fields[1].GetUInt64());
            handler->PSendSysMessage(LANG_BANINFO_HISTORYENTRY,
                                     fields[0].GetCString(), banTime, active ? handler->GetWarheadString(LANG_YES) : handler->GetWarheadString(LANG_NO), fields[4].GetCString(), fields[5].GetCString());
        } while (result->NextRow());

        return true;
    }

    static bool HandleBanInfoIPCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* ipStr = strtok((char*)args, "");
        if (!ipStr)
            return false;

        if (!IsIPAddress(ipStr))
            return false;

        std::string IP = ipStr;

        LoginDatabase.EscapeString(IP);
        QueryResult result = LoginDatabase.PQuery("SELECT ip, FROM_UNIXTIME(bandate), FROM_UNIXTIME(unbandate), unbandate-UNIX_TIMESTAMP(), banreason, bannedby, unbandate-bandate FROM ip_banned WHERE ip = '{}'", IP);
        if (!result)
        {
            handler->PSendSysMessage(LANG_BANINFO_NOIP);
            return true;
        }

        Field* fields = result->Fetch();
        bool permanent = !fields[6].GetUInt64();

        handler->PSendSysMessage(LANG_BANINFO_IPENTRY, fields[0].GetCString(), fields[1].GetCString(),
            permanent ? handler->GetWarheadString(LANG_BANINFO_NEVER) : fields[2].GetCString(),
            permanent ? handler->GetWarheadString(LANG_BANINFO_INFINITE) : Warhead::Time::ToTimeString<Seconds>(fields[3].GetUInt64()),
            fields[4].GetCString(), fields[5].GetCString());

        return true;
    }

    static bool HandleBanListAccountCommand(ChatHandler* handler, char const* args)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS);
        LoginDatabase.Execute(stmt);

        char* filterStr = strtok((char*)args, " ");
        std::string filter = filterStr ? filterStr : "";

        PreparedQueryResult result;

        if (filter.empty())
        {
            LoginDatabasePreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED_ALL);
            result = LoginDatabase.Query(stmt2);
        }
        else
        {
            LoginDatabasePreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BANNED_BY_USERNAME);
            stmt2->SetData(0, filter);
            result = LoginDatabase.Query(stmt2);
        }

        if (!result)
        {
            handler->PSendSysMessage(LANG_BANLIST_NOACCOUNT);
            return true;
        }

        return HandleBanListHelper(result, handler);
    }

    static bool HandleBanListHelper(PreparedQueryResult result, ChatHandler* handler)
    {
        handler->PSendSysMessage(LANG_BANLIST_MATCHINGACCOUNT);

        // Chat short output
        if (handler->GetSession())
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 accountid = fields[0].GetUInt32();

                QueryResult banResult = LoginDatabase.PQuery("SELECT account.username FROM account, account_banned WHERE account_banned.id='{}' AND account_banned.id=account.id", accountid);
                if (banResult)
                {
                    Field* fields2 = banResult->Fetch();
                    handler->PSendSysMessage("{}", fields2[0].GetCString());
                }
            } while (result->NextRow());
        }
        // Console wide output
        else
        {
            handler->SendSysMessage(LANG_BANLIST_ACCOUNTS);
            handler->SendSysMessage(" ===============================================================================");
            handler->SendSysMessage(LANG_BANLIST_ACCOUNTS_HEADER);
            do
            {
                handler->SendSysMessage("-------------------------------------------------------------------------------");
                Field* fields = result->Fetch();
                uint32 accountId = fields[0].GetUInt32();

                std::string accountName;

                // "account" case, name can be get in same query
                if (result->GetFieldCount() > 1)
                    accountName = fields[1].GetString();
                // "character" case, name need extract from another DB
                else
                    AccountMgr::GetName(accountId, accountName);

                // No SQL injection. id is uint32.
                QueryResult banInfo = LoginDatabase.PQuery("SELECT bandate, unbandate, bannedby, banreason FROM account_banned WHERE id = {} ORDER BY unbandate", accountId);
                if (banInfo)
                {
                    Field* fields2 = banInfo->Fetch();
                    do
                    {
                        time_t timeBan = time_t(fields2[0].GetUInt32());
                        tm tmBan;
                        localtime_r(&timeBan, &tmBan);

                        if (fields2[0].GetUInt32() == fields2[1].GetUInt32())
                        {
                            handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|   permanent  |%-15.15s|%-15.15s|",
                                                     accountName, tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                                     fields2[2].GetCString(), fields2[3].GetCString());
                        }
                        else
                        {
                            time_t timeUnban = time_t(fields2[1].GetUInt32());
                            tm tmUnban;
                            localtime_r(&timeUnban, &tmUnban);
                            handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|{:02}-{:02}-{:02} {:02}:{:02}|%-15.15s|%-15.15s|",
                                                     accountName, tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                                     tmUnban.tm_year % 100, tmUnban.tm_mon + 1, tmUnban.tm_mday, tmUnban.tm_hour, tmUnban.tm_min,
                                                     fields2[2].GetCString(), fields2[3].GetCString());
                        }
                    } while (banInfo->NextRow());
                }
            } while (result->NextRow());

            handler->SendSysMessage(" ===============================================================================");
        }

        return true;
    }

    static bool HandleBanListCharacterCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* filterStr = strtok((char*)args, " ");
        if (!filterStr)
            return false;

        std::string filter(filterStr);
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME_FILTER);
        stmt->SetData(0, filter);
        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (!result)
        {
            handler->PSendSysMessage(LANG_BANLIST_NOCHARACTER);
            return true;
        }

        handler->PSendSysMessage(LANG_BANLIST_MATCHINGCHARACTER);

        // Chat short output
        if (handler->GetSession())
        {
            do
            {
                Field* fields = result->Fetch();

                CharacterDatabasePreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANNED_NAME);
                stmt2->SetData(0, fields[0].GetUInt32());

                PreparedQueryResult banResult = CharacterDatabase.Query(stmt2);
                if (banResult)
                    handler->PSendSysMessage("{}", (*banResult)[0].GetCString());
            } while (result->NextRow());
        }
        // Console wide output
        else
        {
            handler->SendSysMessage(LANG_BANLIST_CHARACTERS);
            handler->SendSysMessage(" =============================================================================== ");
            handler->SendSysMessage(LANG_BANLIST_CHARACTERS_HEADER);
            do
            {
                handler->SendSysMessage("-------------------------------------------------------------------------------");

                Field* fields = result->Fetch();

                std::string char_name = fields[1].GetString();

                CharacterDatabasePreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BANINFO_LIST);
                stmt2->SetData(0, fields[0].GetUInt32());

                PreparedQueryResult banInfo = CharacterDatabase.Query(stmt2);
                if (banInfo)
                {
                    Field* banFields = banInfo->Fetch();
                    do
                    {
                        time_t timeBan = time_t(banFields[0].GetUInt32());
                        tm tmBan;
                        localtime_r(&timeBan, &tmBan);

                        if (banFields[0].GetUInt32() == banFields[1].GetUInt32())
                        {
                            handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|   permanent  |%-15.15s|%-15.15s|",
                                                     char_name, tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                                     banFields[2].GetCString(), banFields[3].GetCString());
                        }
                        else
                        {
                            time_t timeUnban = time_t(banFields[1].GetUInt32());
                            tm tmUnban;
                            localtime_r(&timeUnban, &tmUnban);
                            handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|{:02}-{:02}-{:02} {:02}:{:02}|%-15.15s|%-15.15s|",
                                                     char_name, tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                                     tmUnban.tm_year % 100, tmUnban.tm_mon + 1, tmUnban.tm_mday, tmUnban.tm_hour, tmUnban.tm_min,
                                                     banFields[2].GetCString(), banFields[3].GetCString());
                        }
                    } while (banInfo->NextRow());
                }
            } while (result->NextRow());
            handler->SendSysMessage(" =============================================================================== ");
        }

        return true;
    }

    static bool HandleBanListIPCommand(ChatHandler* handler, char const* args)
    {
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_EXPIRED_IP_BANS);
        LoginDatabase.Execute(stmt);

        char* filterStr = strtok((char*)args, " ");
        std::string filter = filterStr ? filterStr : "";
        LoginDatabase.EscapeString(filter);

        PreparedQueryResult result;

        if (filter.empty())
        {
            LoginDatabasePreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_IP_BANNED_ALL);
            result = LoginDatabase.Query(stmt2);
        }
        else
        {
            LoginDatabasePreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_IP_BANNED_BY_IP);
            stmt2->SetData(0, filter);
            result = LoginDatabase.Query(stmt2);
        }

        if (!result)
        {
            handler->PSendSysMessage(LANG_BANLIST_NOIP);
            return true;
        }

        handler->PSendSysMessage(LANG_BANLIST_MATCHINGIP);
        // Chat short output
        if (handler->GetSession())
        {
            do
            {
                Field* fields = result->Fetch();
                handler->PSendSysMessage("{}", fields[0].GetCString());
            } while (result->NextRow());
        }
        // Console wide output
        else
        {
            handler->SendSysMessage(LANG_BANLIST_IPS);
            handler->SendSysMessage(" ===============================================================================");
            handler->SendSysMessage(LANG_BANLIST_IPS_HEADER);
            do
            {
                handler->SendSysMessage("-------------------------------------------------------------------------------");
                Field* fields = result->Fetch();
                time_t timeBan = time_t(fields[1].GetUInt32());
                tm tmBan;
                localtime_r(&timeBan, &tmBan);
                if (fields[1].GetUInt32() == fields[2].GetUInt32())
                {
                    handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|   permanent  |%-15.15s|%-15.15s|",
                                             fields[0].GetCString(), tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                             fields[3].GetCString(), fields[4].GetCString());
                }
                else
                {
                    time_t timeUnban = time_t(fields[2].GetUInt32());
                    tm tmUnban;
                    localtime_r(&timeUnban, &tmUnban);
                    handler->PSendSysMessage("|%-15.15s|{:02}-{:02}-{:02} {:02}:{:02}|{:02}-{:02}-{:02} {:02}:{:02}|%-15.15s|%-15.15s|",
                                             fields[0].GetCString(), tmBan.tm_year % 100, tmBan.tm_mon + 1, tmBan.tm_mday, tmBan.tm_hour, tmBan.tm_min,
                                             tmUnban.tm_year % 100, tmUnban.tm_mon + 1, tmUnban.tm_mday, tmUnban.tm_hour, tmUnban.tm_min,
                                             fields[3].GetCString(), fields[4].GetCString());
                }
            } while (result->NextRow());

            handler->SendSysMessage(" ===============================================================================");
        }

        return true;
    }

    static bool HandleUnBanAccountCommand(ChatHandler* handler, char const* args)
    {
        return HandleUnBanHelper(BAN_ACCOUNT, args, handler);
    }

    static bool HandleUnBanCharacterCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* nameStr = strtok((char*)args, " ");
        if (!nameStr)
            return false;

        std::string CharacterName = nameStr;

        if (!normalizePlayerName(CharacterName))
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!sBan->RemoveBanCharacter(CharacterName))
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandleUnBanAccountByCharCommand(ChatHandler* handler, char const* args)
    {
        return HandleUnBanHelper(BAN_CHARACTER, args, handler);
    }

    static bool HandleUnBanIPCommand(ChatHandler* handler, char const* args)
    {
        return HandleUnBanHelper(BAN_IP, args, handler);
    }

    static bool HandleUnBanHelper(BanMode mode, char const* args, ChatHandler* handler)
    {
        if (!*args)
            return false;

        char* nameOrIPStr = strtok((char*)args, " ");
        if (!nameOrIPStr)
            return false;

        std::string nameOrIP = nameOrIPStr;

        switch (mode)
        {
            case BAN_ACCOUNT:
                if (!Utf8ToUpperOnlyLatin(nameOrIP))
                {
                    handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, nameOrIP);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_CHARACTER:
                if (!normalizePlayerName(nameOrIP))
                {
                    handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_IP:
                if (!IsIPAddress(nameOrIP.c_str()))
                    return false;
                break;
        }

        switch (mode)
        {
            case BAN_ACCOUNT:
                if (sBan->RemoveBanAccount(nameOrIP))
                    handler->PSendSysMessage(LANG_UNBAN_UNBANNED, nameOrIP);
                else
                    handler->PSendSysMessage(LANG_UNBAN_ERROR, nameOrIP);
                break;
            case BAN_CHARACTER:
                if (sBan->RemoveBanAccountByPlayerName(nameOrIP))
                    handler->PSendSysMessage(LANG_UNBAN_UNBANNED, nameOrIP);
                else
                    handler->PSendSysMessage(LANG_UNBAN_ERROR, nameOrIP);
                break;
            case BAN_IP:
                if (sBan->RemoveBanIP(nameOrIP))
                    handler->PSendSysMessage(LANG_UNBAN_UNBANNED, nameOrIP);
                else
                    handler->PSendSysMessage(LANG_UNBAN_ERROR, nameOrIP);
                break;
            default:
                break;
        }

        return true;
    }
};

void AddSC_ban_commandscript()
{
    new ban_commandscript();
}
