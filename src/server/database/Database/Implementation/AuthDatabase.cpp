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

#include "AuthDatabase.h"

AuthDatabasePool AuthDatabase;

void AuthDatabasePool::DoPrepareStatements()
{
    PrepareStatement(LOGIN_SEL_LOGONCHALLENGE,
                     "SELECT a.id, a.username, a.locked, a.lock_country, a.last_ip, a.failed_logins, "
                     "ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, ab.unbandate = ab.bandate, "
                     "ipb.unbandate > UNIX_TIMESTAMP() OR ipb.unbandate = ipb.bandate, ipb.unbandate = ipb.bandate, "
                     "aa.gmlevel, a.totp_secret, a.salt, a.verifier "
                     "FROM account a "
                     "LEFT JOIN account_access aa ON a.id = aa.id "
                     "LEFT JOIN account_banned ab ON ab.id = a.id AND ab.active = 1 "
                     "LEFT JOIN ip_banned ipb ON ipb.ip = ? "
                     "WHERE a.username = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_RECONNECTCHALLENGE,
                     "SELECT a.id, a.username, a.locked, a.lock_country, a.last_ip, a.failed_logins, "
                     "ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, ab.unbandate = ab.bandate, "
                     "ipb.unbandate > UNIX_TIMESTAMP() OR ipb.unbandate = ipb.bandate, ipb.unbandate = ipb.bandate, "
                     "aa.gmlevel, a.session_key "
                     "FROM account a "
                     "LEFT JOIN account_access aa ON a.id = aa.id "
                     "LEFT JOIN account_banned ab ON ab.id = a.id AND ab.active = 1 "
                     "LEFT JOIN ip_banned ipb ON ipb.ip = ? "
                     "WHERE a.username = ? AND a.session_key IS NOT NULL", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME, "SELECT a.id, a.session_key, a.last_ip, a.locked, a.lock_country, a.expansion, a.locale, a.recruiter, a.os, a.totaltime, "
                     "aa.gmlevel, ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, r.id FROM account a LEFT JOIN account_access aa ON a.id = aa.id AND aa.RealmID IN (-1, ?) "
                     "LEFT JOIN account_banned ab ON a.id = ab.id AND ab.active = 1 LEFT JOIN account r ON a.id = r.recruiter WHERE a.username = ? "
                     "AND a.session_key IS NOT NULL ORDER BY aa.RealmID DESC LIMIT 1", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_IP_INFO, "SELECT unbandate > UNIX_TIMESTAMP() OR unbandate = bandate AS banned, NULL as country FROM ip_banned WHERE ip = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_REALMLIST, "SELECT id, name, address, localAddress, localSubnetMask, port, icon, flag, timezone, allowedSecurityLevel, population, gamebuild FROM realmlist WHERE flag <> 3 ORDER BY name", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_DEL_EXPIRED_IP_BANS, "DELETE FROM ip_banned WHERE unbandate<>bandate AND unbandate<=UNIX_TIMESTAMP()", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_EXPIRED_ACCOUNT_BANS, "UPDATE account_banned SET active = 0 WHERE active = 1 AND unbandate<>bandate AND unbandate<=UNIX_TIMESTAMP()", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_IP_BANNED, "SELECT * FROM ip_banned WHERE ip = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_INS_IP_AUTO_BANNED, "INSERT INTO ip_banned (ip, bandate, unbandate, bannedby, banreason) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, 'Trinity realmd', 'Failed login autoban')", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_IP_BANNED_ALL, "SELECT ip, bandate, unbandate, bannedby, banreason FROM ip_banned WHERE (bandate = unbandate OR unbandate > UNIX_TIMESTAMP()) ORDER BY unbandate", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_IP_BANNED_BY_IP, "SELECT ip, bandate, unbandate, bannedby, banreason FROM ip_banned WHERE (bandate = unbandate OR unbandate > UNIX_TIMESTAMP()) AND ip LIKE CONCAT('%%', ?, '%%') ORDER BY unbandate", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED, "SELECT bandate, unbandate FROM account_banned WHERE id = ? AND active = 1", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED_ALL, "SELECT account.id, username FROM account, account_banned WHERE account.id = account_banned.id AND active = 1 GROUP BY account.id", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BANNED_BY_USERNAME, "SELECT account.id, username FROM account, account_banned WHERE account.id = account_banned.id AND active = 1 AND username LIKE CONCAT('%%', ?, '%%') GROUP BY account.id", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_INS_ACCOUNT_AUTO_BANNED, "INSERT INTO account_banned VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, 'Trinity realmd', 'Failed login autoban', 1)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_ACCOUNT_BANNED, "DELETE FROM account_banned WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_LOGON, "UPDATE account SET salt = ?, verifier = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_LOGONPROOF, "UPDATE account SET session_key = ?, last_ip = ?, last_login = NOW(), locale = ?, failed_logins = 0, os = ? WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_UPD_FAILEDLOGINS, "UPDATE account SET failed_logins = failed_logins + 1 WHERE username = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_FAILEDLOGINS, "SELECT id, failed_logins FROM account WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME, "SELECT id FROM account WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_LIST_BY_NAME, "SELECT id, username FROM account WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_LIST_BY_EMAIL, "SELECT id, username FROM account WHERE email = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_NUM_CHARS_ON_REALM, "SELECT numchars FROM realmcharacters WHERE realmid = ? AND acctid= ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_REALM_CHARACTER_COUNTS, "SELECT realmid, numchars FROM realmcharacters WHERE acctid = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BY_IP, "SELECT id, username FROM account WHERE last_ip = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_BY_ID, "SELECT 1 FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_INS_IP_BANNED, "INSERT INTO ip_banned (ip, bandate, unbandate, bannedby, banreason) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, ?, ?)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_IP_NOT_BANNED, "DELETE FROM ip_banned WHERE ip = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_INS_ACCOUNT_BANNED, "INSERT INTO account_banned VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+?, ?, ?, 1)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED, "UPDATE account_banned SET active = 0 WHERE id = ? AND active != 0", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_REALM_CHARACTERS, "DELETE FROM realmcharacters WHERE acctid = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_REP_REALM_CHARACTERS, "REPLACE INTO realmcharacters (numchars, acctid, realmid) VALUES (?, ?, ?)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_SUM_REALM_CHARACTERS, "SELECT SUM(numchars) FROM realmcharacters WHERE acctid = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_INS_ACCOUNT, "INSERT INTO account(username, salt, verifier, expansion, joindate) VALUES(?, ?, ?, ?, NOW())", ConnectionFlags::Async);
    PrepareStatement(LOGIN_INS_REALM_CHARACTERS_INIT, "INSERT INTO realmcharacters (realmid, acctid, numchars) SELECT realmlist.id, account.id, 0 FROM realmlist, account LEFT JOIN realmcharacters ON acctid=account.id WHERE acctid IS NULL", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_EXPANSION, "UPDATE account SET expansion = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_ACCOUNT_LOCK, "UPDATE account SET locked = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_ACCOUNT_LOCK_COUNTRY, "UPDATE account SET lock_country = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_USERNAME, "UPDATE account SET username = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_LAST_IP, "UPDATE account SET last_ip = ? WHERE username = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_LAST_ATTEMPT_IP, "UPDATE account SET last_attempt_ip = ? WHERE username = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_ACCOUNT_ONLINE, "UPDATE account SET online = ? WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_UPTIME_PLAYERS, "UPDATE uptime SET uptime = ?, maxplayers = ? WHERE realmid = ? AND starttime = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_OLD_LOGS, "DELETE FROM logs WHERE (time + ?) < ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_ACCOUNT_ACCESS, "DELETE FROM account_access WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM, "DELETE FROM account_access WHERE id = ? AND (RealmID = ? OR RealmID = -1)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_INS_ACCOUNT_ACCESS, "INSERT INTO account_access (id,gmlevel,RealmID) VALUES (?, ?, ?)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME, "SELECT id FROM account WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_GET_ACCOUNT_ACCESS_GMLEVEL, "SELECT gmlevel FROM account_access WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_GET_GMLEVEL_BY_REALMID, "SELECT gmlevel FROM account_access WHERE id = ? AND (RealmID = ? OR RealmID = -1)", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_GET_USERNAME_BY_ID, "SELECT username FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD, "SELECT salt, verifier FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_CHECK_PASSWORD_BY_NAME, "SELECT salt, verifier FROM account WHERE username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_PINFO, "SELECT a.username, aa.gmlevel, a.email, a.reg_mail, a.last_ip, DATE_FORMAT(a.last_login, '%Y-%m-%d %T'), a.failed_logins, a.locked, a.OS FROM account a LEFT JOIN account_access aa ON (a.id = aa.id AND (aa.RealmID = ? OR aa.RealmID = -1)) WHERE a.id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_PINFO_BANS, "SELECT unbandate, bandate = unbandate, bannedby, banreason FROM account_banned WHERE id = ? AND active ORDER BY bandate ASC LIMIT 1", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_GM_ACCOUNTS, "SELECT a.username, aa.gmlevel FROM account a, account_access aa WHERE a.id=aa.id AND aa.gmlevel >= ? AND (aa.realmid = -1 OR aa.realmid = ?)", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_INFO, "SELECT a.username, a.last_ip, aa.gmlevel, a.expansion FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE a.id = ? ORDER BY a.last_ip", ConnectionFlags::Sync); // Only used in ".account onlinelist" command
    PrepareStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST, "SELECT 1 FROM account_access WHERE id = ? AND gmlevel > ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_ACCESS, "SELECT a.id, aa.gmlevel, aa.RealmID FROM account a LEFT JOIN account_access aa ON (a.id = aa.id) WHERE a.username = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_RECRUITER, "SELECT 1 FROM account WHERE recruiter = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_BANS, "SELECT 1 FROM account_banned WHERE id = ? AND active = 1 UNION SELECT 1 FROM ip_banned WHERE ip = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_WHOIS, "SELECT username, email, last_ip FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_LAST_ATTEMPT_IP, "SELECT last_attempt_ip FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_LAST_IP, "SELECT last_ip FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_REALMLIST_SECURITY_LEVEL, "SELECT allowedSecurityLevel from realmlist WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_DEL_ACCOUNT, "DELETE FROM account WHERE id = ?", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_AUTOBROADCAST, "SELECT id, weight, text FROM autobroadcast WHERE realmid = ? OR realmid = -1", ConnectionFlags::Sync);

    // 0: uint32, 1: uint32, 2: uint8, 3: uint32, 4: string // Complete name: "Login_Insert_AccountLoginDelete_IP_Logging"
    PrepareStatement(LOGIN_INS_ALDL_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, (SELECT last_ip FROM account WHERE id = ?), ?, unix_timestamp(NOW()), NOW())", ConnectionFlags::Async);
    // 0: uint32, 1: uint32, 2: uint8, 3: uint32, 4: string // Complete name: "Login_Insert_FailedAccountLogin_IP_Logging"
    PrepareStatement(LOGIN_INS_FACL_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, (SELECT last_attempt_ip FROM account WHERE id = ?), ?, unix_timestamp(NOW()), NOW())", ConnectionFlags::Async);
    // 0: uint32, 1: uint32, 2: uint8, 3: string, 4: string // Complete name: "Login_Insert_CharacterDelete_IP_Logging"
    PrepareStatement(LOGIN_INS_CHAR_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES (?, ?, ?, ?, ?, unix_timestamp(NOW()), NOW())", ConnectionFlags::Async);
    // 0: string, 1: string, 2: string                      // Complete name: "Login_Insert_Failed_Account_Login_due_password_IP_Logging"
    PrepareStatement(LOGIN_INS_FALP_IP_LOGGING, "INSERT INTO logs_ip_actions (account_id,character_guid,type,ip,systemnote,unixtime,time) VALUES ((SELECT id FROM account WHERE username = ?), 0, 1, ?, ?, unix_timestamp(NOW()), NOW())", ConnectionFlags::Async);

    // DB logging
    PrepareStatement(LOGIN_INS_LOG, "INSERT INTO logs (time, realm, type, level, string) VALUES (?, ?, ?, ?, ?)", ConnectionFlags::Async);

    // TOTP
    PrepareStatement(LOGIN_SEL_SECRET_DIGEST, "SELECT digest FROM secret_digest WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_INS_SECRET_DIGEST, "INSERT INTO secret_digest (id, digest) VALUES (?,?)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_SECRET_DIGEST, "DELETE FROM secret_digest WHERE id = ?", ConnectionFlags::Async);

    PrepareStatement(LOGIN_SEL_ACCOUNT_TOTP_SECRET, "SELECT totp_secret FROM account WHERE id = ?", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_UPD_ACCOUNT_TOTP_SECRET, "UPDATE account SET totp_secret = ? WHERE id = ?", ConnectionFlags::Async);

    // Mute system
    PrepareStatement(LOGIN_INS_ACCOUNT_MUTE, "INSERT INTO `account_muted` (`accountid`, `mutedate`, `mutetime`, `mutedby`, `mutereason`, `active`) VALUES (?, ?, ?, ?, ?, 1)", ConnectionFlags::Async);
    PrepareStatement(LOGIN_UPD_ACCOUNT_MUTE_DATE, "UPDATE `account_muted` SET `mutedate` = ? WHERE `accountid` = ? AND `active` = 1", ConnectionFlags::Async);
    PrepareStatement(LOGIN_SEL_ACCOUNT_MUTE_INFO, "SELECT `mutedate`, `mutetime`, `mutereason`, `mutedby` FROM `account_muted` WHERE `accountid` = ? ORDER BY `mutedate` ASC", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_SEL_ACCOUNT_MUTE, "SELECT `mutedate`, `mutetime`, `mutereason`, `mutedby` FROM `account_muted` WHERE `accountid` = ? AND `active` = 1 ORDER BY `mutedate` + `mutetime` DESC LIMIT 1", ConnectionFlags::Sync);
    PrepareStatement(LOGIN_UPD_ACCOUNT_MUTE_EXPIRED, "UPDATE `account_muted` SET `active` = 0 WHERE `active` = 1 AND `mutetime` > 0 AND mutedate > 0 AND `accountid` = ? AND UNIX_TIMESTAMP() >= `mutedate` + `mutetime`", ConnectionFlags::Async);
    PrepareStatement(LOGIN_DEL_ACCOUNT_MUTE, "UPDATE `account_muted` SET `active` = 0 WHERE `active` = 1 AND `accountid` = ?", ConnectionFlags::Async);

    // DDOS protection
    PrepareStatement(LOGIN_INS_DDOS_PROTECTION, "INSERT IGNORE INTO `ddos_protection` (`IP`, `BanDate`, `UnBanDate`, `Reason`) VALUES (?, UNIX_TIMESTAMP(), UNIX_TIMESTAMP() + ?, ?)", ConnectionFlags::Async);
}
