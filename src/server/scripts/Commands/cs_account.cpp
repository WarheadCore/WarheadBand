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
Name: account_commandscript
%Complete: 100
Comment: All account related commands
Category: commandscripts
EndScriptData */

#include "AES.h"
#include "AccountMgr.h"
#include "Base32.h"
#include "Chat.h"
#include "CryptoGenerics.h"
#include "GameConfig.h"
#include "IPLocation.h"
#include "Language.h"
#include "Player.h"
#include "Realm.h"
#include "ScriptObject.h"
#include "ScriptMgr.h"
#include "SecretMgr.h"
#include "StringConvert.h"
#include "TOTP.h"
#include <openssl/rand.h>
#include <unordered_map>

#if WARHEAD_COMPILER == WARHEAD_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Warhead::ChatCommands;

class account_commandscript : public CommandScript
{
public:
    account_commandscript() : CommandScript("account_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable accountSetCommandTable =
        {
            { "addon",      SEC_GAMEMASTER,     true,   &HandleAccountSetAddonCommand,          "" },
            { "gmlevel",    SEC_CONSOLE,        true,   &HandleAccountSetGmLevelCommand,        "" },
            { "password",   SEC_CONSOLE,        true,   &HandleAccountSetPasswordCommand,       "" },
            { "2fa",        SEC_PLAYER,         true,   &HandleAccountSet2FACommand,            "" }
        };

        static ChatCommandTable accountLockCommandTable
        {
            { "country",    SEC_PLAYER,         true,   &HandleAccountLockCountryCommand,       "" },
            { "ip",         SEC_PLAYER,         true,   &HandleAccountLockIpCommand,            "" }
        };

        static ChatCommandTable account2faCommandTable
        {
            { "setup",      SEC_PLAYER,         false,  &HandleAccount2FASetupCommand,          "" },
            { "remove",     SEC_PLAYER,         false,  &HandleAccount2FARemoveCommand,         "" },
        };

        static ChatCommandTable accountRemoveCommandTable
        {
            { "2fa",        SEC_PLAYER,         true,   nullptr, "", account2faCommandTable    },
            { "addon",      SEC_MODERATOR,      false,  &HandleAccountAddonCommand,         "" },
            { "create",     SEC_CONSOLE,        true,   &HandleAccountCreateCommand,        "" },
            { "delete",     SEC_CONSOLE,        true,   &HandleAccountDeleteCommand,        "" },
            { "onlinelist", SEC_CONSOLE,        true,   &HandleAccountOnlineListCommand,    "" },
            { "lock",       SEC_PLAYER,         false,  nullptr, "", accountLockCommandTable   },
            { "set",        SEC_ADMINISTRATOR,  true,   nullptr, "", accountSetCommandTable    },
            { "password",   SEC_PLAYER,         false,  &HandleAccountPasswordCommand,      "" }
        };

        static ChatCommandTable accountCommandTable =
        {
            { "2fa",        SEC_PLAYER,         true,   nullptr, "", account2faCommandTable        },
            { "addon",      SEC_MODERATOR,      false,  &HandleAccountAddonCommand,             "" },
            { "create",     SEC_CONSOLE,        true,   &HandleAccountCreateCommand,            "" },
            { "delete",     SEC_CONSOLE,        true,   &HandleAccountDeleteCommand,            "" },
            { "onlinelist", SEC_CONSOLE,        true,   &HandleAccountOnlineListCommand,        "" },
            { "lock",       SEC_PLAYER,         false,  nullptr, "", accountLockCommandTable       },
            { "set",        SEC_ADMINISTRATOR,  true,   nullptr, "", accountSetCommandTable        },
            { "password",   SEC_PLAYER,         false,  &HandleAccountPasswordCommand,          "" },
            { "remove",     SEC_ADMINISTRATOR,  true,   nullptr, "", accountRemoveCommandTable     },
            { "info",       SEC_PLAYER,         false,  &HandleAccountInfoCommand,              "" }
        };

        static ChatCommandTable commandTable =
        {
            { "account", SEC_PLAYER, true, nullptr, "", accountCommandTable }
        };

        return commandTable;
    }

    static bool HandleAccount2FASetupCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto token = Warhead::StringTo<uint32>(args);

        auto const& masterKey = sSecretMgr->GetSecret(SECRET_TOTP_MASTER_KEY);
        if (!masterKey.IsAvailable())
        {
            handler->SendSysMessage(LANG_2FA_COMMANDS_NOT_SETUP);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 const accountId = handler->GetSession()->GetAccountId();

        { // check if 2FA already enabled
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_TOTP_SECRET);
            stmt->SetData(0, accountId);
            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (!result)
            {
                LOG_ERROR("misc", "Account {} not found in login database when processing .account 2fa setup command.", accountId);
                handler->SendSysMessage(LANG_UNKNOWN_ERROR);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (!result->Fetch()->IsNull())
            {
                handler->SendSysMessage(LANG_2FA_ALREADY_SETUP);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // store random suggested secrets
        static std::unordered_map<uint32, Warhead::Crypto::TOTP::Secret> suggestions;
        auto pair = suggestions.emplace(std::piecewise_construct, std::make_tuple(accountId), std::make_tuple(Warhead::Crypto::TOTP::RECOMMENDED_SECRET_LENGTH)); // std::vector 1-argument size_t constructor invokes resize

        if (pair.second) // no suggestion yet, generate random secret
            Warhead::Crypto::GetRandomBytes(pair.first->second);

        if (!pair.second && token) // suggestion already existed and token specified - validate
        {
            if (Warhead::Crypto::TOTP::ValidateToken(pair.first->second, *token))
            {
                if (masterKey)
                    Warhead::Crypto::AEEncryptWithRandomIV<Warhead::Crypto::AES>(pair.first->second, *masterKey);

                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_TOTP_SECRET);
                stmt->SetData(0, pair.first->second);
                stmt->SetData(1, accountId);
                LoginDatabase.Execute(stmt);

                suggestions.erase(pair.first);
                handler->SendSysMessage(LANG_2FA_SETUP_COMPLETE);
                return true;
            }
            else
                handler->SendSysMessage(LANG_2FA_INVALID_TOKEN);
        }

        // new suggestion, or no token specified, output TOTP parameters
        handler->PSendSysMessage(LANG_2FA_SECRET_SUGGESTION, Warhead::Encoding::Base32::Encode(pair.first->second));
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccount2FARemoveCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto token = Warhead::StringTo<uint32>(args);

        auto const& masterKey = sSecretMgr->GetSecret(SECRET_TOTP_MASTER_KEY);
        if (!masterKey.IsAvailable())
        {
            handler->SendSysMessage(LANG_2FA_COMMANDS_NOT_SETUP);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 const accountId = handler->GetSession()->GetAccountId();
        Warhead::Crypto::TOTP::Secret secret;
        { // get current TOTP secret
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_TOTP_SECRET);
            stmt->SetData(0, accountId);
            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (!result)
            {
                LOG_ERROR("misc", "Account {} not found in login database when processing .account 2fa setup command.", accountId);
                handler->SendSysMessage(LANG_UNKNOWN_ERROR);
                handler->SetSentErrorMessage(true);
                return false;
            }

            Field* field = result->Fetch();
            if (field->IsNull())
            { // 2FA not enabled
                handler->SendSysMessage(LANG_2FA_NOT_SETUP);
                handler->SetSentErrorMessage(true);
                return false;
            }

            secret = field->Get<Binary>();
        }

        if (token)
        {
            if (masterKey)
            {
                bool success = Warhead::Crypto::AEDecrypt<Warhead::Crypto::AES>(secret, *masterKey);
                if (!success)
                {
                    LOG_ERROR("misc", "Account {} has invalid ciphertext in TOTP token.", accountId);
                    handler->SendSysMessage(LANG_UNKNOWN_ERROR);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }

            if (Warhead::Crypto::TOTP::ValidateToken(secret, *token))
            {
                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_TOTP_SECRET);
                stmt->SetData(0);
                stmt->SetData(1, accountId);
                LoginDatabase.Execute(stmt);
                handler->SendSysMessage(LANG_2FA_REMOVE_COMPLETE);
                return true;
            }
            else
                handler->SendSysMessage(LANG_2FA_INVALID_TOKEN);
        }

        handler->SendSysMessage(LANG_2FA_REMOVE_NEED_TOKEN);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountAddonCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* exp = strtok((char*)args, " ");

        uint32 accountId = handler->GetSession()->GetAccountId();

        auto expansion = Warhead::StringTo<uint8>(exp); //get int anyway (0 if error)
        if (!expansion || *expansion > CONF_GET_INT("Expansion"))
        {
            handler->SendSysMessage(LANG_IMPROPER_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->SetData(0, *expansion);
        stmt->SetData(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_ADDON, *expansion);
        return true;
    }

    /// Create an account
    static bool HandleAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- %Parse the command line arguments
        char* accountName = strtok((char*)args, " ");
        char* password = strtok(nullptr, " ");
        if (!accountName || !password)
            return false;

        AccountOpResult result = AccountMgr::CreateAccount(std::string(accountName), std::string(password));
        switch (result)
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
                if (handler->GetSession())
                {
                    LOG_DEBUG("warden", "Account: {} (IP: {}) Character:[{}] ({}) Change Password.",
                                   handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress(),
                                   handler->GetSession()->GetPlayer()->GetName(), handler->GetSession()->GetPlayer()->GetGUID().ToString());
                }
                break;
            case AOR_NAME_TOO_LONG:
                handler->SendSysMessage(LANG_ACCOUNT_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_ACCOUNT_PASS_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_NAME_ALREADY_EXIST:
                handler->SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Delete a user account and all associated characters in this realm
    /// \todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm then delete account)
    static bool HandleAccountDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char* account = strtok((char*)args, " ");
        if (!account)
            return false;

        std::string accountName = account;
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
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// Commands not recommended call from chat, but support anyway
        /// can delete only for account with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(nullptr, accountId, true))
            return false;

        AccountOpResult result = AccountMgr::DeleteAccount(accountId);
        switch (result)
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_DELETED, accountName);
                break;
            case AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED, accountName);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Display info on users currently in the realm
    static bool HandleAccountOnlineListCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the list of accounts ID logged to the realm
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ONLINE);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (!result)
        {
            handler->SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
            return true;
        }

        ///- Display the list of account/characters online
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);

        ///- Cycle through accounts
        do
        {
            Field* fieldsDB = result->Fetch();
            std::string name = fieldsDB[0].Get<std::string>();
            uint32 account = fieldsDB[1].Get<uint32>();

            ///- Get the username, last IP and GM level of each account
            // No SQL injection. account is uint32.
            LoginDatabasePreparedStatement* loginStmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO);
            stmt->SetData(0, account);

            PreparedQueryResult resultLogin = LoginDatabase.Query(loginStmt);

            if (resultLogin)
            {
                Field* fieldsLogin = resultLogin->Fetch();
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_LINE,
                                         fieldsLogin[0].Get<std::string_view>(), name, fieldsLogin[1].Get<std::string_view>(),
                                         fieldsDB[2].Get<uint16>(), fieldsDB[3].Get<uint16>(), fieldsLogin[3].Get<uint8>(),
                                         fieldsLogin[2].Get<uint8>());
            }
            else
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_ERROR, name);
        } while (result->NextRow());

        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);
        return true;
    }

    static bool HandleAccountRemoveLockCountryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ///- %Parse the command line arguments
        char* _accountName = strtok((char*)args, " ");
        if (!_accountName)
            return false;

        std::string accountName = _accountName;
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
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_COUNTRY);
        stmt->SetData(0, "00");
        stmt->SetData(1, accountId);
        LoginDatabase.Execute(stmt);
        handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);

        return true;
    }

    static bool HandleAccountLockCountryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (!param.empty())
        {
            if (param == "on")
            {
                if (IpLocationRecord const* location = sIPLocation->GetLocationRecord(handler->GetSession()->GetRemoteAddress()))
                {
                    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_COUNTRY);
                    stmt->SetData(0, location->CountryCode);
                    stmt->SetData(1, handler->GetSession()->GetAccountId());
                    LoginDatabase.Execute(stmt);
                    handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
                }
                else
                {
                    handler->PSendSysMessage("No IP2Location information - account not locked");
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }
            else if (param == "off")
            {
                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_COUNTRY);
                stmt->SetData(0, "00");
                stmt->SetData(1, handler->GetSession()->GetAccountId());
                LoginDatabase.Execute(stmt);
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountLockIpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (!param.empty())
        {
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK);

            if (param == "on")
            {
                stmt->SetData(0, true);                                     // locked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
            }
            else if (param == "off")
            {
                stmt->SetData(0, false);                                    // unlocked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }

            stmt->SetData(1, handler->GetSession()->GetAccountId());

            LoginDatabase.Execute(stmt);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* oldPassword = strtok((char*)args, " ");
        char* newPassword = strtok(nullptr, " ");
        char* passwordConfirmation = strtok(nullptr, " ");

        if (!oldPassword || !newPassword || !passwordConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(oldPassword)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (strcmp(newPassword, passwordConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(handler->GetSession()->GetAccountId(), std::string(newPassword));
        switch (result)
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                sScriptMgr->OnPasswordChange(handler->GetSession()->GetAccountId());
                break;
            case AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    static bool HandleAccountSet2FACommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* _account = strtok((char*)args, " ");
        char* _secret = strtok(nullptr, " ");

        if (!_account || !_secret)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string accountName = _account;
        std::string secret = _secret;

        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurityAccount(nullptr, targetAccountId, true))
            return false;

        if (secret == "off")
        {
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_TOTP_SECRET);
            stmt->SetData(0);
            stmt->SetData(1, targetAccountId);
            LoginDatabase.Execute(stmt);
            handler->PSendSysMessage(LANG_2FA_REMOVE_COMPLETE);
            return true;
        }

        auto const& masterKey = sSecretMgr->GetSecret(SECRET_TOTP_MASTER_KEY);
        if (!masterKey.IsAvailable())
        {
            handler->SendSysMessage(LANG_2FA_COMMANDS_NOT_SETUP);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Optional<std::vector<uint8>> decoded = Warhead::Encoding::Base32::Decode(secret);
        if (!decoded)
        {
            handler->SendSysMessage(LANG_2FA_SECRET_INVALID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (128 < (decoded->size() + Warhead::Crypto::AES::IV_SIZE_BYTES + Warhead::Crypto::AES::TAG_SIZE_BYTES))
        {
            handler->SendSysMessage(LANG_2FA_SECRET_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (masterKey)
            Warhead::Crypto::AEEncryptWithRandomIV<Warhead::Crypto::AES>(*decoded, *masterKey);

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_TOTP_SECRET);
        stmt->SetData(0, *decoded);
        stmt->SetData(1, targetAccountId);
        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_2FA_SECRET_SET_COMPLETE, accountName);
        return true;
    }

    static bool HandleAccountInfoCommand(ChatHandler* handler)
    {
        AccountTypes gmLevel = handler->GetSession()->GetSecurity();
        handler->PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmLevel));
        return true;
    }

    /// Set/Unset the expansion level for an account
    static bool HandleAccountSetAddonCommand(ChatHandler* handler, char const* args)
    {
        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* exp = strtok(nullptr, " ");

        if (!account)
            return false;

        std::string accountName;
        uint32 accountId;

        if (!exp)
        {
            Player* player = handler->getSelectedPlayer();
            if (!player)
                return false;

            accountId = player->GetSession()->GetAccountId();
            AccountMgr::GetName(accountId, accountName);
            exp = account;
        }
        else
        {
            ///- Convert Account name to Upper Format
            accountName = account;
            if (!Utf8ToUpperOnlyLatin(accountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            }

            accountId = AccountMgr::GetId(accountName);
            if (!accountId)
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Let set addon state only for lesser (strong) security level
        // or to self account
        if (handler->GetSession() && handler->GetSession()->GetAccountId() != accountId &&
                handler->HasLowerSecurityAccount(nullptr, accountId, true))
            return false;

        auto expansion = Warhead::StringTo<uint8>(exp); //get int anyway (0 if error)
        if (!expansion || *expansion > CONF_GET_INT("Expansion"))
            return false;

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->SetData(0, *expansion);
        stmt->SetData(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_SETADDON, accountName, accountId, *expansion);
        return true;
    }

    static bool HandleAccountSetGmLevelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string targetAccountName;
        uint32 targetAccountId = 0;
        uint32 targetSecurity = 0;
        uint32 gm = 0;
        char* arg1 = strtok((char*)args, " ");
        char* arg2 = strtok(nullptr, " ");
        char* arg3 = strtok(nullptr, " ");
        bool isAccountNameGiven = true;

        if (arg1 && !arg3)
        {
            if (!handler->getSelectedPlayer())
                return false;
            isAccountNameGiven = false;
        }

        // Check for second parameter
        if (!isAccountNameGiven && !arg2)
            return false;

        // Check for account
        if (isAccountNameGiven)
        {
            targetAccountName = arg1;
            if (!Utf8ToUpperOnlyLatin(targetAccountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, targetAccountName);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check for invalid specified GM level.
        gm = (isAccountNameGiven) ? Warhead::StringTo<int32>(arg2).value_or(0) : Warhead::StringTo<int32>(arg1).value_or(0);
        if (gm > SEC_CONSOLE)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // handler->getSession() == nullptr only for console
        targetAccountId = (isAccountNameGiven) ? AccountMgr::GetId(targetAccountName) : handler->getSelectedPlayer()->GetSession()->GetAccountId();
        int32 gmRealmID = (isAccountNameGiven) ? Warhead::StringTo<int32>(arg3).value_or(0) : Warhead::StringTo<int32>(arg2).value_or(0);
        uint32 playerSecurity;
        if (handler->GetSession())
            playerSecurity = AccountMgr::GetSecurity(handler->GetSession()->GetAccountId(), gmRealmID);
        else
            playerSecurity = SEC_CONSOLE;

        // can set security level only for target with less security and to less security that we have
        // This is also reject self apply in fact
        targetSecurity = AccountMgr::GetSecurity(targetAccountId, gmRealmID);
        if (targetSecurity >= playerSecurity || gm >= playerSecurity)
        {
            handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Check and abort if the target gm has a higher rank on one of the realms and the new realm is -1
        if (gmRealmID == -1 && !AccountMgr::IsConsoleAccount(playerSecurity))
        {
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST);

            stmt->SetData(0, targetAccountId);
            stmt->SetData(1, uint8(gm));

            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (result)
            {
                handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check if provided realm.Id.Realm has a negative value other than -1
        if (gmRealmID < -1)
        {
            handler->SendSysMessage(LANG_INVALID_REALMID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // If gmRealmID is -1, delete all values for the account id, else, insert values for the specific realm.Id.Realm
        LoginDatabasePreparedStatement* stmt;

        if (gmRealmID == -1)
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS);
            stmt->SetData(0, targetAccountId);
        }
        else
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM);
            stmt->SetData(0, targetAccountId);
            stmt->SetData(1, realm.Id.Realm);
        }

        LoginDatabase.Execute(stmt);

        if (gm != 0)
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_ACCESS);

            stmt->SetData(0, targetAccountId);
            stmt->SetData(1, uint8(gm));
            stmt->SetData(2, gmRealmID);

            LoginDatabase.Execute(stmt);
        }

        handler->PSendSysMessage(LANG_YOU_CHANGE_SECURITY, targetAccountName, gm);
        return true;
    }

    /// Set password for account
    static bool HandleAccountSetPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* password = strtok(nullptr, " ");
        char* passwordConfirmation = strtok(nullptr, " ");

        if (!account || !password || !passwordConfirmation)
            return false;

        std::string accountName = account;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set password only for target with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(nullptr, targetAccountId, true))
            return false;

        if (strcmp(password, passwordConfirmation))
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(targetAccountId, password);

        switch (result)
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                break;
            case AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }
        return true;
    }
};

void AddSC_account_commandscript()
{
    new account_commandscript();
}
