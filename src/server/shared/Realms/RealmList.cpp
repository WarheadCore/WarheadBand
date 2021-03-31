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

#include "Common.h"
#include "DatabaseEnv.h"
#include "RealmList.h"
#include "Log.h"

RealmList::RealmList() :  m_NextUpdateTime(time(nullptr)) { }

RealmList* RealmList::instance()
{
    static RealmList instance;
    return &instance;
}

// Load the realm list from the database
void RealmList::Initialize(uint32 updateInterval)
{
    m_UpdateInterval = updateInterval;

    // Get the content of the realmlist table in the database
    UpdateRealms(true);
}

void RealmList::UpdateRealm(uint32 id, const std::string& name, ACE_INET_Addr const& address, ACE_INET_Addr const& localAddr, ACE_INET_Addr const& localSubmask, uint8 icon, RealmFlags flag, uint8 timezone, AccountTypes allowedSecurityLevel, float popu, uint32 build)
{
    // Create new if not exist or update existed
    Realm& realm = m_realms[name];

    realm.m_ID = id;
    realm.name = name;
    realm.icon = icon;
    realm.flag = flag;
    realm.timezone = timezone;
    realm.allowedSecurityLevel = allowedSecurityLevel;
    realm.populationLevel = popu;

    // Append port to IP address.
    realm.ExternalAddress = address;
    realm.LocalAddress = localAddr;
    realm.LocalSubnetMask = localSubmask;
    realm.gamebuild = build;
}

void RealmList::UpdateIfNeed()
{
    // maybe disabled or updated recently
    if (!m_UpdateInterval || m_NextUpdateTime > time(nullptr))
        return;

    m_NextUpdateTime = time(nullptr) + m_UpdateInterval;

    // Clears Realm list
    m_realms.clear();

    // Get the content of the realmlist table in the database
    UpdateRealms();
}

void RealmList::UpdateRealms(bool init)
{
    LOG_INFO("server", "Updating Realm List...");

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST);
    PreparedQueryResult result = LoginDatabase.Query(stmt);

    // Circle through results and add them to the realm map
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 realmId              = fields[0].GetUInt32();
            std::string name            = fields[1].GetString();
            std::string externalAddress = fields[2].GetString();
            std::string localAddress    = fields[3].GetString();
            std::string localSubmask    = fields[4].GetString();
            uint16 port                 = fields[5].GetUInt16();
            uint8 icon                  = fields[6].GetUInt8();
            RealmFlags flag             = RealmFlags(fields[7].GetUInt8());
            uint8 timezone              = fields[8].GetUInt8();
            uint8 allowedSecurityLevel  = fields[9].GetUInt8();
            float pop                   = fields[10].GetFloat();
            uint32 build                = fields[11].GetUInt32();

            ACE_INET_Addr externalAddr(port, externalAddress.c_str(), AF_INET);
            ACE_INET_Addr localAddr(port, localAddress.c_str(), AF_INET);
            ACE_INET_Addr submask(0, localSubmask.c_str(), AF_INET);

            UpdateRealm(realmId, name, externalAddr, localAddr, submask, icon, flag, timezone, (allowedSecurityLevel <= SEC_ADMINISTRATOR ? AccountTypes(allowedSecurityLevel) : SEC_ADMINISTRATOR), pop, build);

            if (init)
                LOG_INFO("server", "Added realm \"%s\" at %s:%u.", name.c_str(), m_realms[name].ExternalAddress.get_host_addr(), port);
        } while (result->NextRow());
    }
}
